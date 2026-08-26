#include "Preferences.h"

#include "ST7789V3.h"

#define CAPTURE_PIN     1
#define SD_CARD_PIN     44
#define DB_DELAY        50

ST7789V3 st7789v3 (ST7789V3_CS_PIN, ST7789V3_DC_PIN, ST7789V3_RST_PIN);

void setup () {
    Serial.begin (115200);
    while (!Serial);

    st7789v3.Init_ST7789V3 (false);

    // Set entire screen to white
    byte pixel_data[3] = { 0x7E, 0x00, 0x00 };
    st7789v3.Fill_Screen (pixel_data);

    // Write a box to the screen
    st7789v3.Set_Window_Location (0x0062, 0x007C, 0x0085, 0x00A9);
    byte pixel_data_1[3] = { 0x00, 0x00, 0x7E };
    st7789v3.Draw_Block (pixel_data_1, 0x001A, 0x24);

    // Write a box to the screen   
    st7789v3.Set_Window_Location (0x0084, 0x00AE, 0x00C7, 0x00DB);
    byte pixel_data_2[3] = { 0x00, 0x00, 0xFC };
    st7789v3.Draw_Block (pixel_data_2, 0x002A, 0x14);
}

void loop () {

}
