#include "devices/snapquik.h"

/* machine/pmd85.c */
extern READ8_HANDLER ( primo_be_1_r );
extern READ8_HANDLER ( primo_be_2_r );
extern WRITE8_HANDLER ( primo_ki_1_w );
extern WRITE8_HANDLER ( primo_ki_2_w );
extern WRITE8_HANDLER ( primo_FD_w );
extern DRIVER_INIT ( primo32 );
extern DRIVER_INIT ( primo48 );
extern DRIVER_INIT ( primo64 );
extern MACHINE_RESET( primoa );
extern MACHINE_RESET( primob );
extern INTERRUPT_GEN( primo_vblank_interrupt );
extern SNAPSHOT_LOAD( primo );
extern QUICKLOAD_LOAD( primo );

/* vidhrdw/pmd85.c */
extern VIDEO_START( primo );
extern VIDEO_UPDATE( primo );
extern unsigned char primo_palette[2*3];
extern unsigned short primo_colortable[1][2];
extern PALETTE_INIT( primo );
extern UINT16 primo_video_memory_base;
