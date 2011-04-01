/******************************************************************************
    BBC Model B

    MESS Driver By:

    Gordon Jefferyes
    mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

#include <ctype.h>
#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/sn76496.h"
#include "sound/tms5220.h"
#include "machine/6522via.h"
#include "machine/wd17xx.h"
#include "imagedev/flopdrv.h"
#include "includes/bbc.h"
#include "machine/upd7002.h"
#include "machine/i8271.h"
#include "machine/mc146818.h"
#include "machine/ctronics.h"
#include "imagedev/cassette.h"

/* BBC Memory Size */
/* this stores the DIP switch setting for the DFS type being used */
/* this stores the DIP switch setting for the SWRAM type being used */

/* if 0 then we are emulating a BBC B style machine
   if 1 then we are emulating a BBC Master style machine */


/****************************
 IRQ inputs
*****************************/


/************************
Sideways RAM/ROM code
*************************/

//This is the latch that holds the sideways ROM bank to read

//This stores the sideways RAM latch type.
//Acorn and others use the bbc_rombank latch to select the write bank to be used.(type 0)
//Solidisc use the BBC's userport to select the write bank to be used (type 1)



/*************************
Model A memory handling functions
*************************/

/* for the model A just address the 4 on board ROM sockets */
WRITE8_HANDLER ( bbc_page_selecta_w )
{
	memory_set_bankptr(space->machine(),"bank4",space->machine().region("user1")->base()+((data&0x03)<<14));
}


WRITE8_HANDLER ( bbc_memorya1_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	space->machine().region("maincpu")->base()[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	state->m_vidmem[offset]=1;
}

/*************************
Model B memory handling functions
*************************/

/* the model B address all 16 of the ROM sockets */
/* I have set bank 1 as a special case to load different DFS roms selectable from MESS's DIP settings var:bbc_DFSTypes */
WRITE8_HANDLER ( bbc_page_selectb_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	state->m_rombank=data&0x0f;
	if (state->m_rombank!=1)
	{
		memory_set_bankptr(space->machine(), "bank4", space->machine().region("user1")->base() + (state->m_rombank << 14));
	}
	else
	{
		memory_set_bankptr(space->machine(), "bank4", space->machine().region("user2")->base() + ((state->m_DFSType) << 14));
	}
}


WRITE8_HANDLER ( bbc_memoryb3_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_RAMSize)
	{
		space->machine().region("maincpu")->base()[offset + 0x4000] = data;
		// this array is set so that the video emulator know which addresses to redraw
		state->m_vidmem[offset + 0x4000] = 1;
	}
	else
	{
		space->machine().region("maincpu")->base()[offset] = data;
		state->m_vidmem[offset] = 1;
	}

}

/* I have setup 3 types of sideways ram:
0: none
1: 128K (bank 8 to 15) Solidisc sidewaysram userport bank latch
2: 64K (banks 4 to 7) for Acorn sideways ram FE30 bank latch
3: 128K (banks 8 to 15) for Acown sideways ram FE30 bank latch
*/
static const unsigned short bbc_SWRAMtype1[16]={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};
static const unsigned short bbc_SWRAMtype2[16]={0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0};
static const unsigned short bbc_SWRAMtype3[16]={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1};

WRITE8_HANDLER ( bbc_memoryb4_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_rombank == 1)
	{
		// special DFS case for Acorn DFS E00 Hack that can write to the DFS RAM Bank;
		if (state->m_DFSType == 3) space->machine().region("user2")->base()[((state->m_DFSType) << 14) + offset] = data;
	} else
	{
		switch (state->m_SWRAMtype)
		{
			case 1:	if (bbc_SWRAMtype1[state->m_userport]) space->machine().region("user1")->base()[(state->m_userport << 14) + offset] = data;
			case 2:	if (bbc_SWRAMtype2[state->m_rombank])  space->machine().region("user1")->base()[(state->m_rombank << 14) + offset] = data;
			case 3:	if (bbc_SWRAMtype3[state->m_rombank])  space->machine().region("user1")->base()[(state->m_rombank << 14) + offset] = data;
		}
	}
}

/****************************************/
/* BBC B Plus memory handling function */
/****************************************/


/*  this function should return true if
    the instruction is in the VDU driver address ranged
    these are set when:
    PC is in the range c000 to dfff
    or if pagedRAM set and PC is in the range a000 to afff
*/
static int vdudriverset(running_machine &machine)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	int PC;
	PC = cpu_get_pc(machine.device("maincpu")); // this needs to be set to the 6502 program counter
	return (((PC >= 0xc000) && (PC <= 0xdfff)) || ((state->m_pagedRAM) && ((PC >= 0xa000) && (PC <= 0xafff))));
}


/* the model B Plus addresses all 16 of the ROM sockets plus the extra 12K of ram at 0x8000
   and 20K of shadow ram at 0x3000 */
WRITE8_HANDLER ( bbc_page_selectbp_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if ((offset&0x04)==0)
	{
		state->m_pagedRAM = (data >> 7) & 0x01;
		state->m_rombank =  data & 0x0f;

		if (state->m_pagedRAM)
		{
			/* if paged ram then set 8000 to afff to read from the ram 8000 to afff */
			memory_set_bankptr(space->machine(), "bank4", space->machine().region("maincpu")->base() + 0x8000);
		}
		else
		{
			/* if paged rom then set the rom to be read from 8000 to afff */
			memory_set_bankptr(space->machine(), "bank4", space->machine().region("user1")->base() + (state->m_rombank << 14));
		};

		/* set the rom to be read from b000 to bfff */
		memory_set_bank(space->machine(), "bank6", state->m_rombank);
	}
	else
	{
		//the video display should now use this flag to display the shadow ram memory
		state->m_vdusel=(data>>7)&0x01;
		bbcbp_setvideoshadow(space->machine(), state->m_vdusel);
		//need to make the video display do a full screen refresh for the new memory area
		memory_set_bankptr(space->machine(), "bank2", space->machine().region("maincpu")->base()+0x3000);
	}
}


/* write to the normal memory from 0x0000 to 0x2fff
   the writes to this memory are just done the normal
   way */

WRITE8_HANDLER ( bbc_memorybp1_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	space->machine().region("maincpu")->base()[offset]=data;

	// this array is set so that the video emulator know which addresses to redraw
	state->m_vidmem[offset]=1;
}





/* the next two function handle reads and write to the shadow video ram area
   between 0x3000 and 0x7fff

   when vdusel is set high the video display uses the shadow ram memory
   the processor only reads and write to the shadow ram when vdusel is set
   and when the instruction being executed is stored in a set range of memory
   addresses known as the VDU driver instructions.
*/


DIRECT_UPDATE_HANDLER( bbcbp_direct_handler )
{
	bbc_state *state = machine->driver_data<bbc_state>();
	UINT8 *ram = machine->region("maincpu")->base();
	if (state->m_vdusel == 0)
	{
		// not in shadow ram mode so just read normal ram
		memory_set_bankptr(*machine, "bank2", ram + 0x3000);
	}
	else
	{
		if (vdudriverset(*machine))
		{
			// if VDUDriver set then read from shadow ram
			memory_set_bankptr(*machine, "bank2", ram + 0xb000);
		}
		else
		{
			// else read from normal ram
			memory_set_bankptr(*machine, "bank2", ram + 0x3000);
		}
	}
	return address;
}


