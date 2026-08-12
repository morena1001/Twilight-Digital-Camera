#include "Arduino.h"
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13
#define LED_GPIO_NUM      21

void Photo_Save (const char * file_name);
void Write_File (fs::FS & fs, const char * path, uint8_t * data, size_t len);

camera_config_t camera;
unsigned long last_capture_time = 0;
int image_count = 1;

void setup () {
  Serial.begin (115200);
  while (!Serial);

  camera.pin_pwdn       = PWDN_GPIO_NUM;
  camera.pin_reset      = RESET_GPIO_NUM;
  camera.pin_xclk       = XCLK_GPIO_NUM;
  camera.pin_sccb_sda   = SIOD_GPIO_NUM;
  camera.pin_sccb_scl   = SIOC_GPIO_NUM;
  camera.pin_d7         = Y9_GPIO_NUM;
  camera.pin_d6         = Y8_GPIO_NUM;
  camera.pin_d5         = Y7_GPIO_NUM;
  camera.pin_d4         = Y6_GPIO_NUM;
  camera.pin_d3         = Y5_GPIO_NUM;
  camera.pin_d2         = Y4_GPIO_NUM;
  camera.pin_d1         = Y3_GPIO_NUM;
  camera.pin_d0         = Y2_GPIO_NUM;
  camera.pin_vsync      = VSYNC_GPIO_NUM;
  camera.pin_href       = HREF_GPIO_NUM;
  camera.pin_pclk       = PCLK_GPIO_NUM;

  camera.xclk_freq_hz   = 20000000;
  camera.ledc_timer     = LEDC_TIMER_0;
  camera.ledc_channel   = LEDC_CHANNEL_0;
  camera.pixel_format   = PIXFORMAT_JPEG;
  camera.frame_size     = FRAMESIZE_UXGA;
  camera.jpeg_quality   = 12;
  camera.fb_count       = 1;
  camera.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;
  camera.fb_location = CAMERA_FB_IN_PSRAM;

  if (camera.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound ()) {
      camera.jpeg_quality = 10;
      camera.fb_count = 2;
      camera.grab_mode = CAMERA_GRAB_LATEST;
      Serial.println ("psram found");
    } else {
      camera.frame_size = FRAMESIZE_SVGA;
      camera.fb_location = CAMERA_FB_IN_DRAM;
      Serial.println ("psram not found");
    }
  } else {
    camera.frame_size = FRAMESIZE_240X240;
    camera.fb_count = 2;
  }

  // Init camera
  esp_err_t err = esp_camera_init (&camera);
  if (err != ESP_OK) {
    Serial.printf ("Camera init failed with code 0x%x", err);
    return;
  }

  // Init SD card
  if (!SD.begin (21)) {
    Serial.println ("Card mount failed");
    return;
  }
  uint8_t card_type = SD.cardType ();

  if (card_type == CARD_NONE) {
    Serial.println ("No SD card attached");
    return;
  }

  Serial.print ("SD card type: ");
  switch (card_type) {
    case CARD_MMC:    Serial.println ("MMC"); break;
    case CARD_SD:     Serial.println ("SDSC"); break;
    case CARD_SDHC:   Serial.println ("SDHC"); break;
    default:          Serial.println ("Unkown"); break;
  }

  Serial.println ("20 seconds until photo capture");
}

void loop () {
  unsigned long now = millis ();

  if ((now - last_capture_time) >= 20000) {
    char file_name [32];
    sprintf (file_name, "/image%d.jpg", image_count);
    Photo_Save (file_name);
    Serial.printf ("Saved picture: %s\n", file_name);
    Serial.println ("20 seconds until photo capture");
    image_count++;
    last_capture_time = now;
  }
  // Serial.println ("1");
  // delay (1000);
  // Serial.println ("2");
  // delay (1000);
}



void Photo_Save (const char * file_name) {
  camera_fb_t *fb = esp_camera_fb_get ();
  if (!fb) {
    Serial.println ("Failed to get camera frame buffer");
    return;
  }

  Write_File (SD, file_name, fb->buf, fb->len);

  esp_camera_fb_return (fb);

  Serial.println ("Photo saved to file");
}

void Write_File (fs::FS & fs, const char * path, uint8_t * data, size_t len) {
  Serial.printf ("Writing file: %s\n", path);

  File file = fs.open (path, FILE_WRITE);
  if (!file) {
    Serial.println ("Failed to open file for writing");
    return;
  }

  if (file.write (data, len) == len)  Serial.println ("Write succeeded");
  else                                Serial.println ("Write failed");

  file.close ();
}
