#ifndef _SMS_H_
#define _SMS_H_

#define LOG_REG
#define LOG_PAGING
//#define LOG_CURLINE
#define LOG_COLOR

#define NVRAM_SIZE								(0x8000)
#define CPU_ADDRESSABLE_SIZE			(0x10000)


#define FLAG_GAMEGEAR			0x00010000
#define FLAG_BIOS_0400			0x00020000
#define FLAG_BIOS_2000			0x00040000
#define FLAG_BIOS_FULL			0x00080000
#define FLAG_FM				0x00100000
#define FLAG_REGION_JAPAN		0x00200000

#define IS_GAMEGEAR			( Machine->gamedrv->flags & FLAG_GAMEGEAR )
#define HAS_BIOS_0400			( Machine->gamedrv->flags & FLAG_BIOS_0400 )
#define HAS_BIOS_2000			( Machine->gamedrv->flags & FLAG_BIOS_2000 )
#define HAS_BIOS_FULL			( Machine->gamedrv->flags & FLAG_BIOS_FULL )
#define HAS_BIOS			( Machine->gamedrv->flags & ( FLAG_BIOS_0400 | FLAG_BIOS_2000 | FLAG_BIOS_FULL ) )
#define HAS_FM				( Machine->gamedrv->flags & FLAG_FM )
#define IS_REGION_JAPAN			( Machine->gamedrv->flags & FLAG_REGION_JAPAN )

/* Function prototypes */

WRITE8_HANDLER(sms_cartram_w);
WRITE8_HANDLER(sms_cartram2_w);
WRITE8_HANDLER(sms_fm_detect_w);
 READ8_HANDLER(sms_fm_detect_r);
 READ8_HANDLER(sms_input_port_0_r);
WRITE8_HANDLER(sms_YM2413_register_port_0_w);
WRITE8_HANDLER(sms_YM2413_data_port_0_w);
WRITE8_HANDLER(sms_version_w);
 READ8_HANDLER(sms_version_r);
WRITE8_HANDLER(sms_mapper_w);
 READ8_HANDLER(sms_mapper_r);
WRITE8_HANDLER(sms_bios_w);
WRITE8_HANDLER(gg_sio_w);
 READ8_HANDLER(gg_sio_r);
 READ8_HANDLER(gg_psg_r);
WRITE8_HANDLER(gg_psg_w);
 READ8_HANDLER(gg_input_port_2_r);

void setup_rom(void);

void check_pause_button( void );

DEVICE_INIT( sms_cart );
DEVICE_LOAD( sms_cart );

MACHINE_START(sms);
MACHINE_RESET(sms);
INTERRUPT_GEN(sms);

#define SMS_X_PIXELS				342				/* 342 pixels */
#define NTSC_Y_PIXELS				262				/* 262 lines */
#define PAL_Y_PIXELS				313				/* 313 lines */
#define LBORDER_X_PIXELS			(0x0D)				/* 13 pixels */
#define RBORDER_X_PIXELS			(0x0F)				/* 15 pixels */
#define NTSC_192_TBORDER_Y_PIXELS		(0x1B)				/* 27 lines */
#define NTSC_192_BBORDER_Y_PIXELS		(0x18)				/* 24 lines */
#define NTSC_224_TBORDER_Y_PIXELS		(0x0B)				/* 11 lines */
#define NTSC_224_BBORDER_Y_PIXELS		(0x08)				/* 8 lines */
#define PAL_192_TBORDER_Y_PIXELS		(0x36)				/* 54 lines */
#define PAL_192_BBORDER_Y_PIXELS		(0x30)				/* 48 lines */
#define PAL_224_TBORDER_Y_PIXELS		(0x26)				/* 38 lines */
#define PAL_224_BBORDER_Y_PIXELS		(0x20)				/* 32 lines */
#define PAL_240_TBORDER_Y_PIXELS		(0x1E)				/* 30 lines */
#define PAL_240_BBORDER_Y_PIXELS		(0x18)				/* 24 lines */

/* Hard coded top and bottom borders for ease of reference */
/* using 11 works for both NTSC and PAL */
#define TBORDER_Y_PIXELS			(0x0B)					/* 11 lines */
#define BBORDER_Y_PIXELS			(0x0B)					/* 11 lines */

#define GG_CRAM_SIZE				(0x40)	/* 32 colors x 2 bytes per color = 64 bytes */
#define SMS_CRAM_SIZE				(0x20)	/* 32 colors x 1 bytes per color = 32 bytes */
#define MAX_CRAM_SIZE				0x40

#define VRAM_SIZE						(0x4000)

#define NUM_OF_REGISTER			(0x10)	/* 16 registers */

#define STATUS_VINT					(0x80)	/* Pending vertical interrupt flag */
#define STATUS_SPROVR					(0x40)	/* Sprite overflow flag */
#define STATUS_SPRCOL					(0x20)	/* Object collision flag */
#define STATUS_HINT					(0x02)	/* Pending horizontal interrupt flag */

#define IO_EXPANSION				(0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE				(0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD							(0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM					(0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM					(0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP							(0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */

#define BACKDROP_COLOR			(0x10 + (reg[0x07] & 0x0F))

/* Function prototypes */

extern int currentLine;

VIDEO_START(sms_pal);
VIDEO_START(sms_ntsc);
VIDEO_UPDATE(sms);
 READ8_HANDLER(sms_vdp_curline_r);
 READ8_HANDLER(sms_vdp_data_r);
WRITE8_HANDLER(sms_vdp_data_w);
 READ8_HANDLER(sms_vdp_ctrl_r);
WRITE8_HANDLER(sms_vdp_ctrl_w);
void sms_refresh_line(mame_bitmap *bitmap, int line);
void sms_update_palette(void);
void sms_set_ggsmsmode(int mode);

#endif /* _SMS_H_ */