WRITE8_HANDLER ( bbc_memorybp2_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	UINT8 *ram = space->machine().region("maincpu")->base();
	if (state->m_vdusel==0)
	{
		// not in shadow ram mode so just write to normal ram
		ram[offset + 0x3000] = data;
		state->m_vidmem[offset + 0x3000] = 1;
	}
	else
	{
		if (vdudriverset(space->machine()))
		{
			// if VDUDriver set then write to shadow ram
			ram[offset + 0xb000] = data;
			state->m_vidmem[offset + 0xb000] = 1;
		}
		else
		{
			// else write to normal ram
			ram[offset + 0x3000] = data;
			state->m_vidmem[offset + 0x3000] = 1;
		}
	}
}


/* if the pagedRAM is set write to RAM between 0x8000 to 0xafff
otherwise this area contains ROM so no write is required */
WRITE8_HANDLER ( bbc_memorybp4_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_pagedRAM)
	{
		space->machine().region("maincpu")->base()[offset+0x8000]=data;
	}
}


/* the BBC B plus 128K had extra ram mapped in replacing the
rom bank 0,1,c and d.
The function memorybp3_128_w handles memory writes from 0x8000 to 0xafff
which could either be sideways ROM, paged RAM, or sideways RAM.
The function memorybp4_128_w handles memory writes from 0xb000 to 0xbfff
which could either be sideways ROM or sideways RAM */


static const unsigned short bbc_b_plus_sideways_ram_banks[16]={ 1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0 };


WRITE8_HANDLER ( bbc_memorybp4_128_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_pagedRAM)
	{
		space->machine().region("maincpu")->base()[offset+0x8000]=data;
	}
	else
	{
		if (bbc_b_plus_sideways_ram_banks[state->m_rombank])
		{
			space->machine().region("user1")->base()[offset+(state->m_rombank<<14)]=data;
		}
	}
}

WRITE8_HANDLER ( bbc_memorybp6_128_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (bbc_b_plus_sideways_ram_banks[state->m_rombank])
	{
		space->machine().region("user1")->base()[offset+(state->m_rombank<<14)+0x3000]=data;
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

b7 IRR  1=Causes an IRQ to the processor
b6 TST  1=Selects &FC00-&FEFF read from OS-ROM
b5 IFJ  1=Internal 1 MHz bus
        0=External 1MHz bus
b4 ITU  1=Internal Tube
        0=External Tube
b3 Y    1=Read/Write HAZEL &C000-&DFFF RAM
        0=Read/Write ROM &C000-&DFFF OS-ROM
b2 X    1=Read/Write LYNNE
        0=Read/WRITE main memory &3000-&8000
b1 E    1=Causes shadow if VDU code
        0=Main all the time
b0 D    1=Display LYNNE as screen
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

//static int ACCCON_IRR=0; This is defined at the top in the IRQ code

READ8_HANDLER ( bbcm_ACCCON_read )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	logerror("ACCCON read %d\n",offset);
	return state->m_ACCCON;
}

WRITE8_HANDLER ( bbcm_ACCCON_write )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	int tempIRR;
	state->m_ACCCON=data;

	logerror("ACCCON write  %d %d \n",offset,data);

	tempIRR=state->m_ACCCON_IRR;
	state->m_ACCCON_IRR=(data>>7)&1;

	state->m_ACCCON_TST=(data>>6)&1;
	state->m_ACCCON_IFJ=(data>>5)&1;
	state->m_ACCCON_ITU=(data>>4)&1;
	state->m_ACCCON_Y  =(data>>3)&1;
	state->m_ACCCON_X  =(data>>2)&1;
	state->m_ACCCON_E  =(data>>1)&1;
	state->m_ACCCON_D  =(data>>0)&1;

	if (tempIRR!=state->m_ACCCON_IRR)
	{
		cputag_set_input_line(space->machine(), "maincpu", M6502_IRQ_LINE, state->m_ACCCON_IRR);
	}

	if (state->m_ACCCON_Y)
	{
		memory_set_bankptr(space->machine(), "bank7", space->machine().region("maincpu")->base() + 0x9000);
	}
	else
	{
		memory_set_bankptr(space->machine(), "bank7", space->machine().region("user1")->base() + 0x40000);
	}

	bbcbp_setvideoshadow(space->machine(), state->m_ACCCON_D);


	if (state->m_ACCCON_X)
	{
		memory_set_bankptr(space->machine(), "bank2", space->machine().region( "maincpu" )->base() + 0xb000 );
	}
	else
	{
		memory_set_bankptr(space->machine(), "bank2", space->machine().region( "maincpu" )->base() + 0x3000 );
	}

	/* ACCCON_TST controls paging of rom reads in the 0xFC00-0xFEFF reigon */
	/* if 0 the I/O is paged for both reads and writes */
	/* if 1 the the ROM is paged in for reads but writes still go to I/O   */
	if (state->m_ACCCON_TST)
	{
		memory_set_bankptr(space->machine(), "bank8", space->machine().region("user1")->base()+0x43c00);
		space->install_read_bank(0xFC00,0xFEFF,"bank8");
	}
	else
	{
		space->install_legacy_read_handler(0xFC00,0xFEFF,FUNC(bbcm_r));
	}

}


static int bbcm_vdudriverset(running_machine &machine)
{
	int PC;
	PC = cpu_get_pc(machine.device("maincpu"));
	return ((PC >= 0xc000) && (PC <= 0xdfff));
}


static WRITE8_HANDLER ( page_selectbm_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	state->m_pagedRAM = (data & 0x80) >> 7;
	state->m_rombank = data & 0x0f;

	if (state->m_pagedRAM)
	{
		memory_set_bankptr(space->machine(), "bank4", space->machine().region("maincpu")->base() + 0x8000);
		memory_set_bank(space->machine(), "bank5", state->m_rombank);
	}
	else
	{
		memory_set_bankptr(space->machine(), "bank4", space->machine().region("user1")->base() + ((state->m_rombank) << 14));
		memory_set_bank(space->machine(), "bank5", state->m_rombank);
	}
}



WRITE8_HANDLER ( bbc_memorybm1_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	space->machine().region("maincpu")->base()[offset] = data;
	state->m_vidmem[offset] = 1;
}


DIRECT_UPDATE_HANDLER( bbcm_direct_handler )
{
	bbc_state *state = machine->driver_data<bbc_state>();
	if (state->m_ACCCON_X)
	{
		memory_set_bankptr( *machine, "bank2", machine->region( "maincpu" )->base() + 0xb000 );
	}
	else
	{
		if (state->m_ACCCON_E && bbcm_vdudriverset(*machine))
		{
			memory_set_bankptr( *machine, "bank2", machine->region( "maincpu" )->base() + 0xb000 );
		}
		else
		{
			memory_set_bankptr( *machine, "bank2", machine->region( "maincpu" )->base() + 0x3000 );
		}
	}

	return address;
}



WRITE8_HANDLER ( bbc_memorybm2_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	UINT8 *ram = space->machine().region("maincpu")->base();
	if (state->m_ACCCON_X)
	{
		ram[offset + 0xb000] = data;
		state->m_vidmem[offset + 0xb000] = 1;
	}
	else
	{
		if (state->m_ACCCON_E && bbcm_vdudriverset(space->machine()))
		{
			ram[offset + 0xb000] = data;
			state->m_vidmem[offset + 0xb000] = 1;
		}
		else
		{
			ram[offset + 0x3000] = data;
			state->m_vidmem[offset + 0x3000] = 1;
		}
	}
}

static const unsigned short bbc_master_sideways_ram_banks[16]=
{
	0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0
};


