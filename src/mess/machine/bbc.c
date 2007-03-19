/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include "ctype.h"
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/wd17xx.h"
#include "includes/bbc.h"
#include "includes/upd7002.h"
#include "includes/i8271.h"
#include "machine/mc146818.h"
#include "machine/mc6850.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "sound/sn76496.h"
#include "sound/tms5220.h"

#include "image.h"


/* BBC Memory Size */
static int bbc_RAMSize=1;
/* this stores the DIP switch setting for the DFS type being used */
static int bbc_DFSType=0;
/* this stores the DIP switch setting for the SWRAM type being used */
static int bbc_SWRAMtype=0;

/* if 0 then we are emulating a BBC B style machine
   if 1 then we are emulating a BBC Master style machine */
static int bbc_Master=0;


/****************************
 IRQ inputs
*****************************/

static int via_system_irq=0;
static int via_user_irq=0;
static int MC6850_irq=0;
static int ACCCON_IRR=0;

static void bbc_setirq(void)
{
  cpunum_set_input_line(0, M6502_IRQ_LINE, via_system_irq|via_user_irq|MC6850_irq|ACCCON_IRR);
}

/************************
Sideways RAM/ROM code
*************************/

//This is the latch that holds the sideways ROM bank to read
static int bbc_rombank=0;

//This stores the sideways RAM latch type.
//Acorn and others use the bbc_rombank latch to select the write bank to be used.(type 0)
//Solidisc use the BBC's userport to select the write bank to be used (type 1)

static int bbc_userport=0;


/*************************
Model A memory handling functions
*************************/

/* for the model A just address the 4 on board ROM sockets */
WRITE8_HANDLER ( page_selecta_w )
{
	memory_set_bankptr(4,memory_region(REGION_USER1)+((data&0x03)<<14));
}


WRITE8_HANDLER ( memorya1_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}

/*************************
Model B memory handling functions
*************************/

/* the model B address all 16 of the ROM sockets */
/* I have set bank 1 as a special case to load different DFS roms selectable from MESS's DIP settings var:bbc_DFSTypes */
WRITE8_HANDLER ( page_selectb_w )
{
	bbc_rombank=data&0x0f;
	if (bbc_rombank!=1)
	{
		memory_set_bankptr(4,memory_region(REGION_USER1)+(bbc_rombank<<14));
	} else {
		memory_set_bankptr(4,memory_region(REGION_USER2)+((bbc_DFSType)<<14));
	}
}


WRITE8_HANDLER ( memoryb3_w )
{
	if (bbc_RAMSize) {
		memory_region(REGION_CPU1)[offset+0x4000]=data;
		// this array is set so that the video emulator know which addresses to redraw
		vidmem[offset+0x4000]=1;
	} else {
		memory_region(REGION_CPU1)[offset]=data;
		vidmem[offset]=1;
	}

}

/* I have setup 3 types of sideways ram:
0: none
1: 128K (bank 8 to 15) Solidisc sidewaysram userport bank latch
2: 64K (banks 4 to 7) for Acorn sideways ram FE30 bank latch
3: 128K (banks 8 to 15) for Acown sideways ram FE30 bank latch
*/
static unsigned short bbc_SWRAMtype1[16]={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
static unsigned short bbc_SWRAMtype2[16]={0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0};
static unsigned short bbc_SWRAMtype3[16]={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};

WRITE8_HANDLER ( memoryb4_w )
{
	if (bbc_rombank==1)
	{
		// special DFS case for Acorn DFS E00 Hack that can write to the DFS RAM Bank;
		if (bbc_DFSType==3) memory_region(REGION_USER2)[((bbc_DFSType)<<14)+offset]=data;
	} else
	{
		switch (bbc_SWRAMtype) {
			case 1:	if (bbc_SWRAMtype1[bbc_userport]) memory_region(REGION_USER1)[(bbc_userport<<14)+offset]=data;
			case 2:	if (bbc_SWRAMtype2[bbc_rombank])  memory_region(REGION_USER1)[(bbc_rombank<<14)+offset]=data;
			case 3:	if (bbc_SWRAMtype3[bbc_rombank])  memory_region(REGION_USER1)[(bbc_rombank<<14)+offset]=data;
		}
	}
}

/****************************************/
/* BBC B Plus memory handling function */
/****************************************/

static int pagedRAM=0;
static int vdusel=0;

/*  this function should return true if
	the instruction is in the VDU driver address ranged
	these are set when:
	PC is in the range c000 to dfff
	or if pagedRAM set and PC is in the range a000 to afff
*/
static int vdudriverset(void)
{
	int PC;
	PC=activecpu_get_pc(); // this needs to be set to the 6502 program counter
	return (((PC>=0xc000) && (PC<=0xdfff)) || ((pagedRAM) && ((PC>=0xa000) && (PC<=0xafff))));
}


/* the model B Plus addresses all 16 of the ROM sockets plus the extra 12K of ram at 0x8000
   and 20K of shadow ram at 0x3000 */
WRITE8_HANDLER ( page_selectbp_w )
{
	if ((offset&0x04)==0)
	{
		pagedRAM   =(data>>7)&0x01;
		bbc_rombank= data    &0x0f;

		if (pagedRAM)
		{
			/* if paged ram then set 8000 to afff to read from the ram 8000 to afff */
			memory_set_bankptr(4,memory_region(REGION_CPU1)+0x8000);
		} else {
			/* if paged rom then set the rom to be read from 8000 to afff */
			memory_set_bankptr(4,memory_region(REGION_USER1)+(bbc_rombank<<14));
		};

		/* set the rom to be read from b000 to bfff */
		memory_set_bank(6, bbc_rombank);
	}
	else
	{
		//the video display should now use this flag to display the shadow ram memory
		vdusel=(data>>7)&0x01;
		bbcbp_setvideoshadow(vdusel);
		//need to make the video display do a full screen refresh for the new memory area
		memory_set_bankptr(2, memory_region(REGION_CPU1)+0x3000);
	}
}


/* write to the normal memory from 0x0000 to 0x2fff
   the writes to this memory are just done the normal
   way */

WRITE8_HANDLER ( memorybp1_w )
{
	memory_region(REGION_CPU1)[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	vidmem[offset]=1;
}





/* the next two function handle reads and write to the shadow video ram area
   between 0x3000 and 0x7fff

   when vdusel is set high the video display uses the shadow ram memory
   the processor only reads and write to the shadow ram when vdusel is set
   and when the instruction being executed is stored in a set range of memory
   addresses known as the VDU driver instructions.
*/


static OPBASE_HANDLER( bbcbp_opbase_handler )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just read normal ram
		memory_set_bankptr(2, memory_region(REGION_CPU1)+0x3000);
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then read from shadow ram
			memory_set_bankptr(2, memory_region(REGION_CPU1)+0xb000);
		} else {
			// else read from normal ram
			memory_set_bankptr(2, memory_region(REGION_CPU1)+0x3000);
		}
	}
	return address;
}


WRITE8_HANDLER ( memorybp2_w )
{
	if (vdusel==0)
	{
		// not in shadow ram mode so just write to normal ram
		memory_region(REGION_CPU1)[offset+0x3000]=data;
		vidmem[offset+0x3000]=1;
	} else {
		if (vdudriverset())
		{
			// if VDUDriver set then write to shadow ram
			memory_region(REGION_CPU1)[offset+0xb000]=data;
			vidmem[offset+0xb000]=1;
		} else {
			// else write to normal ram
			memory_region(REGION_CPU1)[offset+0x3000]=data;
			vidmem[offset+0x3000]=1;
		}
	}
}


