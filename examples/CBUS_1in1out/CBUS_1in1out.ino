
//
///
//

/*
  Copyright (C) Phil Taylor 2022 (phil@m0vse.uk)
  Copyright (C) Duncan Greenwood 2017 (duncan_greenwood@hotmail.com)

  This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                  and indicate if changes were made. You may do so in any reasonable manner,
                  but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                 your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                 legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

*/

/*
      3rd party libraries needed for compilation: (not for binary-only distributions)

      Streaming   -- C++ stream style output, v5, (http://arduiniana.org/libraries/streaming/)
      ACAN2515    -- library to support the MCP2515/25625 CAN controller IC
*/

// 3rd party libraries
#include <Streaming.h>

// CBUS library header files
#include <CBUST4.h>               // CAN controller and CBUS class
#include <CBUSswitch.h>             // pushbutton switch
#include <CBUSLED.h>                // CBUS LEDs
#include <CBUSconfig.h>             // module configuration
#include <CBUSParams.h>             // CBUS parameters
#include <cbusdefs.h>               // MERG CBUS constants

// constants
const byte VER_MAJ = 1;             // code major version
const char VER_MIN = 'a';           // code minor version
const byte VER_BETA = 0;            // code beta sub-version
const byte MODULE_ID = 99;          // CBUS module type

const byte LED_GRN = LED_BUILTIN;             // CBUS green SLiM LED pin
const byte LED_YLW = 0;             // CBUS yellow FLiM LED pin
const byte SWITCH0 = 0;             // CBUS push button switch pin

// CBUS objects
CBUSConfig modconfig;               // configuration object
CBUST4 CBUS(&modconfig);          // CBUS object
CBUSLED ledGrn, ledYlw;             // two LED objects
CBUSSwitch pb_switch;               // switch object

// module objects
CBUSSwitch moduleSwitch;            // an example switch as input
CBUSLED moduleLED;                  // an example LED as output

// module name, must be 7 characters, space padded.
unsigned char mname[7] = { '1', 'I', 'N', '1', 'O', 'U', 'T' };

// forward function declarations
void eventhandler(byte index, byte opc);

//
/// setup CBUS - runs once at power on from setup()
//

void setupCBUS() {

  // set config layout parameters
  modconfig.EE_NVS_START = 10;
  modconfig.EE_NUM_NVS = 10;
  modconfig.EE_EVENTS_START = 20;
  modconfig.EE_MAX_EVENTS = 32;
  modconfig.EE_NUM_EVS = 1;
  modconfig.EE_BYTES_PER_EVENT = (modconfig.EE_NUM_EVS + 4);

  // initialise and load configuration
  modconfig.setEEPROMtype(EEPROM_INTERNAL);
  modconfig.begin();

  Serial << F("> mode = ") << ((modconfig.FLiM) ? "FLiM" : "SLiM") << F(", CANID = ") << modconfig.CANID;
  Serial << F(", NN = ") << modconfig.nodeNum << endl;

  // show code version and copyright notice
  printConfig();

  // set module parameters
  CBUSParams params(modconfig);
  params.setVersion(VER_MAJ, VER_MIN, VER_BETA);
  params.setModuleId(MODULE_ID);
  params.setFlags(PF_FLiM | PF_COMBI);

  // assign to CBUS
  CBUS.setParams(params.getParams());
  CBUS.setName(mname);

  // set CBUS LED pins and assign to CBUS
  ledGrn.setPin(LED_GRN);
  ledYlw.setPin(LED_YLW);
  CBUS.setLEDs(ledGrn, ledYlw);

  // initialise CBUS switch and assign to CBUS
  pb_switch.setPin(SWITCH0, LOW);
  pb_switch.run();
  CBUS.setSwitch(pb_switch);

  // module reset - if switch is depressed at startup and module is in SLiM mode
  if (pb_switch.isPressed() && !modconfig.FLiM) {
    Serial << F("> switch was pressed at startup in SLiM mode") << endl;
    modconfig.resetModule(ledGrn, ledYlw, pb_switch);
  }

  // opportunity to set default NVs after module reset
  if (modconfig.isResetFlagSet()) {
    Serial << F("> module has been reset") << endl;
    modconfig.clearResetFlag();
  }

  // register our CBUS event handler, to receive event messages of learned events
  CBUS.setEventHandler(eventhandler);

  // set CBUS LEDs to indicate mode
  CBUS.indicateMode(modconfig.FLiM);


  // configure and start CAN bus and CBUS message processing
  // These settings can be changed but will default to the below:
  // CBUS.setNumBuffers(32, 16);      // more buffers = more memory used, fewer = less
  // CBUS.setPort(1);           // select CAN port number.

  if (!CBUS.begin()) {
    Serial << F("> error starting CBUS") << endl;
  }
}

