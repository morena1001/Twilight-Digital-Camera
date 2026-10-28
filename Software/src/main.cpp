#include "Arduino.h"
#include "Preferences.h"

#include "camera.h"

#include "TFT_eSPI.h"
#include "PNGenc.h"

#define CAPTURE_PIN     1
#define SAVE_PIN        2
#define BL_PIN          3
#define SD_CARD_PIN     44
#define DB_DELAY        50 // Max delay for software debounce
#define LP_DELAY        1000 // Max delay for double press

#define DISPLAY_START   0
#define DISPLAY_WIDTH   172
#define DISPLAY_HEIGHT  320
#define IMAGE_WIDTH     640   
#define IMAGE_HEIGHT    480   

#define MSG_DISPLAY_TIME        2500
#define MSG_X                   0
#define MSG_Y                   15
#define MSG_HEIGHT              18
#define MSG_BOX_ROUNDNESS       5  
#define MSG_HORIZONTAL_PADDING  12   
#define MSG_CHAR_WIDTH          6
#define PROG_NUM_X              53
#define PROG_NUM_Y              20

#define DISPLAY_JPG_SCALE       4
#define FILE_JPG_SCALE          1

#define WARMTH_COEF             -0.15f // ((-100.0f / 100.0f) * 0.15f)
#define TINT_COEF               -0.09f // ((-40.0f / 100.0f) * 0.15f)
#define MIDPOINT                128.0f
#define CONTRAST_FACTOR         1.05f

Camera camera;
TFT_eSPI tft = TFT_eSPI();
PNGENC png;
File png_file;
uint8_t png_truecolor[640 * 3];

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

enum Message { SD_EJECTABLE, SD_DISCOVERED, NO_SD_DISCOVERED, PHOTO_SAVED, PHOTO_NOT_SAVED, SHOW_PROGRESS, UPDATE_PROGRESS };

void Display_Message (Message msg, uint16_t img_num = 0);
void Compress_To_Display (uint8_t *buffer);

void * My_Open (const char *filename) {
    png_file = SD.open(filename, "w+r"); // This option is needed to both read and write
  return &png_file;
}

void My_Close (PNGFILE *handle) {
    File *f = (File *) handle->fHandle;
    f->flush ();
    f->close ();
}

int32_t My_Read (PNGFILE *handle, uint8_t *buffer, int32_t length) {
    int32_t iBytesRead;
    iBytesRead = length;
    File *f = static_cast<File *>(handle->fHandle);
    iBytesRead = (int32_t)f->read(buffer, iBytesRead);
    handle->iPos = f->position();
    return iBytesRead;
}

int32_t My_Write (PNGFILE *handle, uint8_t *buffer, int32_t length) {    
    File *f = (File *) handle->fHandle;
    handle->iPos += length;
    return f->write (buffer, length);
}

int32_t My_Seek (PNGFILE *handle, int32_t position) {
    File *f = (File *)handle->fHandle;
    f->seek(position);
    handle->iPos = (int)f->position();
    return handle->iPos;
}

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
    
    preferences.begin ("memory", false);
    camera.Set_Image_Count (preferences.getUInt ("counter", 1));

    char file_name [32];
    for (uint32_t i = 1; i < 4294967295; i++) {
        sprintf (file_name, "/image%d.jpg", i);
        if (!SD.exists (file_name)) {
            camera.Set_Image_Count (i);
            Serial.printf ("FOUND IT : %d\r\n", i);
            break;
        }
    }
    
    delay (1000);    
    tft.fillScreen (TFT_BLACK);

    if (sd_present)     Display_Message (SD_DISCOVERED);
    else                Display_Message (NO_SD_DISCOVERED);

    Serial.println ("Begin photo capture");
}

