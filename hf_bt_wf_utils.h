#ifndef HF_BT_WF_UTILS_H
#define HF_BT_WF_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

// Exponemos las variables globales para que el sketch principal pueda acceder a ellas si lo necesita
extern String wf_ssid;
extern String wf_pwd;
extern Preferences preferences;

// Declaración de los métodos
void wf_connect();
void read_from_store();
void write_to_store(String wssid, String wpass);
void handle_wifi_async(); // <-- Nueva función de monitoreo asíncrono

#endif