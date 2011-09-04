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
#include "machine/kcexp.h"
#include "machine/kc_ram.h"
#include "machine/kc_rom.h"
#include "machine/kc_d002.h"
#include "machine/kc_d004.h"

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
	kc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	int m_kc85_pio_data[2];
	device_t *m_kc85_z80pio;
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

	kcexp_slot_device *m_expansions[3];
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

QUICKLOAD_LOAD( kc );

MACHINE_START( kc85 );
MACHINE_RESET( kc85_3 );
MACHINE_RESET( kc85_4 );

/* cassette */
READ8_HANDLER(kc85_4_84_r);
WRITE8_HANDLER(kc85_4_84_w);

READ8_HANDLER(kc85_4_86_r);
WRITE8_HANDLER(kc85_4_86_w);

extern const z80pio_interface kc85_2_pio_intf;
extern const z80pio_interface kc85_4_pio_intf;
extern const z80ctc_interface kc85_ctc_intf;


/*** MODULE SYSTEM ***/

READ8_HANDLER ( kc_expansion_io_read );
WRITE8_HANDLER ( kc_expansion_io_write );

#endif /* KC_H_ */
