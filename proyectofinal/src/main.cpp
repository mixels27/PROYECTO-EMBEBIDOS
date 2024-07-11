#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <internet.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);//Crea un objeto lcd para control de la pantalla (DIRECCIÓN pantalla, Tamaño x, Tamño y)

//Pines y letras para el teclado 4x4

const byte ROWS = 4; //Cuatro filas
const byte COLS = 4; //Cuatro columnas

byte ROWSPins[ROWS] = {18, 5, 17, 16}; //Pines del teclado 4x4 (filas) que se usan en el esp32
byte COLSPins[COLS] = {4, 0, 2, 15}; //Pines del teclado 4x4 (columna) que se usan en el esp32

//Definición de los simbolos en el teclado 4x4

char keys[ROWS][COLS] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'S', '0', 'T', 'D'},
};

//Crear instancia de la clase keypad

Keypad keypad = Keypad (makeKeymap(keys), ROWSPins, COLSPins, ROWS, COLS);

//Pines de transistores que se usan en la esp32
const int transistor_pin1 = 23;
const int transistor_pin2 = 19;
const int transistor_pin3 = 12;
const int transistor_pin4 = 13;

//Pines de echo y trigger de los sensores ultrasonico en la esp32

const int TRIG_PIN_1 = 33;
const int TRIG_PIN_2 = 25;
const int TRIG_PIN_3 = 26;
const int TRIG_PIN_4 = 27;
const int TRIG_PIN_5 = 14;

const int ECHO_PIN_1 = 36;
const int ECHO_PIN_2 = 39;
const int ECHO_PIN_3 = 34;
const int ECHO_PIN_4 = 35;
const int ECHO_PIN_5 = 32;

//Variables para interrupciones del programa
volatile bool tecla_presionada = false;
char tecla;

//FUNCIONES

//Funcion para medir distancia con los sensores ultrasonicos
long readUltrasonicDistance(int trigPin, int echoPin) {
  // Enviar un pulso de 10 microsegundos en el pin TRIG
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Leer la duración del pulso en el pin ECHO
  long duration = pulseIn(echoPin, HIGH);

  // Calcular la distancia en centímetros
  long distance = duration * 0.034 / 2;

  return distance;
}

//Funcion para interrupción
void IRAM_ATTR keypress_ISR() {
  tecla_presionada = true;
}

//CODIGO PRINCIPAL
void setup(){

	Serial.begin(9600);

  //Pantalla
  lcd.init();//inicializar la pantalla lcd

  //Se establece los pines en modo de salida
	pinMode(transistor_pin1, OUTPUT);
	pinMode(transistor_pin2, OUTPUT);
	pinMode(transistor_pin3, OUTPUT);
	pinMode(transistor_pin4, OUTPUT);

	//Se inicializa los pines en low para que se encuentren desactivados
	digitalWrite(transistor_pin1, LOW);
	digitalWrite(transistor_pin2, LOW);
	digitalWrite(transistor_pin3, LOW);
	digitalWrite(transistor_pin4, LOW);

  // Configurar los pines de TRIG como salidas
  pinMode(TRIG_PIN_1, OUTPUT);
  pinMode(TRIG_PIN_2, OUTPUT);
  pinMode(TRIG_PIN_3, OUTPUT);
  pinMode(TRIG_PIN_4, OUTPUT);
  pinMode(TRIG_PIN_5, OUTPUT);

  // Configurar los pines de ECHO como entradas
  pinMode(ECHO_PIN_1, INPUT);
  pinMode(ECHO_PIN_2, INPUT);
  pinMode(ECHO_PIN_3, INPUT);
  pinMode(ECHO_PIN_4, INPUT);
  pinMode(ECHO_PIN_5, INPUT);

  //Configurar interrupciones para cada tecla del teclado 4x4
  for (byte i = 0; i < COLS; i++) {
      pinMode(COLSPins[i], INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(COLSPins[i]), keypress_ISR, FALLING);
    }

  //Wifi
  readCredentialsFromEEPROM();

    if (storedSSID1.length() > 0 && storedPassword1.length() > 0) {
        Serial.print("Intentando conectar a ");
        Serial.println(storedSSID1);

        WiFi.begin(storedSSID1.c_str(), storedPassword1.c_str());

        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Conexión establecida a la red 1");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            return;
        } else {
            Serial.println("No se pudo conectar a la red 1");
        }
    }

    if (storedSSID2.length() > 0 && storedPassword2.length() > 0) {
        Serial.print("Intentando conectar a ");
        Serial.println(storedSSID2);

        WiFi.begin(storedSSID2.c_str(), storedPassword2.c_str());

        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED && cnt < 20) {
            delay(500);
            Serial.print(".");
            cnt++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Conexión establecida a la red 2");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            return;
        } else {
            Serial.println("No se pudo conectar a la red 2");
        }
    }

    initAP("espAP", "123456789");
    
}


