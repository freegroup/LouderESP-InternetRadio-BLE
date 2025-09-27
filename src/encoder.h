#ifndef ENCODER_H
#define ENCODER_H

// --- Public Function Declarations ---

/**
 * @brief Initialisiert den Rotary Encoder und den TAS5805M Verstärker.
 * Muss einmal in der setup()-Funktion aufgerufen werden.
 */
void init_encoder();

/**
 * @brief Prüft den Encoder auf Änderungen und passt die Lautstärke an.
 * Muss regelmäßig in der loop()-Funktion aufgerufen werden.
 */
int handle_encoder();

#endif // ENCODER_H