#include "internet.h"
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE 512
#define SSID1_ADDR 0
#define PASSWORD1_ADDR 32
#define SSID2_ADDR 64
#define PASSWORD2_ADDR 96

WebServer server(80);

String storedSSID1, storedPassword1, storedSSID2, storedPassword2;
long distances[5] = {0, 0, 0, 0, 0};
bool motorStates[4] = {false, false, false, false};

void readCredentialsFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    char ssid1[32], password1[32], ssid2[32], password2[32];
    for (int i = 0; i < 32; i++) {
        ssid1[i] = EEPROM.read(SSID1_ADDR + i);
        password1[i] = EEPROM.read(PASSWORD1_ADDR + i);
        ssid2[i] = EEPROM.read(SSID2_ADDR + i);
        password2[i] = EEPROM.read(PASSWORD2_ADDR + i);
    }
    EEPROM.end();
    storedSSID1 = String(ssid1);
    storedPassword1 = String(password1);
    storedSSID2 = String(ssid2);
    storedPassword2 = String(password2);
}

void handleRoot() {
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }";
    html += "h1 { color: #333; }";
    html += ".status { margin-bottom: 20px; }";
    html += ".motor { display: inline-block; width: 20px; height: 20px; border-radius: 50%; margin-right: 10px; }";
    html += ".on { background-color: green; }";
    html += ".off { background-color: red; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Drink Machine Status</h1>";
    html += "<div id='status'></div>";
    html += "<br><a href='/config'>Configure Wi-Fi</a>";
    html += "<script>";
    html += "function updateStatus() {";
    html += "  fetch('/status')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      let statusHtml = '';";
    html += "      for (let i = 0; i < 4; i++) {";
    html += "        statusHtml += `<div class='status'>`;";
    html += "        statusHtml += `Drink ${i + 1}: ${data.distances[i]} cm<br>`;";
    html += "        statusHtml += `Motor ${i + 1}: <span class='motor ${data.motorStates[i] ? 'on' : 'off'}'></span>`;";
    html += "        statusHtml += `${data.motorStates[i] ? 'ON' : 'OFF'}</div>`;";
    html += "      }";
    html += "      statusHtml += `Glass level: ${data.distances[4]} cm`;";
    html += "      document.getElementById('status').innerHTML = statusHtml;";
    html += "    });";
    html += "}";
    html += "setInterval(updateStatus, 1000);";
    html += "updateStatus();";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleStatus() {
    StaticJsonDocument<200> doc;
    JsonArray distancesArray = doc.createNestedArray("distances");
    JsonArray motorStatesArray = doc.createNestedArray("motorStates");
    
    for (int i = 0; i < 5; i++) {
        distancesArray.add(getDistance(i));
    }
    for (int i = 0; i < 4; i++) {
        motorStatesArray.add(getMotorState(i));
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleConfig() {
    String html = "<html><body>";
    html += "<h1>Configure Wi-Fi</h1>";
    html += "<form method='POST' action='/saveconfig'>";
    html += "<h2>Primary Wi-Fi</h2>";
    html += "SSID: <input type='text' name='ssid1' value='" + storedSSID1 + "'><br>";
    html += "Password: <input type='password' name='password1'><br>";
    html += "<h2>Secondary Wi-Fi</h2>";
    html += "SSID: <input type='text' name='ssid2' value='" + storedSSID2 + "'><br>";
    html += "Password: <input type='password' name='password2'><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

void handleSaveConfig() {
    String ssid1 = server.arg("ssid1");
    String password1 = server.arg("password1");
    String ssid2 = server.arg("ssid2");
    String password2 = server.arg("password2");

    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < 32; i++) {
        EEPROM.write(SSID1_ADDR + i, ssid1[i]);
        EEPROM.write(PASSWORD1_ADDR + i, password1[i]);
        EEPROM.write(SSID2_ADDR + i, ssid2[i]);
        EEPROM.write(PASSWORD2_ADDR + i, password2[i]);
    }
    EEPROM.commit();
    EEPROM.end();

    String html = "<html><body>";
    html += "<h1>Configuration Saved</h1>";
    html += "Primary SSID: " + ssid1 + "<br>";
    html += "Secondary SSID: " + ssid2 + "<br>";
    html += "The configuration has been saved. Restart the device to connect to the new networks.";
    html += "<br><br><a href='/'>Back to Home</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setupWiFiAndServer() {
    readCredentialsFromEEPROM();

    bool connected = false;

    if (storedSSID1.length() > 0 && storedPassword1.length() > 0) {
        WiFi.begin(storedSSID1.c_str(), storedPassword1.c_str());
        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            Serial.println("Connected to primary WiFi");
        }
    }

    if (!connected && storedSSID2.length() > 0 && storedPassword2.length() > 0) {
        WiFi.begin(storedSSID2.c_str(), storedPassword2.c_str());
        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            Serial.println("Connected to secondary WiFi");
        }
    }

    if (connected) {
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Could not connect to WiFi. Starting AP mode.");
        WiFi.softAP("ESP32-DrinkMachine", "password");
    }

    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/saveconfig", HTTP_POST, handleSaveConfig);
    server.on("/status", handleStatus);
    server.begin();
}

void updateMotorState(int index, bool state) {
    if (index >= 0 && index < 4) {
        motorStates[index] = state;
    }
}

bool getMotorState(int index) {
    if (index >= 0 && index < 4) {
        return motorStates[index];
    }
    return false;
}

void handleClient() {
    server.handleClient();
}

void updateDistance(int index, long distance) {
    if (index >= 0 && index < 5) {
        distances[index] = distance;
    }
}

long getDistance(int index) {
    if (index >= 0 && index < 5) {
        return distances[index];
    }
    return 0;
}
