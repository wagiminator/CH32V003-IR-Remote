// ===================================================================================
// User Configurations
// ===================================================================================

#pragma once

// Assign IR commands to the keys (multiple commands must be separated by semicolons!)
#define KEY1  NEC_sendCode(0x04,0x08)     // LG TV Power: addr 0x04, cmd 0x08
#define KEY2  RC5_sendCode(0x00,0x0B)     // Philips TV Power: addr 0x00, cmd 0x0B
#define KEY3  SON_sendCode(0x01,0x15,12)  // Sony TV Power: addr 0x01, cmd 0x15, 12-bit version
#define KEY4  SAM_sendCode(0x07,0x02)     // Samsung TV Power: addr: 07, cmd: 02
#define KEY5  DLY_ms(10)                  // nothing

// Pin definitions for keys (pin numbers must be different, regardless of the port!)
#define PIN_KEY1    PC2                   // define pin to KEY1 (active low)
#define PIN_KEY2    PC4                   // define pin to KEY2 (active low)
#define PIN_KEY3    PD5                   // define pin to KEY3 (active low)
#define PIN_KEY4    PD6                   // define pin to KEY4 (active low)
#define PIN_KEY5    PC1                   // define pin to KEY5 (active low)

// Pin definition for IR-LED (active low, do not change until you reconfigure timer!)
#define PIN_LED     PA2