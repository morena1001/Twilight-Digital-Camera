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

#define DISPLAY_START   0
#define DISPLAY_WIDTH   172
#define DISPLAY_HEIGHT  320

#define MSG_DISPLAY_TIME        2500
#define MSG_X                   0
#define MSG_Y                   15
#define MSG_HEIGHT              18
#define MSG_BOX_ROUNDNESS       5  
#define MSG_HORIZONTAL_PADDING  12   
#define MSG_CHAR_WIDTH          6

Camera camera;
TFT_eSPI tft = TFT_eSPI();

bool sd_present = false;
bool photo_captured = false;
bool long_press_detected = false;
bool save_button_pressed = false;
bool msg_displayed = false;

unsigned long capture_last_db_time = 0;
unsigned long save_last_db_time = 0;
unsigned long capture_last_press_time = 0;
unsigned long save_last_press_time = 0;
unsigned long long_press_time = 0;
unsigned long msg_display_time = 0;

int capture_state = LOW;
int save_state = LOW;
int capture_last_state = LOW;
int save_last_state = LOW;

Preferences preferences;
TaskHandle_t screen_handle = NULL;
TaskHandle_t camera_handle = NULL;

enum Message { SD_EJECTABLE, SD_DISCOVERED, NO_SD_DISCOVERED, PHOTO_SAVED, PHOTO_NOT_SAVED };

bool Callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
void Display_Message (Message msg, uint16_t img_num = 0);

void setup () {
    Serial.begin (115200);
    while (!Serial);

    if (camera.Init_Camera () != ESP_OK)    return;

    if (SD.begin (SD_CARD_PIN) && SD.cardType () != CARD_NONE)  sd_present = true;
    else    Serial.println ("Micro sd card not detected. Unable to save photos");

    pinMode (CAPTURE_PIN, INPUT_PULLUP);
    pinMode (SAVE_PIN, INPUT_PULLUP);
    pinMode (BL_PIN, INPUT_PULLUP);

    digitalWrite (BL_PIN, LOW);

    tft.init ();
    tft.setRotation (1);
    // tft.invertDisplay (1);
    tft.setSwapBytes(true);
    tft.setTextSize (1);
    tft.setTextColor (TFT_WHITE);

    delay (500);
    tft.pushImage (DISPLAY_START, DISPLAY_START, DISPLAY_HEIGHT, DISPLAY_WIDTH, splash_screen);
    digitalWrite (BL_PIN, HIGH);
    
    TJpgDec.setJpgScale (4);
    TJpgDec.setCallback (Callback);
    
    preferences.begin ("memory", false);
    camera.Set_Image_Count (preferences.getUInt ("counter", 1));
    
    delay (1000);    
    tft.fillScreen (TFT_BLACK);
    // tft.fillScreen (TFT_BLUE);

    // Display_Message (PHOTO_SAVED, 200);

    Serial.println ("Begin photo capture");
}

void loop () {
    if (msg_displayed && (millis () - msg_display_time) > MSG_DISPLAY_TIME) {
        if (photo_captured)     TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb ()->buf, camera.Get_Fb ()->len);
        else                    tft.fillScreen (TFT_BLACK);

        msg_displayed = false;
    }

    int capture_reading = digitalRead (CAPTURE_PIN);
    int save_reading = digitalRead (SAVE_PIN);

    // long press
    if (save_reading == LOW && !long_press_detected && save_button_pressed && (millis () - long_press_time) > LP_DELAY) {
        if (sd_present) {
            Serial.println ("Trying to eject sd card");
            SD.end ();
            Serial.println ("sd card safe to eject");
            Display_Message (SD_EJECTABLE);
            sd_present = false;
        } else {
            Serial.println ("Trying to open sd card");
            if (SD.begin_wot (SD_CARD_PIN) && SD.cardType () != CARD_NONE) {
                sd_present = true;
                Serial.println ("Micro sd card detected and opened");
                Display_Message (SD_DISCOVERED);
            } else {
                Serial.println ("Micro sd card not detected. Unable to save photos");
                Display_Message (NO_SD_DISCOVERED);
            }
        }
        long_press_detected = true;
    }   

    if (capture_reading != capture_last_state)  capture_last_db_time = millis ();
    if (save_reading != save_last_state)  save_last_db_time = millis ();
    
    if ((millis () - capture_last_db_time) > DB_DELAY && capture_reading != capture_state) {
        capture_state = capture_reading;
        if (capture_state == LOW) {
            Serial.println ("Trying to display photo");
            if (photo_captured) {
                esp_camera_fb_return (camera.Get_Fb ());
                photo_captured = false;
            }
            camera.Set_Fb (esp_camera_fb_get ());
            if (!camera.Get_Fb ())    Serial.println ("Could not get photo buffer");
            else {
                msg_displayed = false;
                photo_captured = true;
                TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb ()->buf, camera.Get_Fb ()->len);
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
                    tft.fillScreen (TFT_BLACK);
                    Display_Message (PHOTO_SAVED, camera.Get_Image_Count () - 1);
                    photo_captured = false;
                } else {
                    Serial.println ("Unable to save photo, try again");
                    Display_Message (PHOTO_NOT_SAVED);
                }
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
    return true;
}

void Display_Message (Message msg, uint16_t img_num) {
    // Calculate final length of bounding box
    uint8_t char_length = 0;
    switch (msg) {
        case SD_EJECTABLE:      char_length = 22; break;
        case SD_DISCOVERED:     char_length = 17; break;
        case NO_SD_DISCOVERED:  char_length = 21; break;
        case PHOTO_SAVED: {
            if (img_num >= 10000)       char_length += 5;
            else if (img_num >= 1000)   char_length += 4;
            else if (img_num >= 100)    char_length += 3;
            else if (img_num >= 10)     char_length += 2;
            else if (img_num >= 1)      char_length += 1;
        } 
        case PHOTO_NOT_SAVED:   char_length += 24; break;
    }

    // Print bound box
    tft.fillSmoothRoundRect (MSG_X, MSG_Y, (MSG_HORIZONTAL_PADDING + (char_length * MSG_CHAR_WIDTH)), MSG_HEIGHT, MSG_BOX_ROUNDNESS, TFT_DARKGREY, TFT_DARKGREY);

    // Print characters
    tft.setCursor (5, 20);
    switch (msg) {
        case SD_EJECTABLE:      tft.print  ("SD card can be ejected"); break;
        case SD_DISCOVERED:     tft.print  ("SD card connected"); break;
        case NO_SD_DISCOVERED:  tft.print  ("SD card not connected"); break;
        case PHOTO_SAVED:       tft.printf ("Photo saved as image%d.jpg", img_num); break;
        case PHOTO_NOT_SAVED:   tft.print  ("Photo could not be saved"); break;
    }

    msg_displayed = true;
    msg_display_time = millis ();
}
