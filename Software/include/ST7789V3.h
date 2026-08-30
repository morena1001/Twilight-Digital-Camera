#include "SPI.h"

/*
    DEFAULT GPIO PINS
*/

#define ST7789V3_CS_PIN    43 
#define ST7789V3_DC_PIN    6 
#define ST7789V3_RST_PIN   5 

/*
    COMMANDS
*/

#define ST7789V3_CMD_SWRESET    0x01
#define ST7789V3_CMD_SLPIN      0x10
#define ST7789V3_CMD_SLPOUT     0x11
#define ST7789V3_CMD_INVOFF     0x20
#define ST7789V3_CMD_INVON      0x21
#define ST7789V3_CMD_DISPOFF    0x28
#define ST7789V3_CMD_DISPON     0x29
#define ST7789V3_CMD_CASET      0x2A
#define ST7789V3_CMD_RASET      0x2B
#define ST7789V3_CMD_RAMWR      0x2C
#define ST7789V3_CMD_MADCTL     0x36
#define ST7789V3_CMD_IDMOFF     0x38
#define ST7789V3_CMD_COLMOD     0x3A
#define ST7789V3_CMD_WRMEMC     0x3C
#define ST7789V3_CMD_WRDISBV    0x51
#define ST7789V3_CMD_WRCTRLD    0x53

/*
    MISC
*/

#define LONG_DELAY              120
#define SHORT_DELAY             5
#define FULL_BRIGHTNESS         0xFF
#define HALF_BRIGHTNESS         0x7F
#define NO_BRIGHTNESS           0x7F
#define WINDOW_SET_DATA_LENGTH  0x04

#define COLMOD_DEFAULT          0x06    // p.183, 18 bits per pixel color format 
#define MADCTL_DEFAULT          0x00    // p.174
#define MADCTL_DEFAULT          0x00    // p.174
#define WRCTRLD_DEFAULT         0x00    // p.196

#define SCREEN_WIDTH            172
#define SCREEN_LENGTH           320
#define RAM_WIDTH_START         0x0022 // display is smaller than display RAM and sits at the midpoint of the width of the 2D RAM
#define RAM_WIDTH_END           0x00CE // display is smaller than display RAM and sits at the midpoint of the width of the 2D RAM
#define RAM_LENGTH_START        0x0000
#define RAM_LENGTH_END          0x0140

#define COLOR_WHITE     0xFCFCFC
#define COLOR_BLACK     0x000000
#define COLOR_RED       0xFC0000
#define COLOR_BLUE      0x00FC00
#define COLOR_GREEN     0x0000FC

class ST7789V3 {
    public: 
        ST7789V3 (uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin);
        void Init_ST7789V3 (bool is_spi_init);
        esp_err_t Reset ();

        void Set_Window_Location (uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end); // Set drawing window given corners of window box
        void Set_Window_Location_Size (uint16_t x_start, uint16_t x_length, uint16_t y_start, uint16_t y_length); // Set drawing window given top left corner of window box
        
        void Draw_Pixel (uint32_t color); // in RGB Format (Max value for each channel is 0xFC)
        void Draw_Pixel (uint8_t *color); // in RGB Format (Max value for each channel is 0xFC)

        void Draw_Pixels (uint32_t *color, uint16_t length); // Draw pixels given the colors for each pixel
        void Draw_Pixels (uint16_t *color, uint16_t length); // Draw pixels given the colors for each pixel
        void Draw_Pixels (uint8_t **color, uint16_t length); // Draw pixels given the colors for each pixel

        void Draw_Block (uint32_t color, uint16_t length, uint16_t width); // Draw a block of pixels given a single color and the dimensions of the block
        void Draw_Block (uint8_t *color, uint16_t length, uint16_t width); // Draw a block of pixels given a single color and the dimensions of the block

        void Fill_Screen (uint32_t color); // Fill the screen given a single color
        void Fill_Screen (uint8_t *color); // Fill the screen given a single color

        void Clear_Screen (); // Fill the screen with white

        uint8_t Get_cs_Pin ();
        void Set_cs_Pin (uint8_t pin);

        uint8_t Get_dc_Pin ();
        void Set_dc_Pin (uint8_t pin);
        uint8_t Get_rst_Pin ();
        void Set_rst_Pin (uint8_t pin);

    private:
        void Transmit_Cmd (uint8_t cmd); // Transmit one command byte
        void Transmit_Single_Data (uint8_t data); // Transmit one byte of data
        void Transmit_Multiple_Data (uint8_t *data, uint8_t length); // Transmit multiple bytes of data, one byte at a time
        void Transmit_Multiple_Data_Array (uint8_t *data, uint8_t length); // Transmit multiple bytes of data at the same time
        void Transmit_Cmd_S_Data (uint8_t cmd, uint8_t data); // Transmit a command a single data byte
        void Transmit_Cmd_M_Data (uint8_t cmd, uint8_t *data, uint8_t length); // Transmit a command byte and multiple data bytes, one byte at a time
        void Transmit_Cmd_M_Data_Array (uint8_t cmd, uint8_t *data, uint8_t length); // Transmit a command byte and multiple data bytes at the same time
        
        uint8_t cs_pin_, dc_pin_, rst_pin_;
};