WRITE8_HANDLER ( bbc_memorybm4_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_pagedRAM)
	{
		space->machine().region("maincpu")->base()[offset+0x8000]=data;
	}
	else
	{
		if (bbc_master_sideways_ram_banks[state->m_rombank])
		{
			space->machine().region("user1")->base()[offset+(state->m_rombank<<14)]=data;
		}
	}
}


WRITE8_HANDLER ( bbc_memorybm5_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (bbc_master_sideways_ram_banks[state->m_rombank])
	{
		space->machine().region("user1")->base()[offset+(state->m_rombank<<14)+0x1000]=data;
	}
}


WRITE8_HANDLER ( bbc_memorybm7_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	if (state->m_ACCCON_Y)
	{
		space->machine().region("maincpu")->base()[offset+0x9000]=data;
	}
}



/******************************************************************************
&FC00-&FCFF FRED
&FD00-&FDFF JIM
&FE00-&FEFF SHEILA      Read                    Write
&00-&07 6845 CRTC       Video controller        Video Controller         8 ( 2 bytes x  4 )
&08-&0F 6850 ACIA       Serial controller       Serial Controller        8 ( 2 bytes x  4 )
&10-&17 Serial ULA      -                       Serial system chip       8 ( 1 byte  x  8 )
&18-&1F uPD7002         A to D converter        A to D converter         8 ( 4 bytes x  2 )
&20-&23 Video ULA       -                       Video system chip        4 ( 2 bytes x  2 )
&24-&27 FDC Latch       1770 Control latch      1770 Control latch       4 ( 1 byte  x  4 )
&28-&2F 1770 registers  1770 Disc Controller    1170 Disc Controller     8 ( 4 bytes x  2 )
&30-&33 ROMSEL          -                       ROM Select               4 ( 1 byte  x  4 )
&34-&37 ACCCON          ACCCON select reg.      ACCCON select reg        4 ( 1 byte  x  4 )
&38-&3F NC              -                       -
&40-&5F 6522 VIA        SYSTEM VIA              SYSTEM VIA              32 (16 bytes x  2 ) 1MHz
&60-&7F 6522 VIA        USER VIA                USER VIA                32 (16 bytes x  2 ) 1MHz
&80-&9F Int. Modem      Int. Modem              Int Modem
&A0-&BF 68B54 ADLC      ECONET controller       ECONET controller       32 ( 4 bytes x  8 ) 2MHz
&C0-&DF NC              -                       -
&E0-&FF Tube ULA        Tube system interface   Tube system interface   32 (32 bytes x  1 ) 2MHz
******************************************************************************/

READ8_HANDLER ( bbcm_r )
{
	//bbc_state *state = space->machine().driver_data<bbc_state>();
long myo;

	/* Now handled in bbcm_ACCCON_write PHS - 2008-10-11 */
//  if ( state->m_ACCCON_TST )
//  {
//      return space->machine().region("user1")->base()[offset+0x43c00];
//  };

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
		via6522_device *via_0 = space->machine().device<via6522_device>("via6522_0");
		via6522_device *via_1 = space->machine().device<via6522_device>("via6522_1");

		myo=offset-0x200;
		if ((myo>=0x00) && (myo<=0x07)) return bbc_6845_r(space, myo-0x00);		/* Video Controller */
		if ((myo>=0x08) && (myo<=0x0f))
		{
			device_t *acia = space->machine().device("acia6850");

			if ((myo - 0x08) & 1)
				return acia6850_stat_r(acia, 0);
			else
				return acia6850_data_r(acia, 0);
		}
		if ((myo>=0x10) && (myo<=0x17)) return 0xfe;						/* Serial System Chip */
		if ((myo>=0x18) && (myo<=0x1f)) return uPD7002_r(space->machine().device("upd7002"), myo-0x18);			/* A to D converter */
		if ((myo>=0x20) && (myo<=0x23)) return 0xfe;						/* VideoULA */
		if ((myo>=0x24) && (myo<=0x27)) return bbcm_wd1770l_read(space, myo-0x24); /* 1770 */
		if ((myo>=0x28) && (myo<=0x2f)) return bbcm_wd1770_read(space, myo-0x28);  /* disc control latch */
		if ((myo>=0x30) && (myo<=0x33)) return 0xfe;						/* page select */
		if ((myo>=0x34) && (myo<=0x37)) return bbcm_ACCCON_read(space, myo-0x34);	/* ACCCON */
		if ((myo>=0x38) && (myo<=0x3f)) return 0xfe;						/* NC ?? */
		if ((myo>=0x40) && (myo<=0x5f)) return via_0->read(*space,myo-0x40);
		if ((myo>=0x60) && (myo<=0x7f)) return via_1->read(*space,myo-0x60);
		if ((myo>=0x80) && (myo<=0x9f)) return 0xfe;
		if ((myo>=0xa0) && (myo<=0xbf)) return 0xfe;
		if ((myo>=0xc0) && (myo<=0xdf)) return 0xfe;
		if ((myo>=0xe0) && (myo<=0xff)) return 0xfe;
	}

	return 0xfe;
}

