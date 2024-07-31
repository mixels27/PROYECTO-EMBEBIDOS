#ifndef INTERNET_H
#define INTERNET_H

#include <Arduino.h>

void setupWiFiAndServer();
void handleClient();
void updateDistance(int index, long distance);
long getDistance(int index);
void updateMotorState(int index, bool state);
bool getMotorState(int index);
void readCredentialsFromEEPROM();
void handleConfig();
void handleSaveConfig();
void handleRoot();
void handleStatus();

#endif