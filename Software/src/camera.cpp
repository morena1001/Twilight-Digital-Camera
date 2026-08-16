#include "camera.h"

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
    camera.pixel_format     = PIXFORMAT_JPEG;
    camera.frame_size       = FRAMESIZE_UXGA;
    camera.jpeg_quality     = 12;
    camera.fb_count         = 1;
    camera.grab_mode        = CAMERA_GRAB_WHEN_EMPTY;
    camera.fb_location      = CAMERA_FB_IN_PSRAM;

    if (camera.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound ()) {
            camera.jpeg_quality = 10;
            camera.fb_count = 2;
            camera.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            camera.frame_size = FRAMESIZE_SVGA;
            camera.fb_location = CAMERA_FB_IN_DRAM;
        }
    } else {
        camera.frame_size = FRAMESIZE_240X240;
        camera.fb_count = 2;
    }
}

esp_err_t Camera::Init_Camera () {
    return esp_camera_init (&camera);
}

void Camera::Photo_Save () {
    camera_fb_t *fb = esp_camera_fb_get ();
    if (!fb)    return;

    char file_name [32];
    sprintf (file_name, "/image%d.jpg", image_count);

    Write_File (SD, file_name, fb->buf, fb->len);
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

void Camera::Set_Image_Count (uint32_t count) {
    image_count = count;
}

uint32_t Camera::Get_Image_Count () {
    return image_count;
}