WRITE8_HANDLER ( bbcm_w )
{
	//bbc_state *state = space->machine().driver_data<bbc_state>();
long myo;

	if ((offset>=0x200) && (offset<=0x2ff)) /* SHEILA */
	{
		via6522_device *via_0 = space->machine().device<via6522_device>("via6522_0");
		via6522_device *via_1 = space->machine().device<via6522_device>("via6522_1");

		myo=offset-0x200;
		if ((myo>=0x00) && (myo<=0x07)) bbc_6845_w(space, myo-0x00,data);			/* Video Controller */
		if ((myo>=0x08) && (myo<=0x0f))
		{
			device_t *acia = space->machine().device("acia6850");

			if ((myo - 0x08) & 1)
				acia6850_ctrl_w(acia, 0, data);
			else
				acia6850_data_w(acia, 0, data);
		}
		if ((myo>=0x10) && (myo<=0x17)) bbc_SerialULA_w(space, myo-0x10,data);		/* Serial System Chip */
		if ((myo>=0x18) && (myo<=0x1f)) uPD7002_w(space->machine().device("upd7002"),myo-0x18,data);			/* A to D converter */
		if ((myo>=0x20) && (myo<=0x23)) bbc_videoULA_w(space, myo-0x20,data);			/* VideoULA */
		if ((myo>=0x24) && (myo<=0x27)) bbcm_wd1770l_write(space, myo-0x24,data);	/* 1770 */
		if ((myo>=0x28) && (myo<=0x2f)) bbcm_wd1770_write(space, myo-0x28,data);	/* disc control latch */
		if ((myo>=0x30) && (myo<=0x33)) page_selectbm_w(space, myo-0x30,data);		/* page select */
		if ((myo>=0x34) && (myo<=0x37)) bbcm_ACCCON_write(space, myo-0x34,data);	/* ACCCON */
		//if ((myo>=0x38) && (myo<=0x3f))                                   /* NC ?? */
		if ((myo>=0x40) && (myo<=0x5f)) via_0->write(*space,myo-0x40, data);
		if ((myo>=0x60) && (myo<=0x7f)) via_1->write(*space,myo-0x60, data);
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
PortA   Keyboard
D0  Pin 8
D1  Pin 9
D2  Pin 10
D3  Pin 11
D4  Pin 5
D5  Pin 6
D6  Pin 7
D7  Pin 12

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
Mode    Size    Start of screen  Number to add  B5      B4
0,1,2   20K &3000        12K        1       1
3       16K &4000        16K        0   0
4,5     10K &5800 (or &1800) 22K        1   0
6       8K  &6000 (or &2000) 24K        0   1
B6 - Operates the CAPS lock LED  (Pin 17 keyboard connector)
B7 - Operates the SHIFT lock LED (Pin 16 keyboard connector)


******************************************************************************/







// this is a counter on the keyboard


INTERRUPT_GEN( bbcb_keyscan )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	static const char *const colnames[] = {
		"COL0", "COL1", "COL2", "COL3", "COL4",
		"COL5", "COL6", "COL7", "COL8", "COL9"
	};
	via6522_device *via_0 = device->machine().device<via6522_device>("via6522_0");

	/* only do auto scan if keyboard is not enabled */
	if (state->m_b3_keyboard == 1)
	{
		/* KBD IC1 4 bit addressable counter */
		/* KBD IC3 4 to 10 line decoder */
		/* keyboard not enabled so increment counter */
		state->m_column = (state->m_column + 1) % 16;
		if (state->m_column < 10)
		{
			/* KBD IC4 8 input NAND gate */
			/* set the value of via_system ca2, by checking for any keys
                 being pressed on the selected state->m_column */
			if ((input_port_read(device->machine(), colnames[state->m_column]) | 0x01) != 0xff)
			{
				via_0->write_ca2(1);
			}
			else
			{
				via_0->write_ca2(0);
			}

		}
		else
		{
			via_0->write_ca2(0);
		}
	}
}


INTERRUPT_GEN( bbcm_keyscan )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	static const char *const colnames[] = {
		"COL0", "COL1", "COL2", "COL3", "COL4",
		"COL5", "COL6", "COL7", "COL8", "COL9"
	};
	via6522_device *via_0 = device->machine().device<via6522_device>("via6522_0");

	/* only do auto scan if keyboard is not enabled */
	if (state->m_b3_keyboard == 1)
	{
		/* KBD IC1 4 bit addressable counter */
		/* KBD IC3 4 to 10 line decoder */
		/* keyboard not enabled so increment counter */
		state->m_column = (state->m_column + 1) % 16;

		/* this IF should be removed as soon as the dip switches (keyboard keys) are set for the master */
		if (state->m_column < 10)
		{
			/* KBD IC4 8 input NAND gate */
			/* set the value of via_system ca2, by checking for any keys
                 being pressed on the selected state->m_column */
			if ((input_port_read(device->machine(), colnames[state->m_column]) | 0x01) != 0xff)
			{
				via_0->write_ca2(1);
			}
			else
			{
				via_0->write_ca2(0);
			}

		}
		else
		{
			via_0->write_ca2(0);
		}
	}
}



static int bbc_keyboard(address_space *space, int data)
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	int bit;
	int row;
	int res;
	static const char *const colnames[] = {
		"COL0", "COL1", "COL2", "COL3", "COL4",
		"COL5", "COL6", "COL7", "COL8", "COL9"
	};
	via6522_device *via_0 = space->machine().device<via6522_device>("via6522_0");

	state->m_column = data & 0x0f;
	row = (data>>4) & 0x07;

	bit = 0;

	if (state->m_column < 10)
	{
		res = input_port_read(space->machine(), colnames[state->m_column]);
	}
	else
	{
		res = 0xff;
	}

	/* Normal keyboard result */
	if ((res & (1<<row)) == 0)
	{
		bit = 1;
	}

	if ((res | 1) != 0xff)
	{
		via_0->write_ca2(1);
	}
	else
	{
		via_0->write_ca2(0);
	}

	return (data & 0x7f) | (bit<<7);
}


static void bbcb_IC32_initialise(bbc_state *state)
{
	state->m_b0_sound=0x01;				// Sound is negative edge trigered
	state->m_b1_speech_read=0x01;		// ????
	state->m_b2_speech_write=0x01;		// ????
	state->m_b3_keyboard=0x01;			// Keyboard is negative edge trigered
	state->m_b4_video0=0x01;
	state->m_b5_video1=0x01;
	state->m_b6_caps_lock_led=0x01;
	state->m_b7_shift_lock_led=0x01;

}


/* This the BBC Masters Real Time Clock and NVRam IC */
static void MC146818_set(address_space *space)
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	logerror ("146181 WR=%d DS=%d AS=%d CE=%d \n",state->m_MC146818_WR,state->m_MC146818_DS,state->m_MC146818_AS,state->m_MC146818_CE);
	mc146818_device *rtc = space->machine().device<mc146818_device>("rtc");

	// if chip enabled
	if (state->m_MC146818_CE)
	{
		// if data select is set then access the data in the 146818
		if (state->m_MC146818_DS)
		{
			if (state->m_MC146818_WR)
			{
				state->m_via_system_porta=rtc->read(*space, 1);
				//logerror("read 146818 data %d \n",state->m_via_system_porta);
			}
			else
			{
				rtc->write(*space, 1, state->m_via_system_porta);
				//logerror("write 146818 data %d \n",state->m_via_system_porta);
			}
		}

		// if address select is set then set the address in the 146818
		if (state->m_MC146818_AS)
		{
			rtc->write(*space, 0, state->m_via_system_porta);
			//logerror("write 146818 address %d \n",state->m_via_system_porta);
		}
	}
}


static WRITE8_DEVICE_HANDLER( bbcb_via_system_write_porta )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	address_space *space = device->machine().device("maincpu")->memory().space(AS_PROGRAM);
	//logerror("SYSTEM write porta %d\n",data);

	state->m_via_system_porta = data;
	if (state->m_b0_sound == 0)
	{
		//logerror("Doing an unsafe write to the sound chip %d \n",data);
		sn76496_w(device->machine().device("sn76489"), 0,state->m_via_system_porta);
	}
	if (state->m_b3_keyboard == 0)
	{
		//logerror("Doing an unsafe write to the keyboard %d \n",data);
		state->m_via_system_porta = bbc_keyboard(space, state->m_via_system_porta);
	}
	if (state->m_Master) MC146818_set(space);
}


