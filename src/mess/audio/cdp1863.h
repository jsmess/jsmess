/*

              CDP1863 CMOS 8-Bit Programmable Frequency Generator

                                ________________
                _RESET   1  ---|       \/       |---  16  Vdd
                 CLK 2   2  ---|                |---  15  OE
                 CLK 1   3  ---|                |---  14  OUT
                   STR   4  ---|    CDP1863C    |---  13  DI7
                   DI0   5  ---|                |---  12  DI6
                   DI1   6  ---|                |---  11  DI5
                   DI2   7  ---|                |---  10  DI4
                   Vss   8  ---|________________|---  9   DI3

*/

#ifndef __CDP1863__
#define __CDP1863__

#include "sound/beep.h"

#define CDP1863		DEVICE_GET_INFO_NAME(cdp1863)

/* interface */
typedef struct _cdp1863_interface cdp1863_interface;
struct _cdp1863_interface
{
	int clock1;					/* the clock 1 (pin 3) of the chip */
	int clock2;					/* the clock 2 (pin 2) of the chip */
};
#define CDP1863_INTERFACE(name) const cdp1863_interface (name)=

/* device interface */
DEVICE_GET_INFO( cdp1863 );

/* load tone latch */
WRITE8_DEVICE_HANDLER( cdp1863_str_w );

/* output enable */
void cdp1863_oe_w(running_device *device, int level);

#endif
