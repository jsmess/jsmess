#ifndef __NICK_GRAPHICS_CHIP_HEADER_INCLUDED__
#define __NICK_GRAPHICS_CHIP_HEADER_INCLUDED__

/* Nick Graphics Chip - found in Enterprise */

#include "driver.h"

/* initialise palette function */
extern PALETTE_INIT( nick );

#define NICK_PALETTE_SIZE	256
#define NICK_COLOURTABLE_SIZE	256

/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_RED8(x) \
	((		\
	  ( ( (x & (1<<0)) >>0) <<2) | \
	  ( ( (x & (1<<3)) >>3) <<1) | \
	  ( ( (x & (1<<6)) >>6) <<0) \
	)<<5)

/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_GREEN8(x) \
	((	\
	  ( ( (x & (1<<1)) >>1) <<2) | \
	  ( ( (x & (1<<4)) >>4) <<1) | \
	  ( ( (x & (1<<7)) >>7) <<0) \
	)<<5)
 
/* given a colour index in range 0..255 gives the Red component */
#define NICK_GET_BLUE8(x) \
	(( \
	  ( ( (x & (1<<2)) >>2) <<1) | \
	  ( ( (x & (1<<5)) >>5) <<0) \
	)<<6)


/* Nick executes a Display list, in the form of a list of Line Parameter
Tables, this is the form of the data */
typedef struct LPT_ENTRY
{
	unsigned char SC;		/* scanlines in this modeline (two's complement) */
	unsigned char MB;		/* the MODEBYTE (defines video display mode) */
	unsigned char LM;		/* left margin etc */
	unsigned char RM;		/* right margin etc */
	unsigned char LD1L;	/* (a7..a0) of line data pointer LD1 */
	unsigned char LD1H;	/* (a8..a15) of line data pointer LD1 */
	unsigned char LD2L;	/* (a7..a0) of line data pointer LD2 */
	unsigned char LD2H;	/* (a8..a15) of line data pointer LD2 */
	unsigned char COL[8];	/* COL0..COL7 */
} LPT_ENTRY;

typedef struct NICK_STATE
{
	/* horizontal position */
	unsigned char HorizontalClockCount;
	/* current scanline within LPT */
	unsigned char ScanLineCount;

	unsigned char FIXBIAS;
	unsigned char BORDER;
	unsigned char LPL;
	unsigned char LPH;

	unsigned long LD1;
	unsigned long LD2;

	LPT_ENTRY	LPT;
	unsigned short *dest;

	unsigned char Reg[16];
} NICK_STATE;

/* colour mode types */
#define	NICK_2_COLOUR_MODE	0
#define	NICK_4_COLOUR_MODE	1
#define	NICK_16_COLOUR_MODE	2
#define	NICK_256_COLOUR_MODE	3	

/* Display mode types */
#define	NICK_VSYNC_MODE	0
#define	NICK_PIXEL_MODE	1
#define	NICK_ATTR_MODE	2
#define	NICK_CH256_MODE	3
#define	NICK_CH128_MODE	4
#define	NICK_CH64_MODE	5
#define	NICK_UNUSED_MODE	6
#define	NICK_LPIXEL_MODE	7

/* MODEBYTE defines */
#define	NICK_MB_VIRQ			(1<<7)
#define	NICK_MB_VRES			(1<<4)
#define	NICK_MB_LPT_RELOAD		(1<<0)

/* Left margin defines */
#define	NICK_LM_MSBALT			(1<<7)
#define	NICK_LM_LSBALT			(1<<6)

/* Right margin defines */
#define	NICK_RM_ALTIND1			(1<<7)
#define	NICK_RM_ALTIND0			(1<<6)

/* useful macros */
#define	NICK_GET_LEFT_MARGIN(x)		(x & 0x03f)
#define	NICK_GET_RIGHT_MARGIN(x)	(x & 0x03f)
#define	NICK_GET_DISPLAY_MODE(x)	((x>>1) & 0x07)
#define	NICK_GET_COLOUR_MODE(x)		((x>>5) & 0x03)

#define NICK_RELOAD_LPT(x)			(x & 0x080)
#define NICK_CLOCK_LPT(x)			(x & 0x040)

/* Macros to generate memory address is CHx modes */
/* x = LD2, y = buf1 */
#define ADDR_CH256(x,y)		(((x & 0x0ff)<<8) | (y & 0x0ff))
#define ADDR_CH128(x,y)		(((x & 0x01ff)<<7) | (y & 0x07f))
#define ADDR_CH64(x,y)		(((x & 0x03ff)<<6) | (y & 0x03f))

#endif



