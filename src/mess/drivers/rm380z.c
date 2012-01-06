
/*

Research Machines RM-380Z
microcomputer produced by Research Machines Limited
1978-1985
MESS driver by judge, friol

===

Memory map:

0x0000-0x0fff - COS ROM (in page 0)/RAM (in page 1)
0x1c00-0x1dff - additional ROM (?)
0x4000-0xbc00 - RAM page0
0xE000-0xEFFF - COS ROM mirror
0xF000-0xF5FF - video ram
0xf600-0xf9ff - ROM (?)
0xfa00-0xffff - RAM

0xfbfc - port0
0xfbfd - "scroll" register in COS 4.0 (?)

According to the manuals, VDU-1 chargen is Texas 74LS262.

===

COS 4.0 disassembly

- routine at 0xe438 is called at startup in COS 4.0 and it sets the RST vectors in RAM
- routine at 0xe487 finds "top" of system RAM and stores it in 0x0006 and 0x000E
- 0xeca0 - outputs a string (null terminated) to screen (?)
- 0xff18 - does char output to screen (char in A?)

*/

#define ADDRESS_MAP_MODERN


#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/flopdrv.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"


#define PORTS_ENABLED_HIGH	( m_port0 & 0x80 )
#define PORTS_ENABLED_LOW	( ( m_port0 & 0x80 ) == 0x00 )

//
//
//

class rm380z_state : public driver_device
{
private: 
	
	UINT8 decode_videoram_char(UINT8 ch1,UINT8 ch2);
	
protected:
	virtual void machine_reset();

public:

	UINT8 m_port0;
	UINT8 m_port0_kbd;
	UINT8 m_port1;
	UINT8 m_fbfd;
	UINT8 *m_cos_rom;

	int m_ksampler;

	UINT8	m_vram[0x600];
	UINT8 m_mainVideoram[0x600];

	UINT8	m_vramchars[0x600];
	UINT8	m_vramattribs[0x600];

	UINT8 m_mainram[0x10000];
	
	int m_rasterlineCtr;
	emu_timer* m_vblankTimer;
	int m_old_fbfd;

	required_device<cpu_device> m_maincpu;
	optional_device<device_t> m_fdc;

	rm380z_state(const machine_config &mconfig, device_type type, const char *tag): driver_device(mconfig, type, tag), 
		m_maincpu(*this, "maincpu"),
		m_fdc(*this, "wd1771")
	{ 
	}

	DECLARE_READ8_MEMBER( port_fbfc_read );
	DECLARE_WRITE8_MEMBER( port_fbfc_write );

	DECLARE_READ8_MEMBER( main_read );
	DECLARE_WRITE8_MEMBER( main_write );

	DECLARE_READ8_MEMBER( videoram_read );
	DECLARE_WRITE8_MEMBER( videoram_write );

	DECLARE_WRITE8_MEMBER(disk_0_control);

	DECLARE_WRITE8_MEMBER( keyboard_put );

};


WRITE8_MEMBER( rm380z_state::port_fbfc_write )
{
	switch ( offset )
	{
	case 0x00:		// PORT0
		//printf("write of [%x] to FBFC\n",data);
		m_port0 = data;
		if (data&0x01) 
		{
			//printf("WARNING: bit0 of port0 reset\n");
			m_port0_kbd=0;
		}
		break;

	case 0x01:		// screen line counter (?)
		//printf("write of [%x] to FBFD\n",data&0x1f);
		m_fbfd=data&0x1f;
		break;

	case 0x02:		// PORT1
		//printf("write of [%x] to FBFE\n",data);
		break;

	case 0x03:		// user I/O port
		//printf("write of [%x] to FBFF\n",data);
		//logerror("%s: Write %02X to user I/O port\n", machine().describe_context(), data );
		break;
	}
}
READ8_MEMBER( rm380z_state::port_fbfc_read )
{
	UINT8 data = 0xFF;

	switch ( offset )
	{
	case 0x00:		// PORT0
		//m_port0_kbd=getKeyboard();
		data = m_port0_kbd;
		//if (m_port0_kbd!=0) m_port0_kbd=0;
		//m_port0_kbd=0;
		//printf("read of port0 (kbd) at [%f] from PC [%x]\n",machine().time().as_double(),cpu_get_pc(machine().device("maincpu")));
		break;

	case 0x01:		// "counter" (?)
		printf("%s: Read from counter FBFD\n", machine().describe_context());
		data = 0x00;
		break;

	case 0x02:		// PORT1
		data = m_port1;
		break;

	case 0x03:		// user port
		printf("%s: Read from user port\n", machine().describe_context());
		break;
	}

	return data;
}

