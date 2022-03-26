#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <PPMReader.h>
#include "opentxbt.h"

#define INTERRUPT_PIN 2
#define SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fc"
#define CHARACTERISTIC_UUID "0000fff6-0000-1000-8000-00805f9b34fc"

bool deviceConnected = false;
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
BLEService *pService = nullptr;
PPMReader *ppmReader = nullptr;
int16_t channelOutputs[CHANNEL_AMOUNT];

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

    ppmReader = new PPMReader(INTERRUPT_PIN, CHANNEL_AMOUNT);

    BLEDevice::init("Hello");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
//            BLECharacteristic::PROPERTY_WRITE |
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
    for (int i = 0; i < CHANNEL_AMOUNT; ++i) {
        uint16_t ppmValue = ppmReader->rawChannelValue(i);
        channelOutputs[i] = (int16_t) map(
                ppmValue,
                ppmReader->minChannelValue, ppmReader->maxChannelValue,
                -PPM_RANGE, PPM_RANGE);
    }

    if (deviceConnected) {
        sendTrainer();
    }

    delay(50);
}