static WRITE8_DEVICE_HANDLER( bbcb_via_system_write_portb )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	address_space *space = device->machine().device("maincpu")->memory().space(AS_PROGRAM);
	int bit, value;
	bit = data & 0x07;
	value = (data >> 3) & 0x01;


    //logerror("SYSTEM write portb %d %d %d\n",data,bit,value);

	if (value)
	{
		switch (bit)
		{
		case 0:
			if (state->m_b0_sound == 0)
			{
				state->m_b0_sound = 1;
			}
			break;
		case 1:
			if (state->m_Master)
			{
				if (state->m_MC146818_WR == 0)
				{
					/* BBC MASTER has NV RAM Here */
					state->m_MC146818_WR = 1;
					MC146818_set(space);
				}
			}
			else
			{
				if (state->m_b1_speech_read == 0)
				{
					/* VSP TMS 5220 */
					state->m_b1_speech_read = 1;
				}
			}
			break;
		case 2:
			if (state->m_Master)
			{
				if (state->m_MC146818_DS == 0)
				{
					/* BBC MASTER has NV RAM Here */
					state->m_MC146818_DS = 1;
					MC146818_set(space);
				}
			}
			else
			{
				if (state->m_b2_speech_write == 0)
				{
					/* VSP TMS 5220 */
					state->m_b2_speech_write = 1;
				}
			}
			break;
		case 3:
			if (state->m_b3_keyboard == 0)
			{
				state->m_b3_keyboard = 1;
			}
			break;
		case 4:
			if (state->m_b4_video0 == 0)
			{
				state->m_b4_video0 = 1;
				bbc_setscreenstart(device->machine(), state->m_b4_video0, state->m_b5_video1);
			}
			break;
		case 5:
			if (state->m_b5_video1 == 0)
			{
				state->m_b5_video1 = 1;
				bbc_setscreenstart(device->machine(), state->m_b4_video0, state->m_b5_video1);
			}
			break;
		case 6:
			if (state->m_b6_caps_lock_led == 0)
			{
				state->m_b6_caps_lock_led = 1;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (state->m_b7_shift_lock_led == 0)
			{
				state->m_b7_shift_lock_led = 1;
				/* call shift lock led update */
			}
			break;
		}
	}
	else
	{
		switch (bit)
		{
		case 0:
			if (state->m_b0_sound == 1)
			{
				state->m_b0_sound = 0;
				sn76496_w(space->machine().device("sn76489"), 0, state->m_via_system_porta);
			}
			break;
		case 1:
			if (state->m_Master)
			{
				if (state->m_MC146818_WR == 1)
				{
					/* BBC MASTER has NV RAM Here */
					state->m_MC146818_WR = 0;
					MC146818_set(space);
				}
			}
			else
			{
				if (state->m_b1_speech_read == 1)
				{
					/* VSP TMS 5220 */
					state->m_b1_speech_read = 0;
				}
			}
			break;
		case 2:
			if (state->m_Master)
			{
				if (state->m_MC146818_DS == 1)
				{
					/* BBC MASTER has NV RAM Here */
					state->m_MC146818_DS = 0;
					MC146818_set(space);
				}
			}
			else
			{
				if (state->m_b2_speech_write == 1)
				{
					/* VSP TMS 5220 */
					state->m_b2_speech_write = 0;
				}
			}
			break;
		case 3:
			if (state->m_b3_keyboard == 1)
			{
				state->m_b3_keyboard = 0;
				/* *** call keyboard enabled *** */
				state->m_via_system_porta=bbc_keyboard(space, state->m_via_system_porta);
			}
			break;
		case 4:
			if (state->m_b4_video0 == 1)
			{
				state->m_b4_video0 = 0;
				bbc_setscreenstart(device->machine(), state->m_b4_video0, state->m_b5_video1);
			}
			break;
		case 5:
			if (state->m_b5_video1 == 1)
			{
				state->m_b5_video1 = 0;
				bbc_setscreenstart(device->machine(), state->m_b4_video0, state->m_b5_video1);
			}
			break;
		case 6:
			if (state->m_b6_caps_lock_led == 1)
			{
				state->m_b6_caps_lock_led = 0;
				/* call caps lock led update */
			}
			break;
		case 7:
			if (state->m_b7_shift_lock_led == 1)
			{
				state->m_b7_shift_lock_led = 0;
				/* call shift lock led update */
			}
			break;
		}
	}



	if (state->m_Master)
	{
		//set the Address Select
		if (state->m_MC146818_AS != ((data>>7)&1))
		{
			state->m_MC146818_AS=(data>>7)&1;
			MC146818_set(space);
		}

		//if CE changes
		if (state->m_MC146818_CE != ((data>>6)&1))
		{
			state->m_MC146818_CE=(data>>6)&1;
			MC146818_set(space);
		}
	}
}


static READ8_DEVICE_HANDLER( bbcb_via_system_read_porta )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	//logerror("SYSTEM read porta %d\n",state->m_via_system_porta);
	return state->m_via_system_porta;
}

// D4 of portb is joystick fire button 1
// D5 of portb is joystick fire button 2
// D6 VSPINT
// D7 VSPRDY

/* this is the interupt and ready signal from the BBC B Speech processor */
static const int TMSint=1;
static const int TMSrdy=1;

#ifdef UNUSED_FUNCTION
void bbc_TMSint(int status)
{
	TMSint=(!status)&1;
	TMSrdy=(!tms5220_readyq_r())&1;
	via_0_portb_w(0,(0xf | input_port_read(machine, "IN0")|(TMSint<<6)|(TMSrdy<<7)));
}
#endif


static READ8_DEVICE_HANDLER( bbcb_via_system_read_portb )
{
	//TMSint=(!tms5220_int_r())&1;
	//TMSrdy=(!tms5220_readyq_r())&1;

	//logerror("SYSTEM read portb %d\n",0xf | input_port(machine, "IN0")|(TMSint<<6)|(TMSrdy<<7));

	return (0xf | input_port_read(device->machine(), "IN0")|(TMSint<<6)|(TMSrdy<<7));
}


/* vertical sync pulse from video circuit */
static READ8_DEVICE_HANDLER( bbcb_via_system_read_ca1 )
{
	return 0x01;
}


/* joystick EOC */
static READ8_DEVICE_HANDLER( bbcb_via_system_read_cb1 )
{
	return uPD7002_EOC_r(device->machine().device("upd7002"),0);
}


/* keyboard pressed detect */
static READ8_DEVICE_HANDLER( bbcb_via_system_read_ca2 )
{
	return 0x01;
}


/* light pen strobe detect (not emulated) */
static READ8_DEVICE_HANDLER( bbcb_via_system_read_cb2 )
{
	return 0x01;
}


const via6522_interface bbcb_system_via =
{
	DEVCB_HANDLER(bbcb_via_system_read_porta),
	DEVCB_HANDLER(bbcb_via_system_read_portb),
	DEVCB_HANDLER(bbcb_via_system_read_ca1),
	DEVCB_HANDLER(bbcb_via_system_read_cb1),
	DEVCB_HANDLER(bbcb_via_system_read_ca2),
	DEVCB_HANDLER(bbcb_via_system_read_cb2),
	DEVCB_HANDLER(bbcb_via_system_write_porta),
	DEVCB_HANDLER(bbcb_via_system_write_portb),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE("maincpu", M6502_IRQ_LINE)
};


/**********************************************************************
USER VIA
Port A output is buffered before being connected to the printer connector.
This means that they can only be operated as output lines.
CA1 is pulled high by a 4K7 resistor. CA1 normally acts as an acknowledge
line when a printer is used. CA2 is buffered so that it has become an open
collector output only. It usially acts as the printer strobe line.
***********************************************************************/

/* USER VIA 6522 port B is connected to the BBC user port */
static READ8_DEVICE_HANDLER( bbcb_via_user_read_portb )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( bbcb_via_user_write_portb )
{
	bbc_state *state = device->machine().driver_data<bbc_state>();
	state->m_userport = data;
}

const via6522_interface bbcb_user_via =
{
	DEVCB_NULL,	//via_user_read_porta,
	DEVCB_HANDLER(bbcb_via_user_read_portb),
	DEVCB_NULL,	//via_user_read_ca1,
	DEVCB_NULL,	//via_user_read_cb1,
	DEVCB_NULL,	//via_user_read_ca2,
	DEVCB_NULL,	//via_user_read_cb2,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_HANDLER(bbcb_via_user_write_portb),
	DEVCB_NULL, //via_user_write_ca1
	DEVCB_NULL, //via_user_write_cb1
	DEVCB_DEVICE_LINE("centronics", centronics_strobe_w),
	DEVCB_NULL,	//via_user_write_cb2,
	DEVCB_CPU_INPUT_LINE("maincpu", M6502_IRQ_LINE)
};


/**************************************
BBC Joystick Support
**************************************/