/* if the pagedRAM is set write to RAM between 0x8000 to 0xafff
otherwise this area contains ROM so no write is required */
WRITE8_HANDLER ( memorybp4_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
}


/* the BBC B plus 128K had extra ram mapped in replacing the
rom bank 0,1,c and d.
The function memorybp3_128_w handles memory writes from 0x8000 to 0xafff
which could either be sideways ROM, paged RAM, or sideways RAM.
The function memorybp4_128_w handles memory writes from 0xb000 to 0xbfff
which could either be sideways ROM or sideways RAM */


static unsigned short bbc_b_plus_sideways_ram_banks[16]={ 1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0 };


WRITE8_HANDLER ( memorybp4_128_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
	else
	{
		if (bbc_b_plus_sideways_ram_banks[bbc_rombank])
		{
			memory_region(REGION_USER1)[offset+(bbc_rombank<<14)]=data;
		}
	}
}

WRITE8_HANDLER ( memorybp6_128_w )
{
	if (bbc_b_plus_sideways_ram_banks[bbc_rombank])
	{
		memory_region(REGION_USER1)[offset+(bbc_rombank<<14)+0x3000]=data;
	}
}



/****************************************/
/* BBC Master functions                 */
/****************************************/

/*
ROMSEL - &FE30 write Only
B7 RAM 1=Page in ANDY &8000-&8FFF
	   0=Page in ROM  &8000-&8FFF
B6 Not Used
B5 Not Used
B4 Not Used
B3-B0  Rom/Ram Bank Select

ACCCON

b7 IRR	1=Causes an IRQ to the processor
b6 TST  1=Selects &FC00-&FEFF read from OS-ROM
b5 IFJ  1=Internal 1Mhz bus
		0=External 1Mhz bus
b4 ITU  1=Internal Tube
		0=External Tube
b3 Y	1=Read/Write HAZEL &C000-&DFFF RAM
		0=Read/Write ROM &C000-&DFFF OS-ROM
b2 X	1=Read/Write LYNNE
		0=Read/WRITE main memory &3000-&8000
b1 E    1=Causes shadow if VDU code
		0=Main all the time
b0 D	1=Display LYNNE as screen
		0=Display main RAM screen

ACCCON is a read/write register

HAZEL is the 8K of RAM used by the MOS,filling system, and other Roms at &C000-&DFFF

ANDY is the name of the 4K of RAM used by the MOS at &8000-&8FFF

b7:This causes an IRQ to occur. If you set this bit, you must write a routine on IRQ2V to clear it.

b6:If set, this switches in the ROM at &FD00-&FEFF for reading, writes to these addresses are still directed to the I/O
The MOS will not work properly with this but set. the Masters OS uses this feature to place some of the reset code in this area,
which is run at powerup.

b3:If set, access to &C000-&DFFF are directed to HAZEL, an 8K bank of RAM. IF clear, then operating system ROM is read.

b2:If set, read/write access to &3000-&7FFF are directed to the LYNNE, the shadow screen RAM. If clear access is made to the main RAM.

b1:If set, when the program counter is between &C000 and &DFFF, read/write access is directed to the LYNNE shadow RAM.
if the program counter is anywhere else main ram is accessed.

*/

static int ACCCON=0;
//static int ACCCON_IRR=0; This is defined at the top in the IRQ code
static int ACCCON_TST=0;
static int ACCCON_IFJ=0;
static int ACCCON_ITU=0;
static int ACCCON_Y=0;
static int ACCCON_X=0;
static int ACCCON_E=0;
static int ACCCON_D=0;

READ8_HANDLER ( bbcm_ACCCON_read )
{
	logerror("ACCCON read %d\n",offset);
	return ACCCON;
}

WRITE8_HANDLER ( bbcm_ACCCON_write )
{
	int tempIRR;
	ACCCON=data;

  	logerror("ACCCON write  %d %d \n",offset,data);

  	tempIRR=ACCCON_IRR;
  	ACCCON_IRR=(data>>7)&1;

  	ACCCON_TST=(data>>6)&1;
  	ACCCON_IFJ=(data>>5)&1;
  	ACCCON_ITU=(data>>4)&1;
  	ACCCON_Y  =(data>>3)&1;
  	ACCCON_X  =(data>>2)&1;
  	ACCCON_E  =(data>>1)&1;
  	ACCCON_D  =(data>>0)&1;

  	if (tempIRR!=ACCCON_IRR)
  	{
		bbc_setirq();
	}

	if (ACCCON_Y)
	{
		memory_set_bankptr(7,memory_region(REGION_CPU1)+0x9000);
	} else {
		memory_set_bankptr(7,memory_region(REGION_USER1)+0x40000);
	}

	bbcbp_setvideoshadow(ACCCON_D);


	if (ACCCON_X)
	{
		memory_set_bankptr( 2, memory_region( REGION_CPU1 ) + 0xb000 );
	} else {
		memory_set_bankptr( 2, memory_region( REGION_CPU1 ) + 0x3000 );
	}

}


static int bbcm_vdudriverset(void)
{
	int PC;
	PC=activecpu_get_pc();
	return ((PC>=0xc000) && (PC<=0xdfff));
}


WRITE8_HANDLER ( page_selectbm_w )
{
	pagedRAM=(data&0x80)>>7;
	bbc_rombank=data&0x0f;

	if (pagedRAM)
	{
		memory_set_bankptr(4,memory_region(REGION_CPU1)+0x8000);
		memory_set_bank(5, bbc_rombank);
	} else {
		memory_set_bankptr(4,memory_region(REGION_USER1)+((bbc_rombank)<<14));
		memory_set_bank(5, bbc_rombank);
	}
}



WRITE8_HANDLER ( memorybm1_w )
{
	memory_region(REGION_CPU1)[offset]=data;
	vidmem[offset]=1;
}


static OPBASE_HANDLER( bbcm_opbase_handler )
{
	if (ACCCON_X)
	{
		memory_set_bankptr( 2, memory_region( REGION_CPU1 ) + 0xb000 );
	} else {
		if (ACCCON_E && bbcm_vdudriverset())
		{
			memory_set_bankptr( 2, memory_region( REGION_CPU1 ) + 0xb000 );
		} else {
			memory_set_bankptr( 2, memory_region( REGION_CPU1 ) + 0x3000 );
		}
	}

	return address;
}



WRITE8_HANDLER ( memorybm2_w )
{
	if (ACCCON_X)
	{
		memory_region(REGION_CPU1)[offset+0xb000]=data;
		vidmem[offset+0xb000]=1;
	} else {
		if (ACCCON_E && bbcm_vdudriverset())
		{
			memory_region(REGION_CPU1)[offset+0xb000]=data;
			vidmem[offset+0xb000]=1;
		} else {
			memory_region(REGION_CPU1)[offset+0x3000]=data;
			vidmem[offset+0x3000]=1;
		}
	}
}

static unsigned short bbc_master_sideways_ram_banks[16]=
{
	0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0
};


WRITE8_HANDLER ( memorybm4_w )
{
	if (pagedRAM)
	{
		memory_region(REGION_CPU1)[offset+0x8000]=data;
	}
	else
	{
		if (bbc_master_sideways_ram_banks[bbc_rombank])
		{
			memory_region(REGION_USER1)[offset+(bbc_rombank<<14)]=data;
		}
	}
}


WRITE8_HANDLER ( memorybm5_w )
{
	if (bbc_master_sideways_ram_banks[bbc_rombank])
	{
		memory_region(REGION_USER1)[offset+(bbc_rombank<<14)+0x1000]=data;
	}
}


