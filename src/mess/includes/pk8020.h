/*****************************************************************************
 *
 * includes/pk8020.h
 *
 ****************************************************************************/

#ifndef PK8020_H_
#define PK8020_H_

#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "sound/wave.h"

/*----------- defined in machine/pk8020.c -----------*/
extern UINT8 pk8020_color;
extern UINT8 pk8020_video_page;
extern UINT8 pk8020_wide;
extern UINT8 pk8020_font;
extern MACHINE_RESET( pk8020 );
extern const i8255a_interface pk8020_ppi8255_interface_1;
extern const i8255a_interface pk8020_ppi8255_interface_2;
extern const i8255a_interface pk8020_ppi8255_interface_3;
extern const struct pit8253_config pk8020_pit8253_intf;
extern const struct pic8259_interface pk8020_pic8259_config;
extern INTERRUPT_GEN( pk8020_interrupt );
/*----------- defined in video/pk8020.c -----------*/

extern PALETTE_INIT( pk8020 );
extern VIDEO_START( pk8020 );
extern VIDEO_UPDATE( pk8020 );

#endif /* pk8020_H_ */