#define LINE_SUBDIVISION 40
#define TIMER_SPEED 50*100*LINE_SUBDIVISION

//
// this simulates line+frame blanking
// according to the System manual, "frame blanking bit (bit 6) of port1 becomes high 
// for about 4.5 milliseconds every 20 milliseconds"
//

static TIMER_CALLBACK(static_vblank_timer)
{
	//printf("timer callback called at [%f]\n",machine.time().as_double());

	rm380z_state *state = machine.driver_data<rm380z_state>();
	
	state->m_rasterlineCtr++; 
	state->m_rasterlineCtr%=(100*LINE_SUBDIVISION);
	
	if (state->m_rasterlineCtr>=((100-22)*LINE_SUBDIVISION))
	{
		// frame blanking
		state->m_port1=0x41;
	}
	else 
	{
		state->m_port1=0x00;
	}

	if ((state->m_rasterlineCtr%LINE_SUBDIVISION)==0) state->m_port1|=0x80; // line blanking

	//		
	// poll keyboard at fixed intervals, otherwise keys get pressed too fast
	//
	
	state->m_ksampler++;
	if (state->m_ksampler>(4*100*LINE_SUBDIVISION))
	{
		state->m_ksampler=0;
	}
}

void rm380z_state::machine_reset()
{
	m_port0=0x00;
	m_port0_kbd=0x00;
	m_port1=0x00;
	m_fbfd=0x00;
	m_old_fbfd=0x00;
	
	m_rasterlineCtr=0;
	m_ksampler=0;
	
	memset(m_vramattribs,0,0x600);
	memset(m_vramchars,0,0x600);
	
	m_cos_rom=machine().region("maincpu")->base();

	machine().scheduler().timer_pulse(attotime::from_hz(TIMER_SPEED), FUNC(static_vblank_timer));
		
	wd17xx_reset(machine().device("wd1771"));
}

//

WRITE8_MEMBER( rm380z_state::main_write )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	if ( PORTS_ENABLED_LOW )
	{
		if ((offset>=0x1bfc)&&(offset<=0x1bff))
		{
			state->port_fbfc_write(space,offset-0x1bfc,data);
			return;
		}

		if ((offset>=0x0000)&&(offset<0x1000))
		{
			// ROM
			printf("ERROR: write on rom at [%x]\n",offset);
			return;
		}
		else if ((offset>=0x1000)&&(offset<0x4000))
		{
			// ?
			printf("WARNING: unhandled write page0 at [%x]\n",offset);
			return;
		}		
		else if ((offset>=0x4000)&&(offset<0xBC00))
		{
			state->m_mainram[offset-0x4000]=data;
			return;
		}
	}
	else
	{
		if ((offset==0x000e)||(offset==0x000f))
		{
			printf("WARNING: writing [%x] at 0x000f at PC [%x]\n",data,cpu_get_pc(machine().device("maincpu")));
		}

		//if ((offset>=0x0000)&&(offset<0x7c00))
		if ((offset>=0x0000)&&(offset<0xdf00))
		{
			// RAM
			state->m_mainram[offset]=data;
			return;
		}
	}
	
	//printf("ERROR: unhandled default write at [%x] of [%x]\n",offset,data);
}

READ8_MEMBER( rm380z_state::main_read )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	if ( PORTS_ENABLED_LOW )
	{
		if ((offset>=0x1bfc)&&(offset<=0x1bff))
		{
			return state->port_fbfc_read(space,offset-0x1bfc);
		}

		if ((offset>=0x0000)&&(offset<0x1000))
		{
			// ROM
			return state->m_cos_rom[offset+ 0xe000];
		}
		else if ((offset>=0x1c00)&&(offset<=0x1dff))
		{
			return m_cos_rom[offset];
		}
		else if ((offset>=0x1000)&&(offset<0x4000))
		{
			// ?
			//printf("WARNING: unhandled read page0 at [%x]\n",offset);
			return 0xff;
		}		
		else if ((offset>=0x4000)&&(offset<0xBC00))
		{
			return state->m_mainram[offset-0x4000];
		}
	}
	else
	{
		//if ((offset>=0x0000)&&(offset<0x7c00))
		if ((offset>=0x0000)&&(offset<0xdf00))
		{
			// ROM
			return state->m_mainram[offset];
		}
	}

	//printf("ERROR: unhandled default read at [%x]\n",offset);	
	return 0xff;
}

