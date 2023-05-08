// ===================================================================================
// USB Handler for CH551, CH552 and CH554
// ===================================================================================

#pragma once
#include <stdint.h>
#include "usb_descr.h"

// ===================================================================================
// Variables
// ===================================================================================
#define USB_setupBuf ((PUSB_SETUP_REQ)EP0_buffer)
extern uint8_t SetupReq;

// ===================================================================================
// Custom External USB Handler Functions
// ===================================================================================
void HID_setup(void);
void HID_reset(void);
void HID_EP1_IN(void);
void HID_EP1_OUT(void);

// ===================================================================================
// USB Handler Defines
// ===================================================================================
// Custom USB handler functions
#define USB_INIT_handler    HID_setup         // init custom endpoints
#define USB_RESET_handler   HID_reset         // custom USB reset handler

// Endpoint callback functions
#define EP0_SETUP_callback  USB_EP0_SETUP
#define EP0_IN_callback     USB_EP0_IN
#define EP0_OUT_callback    USB_EP0_OUT
#define EP1_IN_callback     HID_EP1_IN
#define EP1_OUT_callback    HID_EP1_OUT

// ===================================================================================
// Functions
// ===================================================================================
void USB_interrupt(void);
void USB_init(void);