void loop () {
    if (msg_displayed && (millis () - msg_display_time) > MSG_DISPLAY_TIME) {
        if (photo_captured)     Compress_To_Display (camera.Get_Fb_Buf ());// TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb_Buf (), camera.Get_Fb_Len ());
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
            esp_camera_fb_return (camera.Get_Fb ());
            camera.Set_Fb (esp_camera_fb_get ());
            if (!camera.Get_Fb ())    Serial.println ("Could not get photo buffer");
            else {
                msg_displayed = false;
                photo_captured = true;
                Serial.printf ("%d\r\n", camera.Get_Fb ()->len);
                camera.Set_Fb_Buf (camera.Get_Fb ()->buf);
                camera.Set_Fb_Len (camera.Get_Fb ()->len);
                Compress_To_Display (camera.Get_Fb_Buf ());
                Serial.println ("Displayed");
                // esp_camera_fb_return (camera.Get_Fb ());
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
                    char name [32];
                    for (uint32_t i = camera.Get_Image_Count (); i < 4294967295; i++) {
                        sprintf (name, "/image%d.jpg", i);
                        if (!SD.exists (name)) {
                            camera.Set_Image_Count (i);
                            break;
                        }
                    }
                    Serial.printf ("CHOOSING NUMBER : %d\r\n", camera.Get_Image_Count ());
                    sprintf (name, "/image%d.jpg", camera.Get_Image_Count ());
                    int rc;
                    rc = png.open (name, My_Open, My_Close, My_Read, My_Write, My_Seek);
                    if (rc == PNG_SUCCESS) {
                        Serial.println ("PNG file opened successfully");

                        rc = png.encodeBegin (640, 480, PNG_PIXEL_TRUECOLOR, 24, NULL, 5);
                        if (rc == PNG_SUCCESS) {
                            Serial.println ("PNG encode began successfully");

                            uint8_t *buffer = camera.Get_Fb_Buf ();
                            uint16_t pixel;
                            uint8_t R, G, B;

                            for (uint16_t i = 0; i < IMAGE_HEIGHT; i++) {
                                for (uint16_t j = 0; j < IMAGE_WIDTH; j++) {
                                    pixel = ((uint16_t) buffer[(i * 1280) + (j * 2)] << 8) | buffer[(i * 1280) + (j * 2) + 1];
                                    // R = ((pixel >> 11) & 0x1F);
                                    // G = ((pixel >> 5) & 0x3F);
                                    // B = (pixel & 0x1F);

                                    R = ((((pixel >> 11) & 0x1F) * 527) + 23) >> 6;
                                    G = ((((pixel >> 5) & 0x3F) * 259) + 33) >> 6;
                                    B = (((pixel & 0x1F) * 527) + 23) >> 6;
                                    
                                    png_truecolor[j * 3 + 0] = (uint8_t) ((float) (R) + ((float) (R) * WARMTH_COEF) + ((float) (R) * TINT_COEF));
                                    png_truecolor[j * 3 + 1] = G;
                                    png_truecolor[j * 3 + 2] = (uint8_t) ((float) (B) + ((float) (B) * WARMTH_COEF) + ((float) (B) * TINT_COEF));

                                    // png_truecolor[j * 3 + 0] = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 0]) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 0]) - MIDPOINT)));  
                                    // png_truecolor[j * 3 + 1] = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 1]) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 1]) - MIDPOINT))); 
                                    // png_truecolor[j * 3 + 2] = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 2]) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (png_truecolor[j * 3 + 2]) - MIDPOINT))); 

                                    // R = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (R) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (R) - MIDPOINT))); 
                                    // G = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (G) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (G) - MIDPOINT))); 
                                    // B = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (B) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (B) - MIDPOINT))); 
                                }

                                png.addLine (png_truecolor);
                                Display_Message (i == 0 ? SHOW_PROGRESS : UPDATE_PROGRESS, (100 * i) / IMAGE_HEIGHT);
                            }

                            int iDataSize = png.close();
                            Serial.printf("640 x 480 PNG image compressed to %d bytes\r\n", iDataSize);
                            Serial.printf ("name : %s\r\n", name);
                        }
                    }                    

                    // camera.Photo_Save ();
                    camera.Set_Image_Count (camera.Get_Image_Count () + 1);
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



void Compress_To_Display (uint8_t *buffer) {
    uint16_t x, y;
    uint8_t w, h;
    uint16_t R, G, B;
    
    // Serial.printf ("%f | %f %f | %d %d\r\n", warmth, (float) (255) + ((float) (255) * warmth), (float) (255) - ((float) (255) * warmth), (uint8_t) ((float) (255) + ((float) (255) * warmth)), (uint8_t) ((float) (255) - ((float) (255) * warmth)));

    uint16_t block[320];

    for (int j = 0; j < 172; j++) {
        for (int i = 0; i < 320; i++) {
            // Simple pixel skipping 
            block[i] = ((uint16_t) buffer[(i * 4) + ((j + 34) * 2 * 2 * 640)] << 8) | buffer[(i * 4) + ((j + 34) * 2 * 2 * 640) + 1];
            R = ((block[i] >> 11) & 0x1F);
            G = ((block[i] >> 5) & 0x3F);
            B = (block[i] & 0x1F);

            // R = (uint8_t) ((float) (R) + ((float) (R) * warmth));
            // B = (uint8_t) ((float) (B) + ((float) (B) * warmth));
            R = (uint8_t) ((float) (R) + ((float) (R) * WARMTH_COEF) + ((float) (R) * TINT_COEF));
            B = (uint8_t) ((float) (B) + ((float) (B) * WARMTH_COEF) + ((float) (B) * TINT_COEF));

            R = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (R) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (R) - MIDPOINT))); 
            G = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (G) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (G) - MIDPOINT))); 
            B = (uint8_t) ((MIDPOINT + CONTRAST_FACTOR * ((float) (B) - MIDPOINT)) < 0 ? 0 : (MIDPOINT + CONTRAST_FACTOR * ((float) (B) - MIDPOINT))); 



            // Serial.printf ("%X | %X  %X  %X | %X\r\n", block[i], R, G, B, (R << 11) | ((G << 5) & 0x7FF) | (B & 0x1F));
            
            block[i] = (R << 11) | ((G << 5) & 0x7FF) | (B & 0x1F);
        }

        tft.pushImage (0, j, 320, 1, block);
    }
}

void Display_Message (Message msg, uint16_t img_num) {
    uint8_t char_length = 0;

    // Reset display if message is already displayed
    if (msg_displayed) {
        if (msg == UPDATE_PROGRESS) {
            if (img_num % 10 != 0)      return;
            char_length = 14;
            if (img_num == 100)     char_length++;

            tft.fillSmoothRoundRect (PROG_NUM_X, MSG_Y, ((char_length - 12) * MSG_CHAR_WIDTH), MSG_HEIGHT, MSG_BOX_ROUNDNESS, TFT_DARKGREY, TFT_DARKGREY);
            tft.setCursor (PROG_NUM_X, PROG_NUM_Y);
            tft.printf ("%d%%", img_num);
            return;
        }
        else if (photo_captured)     Compress_To_Display (camera.Get_Fb_Buf ());// TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb_Buf (), camera.Get_Fb_Len ());
        else                    tft.fillScreen (TFT_BLACK);
    }

    // Calculate final length of bounding box
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
        case SHOW_PROGRESS:     char_length = 12; break;
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
        case SHOW_PROGRESS:     {
            tft.print  ("Saving: 0%%");
            break;
        }
    }

    msg_displayed = true;
    msg_display_time = millis ();
}