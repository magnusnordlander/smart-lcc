//
// Created by Magnus Nordlander on 2023-01-07.
//

#ifndef SMART_LCC_USBDEBUG_H
#define SMART_LCC_USBDEBUG_H

#define USB_DEBUG 1

#if USB_DEBUG
#define INIT_USB_DEBUG() stdio_usb_init()
#define USB_PRINTF(...) printf (__VA_ARGS__)

#else
#define INIT_USB_DEBUG() do {} while (0)
#define USB_PRINTF(...) do {} while (0)

#endif

#define USB_PRINT_BUF(BUF, LEN) for (int __i = 0; __i < LEN; __i++) { USB_PRINTF("%02X", ((uint8_t*)BUF)[__i]); }

#endif //SMART_LCC_USBDEBUG_H
