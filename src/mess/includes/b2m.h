/*****************************************************************************
 *
 * includes/b2m.h
 *
 ****************************************************************************/

#ifndef b2m_H_
#define b2m_H_

#include "mame.h"
#include "sound/custom.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"

/*----------- defined in machine/b2m.c -----------*/

extern UINT8 b2m_video_page;
extern UINT16 b2m_video_scroll;

extern const struct pit8253_config b2m_pit8253_intf;

extern const struct pic8259_interface b2m_pic8259_config;

extern const ppi8255_interface b2m_ppi8255_interface_1;
extern const ppi8255_interface b2m_ppi8255_interface_2;
extern const ppi8255_interface b2m_ppi8255_interface_3;

extern const custom_sound_interface b2m_sound_interface;
extern void b2m_sh_change_clock(double);

extern DRIVER_INIT( b2m );
extern MACHINE_START( b2m );
extern MACHINE_RESET( b2m );
extern INTERRUPT_GEN( b2m_vblank_interrupt );
extern READ8_HANDLER( b2m_8255_0_r );
extern WRITE8_HANDLER( b2m_8255_0_w );
extern READ8_HANDLER( b2m_8255_1_r );
extern WRITE8_HANDLER( b2m_8255_1_w );
extern READ8_HANDLER( b2m_8255_2_r );
extern WRITE8_HANDLER( b2m_8255_2_w );
extern READ8_HANDLER( b2m_palette_r );
extern WRITE8_HANDLER( b2m_palette_w );
extern READ8_HANDLER( b2m_localmachine_r );
extern WRITE8_HANDLER( b2m_localmachine_w );
extern DEVICE_IMAGE_LOAD( b2m_floppy );


/*----------- defined in video/b2m.c -----------*/

extern VIDEO_START( b2m );
extern VIDEO_UPDATE( b2m );
extern PALETTE_INIT( b2m );

#endif
