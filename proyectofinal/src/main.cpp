#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include "internet.h"
#include <esp_sleep.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
byte ROWSPins[ROWS] = {18, 5, 17, 16};
byte COLSPins[COLS] = {4, 0, 2, 15};

char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'S', '0', 'T', 'D'},
};

Keypad keypad = Keypad(makeKeymap(keys), ROWSPins, COLSPins, ROWS, COLS);

const uint8_t transistor_pins[] = {19, 23, 12, 13};
const uint8_t trig_pins[] = {33, 25, 26, 27, 14};
const uint8_t echo_pins[] = {36, 39, 34, 35, 32};

volatile bool tecla_presionada = false;
char tecla;

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

void IRAM_ATTR keypress_ISR() {
    tecla_presionada = true;
}

void IRAM_ATTR wakeup_isr() {
    // Esta función se llamará cuando el ESP32 se despierte
    // No es necesario hacer nada aquí, ya que el ESP32 se reiniciará
}

void setup() {
    Serial.begin(9600);
    lcd.init();

    for (uint8_t pin : transistor_pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    for (uint8_t i = 0; i < 5; i++) {
        pinMode(trig_pins[i], OUTPUT);
        pinMode(echo_pins[i], INPUT);
    }

    for (byte i = 0; i < COLS; i++) {
        pinMode(COLSPins[i], INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(COLSPins[i]), keypress_ISR, FALLING);
    }

    setupWiFiAndServer();
    // Configurar el pin GPIO para despertar el ESP32
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, LOW); // GPIO_NUM_4 corresponde al pin 4 (columna 0 del teclado)
}



void loop() {
    handleClient();

    tecla_presionada = false;
    tecla = keypad.getKey();

    if (tecla == 'S') {
        lcd.backlight();
        lcd.display();
        lcd.setCursor(0, 0);
        lcd.print("Bienvenido a la");
        lcd.setCursor(0, 1);
        lcd.print("maquina de bebidas");
        delay(2000);

        // Desplaza el mensaje a la izquierda
        for (int i = 0; i < 16; i++) {
            lcd.scrollDisplayLeft();
            delay(600); // Ajusta el tiempo de retraso para controlar la velocidad de desplazamiento
        }

        // Actualiza el estado de las bebidas
        for (int i = 0; i < 5; i++) {
            long distance = readUltrasonicDistance(trig_pins[i], echo_pins[i]);
            Serial.print("Distancia sensor ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(distance);
            updateDistance(i, distance);
        }

        // Actualizar estados de los motores
        for (int i = 0; i < 4; i++) {
            updateMotorState(i, digitalRead(transistor_pins[i]) == HIGH);
        }

        lcd.clear();
        tecla_presionada = false;
        while (true) {
            delay(10);
            handleClient();
            lcd.setCursor(0, 0);
            lcd.print("Seleccione");
            lcd.setCursor(0, 1);
            lcd.print("una bebida");

            if (tecla_presionada) {
                tecla_presionada = false;
                tecla = keypad.getKey();

                if (tecla >= 'A' && tecla <= 'D') {
                    int index = tecla - 'A';
                    long distancia = readUltrasonicDistance(trig_pins[index], echo_pins[index]);
                    Serial.print("Distancia bebida ");
                    Serial.print(index + 1);
                    Serial.print(": ");
                    Serial.println(distancia);
                    handleClient();

                    if (distancia < 12) {
                        long distancia5 = readUltrasonicDistance(trig_pins[4], echo_pins[4]);
                        Serial.print("Distancia vaso: ");
                        Serial.println(distancia5);
                        handleClient();

                        if (distancia5 <= 14) {
                            handleClient();
                            lcd.clear();
                            lcd.print("Vaso Lleno");
                            delay(3000);
                            lcd.clear();
                        } else {
                            while (distancia5 >= 14 && distancia < 12) {
                                handleClient();
                                updateDistance(4, readUltrasonicDistance(trig_pins[4], echo_pins[4]));
                                updateDistance(index, readUltrasonicDistance(trig_pins[index], echo_pins[index]));
                                distancia = readUltrasonicDistance(trig_pins[index], echo_pins[index]);
                                delay(10);
                                distancia5 = readUltrasonicDistance(trig_pins[4], echo_pins[4]);
                                delay(10);

                                lcd.clear();
                                lcd.print("Llenando Bebida ");
                                lcd.print(index + 1);

                                digitalWrite(transistor_pins[index], HIGH);
                                updateMotorState(index, true);
                            }
                            digitalWrite(transistor_pins[index], LOW);
                            updateMotorState(index, false);
                            lcd.clear();
                        }
                    } else {
                        handleClient();
                        lcd.clear();
                        lcd.print("No hay Bebida ");
                        lcd.print(index + 1);
                        delay(4000);
                        lcd.clear();
                    }
                } else if (tecla == 'T') {
                    handleClient();
                    lcd.clear();
                    lcd.noBacklight();
                    lcd.noDisplay();
                    esp_deep_sleep_start();
                }
            }
        }
    } 
}