static UPD7002_GET_ANALOGUE(BBC_get_analogue_input)
{
	switch(channel_number)
	{
		case 0:
			return ((0xff-input_port_read(device->machine(), "JOY0"))<<8);
		case 1:
			return ((0xff-input_port_read(device->machine(), "JOY1"))<<8);
		case 2:
			return ((0xff-input_port_read(device->machine(), "JOY2"))<<8);
		case 3:
			return ((0xff-input_port_read(device->machine(), "JOY3"))<<8);
	}

	return 0;
}

static UPD7002_EOC(BBC_uPD7002_EOC)
{
	via6522_device *via_0 = device->machine().device<via6522_device>("via6522_0");
	via_0->write_cb1(data);
}

const uPD7002_interface bbc_uPD7002 =
{
	BBC_get_analogue_input,
	BBC_uPD7002_EOC
};


/***************************************
  BBC 2C199 Serial Interface Cassette
****************************************/

//static int longbit=0;
//static int shortbit = 0;



static void MC6850_Receive_Clock(running_machine &machine, int new_clock)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	if (!state->m_mc6850_clock && new_clock)
	{
		device_t *acia = machine.device("acia6850");
		acia6850_tx_clock_in(acia);
	}
	state->m_mc6850_clock = new_clock;
}

static TIMER_CALLBACK(bbc_tape_timer_cb)
{
	bbc_state *state = machine.driver_data<bbc_state>();

	double dev_val;
	dev_val=cassette_input(machine.device("cassette"));

	// look for rising edges on the cassette wave
	if (((dev_val>=0.0) && (state->m_last_dev_val<0.0)) || ((dev_val<0.0) && (state->m_last_dev_val>=0.0)))
	{
		if (state->m_wav_len>(9*3))
		{
			//this is to long to recive anything so reset the serial IC. This is a hack, this should be done as a timer in the MC6850 code.
			logerror ("Cassette length %d\n",state->m_wav_len);
			state->m_len0=0;
			state->m_len1=0;
			state->m_len2=0;
			state->m_len3=0;
			state->m_wav_len=0;

		}

		state->m_len3=state->m_len2;
		state->m_len2=state->m_len1;
		state->m_len1=state->m_len0;
		state->m_len0=state->m_wav_len;

		state->m_wav_len=0;
		logerror ("cassette  %d  %d  %d  %d\n",state->m_len3,state->m_len2,state->m_len1,state->m_len0);

		if ((state->m_len0+state->m_len1)>=(18+18-5))
		{
			/* Clock a 0 onto the serial line */
			logerror("Serial value 0\n");
			MC6850_Receive_Clock(machine, 0);
			state->m_len0=0;
			state->m_len1=0;
			state->m_len2=0;
			state->m_len3=0;
		}

		if (((state->m_len0+state->m_len1+state->m_len2+state->m_len3)<=41) && (state->m_len3!=0))
		{
			/* Clock a 1 onto the serial line */
			logerror("Serial value 1\n");
			MC6850_Receive_Clock(machine, 1);
			state->m_len0=0;
			state->m_len1=0;
			state->m_len2=0;
			state->m_len3=0;
		}


	}

	state->m_wav_len++;
	state->m_last_dev_val=dev_val;

}

static void BBC_Cassette_motor(running_machine &machine, unsigned char status)
{
	bbc_state *state = machine.driver_data<bbc_state>();
	if (status)
	{
		cassette_change_state(machine.device("cassette"), CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		state->m_tape_timer->adjust(attotime::zero, 0, attotime::from_hz(44100));
	}
	else
	{
		cassette_change_state(machine.device("cassette"), CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		state->m_tape_timer->reset();
		state->m_len0 = 0;
		state->m_len1 = 0;
		state->m_len2 = 0;
		state->m_len3 = 0;
		state->m_wav_len = 0;
	}
}



WRITE8_HANDLER ( bbc_SerialULA_w )
{
	BBC_Cassette_motor(space->machine(), (data & 0x80) >> 7);
}

/**************************************
   i8271 disc control function
***************************************/


static void	bbc_i8271_interrupt(device_t *device, int state)
{
	bbc_state *drvstate = device->machine().driver_data<bbc_state>();
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
    the nmi will be triggered, but when the state changes because the int
    is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=drvstate->m_previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
            it would clear - to be checked */
			device_set_input_line(device->machine().device("maincpu"), INPUT_LINE_NMI,PULSE_LINE);
		}
	}

	drvstate->m_previous_i8271_int_state = state;
}


const i8271_interface bbc_i8271_interface=
{
	bbc_i8271_interrupt,
    NULL,
	{FLOPPY_0, FLOPPY_1}
};


static READ8_HANDLER( bbc_i8271_read )
{
	int ret;
	device_t *i8271 = space->machine().device("i8271");
	ret=0x0ff;
	logerror("i8271 read %d  ",offset);
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			ret=i8271_r(i8271, offset);
			logerror("  %d\n",ret);
			return ret;
		case 4:
			ret=i8271_data_r(i8271, offset);
			logerror("  %d\n",ret);
			return ret;
		default:
			break;
	}
	logerror("  void\n");
	return 0x0ff;
}

static WRITE8_HANDLER( bbc_i8271_write )
{
	device_t *i8271 = space->machine().device("i8271");
	logerror("i8271 write  %d  %d\n",offset,data);

	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(i8271, offset, data);
			return;
		case 4:
			i8271_data_w(i8271, offset, data);
			return;
		default:
			break;
	}
}



/**************************************
   WD1770 disc control function
***************************************/




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

static void bbc_update_fdq_int(running_machine &machine, int state)
{
	bbc_state *drvstate = machine.driver_data<bbc_state>();
	int bbc_state;

	/* if drq or irq is set, and interrupt is enabled */
	if ((drvstate->m_wd177x_irq_state || drvstate->m_wd177x_drq_state) && (drvstate->m_1770_IntEnabled))
	{
		/* int trigger */
		bbc_state = 1;
	}
	else
	{
		/* do not trigger int */
		bbc_state = 0;
	}

	/* nmi is edge triggered, and triggers when the state goes from clear->set.
    Here we are checking this transition before triggering the nmi */
	if (bbc_state!=drvstate->m_previous_wd177x_int_state)
	{
		if (bbc_state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
            it would clear - to be checked */
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI,PULSE_LINE);
		}
	}

	drvstate->m_previous_wd177x_int_state = bbc_state;
}

static WRITE_LINE_DEVICE_HANDLER( bbc_wd177x_intrq_w )
{
	bbc_state *drvstate = device->machine().driver_data<bbc_state>();
	drvstate->m_wd177x_irq_state = state;
	bbc_update_fdq_int(device->machine(), state);
}

static WRITE_LINE_DEVICE_HANDLER( bbc_wd177x_drq_w )
{
	bbc_state *drvstate = device->machine().driver_data<bbc_state>();
	drvstate->m_wd177x_drq_state = state;
	bbc_update_fdq_int(device->machine(), state);
}

const wd17xx_interface bbc_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(bbc_wd177x_intrq_w),
	DEVCB_LINE(bbc_wd177x_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static WRITE8_HANDLER(bbc_wd177x_status_w)
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	device_t *fdc = space->machine().device("wd177x");
	state->m_drive_control = data;

	/* set drive */
	if ((data>>0) & 0x01) wd17xx_set_drive(fdc,0);
	if ((data>>1) & 0x01) wd17xx_set_drive(fdc,1);

	/* set side */
	wd17xx_set_side(fdc,(data>>2) & 0x01);

	/* set density */
	wd17xx_dden_w(fdc, BIT(data, 3));

	state->m_1770_IntEnabled=(((data>>4) & 0x01)==0);

}



