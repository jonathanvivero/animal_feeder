#include "hf_bt_wf_utils.h"

// Vinculamos la función de envío BLE que está definida en el archivo .ino
extern void send_ble_message(String msg);

String wf_ssid = "";
String wf_pwd  = "";
Preferences preferences;

// Variables internas para el control de estado Wi-Fi sin bloquear
bool is_connecting = false;
int wifi_attempts = 0;
unsigned long wifi_timeout_ms = 0;

void wf_connect() {
    if (wf_ssid.length() == 0) {
        Serial.println("SSID vacío, se omite la conexión Wi-Fi.");
        return;
    }

    Serial.println("Iniciando solicitud de conexión Wi-Fi...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); 
    
    // Configuramos los estados iniciales de la máquina de progreso
    is_connecting = true;
    wifi_attempts = 1;
    WiFi.begin(wf_ssid.c_str(), wf_pwd.c_str());
    wifi_timeout_ms = millis();
}

void read_from_store() {
    preferences.begin("feedengine", true); 
    wf_ssid = preferences.getString("wf_ssid", "");
    wf_pwd  = preferences.getString("wf_pwd", "");
    preferences.end();

    Serial.println("Datos recuperados de NVS. SSID: " + (wf_ssid == "" ? "[Ninguno]" : wf_ssid));

    if (wf_ssid.length() > 0) {
        wf_connect();
    }
}

void write_to_store(String wssid, String wpass) {
    wf_ssid = wssid;
    wf_pwd  = wpass;

    preferences.begin("feedengine", false);
    preferences.putString("wf_ssid", wf_ssid);
    preferences.putString("wf_pwd", wf_pwd);
    preferences.end();

    Serial.println("Credenciales actualizadas en NVS.");
    wf_connect();
}

// Esta función procesa el progreso en segundo plano desde el loop principal
void handle_wifi_async() {
    if (!is_connecting) return;

    // CASO EXITO: Se ha conectado
    if (WiFi.status() == WL_CONNECTED) {
        is_connecting = false;
        Serial.print("¡Wi-Fi Conectado! IP: ");
        Serial.println(WiFi.localIP());
        
        // Desencadenamos el EVENTO push hacia el móvil de forma asíncrona
        String eventMsg = "\n[EVENTO] Wi-Fi Conectado exitosamente.\nIP: " + WiFi.localIP().toString() + "\nQ?";
        send_ble_message(eventMsg);
        return;
    }

    // CASO TIMEOUT: Si pasan 5 segundos y no ha conectado, evaluamos el siguiente intento
    if (millis() - wifi_timeout_ms > 5000) {
        wifi_attempts++;
        
        if (wifi_attempts > 3) {
            // Superados los 3 intentos, abortamos la operación
            is_connecting = false;
            WiFi.disconnect();
            Serial.println("Fallo definitivo: Superados los 3 intentos de Wi-Fi.");
            
            // Desencadenamos el EVENTO de error push hacia el móvil
            send_ble_message("\n[EVENTO] Error: No se pudo conectar a la red Wi-Fi tras 3 intentos.\nQ?");
        } else {
            // Siguiente intento
            Serial.printf("Intento Wi-Fi %d de 3 de forma asíncrona...\n", wifi_attempts);
            WiFi.begin(wf_ssid.c_str(), wf_pwd.c_str());
            wifi_timeout_ms = millis();
        }
    }
}