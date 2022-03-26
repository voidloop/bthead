#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include "opentxbt.h"

#define SERVICE_UUID        "0000fff0-0000-1000-8000-00805f9b34fc"
#define CHARACTERISTIC_UUID "0000fff6-0000-1000-8000-00805f9b34fc"

bool deviceConnected = false;
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
BLEService *pService = nullptr;

int16_t channelOutputs[8];

void write(const uint8_t *data, uint8_t length) {
    pCharacteristic->setValue((uint8_t *) data, length);
    pCharacteristic->notify();
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *) override {
        deviceConnected = true;
        Serial.println("Radio connected!");
    };

    void onDisconnect(BLEServer *) override {
        deviceConnected = false;
        Serial.println("Radio disconnected!");
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    BLEDevice::init("Hello");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
    );

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Server started, waiting radio...");
}

void loop() {
    if (deviceConnected) {
        for (int i=0; i<8; ++i) {
            channelOutputs[i] = -1024;
        }
        sendTrainer();
        delay(50);
    }
}


//#include <Arduino.h>
//#include <BLEDevice.h>
//#include "opentxbt.h"
//
//const int scanTime = 5; // seconds
//const char *serviceUUID = "0000fff0-0000-1000-8000-00805f9b34fb";
//const char *charUUID = "0000fff6-0000-1000-8000-00805f9b34fb";
//
//bool isConnected = false;
//BLEScan *pBLEScan = nullptr;
//BLEClient *pClient = nullptr;
//BLEAdvertisedDevice *pDevice = nullptr;
//BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
//uint16_t ppmInput[8];
//int16_t channelOutputs[8];
//
//class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
//    void onResult(BLEAdvertisedDevice advertisedDevice) override {
//        if (advertisedDevice.haveServiceUUID()) {
//            const std::string name = advertisedDevice.getName();
//            const std::string uuid = advertisedDevice.getServiceUUID().toString();
//            if (uuid == serviceUUID && name == "Hello") {
//                delete pDevice;
//                pDevice = new BLEAdvertisedDevice(advertisedDevice);
//                advertisedDevice.getScan()->stop();
//            }
//        }
//    }
//};
//
//class MyClientCallback : public BLEClientCallbacks {
//    void onConnect(BLEClient *) override {
//        Serial.println("Client connected");
//    }
//
//    void onDisconnect(BLEClient *) override {
//        Serial.println("Client disconnected");
//        isConnected = false;
//    }
//};
//
//void write(const uint8_t *data, uint8_t length) {
////    for (int i = 0; i < (int) length; ++i) {
////        Serial.print(data[i], HEX);
////        Serial.print(' ');
////    }
////    Serial.println();
////
////    pRemoteCharacteristic->writeValue((uint8_t *) data, length);
//}
//
//void notifyCallback(BLERemoteCharacteristic *, uint8_t *data, size_t length, bool) {
//
//    Serial.print("< ");
//    for (int i=0; i< (int)length; ++i) {
//        if (i != 0) Serial.print(' ');
//        Serial.print(data[i], HEX);
//    }
//    Serial.println();
//
//    for (size_t i = 0; i < length; ++i) {
//        processTrainerByte(data[i]);
//    }
//}
//
//void setup() {
//    Serial.begin(115200);
//    BLEDevice::init("");
//
//    pBLEScan = BLEDevice::getScan();
//    pClient = BLEDevice::createClient();
//    pClient->setClientCallbacks(new MyClientCallback());
//
//    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
//    pBLEScan->setActiveScan(true);  // active scan uses more power, but get results faster
//    pBLEScan->setInterval(100);
//    pBLEScan->setWindow(99);  // less or equal setInterval value
//}
//
//bool connect() {
//    Serial.println("Connecting...");
//
//    if (pClient != nullptr && pClient->isConnected()) {
//        pClient->disconnect();
//    }
//
//    pClient->connect(pDevice);
//    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
//    if (pRemoteService == nullptr) {
//        Serial.print("Failed to find service UUID: ");
//        Serial.println(serviceUUID);
//        return false;
//    }
//
////    delete pRemoteCharacteristic;
////    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
////    if (pRemoteCharacteristic == nullptr) {
////        Serial.print("Failed to find our characteristic UUID: ");
////        Serial.println(charUUID);
////        return false;
////    }
////
////    pRemoteCharacteristic->registerForNotify(notifyCallback);
//
//
//    auto *characteristics = pRemoteService->getCharacteristics();
//    for (auto &it: *characteristics) {
//        Serial.println(it.second->toString().c_str());
//    }
//
//
//    return true;
//}
//
//void loop() {
//    if (!isConnected) {
//        Serial.println("Scanning...");
//        pBLEScan->start(scanTime);
//        // delete results from BLEScan buffer to release memory
//        pBLEScan->clearResults();
//
//        if (pDevice != nullptr) {
//            std::string address = pDevice->getAddress().toString();
//            Serial.printf("Radio found! (%s)\n", address.c_str());
//            isConnected = connect();
//        }
//    } else {
//        for (int16_t i=0; i<8; ++i) {
//            channelOutputs[i] = i * 100; // NOLINT(cppcoreguidelines-narrowing-conversions)
//        }
//        sendTrainer();
//    }
//}