#include "buttons.h"

static int lastButtonState = 0;
static int currentButtonState = 0;
static unsigned long lastDebounceTime = 0;
static unsigned long debounceDelay = 50;


int handle_button() {
    int rawValue = analogRead(BUTTON_ADC_PIN);
    int reading;
    Serial.println(rawValue);
    // Schwellenwerte, passend zu den Widerständen (1k, 10k, 100k)
    if (rawValue < 100) {       // Schwellenwert für Taste 1
        reading = 1; 
    } else if (rawValue < 600) { // Schwellenwert für Taste 2
        reading = 2; 
    } else if (rawValue < 1000) { // Schwellenwert für Taste 3
        reading = 3; 
    } else {
        reading = 0;
    }

    // Debouncing-Logik
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != currentButtonState) {
            currentButtonState = reading;
        }
    }
    lastButtonState = reading;
    
    return currentButtonState;
}