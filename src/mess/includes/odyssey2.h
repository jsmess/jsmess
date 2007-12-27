#include "sound/custom.h"
#include "streams.h"

#define P1_BANK_LO_BIT            (0x01)
#define P1_BANK_HI_BIT            (0x02)
#define P1_KEYBOARD_SCAN_ENABLE   (0x04)  /* active low */
#define P1_VDC_ENABLE             (0x08)  /* active low */
#define P1_EXT_RAM_ENABLE         (0x10)  /* active low */
#define P1_VDC_COPY_MODE_ENABLE   (0x40)
                                  
#define P2_KEYBOARD_SELECT_MASK   (0x07)  /* select row to scan */

#define VDC_CONTROL_REG_STROBE_XY (0x02)

#define COLLISION_SPRITE_0        (0x01)
#define COLLISION_SPRITE_1        (0x02)
#define COLLISION_SPRITE_2        (0x04)
#define COLLISION_SPRITE_3        (0x08)
#define COLLISION_VERTICAL_GRID   (0x10)
#define COLLISION_HORIZ_GRID_DOTS (0x20)
#define COLLISION_EXTERNAL_UNUSED (0x40)
#define COLLISION_CHARACTERS      (0x80)

#define COLLISION_SPRITE_0_IND        (1)
#define COLLISION_SPRITE_1_IND        (2)
#define COLLISION_SPRITE_2_IND        (3)
#define COLLISION_SPRITE_3_IND        (4)
#define COLLISION_VERTICAL_GRID_IND   (5)
#define COLLISION_HORIZ_GRID_DOTS_IND (6)
#define COLLISION_EXTERNAL_UNUSED_IND (7)
#define COLLISION_CHARACTERS_IND      (8)

/*----------- defined in video/odyssey2.c -----------*/

extern int odyssey2_vh_hpos;

extern const UINT8 odyssey2_colors[];

VIDEO_START( odyssey2 );
VIDEO_UPDATE( odyssey2 );
PALETTE_INIT( odyssey2 );
READ8_HANDLER ( odyssey2_t1_r );
INTERRUPT_GEN( odyssey2_line );
READ8_HANDLER ( odyssey2_video_r );
WRITE8_HANDLER ( odyssey2_video_w );

void odyssey2_sh_update( void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length );

/*----------- defined in audio/odyssey2.c -----------*/

extern sound_stream *odyssey2_sh_channel;
extern const struct CustomSound_interface odyssey2_sound_interface;
void *odyssey2_sh_start(int clock, const struct CustomSound_interface *config);

/*----------- defined in machine/odyssey2.c -----------*/

DRIVER_INIT( odyssey2 );
MACHINE_RESET( odyssey2 );

/* i/o ports */
READ8_HANDLER ( odyssey2_bus_r );
WRITE8_HANDLER ( odyssey2_bus_w );

READ8_HANDLER( odyssey2_getp1 );
WRITE8_HANDLER ( odyssey2_putp1 );

READ8_HANDLER( odyssey2_getp2 );
WRITE8_HANDLER ( odyssey2_putp2 );

READ8_HANDLER( odyssey2_getbus );
WRITE8_HANDLER ( odyssey2_putbus );

int odyssey2_cart_verify(const UINT8 *cartdata, size_t size);