void loop() {
  while (!tecla_presionada) {
    Serial.println("El sistema de embebido se encuentra apagado.");
  }
  loopAP();
  tecla_presionada = false;
  tecla = keypad.getKey();
  
  if (tecla == 'S') {     
    // Encender el contenido de la pantalla
    lcd.backlight();
    lcd.display();

    while (true) {
      lcd.setCursor(0, 0); // Poner el cursor en las coordenadas (0, 0)
      lcd.print("Bienvenido a la maquina de bebidas");
      lcd.setCursor(0, 1); // Poner el cursor en las coordenadas (0, 1)
      lcd.print("Seleccione su bebida favorita");
      lcd.scrollDisplayLeft();
      delay(300);
      if (tecla_presionada) {
        tecla_presionada = false;
        tecla = keypad.getKey();

        switch (tecla) {
          case 'A':
            {
              long distancia1 = readUltrasonicDistance(TRIG_PIN_1, ECHO_PIN_1);
              if (distancia1 < 20) {
                long distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                if(distancia5<= 2){
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Vaso");
                  lcd.setCursor(0, 1);
                  lcd.print("Lleno");
                  delay(3000);
                }
                
                while (distancia5 > 2 && distancia1 < 20) {
                  // Llenado de bebida 1
                  distancia1 = readUltrasonicDistance(TRIG_PIN_1, ECHO_PIN_1);
                  delay(100);
                  distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                  delay(100);
                  Serial.print("Distancia1: ");
                  Serial.print(distancia1);
                  Serial.println(" cm.");
                  Serial.print("Distancia5: ");
                  Serial.print(distancia5);
                  Serial.println(" cm.");
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Llenando");
                  lcd.setCursor(0, 1);
                  lcd.print("Bebida 1");
                  digitalWrite(transistor_pin1, HIGH);
                }
              } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("No hay");
                lcd.setCursor(0, 1);
                lcd.print("Bebida 1");
                delay(4000);
              }
            digitalWrite(transistor_pin1, LOW);
            }
            break;

          case 'B':
            {
              long distancia2 = readUltrasonicDistance(TRIG_PIN_2, ECHO_PIN_2);
              if (distancia2 < 10) {
                long distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                while (distancia5 > 2 && distancia2 < 20) {
                  // Llenado de bebida 2
                  distancia2 = readUltrasonicDistance(TRIG_PIN_2, ECHO_PIN_2);
                  delay(100);
                  distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                  delay(100);
                  Serial.print("Distancia2: ");
                  Serial.print(distancia2);
                  Serial.println(" cm.");
                  Serial.print("Distancia5: ");
                  Serial.print(distancia5);
                  Serial.println(" cm.");
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Llenando");
                  lcd.setCursor(0, 1);
                  lcd.print("Bebida 2");
                  digitalWrite(transistor_pin2, HIGH);
              
                }
              } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("No hay");
                lcd.setCursor(0, 1);
                lcd.print("Bebida 2");
                delay(4000);
              }
            digitalWrite(transistor_pin2, LOW);
            }
            break;

          case 'C':
            {
              long distancia3 = readUltrasonicDistance(TRIG_PIN_3, ECHO_PIN_3);
              if (distancia3 < 10) {
                long distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                while (distancia5 > 2 && distancia3 < 20) {
                  // Llenado de bebida 3
                  distancia3 = readUltrasonicDistance(TRIG_PIN_3, ECHO_PIN_3);
                  delay(100);
                  distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                  delay(100);
                  Serial.print("Distancia3: ");
                  Serial.print(distancia3);
                  Serial.println(" cm.");
                  Serial.print("Distancia5: ");
                  Serial.println(" cm.");
                  Serial.print(distancia5);
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Llenando");
                  lcd.setCursor(0, 1);
                  lcd.print("Bebida 3");
                  digitalWrite(transistor_pin3, HIGH);
                
                }
              } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("No hay");
                lcd.setCursor(0, 1);
                lcd.print("Bebida 3");
                delay(4000);
              }
            digitalWrite(transistor_pin3, LOW);
            }
            break;

          case 'D':
            {
              long distancia4 = readUltrasonicDistance(TRIG_PIN_4, ECHO_PIN_4);
              if (distancia4 < 10) {
                long distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                while (distancia5 > 2 && distancia4 < 20) {
                  // Llenado de bebida 4
                  distancia4 = readUltrasonicDistance(TRIG_PIN_4, ECHO_PIN_4);
                  delay(100);
                  distancia5 = readUltrasonicDistance(TRIG_PIN_5, ECHO_PIN_5);
                  delay(100);
                  Serial.print("Distancia4: ");
                  Serial.print(distancia4);
                  Serial.println(" cm.");
                  Serial.print("Distancia5: ");
                  Serial.print(distancia5);
                  Serial.println(" cm.");
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Llenando");
                  lcd.setCursor(0, 1);
                  lcd.print("Bebida 4");
                  digitalWrite(transistor_pin4, HIGH);
                  
                }
              } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("No hay");
                lcd.setCursor(0, 1);
                lcd.print("Bebida 4");
                delay(4000);
              }
            digitalWrite(transistor_pin4, LOW);
            }
            break;

          case 'T':
            lcd.noBacklight();
            lcd.noDisplay();
            return; // Salir del bucle while y reiniciar el proceso

          default:
            lcd.setCursor(0, 1);
            lcd.print("Invalido");
            break;
        }
      }
    }
  }
}