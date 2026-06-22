#include <NimBLEDevice.h>
#include <WiFi.h>
#include <Preferences.h>

// UUIDs estándar de la industria (Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // Servicio UART
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX (Móvil -> ESP32)
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX (ESP32 -> Móvil)

// Instancia para el almacenamiento no volátil
Preferences preferences;

// Variables globales de red
String wf_ssid = "";
String wf_pwd  = "";

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
bool deviceConnected = false;
bool txReady = false;

// --------------------------------------------------------
// MÉTODOS DE RED Y ALMACENAMIENTO
// --------------------------------------------------------

void wf_connect() {
    if (wf_ssid.length() == 0) {
        Serial.println("SSID vacío, se omite la conexión Wi-Fi.");
        return;
    }

    Serial.println("Iniciando conexión Wi-Fi a: " + wf_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Limpiar conexiones previas
    delay(100);

    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.printf("Intento Wi-Fi %d de 3...\n", attempt);
        WiFi.begin(wf_ssid.c_str(), wf_pwd.c_str());

        // Esperar hasta 5 segundos por intento de forma no bloqueante extrema
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 5000)) {
            delay(100); // Pequeño respiro para que el Watchdog y BLE respiren
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("¡Conectado a Wi-Fi! IP local: ");
            Serial.println(WiFi.localIP());
            return; // Salir de la función si hay éxito
        }
        
        Serial.println("Fallo de conexión en este intento.");
    }
    
    Serial.println("Se superaron los 3 intentos. Desistiendo de la conexión Wi-Fi.");
    WiFi.disconnect(); // Apagar la radio Wi-Fi para ahorrar energía si falló
}

void read_from_store() {
    // Abrir el espacio de nombres "feedengine" en modo solo lectura (true)
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

    // Abrir en modo lectura/escritura (false)
    preferences.begin("feedengine", false);
    preferences.putString("wf_ssid", wf_ssid);
    preferences.putString("wf_pwd", wf_pwd);
    preferences.end();

    Serial.println("Nuevas credenciales guardadas en NVS de forma persistente.");
    wf_connect();
}

// --------------------------------------------------------
// CALLBACKS BLE
// --------------------------------------------------------
class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        deviceConnected = true;
        txReady = true; 
        Serial.println("¡Dispositivo móvil conectado!");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        deviceConnected = false;
        txReady = false;
        Serial.println("Dispositivo móvil desconectado. Reiniciando anuncios...");
        NimBLEDevice::startAdvertising();
    }
};

// Callbacks de recepción (RX)
class MyRxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
        std::string rxValue = pChar->getValue();

        if (rxValue.length() > 0) {
            String received = String(rxValue.c_str());
            Serial.println("Recibido vía BLE: " + received);

            // Mantenemos el límite de caracteres por seguridad de la memoria dinámica
            if (received.length() > 128) {
                received = received.substring(0, 128);
            }

            // NOTA: Aquí en el futuro analizaremos el comando recibido. 
            // Si el comando es para configurar Wi-Fi, llamaremos a write_to_store()
            
            String echoResponse = "R:" + received;
            pTxCharacteristic->setValue(echoResponse.c_str());
            pTxCharacteristic->notify();
            
            delay(100); 

            txReady = true; // Disparamos la siguiente pregunta
        }
    }
};

// --------------------------------------------------------
// SETUP & LOOP
// --------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Iniciando Dispensador ---");

    // 1. Leer preferencias e intentar conexión Wi-Fi (si procede)
    read_from_store();

    // 2. Configurar el subsistema BLE
    Serial.println("Iniciando BLE con perfil Nordic UART...");
    NimBLEDevice::init("Hrs-Dsp");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );

    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyRxCallbacks());

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("Hrs-Dsp"); 
    pAdvertising->start();

    Serial.println("BLE listo y esperando conexión...");
}

void loop() {
    if (deviceConnected && txReady) {
        txReady = false; 
        
        Serial.println("Enviando: Q?");
        pTxCharacteristic->setValue("Q?");
        pTxCharacteristic->notify(); 
    }
}