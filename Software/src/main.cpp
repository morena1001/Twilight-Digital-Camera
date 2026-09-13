#include "Arduino.h"
#include "Preferences.h"

#include "camera.h"

#include "TFT_eSPI.h"
#include "TJpg_Decoder.h"

#define CAPTURE_PIN     1
#define SAVE_PIN        2
#define BL_PIN          3
#define SD_CARD_PIN     44
#define DB_DELAY        50 // Max delay for software debounce
#define LP_DELAY        1000 // Max delay for double press

bool Callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

Camera camera;
TFT_eSPI tft = TFT_eSPI();

bool sd_present = false;
bool photo_captured = false;
bool long_press_detected = false;
bool save_button_pressed = false;

unsigned long capture_last_db_time = 0;
unsigned long save_last_db_time = 0;
unsigned long capture_last_press_time = 0;
unsigned long save_last_press_time = 0;
unsigned long long_press_time = 0;

int capture_state = LOW;
int save_state = LOW;
int capture_last_state = LOW;
int save_last_state = LOW;

Preferences preferences;
TaskHandle_t screen_handle = NULL;
TaskHandle_t camera_handle = NULL;

void setup () {
    Serial.begin (115200);
    while (!Serial);

    if (camera.Init_Camera () != ESP_OK)    return;

    if (SD.begin (SD_CARD_PIN) && SD.cardType () != CARD_NONE)  sd_present = true;
    else    Serial.println ("Micro sd card not detected. Unable to save photos");

    pinMode (CAPTURE_PIN, INPUT_PULLUP);
    pinMode (SAVE_PIN, INPUT_PULLUP);
    pinMode (BL_PIN, INPUT_PULLDOWN);

    tft.init ();
    tft.setRotation (1);
    // tft.invertDisplay (1);
    tft.setSwapBytes(true);
    tft.fillScreen (TFT_WHITE);

    delay (100);
    
    digitalWrite (BL_PIN, HIGH);

    TJpgDec.setJpgScale (4);
    TJpgDec.setCallback (Callback);


    preferences.begin ("memory", false);
    camera.Set_Image_Count (preferences.getUInt ("counter", 1));

    Serial.println ("Begin photo capture");
}

void loop () {
    int capture_reading = digitalRead (CAPTURE_PIN);
    int save_reading = digitalRead (SAVE_PIN);

    // long press
    if (save_reading == LOW && !long_press_detected && save_button_pressed && (millis () - long_press_time) > LP_DELAY) {
        if (sd_present) {
            Serial.println ("Trying to eject sd card");
            SD.end ();
            Serial.println ("sd card safe to eject");
            sd_present = false;
        } else {
            Serial.println ("Trying to open sd card");
            if (SD.begin_wot (SD_CARD_PIN) && SD.cardType () != CARD_NONE) {
                sd_present = true;
                Serial.println ("Micro sd card detected and opened");
            } else    Serial.println ("Micro sd card not detected. Unable to save photos");
        }
        long_press_detected = true;
    }   

    if (capture_reading != capture_last_state)  capture_last_db_time = millis ();
    if (save_reading != save_last_state)  save_last_db_time = millis ();
    
    if ((millis () - capture_last_db_time) > DB_DELAY && capture_reading != capture_state) {
        capture_state = capture_reading;
        if (capture_state == LOW) {
            Serial.println ("Trying to display photo");
            esp_camera_fb_return (camera.Get_Fb ());
            camera.Set_Fb (esp_camera_fb_get ());
            if (!camera.Get_Fb ())    Serial.println ("Could not get photo buffer");
            else {
                photo_captured = true;
                TJpgDec.drawJpg (0, 0, camera.Get_Fb ()->buf, camera.Get_Fb ()->len);
                Serial.println ("Displayed");
            }
        }
    }
    
    if ((millis () - save_last_db_time) > DB_DELAY && save_reading != save_state) {
        save_state = save_reading;

        if (save_state == LOW) {
            long_press_time = millis ();
            save_button_pressed = true;
        } else {
            if ((millis () - long_press_time) <= LP_DELAY && !long_press_detected) { // short press;
                Serial.println ("Trying to save photo");
                
                if (sd_present && photo_captured) {
                    camera.Photo_Save ();
                    preferences.putUInt ("counter", camera.Get_Image_Count ());
                    esp_camera_fb_return (camera.Get_Fb ());
                    // st7789v3.Clear_Screen ();
                    // photo_captured = false;
                } else      Serial.println ("Unable to save photo, try again");
            } 
        
            long_press_detected = false;
            save_button_pressed = false;
        }
    }

    capture_last_state = capture_reading;
    save_last_state = save_reading;
}



bool Callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height ())     return false;
    tft.pushImage (x, y, w, h, bitmap);
    // st7789v3.Set_Window_Location_Size (x, width, y, length);
    // st7789v3.Draw_Pixels (bitmap, length, width);   
    return true;
}
