#include <NimBLEDevice.h>
#include "hf_bt_wf_utils.h"
#include "hf_cmd_interpreter.h"

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
NimBLECharacteristic* pRxCharacteristic = nullptr;

bool deviceConnected = false;
bool sendInitialPrompt = false; // Bandera para enviar el "Q?" solo al conectar

// --------------------------------------------------------
// CALLBACKS BLE
// --------------------------------------------------------

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        deviceConnected = true;
        sendInitialPrompt = true; // Activamos el prompt único de bienvenida
        Serial.println("¡Dispositivo móvil conectado!");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        deviceConnected = false;
        sendInitialPrompt = false;
        Serial.println("Dispositivo móvil desconectado. Reiniciando anuncios...");
        NimBLEDevice::startAdvertising();
    }
};

class MyRxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
        std::string rxValue = pChar->getValue();

        if (rxValue.length() > 0) {
            String received = String(rxValue.c_str());
            Serial.println("Comando recibido vía BLE: " + received);

            if (received.length() > 128) {
                received = received.substring(0, 128);
            }

            // 1. Procesamos el comando a través del intérprete
            String cmdResponse = process_command(received);

            // 2. Concatenamos el prompt "Q?" al final de la respuesta.
            // Esto garantiza que viajen juntos en orden y evita la condición de carrera.
            cmdResponse += "\nQ?";

            // 3. Enviamos el bloque completo de datos
            pTxCharacteristic->setValue(cmdResponse.c_str());
            pTxCharacteristic->notify();
        }
    }
};

// --------------------------------------------------------
// SETUP & LOOP
// --------------------------------------------------------

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Iniciando Dispensador ---");

    read_from_store();

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
    // El loop solo interviene una vez por conexión para dar la bienvenida
    if (deviceConnected && sendInitialPrompt) {
        sendInitialPrompt = false;
        
        delay(500); // Pequeña pausa para que el móvil termine de abrir sus canales internos
        Serial.println("Enviando prompt inicial: Q?");
        pTxCharacteristic->setValue("Q?");
        pTxCharacteristic->notify(); 
    }
    
    // El loop queda completamente libre de manera no bloqueante para el motor en el futuro
}