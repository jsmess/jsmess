#ifndef __OSBORNE1_H
#define __OSBORNE1_H

extern struct z80_irq_daisy_chain osborne1_daisy_chain[];

WRITE8_HANDLER( osborne1_0000_w );
WRITE8_HANDLER( osborne1_1000_w );
READ8_HANDLER( osborne1_2000_r );
WRITE8_HANDLER( osborne1_2000_w );
WRITE8_HANDLER( osborne1_3000_w );
WRITE8_HANDLER( osborne1_videoram_w );

WRITE8_HANDLER( osborne1_bankswitch_w );

DEVICE_LOAD( osborne1_floppy );

DRIVER_INIT( osborne1 );
MACHINE_RESET( osborne1 );

#endif