WRITE8_HANDLER ( memorybm7_w )
{
	if (ACCCON_Y)
	{
		memory_region(REGION_CPU1)[offset+0x9000]=data;
	}
}



/******************************************************************************
&FC00-&FCFF FRED
&FD00-&FDFF JIM
&FE00-&FEFF SHEILA		Read					Write
&00-&07 6845 CRTC		Video controller		Video Controller		 8 ( 2 bytes x	4 )
&08-&0F 6850 ACIA		Serial controller		Serial Controller		 8 ( 2 bytes x	4 )
&10-&17 Serial ULA		-						Serial system chip		 8 ( 1 byte  x  8 )
&18-&1F uPD7002 		A to D converter	 	A to D converter		 8 ( 4 bytes x	2 )
&20-&23 Video ULA		-						Video system chip		 4 ( 2 bytes x	2 )
&24-&27 FDC Latch		1770 Control latch		1770 Control latch		 4 ( 1 byte  x  4 )
&28-&2F 1770 registers  1770 Disc Controller	1170 Disc Controller	 8 ( 4 bytes x  2 )
&30-&33 ROMSEL			-						ROM Select				 4 ( 1 byte  x  4 )
&34-&37 ACCCON			ACCCON select reg.		ACCCON select reg		 4 ( 1 byte  x  4 )
&38-&3F NC				-						-
&40-&5F 6522 VIA		SYSTEM VIA				SYSTEM VIA				32 (16 bytes x	2 ) 1Mhz
&60-&7F 6522 VIA		USER VIA				USER VIA				32 (16 bytes x	2 ) 1Mhz
&80-&9F Int. Modem		Int. Modem				Int Modem
&A0-&BF 68B54 ADLC		ECONET controller		ECONET controller		32 ( 4 bytes x	8 ) 2Mhz
&C0-&DF NC				-						-
&E0-&FF Tube ULA		Tube system interface	Tube system interface	32 (32 bytes x  1 ) 2Mhz
******************************************************************************/

READ8_HANDLER ( bbcm_r )
{
long myo;

	if ( ACCCON_TST )
	{
		return memory_region(REGION_USER1)[offset+0x43c00];
	};

	if ((offset>=0x000) && (offset<=0x0ff)) /* FRED */
	{
		return 0xff;
	};

	if ((offset>=0x100) && (offset<=0x1ff)) /* JIM */
	{
		return 0xff;
	};


	if ((offset>=0x200) && (offset<=0x2ff)) /* SHEILA */
	{
		myo=offset-0x200;
		if ((myo>=0x00) && (myo<=0x07)) return BBC_6845_r(myo-0x00);		/* Video Controller */
		if ((myo>=0x08) && (myo<=0x0f)) return BBC_6850_r(myo-0x08);		/* Serial Controller */
		if ((myo>=0x10) && (myo<=0x17)) return 0xfe;						/* Serial System Chip */
		if ((myo>=0x18) && (myo<=0x1f)) return uPD7002_r(myo-0x18);			/* A to D converter */
		if ((myo>=0x20) && (myo<=0x23)) return 0xfe;						/* VideoULA */
		if ((myo>=0x24) && (myo<=0x27)) return bbcm_wd1770l_read(myo-0x24); /* 1770 */
		if ((myo>=0x28) && (myo<=0x2f)) return bbcm_wd1770_read(myo-0x28);  /* disc control latch */
		if ((myo>=0x30) && (myo<=0x33)) return 0xfe;						/* page select */
		if ((myo>=0x34) && (myo<=0x37)) return bbcm_ACCCON_read(myo-0x34);	/* ACCCON */
		if ((myo>=0x38) && (myo<=0x3f)) return 0xfe;						/* NC ?? */
		if ((myo>=0x40) && (myo<=0x5f)) return via_0_r(myo-0x40);
		if ((myo>=0x60) && (myo<=0x7f)) return via_1_r(myo-0x60);
		if ((myo>=0x80) && (myo<=0x9f)) return 0xfe;
		if ((myo>=0xa0) && (myo<=0xbf)) return 0xfe;
		if ((myo>=0xc0) && (myo<=0xdf)) return 0xfe;
		if ((myo>=0xe0) && (myo<=0xff)) return 0xfe;
	}

	return 0xfe;
}

WRITE8_HANDLER ( bbcm_w )
{
long myo;

	if ((offset>=0x200) && (offset<=0x2ff)) /* SHEILA */
	{
		myo=offset-0x200;
		if ((myo>=0x00) && (myo<=0x07)) BBC_6845_w(myo-0x00,data);			/* Video Controller */
		if ((myo>=0x08) && (myo<=0x0f)) BBC_6850_w(myo-0x08,data);			/* Serial Controller */
		if ((myo>=0x10) && (myo<=0x17)) BBC_SerialULA_w(myo-0x10,data);		/* Serial System Chip */
		if ((myo>=0x18) && (myo<=0x1f)) uPD7002_w(myo-0x18,data);			/* A to D converter */
		if ((myo>=0x20) && (myo<=0x23)) videoULA_w(myo-0x20,data);			/* VideoULA */
		if ((myo>=0x24) && (myo<=0x27)) bbcm_wd1770l_write(myo-0x24,data); 	/* 1770 */
		if ((myo>=0x28) && (myo<=0x2f)) bbcm_wd1770_write(myo-0x28,data);  	/* disc control latch */
		if ((myo>=0x30) && (myo<=0x33)) page_selectbm_w(myo-0x30,data);		/* page select */
		if ((myo>=0x34) && (myo<=0x37)) bbcm_ACCCON_write(myo-0x34,data);	/* ACCCON */
		//if ((myo>=0x38) && (myo<=0x3f)) 									/* NC ?? */
		if ((myo>=0x40) && (myo<=0x5f)) via_0_w(myo-0x40,data);
		if ((myo>=0x60) && (myo<=0x7f)) via_1_w(myo-0x60,data);
		//if ((myo>=0x80) && (myo<=0x9f))
		//if ((myo>=0xa0) && (myo<=0xbf))
		//if ((myo>=0xc0) && (myo<=0xdf))
		//if ((myo>=0xe0) && (myo<=0xff))
	}

}


