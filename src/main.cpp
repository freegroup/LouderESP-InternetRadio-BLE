#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>

#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#include "BluetoothA2DPSink.h"
#include "BluetoothA2DPSink.h"
#include "BluetoothA2DPOutput.h"

#include "buttons.h"
#include "encoder.h"
#include "amplifier.h" 
#include "bass_boost.h"

// --- Globale Objekte ---
Preferences preferences;
volatile bool switchToBluetoothRequested = false;

// Radio-spezifische Objekte
AudioFileSourceICYStream *file_stream = nullptr;
AudioFileSourceBuffer *buffer = nullptr;
AudioGeneratorMP3 *mp3_player = nullptr;
AudioOutputI2S *i2s_output_radio = nullptr;
AudioEffectBassBoost* bass_boost = nullptr;

// Bluetooth-spezifische Objekte
BluetoothA2DPSink a2dp_sink;
BluetoothA2DPOutputLegacy *i2s_output_bluetooth = nullptr; 

WiFiManager wm;

String current_radio_url = "";
bool is_bass_boost_active = false;
bool radio_is_playing = false;

const char* radio_urls[] = {
    "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3", 
    "http://st03.dlf.de/dlf/03/128/mp3/stream.mp3",   
    "http://www.radioeins.de/livemp3"
};

enum AudioMode {
    MODE_RADIO,
    MODE_BLUETOOTH,
    MODE_IDLE
};
AudioMode active_mode; 

// --- Funktions-Deklarationen ---
void handleModeChange(AudioMode new_mode, String url);

// Wir reaktivieren den Callback aus einem früheren Schritt und passen ihn an
void connection_state_callback(esp_a2d_connection_state_t state, void *object) {
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        // Hole die Adresse des aktuell verbundenen Geräts
        esp_bd_addr_t* bd_addr = a2dp_sink.get_current_peer_address();
        if (bd_addr) {
            char mac_str[18];
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (*bd_addr)[0], (*bd_addr)[1], (*bd_addr)[2],
                    (*bd_addr)[3], (*bd_addr)[4], (*bd_addr)[5]);
            // Speichere mac_str in den Preferences
            preferences.begin("audio_config", false);
            preferences.putString("last_bt_addr", mac_str);
            preferences.end();
            Serial.printf("Adresse %s für Reconnect gespeichert.\n", mac_str);
        }

        // Wenn wir nicht im Bluetooth-Modus waren, forciere einen Neustart in den Bluetooth-Modus:
        if (active_mode != MODE_BLUETOOTH) {
            Serial.println("Verbindung im Radio-Modus erkannt. Leite Neustart in den Bluetooth-Modus ein.");
            switchToBluetoothRequested = true;
        }
    }
    else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
        Serial.println(">>> BT-Verbindung im Callback getrennt.");
    }
}

void stopRadioStream() {
    // Stoppt nur, wenn auch wirklich etwas läuft
    if (!radio_is_playing && !(mp3_player && mp3_player->isRunning())) {
        return;
    }
    Serial.println("Stoppe Radio-Stream und räume Audio-Objekte auf...");
    if (mp3_player) {
        mp3_player->stop();
        delete mp3_player; 
        mp3_player = nullptr;
    }
    if (buffer) {
        delete buffer;
        buffer = nullptr;
    }
    if (file_stream) {
        delete file_stream;
        file_stream = nullptr;
    }
    radio_is_playing = false;
    Serial.println("Radio-Stream gestoppt.");
}


void startRadioStream() {
    // Stellt sicher, dass alles sauber ist, bevor wir starten
    stopRadioStream();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Kein WLAN, kann Radio nicht starten.");
        return;
    }
    
    // Gespeicherte URL laden
    preferences.begin("audio_config", true);
    String radio_url = preferences.getString("radio_url", radio_urls[0]); // Default auf erste URL
    preferences.end();
    Serial.printf("Versuche, Radio von URL zu starten: %s\n", radio_url.c_str());

    // Radio-Stream aufbauen
    file_stream = new AudioFileSourceICYStream(radio_url.c_str());
    buffer = new AudioFileSourceBuffer(file_stream, 16384);
    mp3_player = new AudioGeneratorMP3();

    AudioOutput* final_output;
    if (is_bass_boost_active) {
        Serial.println("Audio-Ziel: Bass-Boost -> I2S");
        final_output = bass_boost;
    } else {
        Serial.println("Audio-Ziel: Direkter I2S-Ausgang (Bypass)");
        final_output = i2s_output_radio;
    }

    if (mp3_player->begin(buffer, final_output)) {
        radio_is_playing = true;
        Serial.println("Radio-Stream erfolgreich gestartet.");
    } else {
        Serial.println("Fehler beim Starten des MP3-Streams. Räume wieder auf.");
        stopRadioStream();
    }
}


