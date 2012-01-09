
/*

RM 380Z machine

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "imagedev/flopdrv.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "includes/rm380z.h"


/*

PORT0 write in COS 4.0B/M:

bit0: 1=reset keyboard latch
bit1: ?
bit2: ?
bit3: ?
bit4: ?
bit5: 40/80 cols switch (?)
bit6: 0=write to char RAM/1=write to attribute RAM
bit7: 1=map ROM at 0000-0fff/0=RAM

*/

WRITE8_MEMBER( rm380z_state::port_write )
{
	switch ( offset )
	{
	case 0x00:		// PORT0
		//printf("write of [%x] to FBFC at PC [%x]\n",data,cpu_get_pc(machine().device("maincpu")));
		m_port0 = data;
		if (data&0x01)
		{
			//printf("WARNING: bit0 of port0 reset\n");
			m_port0_kbd=0;
		}
		config_videomode();
		config_memory_map();
		break;

	case 0x01:		// screen line counter (?)
		//printf("write of [%x] to FBFD at PC [%x] FBFE [%x]\n",data,cpu_get_pc(machine().device("maincpu")),m_fbfe);
		m_fbfd=data;
		break;

	case 0x02:		// something related to screen position
		//printf("write of [%x] to FBFE\n",data);
		m_fbfe=data;
		break;

	case 0x03:		// user I/O port
		//printf("write of [%x] to FBFF\n",data);
		//logerror("%s: Write %02X to user I/O port\n", machine().describe_context(), data );
		break;
	}
}

READ8_MEMBER( rm380z_state::port_read )
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
		//printf("%s: Read from counter FBFD\n", machine().describe_context());
		data = 0x00;
		break;

	case 0x02:		// PORT1
		data = m_port1;
		break;

	case 0x03:		// user port
		//printf("%s: Read from user port\n", machine().describe_context());
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
}

//
// ports c0-cf are related to the floppy disc controller
// c0-c3: wd1771
// c4-c8: disk drive port0
//
// CP/M booting:
// from the service manual: "the B command reads a sector from drive 0, track 0, sector 1 into memory
// at 0x0080 to 0x00FF, then jumps to it if there is no error."
//

WRITE8_MEMBER( rm380z_state::disk_0_control )
{
	device_t *fdc = machine().device("wd1771");

	//printf("disk drive port0 write [%x]\n",data);

	// drive port0
	if (data&0x01)
	{
		// drive select bit 0
		wd17xx_set_drive(fdc,0);
	}

	if (data&0x02)
	{
		// drive select bit 1
		wd17xx_set_drive(fdc,1);
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

WRITE8_MEMBER( rm380z_state::keyboard_put )
{
	m_port0_kbd = data;
}

void rm380z_state::machine_reset()
{
	m_port0=0x00;
	m_port0_kbd=0x00;
	m_port1=0x00;
	m_fbfd=0x00;
	m_fbfe=0x00;
	m_old_fbfd=0x00;
	maxrow=0;
	m_pageAdder=0;
	m_videomode=RM380Z_VIDEOMODE_80COL;
	m_old_videomode=m_videomode;

	m_rasterlineCtr=0;

	// note: from COS 4.0 videos, RAM seems to have garbage at the beginning
	memset(m_mainVideoram,0,0x600);

	memset(m_vramattribs,0,RM380Z_SCREENSIZE);
	memset(m_vramchars,0,RM380Z_SCREENSIZE);
	memset(m_vram,0,RM380Z_SCREENSIZE);

	config_memory_map();

	machine().scheduler().timer_pulse(attotime::from_hz(TIMER_SPEED), FUNC(static_vblank_timer));

	wd17xx_reset(machine().device("wd1771"));
}

void rm380z_state::config_memory_map()
{
	address_space *program = m_maincpu->space(AS_PROGRAM);
	UINT8 *rom = machine().region(RM380Z_MAINCPU_TAG)->base();
	UINT8* m_ram_p = m_messram->pointer();

	if ( RM380Z_PORTS_ENABLED_HIGH )
	{
		program->install_ram( 0x0000, 0xDFFF, m_ram_p );
	}
	else
	{
		program->install_rom( 0x0000, 0x0FFF, rom );
		program->install_readwrite_handler(0x1BFC, 0x1BFF,read8_delegate(FUNC(rm380z_state::port_read), this),write8_delegate(FUNC(rm380z_state::port_write), this)	);
		program->install_rom( 0x1C00, 0x1DFF, rom + 0x1400 );
		program->install_ram( 0x4000, 0xDFFF, m_ram_p );
	}
}
