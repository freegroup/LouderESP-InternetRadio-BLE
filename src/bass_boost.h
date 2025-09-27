#pragma once
#include "AudioOutput.h"

class AudioEffectBassBoost : public AudioOutput {
public:
    AudioEffectBassBoost(AudioOutput* output, float gain = 3.0, float cutoff = 200.0);
    virtual ~AudioEffectBassBoost();
    
    virtual bool begin() override;
    virtual bool ConsumeSample(int16_t* sample) override;
    virtual bool SetGain(float gain_db);
    
protected:
    AudioOutput* _next;
    float _gain;
    float _cutoff;
    
    // Simple IIR Filter state
    float xl1 = 0, xl2 = 0, yl1 = 0, yl2 = 0;
    float xr1 = 0, xr2 = 0, yr1 = 0, yr2 = 0;
    
    float b0, b1, b2, a1, a2;

    void calculateCoefficients(float gain_db, float cutoff_freq);
};
