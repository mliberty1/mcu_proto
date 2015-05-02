mcu_proto
=========

This project contains software that demonstrates network controlled blinking
LEDs, the embedded software version of "Hello World".  The primary goal of the
project is to compare different devices for use in prototyping networked 
devices and the "Internet of Things".  This project supports Matt Liberty's 
Embedded Systems Conference 2015 talk.

The doc directory contains the ESC slides and any other documentation.

The server directory contains a Python 3 CherryPy server.  Although the server
can run on any suitable machine, the devices are all coded to search for the
server running at mcu_proto.jetperch.com.  

The remaining directories each contain an implementation for a specific 
hardware device.