//
/// setup - runs once at power on
//

void setup() {

  Serial.begin (115200);
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
  Serial << endl << endl << F("> ** CBUS 1 in 1 out v1 ** ") << __FILE__ << endl;

  setupCBUS();

  // configure the module switch, attached to pin 7, active low
  moduleSwitch.setPin(7, LOW);

  // configure the module LED, attached to pin 8 via a 1K resistor
  moduleLED.setPin(8);

  // end of setup
  Serial << F("> ready") << endl << endl;
}

//
/// loop - runs forever
//

void loop() {

  //
  /// do CBUS message, switch and LED processing
  //

  CBUS.process();

  //
  /// process console commands
  //

  processSerialInput();

  //
  /// give the switch and LED code some time to run
  //

  moduleSwitch.run();
  moduleLED.run();

  //
  /// Check if smich changed and do any processing for this change.
  //

  processModuleSwitchChange();

  //
  /// check CAN message buffers
  //

  if (CBUS.canp->receiveBufferPeakCount() > CBUS.canp->receiveBufferSize()) {
    Serial << F("> receive buffer overflow") << endl;
  }

  if (CBUS.canp->transmitBufferPeakCount() > CBUS.canp->transmitBufferSize()) {
    Serial << F("> transmit buffer overflow") << endl;
  }

  // bottom of loop()
}

//
/// test for switch input
/// as an example, it must be have been pressed or released for at least half a second
/// then send a long CBUS event with opcode ACON for on and ACOF for off
/// event number (EN) is 1

/// you can just watch for this event in FCU or JMRI, or teach it to another CBUS consumer module
//
void processModuleSwitchChange() {

  if (moduleSwitch.stateChanged()) {

    CANFrame msg;
    msg.id = modconfig.CANID;
    msg.len = 5;
    msg.data[0] = (moduleSwitch.isPressed() ? OPC_ACON : OPC_ACOF);
    msg.data[1] = highByte(modconfig.nodeNum);
    msg.data[2] = lowByte(modconfig.nodeNum);
    msg.data[3] = 0;
    msg.data[4] = 1;            // event number (EN) = 1

    if (CBUS.sendMessage(&msg)) {
      Serial << F("> sent CBUS message") << endl;
    } else {
      Serial << F("> error sending CBUS message") << endl;
    }
  }
}

//
/// user-defined event processing function
/// called from the CBUS library when a learned event is received
/// it receives the event table index and the CAN frame
//

void eventhandler(byte index, CANFrame *msg) {

  // as an example, control an LED

  Serial << F("> event handler: index = ") << index << F(", opcode = 0x") << _HEX(msg->data[0]) << endl;

  // read the value of the first event variable (EV) associated with this learned event
  byte evval = modconfig.getEventEVval(index, 1);
  Serial << F("> EV1 = ") << evval << endl;

  // set the LED according to the opcode of the received event, if the first EV equals 0
  // we turn on the LED and if the first EV equals 1 we use the blink() method of the LED object as an example

  if (msg->data[0] == OPC_ACON) {
    if (evval == 0) {
      Serial << F("> switching the LED on") << endl;
      moduleLED.on();
    } else if (evval == 1) {
      Serial << F("> switching the LED to blink") << endl;
      moduleLED.blink();
    }
  } else if (msg->data[0] == OPC_ACOF) {
    Serial << F("> switching the LED off") << endl;
    moduleLED.off();
  }
}

//
/// print code version config details and copyright notice
//

void printConfig(void) {

  // code version
  Serial << F("> code version = ") << VER_MAJ << VER_MIN << F(" beta ") << VER_BETA << endl;
  Serial << F("> compiled on ") << __DATE__ << F(" at ") << __TIME__ << F(", compiler ver = ") << __cplusplus << endl;

  // copyright
  Serial << F("> ?? Phil Taylor (MERG M9146) 2022") << endl;
}

//
/// command interpreter for serial console input
//

