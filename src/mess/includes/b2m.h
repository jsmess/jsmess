/*****************************************************************************
 *
 * includes/b2m.h
 *
 ****************************************************************************/

#ifndef b2m_H_
#define b2m_H_

#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"

/*----------- defined in machine/b2m.c -----------*/

extern UINT8 b2m_video_page;
extern UINT16 b2m_video_scroll;

extern const struct pit8253_config b2m_pit8253_intf;

extern const struct pic8259_interface b2m_pic8259_config;

extern const i8255a_interface b2m_ppi8255_interface_1;
extern const i8255a_interface b2m_ppi8255_interface_2;
extern const i8255a_interface b2m_ppi8255_interface_3;

extern DRIVER_INIT( b2m );
extern MACHINE_START( b2m );
extern MACHINE_RESET( b2m );
extern INTERRUPT_GEN( b2m_vblank_interrupt );
extern READ8_HANDLER( b2m_palette_r );
extern WRITE8_HANDLER( b2m_palette_w );
extern READ8_HANDLER( b2m_localmachine_r );
extern WRITE8_HANDLER( b2m_localmachine_w );
extern DEVICE_IMAGE_LOAD( b2m_floppy );

#define SOUND_B2M		DEVICE_GET_INFO_NAME( b2m_sound )

DEVICE_GET_INFO( b2m_sound );


/*----------- defined in video/b2m.c -----------*/

extern VIDEO_START( b2m );
extern VIDEO_UPDATE( b2m );
extern PALETTE_INIT( b2m );

#endif
