#include "Arduino.h"
#include "ST7789V3.h"

ST7789V3::ST7789V3 (uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin) {
    cs_pin_ = cs_pin;
    dc_pin_ = dc_pin;
    rst_pin_ = rst_pin;

    // CS pin is low active (p.29)
    pinMode (ST7789V3_CS_PIN, OUTPUT);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
    
    // DC pin: low for command, high for data (p.29)
    pinMode (ST7789V3_DC_PIN, OUTPUT); 
    digitalWrite (ST7789V3_DC_PIN, LOW);
}

void ST7789V3::Init_ST7789V3 (bool is_spi_init) {
    if (!is_spi_init) {
        SPI.begin ();
    }

    // RST pin is low active (p.29)
    pinMode (ST7789V3_RST_PIN, OUTPUT);
    digitalWrite (ST7789V3_RST_PIN, LOW);
    digitalWrite (ST7789V3_RST_PIN, HIGH);

    // 5 ms needed after hardware reset to send commands (p.41)
    delay (SHORT_DELAY);

    // Software reset
    Transmit_Cmd (ST7789V3_CMD_SWRESET);

    // 5 ms needed after software reset to send commands (p.122)
    delay (SHORT_DELAY);
    
    // Sleep out
    Transmit_Cmd (ST7789V3_CMD_SLPOUT);

    // Write display, half brightness
    Transmit_Cmd_S_Data (ST7789V3_CMD_WRDISBV, HALF_BRIGHTNESS);

    // Set color mode (262k colo RGB interface, 18 bits / pixel)
    Transmit_Cmd_S_Data (ST7789V3_CMD_COLMOD, COLMOD_DEFAULT);

    // Memory data access control
    Transmit_Cmd_S_Data (ST7789V3_CMD_MADCTL, MADCTL_DEFAULT);

    // Write CTRL display
    Transmit_Cmd_S_Data (ST7789V3_CMD_WRCTRLD, WRCTRLD_DEFAULT);

    // Display inversion on
    Transmit_Cmd (ST7789V3_CMD_INVON);
    
    // Display on
    Transmit_Cmd (ST7789V3_CMD_DISPON);
}

void ST7789V3::Set_Window_Location (uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    // Set column window location
    byte col_win[WINDOW_SET_DATA_LENGTH] = { (byte) (x_start >> 0x08), (byte) (x_start & 0xFF), (byte) (x_end >> 0x08), (byte) (x_end & 0xFF) }; 
    Transmit_Cmd_M_Data_Array (ST7789V3_CMD_CASET, col_win, WINDOW_SET_DATA_LENGTH);
    
    // Set row window location
    byte row_win[WINDOW_SET_DATA_LENGTH] = { (byte) (y_start >> 0x08), (byte) (y_start & 0xFF), (byte) (y_end >> 0x08), (byte) (y_end & 0xFF) }; 
    Transmit_Cmd_M_Data_Array (ST7789V3_CMD_RASET, row_win, WINDOW_SET_DATA_LENGTH);
}

void ST7789V3::Set_Window_Location_Size (uint16_t x_start, uint16_t x_length, uint16_t y_start, uint16_t y_length) {
    // Set column window location
    byte col_win[WINDOW_SET_DATA_LENGTH] = { (byte) (x_start >> 0x08), (byte) (x_start & 0xFF), (byte) ((x_start + x_length) >> 0x08), (byte) ((x_start + x_length) & 0xFF) }; 
    Transmit_Cmd_M_Data_Array (ST7789V3_CMD_CASET, col_win, WINDOW_SET_DATA_LENGTH);
    
    // Set row window location
    byte row_win[WINDOW_SET_DATA_LENGTH] = { (byte) (y_start >> 0x08), (byte) (y_start & 0xFF), (byte) ((y_start +y_length) >> 0x08), (byte) ((y_start +y_length) & 0xFF) }; 
    Transmit_Cmd_M_Data_Array (ST7789V3_CMD_RASET, row_win, WINDOW_SET_DATA_LENGTH);
}

