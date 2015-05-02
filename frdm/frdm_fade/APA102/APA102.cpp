// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

#include "APA102.h"

APA102::APA102(int pixels, SPI * spi) 
        : spi_(spi)
        , data_(NULL)
        , pixels_(0)
{
    if (pixels > 0) {
        data_ = new uint8_t[pixels * 4];
        if (data_) {
            pixels_ = pixels;
            clear();
        }
    }
}
    
APA102::~APA102() {
    if (data_) {
        delete [] data_;
    }
}
    
bool APA102::setRGB(int pixel, int r, int g, int b) {
    if ((pixel < 0) || (pixel > pixels_)) {
        return false;
    }
    int offset = pixel * 4;
    data_[offset + 1] = b & 0xff;
    data_[offset + 2] = g & 0xff;
    data_[offset + 3] = r & 0xff;
    return true;
}

bool APA102::setHSV(int pixel, float h, float s, float v) {
    if (s == 0.0f) {
        int a = int(floor(255 * v + 0.5f));
        return setRGB(pixel, a, a, a);
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
        case 0:  return setRGB(pixel, w, z, x);
        case 1:  return setRGB(pixel, y, w, x);
        case 2:  return setRGB(pixel, x, w, z);
        case 3:  return setRGB(pixel, x, y, w);
        case 4:  return setRGB(pixel, z, x, w);
        case 5:  return setRGB(pixel, w, x, y);
        default: return setRGB(pixel, 0, 0, 0);
    }
}

void APA102::clear() {
    for (int i = 0; i < pixels_; ++i) {
        int offset = i * 4;
        data_[offset] = 0xFF; // global brightness
        data_[offset + 1] = 0;
        data_[offset + 2] = 0;
        data_[offset + 3] = 0;
    }
}

void APA102::refresh() {
    // start of frame
    spi_->write(0x00);
    spi_->write(0x00);
    spi_->write(0x00);
    spi_->write(0x00);
    
    for (int i = 0; i < pixels_ * 4; ++i) {
        spi_->write(data_[i]);
    }

    // end of frame
    int eof_bytes = pixels_ / 2 / 8;
    if (eof_bytes < 4) {
        eof_bytes = 4;
    }
    for (int i = 0; i < eof_bytes; ++i) {
        spi_->write(0xFF);
    }
}

