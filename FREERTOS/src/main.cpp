//------------------------Librerias Usadas------------------
//Libreria de Arduino
#include <Arduino.h>
//Libreria para conexión de internet y servidor
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
//Librerias para el sistema de embebido
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <esp_sleep.h>

//----------------------------------INTENENET---------------------
//Se define variables para poder almacenar hasta tres redes Wi-Fi en la ESP32.
#define EEPROM_SIZE 512
#define SSID1_ADDR 0
#define PASSWORD1_ADDR 32
#define SSID2_ADDR 64
#define PASSWORD2_ADDR 96
#define SSID3_ADDR 128
#define PASSWORD3_ADDR 160

String storedSSID1, storedPassword1;
String storedSSID2, storedPassword2;
String storedSSID3, storedPassword3;

//Se inicia la conexion del servidor
AsyncWebServer server(80);

//Funcion que permite leer lo que hay en la memoria EEPROM DE LA ESP32
void readCredentialsFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    char ssid1[32], password1[32], ssid2[32], password2[32], ssid3[32], password3[32];
    for (int i = 0; i < 32; i++) {
        ssid1[i] = EEPROM.read(SSID1_ADDR + i);
        password1[i] = EEPROM.read(PASSWORD1_ADDR + i);
        ssid2[i] = EEPROM.read(SSID2_ADDR + i);
        password2[i] = EEPROM.read(PASSWORD2_ADDR + i);
        ssid3[i] = EEPROM.read(SSID3_ADDR + i);
        password3[i] = EEPROM.read(PASSWORD3_ADDR + i);
    }
    EEPROM.end();
    storedSSID1 = String(ssid1);
    storedPassword1 = String(password1);
    storedSSID2 = String(ssid2);
    storedPassword2 = String(password2);
    storedSSID3 = String(ssid3);
    storedPassword3 = String(password3);
}

//Funcion que se usa para iniciar una pagina y agregar la seccion de configuracion de las redes de la ESP32
void handleRoot(AsyncWebServerRequest *request) {
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }";
    html += "h1 { color: #333; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Drink Machine WiFi Configuration</h1>";
    html += "<a href='/config'>Configure Wi-Fi Networks</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
}

//Funcion que configura el apartado de configuracion para registrar las redes desde la pagina web
void handleConfig(AsyncWebServerRequest *request) {
    String html = "<html><body>";
    html += "<h1>Configure Wi-Fi Networks</h1>";
    html += "<form method='POST' action='/saveconfig'>";
    html += "<h2>Primary Wi-Fi</h2>";
    html += "SSID: <input type='text' name='ssid1' value='" + storedSSID1 + "'><br>";
    html += "Password: <input type='password' name='password1'><br>";
    html += "<h2>Secondary Wi-Fi</h2>";
    html += "SSID: <input type='text' name='ssid2' value='" + storedSSID2 + "'><br>";
    html += "Password: <input type='password' name='password2'><br>";
    html += "<h2>Tertiary Wi-Fi</h2>";
    html += "SSID: <input type='text' name='ssid3' value='" + storedSSID3 + "'><br>";
    html += "Password: <input type='password' name='password3'><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
}

//Funcion para guardar las redes registradas desde la web en la memoria EEPROM de la ESP32
void handleSaveConfig(AsyncWebServerRequest *request) {
    String ssid1 = request->arg("ssid1");
    String password1 = request->arg("password1");
    String ssid2 = request->arg("ssid2");
    String password2 = request->arg("password2");
    String ssid3 = request->arg("ssid3");
    String password3 = request->arg("password3");

    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < 32; i++) {
        EEPROM.write(SSID1_ADDR + i, ssid1[i]);
        EEPROM.write(PASSWORD1_ADDR + i, password1[i]);
        EEPROM.write(SSID2_ADDR + i, ssid2[i]);
        EEPROM.write(PASSWORD2_ADDR + i, password2[i]);
        EEPROM.write(SSID3_ADDR + i, ssid3[i]);
        EEPROM.write(PASSWORD3_ADDR + i, password3[i]);
    }
    EEPROM.commit();
    EEPROM.end();

    String html = "<html><body>";
    html += "<h1>Configuration Saved</h1>";
    html += "Primary SSID: " + ssid1 + "<br>";
    html += "Secondary SSID: " + ssid2 + "<br>";
    html += "Tertiary SSID: " + ssid3 + "<br>";
    html += "The configuration has been saved. Restart the device to connect to the new networks.";
    html += "<br><br><a href='/'>Back to Home</a>";
    html += "</body></html>";
    request->send(200, "text/html", html);
}

