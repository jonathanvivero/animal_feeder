#include "hf_bt_wf_utils.h"

// Inicialización real de las variables
String wf_ssid = "";
String wf_pwd  = "";
Preferences preferences;

void wf_connect() {
    if (wf_ssid.length() == 0) {
        Serial.println("SSID vacío, se omite la conexión Wi-Fi.");
        return;
    }

    Serial.println("Iniciando conexión Wi-Fi a: " + wf_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 
    delay(100);

    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.printf("Intento Wi-Fi %d de 3...\n", attempt);
        WiFi.begin(wf_ssid.c_str(), wf_pwd.c_str());

        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 5000)) {
            delay(100); 
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("¡Conectado a Wi-Fi! IP local: ");
            Serial.println(WiFi.localIP());
            return; 
        }
        
        Serial.println("Fallo de conexión en este intento.");
    }
    
    Serial.println("Se superaron los 3 intentos. Desistiendo de la conexión Wi-Fi.");
    WiFi.disconnect();
}

void read_from_store() {
    preferences.begin("feedengine", true); 
    wf_ssid = preferences.getString("wf_ssid", "");
    wf_pwd  = preferences.getString("wf_pwd", "");
    preferences.end();

    Serial.println("Datos recuperados de NVS:");
    Serial.println("SSID guardado: " + (wf_ssid == "" ? "[Ninguno]" : wf_ssid));

    if (wf_ssid.length() > 0) {
        wf_connect();
    } else {
        Serial.println("No existen credenciales en memoria. Dispositivo en modo Offline/BLE.");
    }
}

void write_to_store(String wssid, String wpass) {
    wf_ssid = wssid;
    wf_pwd  = wpass;

    preferences.begin("feedengine", false);
    preferences.putString("wf_ssid", wf_ssid);
    preferences.putString("wf_pwd", wf_pwd);
    preferences.end();

    Serial.println("Nuevas credenciales guardadas en NVS de forma persistente.");
    wf_connect();
}