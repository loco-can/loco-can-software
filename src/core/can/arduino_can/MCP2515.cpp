// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "../../../config.h"

#ifndef MODULE_ARCH_ESP32

#include "MCP2515.h"

#ifdef MODULE_ARCH_AVR
#include <avr/pgmspace.h>
#endif

#define REG_BFPCTRL                0x0c
#define REG_TXRTSCTRL              0x0d

#define REG_CANCTRL                0x0f

#define REG_CNF3                   0x28
#define REG_CNF2                   0x29
#define REG_CNF1                   0x2a

#define REG_TEC                    0x1c
#define REG_REC                    0x1d
#define REG_CANINTE                0x2b
#define REG_CANINTF                0x2c
#define REG_EFLG                   0x2d
#define FLAG_EFLG_TXBO             0x20

#define FLAG_RXnIE(n)              (0x01 << n)
#define FLAG_RXnIF(n)              (0x01 << n)
#define FLAG_TXnIF(n)              (0x04 << n)

#define REG_RXFnSIDH(n)            (0x00 + (n * 4))
#define REG_RXFnSIDL(n)            (0x01 + (n * 4))
#define REG_RXFnEID8(n)            (0x02 + (n * 4))
#define REG_RXFnEID0(n)            (0x03 + (n * 4))

#define REG_RXMnSIDH(n)            (0x20 + (n * 0x04))
#define REG_RXMnSIDL(n)            (0x21 + (n * 0x04))
#define REG_RXMnEID8(n)            (0x22 + (n * 0x04))
#define REG_RXMnEID0(n)            (0x23 + (n * 0x04))

#define REG_TXBnCTRL(n)            (0x30 + (n * 0x10))
#define REG_TXBnSIDH(n)            (0x31 + (n * 0x10))
#define REG_TXBnSIDL(n)            (0x32 + (n * 0x10))
#define REG_TXBnEID8(n)            (0x33 + (n * 0x10))
#define REG_TXBnEID0(n)            (0x34 + (n * 0x10))
#define REG_TXBnDLC(n)             (0x35 + (n * 0x10))
#define REG_TXBnD0(n)              (0x36 + (n * 0x10))

#define REG_RXBnCTRL(n)            (0x60 + (n * 0x10))
#define REG_RXBnSIDH(n)            (0x61 + (n * 0x10))
#define REG_RXBnSIDL(n)            (0x62 + (n * 0x10))
#define REG_RXBnEID8(n)            (0x63 + (n * 0x10))
#define REG_RXBnEID0(n)            (0x64 + (n * 0x10))
#define REG_RXBnDLC(n)             (0x65 + (n * 0x10))
#define REG_RXBnD0(n)              (0x66 + (n * 0x10))

#define FLAG_IDE                   0x08
#define FLAG_SRR                   0x10
#define FLAG_RTR                   0x40
#define FLAG_EXIDE                 0x08

#define FLAG_RXM0                  0x20
#define FLAG_RXM1                  0x40

#ifndef MCP2515_TX_TIMEOUT_MS
#define MCP2515_TX_TIMEOUT_MS 25
#endif


MCP2515Class::MCP2515Class() :
  CANControllerClass(),
  _spiSettings(10E6, MSBFIRST, SPI_MODE0),
  _csPin(MCP2515_DEFAULT_CS_PIN),
  _intPin(MCP2515_DEFAULT_INT_PIN),
  _clockFrequency(MCP2515_DEFAULT_CLOCK_FREQUENCY)
{
}

MCP2515Class::~MCP2515Class()
{
}

