<img align="right" src="arduino_cbus_logo.png"  width="150" height="75">

# Teensy 4.x library for MERG CBUS running over CAN bus

A library that implements the abstract CBUS base class. It contains methods to support the FlexCAN ports within Teensy 4.x 

Note that this library depends on a number of other libraries which must also be downloaded and included in the sketch:

CBUS 			- abstract CBUS base class
ACAN_t4		    - FlexCAN Controller class
CBUSswitch
CBUSLED
CBUSconfig
Streaming		- C++ style output

## Hardware

Currently supports the Teensy 4.x FlexCAN modules via the ACAN_t4 library

## Documentation

See the included example sketch and documentation file

## License

Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