//Funcion que conecta a la ESP32 a una red Wi-Fi registrada, si se logra conectar envia un valor True, caso contrario envia False
bool connectToWiFi() {
    readCredentialsFromEEPROM();

    const char* ssids[] = {storedSSID1.c_str(), storedSSID2.c_str(), storedSSID3.c_str()};
    const char* passwords[] = {storedPassword1.c_str(), storedPassword2.c_str(), storedPassword3.c_str()};

    for (int i = 0; i < 3; i++) {
        if (strlen(ssids[i]) > 0 && strlen(passwords[i]) > 0) {
            WiFi.begin(ssids[i], passwords[i]);
            int cnt = 0;
            while (WiFi.status() != WL_CONNECTED && cnt < 20) {
                delay(500);
                Serial.print(".");
                cnt++;
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi: " + String(ssids[i]));
                Serial.print("IP Address: ");
                Serial.println(WiFi.localIP());
                return true;
            }
        }
    }
    return false;
}

//Funcion que permite establecer la conexion Wi-Fi y activa la pagina web
void setupWiFiAndServer() {
    if (!connectToWiFi()) {
        Serial.println("Could not connect to any stored WiFi. Starting AP mode.");
        WiFi.softAP("ESP32-DrinkMachine", "password");
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/config", HTTP_GET, handleConfig);
    server.on("/saveconfig", HTTP_POST, handleSaveConfig);
    server.begin();
}

//---------------------------------------TASKS PARA FREERTOS------------------------
//void TestHwm(char *taskName);
void TaskServer(void *parameter);
void TaskDrinksMachine(void *pvParameters);

//------------------CONFIGURACION DE LOS PINES A USAR EN LA ESP32----------------------
// Configuración de la pantalla LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Configuración del teclado 4x4
byte ROWSPins[4] = {18, 5, 17, 16};
byte COLSPins[4] = {4, 0, 2, 15};

char keys[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'S', '0', 'T', 'D'},
};

Keypad keypad = Keypad(makeKeymap(keys), ROWSPins, COLSPins, 4, 4);

// Pines para la activación de las bombas
const uint8_t transistor_pins[] = {19, 23, 12, 13};

// Pines para los sensores de ultrasonido
const uint8_t trig_pins[] = {33, 25, 26, 27, 14};
const uint8_t echo_pins[] = {36, 39, 34, 35, 32};

//------------DECLARACION DE VARIABLES QUE SE USARAN PARA EL SISTEMA DE EMBEBIDO------------
// Variable para almacenar la tecla presionada en el teclado 4x4
volatile char tecla= '\0';
// Variable para almacenar el boton presionado en la aplicacion
volatile char tecla_digital = '\0';
// Variable que analiza el estado del vaso para enviar a la aplicacion
volatile bool glassfull = false;

// Declaración del arreglo para almacenar las distancias y alturas
long distancias[4];
long alturas[4];

//--------------------------FUNCIONES-----------------------
// Función para medir la distancia en los sensores de ultrasonido
long readUltrasonicDistance(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    long distance = duration * 0.034 / 2;
    return distance;
}

