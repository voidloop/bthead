#ifndef OPENTX_BT_H
#define OPENTX_BT_H

#include <Arduino.h>

#define PPM_CENTER 1500

#if defined(PPM_CENTER_ADJUSTABLE)
    #define PPM_CH_CENTER(ch) (PPM_CENTER + limitAddress(ch)->ppmCenter)
#else
    #define PPM_CH_CENTER(ch) (PPM_CENTER)
#endif

void processTrainerByte(uint8_t data);
void sendTrainer();

template<class t>
inline t limit(t mi, t x, t ma) {
    return min(max(mi, x), ma);
}

enum {
    STATE_DATA_IDLE,
    STATE_DATA_START,
    STATE_DATA_XOR,
    STATE_DATA_IN_FRAME
};

#define BLUETOOTH_LINE_LENGTH           32
#define BLUETOOTH_PACKET_SIZE           14

#endif