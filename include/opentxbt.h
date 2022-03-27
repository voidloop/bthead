#ifndef OPENTX_BT_H
#define OPENTX_BT_H

#include <Arduino.h>

#define CHANNEL_AMOUNT 8

constexpr uint16_t PPM_CENTER = 1500;
constexpr uint16_t PPM_RANGE = 512 * 2;
constexpr uint16_t MIN_PPM_OUTPUT = PPM_CENTER - PPM_RANGE / 2;
constexpr uint16_t MAX_PPM_OUTPUT = PPM_CENTER + PPM_RANGE / 2;

template<class t>
inline t limit(t mi, t x, t ma) { return min(max(mi, x), ma); }

void processTrainerByte(uint8_t data);

void sendTrainer();

enum {
    STATE_DATA_IDLE,
    STATE_DATA_START,
    STATE_DATA_XOR,
    STATE_DATA_IN_FRAME
};

#define BLUETOOTH_LINE_LENGTH           32
#define BLUETOOTH_PACKET_SIZE           14

#endif