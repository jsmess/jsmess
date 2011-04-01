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
#include "imagedev/snapquik.h"
#include "machine/upd765.h"

#define KC85_4_CLOCK 1750000
#define KC85_3_CLOCK 1750000

#define KC85_4_SCREEN_PIXEL_RAM_SIZE 0x04000
#define KC85_4_SCREEN_COLOUR_RAM_SIZE 0x04000

#define KC85_PALETTE_SIZE 24
#define KC85_SCREEN_WIDTH 320
#define KC85_SCREEN_HEIGHT 256

/* number of keycodes that can be stored in queue */
#define KC_KEYCODE_QUEUE_LENGTH 256

#define KC_KEYBOARD_NUM_LINES	9

typedef struct kc_keyboard
{
	/* list of stored keys */
	unsigned char keycodes[KC_KEYCODE_QUEUE_LENGTH];
	/* index of start of list */
	int head;
	/* index of end of list */
	int tail;

	/* transmitting state */
	int transmit_state;

	/* number of pulses remaining to be transmitted */
	int	transmit_pulse_count_remaining;
	/* count of pulses transmitted so far */
	int transmit_pulse_count;

	/* pulses to transmit */
	unsigned char transmit_buffer[32];
} kc_keyboard;


class kc_state : public driver_device
{
public:
	kc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_kc85_pio_data[2];
	device_t *m_kc85_z80pio;
	unsigned char m_kc85_disc_hw_input_gate;
	unsigned char *m_kc85_module_rom;
	emu_timer *m_cassette_timer;
	int m_cassette_motor_state;
	unsigned char m_ardy;
	int m_previous_keyboard[KC_KEYBOARD_NUM_LINES-1];
	unsigned char m_brdy;
	kc_keyboard m_keyboard_data;
	int m_kc85_84_data;
	int m_kc85_86_data;
	int m_kc85_50hz_state;
	int m_kc85_15khz_state;
	int m_kc85_15khz_count;
	int m_kc85_blink_state;
	unsigned char *m_kc85_4_display_video_ram;
	unsigned char *m_kc85_4_video_ram;
	const struct kc85_module *m_modules[256>>2];
};


/*----------- defined in video/kc.c -----------*/

extern PALETTE_INIT( kc85 );

void kc85_video_set_blink_state(running_machine &machine, int data);

VIDEO_START( kc85_3 );
VIDEO_START( kc85_4 );
SCREEN_UPDATE( kc85_3 );
SCREEN_UPDATE( kc85_4 );

/* select video ram to display */
void kc85_4_video_ram_select_bank(running_machine &machine, int bank);
/* select video ram which is visible in address space */
unsigned char *kc85_4_get_video_ram_base(running_machine &machine, int bank, int colour);


/*----------- defined in machine/kc.c -----------*/

extern const upd765_interface kc_fdc_interface;
QUICKLOAD_LOAD( kc );

MACHINE_START( kc85 );
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
/* output port to set UPD765 terminal count input */
WRITE8_HANDLER(kc85_disc_hw_terminal_count_w);

/* these are used by the kc85 to control the disc interface */
/* xxf4 - latch used to reset cpu in disc interface */
WRITE8_HANDLER(kc85_disc_interface_latch_w);
/* xxf0-xxf3 write to kc85 disc interface ram */
WRITE8_HANDLER(kc85_disc_interface_ram_w);
/* xxf0-xxf3 read from kc85 disc interface ram */
READ8_HANDLER(kc85_disc_interface_ram_r);


#endif /* KC_H_ */