void ST7789V3::Draw_Pixel (uint32_t color) {
    uint8_t rgb[3] = { (uint8_t) (color >> 0x10), (uint8_t) ((color >> 0x08) & 0xFF), (uint8_t) (color & 0xFF) };
    Transmit_Cmd_M_Data (ST7789V3_CMD_RAMWR, rgb, 3);
}

void ST7789V3::Draw_Pixel (uint8_t *color) {
    Transmit_Cmd_M_Data (ST7789V3_CMD_RAMWR, color, 3);
}

void ST7789V3::Draw_Pixels (uint32_t *color, uint16_t length) {
    uint8_t rgb[3];

    Transmit_Cmd (ST7789V3_CMD_RAMWR);
    for (uint16_t i = 0; i < length; i++) {
        rgb[0] = (uint8_t) (color[i] >> 0x10);
        rgb[1] = (uint8_t) ((color[i] >> 0x08) & 0xFF);
        rgb[2] = (uint8_t) (color[i] & 0xFF);
        Transmit_Multiple_Data (rgb, 3);
    }
}

void ST7789V3::Draw_Pixels (uint8_t **color, uint16_t length) {
    Transmit_Cmd (ST7789V3_CMD_RAMWR);
    for (uint16_t i = 0; i < length; i++)
        Transmit_Multiple_Data (color[i], 3);
}

void ST7789V3::Draw_Block (uint32_t color, uint16_t length, uint16_t width) {
    uint8_t rgb[3] = { (uint8_t) (color >> 0x10), (uint8_t) ((color >> 0x08) & 0xFF), (uint8_t) (color & 0xFF) };

    Transmit_Cmd (ST7789V3_CMD_RAMWR);
    for (uint16_t i = 0; i < length + 1; i++)
        for (uint16_t j = 0; j < width + 1; j++)
            Transmit_Multiple_Data (rgb, 3);
}

void ST7789V3::Draw_Block (uint8_t *color, uint16_t length, uint16_t width) {
    Transmit_Cmd (ST7789V3_CMD_RAMWR);
    for (uint16_t i = 0; i < length + 1; i++)
        for (uint16_t j = 0; j < width + 1; j++)
            Transmit_Multiple_Data (color, 3);
}

void ST7789V3::Fill_Screen (uint32_t color) {
    Set_Window_Location_Size (0x0022, SCREEN_WIDTH, 0x0000, SCREEN_LENGTH);
    Draw_Block (color, SCREEN_LENGTH, SCREEN_WIDTH);
}

void ST7789V3::Fill_Screen (uint8_t *color) {
    Set_Window_Location_Size (0x0022, SCREEN_WIDTH, 0x0000, SCREEN_LENGTH);
    Draw_Block (color, SCREEN_LENGTH, SCREEN_WIDTH);
}

void ST7789V3::Clear_Screen () {
    Fill_Screen (COLOR_WHITE);
}






void ST7789V3::Transmit_Cmd (uint8_t cmd) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (cmd);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Single_Data (uint8_t data) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (data);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Multiple_Data (uint8_t *data, uint8_t length) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    for (uint16_t i = 0; i < length; i++)   SPI.transfer (data[i]);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Multiple_Data_Array (uint8_t *data, uint8_t length) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (data, length);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Cmd_S_Data (uint8_t cmd, uint8_t data) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (cmd);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (data);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Cmd_M_Data (uint8_t cmd, uint8_t *data, uint8_t length) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (cmd);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    for (uint16_t i = 0; i < length; i++)   SPI.transfer (data[i]);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}

void ST7789V3::Transmit_Cmd_M_Data_Array (uint8_t cmd, uint8_t *data, uint8_t length) {
    digitalWrite (ST7789V3_CS_PIN, LOW);
    digitalWrite (ST7789V3_DC_PIN, LOW);
    SPI.transfer (cmd);
    digitalWrite (ST7789V3_DC_PIN, HIGH);
    SPI.transfer (data, length);
    digitalWrite (ST7789V3_CS_PIN, HIGH);
}