int MCP2515Class::begin(long baudRate)
{
  CANControllerClass::begin(baudRate);

  pinMode(_csPin, OUTPUT);

  // start SPI
  SPI.begin();

  reset();

  writeRegister(REG_CANCTRL, 0x80);
  if (readRegister(REG_CANCTRL) != 0x80) {
    return 0;
  }

  /*
   * Keep this table off the stack. A local copy is ~400 bytes and
   * smashes AVR BSS (CS/INT become garbage on the second begin try).
   */
  struct CnfRow {
    int32_t clockFrequency;
    int32_t baudRate;
    uint8_t cnf[3];
  };

#ifdef MODULE_ARCH_AVR
  static const CnfRow CNF_MAPPER[] PROGMEM = {
#else
  static const CnfRow CNF_MAPPER[] = {
#endif
    {  8000000L, 1000000L, { 0x00, 0x80, 0x00 } },
    {  8000000L,  666666L, { 0xC0, 0xB8, 0x01 } },
    {  8000000L,  500000L, { 0x00, 0x90, 0x02 } },
    {  8000000L,  250000L, { 0x00, 0xb1, 0x05 } },
    {  8000000L,  200000L, { 0x00, 0xb4, 0x06 } },
    {  8000000L,  125000L, { 0x01, 0xb1, 0x05 } },
    {  8000000L,  100000L, { 0x01, 0xb4, 0x06 } },
    {  8000000L,   80000L, { 0x01, 0xbf, 0x07 } },
    {  8000000L,   50000L, { 0x03, 0xb4, 0x06 } },
    {  8000000L,   40000L, { 0x03, 0xbf, 0x07 } },
    {  8000000L,   20000L, { 0x07, 0xbf, 0x07 } },
    {  8000000L,   10000L, { 0x0f, 0xbf, 0x07 } },
    {  8000000L,    5000L, { 0x1f, 0xbf, 0x07 } },
    { 16000000L, 1000000L, { 0x00, 0xd0, 0x82 } },
    { 16000000L,  666666L, { 0xC0, 0xF8, 0x81 } },
    { 16000000L,  500000L, { 0x00, 0xf0, 0x86 } },
    { 16000000L,  250000L, { 0x41, 0xf1, 0x85 } },
    { 16000000L,  200000L, { 0x01, 0xfa, 0x87 } },
    { 16000000L,  125000L, { 0x03, 0xf0, 0x86 } },
    { 16000000L,  100000L, { 0x03, 0xfa, 0x87 } },
    { 16000000L,   80000L, { 0x03, 0xff, 0x87 } },
    { 16000000L,   50000L, { 0x07, 0xfa, 0x87 } },
    { 16000000L,   40000L, { 0x07, 0xff, 0x87 } },
    { 16000000L,   20000L, { 0x0f, 0xff, 0x87 } },
    { 16000000L,   10000L, { 0x1f, 0xff, 0x87 } },
    { 16000000L,    5000L, { 0x3f, 0xff, 0x87 } },
  };

  uint8_t cnf[3];
  bool found = false;

  for (unsigned int i = 0; i < (sizeof(CNF_MAPPER) / sizeof(CNF_MAPPER[0])); i++) {
#ifdef MODULE_ARCH_AVR
    const int32_t clockFrequency = (int32_t)pgm_read_dword(&CNF_MAPPER[i].clockFrequency);
    const int32_t rowBaud = (int32_t)pgm_read_dword(&CNF_MAPPER[i].baudRate);
#else
    const int32_t clockFrequency = CNF_MAPPER[i].clockFrequency;
    const int32_t rowBaud = CNF_MAPPER[i].baudRate;
#endif
    if (clockFrequency == _clockFrequency && rowBaud == baudRate) {
#ifdef MODULE_ARCH_AVR
      cnf[0] = pgm_read_byte(&CNF_MAPPER[i].cnf[0]);
      cnf[1] = pgm_read_byte(&CNF_MAPPER[i].cnf[1]);
      cnf[2] = pgm_read_byte(&CNF_MAPPER[i].cnf[2]);
#else
      cnf[0] = CNF_MAPPER[i].cnf[0];
      cnf[1] = CNF_MAPPER[i].cnf[1];
      cnf[2] = CNF_MAPPER[i].cnf[2];
#endif
      found = true;
      break;
    }
  }

  if (!found) {
    return 0;
  }

  writeRegister(REG_CNF1, cnf[0]);
  writeRegister(REG_CNF2, cnf[1]);
  writeRegister(REG_CNF3, cnf[2]);

  writeRegister(REG_CANINTE, FLAG_RXnIE(1) | FLAG_RXnIE(0));
  writeRegister(REG_BFPCTRL, 0x00);
  writeRegister(REG_TXRTSCTRL, 0x00);
  writeRegister(REG_RXBnCTRL(0), FLAG_RXM1 | FLAG_RXM0);
  writeRegister(REG_RXBnCTRL(1), FLAG_RXM1 | FLAG_RXM0);

  writeRegister(REG_CANCTRL, 0x00);
  if (readRegister(REG_CANCTRL) != 0x00) {
    return 0;
  }

  return 1;
}

void MCP2515Class::end()
{
  SPI.end();

  CANControllerClass::end();
}

int MCP2515Class::endPacket()
{
  if (!CANControllerClass::endPacket()) {
    return 0;
  }

  int n = 0;
  uint32_t start = millis();

  /*
   * TXERR (ctrl 0x10) is set on a missing ACK or a bit error. The
   * controller retries on its own. Aborting on that bit cancelled the
   * heartbeat before any retry, so a slow or busy bus always looked like
   * send() failure.
   */
  while (readRegister(REG_TXBnCTRL(n)) & 0x08) {
    if ((millis() - start) > MCP2515_TX_TIMEOUT_MS) {
      return 0;
    }
    if (readRegister(REG_EFLG) & FLAG_EFLG_TXBO) {
      writeRegister(REG_CANCTRL, 0x00);
    }
    yield();
  }

  if (_txExtended) {
    writeRegister(REG_TXBnSIDH(n), _txId >> 21);
    writeRegister(REG_TXBnSIDL(n), (((_txId >> 18) & 0x07) << 5) | FLAG_EXIDE | ((_txId >> 16) & 0x03));
    writeRegister(REG_TXBnEID8(n), (_txId >> 8) & 0xff);
    writeRegister(REG_TXBnEID0(n), _txId & 0xff);
  } else {
    writeRegister(REG_TXBnSIDH(n), _txId >> 3);
    writeRegister(REG_TXBnSIDL(n), _txId << 5);
    writeRegister(REG_TXBnEID8(n), 0x00);
    writeRegister(REG_TXBnEID0(n), 0x00);
  }

  if (_txRtr) {
    writeRegister(REG_TXBnDLC(n), 0x40 | _txLength);
  } else {
    writeRegister(REG_TXBnDLC(n), _txLength);

    for (int i = 0; i < _txLength; i++) {
      writeRegister(REG_TXBnD0(n) + i, _txData[i]);
    }
  }

  writeRegister(REG_TXBnCTRL(n), 0x08);

  start = millis();

  while (readRegister(REG_TXBnCTRL(n)) & 0x08) {
    if ((millis() - start) > MCP2515_TX_TIMEOUT_MS) {
      #ifdef DEBUG
        Serial.print("> CAN TX timeout ctrl=0x");
        Serial.print(readRegister(REG_TXBnCTRL(n)), HEX);
        Serial.print(" eflg=0x");
        Serial.print(readRegister(REG_EFLG), HEX);
        Serial.print(" tec=");
        Serial.println(readRegister(REG_TEC));
      #endif
      /* Leave TXREQ set so the frame is still retried until a node ACKs. */
      return 0;
    }
    if (readRegister(REG_EFLG) & FLAG_EFLG_TXBO) {
      writeRegister(REG_CANCTRL, 0x00);
    }
    yield();
  }

  modifyRegister(REG_CANINTF, FLAG_TXnIF(n), 0x00);
  return 1;
}

int MCP2515Class::parsePacket()
{
  int n;

  uint8_t intf = readRegister(REG_CANINTF);

  if (intf & FLAG_RXnIF(0)) {
    n = 0;
  } else if (intf & FLAG_RXnIF(1)) {
    n = 1;
  } else {
    _rxId = -1;
    _rxExtended = false;
    _rxRtr = false;
    _rxLength = 0;
    return 0;
  }

  _rxExtended = (readRegister(REG_RXBnSIDL(n)) & FLAG_IDE) ? true : false;

  uint32_t idA = ((readRegister(REG_RXBnSIDH(n)) << 3) & 0x07f8) | ((readRegister(REG_RXBnSIDL(n)) >> 5) & 0x07);
  if (_rxExtended) {
    uint32_t idB = (((uint32_t)(readRegister(REG_RXBnSIDL(n)) & 0x03) << 16) & 0x30000) | ((readRegister(REG_RXBnEID8(n)) << 8) & 0xff00) | readRegister(REG_RXBnEID0(n));

    _rxId = (idA << 18) | idB;
    _rxRtr = (readRegister(REG_RXBnDLC(n)) & FLAG_RTR) ? true : false;
  } else {
    _rxId = idA;
    _rxRtr = (readRegister(REG_RXBnSIDL(n)) & FLAG_SRR) ? true : false;
  }
  _rxDlc = readRegister(REG_RXBnDLC(n)) & 0x0f;
  _rxIndex = 0;

  if (_rxRtr) {
    _rxLength = 0;
  } else {
    _rxLength = _rxDlc;

    for (int i = 0; i < _rxLength; i++) {
      _rxData[i] = readRegister(REG_RXBnD0(n) + i);
    }
  }

  modifyRegister(REG_CANINTF, FLAG_RXnIF(n), 0x00);

  return _rxDlc;
}

void MCP2515Class::onReceive(void(*callback)(int))
{
  CANControllerClass::onReceive(callback);

  if (_intPin < 0) {
    return;
  }

  pinMode(_intPin, INPUT);

  if (callback) {
    SPI.usingInterrupt(digitalPinToInterrupt(_intPin));
    attachInterrupt(digitalPinToInterrupt(_intPin), MCP2515Class::onInterrupt, LOW);
  } else {
    detachInterrupt(digitalPinToInterrupt(_intPin));
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.notUsingInterrupt(digitalPinToInterrupt(_intPin));
#endif
  }
}

int MCP2515Class::filter(int id, int mask)
{
  id &= 0x7ff;
  mask &= 0x7ff;

  // config mode
  writeRegister(REG_CANCTRL, 0x80);
  if (readRegister(REG_CANCTRL) != 0x80) {
    return 0;
  }

  for (int n = 0; n < 2; n++) {
    // standard only
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM0);
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM0);

    writeRegister(REG_RXMnSIDH(n), mask >> 3);
    writeRegister(REG_RXMnSIDL(n), mask << 5);
    writeRegister(REG_RXMnEID8(n), 0);
    writeRegister(REG_RXMnEID0(n), 0);
  }

  for (int n = 0; n < 6; n++) {
    writeRegister(REG_RXFnSIDH(n), id >> 3);
    writeRegister(REG_RXFnSIDL(n), id << 5);
    writeRegister(REG_RXFnEID8(n), 0);
    writeRegister(REG_RXFnEID0(n), 0);
  }

  // normal mode
  writeRegister(REG_CANCTRL, 0x00);
  if (readRegister(REG_CANCTRL) != 0x00) {
    return 0;
  }

  return 1;
}

