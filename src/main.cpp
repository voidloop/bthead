#include <Arduino.h>
#include <NimBLEDevice.h>

const int scanTime = 20; // seconds
NimBLEScan *pBLEScan;
NimBLEAdvertisedDevice *pDevice = nullptr;

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *pAdvertisedDevice) override {
        if (pAdvertisedDevice->haveServiceUUID()) {
            std::string name = pAdvertisedDevice->getName();
            std::string uuid = pAdvertisedDevice->getServiceUUID().toString();
            if (uuid == "0xfff0" && name == "Hello") {
                Serial.println(pAdvertisedDevice->toString().c_str());
                pAdvertisedDevice->getScan()->stop();
                pDevice = pAdvertisedDevice;
            }
        }
    }
};

class MyClientCallback : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *pClient) override {
        Serial.println("Client connected");
        digitalWrite(5, HIGH);
    }

    void onDisconnect(NimBLEClient *pClient) override {
        Serial.println("Client disconnected");
        digitalWrite(5, LOW);
    }
};


/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
    std::string str = isNotify ? "Notification" : "Indication";
    str += " from ";
    /** NimBLEAddress and NimBLEUUID have std::string operators */
    str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
    str += ", Value = " + std::string((char*)pData, length);
    Serial.println(str.c_str());
}

void setup() {
    Serial.begin(115200);
    Serial.println("Scanning...");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    // active scan uses more power, but get results faster
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
    if (pDevice == nullptr) {
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        Serial.printf("Devices found: %d\n", foundDevices.getCount());

        if (pDevice != nullptr) {
            Serial.println("Radio found!");
            Serial.println(pDevice->toString().c_str());

            Serial.println("Connecting...");
            BLEClient *pClient = BLEDevice::createClient();
            pClient->setClientCallbacks(new MyClientCallback());
            pClient->connect(pDevice);

            if (pClient->isConnected()) {
                BLERemoteService *pRemoteService = pClient->getService(pDevice->getServiceUUID());
                Serial.println(pRemoteService->toString().c_str());

                std::vector<BLERemoteCharacteristic *> *pCharacteristics = pRemoteService->getCharacteristics(true);
                for (auto &pCharacteristic: *pCharacteristics) {
                    Serial.println(pCharacteristic->toString().c_str());
                    if (pCharacteristic->getUUID().toString() == "0xfff6") {
                        pCharacteristic->subscribe(true, notifyCB);
                        break;
                    }
                }

            }
        } else {
            // delete results from BLEScan buffer to release memory
            pBLEScan->clearResults();
        }
    }

    delay(2000);
}