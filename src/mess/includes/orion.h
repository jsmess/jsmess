/*****************************************************************************
 *
 * includes/orion.h
 *
 ****************************************************************************/

#ifndef ORION_H_
#define ORION_H_

#include "includes/radio86.h"
#include "machine/i8255a.h"

class orion_state : public radio86_state
{
public:
	orion_state(running_machine &machine, const driver_device_config_base &config)
		: radio86_state(machine, config) { }

	UINT8 m_orion128_video_mode;
	UINT8 m_orion128_video_page;
	UINT8 m_orion128_video_width;
	UINT8 m_video_mode_mask;
	UINT8 m_orionpro_pseudo_color;
	UINT8 m_romdisk_lsb;
	UINT8 m_romdisk_msb;
	UINT8 m_orion128_memory_page;
	UINT8 m_orionz80_memory_page;
	UINT8 m_orionz80_dispatcher;
	UINT8 m_speaker;
	UINT8 m_orionpro_ram0_segment;
	UINT8 m_orionpro_ram1_segment;
	UINT8 m_orionpro_ram2_segment;
	UINT8 m_orionpro_page;
	UINT8 m_orionpro_128_page;
	UINT8 m_orionpro_rom2_segment;
	UINT8 m_orionpro_dispatcher;
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
extern SCREEN_UPDATE( orion128 );
extern PALETTE_INIT( orion128 );

#endif /* ORION_H_ */

