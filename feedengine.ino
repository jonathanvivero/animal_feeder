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
bool sendInitialPrompt = false;

// 📢 FUNCIÓN GLOBAL DE EVENTOS BLE
// Puede ser llamada desde cualquier parte del proyecto (.ino o .cpp)
void send_ble_message(String msg) {
    if (deviceConnected && pTxCharacteristic != nullptr) {
        pTxCharacteristic->setValue(msg.c_str());
        pTxCharacteristic->notify();
    }
}

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        deviceConnected = true;
        sendInitialPrompt = true; 
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
            Serial.println("Comando recibido: " + received);

            if (received.length() > 128) {
                received = received.substring(0, 128);
            }

            String cmdResponse = process_command(received);
            cmdResponse += "\nQ?";

            send_ble_message(cmdResponse);
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Iniciando Dispensador ---");

    // Inicia lectura e intento de conexión inicial (si aplica)
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
    // 1. Atendemos de forma continua y no bloqueante los eventos del Wi-Fi
    handle_wifi_async();

    // 2. Control del prompt de bienvenida único
    if (deviceConnected && sendInitialPrompt) {
        sendInitialPrompt = false;
        delay(500);
        send_ble_message("Q?"); 
    }
}