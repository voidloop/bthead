#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <PPMReader.h>
#include "opentxbt.h"

#define INTERRUPT_PIN 16
#define LED_PIN 5
#define SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fc"
#define CHARACTERISTIC_UUID "0000fff6-0000-1000-8000-00805f9b34fc"

bool deviceConnected = false;
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
BLEService *pService = nullptr;
PPMReader *ppmReader = nullptr;
uint16_t ppmOutput[CHANNEL_AMOUNT];

void write(const uint8_t *data, uint8_t length) {
    pCharacteristic->setValue((uint8_t *) data, length);
    pCharacteristic->notify();
}

void blinkTask(void *) {
    while (!deviceConnected) {
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(1900));
    }
    digitalWrite(LED_PIN, LOW);
    vTaskDelete(nullptr);
}

inline void runBlinkTask() {
    xTaskCreate(blinkTask, "blink", 1024,
                nullptr, tskIDLE_PRIORITY, nullptr);
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *) override {
        deviceConnected = true;
        Serial.println("Radio connected!");
    };

    void onDisconnect(BLEServer *) override {
        deviceConnected = false;
        Serial.println("Radio disconnected!");
        runBlinkTask();
    }
};

void printChannelOutputs() {
    for (uint8_t ch = 1; ch <= CHANNEL_AMOUNT; ++ch) {
        if (ch != 1) Serial.print(' ');
        Serial.printf("%4d", ppmOutput[ch - 1]);
        if (ch == CHANNEL_AMOUNT) Serial.println();
    }
}

inline void updateChannelOutputs() {
    for (int ch = 1; ch <= CHANNEL_AMOUNT; ++ch) {
        const uint16_t ppmValue = ppmReader->latestValidChannelValue(ch, PPM_CENTER);
        ppmOutput[ch - 1] = (uint16_t) map(ppmValue, ppmReader->minChannelValue, ppmReader->maxChannelValue,
                                           MIN_PPM_OUTPUT, MAX_PPM_OUTPUT);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(INTERRUPT_PIN, INPUT);

    ppmReader = new PPMReader(INTERRUPT_PIN, CHANNEL_AMOUNT);
    BLEDevice::init("Hello");

    delay(1000);
    updateChannelOutputs();

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Server started, waiting radio...");
    runBlinkTask();
}

void loop() {
    updateChannelOutputs();
    if (deviceConnected) {
        sendTrainer();
    }
    delay(5);
}