/******************************************************************************

System VIA 6522

PA0-PA7
Port A forms a slow data bus to the keyboard sound and speech processors
PortA	Keyboard
D0	Pin 8
D1	Pin 9
D2	Pin 10
D3	Pin 11
D4	Pin 5
D5	Pin 6
D6	Pin 7
D7	Pin 12

PB0-PB2 outputs
---------------
These 3 outputs form the address to an 8 bit addressable latch.
(IC32 74LS259)


PB3 output
----------
This output holds the data to be written to the selected
addressable latch bit.


PB4 and PB5 inputs
These are the inputs from the joystick FIRE buttons. They are
normally at logic 1 with no button pressed and change to 0
when a button is pressed.


PB6 and PB7 inputs from the speech processor
PB6 is the speech processor 'ready' output and PB7 is from the
speech processor 'interrupt' output.


CA1 input
This is the vertical sync input from the 6845. CA1 is set up to
interrupt the 6502 every 20ms (50Hz) as a vertical sync from
the video circuity is detected. The operation system changes
the flash colours on the display in this interrupt time so that
they maintain synchronisation with the rest of the picture.
----------------------------------------------------------------
This is required for a lot of time function within the machine
and must be triggered every 20ms. (Should check at some point
how this 20ms signal is made, and see if none standard shapped
screen modes change this time period.)


CB1 input
The CB1 input is the end of conversion (EOC) signal from the
7002 analogue to digital converter. It can be used to interrupt
the 6502 whenever a conversion is complete.


CA2 input
This input comes from the keyboard circuit, and is used to
generate an interrupt whenever a key is pressed. See the
keyboard circuit section for more details.



CB2 input
This is the light pen strobe signal (LPSTB) from the light pen.
If also connects to the 6845 video processor,
CB2 can be programmed to interrupt the processor whenever
a light pen strobe occurs.
----------------------------------------------------------------
CB2 is not needed in the initial emulation
and should be set to logic low, should be mapped through to
a light pen emulator later.

IRQ output
This connects to the IRQ line of the 6502


The addressable latch
This 8 bit addressable latch is operated from port B lines 0-3.
PB0-PB2 are set to the required address of the output bit to be set.
PB3 is set to the value which should be programmed at that bit.
The function of the 8 output bits from this latch are:-

B0 - Write Enable to the sound generator IC
B1 - READ select on the speech processor
B2 - WRITE select on the speech processor
B3 - Keyboard write enable
B4,B5 - these two outputs define the number to be added to the
start of screen address in hardware to control hardware scrolling:-
Mode	Size	Start of screen	 Number to add	B5   	B4
0,1,2	20K	&3000		 12K		1    	1
3		16K	&4000		 16K		0	0
4,5		10K	&5800 (or &1800) 22K		1	0
6		8K	&6000 (or &2000) 24K		0	1
B6 - Operates the CAPS lock LED  (Pin 17 keyboard connector)
B7 - Operates the SHIFT lock LED (Pin 16 keyboard connector)


******************************************************************************/

static int b0_sound;
static int b1_speech_read;
static int b2_speech_write;
static int b3_keyboard;
static int b4_video0;
static int b5_video1;
static int b6_caps_lock_led;
static int b7_shift_lock_led;



static int MC146818_WR=0;	// FE30 bit 1 replaces  b1_speech_read
static int MC146818_DS=0;	// FE30 bit 2 replaces  b2_speech_write
static int MC146818_AS=0;	// 6522 port b bit 7
static int MC146818_CE=0;	// 6522 port b bit 6

static int via_system_porta;


// this is a counter on the keyboard
static int column=0;


INTERRUPT_GEN( bbcb_keyscan )
{
  	/* only do auto scan if keyboard is not enabled */
	if (b3_keyboard==1)
	{

  		/* KBD IC1 4 bit addressable counter */
  		/* KBD IC3 4 to 10 line decoder */
		/* keyboard not enabled so increment counter */
		column=(column+1)%16;
		if (column<10)
		{

  			/* KBD IC4 8 input NAND gate */
  			/* set the value of via_system ca2, by checking for any keys
    			 being pressed on the selected column */

			if ((readinputport(column)|0x01)!=0xff)
			{
				via_0_ca2_w(0,1);
			} else {
				via_0_ca2_w(0,0);
			}

		} else {
			via_0_ca2_w(0,0);
		}
	}
}


INTERRUPT_GEN( bbcm_keyscan )
{
  	/* only do auto scan if keyboard is not enabled */
	if (b3_keyboard==1)
	{

  		/* KBD IC1 4 bit addressable counter */
  		/* KBD IC3 4 to 10 line decoder */
		/* keyboard not enabled so increment counter */
		column=(column+1)%16;

		/* this IF should be removed as soon as the dip switches (keyboard keys) are set for the master */
		if (column<10)
			{
			/* KBD IC4 8 input NAND gate */
			/* set the value of via_system ca2, by checking for any keys
				 being pressed on the selected column */

			if ((readinputport(column)|0x01)!=0xff)
			{
				via_0_ca2_w(0,1);
			} else {
				via_0_ca2_w(0,0);
			}

		} else {
			via_0_ca2_w(0,0);
		}
	}
}



static int bbc_keyboard(int data)
{
	int bit;
	int row;
	int res;
	column=data & 0x0f;
	row=(data>>4) & 0x07;

	bit=0;

	if (column<10) {
		res=readinputport(column);
	} else {
		res=0xff;
	}

	/* Normal keyboard result */
	if ((res&(1<<row))==0)
		{ bit=1; }

	if ((res|1)!=0xff)
	{
		via_0_ca2_w(0,1);
	} else {
		via_0_ca2_w(0,0);
	}

	return (data & 0x7f) | (bit<<7);
}


static void bbcb_IC32_initialise(void)
{
	b0_sound=0x01;				// Sound is negative edge trigered
	b1_speech_read=0x01;		// ????
	b2_speech_write=0x01;		// ????
	b3_keyboard=0x01;			// Keyboard is negative edge trigered
	b4_video0=0x01;
	b5_video1=0x01;
	b6_caps_lock_led=0x01;
	b7_shift_lock_led=0x01;

}


/* This the BBC Masters Real Time Clock and NVRam IC */
void MC146818_set(void)
{
	logerror ("146181 WR=%d DS=%d AS=%d CE=%d \n",MC146818_WR,MC146818_DS,MC146818_AS,MC146818_CE);

	// if chip enabled
	if (MC146818_CE)
	{
		// if data select is set then access the data in the 146818
		if (MC146818_DS)
		{
			if (MC146818_WR)
			{
				via_system_porta=mc146818_port_r(1);
				//logerror("read 146818 data %d \n",via_system_porta);
			} else {
				mc146818_port_w(1,via_system_porta);
				//logerror("write 146818 data %d \n",via_system_porta);
			}
		}

		// if address select is set then set the address in the 146818
		if (MC146818_AS)
		{
			mc146818_port_w(0,via_system_porta);
			//logerror("write 146818 address %d \n",via_system_porta);
		}
	}
}


static WRITE8_HANDLER( bbcb_via_system_write_porta )
{

	//logerror("SYSTEM write porta %d\n",data);

	via_system_porta=data;
	if (b0_sound==0)
	{
 		//logerror("Doing an unsafe write to the sound chip %d \n",data);
		SN76496_0_w(0,via_system_porta);
	}
	if (b3_keyboard==0)
	{
 		//logerror("Doing an unsafe write to the keyboard %d \n",data);
		via_system_porta=bbc_keyboard(via_system_porta);
	}
	if (bbc_Master) MC146818_set();
}


