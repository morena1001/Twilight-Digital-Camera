#include "esp_camera.h"
#include "SD.h"
#include "SPI.h"
#include "FS.h"

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

class Camera {
    public: 
        Camera ();
        esp_err_t Init_Camera ();
        void Photo_Save ();
        void Photo_Save (const char * file_name);
        void Write_File (fs::FS & fs, const char * path, uint8_t * data, size_t len);
        void Set_Image_Count (uint32_t count);
        uint32_t Get_Image_Count ();

    private:
        camera_config_t camera;
        uint32_t image_count;
};
