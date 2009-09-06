/*****************************************************************************
 *
 * includes/kc.h
 *
 ****************************************************************************/

#ifndef KC_H_
#define KC_H_

#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "devices/snapquik.h"
#include "machine/nec765.h"

#define KC85_4_CLOCK 1750000
#define KC85_3_CLOCK 1750000

#define KC85_4_SCREEN_PIXEL_RAM_SIZE 0x04000
#define KC85_4_SCREEN_COLOUR_RAM_SIZE 0x04000

#define KC85_PALETTE_SIZE 24
#define KC85_SCREEN_WIDTH 320
#define KC85_SCREEN_HEIGHT 256


/*----------- defined in video/kc.c -----------*/

extern PALETTE_INIT( kc85 );

void kc85_video_set_blink_state(running_machine *machine, int data);

VIDEO_START( kc85_3 );
VIDEO_START( kc85_4 );
VIDEO_UPDATE( kc85_3 );
VIDEO_UPDATE( kc85_4 );

/* select video ram to display */
void kc85_4_video_ram_select_bank(int bank);
/* select video ram which is visible in address space */
unsigned char *kc85_4_get_video_ram_base(int bank, int colour);


/*----------- defined in machine/kc.c -----------*/

extern const nec765_interface kc_fdc_interface;
extern QUICKLOAD_LOAD( kc );

MACHINE_RESET( kc85_3 );
MACHINE_RESET( kc85_4 );
MACHINE_RESET( kc85_4d );

/* cassette */
READ8_HANDLER(kc85_4_84_r);
WRITE8_HANDLER(kc85_4_84_w);

READ8_HANDLER(kc85_4_86_r);
WRITE8_HANDLER(kc85_4_86_w);

READ8_HANDLER(kc85_unmapped_r);

READ8_HANDLER(kc85_pio_data_r);

WRITE8_HANDLER(kc85_4_pio_data_w);
WRITE8_HANDLER(kc85_3_pio_data_w);

READ8_HANDLER(kc85_pio_control_r);
WRITE8_HANDLER(kc85_pio_control_w);

READ8_DEVICE_HANDLER(kc85_ctc_r);
WRITE8_DEVICE_HANDLER(kc85_ctc_w);

extern const z80pio_interface kc85_pio_intf;
extern const z80ctc_interface kc85_ctc_intf;


/*** MODULE SYSTEM ***/
/* read from xx80 port */
READ8_HANDLER(kc85_module_r);
/* write to xx80 port */
WRITE8_HANDLER(kc85_module_w);


/*** DISC INTERFACE **/

/* IO_FLOPPY device */

/* used to setup machine */

#define KC_DISC_INTERFACE_PORT_R \
	{0x0f0, 0x0f3, kc85_disc_interface_ram_r},

#define KC_DISC_INTERFACE_PORT_W \
	{0x0f0, 0x0f3, kc85_disc_interface_ram_w}, \
	{0x0f4, 0x0f4, kc85_disc_interface_latch_w},

#define KC_DISC_INTERFACE_ROM



/* these are internal to the disc interface */

/* disc hardware internal i/o */
READ8_DEVICE_HANDLER(kc85_disk_hw_ctc_r);
/* disc hardware internal i/o */
WRITE8_DEVICE_HANDLER(kc85_disk_hw_ctc_w);
/* 4-bit input latch: DMA Data Request, FDC Int, FDD Ready.. */
READ8_HANDLER(kc85_disc_hw_input_gate_r);
/* output port to set NEC765 terminal count input */
WRITE8_HANDLER(kc85_disc_hw_terminal_count_w);

/* these are used by the kc85 to control the disc interface */
/* xxf4 - latch used to reset cpu in disc interface */
WRITE8_HANDLER(kc85_disc_interface_latch_w);
/* xxf0-xxf3 write to kc85 disc interface ram */
WRITE8_HANDLER(kc85_disc_interface_ram_w);
/* xxf0-xxf3 read from kc85 disc interface ram */
READ8_HANDLER(kc85_disc_interface_ram_r);


#endif /* KC_H_ */
