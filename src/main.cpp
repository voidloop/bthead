#include <Arduino.h>
#include <BLEDevice.h>
#include <sys/param.h>

#include "opentxbt.h"

const int scanTime = 5; // seconds
const char *serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
const char *charUUID = "0000fff6-0000-1000-8000-00805f9b34fb";

BLEScan *pBLEScan = nullptr;
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
    void onConnect(BLEClient *pClient) override {
        Serial.println("Client connected");
        digitalWrite(5, HIGH);
    }

    void onDisconnect(BLEClient *pClient) override {
        Serial.println("Client disconnected");
        digitalWrite(5, LOW);
        delete pDevice;
        pDevice = nullptr;
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
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    // active scan uses more power, but get results faster
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
}


void connectToServer() {
    Serial.println("Connecting...");
    BLEClient *pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(pDevice);

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find service UUID: ");
        Serial.println(serviceUUID);
        pClient->disconnect();
        return;
    }

    Serial.println(pRemoteService->toString().c_str());

    BLERemoteCharacteristic *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID);
        pClient->disconnect();
        return;
    }

    if(pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }
}


void loop() {
    if (pDevice == nullptr) {
        Serial.println("Scanning...");
        BLEScanResults foundDevices = pBLEScan->start(scanTime);

        // delete results from BLEScan buffer to release memory
        pBLEScan->clearResults();

        if (pDevice != nullptr) {
            Serial.println("Device found!");
            Serial.println(pDevice->toString().c_str());
            connectToServer();
        }
    }

    delay(2000);
}