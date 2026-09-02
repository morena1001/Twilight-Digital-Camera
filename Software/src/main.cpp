#include "Arduino.h"
#include "Preferences.h"

#include "camera.h"
#include "ST7789V3.h"
#include "TJpg_Decoder.h"

#define CAPTURE_PIN     1
#define SD_CARD_PIN     44
#define DB_DELAY        50

Camera camera;
ST7789V3 st7789v3 (ST7789V3_CS_PIN, ST7789V3_DC_PIN, ST7789V3_RST_PIN);

bool sd_detected = false;
unsigned long capture_last_db_time = 0;
unsigned long capture_last_press_time = 0;
int capture_state = LOW;
int capture_last_state = LOW;

Preferences preferences;

bool callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= SCREEN_LENGTH)     return false;
    st7789v3.Set_Window_Location_Size (x, w, y, h);
    st7789v3.Draw_Pixels (bitmap, h, w);   
    return true;
}

void setup () {
    Serial.begin (115200);
    while (!Serial);

    if (camera.Init_Camera () != ESP_OK)    return;

    if (SD.begin (SD_CARD_PIN) && SD.cardType () != CARD_NONE)  sd_detected = true;
    else    Serial.println ("Micro sd card not detected. Unable to save photos");

    st7789v3.Init_ST7789V3 (true);

    // Reset screen
    st7789v3.Fill_Screen (COLOR_WHITE);

    TJpgDec.setJpgScale (4);
    TJpgDec.setCallback (callback);


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
            Serial.println ("Trying to display photo");
            camera_fb_t *fb = esp_camera_fb_get ();
            if (!fb)    Serial.println ("Could not get photo buffer");
            TJpgDec.drawJpg (RAM_WIDTH_PIC_START, RAM_LENGTH_START, fb->buf, fb->len);
            esp_camera_fb_return (fb);
            Serial.println ("Displayed");
            
            // if (sd_detected) {
            //     camera.Photo_Save ();
            //     preferences.putUInt ("counter", camera.Get_Image_Count ());
            // } else      Serial.println ("Unable to save photo, try again");
        }
    }

    capture_last_state = capture_reading;
}
