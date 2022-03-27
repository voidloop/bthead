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
int16_t channelOutputs[CHANNEL_AMOUNT];

void write(const uint8_t *data, uint8_t length) {
    pCharacteristic->setValue((uint8_t *) data, length);
    pCharacteristic->notify();
}

void blinkTask(void *) {
    while (!deviceConnected) {
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
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

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(INTERRUPT_PIN, INPUT);

    ppmReader = new PPMReader(INTERRUPT_PIN, CHANNEL_AMOUNT);
    runBlinkTask();

    BLEDevice::init("Hello");

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
}

void loop() {
    for (uint8_t ch = 1; ch <= CHANNEL_AMOUNT; ++ch) {
        const uint16_t ppmValue = ppmReader->latestValidChannelValue(ch, PPM_CENTER);
        channelOutputs[ch - 1] = (int16_t) map(
                ppmValue,
                ppmReader->minChannelValue, ppmReader->maxChannelValue,
                -PPM_RANGE, PPM_RANGE);

        if (ch != 1) Serial.print(' ');
        Serial.printf("%4d", ppmValue);
        if (ch == CHANNEL_AMOUNT) Serial.println();
    }

    if (deviceConnected) {
        sendTrainer();
    }

    delay(50);
}
