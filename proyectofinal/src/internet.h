#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>

WebServer server(80);

// Define las posiciones de memoria para SSID y contraseña
const int EEPROM_SIZE = 512;
const int SSID1_ADDR = 50;
const int PASSWORD1_ADDR = 100;
const int SSID2_ADDR = 150;
const int PASSWORD2_ADDR = 200;

String storedSSID1;
String storedPassword1;
String storedSSID2;
String storedPassword2;

void readCredentialsFromEEPROM() {
    char ssid1[32];
    char password1[32];
    char ssid2[32];
    char password2[32];

    EEPROM.begin(EEPROM_SIZE);

    // Leer SSID1
    for (int i = 0; i < 32; ++i) {
        ssid1[i] = EEPROM.read(SSID1_ADDR + i);
    }
    storedSSID1 = String(ssid1);

    // Leer contraseña1
    for (int i = 0; i < 32; ++i) {
        password1[i] = EEPROM.read(PASSWORD1_ADDR + i);
    }
    storedPassword1 = String(password1);

    // Leer SSID2
    for (int i = 0; i < 32; ++i) {
        ssid2[i] = EEPROM.read(SSID2_ADDR + i);
    }
    storedSSID2 = String(ssid2);

    // Leer contraseña2
    for (int i = 0; i < 32; ++i) {
        password2[i] = EEPROM.read(PASSWORD2_ADDR + i);
    }
    storedPassword2 = String(password2);

    EEPROM.end();
}

void handleRoot() {
    String html = "<html><body>";
    html += "<form method='POST' action='/wifi'>";
    html += "Red Wi-Fi 1: <input type='text' name='ssid1'><br>";
    html += "Contraseña: <input type='password' name='password1'><br>";
    html += "Red Wi-Fi 2: <input type='text' name='ssid2'><br>";
    html += "Contraseña: <input type='password' name='password2'><br>";
    html += "<input type='submit' value='Conectar'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

void handleWifi() {
    String ssid1 = server.arg("ssid1");
    String password1 = server.arg("password1");
    String ssid2 = server.arg("ssid2");
    String password2 = server.arg("password2");

    bool connected = false;

    if (ssid1.length() > 0 && password1.length() > 0) {
        Serial.print("Conectando a la red Wi-Fi ");
        Serial.println(ssid1);

        WiFi.disconnect();
        WiFi.begin(ssid1.c_str(), password1.c_str());

        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Conexión establecida a la red 1");
            server.send(200, "text/plain", "Conexión establecida a la red 1");

            // Almacenar las credenciales en la EEPROM
            EEPROM.begin(EEPROM_SIZE);

            for (int i = 0; i < 32; ++i) {
                EEPROM.write(SSID1_ADDR + i, ssid1[i]);
            }

            for (int i = 0; i < 32; ++i) {
                EEPROM.write(PASSWORD1_ADDR + i, password1[i]);
            }

            EEPROM.commit();
            EEPROM.end();
            connected = true;
        } else {
            Serial.println("Conexión no establecida a la red 1");
            server.send(200, "text/plain", "Conexión no establecida a la red 1");
        }
    }

    if (!connected && ssid2.length() > 0 && password2.length() > 0) {
        Serial.print("Conectando a la red Wi-Fi ");
        Serial.println(ssid2);

        WiFi.disconnect();
        WiFi.begin(ssid2.c_str(), password2.c_str());

        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Conexión establecida a la red 2");
            server.send(200, "text/plain", "Conexión establecida a la red 2");

            // Almacenar las credenciales en la EEPROM
            EEPROM.begin(EEPROM_SIZE);

            for (int i = 0; i < 32; ++i) {
                EEPROM.write(SSID2_ADDR + i, ssid2[i]);
            }

            for (int i = 0; i < 32; ++i) {
                EEPROM.write(PASSWORD2_ADDR + i, password2[i]);
            }

            EEPROM.commit();
            EEPROM.end();
            connected = true;
        } else {
            Serial.println("Conexión no establecida a la red 2");
            server.send(200, "text/plain", "Conexión no establecida a la red 2");
        }
    }

    if (!connected) {
        server.send(200, "text/plain", "No se pudo establecer conexión a ninguna red");
    }
}

void initAP(const char* apSsid, const char* apPassword) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);

    server.begin();
    Serial.print("IP de ESP32: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("Servidor web iniciado");
}

void loopAP() {
    server.handleClient();
}