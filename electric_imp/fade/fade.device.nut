// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

class NeoPixels {
    // Copyright (c) 2015 Electric Imp
    // This file is licensed under the MIT License
    // http://opensource.org/licenses/MIT
    // https://github.com/electricimp/NeoPixels
    
    // This class uses SPI to emulate the newpixels' one-wire protocol.
    // This requires one byte per bit to send data at 7.5 MHz via SPI.
    // These consts define the "waveform" to represent a zero or one

    static ZERO            = 0xC0;
    static ONE             = 0xF8;
    static BYTESPERPIXEL   = 24;

    // when instantiated, the neopixel class will fill this array with blobs to
    // represent the waveforms to send the numbers 0 to 255. This allows the blobs to be
    // copied in directly, instead of being built for each pixel - which makes the class faster.

    bits            = null;

    // Like bits, this blob holds the waveform to send the color [0,0,0], to clear pixels faster

    clearblob       = blob(12);

    // private variables passed into the constructor

    spi             = null; // imp SPI interface (pre-configured)
    frameSize       = null; // number of pixels per frame
    frame           = null; // a blob to hold the current frame

    // _spi - A configured spi (MSB_FIRST, 7.5MHz)
    // _frameSize - Number of Pixels per frame

    constructor(_spi, _frameSize) {
        this.spi = _spi;
        this.frameSize = _frameSize;
        this.frame = blob(frameSize*BYTESPERPIXEL + 1);
        this.frame[frameSize*BYTESPERPIXEL] = 0;

        // prepare the bits array and the clearblob blob

        initialize();

        clearFrame();
        writeFrame();
    }

    // fill the array of representative 1-wire waveforms.
    // done by the constructor at instantiation.

    function initialize() {
        // fill the bits array first

        bits = array(256);
        for (local i = 0; i < 256; i++) {
            local valblob = blob(BYTESPERPIXEL / 3);
            valblob.writen((i & 0x80) ? ONE:ZERO,'b');
            valblob.writen((i & 0x40) ? ONE:ZERO,'b');
            valblob.writen((i & 0x20) ? ONE:ZERO,'b');
            valblob.writen((i & 0x10) ? ONE:ZERO,'b');
            valblob.writen((i & 0x08) ? ONE:ZERO,'b');
            valblob.writen((i & 0x04) ? ONE:ZERO,'b');
            valblob.writen((i & 0x02) ? ONE:ZERO,'b');
            valblob.writen((i & 0x01) ? ONE:ZERO,'b');
            bits[i] = valblob;
        }

        // now fill the clearblob
        for(local j = 0; j < BYTESPERPIXEL; j++) {
            clearblob.writen(ZERO, 'b');
        }
    }

    // sets a pixel in the frame buffer
    // but does not write it to the pixel strip
    // color is an array of the form [r, g, b]

    function writePixel(p, color) {
        frame.seek(p*BYTESPERPIXEL);

        // red and green are swapped for some reason, so swizzle them back

        frame.writeblob(bits[color[1]]);
        frame.writeblob(bits[color[0]]);
        frame.writeblob(bits[color[2]]);
    }

    // Clears the frame buffer
    // but does not write it to the pixel strip

    function clearFrame() {
        frame.seek(0);
        for (local p = 0; p < frameSize; p++) frame.writeblob(clearblob);
    }

    // writes the frame buffer to the pixel strip
    // ie - this function changes the pixel strip

    function writeFrame() {
        spi.write(frame);
    }
}


// SPI Configuration for NeoPixels
PIXELS <- 24
spi <- hardware.spi257
spi.configure(MSB_FIRST, 7000)
pixels <- NeoPixels(hardware.spi257, PIXELS)


function hsv2rgb(h, s, v) {
    // h float: hue from 0.0 to 1.0
    // s float: saturation from 0.0 to 1.0
    // v float: value (intensity) from 0.0 to 1.0
    // return: list of RGB color integers: 0 to 255
    // http://www.easyrgb.com/index.php?X=MATH&H=21#text21
    if (s == 0.0) {
        v = math.floor(255 * v + 0.5).tointeger()
        return [v, v, v]
    }
    h = h - math.floor(h)
    h = h * 6.0
    i <- math.floor(h).tointeger()
    f <- h - i // fractional part of h
    x <- v * (1.0 - s)
    y <- v * (1.0 - s * f)
    z <- v * (1.0 - s * (1.0 - f))
    x = math.floor(255 * x + 0.5).tointeger()
    y = math.floor(255 * y + 0.5).tointeger()
    z = math.floor(255 * z + 0.5).tointeger()
    v = math.floor(255 * v + 0.5).tointeger()
    switch (i.tointeger()) {
        case 0:  return [v, z, x]
        case 1:  return [y, v, x]
        case 2:  return [x, v, z]
        case 3:  return [x, y, v]
        case 4:  return [z, x, v]
        case 5:  return [v, x, y]
        default: return [0, 0, 0]
    }
}


_hue <- 0.0         // Starting pixel hue (0.0 .. 1.0)
_saturation <- 1.0  // Pixel saturation (0.0 .. 1.0)
_value <- 0.1       // Pixel value / brightness (0.0 .. 1.0)
_hue_incr <- 0.01
 
function rotate() {
    // now fill the clearblob
    local h = _hue
    for(local i = 0; i < PIXELS; i++) {
        if (h >= 1.0) {
            h = h - 1.0
        }
        color <- hsv2rgb(h, _saturation, _value)
        pixels.writePixel(i, color)
        h = h + 1.0 / PIXELS
    }

    _hue = _hue + _hue_incr
    _hue = _hue - math.floor(_hue)
}
 

_mode <- "off"

function animate() {
    switch (_mode) {
        case "rotate_hue":
            rotate()
            break
        default:
            pixels.clearFrame()
            break
            
    }
    // Write the frame buffer to the hardware
    pixels.writeFrame()
    imp.wakeup(0.050, animate)
    _value = 0.1
}


function setMode(mode) {
    server.log("Device received mode " + mode)
    _mode = mode
    _value = 1.0
}


agent.on("setMode", setMode)
animate() // start the process
server.log("Device has started!")