// Die Funktion heißt jetzt handleModeChange und ist intelligenter.
void handleModeChange(AudioMode new_mode, String url) {
    if (new_mode == MODE_IDLE) {
        Serial.println("Wechsle in den IDLE-Modus: Kein WLAN, nur BT im Lauschmodus.");

        preferences.begin("audio_config", false);
        preferences.putUInt("audio_mode", new_mode);
        preferences.end();

        // Deaktiviere WLAN
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        // Stoppe evtl. Radio
        stopRadioStream();

        // Starte Bluetooth im Lauschmodus
        a2dp_sink.set_output_active(false);
        a2dp_sink.start(BT_DEVICE_NAME);

        active_mode = MODE_IDLE;
        return;
    }

    if (active_mode == MODE_RADIO && new_mode == MODE_RADIO) {
        Serial.println("=> Weicher Senderwechsel wird durchgeführt (kein Neustart)...");
        if (url == current_radio_url) {
            Serial.println("Gleicher Sender: Schalte Bass-Boost um...");
            is_bass_boost_active = !is_bass_boost_active; // Zustand umkehren
            startRadioStream();
        } else {
            current_radio_url = url;
            preferences.begin("audio_config", false);
            preferences.putString("radio_url", url);
            preferences.end();
            startRadioStream(); 
        }
        return;
    }

    // --- Standard-Logik für einen kompletten Moduswechsel mit Neustart ---
    // (Wird für Radio->BT oder BT->Radio verwendet)
    Serial.printf("Kompletter Moduswechsel wird vorbereitet: %d\n", new_mode);

    preferences.begin("audio_config", false);
    preferences.putUInt("audio_mode", new_mode);
    if (new_mode == MODE_RADIO && url != "") {
        preferences.putString("radio_url", url);
        Serial.printf("Radio-URL gespeichert: %s\n", url.c_str());
    }
    preferences.end();
    
    ESP.restart();
}


// --- Hauptprogramm ---

