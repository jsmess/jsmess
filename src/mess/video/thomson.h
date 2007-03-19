/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

/* 
   TO7 video:
   one line (64 us) =
      56 left border pixels ( 7 us)
   + 320 active pixels (40 us)
   +  56 right border pixels ( 7 us)
   +     horizontal retrace (10 us)

   one image (20 ms) =
      47 top border lines (~3 ms)
   + 200 active lines (12.8 ms)
   +  47 bottom border lines (~3 ms)
   +     vertical retrace (~1 ms)

   TO9 and up introduced a half (160 pixels) and double (640 pixels)
   horizontal mode, but still in 40 us (no change in refresh rate).
*/




/***************************** dimensions **************************/

/* original screen dimension (may be different from emulated screen!) */
#define THOM_ACTIVE_WIDTH  320
#define THOM_BORDER_WIDTH   56
#define THOM_ACTIVE_HEIGHT 200
#define THOM_BORDER_HEIGHT  47
#define THOM_TOTAL_WIDTH   432
#define THOM_TOTAL_HEIGHT  294

/* Emulated screen dimension may be doubled to allow hi-res 640x200 mode.
   Emulated screen can have smaller borders.
 */

/* maximum number of video pages:
   1 for TO7 generation (including MO5)
   4 for TO8 generation (including TO9, MO6)
 */
#define THOM_NB_PAGES 4

/* page 0 is banked */
#define THOM_VRAM_BANK 1

extern UINT8* thom_vram;

/*********************** video signals *****************************/

struct thom_vsignal {
  unsigned count;  /* pixel counter */
  unsigned init;   /* 1 -> active vertical windos, 0 -> border/VBLANK */
  unsigned inil;   /* 1 -> active horizontal window, 0 -> border/HBLANK */
  unsigned lt3;    /* bit 3 of us counter */
  unsigned line;   /* line counter */
};

/* current video position */
extern struct thom_vsignal thom_get_vsignal ( void );


/************************* lightpen ********************************/

/* specific TO7 / T9000 lightpen code (no video gate-array) */
extern unsigned to7_lightpen_gpl ( int decx, int decy );

/* video position corresponding to lightpen (with some offset) */
extern struct thom_vsignal thom_get_lightpen_vsignal ( int xdec, int ydec, 
						       int xdec2 );

/* specify a lightpencall-back function, called nb times per frame */
extern void thom_set_lightpen_callback ( int nb, void (*cb) ( int step ) );


/***************************** commons *****************************/

extern VIDEO_START  ( thom );
extern VIDEO_UPDATE ( thom );
extern PALETTE_INIT ( thom );
extern VIDEO_EOF    ( thom );

extern void thom_video_postload ( void );

/* pass video init signal */
extern void thom_set_init_callback ( void (*cb) ( int init ) );

/* TO7 TO7/70 MO5 video bank switch */
extern void thom_set_mode_point ( int point );

/* set the palette index for the border color */
extern void thom_set_border_color ( unsigned color );

/* set one of 16 palette indices to one of 4096 colors */
extern void thom_set_palette ( unsigned index, UINT16 color );


/* video modes */
#define THOM_VMODE_TO770       0
#define THOM_VMODE_MO5         1
#define THOM_VMODE_BITMAP4     2
#define THOM_VMODE_BITMAP4_ALT 3
#define THOM_VMODE_80          4
#define THOM_VMODE_BITMAP16    5
#define THOM_VMODE_PAGE1       6
#define THOM_VMODE_PAGE2       7
#define THOM_VMODE_OVERLAY     8
#define THOM_VMODE_OVERLAY3    9
#define THOM_VMODE_TO9        10
#define THOM_VMODE_80_TO9     11
#define THOM_VMODE_NB         12

/* change the current video-mode */
extern void thom_set_video_mode ( unsigned mode );

/* select which video page shown among the 4 available */
extern void thom_set_video_page ( unsigned page );

/* to tell there is some floppy activity, stays up for a few frames */
extern void thom_floppy_active ( int write );


/***************************** TO7 / T9000 *************************/

extern WRITE8_HANDLER ( to7_vram_w );

/* specific TO7 / T9000 lightpen code (no video gate-array) */
extern unsigned to7_lightpen_gpl ( int decx, int decy );


/***************************** TO7/70 ******************************/

extern WRITE8_HANDLER ( to770_vram_w );


/***************************** TO8 ******************************/

/* write to video memory through system space (always page 1) */
WRITE8_HANDLER ( to8_sys_lo_w );
WRITE8_HANDLER ( to8_sys_hi_w );

/* write to video memory through data space */
WRITE8_HANDLER ( to8_data_lo_w );
WRITE8_HANDLER ( to8_data_hi_w );

/* write to video memory page through cartridge addresses space */
WRITE8_HANDLER ( to8_vcart_w );
