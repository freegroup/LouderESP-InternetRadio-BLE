#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>


/**
 * @brief Liest den Zustand der Widerstandsleiter und gibt die gedrückte Taste zurück.
 * Beinhaltet eine nicht-blockierende Entprellung (Debouncing).
 * @return int Die Nummer der gedrückten Taste (1-4). Gibt 0 zurück, wenn keine Taste gedrückt ist.
 */
int handle_button();

#endif // BUTTONS_H