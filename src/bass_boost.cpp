#include "bass_boost.h"
#include <math.h>

#define SAMPLE_RATE 44100.0

AudioEffectBassBoost::AudioEffectBassBoost(AudioOutput* output, float gain_db, float cutoff)
    : _next(output), _gain(gain_db), _cutoff(cutoff) {
    calculateCoefficients(gain_db, cutoff);
}

AudioEffectBassBoost::~AudioEffectBassBoost() {}

bool AudioEffectBassBoost::begin() {
    return _next->begin();
}

bool AudioEffectBassBoost::SetGain(float gain_db) {
    _gain = gain_db;
    calculateCoefficients(_gain, _cutoff);
    return true;
}
void AudioEffectBassBoost::calculateCoefficients(float gain_db, float cutoff_freq) {
    // Wandelt den Gain von dB in einen linearen Faktor A um
    float A = pow(10, gain_db / 40.0);
    
    // Berechne die normalisierte Frequenz (omega) und alpha
    float omega = 2.0 * M_PI * cutoff_freq / SAMPLE_RATE;
    float cos_omega = cos(omega);
    float sin_omega = sin(omega);
    
    // Der Parameter S für die Steilheit des Shelving-Filters. 1.0 ist ein guter Standardwert.
    // In deiner alten Formel war dies durch sqrt((...)*(1.0/0.707 - 1)+2) fest einkodiert.
    // Diese Formel ist etwas sauberer.
    float alpha = sin_omega / 2.0 * sqrt( (A + 1.0/A) * (1.0/1.0 - 1.0) + 2.0 );

    // --- KORREKTE KOEFFIZIENTEN-BERECHNUNG (nach R. Bristow-Johnson) ---

    // Nenner-Koeffizienten (a)
    float a0 = (A + 1) + (A - 1) * cos_omega + 2 * sqrt(A) * alpha;
    a1 = -2 * ((A - 1) + (A + 1) * cos_omega);
    a2 = (A + 1) + (A - 1) * cos_omega - 2 * sqrt(A) * alpha;

    // Zähler-Koeffizienten (b)
    b0 = A * ((A + 1) - (A - 1) * cos_omega + 2 * sqrt(A) * alpha);
    b1 = 2 * A * ((A - 1) - (A + 1) * cos_omega);
    b2 = A * ((A + 1) - (A - 1) * cos_omega - 2 * sqrt(A) * alpha);

    // --- Normalisieren aller Koeffizienten durch a0 ---
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0; // a1 und a2 werden durch a0 geteilt, um die Form y[n] = ... - a1*y[n-1] ... zu erhalten
    a2 /= a0;
}


bool AudioEffectBassBoost::ConsumeSample(int16_t* sample) {
    // 1. Lade die int16_t-Werte
    float xL = sample[0];
    float xR = sample[1];

    // 2. Normalisieren: Skaliere den Wertebereich von int16_t auf float [-1.0, 1.0]
    float xL_norm = xL / 32768.0f;
    float xR_norm = xR / 32768.0f;

    // 3. Wende den Filter auf die normalisierten Daten an
    float yL_norm = b0 * xL_norm + b1 * xl1 + b2 * xl2 - a1 * yl1 - a2 * yl2;
    float yR_norm = b0 * xR_norm + b1 * xr1 + b2 * xr2 - a1 * yr1 - a2 * yr2;

    // 4. Aktualisiere die Filter-Zustände mit den *normalisierten* Werten
    xl2 = xl1; xl1 = xL_norm; yl2 = yl1; yl1 = yL_norm;
    xr2 = xr1; xr1 = xR_norm; yr2 = yr1; yr1 = yR_norm;
    
    // Optional, aber empfohlen: Clipping im Float-Bereich, um Hard-Clipping-Artefakte zu reduzieren
    yL_norm = fmaxf(-1.0f, fminf(1.0f, yL_norm));
    yR_norm = fmaxf(-1.0f, fminf(1.0f, yR_norm));

    // 5. De-normalisieren: Skaliere zurück in den int16_t Bereich
    int16_t outL = (int16_t)(yL_norm * 32767.0f);
    int16_t outR = (int16_t)(yR_norm * 32767.0f);

    int16_t stereoOut[2] = { outL, outR };

    return _next->ConsumeSample(stereoOut);
}