//

UINT8 rm380z_state::decode_videoram_char(UINT8 ch1,UINT8 ch2)
{
	if ((ch1>0x0f)&&(ch1<0x80))
	{
		return ch1;
	}
	else if ((ch2>0x0f)&&(ch2<0x80))
	{
		return ch2;
	}
	else if (ch1==0x80)
	{
		// "blank"?
		return 0x80;
	}
	else if ((ch1==0)&&(ch2==8))
	{
		// cursor
		return 0x7f;
	}
	else if ((ch1==0)&&(ch2==0))
	{
		return 0x20;
	}
	else if ((ch1==4)&&(ch2==4))
	{
		// reversed cursor?
		return 0x20;
	}
	else if ((ch1==4)&&(ch2==8))
	{
		// normal cursor
		return 0x7f;
	}
	else
	{
		//printf("unhandled character combination [%x][%x]\n",ch1,ch2);
	}
	
	return 0;
}

WRITE8_MEMBER( rm380z_state::videoram_write )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();

	//printf("vramw [%x][%x] port0 [%x] fbfd [%x]\n",offset,data,state->m_port0,m_fbfd);
	
	if (m_old_fbfd>m_fbfd)
	{
		printf("WARN: fbfd wrapped\n");
	}
	m_old_fbfd=m_fbfd;
	
	int lineAdder=(m_fbfd&0x1f)*0x40;
	int realA=offset+lineAdder;

	// we suppose videoram is being written as character/attribute couple
	// fbfc 6th bit set=attribute, unset=char

	if (!(state->m_port0&0x40))
	{
		m_vramchars[realA%0x600]=data;
	}
	else
	{
		m_vramattribs[realA%0x600]=data;
	}

	UINT8 curch=decode_videoram_char(m_vramchars[realA],m_vramattribs[realA]);
	state->m_vram[realA%0x600]=curch;

	state->m_mainVideoram[offset]=data;
}

READ8_MEMBER( rm380z_state::videoram_read )
{
	rm380z_state *state = machine().driver_data<rm380z_state>();
	//printf("read from videoram at [%x]\n",offset);	
	//return state->m_vram[offset];
	return state->m_mainVideoram[offset];
}

//
// ports c0-cf are related to the floppy disc controller
// c0-c3: wd1771
// c4-c8: disk drive port0
// code at f7df writes 2f to c0
// code at e526 reads from c1
//
// the manual says: "the B command reads a sector from drive 0, track 0, sector 1 into memory
// at 0x0080 to 0x00FF, then jumps to it if there is no error.
//


WRITE8_MEMBER( rm380z_state::disk_0_control )
{
	device_t *fdc = machine().device("wd1771");
	
	printf("disk drive port0 write [%x]\n",data);
	
	// drive port0
	if (data&0x01)
	{
		// drive select bit 0
		wd17xx_set_drive(fdc,0);
	}
	
	if (data&0x08)
	{
		// motor on
	}
	
	// "MSEL (dir/side select bit)
	if (data&0x20)
	{
		wd17xx_set_side(fdc,1);
	}
	else
	{
		wd17xx_set_side(fdc,0);
	}
	
	// set drive en- (?)
	if (data&0x40)
	{
	}
}

static ADDRESS_MAP_START(rm380z_mem, AS_PROGRAM, 8, rm380z_state)
	AM_RANGE( 0x0000, 0xdfff ) AM_READWRITE(main_read,main_write)
	AM_RANGE( 0xe000, 0xefff ) AM_ROM
	AM_RANGE( 0xf000, 0xf5ff ) AM_READWRITE(videoram_read,videoram_write)
	AM_RANGE( 0xf600, 0xf9ff ) AM_ROM /* Extra ROM space for COS4.0 */
	AM_RANGE( 0xfa00, 0xfbfb ) AM_RAM
	AM_RANGE( 0xfbfc, 0xfbff ) AM_READWRITE( port_fbfc_read, port_fbfc_write )
	AM_RANGE( 0xfc00, 0xffff ) AM_RAM