void setup() {
    Serial.begin(115200);
    if (psramFound()) Serial.println("PSRAM gefunden.");

 
    preferences.begin("audio_config", true);
    active_mode = (AudioMode)preferences.getUInt("audio_mode", MODE_RADIO); 
    preferences.end();
    
    Serial.printf("\n--- Starte im Modus: %d ---\n\n", active_mode);
    
    if (active_mode == MODE_BLUETOOTH) {
        Serial.println("Bluetooth-Modus wird initialisiert...");

        // Erstellen der Standard I2S-Output-Instanz
        i2s_output_bluetooth = new BluetoothA2DPOutputLegacy();
        i2s_pin_config_t pinCfg = {
            .bck_io_num   = PIN_I2S_SCK,
            .ws_io_num    = PIN_I2S_FS,
            .data_out_num = PIN_I2S_SD,
            .data_in_num  = I2S_PIN_NO_CHANGE
        };
        i2s_output_bluetooth->set_pin_config(pinCfg);
        i2s_output_bluetooth->set_i2s_port((i2s_port_t)0);

        // Verstärker initialisieren, nachdem der I2S-Bus konfiguriert wurde.
        init_amplifier();
        init_encoder();

        a2dp_sink.set_output(*i2s_output_bluetooth);
        a2dp_sink.start(BT_DEVICE_NAME); 
        Serial.println("Bluetooth-Modus initialisiert. Bereit zum Verbinden.");

        // 1) Kurzes Delay, damit der A2DP-Stack vollständig hochfährt
        delay(500);

        // override the save volume for Bluetooth controll. The volume control happens 
        // on the cell phone. set 80% on our board amp
        //
        set_amplifier_temp_volume(90);

        // 2) gespeicherte MAC-Adresse aus Preferences holen
        preferences.begin("audio_config", true);
        String lastMac = preferences.getString("last_bt_addr", "");
        preferences.end();

        if (lastMac.length() == 17) {
            // 3) In esp_bd_addr_t umwandeln
            esp_bd_addr_t peer_addr;
            // Beispiel: "ab:cd:ef:12:34:56"
            int values[6];
            if (sscanf(lastMac.c_str(), "%x:%x:%x:%x:%x:%x",
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5]) == 6) {
                for (int i = 0; i < 6; ++i) {
                    peer_addr[i] = (uint8_t) values[i];
                }
                // 4) aktiver Verbindungsversuch
                bool ok = a2dp_sink.connect_to(peer_addr);
                if (ok) {
                    Serial.printf("Autoconnect zu %s gestartet...\n", lastMac.c_str());
                } else {
                    Serial.printf("Autoconnect fehlgeschlagen für Adresse %s\n", lastMac.c_str());
                }
            } else {
                Serial.println("Fehler beim Parsen der gespeicherten MAC-Adresse.");
            }
        } else {
            Serial.println("Keine gültige gespeicherte MAC-Adresse; warte auf Telefon-Verbindung.");
        }
    } 
    else if (active_mode == MODE_RADIO) {
        Serial.println("Radio-Modus wird initialisiert...");

        // Radio-spezifisches I2S-Objekt erstellen und starten
        i2s_output_radio = new AudioOutputI2S(0, AudioOutputI2S::EXTERNAL_I2S, 20);
        i2s_output_radio->SetPinout(PIN_I2S_SCK, PIN_I2S_FS, PIN_I2S_SD);

        i2s_output_radio->begin();

        bass_boost = new AudioEffectBassBoost(i2s_output_radio, 10.0f, 140.0f); // dB, Hz

        preferences.begin("audio_config", true);
        current_radio_url = preferences.getString("radio_url", radio_urls[0]);
        preferences.end();

        init_amplifier();
        init_encoder();

        // WLAN verbinden
        wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_MS / 1000);
        wm.setConfigPortalTimeout(180);
        if (!wm.autoConnect(AP_SSID, AP_PASSWORD)) {
            Serial.println("WLAN-Verbindung fehlgeschlagen...");
        }
        
        // Stream-Start wird an die Hilfsfunktion übergeben
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Verbunden mit WLAN: "); Serial.println(WiFi.SSID());
            startRadioStream();
        } else {
            Serial.println("Keine WLAN-Verbindung, Radio wird nicht gestartet.");
        }

        // Starte den zusätzlichen "Lausch-Modus" für Bluetooth ---
        Serial.println("Starte zusätzlichen Bluetooth-Stack im 'Lausch-Modus'...");

        // Callback registrieren, um auf Verbindungen zu reagieren
        a2dp_sink.set_on_connection_state_changed(connection_state_callback);
        a2dp_sink.set_output_active(false);
        a2dp_sink.start(BT_DEVICE_NAME);
    }
    else if (active_mode == MODE_IDLE) {
        Serial.println("Starte im IDLE-Modus (nur passives Bluetooth).");
        init_encoder();

        a2dp_sink.set_output_active(false);
        a2dp_sink.set_on_connection_state_changed(connection_state_callback);
        a2dp_sink.start(BT_DEVICE_NAME);
    }
}

int last_button_state = 0;

void loop() {

    if (switchToBluetoothRequested) {
        switchToBluetoothRequested = false;
        handleModeChange(MODE_BLUETOOTH, "");
    }

    // Aktionen für den aktuellen Modus ausführen
    if (active_mode == MODE_RADIO) {
        if (mp3_player && mp3_player->isRunning()) {
            // Fall 1: Alles läuft normal.
            if (!mp3_player->loop()) {
                Serial.println("Radio-Stream beendet oder unterbrochen.");
                stopRadioStream();
                // restart des streams erfolgt in dem zweite durchlaufen der loop()
            }
        } 
        else {
            // Fall 2: Der Player sollte laufen, tut es aber nicht -> Reconnect versuchen.
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Radio spielt nicht, versuche Neustart des Streams...");
                delay(200);
                startRadioStream();
            } else {
                Serial.println("Warte auf WLAN-Verbindung...");
                delay(200);
            }
        }
    } 


    int increment = handle_encoder();
    if (increment != 0) { // Nur reagieren, wenn der Regler gedreht wurde
        if (active_mode == MODE_RADIO) {
            // Im Radio-Modus: Steuere den lokalen Verstärker
            inc_amplifier(increment);
        } 
        else if (active_mode == MODE_BLUETOOTH) {
            int current = a2dp_sink.get_volume();
            int newVolume = constrain(current + increment, 0, 127);
            a2dp_sink.set_volume(newVolume);
            Serial.printf("Aktuelle Lautstärke: %d\n", a2dp_sink.get_volume());
        }
    }


    // Auf eine *neue* Tastendruck-Aktion reagieren
    int button    = handle_button();
    if (button != last_button_state && button > 0) {
        if (button == 1) {
            handleModeChange(MODE_IDLE, ""); 
        }
        else if (button >= 2 && button <= 3) {
            int url_index = button - 1;
            handleModeChange(MODE_RADIO, radio_urls[url_index]);
        }
    }
    last_button_state = button;

    delay(10);
}