static WRITE8_HANDLER( bbcb_via_system_write_portb )
{
	int bit,value;
	bit=data & 0x07;
	value=(data>>3) &0x01;


    //logerror("SYSTEM write portb %d %d %d\n",data,bit,value);

	if (value) {
		switch (bit) {
		case 0:
			if (b0_sound==0) {
				b0_sound=1;
			}
			break;
		case 1:
			if (bbc_Master)
			{
				if (MC146818_WR==0)
				{
					/* BBC MASTER has NV RAM Here */
					MC146818_WR=1;
					MC146818_set();
				}
			} else {
				if (b1_speech_read==0) {
					/* VSP TMS 5220 */
					b1_speech_read=1;
				}
			}
			break;
		case 2:
			if (bbc_Master)
			{
				if (MC146818_DS==0)
				{
					/* BBC MASTER has NV RAM Here */
					MC146818_DS=1;
					MC146818_set();
				}
			} else {
				if (b2_speech_write==0) {
					/* VSP TMS 5220 */
					b2_speech_write=1;
				}
			}
			break;
		case 3:
			if (b3_keyboard==0) {
				b3_keyboard=1;
			}
			break;
		case 4:
			if (b4_video0==0) {
				b4_video0=1;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 5:
			if (b5_video1==0) {
				b5_video1=1;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 6:
			if (b6_caps_lock_led==0) {
				b6_caps_lock_led=1;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (b7_shift_lock_led==0) {
				b7_shift_lock_led=1;
				/* call shift lock led update */
			}
			break;

	}
	} else {
		switch (bit) {
		case 0:
			if (b0_sound==1) {
				b0_sound=0;
				SN76496_0_w(0,via_system_porta);
			}
			break;
		case 1:
			if (bbc_Master)
			{
				if (MC146818_WR==1)
				{
					/* BBC MASTER has NV RAM Here */
					MC146818_WR=0;
					MC146818_set();
				}
			} else {
				if (b1_speech_read==1) {
					/* VSP TMS 5220 */
					b1_speech_read=0;
				}
			}
			break;
		case 2:
			if (bbc_Master)
			{
				if (MC146818_DS==1)
				{
					/* BBC MASTER has NV RAM Here */
					MC146818_DS=0;
					MC146818_set();
				}
			} else {
				if (b2_speech_write==1) {
					/* VSP TMS 5220 */
					b2_speech_write=0;
				}
			}
			break;
		case 3:
			if (b3_keyboard==1) {
				b3_keyboard=0;
				/* *** call keyboard enabled *** */
				via_system_porta=bbc_keyboard(via_system_porta);
			}
			break;
		case 4:
			if (b4_video0==1) {
				b4_video0=0;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 5:
			if (b5_video1==1) {
				b5_video1=0;
				setscreenstart(b4_video0,b5_video1);
			}
			break;
		case 6:
			if (b6_caps_lock_led==1) {
				b6_caps_lock_led=0;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (b7_shift_lock_led==1) {
				b7_shift_lock_led=0;
				/* call shift lock led update */
			}
			break;
		}
	}



	if (bbc_Master)
	{
		//set the Address Select
		if (MC146818_AS != ((data>>7)&1))
		{
			MC146818_AS=(data>>7)&1;
			MC146818_set();
		}

		//if CE changes
		if (MC146818_CE != ((data>>6)&1))
		{
			MC146818_CE=(data>>6)&1;
			MC146818_set();
		}
	}
}


static  READ8_HANDLER( bbcb_via_system_read_porta )
{
	//logerror("SYSTEM read porta %d\n",via_system_porta);
	return via_system_porta;
}

// D4 of portb is joystick fire button 1
// D5 of portb is joystick fire button 2
// D6 VSPINT
// D7 VSPRDY

/* this is the interupt and ready signal from the BBC B Speech processor */
static int TMSint=1;
static int TMSrdy=1;

//void bbc_TMSint(int status)
//{
//	TMSint=(!status)&1;
//	TMSrdy=(!tms5220_ready_r())&1;
//	via_0_portb_w(0,(0xf | readinputport(16)|(TMSint<<6)|(TMSrdy<<7)));
//}



static READ8_HANDLER( bbcb_via_system_read_portb )
{
//	TMSint=(!tms5220_int_r())&1;
//	TMSrdy=(!tms5220_ready_r())&1;

	//logerror("SYSTEM read portb %d\n",0xf | readinputport(16)|(TMSint<<6)|(TMSrdy<<7));

	return (0xf | readinputport(16)|(TMSint<<6)|(TMSrdy<<7));


}


/* vertical sync pulse from video circuit */
static  READ8_HANDLER( bbcb_via_system_read_ca1 )
{
  return 0x01;
}


/* joystick EOC */
static  READ8_HANDLER( bbcb_via_system_read_cb1 )
{
  return uPD7002_EOC_r(0);
}


/* keyboard pressed detect */
static  READ8_HANDLER( bbcb_via_system_read_ca2 )
{
  return 0x01;
}


/* light pen strobe detect (not emulated) */
static  READ8_HANDLER( bbcb_via_system_read_cb2 )
{
  return 0x01;
}


/* this is wired as in input port so writing to this port would be bad */

static WRITE8_HANDLER( bbcb_via_system_write_ca2 )
{
  //if( errorlog ) fprintf(errorlog, "via_system_write_ca2: $%02X\n", data);
}

/* this is wired as in input port so writing to this port would be bad */

static WRITE8_HANDLER( bbcb_via_system_write_cb2 )
{
  //if( errorlog ) fprintf(errorlog, "via_system_write_cb2: $%02X\n", data);
}



static void bbc_via_system_irq(int level)
{
//  logerror("SYSTEM via irq %d %d %d\n",via_system_irq,via_user_irq,level);
  via_system_irq=level;
  bbc_setirq();
}


static struct via6522_interface
bbcb_system_via= {
  bbcb_via_system_read_porta,
  bbcb_via_system_read_portb,
  bbcb_via_system_read_ca1,
  bbcb_via_system_read_cb1,
  bbcb_via_system_read_ca2,
  bbcb_via_system_read_cb2,
  bbcb_via_system_write_porta,
  bbcb_via_system_write_portb,
  NULL, NULL,
  bbcb_via_system_write_ca2,
  bbcb_via_system_write_cb2,
  bbc_via_system_irq
};


/**********************************************************************
USER VIA
Port A output is buffered before being connected to the printer connector.
This means that they can only be operated as output lines.
CA1 is pulled high by a 4K7 resistor. CA1 normally acts as an acknowledge
line when a printer is used. CA2 is buffered so that it has become an open
collector output only. It usially acts as the printer strobe line.
***********************************************************************/

/* this code just sends the output of the printer port to the errorlog
file. I now need to look at the new printer code and see how I should
connect this code up to that */

int bbc_printer_porta;
int bbc_printer_ca1;
int bbc_printer_ca2;

/* USER VIA 6522 port A is buffered as an output through IC70 so
reading from this port will always return 0xff */
static  READ8_HANDLER( bbcb_via_user_read_porta )
{
	return 0xff;
}

/* USER VIA 6522 port B is connected to the BBC user port */
static  READ8_HANDLER( bbcb_via_user_read_portb )
{
	return 0xff;
}

static  READ8_HANDLER( bbcb_via_user_read_ca1 )
{
	return bbc_printer_ca1;
}

static  READ8_HANDLER( bbcb_via_user_read_ca2 )
{
	return 1;
}

static WRITE8_HANDLER( bbcb_via_user_write_porta )
{
	bbc_printer_porta=data;
}

static WRITE8_HANDLER( bbcb_via_user_write_portb )
{
	bbc_userport=data;
}

static WRITE8_HANDLER( bbcb_via_user_write_ca2 )
{
	/* write value to printer on rising edge of ca2 */
	if ((bbc_printer_ca2==0) && (data==1))
	{
		logerror("print character $%02X\n",bbc_printer_porta);
	}
	bbc_printer_ca2=data;

	/* this is a very bad way of returning an acknowledge
	by just linking the strobe output into the acknowledge input */
	bbc_printer_ca1=data;
	via_1_ca1_w(0,data);
}

static void bbc_via_user_irq(int level)
{
  via_user_irq=level;
  bbc_setirq();
}


static struct via6522_interface bbcb_user_via =
{
	bbcb_via_user_read_porta,//via_user_read_porta,
	bbcb_via_user_read_portb,//via_user_read_portb,
	bbcb_via_user_read_ca1,//via_user_read_ca1,
	0,//via_user_read_cb1,
	bbcb_via_user_read_ca2,//via_user_read_ca2,
	0,//via_user_read_cb2,
	bbcb_via_user_write_porta,//via_user_write_porta,
	bbcb_via_user_write_portb,//via_user_write_portb,
	0, //via_user_write_ca1
	0, //via_user_write_cb1
	bbcb_via_user_write_ca2,//via_user_write_ca2,
	0,//via_user_write_cb2,
	bbc_via_user_irq //via_user_irq
};


/**************************************
BBC Joystick Support
**************************************/

static int BBC_get_analogue_input(int channel_number)
{
	switch(channel_number)
	{
		case 0:
			return ((0xff-readinputport(17))<<8);
			break;
		case 1:
			return ((0xff-readinputport(18))<<8);
			break;
		case 2:
			return ((0xff-readinputport(19))<<8);
			break;
		case 3:
			return ((0xff-readinputport(20))<<8);
			break;
	}

	return 0;
}

static void BBC_uPD7002_EOC(int data)
{
	via_0_cb1_w(0,data);
}

static struct uPD7002_interface
BBC_uPD7002= {
	BBC_get_analogue_input,
	BBC_uPD7002_EOC
};




/***************************************
  BBC 6850 Serial Controller
****************************************/

WRITE8_HANDLER ( BBC_6850_w )
{
	switch (offset&1)
	{
		case 0:
			MC6850_control_w(0,data);
			break;
		case 1:
			MC6850_data_w(0,data);
			break;
	}

	//logerror("6850 write to %02x = %02x\n",offset,data);

}

READ8_HANDLER (BBC_6850_r)
{
	int retval=0;

	switch (offset&1)
	{
		case 0:
			retval=MC6850_status_r(0);
			break;
		case 1:
			retval=MC6850_data_r(0);
			break;
	}
	//logerror("6850 read from %02x = %02x\n",offset,retval);
	return retval;
}



void Serial_interrupt(int level)
{
  MC6850_irq=level;
  bbc_setirq();
  //logerror("Set SIO irq  %01x\n",level);
}

static struct MC6850_interface
BBC_MC6850_calls= {
	0,// Transmit data ouput
	0,// Request to Send output
	Serial_interrupt,// Interupt Request output
};


/***************************************
  BBC 2C199 Serial Interface Cassette
****************************************/

static double last_dev_val = 0;
static int wav_len = 0;
//static int longbit=0;
//static int shortbit = 0;

static int len0=0;
static int len1=0;
static int len2=0;
static int len3=0;

void *bbc_tape_timer;

static void bbc_tape_timer_cb(int param)
{

	double dev_val;
	dev_val=cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0));

	// look for rising edges on the cassette wave
	if (((dev_val>=0.0) && (last_dev_val<0.0)) || ((dev_val<0.0) && (last_dev_val>=0.0)))
	{
		if (wav_len>(9*3))
		{
			//this is to long to recive anything so reset the serial IC. This is a hack, this should be done as a timer in the MC6850 code.
			logerror ("Cassette length %d\n",wav_len);
			MC6850_Reset(wav_len);
			len0=0;
			len1=0;
			len2=0;
			len3=0;
			wav_len=0;

		}

		len3=len2;
		len2=len1;
		len1=len0;
		len0=wav_len;

		wav_len=0;
		logerror ("cassette  %d  %d  %d  %d\n",len3,len2,len1,len0);

		if ((len0+len1)>=(18+18-5))
		{
			/* Clock a 0 onto the serial line */
			logerror("Serial value 0\n");
			MC6850_Receive_Clock(0);
			len0=0;
			len1=0;
			len2=0;
			len3=0;
		}

		if (((len0+len1+len2+len3)<=41) && (len3!=0))
		{
			/* Clock a 1 onto the serial line */
			logerror("Serial value 1\n");
			MC6850_Receive_Clock(1);
			len0=0;
			len1=0;
			len2=0;
			len3=0;
		}


	}

	wav_len++;
	last_dev_val=dev_val;

}

void BBC_Cassette_motor(unsigned char status)
{
	if (status)
	{
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),CASSETTE_MOTOR_ENABLED ,CASSETTE_MASK_MOTOR);
		timer_adjust(bbc_tape_timer, 0, 0, TIME_IN_HZ(44100));
	} else {
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
		timer_reset(bbc_tape_timer, TIME_NEVER);
		MC6850_Reset(wav_len);
		len0=0;
		len1=0;
		len2=0;
		len3=0;
		wav_len=0;
	}
}



WRITE8_HANDLER ( BBC_SerialULA_w )
{
	BBC_Cassette_motor((data & 0x80) >> 7);
}




/**************************************
   load floppy disc
***************************************/

DEVICE_LOAD( bbc_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, 80, 1, 10, 256, 0, 0, FALSE);

		return INIT_PASS;
	}

	return INIT_FAIL;
}