ADDRESS_MAP_END

// ports c0-c3 are related to the floppy disk controller
// which seems to be based on WD1771 from the documentation
// (n.b.: this is not working properly, need to get real fdc mapping)

static ADDRESS_MAP_START( rm380z_io , AS_IO, 8, rm380z_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xc0, 0xc0) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_status_r, wd17xx_command_w)
	AM_RANGE(0xc1, 0xc1) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0xc2, 0xc2) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0xc3, 0xc3) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_data_r, wd17xx_data_w)	
	AM_RANGE(0xc4, 0xc4) AM_WRITE(disk_0_control)
	AM_RANGE(0xcc, 0xcc) AM_NOP // CTC (?)
ADDRESS_MAP_END


INPUT_PORTS_START( rm380z )
INPUT_PORTS_END

//
//
//

#define chdimx 5
#define chdimy 9
#define ncx 8
#define ncy 16
#define screencols 40
#define screenrows 24

static void putChar(int charnum,int x,int y,UINT16* pscr,unsigned char* chsb)
{
	if ((charnum>0)&&(charnum<=0x7f))
	{
		int basex=chdimx*(charnum/ncy);
		int basey=chdimy*(charnum%ncy);
		
		int inix=x*chdimx;
		int iniy=y*chdimx*screencols*chdimy;
		
		for (int r=0;r<chdimy;r++)
		{
			for (int c=0;c<chdimx;c++)
			{
				pscr[(inix+c)+(iniy+(r*chdimx*screencols))]=(chsb[((basey+r)*(chdimx*ncx))+(basex+c)])==0xff?0:1;
			}
		}
	}
}

static SCREEN_UPDATE( rm380z )
{
	rm380z_state *state = screen.machine().driver_data<rm380z_state>();

	unsigned char* pChar=screen.machine().region("gfx")->base();
	UINT16* scrptr = &bitmap.pix16(0, 0);

	for (int row=0;row<screenrows;row++)
	{
		for (int col=0;col<screencols;col++)
		{
			unsigned char curchar=state->m_vram[(row*64)+col];
			putChar(curchar,col,row,scrptr,pChar);			
		}
	}

	return 0;
}


WRITE8_MEMBER( rm380z_state::keyboard_put )
{
	m_port0_kbd = data;
}


static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(rm380z_state, keyboard_put)
};


static const floppy_interface rm380z_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSSD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	"floppy0",
	NULL
};


static MACHINE_CONFIG_START( rm380z, rm380z_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(rm380z_mem)
	MCFG_CPU_IO_MAP(rm380z_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE((screencols*chdimx), (screenrows*chdimy))
	MCFG_SCREEN_VISIBLE_AREA(0, (screencols*chdimx)-1, 0, (screenrows*chdimy)-1)
	MCFG_SCREEN_UPDATE(rm380z)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* floppy disk */
	MCFG_FD1771_ADD("wd1771", default_wd17xx_interface)
	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, rm380z_floppy_interface)

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rm380z )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cos40b-m_1c00-1dff.bin", 0x1c00, 0x0200, CRC(0f759f44) SHA1(9689c1c1faa62c56def999cbedbbb0c8d928dcff))
	ROM_LOAD( "cos40b-m.bin", 			0xe000, 0x1000, CRC(1f0b3a5c) SHA1(0b29cb2a3b7eaa3770b34f08c4fd42844f42700f))
	ROM_LOAD( "cos40b-m_f600-f9ff.bin", 0xf600, 0x0400, CRC(e3397d9d) SHA1(490a0c834b0da392daf782edc7d51ca8f0668b1a))
	ROM_REGION( 0x1680, "gfx", 0 )
	ROM_LOAD( "ch3.raw", 0x0000, 0x1680, CRC(e0b10221) SHA1(95304ccad9a7af49d79a349feb2bc23035f90abc))
ROM_END

/* Driver */

COMP(1978, rm380z, 0,      0,       rm380z,    rm380z,  0,    "Research Machines", "RM-380Z", GAME_NO_SOUND_HW)
