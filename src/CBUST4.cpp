
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

// Arduino libraries
#include <SPI.h>

// 3rd party libraries
#include <Streaming.h>

// CBUS MCP2515 library
#include <CBUST4.h>

// globals
ACAN_T4 *can;    // CAN bus controller specific object - for MCP2515/25625

//
/// constructor
//

CBUST4::CBUST4() {
  initMembers();
}

CBUST4::CBUST4(CBUSConfig *the_config) : CBUSbase(the_config) {
  initMembers();
}

//CBUST4::CBUST4(CBUSConfig *the_config) : CBUSbase(the_config) {
//  initMembers();
//}

void CBUST4::initMembers() {

  _num_rx_buffers = NUM_RX_BUFFS;
  _num_tx_buffers = NUM_TX_BUFFS;
  eventhandler = NULL;
  eventhandlerex = NULL;
  framehandler = NULL;
  canp = &ACAN_T4::can1;
}

//
/// initialise the CAN controller and buffers, and attach the ISR
/// default poll arg is set to false, so as not to break existing code
//

bool CBUST4::begin(bool poll, SPIClass spi)
{
  uint16_t ret;
  bool retval = false;

  _numMsgsSent = 0;
  _numMsgsRcvd = 0;

  ACAN_T4_Settings settings(CANBITRATE);

  settings.mReceiveBufferSize = _num_rx_buffers;
  settings.mTransmitBufferSize = _num_tx_buffers;

  ret = canp->begin(settings);

  if (ret == 0) {
    // Serial << F("> CAN controller initialised ok") << endl;
    retval = true;
  } else {
    // Serial << F("> error initialising CAN controller, error code = ") << ret << endl;
  }

  return retval;
}

//
/// check for unprocessed messages in the buffer
//

bool CBUST4::available(void) {

  return canp->available();
}

//
/// get next unprocessed message from the buffer

CANFrame CBUST4::getNextMessage(void) {

  CANMessage message;       // ACANT4 frame class

  canp->receive(message);

  _msg.id = message.id;
  _msg.len = message.len;
  _msg.rtr = message.rtr;
  _msg.ext = message.ext;
  memcpy(_msg.data, message.data, message.len);

  ++_numMsgsRcvd;
  return _msg;
}

//
/// send a CBUS message
//

bool CBUST4::sendMessage(CANFrame *msg, bool rtr, bool ext, byte priority) {

  // caller must populate the message data
  // this method will create the correct frame header (CAN ID and priority bits)
  // rtr and ext default to false unless arguments are supplied - see method definition in .h
  // priority defaults to 1011 low/medium

  CANMessage message;       // ACANt4 frame class
  bool ret = false;

  makeHeader(msg, priority);                      // default priority unless user overrides
  message.id = msg->id;
  message.len = msg->len;
  message.rtr = rtr;
  message.ext = ext;
  memcpy(message.data, msg->data, msg->len);

  ret = canp->tryToSend(message);
  _numMsgsSent += ret;

  if (UI) {
    _ledGrn.pulse();
  }

  return ret;
}

//
/// display the CAN bus status instrumentation
//

void CBUST4::printStatus(void) {

  Serial << F("> CBUS status:");
  Serial << F(" messages received = ") << _numMsgsRcvd << F(", sent = ") << _numMsgsSent << F(", receive errors = ") << \
         canp->receiveErrorCounter() << F(", transmit errors = ") << canp->transmitErrorCounter() << endl;
  return;
}

//
/// reset the CAN bus
//

void CBUST4::reset(void) {
  canp->end();
  begin();
}

//
/// set the CAN Port (can1 is the default)
//

void CBUST4::setPort(byte port)
{
  switch (port) {
  case 1:
    canp = &ACAN_T4::can1;
	break;
  case 2:
    canp = &ACAN_T4::can2;
	break;
  case 3:
    canp = &ACAN_T4::can3;
	break;
  default:
    Serial << F("> CAN") << port << F(" does not exist, defaulting to CAN1!");
    canp = &ACAN_T4::can1;
	break;
  }
}

//
/// set the number of CAN frame receive buffers
/// this can be tuned according to bus load and available memory
//

void CBUST4::setNumBuffers(byte num_rx_buffers, byte num_tx_buffers) {
  _num_rx_buffers = num_rx_buffers;
  _num_tx_buffers = num_tx_buffers;
}
