// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

#include "mbed.h"

/** Controller for APA102 LEDs.
 * Used for controlling one or more APA102 RGB LEDs over SPI.
 */
class APA102 {
public:
    /** Construct a new instance.
     *
     * @param pixels The total number of pixels in the array.
     * @param spi The SPI interface controlling the pixels.  This interface
     *      must not be connected to any other devices.
     */
    APA102(int pixels, SPI * spi);
    
    /** Destructor */
    ~APA102();
    
    /** Set a pixel using red, green, blue.
     *
     * @param pixel The pixel number.
     * @param r The red value from 0 to 255.
     * @param g The green value from 0 to 255.
     * @param b The blue value from 0 to 255.
     * @return True on success or false on failure.
     */ 
    bool setRGB(int pixel, int r, int g, int b);
    
    /** Set a pixel using hue, saturation, value.
     *
     * @param pixel The pixel number.
     * @param h The hue (float) from 0.0 to 1.0
     * @param s The saturation (float) from 0.0 to 1.0.
     * @param v The value/intensity (float) from 0.0 to 1.0
     * @return True on success or false on failure.
     *
     * @see http://www.easyrgb.com/index.php?X=MATH&H=21#text21
     */
    bool setHSV(int pixel, float h, float s, float v);
    
    /** Clear the array and set all LEDs to black. */
    void clear();
    
    /** Refresh the array with the buffered values. */
    void refresh();

private:
    SPI * spi_;
    uint8_t * data_;
    int pixels_;
};
