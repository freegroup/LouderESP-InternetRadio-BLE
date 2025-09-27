#include <Arduino.h>
#include <tas5805m.hpp>
#include <Wire.h>
#include <Preferences.h>
#include "amplifier.h"

extern Preferences preferences;
static tas5805m amplifier(&Wire);
static int currentVolume = TAS5805M_VOLUME_DEFAULT;  // z.B. 48 = 0 dB

void init_amplifier() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    esp_err_t err = amplifier.init();
    if (err != ESP_OK) {
        Serial.printf("TAS5805M Init fehlgeschlagen! Fehlercode: 0x%X (%s)\n", err, esp_err_to_name(err));
    } else {
        amplifier.setVolume(currentVolume);
        Serial.println("TAS5805M erfolgreich initialisiert.");
    }

    preferences.begin("audio_config", true);
    currentVolume = preferences.getUInt("audio_volume", TAS5805M_VOLUME_DEFAULT);
    preferences.end();

    amplifier.setVolume(TAS5805M_VOLUME_MAX -currentVolume);
}

void inc_amplifier(int inc) {
    currentVolume = constrain(currentVolume + inc, TAS5805M_VOLUME_MIN, TAS5805M_VOLUME_MAX);
    amplifier.setVolume(TAS5805M_VOLUME_MAX - currentVolume);
    preferences.begin("audio_config", false);
    preferences.putUInt("audio_volume", currentVolume);
    preferences.end();
}

void set_amplifier_temp_volume(uint8_t percent) {
    percent = constrain(percent, 0, 100);
    int vol = map(percent,0, 100, TAS5805M_VOLUME_MIN, TAS5805M_VOLUME_MAX);
    currentVolume = vol;
    Serial.println(TAS5805M_VOLUME_MAX - currentVolume);
    amplifier.setVolume(TAS5805M_VOLUME_MAX - currentVolume);
}
