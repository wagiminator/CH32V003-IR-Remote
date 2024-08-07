// ===================================================================================
// Project:   IR Remote Control based on CH32V003
// Version:   v1.0
// Year:      2024
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// IR remote control using a CH32V003. Timer1 generates a carrier frequency with a 
// duty cycle of 25% on the output pin to the IR LED. The signal is modulated by 
// toggling the pin to output PWM/output HIGH.
//
// References:
// -----------
// - Charles Lohr:          https://github.com/cnlohr/ch32v003fun
// - San Bergmans:          https://www.sbprojects.net/knowledge/ir/index.php
// - Christoph Niessen:     http://chris.cnie.de/avr/tcm231421.html
// - David Johnson-Davies:  http://www.technoblogy.com/show?UVE
// - WCH Nanjing Qinheng Microelectronics:  http://wch.cn
//
// Compilation Instructions:
// -------------------------
// - Make sure GCC toolchain (gcc-riscv64-unknown-elf, newlib) and Python3 with rvprog
//   are installed. In addition, Linux requires access rights to WCH-LinkE programmer.
// - Connect the WCH-LinkE programmer to the MCU board.
// - Run 'make flash'.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <config.h>                         // user configurations
#include <system.h>                         // system functions
#include <gpio.h>                           // GPIO functions

// ===================================================================================
// Timer/PWM and IR LED Control Functions
// ===================================================================================

// Macros to switch on/off IR LED
#define IR_on()     PIN_alternate(PIN_LED)  // output PWM
#define IR_off()    PIN_output(PIN_LED)     // output LOW

// Init timer for PWM on PA2 (timer1, channel2 N)
void PWM_init(void) {
  RCC->APB2PCENR |= RCC_IOPAEN      // enable I/O Port A
                  | RCC_AFIOEN      // enable auxiliary I/O functions
                  | RCC_TIM1EN;     // enable timer 1 module
  TIM1->CCER      = TIM_CC2NE;      // enable channel 2N output
  TIM1->CHCTLR1   = TIM_OC2M;       // set channel 2 PWM mode 2
  TIM1->BDTR      = TIM_MOE;        // main output enable
  TIM1->CTLR1     = TIM_ARPE        // enable automatic reload register
                  | TIM_CEN;        // enable timer
  PIN_high(PIN_LED);                // set LED pin to output high
  PIN_output(PIN_LED);
}

// Set timer PWM frequency and 25% duty cycle
#define PWM_set(freq) {                   \
  TIM1->ATRLR  = F_CPU / (freq) - 1;      \
  TIM1->CH2CVR = F_CPU / (freq) / 4 + 1;  \
  TIM1->SWEVGR = TIM_UG;                  \
}

// ===================================================================================
// Button Functions
// ===================================================================================
uint8_t KEY_read(void) {
  if(!PIN_read(PIN_KEY1)) return 1;
  if(!PIN_read(PIN_KEY2)) return 2;
  if(!PIN_read(PIN_KEY3)) return 3;
  if(!PIN_read(PIN_KEY4)) return 4;
  if(!PIN_read(PIN_KEY5)) return 5;
  return 0;
}

// ===================================================================================
// NEC Protocol Implementation
// ===================================================================================
//
// The NEC protocol uses pulse distance modulation.
//
//       +---------+     +-+ +-+   +-+   +-+ +-    ON
//       |         |     | | | |   | |   | | |          bit0:  562.5us
//       |   9ms   |4.5ms| |0| | 1 | | 1 | |0| ...
//       |         |     | | | |   | |   | | |          bit1: 1687.5us
// ------+         +-----+ +-+ +---+ +---+ +-+     OFF
//
// IR telegram starts with a 9ms leading burst followed by a 4.5ms pause.
// Afterwards 4 data bytes are transmitted, least significant bit first.
// A "0" bit is a 562.5us burst followed by a 562.5us pause, a "1" bit is
// a 562.5us burst followed by a 1687.5us pause. A final 562.5us burst
// signifies the end of the transmission. The four data bytes are in order:
// - the 8-bit address for the receiving device,
// - the 8-bit logical inverse of the address,
// - the 8-bit command and
// - the 8-bit logical inverse of the command.
// The Extended NEC protocol uses 16-bit addresses. Instead of sending an
// 8-bit address and its logically inverse, first the low byte and then the
// high byte of the address is transmitted.
//
// If the key on the remote controller is kept depressed, a repeat code
// will be issued consisting of a 9ms leading burst, a 2.25ms pause and
// a 562.5us burst to mark the end. The repeat code will continue to be
// sent out at 108ms intervals, until the key is finally released.

// Define carrier frequency in Hertz
#define NEC_FREQ            38000

