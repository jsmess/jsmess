/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

***************************************************************************/

#define DSK1_PORT	224 	/* Disk Drive 1 Port 8 ports - decode port number as (bit 0-1 address lines, bit 2 side) */
#define DSK2_PORT	240 	/* Disk Drive 2 Port 8 ports */

#define LPEN_PORT	248 	/* X location of raster (Not supported yet) */
#define CLUT_PORT	248 	/* Base port for CLUT (Write Only) */
#define LINE_PORT	249 	/* Line interrupt port (Write Only) */
#define STAT_PORT	249 	/* Keyboard status hi (Read Only) */
#define LMPR_PORT	250 	/* Low bank page register */
#define HMPR_PORT	251 	/* Hi bank page register */
#define VMPR_PORT	252 	/* Screen page register */
#define KEYB_PORT	254 	/* Keyboard status low (Read Only) */
#define BORD_PORT	254 	/* Border Port (Write Only) */
#define SSND_DATA	255 	/* Sound data port */
#define HPEN_PORT	504 	/* Y location of raster (currently == curvideo line + 10 vblank lines!) */
#define SSND_ADDR	511 	/* Sound address port */

#define LMPR_RAM0	0x20	/* If bit set ram is paged into bank 0, else its rom0 */
#define LMPR_ROM1	0x40	/* If bit set rom1 is paged into bank 3, else its ram */

extern UINT8 LMPR,HMPR,VMPR;/* Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252) */
extern UINT8 CLUT[16];		/* 16 entries in a palette (no line affects supported yet!) */
extern UINT8 SOUND_ADDR;	/* Current Address in sound registers */
extern UINT8 SOUND_REG[32]; /* 32 sound registers */
extern UINT8 LINE_INT;		/* Line interrupt register */
extern UINT8 LPEN,HPEN; 	/* ??? */
extern UINT8 CURLINE;		/* Current scanline */
extern UINT8 STAT;			/* returned when port 249 read */

void coupe_update_memory(void);

DEVICE_LOAD( coupe_floppy );
MACHINE_RESET( coupe );

void coupe_eof_callback(void);

void drawMode1_line(mame_bitmap *,int);
void drawMode2_line(mame_bitmap *,int);
void drawMode3_line(mame_bitmap *,int);
void drawMode4_line(mame_bitmap *,int);

