#include "Arduino.h"
#include "Preferences.h"

#include "camera.h"

#define CAPTURE_PIN     1
#define SD_CARD_PIN     44
#define DB_DELAY        50

Camera camera;
unsigned long capture_last_db_time = 0;
int capture_state = LOW;
int capture_last_state = LOW;

Preferences preferences;

void setup () {
    Serial.begin (115200);
    while (!Serial);

    if (camera.Init_Camera () != ESP_OK)    return;

    if (!SD.begin (SD_CARD_PIN))            return;
    if (SD.cardType () == CARD_NONE)        return;

    pinMode (CAPTURE_PIN, INPUT_PULLUP);

    preferences.begin ("memory", false);
    camera.Set_Image_Count (preferences.getUInt ("counter", 1));

    Serial.println ("Begin photo capture");
}

void loop () {
    int capture_reading = digitalRead (CAPTURE_PIN);

    if (capture_reading != capture_last_state)  capture_last_db_time = millis ();

    if ((millis () - capture_last_db_time) > DB_DELAY && capture_reading != capture_state) {
        capture_state = capture_reading;
        if (capture_state == LOW) {
            char file_name [32];
            sprintf (file_name, "/image%d.jpg", camera.Get_Image_Count ());

            camera.Photo_Save ();
            
            Serial.printf ("Saved picture: %s\n", file_name);
            preferences.putUInt ("counter", camera.Get_Image_Count ());
        }
    }

    capture_last_state = capture_reading;
}