// Función para entrar en modo reposo la ESP32
void sleep (){
  Serial.println("Se entro en modo reposo");
  lcd.clear();
  lcd.noBacklight();
  lcd.noDisplay();
  esp_deep_sleep_start();
}

// Función para dispensar bebidas
void dispenseBeverage(int index) {
    long distancia = readUltrasonicDistance(trig_pins[index], echo_pins[index]);
    Serial.print("Distancia bebida ");
    Serial.print(index + 1);
    Serial.print(": ");
    Serial.println(distancia);

    if (distancia < 12) {
        long distancia5 = readUltrasonicDistance(trig_pins[4], echo_pins[4]);
        Serial.print("Distancia vaso: ");
        Serial.println(distancia5);

        if (distancia5 <= 14) {
            lcd.clear();
            lcd.print("Vaso Lleno");
            delay(3000);
            lcd.clear();
            Serial.println("Vaso Full");

        } else {
            while (distancia5 >= 14 && distancia < 12) {
                distancia = readUltrasonicDistance(trig_pins[index], echo_pins[index]);
                delay(10);
                distancia5 = readUltrasonicDistance(trig_pins[4], echo_pins[4]);
                delay(10);

                lcd.clear();
                lcd.print("Llenando Bebida ");
                lcd.print(index + 1);

                digitalWrite(transistor_pins[index], HIGH);
            }
            digitalWrite(transistor_pins[index], LOW);
            lcd.clear();
            Serial.println("Vaso Lleno");
            
        }
    } else {
        lcd.clear();
        lcd.print("No hay Bebida ");
        lcd.print(index + 1);
        delay(4000);
        lcd.clear();
        Serial.println("No hay bebida");
    }
    glassfull = true;
}

// Función para actualizar las distancias y enviar al servidor (aplicacion)
void updateDistances() {
    for (int i = 0; i < 4; i++) {
        distancias[i] = readUltrasonicDistance(trig_pins[i], echo_pins[i]);
        delay(10);
    }
    for (int h = 0; h < 4; h++){
        if(distancias[h] < 14){
            alturas[h] = 140 - distancias[h] * 10;
        } else {
            alturas[h] = 1;
        }
    }
}

//-----------------------------CODIGO PRINCIPAL----------------
void setup() {
    //Se inicia la comunicacion serial
    Serial.begin(115200);

    //Se inicia la pantalla
    lcd.init();

    //Establece los pines de los transistores
    for (uint8_t pin : transistor_pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    //Establece los pines para los sensores ultrasonicos
    for (uint8_t i = 0; i < 5; i++) {
        pinMode(trig_pins[i], OUTPUT);
        pinMode(echo_pins[i], INPUT);
    }

    //Establece el pin GPIO4 para poder despertar la ESP32
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, LOW);

    // Crear tareas en el núcleo 1
    
    xTaskCreatePinnedToCore(
        TaskDrinksMachine,   // Función de la tarea
        "TaskDrinksMachine", // Nombre de la tarea
        1700,                  // Tamaño de la pila
        NULL,                  // Parámetros de la tarea
        1,                     // Prioridad de la tarea
        NULL,                  // Identificador de la tarea
        1                      // Núcleo 1
    );

   
// Crea la tarea para el núcleo 0
    xTaskCreatePinnedToCore(
        TaskServer,    // Función de la tarea
        "TaskServer",  // Nombre de la tarea
        2300,        // Tamaño de la pila de la tarea
        NULL,        // Parámetro de la tarea
        1,           // Prioridad de la tarea
        NULL,        // Handle de la tarea
        0);          // Núcleo 0
   
}

void loop() {
    // No hacer nada aquí, ya que el código está en la tarea core0Task
}