void processSerialInput(void) {

  byte uev = 0;
  char msgstr[32], dstr[32];

  if (Serial.available()) {

    char c = Serial.read();

    switch (c) {

    case 'n':

      // node config
      printConfig();

      // node identity
      Serial << F("> CBUS node configuration") << endl;
      Serial << F("> mode = ") << (modconfig.FLiM ? "FLiM" : "SLiM") << F(", CANID = ") << modconfig.CANID << F(", node number = ") << modconfig.nodeNum << endl;
      Serial << endl;
      break;

    case 'e':

      // EEPROM learned event data table
      Serial << F("> stored events ") << endl;
      Serial << F("  max events = ") << modconfig.EE_MAX_EVENTS << F(" EVs per event = ") << modconfig.EE_NUM_EVS << F(" bytes per event = ") << modconfig.EE_BYTES_PER_EVENT << endl;

      for (byte j = 0; j < modconfig.EE_MAX_EVENTS; j++) {
        if (modconfig.getEvTableEntry(j) != 0) {
          ++uev;
        }
      }

      Serial << F("  stored events = ") << uev << F(", free = ") << (modconfig.EE_MAX_EVENTS - uev) << endl;
      Serial << F("  using ") << (uev * modconfig.EE_BYTES_PER_EVENT) << F(" of ") << (modconfig.EE_MAX_EVENTS * modconfig.EE_BYTES_PER_EVENT) << F(" bytes") << endl << endl;

      Serial << F("  Ev#  |  NNhi |  NNlo |  ENhi |  ENlo | ");

      for (byte j = 0; j < (modconfig.EE_NUM_EVS); j++) {
        sprintf(dstr, "EV%03d | ", j + 1);
        Serial << dstr;
      }

      Serial << F("Hash |") << endl;

      Serial << F(" --------------------------------------------------------------") << endl;

      // for each event data line
      for (byte j = 0; j < modconfig.EE_MAX_EVENTS; j++) {

        if (modconfig.getEvTableEntry(j) != 0) {
          sprintf(dstr, "  %03d  | ", j);
          Serial << dstr;

          // for each data byte of this event
          for (byte e = 0; e < (modconfig.EE_NUM_EVS + 4); e++) {
            sprintf(dstr, " 0x%02hx | ", modconfig.readEEPROM(modconfig.EE_EVENTS_START + (j * modconfig.EE_BYTES_PER_EVENT) + e));
            Serial << dstr;
          }

          sprintf(dstr, "%4d |", modconfig.getEvTableEntry(j));
          Serial << dstr << endl;
        }
      }

      Serial << endl;

      break;

    // NVs
    case 'v':

      // note NVs number from 1, not 0
      Serial << "> Node variables" << endl;
      Serial << F("   NV   Val") << endl;
      Serial << F("  --------------------") << endl;

      for (byte j = 1; j <= modconfig.EE_NUM_NVS; j++) {
        byte v = modconfig.readNV(j);
        sprintf(msgstr, " - %02d : %3hd | 0x%02hx", j, v, v);
        Serial << msgstr << endl;
      }

      Serial << endl << endl;

      break;

    // CAN bus status
    case 'c':

      CBUS.printStatus();
      break;

    case 'h':
      // event hash table
      modconfig.printEvHashTable(false);
      break;

    case 'y':
      // reset CAN bus and CBUS message processing
      CBUS.reset();
      break;

    case '*':
      // reboot
      modconfig.reboot();
      break;

    case 'm':
      // free memory
      Serial << F("> free SRAM = ") << modconfig.freeSRAM() << F(" bytes") << endl;
      break;
      // Renegotiate
    case 'r':
      Serial << "> Renegotiate connection" << endl;
      CBUS.renegotiate();
      break;
    case 'z':
      // reset module, clear EEPROM
      static bool ResetRq = false;
      static unsigned long ResWaitTime;
      if (!ResetRq) {
        // Start timeout timer
        Serial << F("Reset & EEPROM wipe requested. Press 'z' again with 2 secs to confirm") << endl;
        ResWaitTime = millis();
        ResetRq = true;
      } else {
        // This is a confirmed request
        if (ResetRq && ((millis() - ResWaitTime) > 2000)) {
          Serial << F("timeout expired, reset not performed") << endl;
          ResetRq = false;
        } else {
          // Reset confirmed within timeout
          Serial << F("RESETTING AND WIPING EEPROM") << endl;
          modconfig.resetModule();
          ResetRq=false;
        }
      }
      break; 
    case '\r':
    case '\n':
      Serial << endl;
      break;

    default:
      // Serial << F("> unknown command ") << c << endl;
      break;
    }
  }
}
