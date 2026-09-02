#include "camera.h"
#include "img_converters.h"

Camera::Camera () {
    camera.pin_pwdn         = PWDN_GPIO_NUM;
    camera.pin_reset        = RESET_GPIO_NUM;
    camera.pin_xclk         = XCLK_GPIO_NUM;
    camera.pin_sccb_sda     = SIOD_GPIO_NUM;
    camera.pin_sccb_scl     = SIOC_GPIO_NUM;
    camera.pin_d7           = Y9_GPIO_NUM;
    camera.pin_d6           = Y8_GPIO_NUM;
    camera.pin_d5           = Y7_GPIO_NUM;
    camera.pin_d4           = Y6_GPIO_NUM;
    camera.pin_d3           = Y5_GPIO_NUM;
    camera.pin_d2           = Y4_GPIO_NUM;
    camera.pin_d1           = Y3_GPIO_NUM;
    camera.pin_d0           = Y2_GPIO_NUM;
    camera.pin_vsync        = VSYNC_GPIO_NUM;
    camera.pin_href         = HREF_GPIO_NUM;
    camera.pin_pclk         = PCLK_GPIO_NUM;

    camera.xclk_freq_hz     = 20000000;
    camera.ledc_timer       = LEDC_TIMER_0;
    camera.ledc_channel     = LEDC_CHANNEL_0;
    camera.pixel_format     = PIXFORMAT_JPEG; // PIXFORMAT_RGB565; // PIXFORMAT_RGB888;
    camera.frame_size       = FRAMESIZE_P_HD; // FRAMESIZE_FHD; // FRAMESIZE_QVGA; // FRAMESIZE_UXGA;
    camera.jpeg_quality     = 12;
    camera.fb_count         = 1;
    camera.grab_mode        = CAMERA_GRAB_LATEST; //CAMERA_GRAB_WHEN_EMPTY;
    camera.fb_location      = CAMERA_FB_IN_PSRAM;
}

esp_err_t Camera::Init_Camera () {
    esp_err_t init = esp_camera_init (&camera);
    if (init != ESP_OK)     return init;

    sensor_t *s = esp_camera_sensor_get ();
    if (s == NULL) return ESP_FAIL;

    s->set_contrast (s, -1);

    return ESP_OK;
}

void Camera::Photo_Save () {
    camera_fb_t *fb = esp_camera_fb_get ();
    if (!fb)    return;

    char file_name [32];
    sprintf (file_name, "/image%d.jpg", image_count);
    
    Write_File (SD, file_name, fb->buf, fb->len);
    Serial.printf ("Saved picture: %s\n", file_name);
    image_count++;
    esp_camera_fb_return (fb);
}

void Camera::Photo_Save (const char * file_name) {
    camera_fb_t *fb = esp_camera_fb_get ();
    if (!fb)    return;

    Write_File (SD, file_name, fb->buf, fb->len);
    esp_camera_fb_return (fb);
}

void Camera::Write_File (fs::FS & fs, const char * path, uint8_t * data, size_t len) {
    File file = fs.open (path, FILE_WRITE);
    if (!file)  return;
    file.write (data, len);
    file.close ();
}

void Camera::Set_Image_Count (uint32_t count)   { image_count = count; }
uint32_t Camera::Get_Image_Count ()     { return image_count; }
