/*****************************************************************************
 *
 * includes/bbc.h
 *
 * BBC Model B
 *
 * Driver by Gordon Jefferyes <mess_bbc@gjeffery.dircon.co.uk>
 *
 ****************************************************************************/

#ifndef BBC_H_
#define BBC_H_

#include "machine/6522via.h"
#include "machine/6850acia.h"
#include "machine/i8271.h"
#include "machine/wd17xx.h"
#include "machine/upd7002.h"


class bbc_state : public driver_device
{
public:
	bbc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	unsigned char vidmem[0x10000];
	int RAMSize;
	int DFSType;
	int SWRAMtype;
	int Master;
	int ACCCON_IRR;
	int rombank;
	int userport;
	int pagedRAM;
	int vdusel;
	int ACCCON;
	int ACCCON_TST;
	int ACCCON_IFJ;
	int ACCCON_ITU;
	int ACCCON_Y;
	int ACCCON_X;
	int ACCCON_E;
	int ACCCON_D;
	int b0_sound;
	int b1_speech_read;
	int b2_speech_write;
	int b3_keyboard;
	int b4_video0;
	int b5_video1;
	int b6_caps_lock_led;
	int b7_shift_lock_led;
	int MC146818_WR;
	int MC146818_DS;
	int MC146818_AS;
	int MC146818_CE;
	int via_system_porta;
	int column;
	double last_dev_val;
	int wav_len;
	int len0;
	int len1;
	int len2;
	int len3;
	int mc6850_clock;
	emu_timer *tape_timer;
	int previous_i8271_int_state;
	int drive_control;
	int wd177x_irq_state;
	int wd177x_drq_state;
	int previous_wd177x_int_state;
	int _1770_IntEnabled;
	int opusbank;
	int video_refresh;
	unsigned char *BBC_Video_RAM;
	unsigned char *vidmem_RAM;
	UINT16 *BBC_display;
	UINT16 *BBC_display_left;
	UINT16 *BBC_display_right;
	bitmap_t *BBC_bitmap;
	int y_screen_pos;
	unsigned int video_ram_lookup0[0x4000];
	unsigned int video_ram_lookup1[0x4000];
	unsigned int video_ram_lookup2[0x4000];
	unsigned int video_ram_lookup3[0x4000];
	unsigned int *video_ram_lookup;
	unsigned char pixel_bits[256];
	int BBC_HSync;
	int BBC_VSync;
	int BBC_Character_Row;
	int BBC_DE;
	int Teletext_Latch_Input_D7;
	int Teletext_Latch;
	int VideoULA_DE;
	int VideoULA_CR;
	int VideoULA_CR_counter;
	int videoULA_Reg;
	int videoULA_master_cursor_size;
	int videoULA_width_of_cursor;
	int videoULA_6845_clock_rate;
	int videoULA_characters_per_line;
	int videoULA_teletext_normal_select;
	int videoULA_flash_colour_select;
	int pixels_per_byte;
	int emulation_pixels_per_real_pixel;
	int emulation_pixels_per_byte;
	int emulation_cursor_size;
	int cursor_state;
	int videoULA_pallet0[16];
	int videoULA_pallet1[16];
	int *videoULA_pallet_lookup;
	int x_screen_offset;
	int y_screen_offset;
};


/*----------- defined in machine/bbc.c -----------*/

extern const via6522_interface bbcb_system_via;
extern const via6522_interface bbcb_user_via;
extern const wd17xx_interface bbc_wd17xx_interface;
DRIVER_INIT( bbc );
DRIVER_INIT( bbcm );

MACHINE_START( bbca );
MACHINE_START( bbcb );
MACHINE_START( bbcbp );
MACHINE_START( bbcm );

MACHINE_RESET( bbca );
MACHINE_RESET( bbcb );
MACHINE_RESET( bbcbp );
MACHINE_RESET( bbcm );

INTERRUPT_GEN( bbcb_keyscan );
INTERRUPT_GEN( bbcm_keyscan );

WRITE8_HANDLER ( bbc_memorya1_w );
WRITE8_HANDLER ( bbc_page_selecta_w );

WRITE8_HANDLER ( bbc_memoryb3_w );
WRITE8_HANDLER ( bbc_memoryb4_w );
WRITE8_HANDLER ( bbc_page_selectb_w );


WRITE8_HANDLER ( bbc_memorybp1_w );
//READ8_HANDLER  ( bbc_memorybp2_r );
WRITE8_HANDLER ( bbc_memorybp2_w );
WRITE8_HANDLER ( bbc_memorybp4_w );
WRITE8_HANDLER ( bbc_memorybp4_128_w );
WRITE8_HANDLER ( bbc_memorybp6_128_w );
WRITE8_HANDLER ( bbc_page_selectbp_w );


WRITE8_HANDLER ( bbc_memorybm1_w );
//READ8_HANDLER  ( bbc_memorybm2_r );
WRITE8_HANDLER ( bbc_memorybm2_w );
WRITE8_HANDLER ( bbc_memorybm4_w );
WRITE8_HANDLER ( bbc_memorybm5_w );
WRITE8_HANDLER ( bbc_memorybm7_w );
READ8_HANDLER  ( bbcm_r );
WRITE8_HANDLER ( bbcm_w );
READ8_HANDLER  ( bbcm_ACCCON_read );
WRITE8_HANDLER ( bbcm_ACCCON_write );


//WRITE8_HANDLER ( bbc_bank4_w );

/* disc support */

DEVICE_IMAGE_LOAD ( bbcb_cart );

READ8_HANDLER  ( bbc_disc_r );
WRITE8_HANDLER ( bbc_disc_w );

READ8_HANDLER  ( bbc_wd1770_read );
WRITE8_HANDLER ( bbc_wd1770_write );

READ8_HANDLER  ( bbc_opus_read );
WRITE8_HANDLER ( bbc_opus_write );


READ8_HANDLER  ( bbcm_wd1770l_read );
WRITE8_HANDLER ( bbcm_wd1770l_write );
READ8_HANDLER  ( bbcm_wd1770_read );
WRITE8_HANDLER ( bbcm_wd1770_write );


/* tape support */

WRITE8_HANDLER ( bbc_SerialULA_w );

extern const i8271_interface bbc_i8271_interface;
extern const uPD7002_interface bbc_uPD7002;

/*----------- defined in video/bbc.c -----------*/

VIDEO_START( bbca );
VIDEO_START( bbcb );
VIDEO_START( bbcbp );
VIDEO_START( bbcm );
VIDEO_UPDATE( bbc );

void bbc_set_video_memory_lookups(running_machine *machine, int ramsize);
void bbc_frameclock(running_machine *machine);
void bbc_setscreenstart(running_machine *machine, int b4, int b5);
void bbcbp_setvideoshadow(running_machine *machine, int vdusel);

WRITE8_HANDLER ( bbc_videoULA_w );

WRITE8_HANDLER ( bbc_6845_w );
READ8_HANDLER ( bbc_6845_r );


#endif /* BBC_H_ */
