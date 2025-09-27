#ifndef AMPLIFIER_H
#define AMPLIFIER_H



/**
 * @brief Initialisiert den Rotary Encoder und den TAS5805M Verstärker.
 * Muss einmal in der setup()-Funktion aufgerufen werden.
 */
void init_amplifier();

void inc_amplifier(int inc);

void set_amplifier_temp_volume(uint8_t percent);


#endif // AMPLIFIER_H