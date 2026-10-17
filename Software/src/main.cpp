#include "Arduino.h"
#include "Preferences.h"

#include "camera.h"

#include "TFT_eSPI.h"
#include "TJpg_Decoder.h"
#include "JPEGENC.h"

#define CAPTURE_PIN     1
#define SAVE_PIN        2
#define BL_PIN          3
#define SD_CARD_PIN     44
#define DB_DELAY        50 // Max delay for software debounce
#define LP_DELAY        1000 // Max delay for double press

#define DISPLAY_START   0
#define DISPLAY_WIDTH   172
#define DISPLAY_HEIGHT  320
#define IMAGE_WIDTH     720   
#define IMAGE_HEIGHT    1280   

#define MSG_DISPLAY_TIME        2500
#define MSG_X                   0
#define MSG_Y                   15
#define MSG_HEIGHT              18
#define MSG_BOX_ROUNDNESS       5  
#define MSG_HORIZONTAL_PADDING  12   
#define MSG_CHAR_WIDTH          6

#define DISPLAY_JPG_SCALE       4
#define FILE_JPG_SCALE          1

Camera camera;
TFT_eSPI tft = TFT_eSPI();
JPEGENC jpeg;
JPEGENCODE encoder;
static File my_file;

const int i_width = 1280, i_height = 720;
int i, i_mcu_count, rc, i_data_size;
uint8_t uc_mcu[64];

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
enum Compression { X1 = 1, X2 = 2, X4 = 4, X8 = 8 };

