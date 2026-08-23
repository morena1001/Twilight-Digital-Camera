#include "Arduino.h"
#include "SPI.h"

#define ST7789V3_CS_PIN    43 
#define ST7789V3_DC_PIN    6 
#define ST7789V3_RST_PIN   5 

#define ST7789V3_CMD_SWRESET    0x01
#define ST7789V3_CMD_SLPIN      0x10
#define ST7789V3_CMD_SLPOUT     0x11
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

void setup () {
    Serial.begin (115200);
    while (!Serial);

    // CS pin is low active
    pinMode (ST7789V3_CS_PIN, OUTPUT);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
    
    // DC pin: low for command, high for data
    pinMode (ST7789V3_DC_PIN, OUTPUT);
    digitalWrite (ST7789V3_DC_PIN, LOW);

    // RST pin is low active
    pinMode (ST7789V3_RST_PIN, OUTPUT);
    digitalWrite (ST7789V3_RST_PIN, LOW);
    digitalWrite (ST7789V3_RST_PIN, HIGH);
    
    // 5 ms needed after hardware reset to send commands (p.41)
    delay (200); 

    SPI.begin ();

    // Software reset
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_SWRESET);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // 5 ms needed after software reset to send commands (p.122)
    delay (200); 

    // Sleep out
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_SLPOUT);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // 120 ms needed after hardware reset to send commands (p.143)
    delay (200); 

    // Write display brightness (0x7F is half brightness)
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_WRDISBV);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (0x7F);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // Set color mode (262k colo RGB interface, 18 bits / pixel)
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_COLMOD);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (0x06);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // Memory data access control
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_MADCTL);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (0x00);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // Write CTRL display
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_WRCTRLD);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (0x00);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // // Idle mode off
    // digitalWrite (ST7789V3_CS_PIN, LOW);
    // digitalWrite (ST7789V3_DC_PIN, LOW);
    // SPI.transfer (ST7789V3_CMD_IDMOFF);
    // digitalWrite (ST7789V3_CS_PIN, HIGH);
    

    // Display on
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (ST7789V3_CMD_DISPON);
    digitalWrite (ST7789V3_CS_PIN, HIGH);

    // Set entire screen to white

        // Set column window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_CASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        byte col_win[4] = { 0x00, 0x00, 0x00, 0xEE };
        SPI.transfer (col_win, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Set row window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        byte row_win[4] = { 0x00, 0x00, 0x01, 0x3E };
        SPI.transfer (row_win, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Write pixel data
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RAMWR);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        byte pixel_data[3] = { 0x00, 0x00, 0x00 };
        for (int i = 0; i < 239; i++) {
            for (int j = 0; j < 319; j++) {
                // SPI.transfer (pixel_data, 3);
                SPI.transfer (pixel_data[0]);
                SPI.transfer (pixel_data[1]);
                SPI.transfer (pixel_data[2]);
            }
        }
        digitalWrite (ST7789V3_CS_PIN, HIGH);


    // Write a box to the screen

        // Set column window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_CASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        byte col_win_1[4] = { 0x00, 0x62, 0x00, 0x7C };
        // byte col_win[4] = { 0x00, 0x84, 0x00, 0xAE };
        SPI.transfer (col_win_1, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Set row window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        byte row_win_1[4] = { 0x00, 0x85, 0x00, 0xA9 };
        // byte row_win[4] = { 0x00, 0xC7, 0x00, 0xDB };
        SPI.transfer (row_win_1, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Write pixel data
        byte pixel_data_1[3] = { 0xFC, 0xFC, 0x7E };
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RAMWR);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        for (int i = 0; i < 26; i++) {
            for (int j = 0; j < 36; j++) {
                // SPI.transfer (pixel_data, 3);
                SPI.transfer (pixel_data_1[0]);
                SPI.transfer (pixel_data_1[1]);
                SPI.transfer (pixel_data_1[2]);
            }
        }
        digitalWrite (ST7789V3_CS_PIN, HIGH);


    // Write a box to the screen

        // Set column window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_CASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        // byte col_win[4] = { 0x00, 0x62, 0x00, 0x7C };
        byte col_win_2[4] = { 0x00, 0x84, 0x00, 0xAE };
        SPI.transfer (col_win_2, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Set row window
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RASET);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        // byte row_win[4] = { 0x00, 0x85, 0x00, 0xA9 };
        byte row_win_2[4] = { 0x00, 0xC7, 0x00, 0xDB };
        SPI.transfer (row_win_2, 4);
        digitalWrite (ST7789V3_CS_PIN, HIGH);

        // Write pixel data
        byte pixel_data_2[3] = { 0xFC, 0xFC, 0x00 };
        digitalWrite (ST7789V3_CS_PIN, LOW);
        digitalWrite (ST7789V3_DC_PIN, LOW);
        SPI.transfer (ST7789V3_CMD_RAMWR);
        digitalWrite (ST7789V3_DC_PIN, HIGH);
        for (int i = 0; i < 42; i++) {
            for (int j = 0; j < 20; j++) {
                // SPI.transfer (pixel_data, 3);
                SPI.transfer (pixel_data_2[0]);
                SPI.transfer (pixel_data_2[1]);
                SPI.transfer (pixel_data_2[2]);
            }
        }
        digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void loop () {

}
