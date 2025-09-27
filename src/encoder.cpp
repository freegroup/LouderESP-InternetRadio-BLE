#include <Arduino.h>
#include <ESP32Encoder.h>

#include "encoder.h"

// --- Static (private) Objects and Variables ---
static ESP32Encoder encoder;
static long lastEncoderCount = 0;


void init_encoder() {
    pinMode(ENC_PIN_A, INPUT_PULLUP);
    pinMode(ENC_PIN_B, INPUT_PULLUP);
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachFullQuad(ENC_PIN_A, ENC_PIN_B);
    encoder.clearCount();
    Serial.println("Encoder initialisiert.");
}


int handle_encoder() {
    long newEncoderCount = encoder.getCount();
    if (newEncoderCount != lastEncoderCount) {
        long inc = lastEncoderCount - newEncoderCount;
        lastEncoderCount = newEncoderCount;
        return inc;
    }
    return 0;
}