// Macros to modulate the signals according to NEC protocol (compensated timings for 1.5Mhz)
#define NEC_startPulse()    {IR_on(); DLY_us(9000); IR_off(); DLY_us(4500);}
#define NEC_repeatPulse()   {IR_on(); DLY_us(9000); IR_off(); DLY_us(2250);}
#define NEC_normalPulse()   {IR_on(); DLY_us( 552); IR_off(); DLY_us( 543);}
#define NEC_bit1Pause()     DLY_us(1125)  // 1687.5us - 562.5us = 1125us
#define NEC_repeatCode()    {DLY_ms(40); NEC_repeatPulse(); NEC_normalPulse(); DLY_ms(56);}

// Send a single byte via IR
void NEC_sendByte(uint8_t value) {
  for(uint8_t i=8; i; i--, value>>=1) {   // send 8 bits, LSB first
    NEC_normalPulse();                    // 562us burst, 562us pause
    if(value & 1) NEC_bit1Pause();        // extend pause if bit is 1
  }
}

// Send complete telegram (start frame + address + command) via IR
void NEC_sendCode(uint16_t addr, uint8_t cmd) {
  // Prepare carrier wave
  PWM_set(NEC_FREQ);          // set PWM frequency and duty cycle

  // Send telegram
  NEC_startPulse();           // 9ms burst + 4.5ms pause to signify start of transmission
  if(addr > 0xff) {           // if extended NEC protocol (16-bit address):
    NEC_sendByte(addr);       // send address low byte
    NEC_sendByte(addr >> 8);  // send address high byte
  } 
  else {                      // if standard NEC protocol (8-bit address):
    NEC_sendByte(addr);       // send address byte
    NEC_sendByte(~addr);      // send inverse of address byte
  }
  NEC_sendByte(cmd);          // send command byte
  NEC_sendByte(~cmd);         // send inverse of command byte
  NEC_normalPulse();          // 562us burst to signify end of transmission
  while(KEY_read()) NEC_repeatCode();  // send repeat command until button is released
}

// ===================================================================================
// SAMSUNG Protocol Implementation
// ===================================================================================
//
// The SAMSUNG protocol corresponds to the NEC protocol, except that the start pulse is
// 4.5ms long and the address byte is sent twice. The telegram is repeated every 108ms
// as long as the button is pressed.

#define SAM_startPulse()    {IR_on(); DLY_us(4500); IR_off(); DLY_us(4500);}
#define SAM_repeatPause()   DLY_ms(44)

// Send complete telegram (start frame + address + command) via IR
void SAM_sendCode(uint8_t addr, uint8_t cmd) {
  // Prepare carrier wave
  PWM_set(NEC_FREQ);          // set PWM frequency and duty cycle

  // Send telegram
  do {
    SAM_startPulse();         // 4.5ms burst + 4.5ms pause to signify start of transmission
    NEC_sendByte(addr);       // send address byte
    NEC_sendByte(addr);       // send address byte again
    NEC_sendByte(cmd);        // send command byte
    NEC_sendByte(~cmd);       // send inverse of command byte
    NEC_normalPulse();        // 562us burst to signify end of transmission
    SAM_repeatPause();        // wait for next repeat
  } while(KEY_read());        // repeat sending until button is released
}

// ===================================================================================
// RC-5 Protocol Implementation
// ===================================================================================
//
// The RC-5 protocol uses bi-phase modulation (Manchester coding).
//
//   +-------+                     +-------+    ON
//           |                     |
//     889us | 889us         889us | 889us
//           |                     |
//           +-------+     +-------+            OFF
//
//   |<-- Bit "0" -->|     |<-- Bit "1" -->|
//
// IR telegram starts with two start bits. The first bit is always "1",
// the second bit is "1" in the original protocol and inverted 7th bit
// of the command in the extended RC-5 protocol. The third bit toggles
// after each button release. The next five bits represent the device
// address, MSB first and the last six bits represent the command, MSB
// first.
//
// As long as a key remains down the telegram will be repeated every
// 114ms without changing the toggle bit.

// Define carrier frequency in Hertz
#define RC5_FREQ            36000

// Macros to modulate the signals according to RC-5 protocol (compensated timings for 1.5Mhz)
#define RC5_bit0Pulse()     {IR_on();  DLY_us(880); IR_off(); DLY_us(871);}
#define RC5_bit1Pulse()     {IR_off(); DLY_us(880); IR_on();  DLY_us(871);}
#define RC5_repeatPause()   DLY_ms(89) // 114ms - 14 * 2 * 889us

// Bitmasks
#define RC5_startBit        0b0010000000000000
#define RC5_cmdBit7         0b0001000000000000
#define RC5_toggleBit       0b0000100000000000

// Toggle variable
uint8_t RC5_toggle = 0;

