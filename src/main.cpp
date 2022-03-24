#include <Arduino.h>
#include <BLEDevice.h>
#include <sys/param.h>
#include "opentxbt.h"

const int scanTime = 5; // seconds
const char *serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
const char *charUUID = "0000fff3-0000-1000-8000-00805f9b34fb";

enum States {
    SCANNING,
    CONNECTED,
    BEFORE_DISCONNECT,
    DISCONNECTED
};

States currentState = DISCONNECTED;
BLEScan *pBLEScan = nullptr;
BLEClient *pClient = nullptr;
BLEAdvertisedDevice *pDevice = nullptr;
uint16_t ppmInput[8];

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveServiceUUID()) {
            const std::string name = advertisedDevice.getName();
            const std::string uuid = advertisedDevice.getServiceUUID().toString();
            if (uuid == serviceUUID && name == "Hello") {
                pDevice = new BLEAdvertisedDevice(advertisedDevice);
                advertisedDevice.getScan()->stop();
            }
        }
    }
};

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient *) override {
        Serial.println("Client connected");
        digitalWrite(5, HIGH);
    }

    void onDisconnect(BLEClient *) override {
        Serial.println("Client disconnected");
        digitalWrite(5, LOW);
        currentState = BEFORE_DISCONNECT;
    }
};

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
//    Serial.print("Notify callback for characteristic ");
//    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
//    Serial.print(" of data length ");
//    Serial.println(length);
//    Serial.print("data: ");
//    Serial.println((char*)pData);

    for (size_t i = 0; i< length; ++i) {
        processTrainerByte(pData[i]);
    }

    for(int i=0;i<8;i++) {
        // Limit channels to 1000 - 2000us
        ppmInput[i] = MAX(MIN(ppmInput[i],2000),1000);
        Serial.printf("%d: %d\n", i, ppmInput[i]);
    }
}

void setup() {
    Serial.begin(115200);
    BLEDevice::init("");

    pBLEScan = BLEDevice::getScan();
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);  // active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
}

bool connect() {
    Serial.println("Connecting...");
    pClient->connect(pDevice);

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find service UUID: ");
        Serial.println(serviceUUID);
        return false;
    }

    BLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID);
        return false;
    }

    Serial.printf("canWrite: %d\n", pRemoteCharacteristic->canWrite());

    return true;
}

void loop() {
    switch (currentState) {
        case SCANNING:
            Serial.println("Scanning...");
            pBLEScan->start(scanTime);
            // delete results from BLEScan buffer to release memory
            pBLEScan->clearResults();

            if (pDevice != nullptr) {
                std::string address = pDevice->getAddress().toString();
                Serial.printf("Radio found! (%s)\n", address.c_str());
                currentState = connect() ? CONNECTED : BEFORE_DISCONNECT;
                delete pDevice;
            }
            Serial.println(esp_get_free_heap_size());
            break;

        case CONNECTED:
            currentState = DISCONNECTED;
            break;

        case BEFORE_DISCONNECT:
            pClient->disconnect();
        case DISCONNECTED:
            currentState = SCANNING;
            delay(2000);
            break;
    }
}