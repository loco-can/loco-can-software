# ATmega328P pins (TQFP-32)

Package pin 1 is the corner with the dot, then counterclockwise (top view).
Arduino numbers match MiniCore / this tree: `D14`–`D19` = `A0`–`A5`, `D20`/`D21` = `PB6`/`PB7` (crystal pins on a Nano).


| Pin | IO   | Arduino  | Int           | Analog | Serial / other     |
| --- | ---- | -------- | ------------- | ------ | ------------------ |
| 1   | PD3  | D3       | INT1, PCINT19 |        | PWM OC2B           |
| 2   | PD4  | D4       | PCINT20       |        | T0, XCK            |
| 3   | GND  | P        |               |        |                    |
| 4   | VCC  |          |               |        |                    |
| 5   | GND  |          |               |        |                    |
| 6   | VCC  |          |               |        |                    |
| 7   | PB6  | D20      | PCINT6        |        | XTAL1 / TOSC1      |
| 8   | PB7  | D21      | PCINT7        |        | XTAL2 / TOSC2      |
| 9   | PD5  | D5       | PCINT21       |        | PWM OC0B, T1       |
| 10  | PD6  | D6       | PCINT22       | AIN0   | PWM OC0A           |
| 11  | PD7  | D7       | PCINT23       | AIN1   |                    |
| 12  | PB0  | D8       | PCINT0        |        | ICP1, CLKO         |
| 13  | PB1  | D9       | PCINT1        |        | PWM OC1A           |
| 14  | PB2  | D10      | PCINT2        |        | PWM OC1B, SPI SS   |
| 15  | PB3  | D11      | PCINT3        |        | PWM OC2A, SPI MOSI |
| 16  | PB4  | D12      | PCINT4        |        | SPI MISO           |
| 17  | PB5  | D13      | PCINT5        |        | SPI SCK            |
| 18  | AVCC |          |               |        | Analog VCC         |
| 19  | ADC6 | A6       |               | A6     | ADC only, no GPIO  |
| 20  | AREF |          |               |        | Analog reference   |
| 21  | GND  |          |               |        | Analog GND         |
| 22  | ADC7 | A7       |               | A7     | ADC only, no GPIO  |
| 23  | PC0  | D14 / A0 | PCINT8        | A0     |                    |
| 24  | PC1  | D15 / A1 | PCINT9        | A1     |                    |
| 25  | PC2  | D16 / A2 | PCINT10       | A2     |                    |
| 26  | PC3  | D17 / A3 | PCINT11       | A3     |                    |
| 27  | PC4  | D18 / A4 | PCINT12       | A4     | I²C SDA            |
| 28  | PC5  | D19 / A5 | PCINT13       | A5     | I²C SCL            |
| 29  | PC6  | RESET    | PCINT14       |        | /RESET             |
| 30  | PD0  | D0       | PCINT16       |        | UART RX            |
| 31  | PD1  | D1       | PCINT17       |        | UART TX            |
| 32  | PD2  | D2       | INT0, PCINT18 |        |                    |


External interrupts: **INT0** = D2, **INT1** = D3. Every GPIO also has a pin-change interrupt (`PCINTn`).

TQFP-32 extras versus DIP-28: **A6**, **A7**, and **PB6/PB7** (D20/D21) when no crystal is wired.