bool Callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
bool Callback_Full (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
void Display_Message (Message msg, uint16_t img_num = 0);
void Compress_To_Display (uint8_t *buffer, size_t length, Compression compression);


void * My_Open (const char *filename) {
  my_file = SD.open (filename, FILE_WRITE);
  return (void *) &my_file;
}
void My_Close (JPEGE_FILE *p) {
  File *f = (File *) p->fHandle;
  if (f) f->close ();
}
int32_t My_Read (JPEGE_FILE *p, uint8_t *buffer, int32_t length) {
  File *f = (File *) p->fHandle;
  return f->read (buffer, length);
}

int32_t My_Write (JPEGE_FILE *p, uint8_t *buffer, int32_t length) {
  File *f = (File *) p->fHandle;
  return f->write (buffer, length);
}

int32_t My_Seek (JPEGE_FILE *p, int32_t position) {
  File *f = (File *) p->fHandle;
  return f->seek (position);
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
    
    TJpgDec.setJpgScale (DISPLAY_JPG_SCALE);
    TJpgDec.setCallback (Callback);
    
    preferences.begin ("memory", false);
    camera.Set_Image_Count (preferences.getUInt ("counter", 1));
    
    delay (1000);    
    tft.fillScreen (TFT_BLACK);

    Serial.println ("Begin photo capture");
}

void loop () {
    if (msg_displayed && (millis () - msg_display_time) > MSG_DISPLAY_TIME) {
        if (photo_captured)     TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb_Buf (), camera.Get_Fb_Len ());
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
                // TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb ()->buf, camera.Get_Fb ()->len);
                Serial.printf ("%d\r\n", camera.Get_Fb ()->len);
                camera.Set_Fb_Buf (camera.Get_Fb ()->buf);
                camera.Set_Fb_Len (camera.Get_Fb ()->len);
                Compress_To_Display (camera.Get_Fb_Buf (), camera.Get_Fb_Len (), X4);
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
                    // TJpgDec.setJpgScale (FILE_JPG_SCALE);
                    // TJpgDec.setCallback (Callback_Full);
                    // i = 0, i_data_size = 0;

                    // rc = jpeg.open ("/TEST8.jpg", My_Open, My_Close, My_Read, My_Write, My_Seek);
                    // if (rc == JPEGE_SUCCESS) {
                    //     Serial.println ("JPEG file opened successfully");

                    //     rc = jpeg.encodeBegin (&encoder, i_width, i_height, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_444, JPEGE_Q_HIGH);
                    //     if (rc == JPEGE_SUCCESS) {
                    //         memset (uc_mcu, 0, sizeof (uc_mcu));

                    //         i_mcu_count = ((i_width + encoder.cx - 1) / encoder.cx) * ((i_height + encoder.cy - 1) / encoder.cy);
                    //         TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb_Buf (), camera.Get_Fb_Len ());
                    //         i_data_size = jpeg.close ();
                    //         Serial.print("Output file size = ");
                    //         Serial.println(i_data_size, DEC);
                    //     }
                    // }
                    

                    
                    
                    
                    // TJpgDec.setJpgScale (DISPLAY_JPG_SCALE);
                    // TJpgDec.setCallback (Callback);
                    
                    // camera.Photo_Save ();
                    // preferences.putUInt ("counter", camera.Get_Image_Count ());
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



void Compress_To_Display (uint8_t *buffer, size_t length, Compression compression) {
    uint16_t x, y;
    uint8_t w, h;

    uint16_t block[172];

    // DO NOT PRINT THE FIRST FOUR OR LAST FOUR
    // for (int i = 0; i < 1 /*IMAGE_HEIGHT / compression*/; i++) {
    //     for (int j = 0; j < IMAGE_WIDTH / compression; j++) {
    //         block[j] =  ((uint16_t) buffer[(j * compression) + 1] << 8) | buffer[(j * compression) + 2];
    //         block[j] =  ((uint16_t) buffer[(j * compression) + 1] << 8) | buffer[(j * compression) + 2];
    //         block[j] =  ((uint16_t) buffer[(j * compression) + 1] << 8) | buffer[(j * compression) + 2];
    //         block[j] =  ((uint16_t) buffer[(j * compression) + 1] << 8) | buffer[(j * compression) + 2];
    //     }
    // }

    for (int i = 0; i < 172; i++) {
        block[i] = ((uint16_t) buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    tft.pushImage (0, 0, 172, 1, block);
}

bool Callback (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Serial.printf ("%d  %d | %d  %d | %d\r\n", x, y, w, h, w * h);
    if (y >= tft.height ())     return false;
    for (uint16_t i = 0; i < w * h; i++) {
        // if (bitmap[i] < 0x18C3) Serial.println ("NOOO");
        // bitmap[i] -= 0x18C3; // subtract 6 from r and g, and 3 from b
        // bitmap[i] += 0x0003;
        // if (bitmap[i] & 0x001F >= 0x03)             bitmap[i] -= 0x0003;
        // else        bitmap[i] -= bitmap[i] & 0x001F;
        if ((bitmap[i] & 0x07E0) >> 0x05 >= 0x05)     bitmap[i] -= 0x00A0; //0x00C0;
        // else        bitmap[i] -= (bitmap[i] & 0x07E0) >> 0x05;
        if (bitmap[i] >> 0x0B >= 0x06)              bitmap[i] -= 0x3000;
        // else        bitmap[i] -= bitmap[i] >> 0x0B;
    }
    
    tft.pushImage (x, y, w, h, bitmap);
    return true;
}

bool Callback_Full (int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Serial.printf ("%d  %d | %d  %d | %d\r\n", x, y, w, h, w * h);
    uint16_t left_bitmap[64];
    uint16_t right_bitmap[64];

    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            left_bitmap[i * 8 + j] = bitmap[i * 16 + j];
            right_bitmap[i * 8 + j] = bitmap[i * 16 + (j + 8)];
        }
    }
    jpeg.addMCU (&encoder, (uint8_t *) &left_bitmap, 8);
    jpeg.addMCU (&encoder, (uint8_t *) &right_bitmap, 8);
    
    // jpeg.addMCU (&encoder, (uint8_t *) &bitmap, 16);
    
    return true;
}

void Display_Message (Message msg, uint16_t img_num) {
    // Reset display if message is already displayed
    if (msg_displayed) {
        if (photo_captured)     TJpgDec.drawJpg (DISPLAY_START, DISPLAY_START, camera.Get_Fb_Buf (), camera.Get_Fb_Len ());
        else                    tft.fillScreen (TFT_BLACK);
    }

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
