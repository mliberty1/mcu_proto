server.log("Hello from the device!")

led <- hardware.pin9
led.configure(DIGITAL_OUT, 0)
state <- 0

// Function to blink LED
function blink() {
    // Flip the state variable
    if (state == 0) {
        state = 1
    } else {
        state = 0
    }
    
    // Set LED pin to new value
    led.write(state)         
    
    // Schedule the blink function to run again in 0.5 seconds
    imp.wakeup(0.5, blink)
}
