/*****************************************************************************
 *
 * includes/a7800.h
 *
 ****************************************************************************/

#ifndef A7800_H_
#define A7800_H_

#include "machine/6532riot.h"

class a7800_state : public driver_device
{
public:
	a7800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int lines;
	int ispal;
	unsigned char *cart_bkup;
	unsigned char *bios_bkup;
	int ctrl_lock;
	int ctrl_reg;
	int maria_flag;
	unsigned char *cartridge_rom;
	unsigned int cart_type;
	unsigned long cart_size;
	unsigned char stick_type;
	UINT8 *ROM;
	int maria_palette[8][4];
	int maria_write_mode;
	int maria_scanline;
	unsigned int maria_dll;
	unsigned int maria_dl;
	int maria_holey;
	int maria_offset;
	int maria_vblank;
	int maria_dli;
	int maria_dmaon;
	int maria_dodma;
	int maria_wsync;
	int maria_backcolor;
	int maria_color_kill;
	int maria_cwidth;
	int maria_bcntl;
	int maria_kangaroo;
	int maria_rm;
	unsigned int maria_charbase;
};


/*----------- defined in video/a7800.c -----------*/

VIDEO_START( a7800 );
VIDEO_UPDATE( a7800 );
INTERRUPT_GEN( a7800_interrupt );
 READ8_HANDLER( a7800_MARIA_r );
WRITE8_HANDLER( a7800_MARIA_w );


/*----------- defined in machine/a7800.c -----------*/

extern const riot6532_interface a7800_r6532_interface;


MACHINE_RESET( a7800 );

void a7800_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions);

DEVICE_START( a7800_cart );
DEVICE_IMAGE_LOAD( a7800_cart );

 READ8_HANDLER( a7800_TIA_r );
WRITE8_HANDLER( a7800_TIA_w );
WRITE8_HANDLER( a7800_RAM0_w );
WRITE8_HANDLER( a7800_cart_w );

DRIVER_INIT( a7800_ntsc );
DRIVER_INIT( a7800_pal );


#endif /* A7800_H_ */
