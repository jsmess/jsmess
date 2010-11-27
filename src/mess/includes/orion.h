/*****************************************************************************
 *
 * includes/orion.h
 *
 ****************************************************************************/

#ifndef ORION_H_
#define ORION_H_

#include "machine/i8255a.h"

class orion_state : public driver_device
{
public:
	orion_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 orion128_video_mode;
	UINT8 orion128_video_page;
	UINT8 orion128_video_width;
	UINT8 video_mode_mask;
	UINT8 orionpro_pseudo_color;
	UINT8 romdisk_lsb;
	UINT8 romdisk_msb;
	UINT8 orion128_memory_page;
	UINT8 orionz80_memory_page;
	UINT8 orionz80_dispatcher;
	UINT8 speaker;
	UINT8 orionpro_ram0_segment;
	UINT8 orionpro_ram1_segment;
	UINT8 orionpro_ram2_segment;
	UINT8 orionpro_page;
	UINT8 orionpro_128_page;
	UINT8 orionpro_rom2_segment;
	UINT8 orionpro_dispatcher;
};


/*----------- defined in machine/orion.c -----------*/

extern const i8255a_interface orion128_ppi8255_interface_1;

extern MACHINE_START( orion128 );
extern MACHINE_RESET( orion128 );

extern MACHINE_START( orionz80 );
extern MACHINE_RESET( orionz80 );
extern INTERRUPT_GEN( orionz80_interrupt );

extern READ8_HANDLER ( orion128_system_r );
extern WRITE8_HANDLER ( orion128_system_w );
extern READ8_HANDLER ( orion128_romdisk_r );
extern WRITE8_HANDLER ( orion128_romdisk_w );
extern WRITE8_HANDLER ( orion128_video_mode_w );
extern WRITE8_HANDLER ( orion128_memory_page_w );
extern WRITE8_HANDLER ( orion128_video_page_w );
extern WRITE8_HANDLER ( orionz80_memory_page_w );
extern WRITE8_HANDLER ( orionz80_dispatcher_w );
extern WRITE8_HANDLER ( orionz80_sound_w );
extern READ8_HANDLER ( orion128_floppy_r );
extern WRITE8_HANDLER ( orion128_floppy_w );
extern READ8_HANDLER ( orionz80_io_r );
extern WRITE8_HANDLER ( orionz80_io_w );

extern MACHINE_RESET( orionpro );
extern READ8_HANDLER ( orionpro_io_r );
extern WRITE8_HANDLER ( orionpro_io_w );

/*----------- defined in video/orion.c -----------*/

extern VIDEO_START( orion128 );
extern VIDEO_UPDATE( orion128 );
extern PALETTE_INIT( orion128 );

#endif /* ORION_H_ */