// Tarea que se ejecutará en el núcleo 0
void TaskServer(void *parameter) {
    // Configura el Wi-Fi y el servidor web
    setupWiFiAndServer();

    //Pantalla1 de la aplicacion
    server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
        tecla_digital = 'S';
    });

    //Pantalla2 de la aplicacion
    server.on("/getHeights", HTTP_GET, [](AsyncWebServerRequest *request){
        updateDistances();
        // Crear un array JSON con las alturas
        String json = "[" + String(alturas[0]) + "," + String(alturas[1]) + "," + String(alturas[2]) + "," + String(alturas[3]) + "]";
        
        // Enviar el array como respuesta
        request->send(200, "application/json", json);
    });

    server.on("/dispenseBeverage1", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
        tecla_digital= 'A';
    });

    server.on("/dispenseBeverage2", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
        tecla_digital= 'B';
    });

    server.on("/dispenseBeverage3", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
        tecla_digital = 'C';
    });

    server.on("/dispenseBeverage4", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "OK");
        tecla_digital = 'D';
    });

    //Pantalla3 de la aplicacion
    server.on("/checkFilling", HTTP_GET, [](AsyncWebServerRequest *request){
        String response = glassfull ? "OK" : "ready";
        request->send(200, "text/plain", response);
        if (glassfull) {
        glassfull = false;  // Resetear la variable después de enviar "OK"
        }
    });

    server.begin();
    
    // Mantén la tarea activa
    while(true) {
        // Código adicional que necesites ejecutar en el núcleo 0
        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
}

// Tarea que maneja la selección y dispensado de bebidas en el nucleo 1

void TaskDrinksMachine(void *pvParameters) {
  
    while (true) {
        tecla = keypad.getKey();
        if (tecla == 'S'||tecla_digital =='S') {
            for (int i = 0; i < 4; i++) {
                distancias[i] = readUltrasonicDistance(trig_pins[i], echo_pins[i]);
                Serial.print("Distancia sensor ");
                Serial.print(i+1);
                Serial.print(": ");
                Serial.println(distancias[i]);
            }
    
            lcd.backlight();
            lcd.display();
            lcd.setCursor(0, 0);
            lcd.print("Bienvenido a la");
            lcd.setCursor(0, 1);
            lcd.print("maquina de bebidas");

            for (int i = 0; i < 18; i++) {
                lcd.scrollDisplayLeft();
                vTaskDelay(600 / portTICK_PERIOD_MS);
            }

            lcd.clear();
            Serial.println("Sistema Iniciado");
            while (true) {
                tecla = keypad.getKey();
                lcd.setCursor(0, 0);
                lcd.print("Seleccione");
                lcd.setCursor(0, 1);
                lcd.print("una bebida");
                if (tecla||tecla_digital) {
                    if (tecla >= 'A' && tecla <= 'D') {
                        int index = tecla - 'A';
                        dispenseBeverage(index);
                        tecla = ' ';
                        Serial.println("Se lleno vaso");
                    } 
                    else if (tecla_digital >= 'A' && tecla_digital <= 'D'){
                        int index_digital = tecla_digital - 'A';
                        dispenseBeverage(index_digital);
                        tecla_digital = ' ';
                        Serial.println("Se lleno vaso");
                    } 
                    else if (tecla == 'T') {
                        sleep();
                    }
                }
              vTaskDelay(50 / portTICK_PERIOD_MS); // Pequeño retraso para evitar consumo excesivo de CPU
            }
        }
      vTaskDelay(10 / portTICK_PERIOD_MS); // Pequeño retraso en el bucle principal
    }
}



// Esta funcion ocupa 512 bytes y sirve para ver de forma aproximada cuanto espacio ocupa las tareas en la ESP32
/*
void TestHwm(char *taskName){
  static int stack_hwm, stack_hwm_temp;

  stack_hwm_temp = uxTaskGetStackHighWaterMark(nullptr);
  if(!stack_hwm || (stack_hwm_temp < stack_hwm)){
    stack_hwm=stack_hwm_temp;
    Serial.printf("%s has stack hwm %u\n",taskName,stack_hwm);
  }
} 
*/