/**************************************
   i8271 disc control function
***************************************/

static int previous_i8271_int_state;

static void	bbc_i8271_interrupt(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpunum_set_input_line(0, INPUT_LINE_NMI,PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}


static i8271_interface bbc_i8271_interface=
{
	bbc_i8271_interrupt,
    NULL
};


READ8_HANDLER( bbc_i8271_read )
{
int ret;
	ret=0x0ff;
	logerror("i8271 read %d  ",offset);
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			ret=i8271_r(offset);
			logerror("  %d\n",ret);
			return ret;
		case 4:
			ret=i8271_data_r(offset);
			logerror("  %d\n",ret);
			return ret;
		default:
			break;
	}
	logerror("  void\n");
	return 0x0ff;
}

WRITE8_HANDLER( bbc_i8271_write )
{
	logerror("i8271 write  %d  %d\n",offset,data);

	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(offset, data);
			return;
		case 4:
			i8271_data_w(offset, data);
			return;
		default:
			break;
	}
}



/**************************************
   WD1770 disc control function
***************************************/

static int  drive_control;
/* bit 0: 1 = drq set, bit 1: 1 = irq set */
static int	bbc_wd177x_drq_irq_state;

static int previous_wd177x_int_state;

/*
   B/ B+ drive control:

        Bit       Meaning
        -----------------
        7,6       Not used.
         5        Reset drive controller chip. (0 = reset controller, 1 = no reset)
         4        Interrupt Enable (0 = enable int, 1 = disable int)
         3        Double density select (0 = double, 1 = single).
         2        Side select (0 = side 0, 1 = side 1).
         1        Drive select 1.
         0        Drive select 0.
*/

/*
density select
single density is as the 8271 disc format
double density is as the 8271 disc format but with 16 sectors per track

At some point we need to check the size of the disc image to work out if it is a single or double
density disc image
*/

static int bbc_1770_IntEnabled;

static void bbc_wd177x_callback(int event)
{
	int state;
	/* wd177x_IRQ_SET and latch bit 4 (nmi_enable) are NAND'ED together
	   wd177x_DRQ_SET and latch bit 4 (nmi_enable) are NAND'ED together
	   the output of the above two NAND gates are then OR'ED together and sent to the 6502 NMI line.
		DRQ and IRQ are active low outputs from wd177x. We use wd177x_DRQ_SET for DRQ = 0,
		and wd177x_DRQ_CLR for DRQ = 1. Similarly wd177x_IRQ_SET for IRQ = 0 and wd177x_IRQ_CLR
		for IRQ = 1.

	  The above means that if IRQ or DRQ are set, a interrupt should be generated.
	  The nmi_enable decides if interrupts are actually triggered.
	  The nmi is edge triggered, and triggers on a +ve edge.
	*/

	logerror("callback $%02X\n", event);


	/* update bbc_wd177x_drq_irq_state depending on event */
	switch (event)
	{
        case WD179X_DRQ_SET:
		{
			bbc_wd177x_drq_irq_state |= 1;
		}
		break;

		case WD179X_DRQ_CLR:
		{
			bbc_wd177x_drq_irq_state &= ~1;
		}
		break;

		case  WD179X_IRQ_SET:
		{
			bbc_wd177x_drq_irq_state |= (1<<1);
		}
		break;

		case WD179X_IRQ_CLR:
		{
			bbc_wd177x_drq_irq_state &= ~(1<<1);
		}
		break;
	}

	/* if drq or irq is set, and interrupt is enabled */
	if (((bbc_wd177x_drq_irq_state & 3)!=0) && (bbc_1770_IntEnabled))
	{
		/* int trigger */
		state = 1;
	}
	else
	{
		/* do not trigger int */
		state = 0;
	}

	/* nmi is edge triggered, and triggers when the state goes from clear->set.
	Here we are checking this transition before triggering the nmi */
	if (state!=previous_wd177x_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpunum_set_input_line(0, INPUT_LINE_NMI,PULSE_LINE);
		}
	}

	previous_wd177x_int_state = state;

}