READ8_HANDLER ( bbc_wd1770_read )
{
	int retval=0xff;
	device_t *fdc = space->machine().device("wd177x");
	switch (offset)
	{
	case 4:
		retval=wd17xx_status_r(fdc, 0);
		break;
	case 5:
		retval=wd17xx_track_r(fdc, 0);
		break;
	case 6:
		retval=wd17xx_sector_r(fdc, 0);
		break;
	case 7:
		retval=wd17xx_data_r(fdc, 0);
		break;
	default:
		break;
	}
	logerror("wd177x read: $%02X  $%02X\n", offset,retval);

	return retval;
}

WRITE8_HANDLER ( bbc_wd1770_write )
{
	device_t *fdc = space->machine().device("wd177x");
	logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		bbc_wd177x_status_w(space, 0, data);
		break;
	case 4:
		wd17xx_command_w(fdc, 0, data);
		break;
	case 5:
		wd17xx_track_w(fdc, 0, data);
		break;
	case 6:
		wd17xx_sector_w(fdc, 0, data);
		break;
	case 7:
		wd17xx_data_w(fdc, 0, data);
		break;
	default:
		break;
	}
}


/*********************************************
OPUS CHALLENGER MEMORY MAP
        Read                        Write

&FCF8   1770 Status register        1770 command register
&FCF9               1770 track register
&FCFA               1770 sector register
&FCFB               1770 data register
&FCFC                               1770 drive control


drive control register bits
0   select side 0= side 0   1= side 1
1   select drive 0
2   select drive 1
3   ?unused?
4   ?Always Set
5   Density Select 0=double, 1=single
6   ?unused?
7   ?unused?

The RAM is accessible through JIM (page &FD). One page is visible in JIM at a time.
The selected page is controlled by the two paging registers:

&FCFE       Paging register MSB
&FCFF       Paging register LSB

256K model has 1024 pages &000 to &3ff
512K model has 2048 pages &000 to &7ff

AM_RANGE(0xfc00, 0xfdff) AM_READWRITE(bbc_opus_read     , bbc_opus_write    )


**********************************************/


static WRITE8_HANDLER( bbc_opus_status_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	device_t *fdc = space->machine().device("wd177x");
	state->m_drive_control = data;

	/* set drive */
	if ((data>>1) & 0x01) wd17xx_set_drive(fdc,0);
	if ((data>>2) & 0x01) wd17xx_set_drive(fdc,1);

	/* set side */
	wd17xx_set_side(fdc,(data>>0) & 0x01);

	/* set density */
	wd17xx_dden_w(fdc, BIT(data, 5));

	state->m_1770_IntEnabled=(data>>4) & 0x01;

}

READ8_HANDLER( bbc_opus_read )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	device_t *fdc = space->machine().device("wd177x");
	logerror("wd177x read: $%02X\n", offset);

	if (state->m_DFSType==6)
	{
		if (offset<0x100)
		{
			switch (offset)
			{
				case 0xf8:
					return wd17xx_status_r(fdc, 0);
				case 0xf9:
					return wd17xx_track_r(fdc, 0);
				case 0xfa:
					return wd17xx_sector_r(fdc, 0);
				case 0xfb:
					return wd17xx_data_r(fdc, 0);
			}

		}
		else
		{
			return space->machine().region("disks")->base()[offset + (state->m_opusbank << 8)];
		}
	}
	return 0xff;
}

WRITE8_HANDLER (bbc_opus_write)
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	device_t *fdc = space->machine().device("wd177x");
	logerror("wd177x write: $%02X  $%02X\n", offset,data);

	if (state->m_DFSType==6)
	{
		if (offset<0x100)
		{
			switch (offset)
			{
				case 0xf8:
					wd17xx_command_w(fdc, 0, data);
					break;
				case 0xf9:
					wd17xx_track_w(fdc, 0, data);
					break;
				case 0xfa:
					wd17xx_sector_w(fdc, 0, data);
					break;
				case 0xfb:
					wd17xx_data_w(fdc, 0, data);
					break;
				case 0xfc:
					bbc_opus_status_w(space, 0,data);
					break;
				case 0xfe:
					state->m_opusbank=(state->m_opusbank & 0xff) | (data<<8);
					break;
				case 0xff:
					state->m_opusbank=(state->m_opusbank & 0xff00) | data;
					break;
			}
		}
		else
		{
			space->machine().region("disks")->base()[offset + (state->m_opusbank << 8)] = data;
		}
	}
}


/***************************************
BBC MASTER DISC SUPPORT
***************************************/


READ8_HANDLER ( bbcm_wd1770_read )
{
	int retval=0xff;
	device_t *fdc = space->machine().device("wd177x");
	switch (offset)
	{
	case 0:
		retval=wd17xx_status_r(fdc, 0);
		break;
	case 1:
		retval=wd17xx_track_r(fdc, 0);
		break;
	case 2:
		retval=wd17xx_sector_r(fdc, 0);
		break;
	case 3:
		retval=wd17xx_data_r(fdc, 0);
		break;
	default:
		break;
	}
	return retval;
}


WRITE8_HANDLER ( bbcm_wd1770_write )
{
	device_t *fdc = space->machine().device("wd177x");
	//logerror("wd177x write: $%02X  $%02X\n", offset,data);
	switch (offset)
	{
	case 0:
		wd17xx_command_w(fdc, 0, data);
		break;
	case 1:
		wd17xx_track_w(fdc, 0, data);
		break;
	case 2:
		wd17xx_sector_w(fdc, 0, data);
		break;
	case 3:
		wd17xx_data_w(fdc, 0, data);
		break;
	default:
		break;
	}
}


READ8_HANDLER ( bbcm_wd1770l_read )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	return state->m_drive_control;
}

WRITE8_HANDLER ( bbcm_wd1770l_write )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	device_t *fdc = space->machine().device("wd177x");
	state->m_drive_control = data;

	/* set drive */
	if ((data>>0) & 0x01) wd17xx_set_drive(fdc,0);
	if ((data>>1) & 0x01) wd17xx_set_drive(fdc,1);

	/* set side */
	wd17xx_set_side(fdc,(data>>4) & 0x01);

	/* set density */
	wd17xx_dden_w(fdc, BIT(data, 5));

//  state->m_1770_IntEnabled=(((data>>4) & 0x01)==0);
	state->m_1770_IntEnabled=1;

}


/**************************************
DFS Hardware mapping for different Disc Controller types
***************************************/

READ8_HANDLER( bbc_disc_r )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	switch (state->m_DFSType){
	/* case 0 to 3 are all standard 8271 interfaces */
	case 0: case 1: case 2: case 3:
		return bbc_i8271_read(space, offset);
	/* case 4 is the acown 1770 interface */
	case 4:
		return bbc_wd1770_read(space, offset);
	/* case 5 is the watford 1770 interface */
	case 5:
		return bbc_wd1770_read(space, offset);
	/* case 6 is the Opus challenger interface */
	case 6:
		/* not connected here, opus drive is connected via the 1MHz Bus */
		break;
	/* case 7 in no disc controller */
	case 7:
		break;
	}
	return 0x0ff;
}