int MCP2515Class::filterExtended(long id, long mask)
{
  id &= 0x1FFFFFFF;
  mask &= 0x1FFFFFFF;

  // config mode
  writeRegister(REG_CANCTRL, 0x80);
  if (readRegister(REG_CANCTRL) != 0x80) {
    return 0;
  }

  for (int n = 0; n < 2; n++) {
    // extended only
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM1);
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM1);

    writeRegister(REG_RXMnSIDH(n), mask >> 21);
    writeRegister(REG_RXMnSIDL(n), (((mask >> 18) & 0x03) << 5) | FLAG_EXIDE | ((mask >> 16) & 0x03));
    writeRegister(REG_RXMnEID8(n), (mask >> 8) & 0xff);
    writeRegister(REG_RXMnEID0(n), mask & 0xff);
  }

  for (int n = 0; n < 6; n++) {
    writeRegister(REG_RXFnSIDH(n), id >> 21);
    writeRegister(REG_RXFnSIDL(n), (((id >> 18) & 0x03) << 5) | FLAG_EXIDE | ((id >> 16) & 0x03));
    writeRegister(REG_RXFnEID8(n), (id >> 8) & 0xff);
    writeRegister(REG_RXFnEID0(n), id & 0xff);
  }

  // normal mode
  writeRegister(REG_CANCTRL, 0x00);
  if (readRegister(REG_CANCTRL) != 0x00) {
    return 0;
  }

  return 1;
}

