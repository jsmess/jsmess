/*****************************************************************************
 *
 * includes/gb.h
 *
 ****************************************************************************/

#ifndef GB_H_
#define GB_H_



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


/*----------- defined in audio/gb.c -----------*/

#define SOUND_GAMEBOY	DEVICE_GET_INFO_NAME(gameboy_sound)

/* Custom Sound Interface */
DEVICE_GET_INFO( gameboy_sound );
READ8_DEVICE_HANDLER( gb_sound_r );
WRITE8_DEVICE_HANDLER( gb_sound_w );
READ8_DEVICE_HANDLER( gb_wave_r );
WRITE8_DEVICE_HANDLER( gb_wave_w );


/*----------- defined in machine/gb.c -----------*/

WRITE8_HANDLER( gb_ram_enable );
WRITE8_HANDLER( gb_io_w );
READ8_HANDLER ( gb_io_r );
WRITE8_HANDLER( gb_io2_w );
READ8_HANDLER( gb_ie_r );
WRITE8_HANDLER( gb_ie_w );
DEVICE_START(gb_cart);
DEVICE_IMAGE_LOAD(gb_cart);
INTERRUPT_GEN( gb_scanline_interrupt );
void gb_timer_callback(const device_config *device, int cycles);
WRITE8_HANDLER( gbc_io2_w );
READ8_HANDLER( gbc_io2_r );
MACHINE_START( gb );
MACHINE_RESET( gb );
MACHINE_RESET( gbpocket );

/* -- Super Game Boy specific -- */
#define SGB_BORDER_PAL_OFFSET	64	/* Border colours stored from pal 4-7   */
#define SGB_XOFFSET				48	/* GB screen starts at column 48        */
#define SGB_YOFFSET				40	/* GB screen starts at row 40           */

extern UINT16 sgb_pal_data[4096];	/* 512 palettes of 4 colours            */
extern UINT8 sgb_pal_map[20][18];	/* Palette tile map                     */
extern UINT16 sgb_pal[128];			/* SGB palette remapping                */
extern UINT8 *sgb_tile_data;		/* 256 tiles of 32 bytes each           */
extern UINT8 sgb_tile_map[2048];	/* 32x32 tile map data (0-tile,1-attribute) */
extern UINT8 sgb_window_mask;		/* Current GB screen mask               */
extern UINT8 sgb_hack;				/* Flag set if we're using a hack       */

extern MACHINE_RESET( sgb );
extern WRITE8_HANDLER ( sgb_io_w );

MACHINE_RESET( gbc );


/* -- Megaduck specific -- */
extern DEVICE_IMAGE_LOAD(megaduck_cart);
extern MACHINE_RESET( megaduck );
extern  READ8_HANDLER( megaduck_video_r );
extern WRITE8_HANDLER( megaduck_video_w );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type1 );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type2 );
extern  READ8_HANDLER( megaduck_sound_r1 );
extern WRITE8_HANDLER( megaduck_sound_w1 );
extern  READ8_HANDLER( megaduck_sound_r2 );
extern WRITE8_HANDLER( megaduck_sound_w2 );


/*----------- defined in video/gb.c -----------*/

READ8_HANDLER( gbc_video_r );
WRITE8_HANDLER ( gbc_video_w );

READ8_HANDLER( gb_oam_r );
WRITE8_HANDLER( gb_oam_w );
READ8_HANDLER( gb_vram_r );
WRITE8_HANDLER( gb_vram_w );

enum
{
	GB_VIDEO_DMG = 1,
	GB_VIDEO_MGB,
	GB_VIDEO_SGB,
	GB_VIDEO_CGB
};

extern UINT8 *gb_vram;

PALETTE_INIT( gb );
PALETTE_INIT( gbp );
PALETTE_INIT( sgb );
PALETTE_INIT( gbc );
PALETTE_INIT( megaduck );

READ8_HANDLER( gb_video_r );
WRITE8_HANDLER( gb_video_w );
void gb_video_init( running_machine *machine, int mode );


#endif /* GB_H_ */