WRITE8_HANDLER(bbc_wd177x_status_w)
{
	drive_control = data;

	/* set drive */
	if ((data>>0) & 0x01) wd179x_set_drive(0);
	if ((data>>1) & 0x01) wd179x_set_drive(1);

	/* set side */
	wd179x_set_side((data>>2) & 0x01);

	/* set density */
	if ((data>>3) & 0x01)
	{
		/* single density */
		wd179x_set_density(DEN_FM_HI);
	}
	else
	{
		/* double density */
		wd179x_set_density(DEN_MFM_LO);
	}

	bbc_1770_IntEnabled=(((data>>4) & 0x01)==0);

}



READ8_HANDLER ( bbc_wd1770_read )
{
	int retval=0xff;
	switch (offset)
	{
	case 4:
		retval=wd179x_status_r(0);
		break;
	case 5:
		retval=wd179x_track_r(0);
		break;
	case 6:
		retval=wd179x_sector_r(0);
		break;
	case 7:
		retval=wd179x_data_r(0);
		break;
	default:
		break;
	}
	logerror("wd177x read: $%02X  $%02X\n", offset,retval);

	return retval;
}

WRITE8_HANDLER ( bbc_wd1770_write )
{
	logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		bbc_wd177x_status_w(0, data);
		break;
	case 4:
		wd179x_command_w(0, data);
		break;
	case 5:
		wd179x_track_w(0, data);
		break;
	case 6:
		wd179x_sector_w(0, data);
		break;
	case 7:
		wd179x_data_w(0, data);
		break;
	default:
		break;
	}
}


/*********************************************
OPUS CHALLENGER MEMORY MAP
		Read						Write

&FCF8	1770 Status register		1770 command register
&FCF9				1770 track register
&FCFA				1770 sector register
&FCFB				1770 data register
&FCFC								1770 drive control


drive control register bits
0	select side 0= side 0   1= side 1
1   select drive 0
2   select drive 1
3   ?unused?
4   ?Always Set
5   Density Select 0=double, 1=single
6   ?unused?
7   ?unused?

The RAM is accessible through JIM (page &FD). One page is visible in JIM at a time.
The selected page is controlled by the two paging registers:

&FCFE		Paging register MSB
&FCFF       Paging register LSB

256K model has 1024 pages &000 to &3ff
512K model has 2048 pages &000 to &7ff

AM_RANGE(0xfc00, 0xfdff) AM_READWRITE(bbc_opus_read     , bbc_opus_write	)


**********************************************/
static int opusbank;


WRITE8_HANDLER( bbc_opus_status_w )
{
	drive_control = data;

	/* set drive */
	if ((data>>1) & 0x01) wd179x_set_drive(0);
	if ((data>>2) & 0x01) wd179x_set_drive(1);

	/* set side */
	wd179x_set_side((data>>0) & 0x01);

	/* set density */
	if ((data>>5) & 0x01)
	{
		/* single density */
		wd179x_set_density(DEN_FM_HI);
	}
	else
	{
		/* double density */
		wd179x_set_density(DEN_MFM_LO);
	}

	bbc_1770_IntEnabled=(data>>4) & 0x01;

}

READ8_HANDLER( bbc_opus_read )
{
	logerror("wd177x read: $%02X\n", offset);

	if (bbc_DFSType==6)
	{
		if (offset<0x100)
		{
			switch (offset)
			{
				case 0xf8:
					return wd179x_status_r(0);
					break;
				case 0xf9:
					return wd179x_track_r(0);
					break;
				case 0xfa:
					return wd179x_sector_r(0);
					break;
				case 0xfb:
					return wd179x_data_r(0);
					break;
			}

		} else {
			return memory_region(REGION_DISKS)[offset+(opusbank<<8)];
		}
	}
	return 0xff;
}

WRITE8_HANDLER (bbc_opus_write)
{
	logerror("wd177x write: $%02X  $%02X\n", offset,data);

	if (bbc_DFSType==6)
	{
		if (offset<0x100)
		{
			switch (offset)
			{
				case 0xf8:
					wd179x_command_w(0, data);
					break;
				case 0xf9:
					wd179x_track_w(0, data);
					break;
				case 0xfa:
					wd179x_sector_w(0, data);
					break;
				case 0xfb:
					wd179x_data_w(0, data);
					break;
				case 0xfc:
					bbc_opus_status_w(0,data);
					break;
				case 0xfe:
					opusbank=(opusbank & 0xff) | (data<<8);
					break;
				case 0xff:
					opusbank=(opusbank & 0xff00) | data;
					break;
			}
		} else {
			memory_region(REGION_DISKS)[offset+(opusbank<<8)]=data;
		}
	}
}


/***************************************
BBC MASTER DISC SUPPORT
***************************************/


READ8_HANDLER ( bbcm_wd1770_read )
{
	int retval=0xff;
	switch (offset)
	{
	case 0:
		retval=wd179x_status_r(0);
		break;
	case 1:
		retval=wd179x_track_r(0);
		break;
	case 2:
		retval=wd179x_sector_r(0);
		break;
	case 3:
		retval=wd179x_data_r(0);
		break;
	default:
		break;
	}
	return retval;
}


WRITE8_HANDLER ( bbcm_wd1770_write )
{
	//logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		wd179x_command_w(0, data);
		break;
	case 1:
		wd179x_track_w(0, data);
		break;
	case 2:
		wd179x_sector_w(0, data);
		break;
	case 3:
		wd179x_data_w(0, data);
		break;
	default:
		break;
	}
}


READ8_HANDLER ( bbcm_wd1770l_read )
{
	return drive_control;
}

WRITE8_HANDLER ( bbcm_wd1770l_write )
{
	drive_control = data;

	/* set drive */
	if ((data>>0) & 0x01) wd179x_set_drive(0);
	if ((data>>1) & 0x01) wd179x_set_drive(1);

	/* set side */
	wd179x_set_side((data>>4) & 0x01);

	/* set density */
	if ((data>>5) & 0x01)
	{
		/* single density */
		wd179x_set_density(DEN_FM_HI);
	}
	else
	{
		/* double density */
		wd179x_set_density(DEN_MFM_LO);
	}

//	bbc_1770_IntEnabled=(((data>>4) & 0x01)==0);
	bbc_1770_IntEnabled=1;

}


/**************************************
DFS Hardware mapping for different Disc Controller types
***************************************/