int MCP2515Class::observe()
{
  writeRegister(REG_CANCTRL, 0x60);
  if (readRegister(REG_CANCTRL) != 0x60) {
    return 0;
  }

  return 1;
}

int MCP2515Class::loopback()
{
  writeRegister(REG_CANCTRL, 0x40);
  if (readRegister(REG_CANCTRL) != 0x40) {
    return 0;
  }

  return 1;
}

int MCP2515Class::sleep()
{
  writeRegister(REG_CANCTRL, 0x01);
  if (readRegister(REG_CANCTRL) != 0x01) {
    return 0;
  }

  return 1;
}

int MCP2515Class::wakeup()
{
  writeRegister(REG_CANCTRL, 0x00);
  if (readRegister(REG_CANCTRL) != 0x00) {
    return 0;
  }

  return 1;
}

void MCP2515Class::setPins(int cs, int irq)
{
  _csPin = cs;
  _intPin = irq;
}

void MCP2515Class::setSPIFrequency(uint32_t frequency)
{
  _spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
}

void MCP2515Class::setClockFrequency(long clockFrequency)
{
  _clockFrequency = clockFrequency;
}

void MCP2515Class::dumpRegisters(Stream& out)
{
  for (int i = 0; i < 128; i++) {
    byte b = readRegister(i);

    out.print("0x");
    if (i < 16) {
      out.print('0');
    }
    out.print(i, HEX);
    out.print(": 0x");
    if (b < 16) {
      out.print('0');
    }
    out.println(b, HEX);
  }
}

void MCP2515Class::reset()
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(0xc0);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();

  delayMicroseconds(10);
}

void MCP2515Class::handleInterrupt()
{
  if (readRegister(REG_CANINTF) == 0) {
    return;
  }

  while (parsePacket() || _rxId != -1) {
    _onReceive(available());
  }
}

uint8_t MCP2515Class::readRegister(uint8_t address)
{
  uint8_t value;

  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(0x03);
  SPI.transfer(address);
  value = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();

  return value;
}

void MCP2515Class::modifyRegister(uint8_t address, uint8_t mask, uint8_t value)
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(0x05);
  SPI.transfer(address);
  SPI.transfer(mask);
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();
}

void MCP2515Class::writeRegister(uint8_t address, uint8_t value)
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(0x02);
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();
}

void MCP2515Class::onInterrupt()
{
  CAN.handleInterrupt();
}

MCP2515Class CAN;

#endif