WRITE8_HANDLER ( bbc_disc_w )
{
	bbc_state *state = space->machine().driver_data<bbc_state>();
	switch (state->m_DFSType){
	/* case 0 to 3 are all standard 8271 interfaces */
	case 0: case 1: case 2: case 3:
		bbc_i8271_write(space, offset,data);
		break;
	/* case 4 is the acown 1770 interface */
	case 4:
		bbc_wd1770_write(space, offset,data);
		break;
	/* case 5 is the watford 1770 interface */
	case 5:
		bbc_wd1770_write(space, offset,data);
		break;
	/* case 6 is the Opus challenger interface */
	case 6:
		/* not connected here, opus drive is connected via the 1MHz Bus */
		break;
	/* case 7 in no disc controller */
	case 7:
		break;
	}
}



/**************************************
   BBC B Rom loading functions
***************************************/
DEVICE_IMAGE_LOAD( bbcb_cart )
{
	UINT8 *mem = image.device().machine().region("user1")->base();
	int size, read_;
	int addr = 0;
	int index = 0;

	size = image.length();

	if (strcmp(image.device().tag(),"cart1") == 0)
	{
		index = 0;
	}
	if (strcmp(image.device().tag(),"cart2") == 0)
	{
		index = 1;
	}
	if (strcmp(image.device().tag(),"cart3") == 0)
	{
		index = 2;
	}
	if (strcmp(image.device().tag(),"cart4") == 0)
	{
		index = 3;
	}
	addr = 0x8000 + (0x4000 * index);


	logerror("loading rom %s at %.4x size:%.4x\n", image.filename(), addr, size);


	switch (size)
	{
	case 0x2000:
		read_ = image.fread(mem + addr, size);
		image.fseek(0, SEEK_SET);
		read_ = image.fread(mem + addr + 0x2000, size);
		break;
	case 0x4000:
		read_ = image.fread(mem + addr, size);
		break;
	default:
		read_ = 0;
		logerror("bad rom file size of %.4x\n", size);
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
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_Master=0;
	state->m_tape_timer = machine.scheduler().timer_alloc(FUNC(bbc_tape_timer_cb));
}
DRIVER_INIT( bbcm )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_Master=1;
	state->m_tape_timer = machine.scheduler().timer_alloc(FUNC(bbc_tape_timer_cb));
}

MACHINE_START( bbca )
{
}

MACHINE_RESET( bbca )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	UINT8 *ram = machine.region("maincpu")->base();
	state->m_RAMSize = 1;
	memory_set_bankptr(machine, "bank1",ram);
	memory_set_bankptr(machine, "bank3",ram);

	memory_set_bankptr(machine, "bank4",machine.region("user1")->base());          /* bank 4 is the paged ROMs     from 8000 to bfff */
	memory_set_bankptr(machine, "bank7",machine.region("user1")->base()+0x10000);  /* bank 7 points at the OS rom  from c000 to ffff */

	bbcb_IC32_initialise(state);
}

MACHINE_START( bbcb )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_mc6850_clock = 0;
	//removed from here because MACHINE_START can no longer read DIP swiches.
	//put in MACHINE_RESET instead.
	//state->m_DFSType=  (input_port_read(machine, "BBCCONFIG")>>0)&0x07;
	//state->m_SWRAMtype=(input_port_read(machine, "BBCCONFIG")>>3)&0x03;
	//state->m_RAMSize=  (input_port_read(machine, "BBCCONFIG")>>5)&0x01;

	/*set up the required disc controller*/
	//switch (state->m_DFSType) {
	//case 0:   case 1: case 2: case 3:
		state->m_previous_i8271_int_state=0;
	//  break;
	//case 4: case 5: case 6:
		state->m_previous_wd177x_int_state=1;
	//  break;
	//}
}

MACHINE_RESET( bbcb )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	UINT8 *ram = machine.region("maincpu")->base();
	state->m_DFSType=    (input_port_read(machine, "BBCCONFIG") >> 0) & 0x07;
	state->m_SWRAMtype = (input_port_read(machine, "BBCCONFIG") >> 3) & 0x03;
	state->m_RAMSize=    (input_port_read(machine, "BBCCONFIG") >> 5) & 0x01;

	memory_set_bankptr(machine, "bank1",ram);
	if (state->m_RAMSize)
	{
		/* 32K Model B */
		memory_set_bankptr(machine, "bank3", ram + 0x4000);
		bbc_set_video_memory_lookups(machine, 32);
	}
	else
	{
		/* 16K just repeat the lower 16K*/
		memory_set_bankptr(machine, "bank3", ram);
		bbc_set_video_memory_lookups(machine, 16);
	}

	memory_set_bankptr(machine, "bank4", machine.region("user1")->base());          /* bank 4 is the paged ROMs     from 8000 to bfff */
	memory_set_bankptr(machine, "bank7", machine.region("user1")->base() + 0x40000);  /* bank 7 points at the OS rom  from c000 to ffff */

	bbcb_IC32_initialise(state);


	state->m_opusbank = 0;
	/*set up the required disc controller*/
	//switch (state->m_DFSType) {
	//case 0:   case 1: case 2: case 3:
	//  break;
	//case 4: case 5: case 6:
	//  break;
	//}
}


MACHINE_START( bbcbp )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_mc6850_clock = 0;

	machine.device("maincpu")->memory().space(AS_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(bbcbp_direct_handler, machine));

	/* bank 6 is the paged ROMs     from b000 to bfff */
	memory_configure_bank(machine, "bank6", 0, 16, machine.region("user1")->base() + 0x3000, 1<<14);
}

MACHINE_RESET( bbcbp )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	memory_set_bankptr(machine, "bank1",machine.region("maincpu")->base());
	memory_set_bankptr(machine, "bank2",machine.region("maincpu")->base()+0x03000);  /* bank 2 screen/shadow ram     from 3000 to 7fff */
	memory_set_bankptr(machine, "bank4",machine.region("user1")->base());         /* bank 4 is paged ROM or RAM   from 8000 to afff */
	memory_set_bank(machine, "bank6", 0);
	memory_set_bankptr(machine, "bank7",machine.region("user1")->base()+0x40000); /* bank 7 points at the OS rom  from c000 to ffff */

	bbcb_IC32_initialise(state);


	state->m_previous_wd177x_int_state=1;
}



MACHINE_START( bbcm )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	state->m_mc6850_clock = 0;

	machine.device("maincpu")->memory().space(AS_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(bbcm_direct_handler, machine));

	/* bank 5 is the paged ROMs     from 9000 to bfff */
	memory_configure_bank(machine, "bank5", 0, 16, machine.region("user1")->base()+0x01000, 1<<14);

	/* Set ROM/IO bank to point to rom */
	memory_set_bankptr( machine, "bank8", machine.region("user1")->base()+0x43c00);
	machine.device("maincpu")->memory().space(AS_PROGRAM)->install_read_bank(0xFC00, 0xFEFF, "bank8");
}

MACHINE_RESET( bbcm )
{
	bbc_state *state = machine.driver_data<bbc_state>();
	memory_set_bankptr(machine, "bank1", machine.region("maincpu")->base());			/* bank 1 regular lower ram     from 0000 to 2fff */
	memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x3000);	/* bank 2 screen/shadow ram     from 3000 to 7fff */
	memory_set_bankptr(machine, "bank4", machine.region("user1")->base());         /* bank 4 is paged ROM or RAM   from 8000 to 8fff */
	memory_set_bank(machine, "bank5", 0);
	memory_set_bankptr(machine, "bank7", machine.region("user1")->base() + 0x40000); /* bank 6 OS rom of RAM          from c000 to dfff */

	bbcb_IC32_initialise(state);


	state->m_previous_wd177x_int_state=1;
}
