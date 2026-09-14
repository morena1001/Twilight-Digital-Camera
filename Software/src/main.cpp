#include "Arduino.h"
#include "I2S.h"

#include "sounds.h"

void setup () {
    Serial.begin (115200);
    while (!Serial);

    Serial.println ("Attempting to begin I2S");
    I2S.begin (I2S_PHILIPS_MODE, 8000, 16);

    Serial.println ("Attempting to send data");
    I2S.write (hello, 8730);
}

void loop () {
    
}