// Send complete telegram (startbits + togglebit + address + command) via IR
void RC5_sendCode(uint8_t addr, uint8_t cmd) {
  // Prepare carrier wave
  PWM_set(RC5_FREQ);                          // set PWM frequency and duty cycle

  // Prepare the message
  uint16_t message = addr << 6;               // shift address to the right position
  message |= (cmd & 0x3f);                    // add the low 6 bits of the command
  if(~cmd & 0x40) message |= RC5_cmdBit7;     // add inverse of 7th command bit
  message |= RC5_startBit;                    // add start bit
  if(RC5_toggle) message |= RC5_toggleBit;    // add toggle bit

  // Send the message
  do {
    uint16_t bitmask = RC5_startBit;          // set the bitmask to first bit to send
    for(uint8_t i=14; i; i--, bitmask>>=1) {  // 14 bits, MSB first
      (message & bitmask) ? (RC5_bit1Pulse()) : (RC5_bit0Pulse());  // send the bit
    }
    IR_off();                                 // switch off IR LED
    RC5_repeatPause();                        // wait for next repeat
  } while(KEY_read());                        // repeat sending until button is released
  RC5_toggle ^= 1;                            // toggle the toggle bit
}

// ===================================================================================
// SONY SIRC Protocol Implementation
// ===================================================================================
//
// The SONY SIRC protocol uses pulse length modulation.
//
//       +--------------------+     +-----+     +----------+     +-- ON
//       |                    |     |     |     |          |     |
//       |       2400us       |600us|600us|600us|  1200us  |600us|   ...
//       |                    |     |     |     |          |     |
// ------+                    +-----+     +-----+          +-----+   OFF
//
//       |<------ Start Frame ----->|<- Bit=0 ->|<--- Bit=1 ---->| 
//
// A "0" bit is a 600us burst followed by a 600us space, a "1" bit is a
// 1200us burst followed by a 600us space. An IR telegram starts with a
// 2400us leading burst followed by a 600us space. The command and
// address bits are then transmitted, LSB first. Depending on the
// protocol version, these are in detail:
// - 12-bit version: 7 command bits, 5 address bits
// - 15-bit version: 7 command bits, 8 address bits
// - 20-bit version: 7 command bits, 5 address bits, 8 extended bits
//
// As long as a key remains down the message will be repeated every 45ms.

// Define carrier frequency in Hertz
#define SON_FREQ            40000

// Macros to modulate the signals according to SONY protocol (compensated timings for 1.5Mhz)
#define SON_startPulse()    {IR_on(); DLY_us(2400); IR_off(); DLY_us( 579);}
#define SON_bit0Pulse()     {IR_on(); DLY_us( 587); IR_off(); DLY_us( 579);}
#define SON_bit1Pulse()     {IR_on(); DLY_us(1192); IR_off(); DLY_us( 579);}
#define SON_repeatPause()   DLY_ms(27)

// Send "number" of bits of "value" via IR
void SON_sendByte(uint8_t value, uint8_t number) {
  do {                                        // send number of bits, LSB first
    (value & 1) ? (SON_bit1Pulse()) : (SON_bit0Pulse());  // send bit
    value>>=1;                                // next bit
  } while(--number);
}

// Send complete telegram (start frame + command + address) via IR
void SON_sendCode(uint16_t addr, uint8_t cmd, uint8_t bits) {
  // Prepare carrier wave
  PWM_set(SON_FREQ);                          // set PWM frequency and duty cycle

  // Send telegram
  do {
    SON_startPulse();                         // signify start of transmission
    SON_sendByte(cmd, 7);                     // send 7 command bits
    switch(bits) {
      case 12: SON_sendByte(addr, 5); break;  // 12-bit version: send 5 address bits
      case 15: SON_sendByte(addr, 8); break;  // 15-bit version: send 8 address bits
      case 20: SON_sendByte(addr, 8); SON_sendByte(addr>>8, 5); break; // 20-bit: 13 bits
      default: break;
    }
    SON_repeatPause();                        // wait until next repeat
  } while(KEY_read());                        // repeat sending until button is released
}

// ===================================================================================
// Main Function
// ===================================================================================
int main(void) {
  // Setup
  PIN_input_PU(PIN_KEY1);                     // set key pins to input pullup
  PIN_input_PU(PIN_KEY2);
  PIN_input_PU(PIN_KEY3);
  PIN_input_PU(PIN_KEY4);
  PIN_input_PU(PIN_KEY5);

  PIN_EVT_set(PIN_KEY1, PIN_EVT_FALLING);     // enable event on falling edge (key press)
  PIN_EVT_set(PIN_KEY2, PIN_EVT_FALLING);
  PIN_EVT_set(PIN_KEY3, PIN_EVT_FALLING);
  PIN_EVT_set(PIN_KEY4, PIN_EVT_FALLING);
  PIN_EVT_set(PIN_KEY5, PIN_EVT_FALLING);

  PWM_init();                                 // init timer for PWM on LED pin

  // Loop
  while(1) {
    STDBY_WFE_now();                          // put MCU to standby, wake up by event
    DLY_ms(1);                                // debounce
    uint8_t key = KEY_read();                 // read pressed key
    switch(key) {                             // act according to key
      case 1: KEY1; break;
      case 2: KEY2; break;
      case 3: KEY3; break;
      case 4: KEY4; break;
      case 5: KEY5; break;
      default: break;
    }
  }
}
