// ===================================================================================
// CMSIS-DAP CONFIG I/O
// ===================================================================================

#pragma once
#include "ch554.h"
#include "gpio.h"
#include "config.h"

// SWD I/O pin manipulations
#define SWK_SET(val)          PIN_write(PIN_SWK, val)
#define SWD_SET(val)          PIN_write(PIN_SWD, val)
#define RST_SET(val)          PIN_write(PIN_RST, val)
#define LED_SET(val)          PIN_write(PIN_LED, !(val))
#define SWK_GET()             PIN_read(PIN_SWK)
#define SWD_GET()             PIN_read(PIN_SWD)
#define RST_GET()             PIN_read(PIN_RST)

// Initial SWD port setup (at firmware start)
#define PORT_SWD_SETUP()      // nothing to be done (reset values already fit)

// Connect SWD port (setup port for data transmission)
#define PORT_SWD_CONNECT() {  \
  LED_SET(1);                 \
  SWK_SET(0);                 \
}

// Disconnect SWD port (idle state, input/pullup)
#define PORT_SWD_DISCONNECT() { \
  RST_SET(1);                 \
  SWD_SET(1);                 \
  SWK_SET(1);                 \
  LED_SET(0);                 \
}

// SWCLK clockout cycle
#define SWD_CLOCK_CYCLE() {   \
  SWK_SET(1);                 \
  SWK_SET(0);                 \
}

// Write one bit via SWD
#define SWD_WRITE_BIT(bits) { \
  SWD_SET((bits)&1);          \
  SWD_CLOCK_CYCLE();          \
}

// Read one bit via SWD
#define SWD_READ_BIT(bits) {  \
  bits = SWD_GET();           \
  SWD_CLOCK_CYCLE();          \
}

// Set SWDIO pin to output
#define SWD_OUT_ENABLE()      // nothing to be done (open-drain pullup)

// Set SWDIO pin to input
#define SWD_OUT_DISABLE()     // nothing to be done (open-drain pullup)

// HID transfer buffers
#define DAP_READ_BUF_PTR      EP1_buffer
#define DAP_WRITE_BUF_PTR     EP1_buffer + 64
extern __xdata uint8_t EP1_buffer[];

// DAP init function
#define DAP_init()            {USB_init(); PORT_SWD_SETUP();}
extern void USB_init(void);
