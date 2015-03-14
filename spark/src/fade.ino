// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

// https://github.com/technobly/SparkCore-NeoPixel
// This #include statement was automatically added by the Spark IDE.
#include "neopixel/neopixel.h"
#include <cmath>

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 24
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

const float VALUE_DEFAULT = 0.1f;
const float HUE_INCR = 0.01f;
const int ITERATION_DELAY_MS = 50; // milliseconds

enum mode_e {OFF, ROTATE_HUE};
enum mode_e mode_ = OFF;

float hue_ = 0.0;               // Starting pixel hue (0.0 .. 1.0)
float saturation_ = 1.0;        // Pixel saturation (0.0 .. 1.0)
float value_ = VALUE_DEFAULT;   // Pixel value / brightness (0.0 .. 1.0)


void setup() 
{
    Spark.function("mode", setMode);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void loop() 
{
    switch (mode_) {
        case ROTATE_HUE:
            rotate();
            break;
        default:
            for (int i = 0; i < PIXEL_COUNT; i++) {
                strip.setPixelColor(i, 0);
            }
            break;
    }
    strip.show();
    value_ = VALUE_DEFAULT;
    delay(ITERATION_DELAY_MS);
}


uint32_t hsv2rgb(float h, float s, float v) {
    // h float: hue from 0.0 to 1.0
    // s float: saturation from 0.0 to 1.0
    // v float: value (intensity) from 0.0 to 1.0
    // return: list of RGB color integers: 0 to 255
    // http://www.easyrgb.com/index.php?X=MATH&H=21#text21
    if (s == 0.0f) {
        int a = int(floor(255 * v + 0.5f));
        return strip.Color(a, a, a);
    }
    h = h - floor(h);
    h = h * 6.0f;
    int i = int(floor(h));
    float f = h - i; // fractional part of h
    float xf = v * (1.0f - s);
    float yf = v * (1.0f - s * f);
    float zf = v * (1.0f - s * (1.0f - f));
    uint8_t x = uint8_t(floor(255 * xf + 0.5f));
    uint8_t y = uint8_t(floor(255 * yf + 0.5f));
    uint8_t z = uint8_t(floor(255 * zf + 0.5f));
    uint8_t w = uint8_t(floor(255 * v + 0.5f));
    switch (i) {
        case 0:  return strip.Color(w, z, x);
        case 1:  return strip.Color(y, w, x);
        case 2:  return strip.Color(x, w, z);
        case 3:  return strip.Color(x, y, w);
        case 4:  return strip.Color(z, x, w);
        case 5:  return strip.Color(w, x, y);
        default: return strip.Color(0, 0, 0);
    }
}


void rotate() {
    float h = hue_;
    for (int i = 0; i < PIXEL_COUNT; i++) {
        h = h - floor(h);
        uint32_t color = hsv2rgb(h, saturation_, value_);
        strip.setPixelColor(i, color);
        h += 1.0f / PIXEL_COUNT;
    }
    hue_ += HUE_INCR;
    hue_ = hue_ - floor(hue_);
}


int setMode(String command) {
    if (command == "ROTATE_HUE") {
        mode_ = ROTATE_HUE;
        value_ = 1.0f;
    } else {
        mode_ = OFF;
    }
    return 0;
}
