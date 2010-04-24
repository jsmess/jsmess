#ifndef FMTOWNS_H_
#define FMTOWNS_H_

#include "emu.h"

READ8_HANDLER( towns_gfx_high_r );
WRITE8_HANDLER( towns_gfx_high_w );
READ8_HANDLER( towns_gfx_r );
WRITE8_HANDLER( towns_gfx_w );
READ8_HANDLER( towns_video_cff80_r );
WRITE8_HANDLER( towns_video_cff80_w );
READ8_HANDLER( towns_video_cff80_mem_r );
WRITE8_HANDLER( towns_video_cff80_mem_w );
READ8_HANDLER(towns_video_440_r);
WRITE8_HANDLER(towns_video_440_w);
READ8_HANDLER(towns_video_5c8_r);
WRITE8_HANDLER(towns_video_5c8_w);
READ8_HANDLER(towns_video_fd90_r);
WRITE8_HANDLER(towns_video_fd90_w);
READ8_HANDLER(towns_video_ff81_r);
WRITE8_HANDLER(towns_video_ff81_w);
READ8_HANDLER(towns_spriteram_low_r);
WRITE8_HANDLER(towns_spriteram_low_w);
READ8_HANDLER( towns_spriteram_r);
WRITE8_HANDLER( towns_spriteram_w);


void towns_update_video_banks(const address_space*);

INTERRUPT_GEN( towns_vsync_irq );
VIDEO_START( towns );
VIDEO_UPDATE( towns );

#endif /*FMTOWNS_H_*/
