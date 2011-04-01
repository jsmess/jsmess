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

	unsigned char m_vidmem[0x10000];
	int m_RAMSize;
	int m_DFSType;
	int m_SWRAMtype;
	int m_Master;
	int m_ACCCON_IRR;
	int m_rombank;
	int m_userport;
	int m_pagedRAM;
	int m_vdusel;
	int m_ACCCON;
	int m_ACCCON_TST;
	int m_ACCCON_IFJ;
	int m_ACCCON_ITU;
	int m_ACCCON_Y;
	int m_ACCCON_X;
	int m_ACCCON_E;
	int m_ACCCON_D;
	int m_b0_sound;
	int m_b1_speech_read;
	int m_b2_speech_write;
	int m_b3_keyboard;
	int m_b4_video0;
	int m_b5_video1;
	int m_b6_caps_lock_led;
	int m_b7_shift_lock_led;
	int m_MC146818_WR;
	int m_MC146818_DS;
	int m_MC146818_AS;
	int m_MC146818_CE;
	int m_via_system_porta;
	int m_column;
	double m_last_dev_val;
	int m_wav_len;
	int m_len0;
	int m_len1;
	int m_len2;
	int m_len3;
	int m_mc6850_clock;
	emu_timer *m_tape_timer;
	int m_previous_i8271_int_state;
	int m_drive_control;
	int m_wd177x_irq_state;
	int m_wd177x_drq_state;
	int m_previous_wd177x_int_state;
	int m_1770_IntEnabled;
	int m_opusbank;
	int m_video_refresh;
	unsigned char *m_BBC_Video_RAM;
	unsigned char *m_vidmem_RAM;
	UINT16 *m_BBC_display;
	UINT16 *m_BBC_display_left;
	UINT16 *m_BBC_display_right;
	bitmap_t *m_BBC_bitmap;
	int m_y_screen_pos;
	unsigned int m_video_ram_lookup0[0x4000];
	unsigned int m_video_ram_lookup1[0x4000];
	unsigned int m_video_ram_lookup2[0x4000];
	unsigned int m_video_ram_lookup3[0x4000];
	unsigned int *m_video_ram_lookup;
	unsigned char m_pixel_bits[256];
	int m_BBC_HSync;
	int m_BBC_VSync;
	int m_BBC_Character_Row;
	int m_BBC_DE;
	device_t *m_saa505x;
	int m_Teletext_Latch_Input_D7;
	int m_Teletext_Latch;
	int m_VideoULA_DE;
	int m_VideoULA_CR;
	int m_VideoULA_CR_counter;
	int m_videoULA_Reg;
	int m_videoULA_master_cursor_size;
	int m_videoULA_width_of_cursor;
	int m_videoULA_6845_clock_rate;
	int m_videoULA_characters_per_line;
	int m_videoULA_teletext_normal_select;
	int m_videoULA_flash_colour_select;
	int m_pixels_per_byte;
	int m_emulation_pixels_per_real_pixel;
	int m_emulation_pixels_per_byte;
	int m_emulation_cursor_size;
	int m_cursor_state;
	int m_videoULA_pallet0[16];
	int m_videoULA_pallet1[16];
	int *m_videoULA_pallet_lookup;
	int m_x_screen_offset;
	int m_y_screen_offset;
	void (*m_draw_function)(running_machine &machine);
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
SCREEN_UPDATE( bbc );

void bbc_draw_RGB_in(device_t *device, int offset, int data);
void bbc_set_video_memory_lookups(running_machine &machine, int ramsize);
void bbc_frameclock(running_machine &machine);
void bbc_setscreenstart(running_machine &machine, int b4, int b5);
void bbcbp_setvideoshadow(running_machine &machine, int vdusel);

WRITE8_HANDLER ( bbc_videoULA_w );

WRITE8_HANDLER ( bbc_6845_w );
READ8_HANDLER ( bbc_6845_r );


#endif /* BBC_H_ */
