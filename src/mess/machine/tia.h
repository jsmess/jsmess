/* TIA *Write* Addresses (6 bit) */

#define VSYNC	0x00	/* Vertical Sync Set/Clear              */
#define	VBLANK	0x01    /* Vertical Blank Set/Clear	            */
#define	WSYNC	0x02    /* Wait for Horizontal Blank            */
#define	RSYNC	0x03    /* Reset Horizontal Sync Counter        */
#define	NUSIZ0	0x04    /* Number-Size player/missle 0	        */
#define	NUSIZ1	0x05    /* Number-Size player/missle 1		    */


#define	COLUP0	0x06    /* Color-Luminance Player 0			    */
#define	COLUP1	0x07    /* Color-Luminance Player 1				*/
#define	COLUPF	0x08    /* Color-Luminance Playfield			*/
#define	COLUBK	0x09    /* Color-Luminance BackGround			*/
/*
COLUP0, COLUP1, COLUPF, COLUBK:
These addresses write data into the player, playfield
and background color-luminance registers:

COLOR           D7  D6  D5  D4 | D3  D2  D1  LUM
grey/gold       0   0   0   0  | 0   0   0   black
                0   0   0   1  | 0   0   1   dark grey
orange/brt org  0   0   1   0  | 0   1   0
                0   0   1   1  | 0   1   1   grey
pink/purple     0   1   0   0  | 1   0   0
                0   1   0   1  | 1   0   1
purp/blue/blue  0   1   1   0  | 1   1   0   light grey
                0   1   1   1  | 1   1   1   white
blue/lt blue    1   0   0   0
                1   0   0   1
torq/grn blue   1   0   1   0
                1   0   1   1
grn/yel grn     1   1   0   0
                1   1   0   1
org.grn/lt org  1   1   1   0
                1   1   1   1
*/

#define	CTRLPF	0x0A    /* Control Playfield, Ball, Collisions	*/
#define	REFP0	0x0B    /* Reflection Player 0					*/
#define	REFP1	0x0C    /* Reflection Player 1					*/
#define PF0	    0x0D    /* Playfield Register Byte 0			*/
#define	PF1	    0x0E    /* Playfield Register Byte 1			*/
#define	PF2	    0x0F    /* Playfield Register Byte 2			*/
#define	RESP0	0x10    /* Reset Player 0						*/
#define	RESP1	0x11    /* Reset Player 1						*/
#define	RESM0	0x12    /* Reset Missle 0						*/
#define	RESM1	0x13    /* Reset Missle 1						*/
#define	RESBL	0x14    /* Reset Ball							*/

#define	AUDC0	0x15    /* Audio Control 0						*/
#define	AUDC1	0x16    /* Audio Control 1						*/
#define	AUDF0	0x17    /* Audio Frequency 0					*/
#define	AUDF1	0x18    /* Audio Frequency 1					*/
#define	AUDV0	0x19    /* Audio Volume 0						*/
#define	AUDV1	0x1A    /* Audio Volume 1						*/

/* The next 5 registers are flash registers */
#define	GRP0	0x1B    /* Graphics Register Player 0			*/
#define	GRP1	0x1C    /* Graphics Register Player 0			*/
#define	ENAM0	0x1D    /* Graphics Enable Missle 0				*/
#define	ENAM1	0x1E    /* Graphics Enable Missle 1				*/
#define	ENABL	0x1F    /* Graphics Enable Ball					*/


#define HMP0	0x20    /* Horizontal Motion Player 0			*/
#define	HMP1	0x21    /* Horizontal Motion Player 0			*/
#define	HMM0	0x22    /* Horizontal Motion Missle 0			*/
#define	HMM1	0x23    /* Horizontal Motion Missle 1			*/
#define	HMBL	0x24    /* Horizontal Motion Ball				*/
#define	VDELP0	0x25    /* Vertical Delay Player 0				*/
#define	VDELP1	0x26    /* Vertical Delay Player 1				*/
#define	VDELBL	0x27    /* Vertical Delay Ball					*/
#define	RESMP0	0x28    /* Reset Missle 0 to Player 0			*/
#define	RESMP1	0x29    /* Reset Missle 1 to Player 1			*/
#define	HMOVE	0x2A    /* Apply Horizontal Motion				*/
#define	HMCLR	0x2B    /* Clear Horizontal Move Registers		*/
#define	CXCLR	0x2C    /* Clear Collision Latches				*/

/* TIA *Read* Addresses */
                        /*          bit 6  bit 7				*/
#define	CXM0P	0x00	/* Read Collision M0-P1  M0-P0			*/
#define	CXM1P	0x01    /*                M1-P0  M1-P1			*/
#define	CXP0FB	0x02	/*                P0-PF  P0-BL			*/
#define	CXP1FB	0x03    /*                P1-PF  P1-BL			*/
#define	CXM0FB	0x04    /*                M0-PF  M0-BL			*/
#define	CXM1FB	0x05    /*                M1-PF  M1-BL			*/
#define	CXBLPF	0x06    /*                BL-PF  -----			*/
#define	CXPPMM	0x07    /*                P0-P1  M0-M1			*/
#define	INPT0	0x08    /* Read Pot Port 0						*/
#define	INPT1	0x09    /* Read Pot Port 1						*/
#define	INPT2	0x0A    /* Read Pot Port 2						*/
#define	INPT3	0x0B    /* Read Pot Port 3						*/
#define	INPT4	0x0C    /* Read Input (Trigger) 0				*/
#define	INPT5	0x0D    /* Read Input (Trigger) 1				*/




/* keep a record of the colors here */
typedef struct color_registers {
	int P0; /* player 0   */
	int M0;	/* missile 0  */
	int P1;	/* player 1   */
	int M1;	/* missile 1  */
	int PF;	/* playfield  */
	int BL;	/* ball       */
	int BK;	/* background */
} COLREG;


/* keep a record of the playfield registers here */
typedef struct playfield_registers {
	int B0; /* 8 bits, only left 4 bits used */
	int B1; /* 8 bits  */
	int B2;	/* 8 bits  */
} PFREG;

int scanline_registers[80]; /* array to hold info on who gets displayed */



typedef struct
{
    COLREG colreg;			/* keep a record of the color registers written to */
	PFREG  pfreg;			/* keep a record of the playfield registers written to */
}TIA;
