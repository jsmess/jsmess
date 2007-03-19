#ifndef __GB_H
#define __GB_H

#include "mame.h"
#include "sound/custom.h"

void gameboy_sound_w(int offset, int data);

#ifdef __MACHINE_GB_C
#define EXTERN
#else
#define EXTERN extern
#endif

/* Interrupts */
#define VBL_INT               0       /* V-Blank    */
#define LCD_INT               1       /* LCD Status */
#define TIM_INT               2       /* Timer      */
#define SIO_INT               3       /* Serial I/O */
#define EXT_INT               4       /* Joypad     */

#ifdef TIMER
#undef TIMER
#endif

/* Cartridge types */
#define RAM			0x01	/* Cartridge has RAM                             */
#define BATTERY		0x02	/* Cartridge has a battery to save RAM           */
#define TIMER		0x04	/* Cartridge has a real-time-clock (MBC3 only)   */
#define RUMBLE		0x08	/* Cartridge has a rumble motor (MBC5 only)      */
#define SRAM		0x10	/* Cartridge has SRAM                            */
#define UNKNOWN		0x80	/* Cartridge is of an unknown type               */

#define DMG_FRAMES_PER_SECOND	59.732155
#define SGB_FRAMES_PER_SECOND	61.17

WRITE8_HANDLER( gb_ram_enable );
WRITE8_HANDLER( gb_io_w );
READ8_HANDLER ( gb_io_r );
WRITE8_HANDLER( gb_io2_w );
READ8_HANDLER( gb_ie_r );
WRITE8_HANDLER( gb_ie_w );
DEVICE_INIT(gb_cart);
DEVICE_LOAD(gb_cart);
void gb_scanline_interrupt(void);
void gb_timer_callback(int cycles);
WRITE8_HANDLER( gbc_io2_w );
READ8_HANDLER( gbc_io2_r );
MACHINE_START( gb );
MACHINE_RESET( gb );
MACHINE_RESET( gbpocket );
WRITE8_HANDLER( gb_oam_w );
WRITE8_HANDLER( gb_vram_w );
WRITE8_HANDLER( gbc_vram_w );

/* from vidhrdw/gb.c */
extern UINT8 *gb_oam;
extern UINT8 *gb_vram;

READ8_HANDLER( gb_video_r );
WRITE8_HANDLER( gb_video_w );
int gb_video_oam_locked( void );
int gb_video_vram_locked( void );
VIDEO_START( gb );
VIDEO_UPDATE( gb );
void gb_video_init( void );
void sgb_video_init( void );
void gbc_video_init( void );

EXTERN double lcd_time;
/* Custom Sound Interface */
extern READ8_HANDLER( gb_sound_r );
extern WRITE8_HANDLER( gb_sound_w );
extern READ8_HANDLER( gb_wave_r );
extern WRITE8_HANDLER( gb_wave_w );
void *gameboy_sh_start(int clock, const struct CustomSound_interface *config);

/* -- Super GameBoy specific -- */
#define SGB_BORDER_PAL_OFFSET	64	/* Border colours stored from pal 4-7	*/
#define SGB_XOFFSET				48	/* GB screen starts at column 48		*/
#define SGB_YOFFSET				40	/* GB screen starts at row 40			*/

EXTERN UINT16 sgb_pal_data[4096];	/* 512 palettes of 4 colours			*/
EXTERN UINT8 sgb_pal_map[20][18];	/* Palette tile map						*/
extern UINT8 *sgb_tile_data;		/* 256 tiles of 32 bytes each			*/
EXTERN UINT8 sgb_tile_map[2048];	/* 32x32 tile map data (0-tile,1-attribute)	*/
EXTERN UINT8 sgb_window_mask;		/* Current GB screen mask				*/
EXTERN UINT8 sgb_hack;				/* Flag set if we're using a hack		*/

extern MACHINE_RESET( sgb );
extern WRITE8_HANDLER ( sgb_io_w );

/* -- GameBoy Color specific -- */
#define GBC_MODE_GBC		1		/* GBC is in colour mode				*/
#define GBC_MODE_MONO		2		/* GBC is in mono mode					*/
#define GBC_PAL_OBJ_OFFSET	32		/* Object palette offset				*/

extern UINT8 *GBC_VRAMMap[2];		/* Addressses of GBC video RAM banks	*/
EXTERN UINT8 gbc_mode;				/* is the GBC in mono/colour mode?		*/

MACHINE_RESET( gbc );
WRITE8_HANDLER ( gbc_video_w );


/* -- Megaduck specific -- */
extern DEVICE_LOAD(megaduck_cart);
extern MACHINE_RESET( megaduck );
extern  READ8_HANDLER( megaduck_video_r );
extern WRITE8_HANDLER( megaduck_video_w );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type1 );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type2 );
extern  READ8_HANDLER( megaduck_sound_r1 );
extern WRITE8_HANDLER( megaduck_sound_w1 );
extern  READ8_HANDLER( megaduck_sound_r2 );
extern WRITE8_HANDLER( megaduck_sound_w2 );

#endif
