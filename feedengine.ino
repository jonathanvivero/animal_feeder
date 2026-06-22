#include <NimBLEDevice.h>

// UUIDs estándar de la industria (Nordic UART Service)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // Servicio UART
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX (Móvil -> ESP32)
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX (ESP32 -> Móvil)

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;
bool deviceConnected = false;
bool txReady = false;

// Callbacks de conexión
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
            Serial.print("Datos recibidos: ");
            Serial.println(rxValue.c_str());

            String response = String(rxValue.c_str());
            if (response.length() > 128) {
                response = response.substring(0, 128);
            }

            // Construimos la respuesta
            String echoResponse = "R:" + response;

            // Enviamos de vuelta por el canal TX
            pTxCharacteristic->setValue(echoResponse.c_str());
            pTxCharacteristic->notify();
            
            delay(100); 

            txReady = true; // Disparamos la siguiente pregunta
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando BLE con perfil Nordic UART...");

    // Nombre más corto para que quepa en el paquete de 31 bytes
    NimBLEDevice::init("Dispensador");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    // Característica de Transmisión (TX) - Notifica al móvil
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );

    // Característica de Recepción (RX) - Recibe del móvil
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new MyRxCallbacks());

    pService->start();

    // Configuración de anuncios forzando el nombre
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName("Hrs-Dsp"); 
    pAdvertising->start();

    Serial.println("Esperando conexión...");
}

void loop() {
    if (deviceConnected && txReady) {
        txReady = false; 
        
        Serial.println("Enviando: Q?");
        pTxCharacteristic->setValue("Q?");
        pTxCharacteristic->notify(); 
    }
}