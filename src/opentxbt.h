#ifndef OPENTXBT_H
#define OPENTXBT_H

#include <Arduino.h>

void processTrainerByte(uint8_t data);

enum BluetoothStates {
#if defined(PCBX9E)
    BLUETOOTH_INIT,
    BLUETOOTH_WAIT_TTM,
    BLUETOOTH_WAIT_BAUDRATE_CHANGE,
#endif
    BLUETOOTH_STATE_OFF,
    BLUETOOTH_STATE_FACTORY_BAUDRATE_INIT,
    BLUETOOTH_STATE_BAUDRATE_SENT,
    BLUETOOTH_STATE_BAUDRATE_INIT,
    BLUETOOTH_STATE_NAME_SENT,
    BLUETOOTH_STATE_POWER_SENT,
    BLUETOOTH_STATE_ROLE_SENT,
    BLUETOOTH_STATE_IDLE,
    BLUETOOTH_STATE_DISCOVER_REQUESTED,
    BLUETOOTH_STATE_DISCOVER_SENT,
    BLUETOOTH_STATE_DISCOVER_START,
    BLUETOOTH_STATE_DISCOVER_END,
    BLUETOOTH_STATE_BIND_REQUESTED,
    BLUETOOTH_STATE_CONNECT_SENT,
    BLUETOOTH_STATE_CONNECTED,
    BLUETOOTH_STATE_DISCONNECTED,
    BLUETOOTH_STATE_CLEAR_REQUESTED,
    BLUETOOTH_STATE_FLASH_FIRMWARE
};

enum {
    STATE_DATA_IDLE,
    STATE_DATA_START,
    STATE_DATA_XOR,
    STATE_DATA_IN_FRAME
};

#define BLUETOOTH_LINE_LENGTH           32
#define BLUETOOTH_PACKET_SIZE           14

#endif