READ8_HANDLER( bbc_disc_r )
{
	switch (bbc_DFSType){
	/* case 0 to 3 are all standard 8271 interfaces */
	case 0: case 1: case 2: case 3:
		return bbc_i8271_read(offset);
		break;
	/* case 4 is the acown 1770 interface */
	case 4:
		return bbc_wd1770_read(offset);
		break;
	/* case 5 is the watford 1770 interface */
	case 5:
		return bbc_wd1770_read(offset);
		break;
	/* case 6 is the Opus challenger interface */
	case 6:
		/* not connected here, opus drive is connected via the 1Mhz Bus */
		break;
	/* case 7 in no disc controller */
	case 7:
		break;
	}
	return 0x0ff;
}

WRITE8_HANDLER ( bbc_disc_w )
{
	switch (bbc_DFSType){
	/* case 0 to 3 are all standard 8271 interfaces */
	case 0: case 1: case 2: case 3:
		bbc_i8271_write(offset,data);
		break;
	/* case 4 is the acown 1770 interface */
	case 4:
		bbc_wd1770_write(offset,data);
		break;
	/* case 5 is the watford 1770 interface */
	case 5:
		bbc_wd1770_write(offset,data);
		break;
	/* case 6 is the Opus challenger interface */
	case 6:
		/* not connected here, opus drive is connected via the 1Mhz Bus */
		break;
	/* case 7 in no disc controller */
	case 7:
		break;
	}
}



/**************************************
   BBC B Rom loading functions
***************************************/
DEVICE_LOAD( bbcb_cart )
{
	UINT8 *mem = memory_region (REGION_USER1);
	int size, read_;
	int addr = 0;

	size = image_length (image);

    addr = 0x8000 + (0x4000 * image_index_in_device(image));


	logerror("loading rom %s at %.4x size:%.4x\n",image_filename(image), addr, size);


	switch (size) {
	case 0x2000:
		read_ = image_fread(image, mem + addr, size);
		read_ = image_fread(image, mem + addr + 0x2000, size);
		break;
	case 0x4000:
		read_ = image_fread(image, mem + addr, size);
		break;
	default:
		read_ = 0;
		logerror("bad rom file size of %.4x\n",size);
		break;
	}

	if (read_ != size)
		return 1;
	return 0;
}




/**************************************
   Machine Initialisation functions
***************************************/

DRIVER_INIT( bbc )
{
	bbc_Master=0;
	bbc_tape_timer = timer_alloc(bbc_tape_timer_cb);
}
DRIVER_INIT( bbcm )
{
	bbc_Master=1;
	bbc_tape_timer = timer_alloc(bbc_tape_timer_cb);
    mc146818_init(MC146818_STANDARD);
}

MACHINE_START( bbca )
{
	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);
	return 0;
}

MACHINE_RESET( bbca )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1));
	memory_set_bankptr(3,memory_region(REGION_CPU1));

	memory_set_bankptr(4,memory_region(REGION_USER1));          /* bank 4 is the paged ROMs     from 8000 to bfff */
	memory_set_bankptr(7,memory_region(REGION_USER1)+0x10000);  /* bank 7 points at the OS rom  from c000 to ffff */

	via_reset();

	bbcb_IC32_initialise();

	MC6850_config(&BBC_MC6850_calls);
}

MACHINE_START( bbcb )
{
	//removed from here because MACHINE_START can no longer read DIP swiches.
	//put in MACHINE_RESET instead.
	//bbc_DFSType=  (readinputportbytag("BBCCONFIG")>>0)&0x07;
	//bbc_SWRAMtype=(readinputportbytag("BBCCONFIG")>>3)&0x03;
	//bbc_RAMSize=  (readinputportbytag("BBCCONFIG")>>5)&0x01;

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	/*set up the required disc controller*/
	//switch (bbc_DFSType) {
	//case 0:	case 1: case 2: case 3:
		previous_i8271_int_state=0;
		i8271_init(&bbc_i8271_interface);
	//	break;
	//case 4: case 5: case 6:
		previous_wd177x_int_state=1;
	    wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
	//	break;
	//}
	return 0;
}

MACHINE_RESET( bbcb )
{
	bbc_DFSType=  (readinputportbytag("BBCCONFIG")>>0)&0x07;
	bbc_SWRAMtype=(readinputportbytag("BBCCONFIG")>>3)&0x03;
	bbc_RAMSize=  (readinputportbytag("BBCCONFIG")>>5)&0x01;

	memory_set_bankptr(1,memory_region(REGION_CPU1));
	if (bbc_RAMSize)
	{
		/* 32K Model B */
		memory_set_bankptr(3,memory_region(REGION_CPU1)+0x4000);
		set_video_memory_lookups(32);
	} else {
		/* 16K just repeat the lower 16K*/
		memory_set_bankptr(3,memory_region(REGION_CPU1));
		set_video_memory_lookups(16);
	}

	memory_set_bankptr(4,memory_region(REGION_USER1));          /* bank 4 is the paged ROMs     from 8000 to bfff */
	memory_set_bankptr(7,memory_region(REGION_USER1)+0x40000);  /* bank 7 points at the OS rom  from c000 to ffff */

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);
	MC6850_config(&BBC_MC6850_calls);

	opusbank=0;
	/*set up the required disc controller*/
	//switch (bbc_DFSType) {
	//case 0:	case 1: case 2: case 3:
		i8271_reset();
	//	break;
	//case 4: case 5: case 6:
	    wd179x_reset();
	//	break;
	//}
}


MACHINE_START( bbcbp )
{
	memory_set_opbase_handler(0, bbcbp_opbase_handler);

	/* bank 6 is the paged ROMs     from b000 to bfff */
	memory_configure_bank(6, 0, 16, memory_region(REGION_USER1) + 0x3000, 1<<14);

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
	return 0;
}

MACHINE_RESET( bbcbp )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1));
	memory_set_bankptr(2,memory_region(REGION_CPU1)+0x03000);  /* bank 2 screen/shadow ram     from 3000 to 7fff */
	memory_set_bankptr(4,memory_region(REGION_USER1));         /* bank 4 is paged ROM or RAM   from 8000 to afff */
	memory_set_bank(6, 0);
	memory_set_bankptr(7,memory_region(REGION_USER1)+0x40000); /* bank 7 points at the OS rom  from c000 to ffff */

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);
	MC6850_config(&BBC_MC6850_calls);

	previous_wd177x_int_state=1;
    wd179x_reset();
}



MACHINE_START( bbcm )
{
	memory_set_opbase_handler(0, bbcm_opbase_handler);

	/* bank 5 is the paged ROMs     from 9000 to bfff */
	memory_configure_bank(5, 0, 16, memory_region(REGION_USER1)+0x01000, 1<<14);

	via_config(0, &bbcb_system_via);
	via_set_clock(0,1000000);

	via_config(1, &bbcb_user_via);
	via_set_clock(1,1000000);

	wd179x_init(WD_TYPE_177X,bbc_wd177x_callback);
	return 0;
}

MACHINE_RESET( bbcm )
{
	memory_set_bankptr(1,memory_region(REGION_CPU1));			/* bank 1 regular lower ram		from 0000 to 2fff */
	memory_set_bankptr(2,memory_region(REGION_CPU1)+0x3000);	/* bank 2 screen/shadow ram		from 3000 to 7fff */
	memory_set_bankptr(4,memory_region(REGION_USER1));         /* bank 4 is paged ROM or RAM   from 8000 to 8fff */
	memory_set_bank(5, 0);
	memory_set_bankptr(7,memory_region(REGION_USER1)+0x40000); /* bank 6 OS rom of RAM			from c000 to dfff */

	via_reset();

	bbcb_IC32_initialise();

	uPD7002_config(&BBC_uPD7002);
	MC6850_config(&BBC_MC6850_calls);

	previous_wd177x_int_state=1;
    wd179x_reset();
}
