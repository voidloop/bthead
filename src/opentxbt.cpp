/* From OpenTX 2.3.1 */
#include "opentxbt.h"

constexpr uint8_t START_STOP = 0x7E;
constexpr uint8_t BYTE_STUFF = 0x7D;
constexpr uint8_t STUFF_MASK = 0x20;

uint8_t crc = 0x00;
uint8_t buffer[BLUETOOTH_LINE_LENGTH + 1];
uint8_t bufferIndex = 0;
uint8_t dataState = STATE_DATA_IDLE;

extern uint16_t ppmInput[CHANNEL_AMOUNT];
extern int16_t channelOutputs[CHANNEL_AMOUNT];
extern void write(const uint8_t *data, uint8_t length);

void appendTrainerByte(uint8_t data) {
    if (bufferIndex < BLUETOOTH_LINE_LENGTH) {
        buffer[bufferIndex++] = data;
        // we check for "DisConnected", but the first byte could be altered (if received in state STATE_DATA_XOR)
        if (data == '\n') {
            bufferIndex = 0;
        }
    }
}

void processTrainerFrame(const uint8_t *buf) {
    for (uint8_t channel = 0, i = 1; channel < CHANNEL_AMOUNT; channel += 2, i += 3) {
        // +-500 != 512, but close enough.
        ppmInput[channel] = buf[i] + ((buf[i + 1] & 0xf0) << 4);
        ppmInput[channel + 1] = ((buf[i + 1] & 0x0f) << 4) + ((buf[i + 2] & 0xf0) >> 4) + ((buf[i + 2] & 0x0f) << 8);
    }
}

void processTrainerByte(uint8_t data) {
    switch (dataState) {
        case STATE_DATA_START:
            if (data == START_STOP) {
                dataState = STATE_DATA_IN_FRAME;
                bufferIndex = 0;
            } else {
                appendTrainerByte(data);
            }
            break;

        case STATE_DATA_IN_FRAME:
            if (data == BYTE_STUFF) {
                dataState = STATE_DATA_XOR; // XOR next byte
            } else if (data == START_STOP) {
                dataState = STATE_DATA_IN_FRAME;
                bufferIndex = 0;
            } else {
                appendTrainerByte(data);
            }
            break;

        case STATE_DATA_XOR:
            appendTrainerByte(data ^ STUFF_MASK);
            dataState = STATE_DATA_IN_FRAME;
            break;

        case STATE_DATA_IDLE:
            if (data == START_STOP) {
                bufferIndex = 0;
                dataState = STATE_DATA_START;
            } else {
                appendTrainerByte(data);
            }
            break;
        default:
            break;
    }

    if (bufferIndex >= BLUETOOTH_PACKET_SIZE) {
        crc = 0x00;
        for (int i = 0; i < 13; i++) {
            crc ^= buffer[i];
        }
        if (crc == buffer[13]) {
            if (buffer[0] == 0x80) {
                processTrainerFrame(buffer);
            }
        }
        dataState = STATE_DATA_IDLE;
    }
}

void pushByte(uint8_t byte) {
    crc ^= byte;
    if (byte == START_STOP || byte == BYTE_STUFF) {
        buffer[bufferIndex++] = BYTE_STUFF;
        byte ^= STUFF_MASK;
    }
    buffer[bufferIndex++] = byte;
}

void sendTrainer() {
    const int firstCh = 0;
    const int lastCh = firstCh + CHANNEL_AMOUNT;

    uint8_t *cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
    for (int channel = firstCh; channel < lastCh; channel += 2, cur += 3) {
        uint16_t channelValue1 =
                PPM_CH_CENTER(channel) +
                limit((int16_t) -PPM_RANGE, channelOutputs[channel], (int16_t) PPM_RANGE) / 2;

        uint16_t channelValue2 =
                PPM_CH_CENTER(channel + 1) +
                limit((int16_t) -PPM_RANGE, channelOutputs[channel + 1], (int16_t) PPM_RANGE) / 2;

        pushByte(channelValue1 & 0x00ff);
        pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
        pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
    }
    buffer[bufferIndex++] = crc;
    buffer[bufferIndex++] = START_STOP; // end byte

    write(buffer, bufferIndex);
    bufferIndex = 0;
}