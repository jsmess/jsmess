/*

	TODO:
	
	- PLA dump (http://www.zimmers.net/cbmpics/cbm/c64/pla.txt)
	- cartridge loading

*/

#include "includes/c64.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define A15 BIT(offset, 15)
#define A14 BIT(offset, 14)
#define A13 BIT(offset, 13)
#define A12 BIT(offset, 12)
#define VA13 BIT(va, 13)
#define VA12 BIT(va, 12)



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  check_interrupts -
//-------------------------------------------------

void c64_state::check_interrupts()
{
	int restore = BIT(input_port_read(machine(), "SPECIAL"), 7);
	
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, m_cia1_irq | m_vic_irq | m_exp_irq);
	m_maincpu->set_input_line(INPUT_LINE_NMI, m_cia2_irq | restore | m_exp_nmi);
	
	mos6526_flag_w(m_cia1, m_cass_rd & m_iec_srq);
}



//**************************************************************************
//  MEMORY MANAGEMENT UNIT
//**************************************************************************

//-------------------------------------------------
//  bankswitch -
//-------------------------------------------------

void c64_state::bankswitch(offs_t offset, offs_t va, int rw, int aec, int ba, int cas, int *casram, int *basic, int *kernal, int *charom, int *grw, int *io, int *roml, int *romh)
{
	int game = m_exp->game_r();
	int exrom = m_exp->exrom_r();

	*romh = (m_hiram & A15 & !A14 & A13 & !aec & rw & !exrom & !game 
			|| A15 & A14 & A13 & !aec & exrom & !game 
			|| aec & exrom & !game & VA13 & VA12);

	*roml = (m_loram & m_hiram & A15 & !A14 & !A13 & !aec & rw & !exrom 
			|| A15 & !A14 & !A13 & !aec & exrom & !game);

	*io = (m_hiram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & !exrom & !game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & !exrom & !game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & !exrom & !game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & !exrom & !game 
			|| A15 & A14 & !A13 & A12 & ba & !aec & rw & exrom & !game 
			|| A15 & A14 & !A13 & A12 & !aec & !rw & exrom & !game);

	*grw = (!cas & A15 & A14 & !A13 & A12 &	!aec & !rw);
	
	*charom = (m_hiram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & game 
			|| m_loram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & game 
			|| m_hiram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & !exrom & !game 
			|| m_va14 & aec & game & !VA13 & VA12 
			|| m_va14 & aec & !exrom & !game & !VA13 & VA12);				   

	*kernal = (m_hiram & A15 & A14 & A13 & !aec & rw & game 
			|| m_hiram & A15 & A14 & A13 & !aec & rw & !exrom & !game);
			   
	*basic = (m_loram & m_hiram & A15 & !A14 & A13 & !aec & rw & game);

	*casram = !(m_loram & m_hiram & A15 & !A14 & A13 & !aec & rw & game 
			|| m_hiram & A15 & A14 & A13 & !aec & rw & game 
			|| m_hiram & A15 & A14 & A13 & !aec & rw & !exrom & !game 
			|| m_hiram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & game 
			|| m_loram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & game 
			|| m_hiram & !m_charen & A15 & A14 & !A13 & A12 & !aec & rw & !exrom & !game 
			|| m_va14 & aec & game & !VA13 & VA12 
			|| m_va14 & aec & !exrom & !game & !VA13 & VA12 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & !exrom & !game 
			|| m_hiram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & !exrom & !game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & ba & !aec & rw & !exrom & !game 
			|| m_loram & m_charen & A15 & A14 & !A13 & A12 & !aec & !rw & !exrom & !game 
			|| A15 & A14 & !A13 & A12 & ba & !aec & rw & exrom & !game 
			|| A15 & A14 & !A13 & A12 & !aec & !rw & exrom & !game 
			|| m_loram & m_hiram & A15 & !A14 & !A13 & !aec & rw & !exrom 
			|| A15 & !A14 & !A13 & !aec & exrom & !game 
			|| m_hiram & A15 & !A14 & A13 & !aec & rw & !exrom & !game 
			|| A15 & A14 & A13 & !aec & exrom & !game 
			|| aec & exrom & !game & VA13 & VA12 
			|| !A15 & !A14 & A12 & exrom & !game 
			|| !A15 & !A14 & A13 & exrom & !game 
			|| !A15 & A14 & exrom & !game 
			|| A15 & !A14 & A13 & exrom & !game 
			|| A15 & A14 & !A13 & !A12 & exrom & !game 
			|| cas);
}


//-------------------------------------------------
//  read_memory -
//-------------------------------------------------

UINT8 c64_state::read_memory(address_space &space, offs_t offset, int casram, int basic, int kernal, int charom, int io, int roml, int romh)
{
	UINT8 data = 0;
	
	if (casram)
	{
		data = m_ram->pointer()[offset];
	}
	else if (basic)
	{
		data = m_basic[offset & 0x1fff];
	}
	else if (kernal)
	{
		data = m_kernal[offset & 0x1fff];
	}
	else if (charom)
	{
		data = m_charom[offset & 0xfff];
	}
	else if (io)
	{
		switch ((offset >> 10) & 0x03)
		{
		case 0: // VIC
			data = vic2_port_r(m_vic, offset & 0x3f);
			break;

		case 1: // SID
			data = sid6581_r(m_sid, offset & 0x1f);
			break;

		case 2: // COLOR
			data = m_color_ram[offset & 0x3ff];
			break;

		case 3: // CIAS
			switch ((offset >> 8) & 0x03)
			{
			case 0: // CIA1
				if (offset & 1)
					cia_set_port_mask_value(m_cia1, 1, input_port_read(machine(), "CTRLSEL") & 0x80 ? c64_keyline[9] : c64_keyline[8] );
				else
					cia_set_port_mask_value(m_cia1, 0, input_port_read(machine(), "CTRLSEL") & 0x80 ? c64_keyline[8] : c64_keyline[9] );
					
				data = mos6526_r(m_cia1, offset & 0x0f);
				break;
				
			case 1: // CIA2
				data = mos6526_r(m_cia2, offset & 0x0f);
				break;
				
			case 2: // I/O1
				data = m_exp->io1_r(space, offset);
				break;
				
			case 3: // I/O2
				data = m_exp->io2_r(space, offset);
				break;
			}
			break;
		}
	}
	else if (roml)
	{
		data = m_exp->roml_r(space, offset);
	}
	else if (romh)
	{
		data = m_exp->romh_r(space, offset);
	}
	
	return data;
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( c64_state::read )
{
	int casram, basic, kernal, charom, grw, io, roml, romh;
	
	bankswitch(offset, 0, 1, 0, 1, 0, &casram, &basic, &kernal, &charom, &grw, &io, &roml, &romh);
	
	return read_memory(space, offset, casram, basic, kernal, charom, io, roml, romh);
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( c64_state::write )
{
	int casram, basic, kernal, charom, grw, io, roml, romh;
	
	bankswitch(offset, 0, 0, 0, 1, 0, &casram, &basic, &kernal, &charom, &grw, &io, &roml, &romh);

	if (casram)
	{
		m_ram->pointer()[offset] = data;
	}
	else if (io)
	{
		switch ((offset >> 10) & 0x03)
		{
		case 0: // VIC
			vic2_port_w(m_vic, offset & 0x3f, data);
			break;

		case 1: // SID
			sid6581_w(m_sid, offset & 0x1f, data);
			break;

		case 2: // COLOR
			if (grw) m_color_ram[offset & 0x3ff] = 0xf0 | data;
			break;

		case 3: // CIAS
			switch ((offset >> 8) & 0x03)
			{
			case 0: // CIA1
				mos6526_w(m_cia1, offset & 0x0f, data);
				break;
				
			case 1: // CIA2
				mos6526_w(m_cia2, offset & 0x0f, data);
				break;
				
			case 2: // I/O1
				m_exp->io1_w(space, offset, data);
				break;
				
			case 3: // I/O2
				m_exp->io2_w(space, offset, data);
				break;
			}
			break;
		}
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( c64_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c64_mem, AS_PROGRAM, 8, c64_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(read, write)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( c64 )
//-------------------------------------------------

static INPUT_PORTS_START( c64 )
	PORT_INCLUDE( common_cbm_keyboard )		// ROW0 -> ROW7

	PORT_INCLUDE( c64_special )				// SPECIAL

	PORT_INCLUDE( c64_controls )			// CTRLSEL, JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, LIGHTX, LIGHTY, OTHER
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( c64sw )
//-------------------------------------------------

static INPUT_PORTS_START( c64sw )
	PORT_INCLUDE( c64 )

	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa5") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\xA5')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)								PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)							PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)								PORT_CHAR('-')

	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa4") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\xA4')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb6") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\xB6')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)						PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)						PORT_CHAR(':') PORT_CHAR('*')
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( c64gs )
//-------------------------------------------------

static INPUT_PORTS_START( c64gs )
	PORT_INCLUDE( c64 )

	// 2008 FP: This has to be cleaned up later
	// C64gs should simply not scan these inputs
	// as a temporary solution, we keep PeT IPT_UNUSED shortcut

	PORT_MODIFY( "ROW0" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW1" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW2" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW3" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW4" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW5" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW6" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "ROW7" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
	PORT_MODIFY( "SPECIAL" ) // no keyboard
	PORT_BIT (0xff, 0x00, IPT_UNUSED )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  vic2_interface vic_intf
//-------------------------------------------------

static const unsigned char c64_palette[] =
{
// black, white, red, cyan
// purple, green, blue, yellow
// orange, brown, light red, dark gray,
// medium gray, light green, light blue, light gray
// taken from the vice emulator
	0x00, 0x00, 0x00,  0xfd, 0xfe, 0xfc,  0xbe, 0x1a, 0x24,  0x30, 0xe6, 0xc6,
	0xb4, 0x1a, 0xe2,  0x1f, 0xd2, 0x1e,  0x21, 0x1b, 0xae,  0xdf, 0xf6, 0x0a,
	0xb8, 0x41, 0x04,  0x6a, 0x33, 0x04,  0xfe, 0x4a, 0x57,  0x42, 0x45, 0x40,
	0x70, 0x74, 0x6f,  0x59, 0xfe, 0x59,  0x5f, 0x53, 0xfe,  0xa4, 0xa7, 0xa2
};

static PALETTE_INIT( c64 )
{
	int i;

	for (i = 0; i < sizeof(c64_palette) / 3; i++)
	{
		palette_set_color_rgb(machine, i, c64_palette[i * 3], c64_palette[i * 3 + 1], c64_palette[i * 3 + 2]);
	}
}

static PALETTE_INIT( pet64 )
{
	int i;
	for (i = 0; i < 16; i++)
		palette_set_color_rgb(machine, i, 0, c64_palette[i * 3 + 1], 0);
}

UINT32 c64_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	vic2_video_update(m_vic, bitmap, cliprect);

	return 0;
}

static INTERRUPT_GEN( c64_frame_interrupt )
{
	c64_state *state = device->machine().driver_data<c64_state>();
	
	state->check_interrupts();
	cbm_common_interrupt(device);
}

READ8_MEMBER( c64_state::vic_lightpen_x_cb )
{
	return input_port_read(machine(), "LIGHTX") & ~0x01;
}

READ8_MEMBER( c64_state::vic_lightpen_y_cb )
{
	return input_port_read(machine(), "LIGHTY") & ~0x01;
}

READ8_MEMBER( c64_state::vic_lightpen_button_cb )
{
	return input_port_read(machine(), "OTHER") & 0x04;
}

READ8_MEMBER( c64_state::vic_dma_read )
{
	int casram, basic, kernal, charom, grw, io, roml, romh;
	
	offset = (!m_va15 << 15) | (!m_va14 << 14) | offset;
	
	bankswitch(0, offset, 1, 1, 0, 0, &casram, &basic, &kernal, &charom, &grw, &io, &roml, &romh);
	
	return read_memory(space, offset, casram, basic, kernal, charom, io, roml, romh);
}

READ8_MEMBER( c64_state::vic_dma_read_color )
{
	return m_color_ram[offset & 0x3ff];
}

WRITE_LINE_MEMBER( c64_state::vic_irq_w )
{
	m_vic_irq = state;
	
	check_interrupts();
}

READ8_MEMBER( c64_state::vic_rdy_cb )
{
	return input_port_read(machine(), "CYCLES") & 0x07;
}

static const vic2_interface vic_ntsc_intf = 
{
	SCREEN_TAG,
	M6510_TAG,
	VIC6567,
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_x_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_y_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_button_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_dma_read),
	DEVCB_DRIVER_MEMBER(c64_state, vic_dma_read_color),
	DEVCB_DRIVER_LINE_MEMBER(c64_state, vic_irq_w),
	DEVCB_DRIVER_MEMBER(c64_state, vic_rdy_cb)
};

static const vic2_interface vic_pal_intf = 
{
	SCREEN_TAG,
	M6510_TAG,
	VIC6569,
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_x_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_y_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_lightpen_button_cb),
	DEVCB_DRIVER_MEMBER(c64_state, vic_dma_read),
	DEVCB_DRIVER_MEMBER(c64_state, vic_dma_read_color),
	DEVCB_DRIVER_LINE_MEMBER(c64_state, vic_irq_w),
	DEVCB_DRIVER_MEMBER(c64_state, vic_rdy_cb)
};


//-------------------------------------------------
//  sid6581_interface sid_intf
//-------------------------------------------------

static int paddle_read( device_t *device, int which )
{
	running_machine &machine = device->machine();
	c64_state *state = device->machine().driver_data<c64_state>();
	
	int pot1 = 0xff, pot2 = 0xff, pot3 = 0xff, pot4 = 0xff, temp;
	UINT8 cia0porta = mos6526_pa_r(state->m_cia1, 0);
	int controller1 = input_port_read(machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(machine, "CTRLSEL") & 0x70;
	// Notice that only a single input is defined for Mouse & Lightpen in both ports
	switch (controller1)
	{
		case 0x01:
			if (which)
				pot2 = input_port_read(machine, "PADDLE2");
			else
				pot1 = input_port_read(machine, "PADDLE1");
			break;

		case 0x02:
			if (which)
				pot2 = input_port_read(machine, "TRACKY");
			else
				pot1 = input_port_read(machine, "TRACKX");
			break;

		case 0x03:
			if (which && (input_port_read(machine, "JOY1_2B") & 0x20))	// Joy1 Button 2
				pot1 = 0x00;
			break;

		case 0x04:
			if (which)
				pot2 = input_port_read(machine, "LIGHTY");
			else
				pot1 = input_port_read(machine, "LIGHTX");
			break;

		case 0x06:
			if (which && (input_port_read(machine, "OTHER") & 0x04))	// Lightpen Signal
				pot2 = 0x00;
			break;

		case 0x00:
		case 0x07:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	switch (controller2)
	{
		case 0x10:
			if (which)
				pot4 = input_port_read(machine, "PADDLE4");
			else
				pot3 = input_port_read(machine, "PADDLE3");
			break;

		case 0x20:
			if (which)
				pot4 = input_port_read(machine, "TRACKY");
			else
				pot3 = input_port_read(machine, "TRACKX");
			break;

		case 0x30:
			if (which && (input_port_read(machine, "JOY2_2B") & 0x20))	// Joy2 Button 2
				pot4 = 0x00;
			break;

		case 0x40:
			if (which)
				pot4 = input_port_read(machine, "LIGHTY");
			else
				pot3 = input_port_read(machine, "LIGHTX");
			break;

		case 0x60:
			if (which && (input_port_read(machine, "OTHER") & 0x04))	// Lightpen Signal
				pot4 = 0x00;
			break;

		case 0x00:
		case 0x70:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	if (input_port_read(machine, "CTRLSEL") & 0x80)		// Swap
	{
		temp = pot1; pot1 = pot3; pot3 = temp;
		temp = pot2; pot2 = pot4; pot4 = temp;
	}

	switch (cia0porta & 0xc0)
	{
		case 0x40:
			return which ? pot2 : pot1;

		case 0x80:
			return which ? pot4 : pot3;

		case 0xc0:
			return which ? pot2 : pot1;

		default:
			return 0;
	}
}

static const sid6581_interface sid_intf =
{
	paddle_read
};


//-------------------------------------------------
//  mos6526_interface cia1_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_state::cia1_irq_w )
{
	m_cia1_irq = state;
	
	check_interrupts();
}

READ8_MEMBER( c64_state::cia1_pa_r )
{
	/*
		
		bit		description
		
		PA0		COL0, JOY B0
		PA1		COL1, JOY B1
		PA2		COL2, JOY B2
		PA3		COL3, JOY B3
		PA4		COL4, BTNB
		PA5		COL5
		PA6		COL6
		PA7		COL7
		
	*/

	UINT8 cia0portb = mos6526_pb_r(m_cia1, 0);

	return cbm_common_cia0_port_a_r(m_cia1, cia0portb);
}

READ8_MEMBER( c64_state::cia1_pb_r )
{
	/*
		
		bit		description
		
		PB0		JOY A0
		PB1		JOY A1
		PB2		JOY A2
		PB3		JOY A3
		PB4		BTNA/_LP
		PB5
		PB6
		PB7
		
	*/

	UINT8 cia0porta = mos6526_pa_r(m_cia1, 0);

	return cbm_common_cia0_port_b_r(m_cia1, cia0porta);
}

WRITE8_MEMBER( c64_state::cia1_pb_w )
{
	/*
		
		bit		description
		
		PB0		ROW0
		PB1		ROW1
		PB2		ROW2
		PB3		ROW3
		PB4		ROW4
		PB5		ROW5
		PB6		ROW6
		PB7		ROW7
		
	*/
	
	vic2_lightpen_write(m_vic, BIT(data, 4));
}

static const mos6526_interface cia1_intf =
{
	10,
	DEVCB_DRIVER_LINE_MEMBER(c64_state, cia1_irq_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, sp1_w),
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, cnt1_w),
	DEVCB_DRIVER_MEMBER(c64_state, cia1_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(c64_state, cia1_pb_r),
	DEVCB_DRIVER_MEMBER(c64_state, cia1_pb_w)
};


//-------------------------------------------------
//  mos6526_interface cia2_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_state::cia2_irq_w )
{
	m_cia2_irq = state;
	
	check_interrupts();
}

READ8_MEMBER( c64_state::cia2_pa_r )
{
	/*
		
		bit		description
		
		PA0
		PA1
		PA2		USER PORT
		PA3
		PA4
		PA5
		PA6		CLK
		PA7		DATA
		
	*/
	
	UINT8 data = 0;
	
	// user port
	data |= m_user->pa2_r() << 2;
	
	// IEC bus
	data |= m_iec->clk_r() << 6;
	data |= m_iec->data_r() << 7;
	
	return data;
}

WRITE8_MEMBER( c64_state::cia2_pa_w )
{
	/*
		
		bit		description
		
		PA0		_VA14
		PA1		_VA15
		PA2		USER PORT
		PA3		ATN OUT
		PA4		CLK OUT
		PA5		DATA OUT
		PA6
		PA7
		
	*/
	
	// VIC banking
	m_va14 = BIT(data, 0);
	m_va15 = BIT(data, 1);
	
	// user port
	m_user->pa2_w(BIT(data, 2));
	
	// IEC bus
	m_iec->atn_w(!BIT(data, 3));
	m_iec->clk_w(!BIT(data, 4));
	m_iec->data_w(!BIT(data, 5));
}

static const mos6526_interface cia2_intf =
{
	10,
	DEVCB_DRIVER_LINE_MEMBER(c64_state, cia2_irq_w),
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, pc2_w),
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, sp2_w),
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, cnt2_w),
	DEVCB_DRIVER_MEMBER(c64_state, cia2_pa_r),
	DEVCB_DRIVER_MEMBER(c64_state, cia2_pa_w),
	DEVCB_DEVICE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, pb_r),
	DEVCB_DEVICE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, pb_w)
};


//-------------------------------------------------
//  m6502_interface cpu_intf
//-------------------------------------------------

READ8_MEMBER( c64_state::cpu_r )
{
	/*
		
		bit		description
		
		P0		1
		P1		1
		P2		1
		P3
		P4		CASS SENS
		P5		
		
	*/
	
	UINT8 data = 0x07;
	
	data |= ((m_cassette->get_state() & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED) << 4;
	
	return data;
}

WRITE8_MEMBER( c64_state::cpu_w )
{
	/*
		
		bit		description
		
		P0		LORAM
		P1		HIRAM
		P2		CHAREN
		P3		CASS WRT
		P4
		P5		CASS MOTOR
		
	*/

	// HACK apply pull-up resistors
	data |= (offset ^ 0x07) & 0x07;
	
	m_loram = BIT(data, 0);
	m_hiram = BIT(data, 1);
	m_charen = BIT(data, 2);
	
	// cassette write
	m_cassette->output(BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
	
	// cassette motor
	if (!BIT(data, 5))
	{
		m_cassette->change_state(CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		m_cassette_timer->adjust(attotime::zero, 0, attotime::from_hz(44100));
	}
	else
	{
		m_cassette->change_state(CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		m_cassette_timer->reset();
	}
}

static const m6502_interface cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(c64_state, cpu_r),
	DEVCB_DRIVER_MEMBER(c64_state, cpu_w)
};


//-------------------------------------------------
//  m6502_interface sx64_cpu_intf
//-------------------------------------------------

READ8_MEMBER( sx64_state::cpu_r )
{
	/*
		
		bit		description
		
		P0		1
		P1		1
		P2		1
		P3		1
		P4		1
		P5		1
		
	*/

	return 0x3f;
}

WRITE8_MEMBER( sx64_state::cpu_w )
{
	/*
		
		bit		description
		
		P0		LORAM
		P1		HIRAM
		P2		CHAREN
		P3		
		P4
		P5		
		
	*/

	// HACK apply pull-up resistors
	data |= (offset ^ 0x07) & 0x07;
	
	m_loram = BIT(data, 0);
	m_hiram = BIT(data, 1);
	m_charen = BIT(data, 2);
}

static const m6502_interface sx64_cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(sx64_state, cpu_r),
	DEVCB_DRIVER_MEMBER(sx64_state, cpu_w)
};


//-------------------------------------------------
//  m6502_interface c64g_cpu_intf
//-------------------------------------------------

READ8_MEMBER( c64gs_state::cpu_r )
{
	/*
		
		bit		description
		
		P0		1
		P1		1
		P2		1
		P3		1
		P4		1
		P5		1
		
	*/

	return 0x3f;
}

WRITE8_MEMBER( c64gs_state::cpu_w )
{
	/*
		
		bit		description
		
		P0		LORAM
		P1		HIRAM
		P2		CHAREN
		P3		
		P4
		P5		
		
	*/

	// HACK apply pull-up resistors
	data |= (offset ^ 0x07) & 0x07;
	
	m_loram = BIT(data, 0);
	m_hiram = BIT(data, 1);
	m_charen = BIT(data, 2);
}

static const m6502_interface c64gs_cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(c64gs_state, cpu_r),
	DEVCB_DRIVER_MEMBER(c64gs_state, cpu_w)
};


//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( cassette_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( cassette_tick )
{
	c64_state *state = timer.machine().driver_data<c64_state>();

	state->m_cass_rd = state->m_cassette->input() > +0.0;
	
	state->check_interrupts();
}


//-------------------------------------------------
//  CBM_IEC_INTERFACE( iec_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_state::iec_srq_w )
{
	m_iec_srq = state;
	
	check_interrupts();
}

static CBM_IEC_INTERFACE( iec_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(c64_state, iec_srq_w),
	DEVCB_DEVICE_LINE_MEMBER(C64_USER_PORT_TAG, c64_user_port_device, atn_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  C64_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_state::exp_irq_w )
{
	m_exp_irq = state;
	
	check_interrupts();
}

WRITE_LINE_MEMBER( c64_state::exp_nmi_w )
{
	m_exp_nmi = state;
	
	check_interrupts();
}

static C64_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(c64_state, exp_irq_w),
	DEVCB_DRIVER_LINE_MEMBER(c64_state, exp_nmi_w),
	DEVCB_NULL, // DMA
	DEVCB_CPU_INPUT_LINE(M6510_TAG, INPUT_LINE_RESET)
};


//-------------------------------------------------
//  C64_USER_PORT_INTERFACE( user_intf )
//-------------------------------------------------

static C64_USER_PORT_INTERFACE( user_intf )
{
	DEVCB_DEVICE_LINE(MOS6526_1_TAG, mos6526_sp_w),
	DEVCB_DEVICE_LINE(MOS6526_1_TAG, mos6526_cnt_w),
	DEVCB_DEVICE_LINE(MOS6526_2_TAG, mos6526_sp_w),
	DEVCB_DEVICE_LINE(MOS6526_2_TAG, mos6526_cnt_w),
	DEVCB_DEVICE_LINE(MOS6526_2_TAG, mos6526_flag_w),
	DEVCB_CPU_INPUT_LINE(M6510_TAG, INPUT_LINE_RESET)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( c64 )
//-------------------------------------------------

void c64_state::machine_start()
{
	// find memory regions
	m_basic = machine().region("basic")->base();
	m_kernal = machine().region("kernal")->base();
	m_charom = machine().region("charom")->base();
	
	// allocate memory
	m_color_ram = auto_alloc_array(machine(), UINT8, 0x400);
	
	// state saving
	save_item(NAME(m_loram));
	save_item(NAME(m_hiram));
	save_item(NAME(m_charen));
	save_pointer(NAME(m_color_ram), 0x400);
	save_item(NAME(m_va14));
	save_item(NAME(m_va15));
	save_item(NAME(m_cia1_irq));
	save_item(NAME(m_cia2_irq));
	save_item(NAME(m_vic_irq));
	save_item(NAME(m_exp_irq));
	save_item(NAME(m_exp_nmi));
	save_item(NAME(m_cass_rd));
	save_item(NAME(m_iec_srq));
}


//-------------------------------------------------
//  MACHINE_START( c64c )
//-------------------------------------------------

void c64c_state::machine_start()
{
	c64_state::machine_start();
	
	// find memory regions
	m_basic = machine().region(M6510_TAG)->base();
	m_kernal = machine().region(M6510_TAG)->base() + 0x2000;
}


//-------------------------------------------------
//  MACHINE_START( c64gs )
//-------------------------------------------------

void c64gs_state::machine_start()
{
	c64c_state::machine_start();
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( ntsc )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc, c64_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6567_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(c64_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6567_TAG, vic_ntsc_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6567_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6567_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6567_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_interface)
	MCFG_TIMER_ADD(TIMER_C1531_TAG, cassette_tick)
	MCFG_CBM_IEC_ADD(iec_intf, "c1541")
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "NTSC")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "NTSC")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet64 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet64, ntsc )
	MCFG_PALETTE_INIT( pet64 )
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( ntsc_sx )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc_sx, sx64_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6567_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(sx64_cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(sx64_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6567_TAG, vic_ntsc_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6567_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6567_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6567_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CBM_IEC_BUS_ADD(iec_intf)
	MCFG_CBM_IEC_SLOT_ADD("iec4", 4, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec8", 8, sx1541_iec_devices, "sx1541", NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec9", 9, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec10", 10, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec11", 11, cbm_iec_devices, NULL, NULL)
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "NTSC")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "NTSC")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( ntsc_dx )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc_dx, sx64_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6567_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(sx64_cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(sx64_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6567_TAG, vic_ntsc_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6567_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6567_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6567_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CBM_IEC_BUS_ADD(iec_intf)
	MCFG_CBM_IEC_SLOT_ADD("iec4", 4, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec8", 8, sx1541_iec_devices, "sx1541", NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec9", 9, sx1541_iec_devices, "sx1541", NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec10", 10, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec11", 11, cbm_iec_devices, NULL, NULL)
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "NTSC")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "NTSC")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( ntsc_c )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc_c, c64c_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6567_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(c64c_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6567_TAG, vic_ntsc_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6567_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6567_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6567_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_interface)
	MCFG_TIMER_ADD(TIMER_C1531_TAG, cassette_tick)
	MCFG_CBM_IEC_ADD(iec_intf, "c1541")
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "NTSC")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "NTSC")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pal )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal, c64_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6569_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(50))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(c64_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6569_TAG, vic_pal_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6569_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6569_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6569_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_interface)
	MCFG_TIMER_ADD(TIMER_C1531_TAG, cassette_tick)
	MCFG_CBM_IEC_ADD(iec_intf, "c1541")
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "PAL")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "PAL")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pal_sx )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal_sx, sx64_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6569_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(sx64_cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(50))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(sx64_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6569_TAG, vic_pal_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6569_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6569_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6569_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CBM_IEC_BUS_ADD(iec_intf)
	MCFG_CBM_IEC_SLOT_ADD("iec4", 4, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec8", 8, sx1541_iec_devices, "sx1541", NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec9", 9, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec10", 10, cbm_iec_devices, NULL, NULL)
	MCFG_CBM_IEC_SLOT_ADD("iec11", 11, cbm_iec_devices, NULL, NULL)
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "PAL")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "PAL")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pal_c )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal_c, c64c_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6569_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(50))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(c64c_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6569_TAG, vic_pal_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6569_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6569_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6569_CLOCK, cia2_intf)
	MCFG_QUICKLOAD_ADD("quickload", cbm_c64, "p00,prg,t64", CBM_QUICKLOAD_DELAY_SECONDS)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_interface)
	MCFG_TIMER_ADD(TIMER_C1531_TAG, cassette_tick)
	MCFG_CBM_IEC_ADD(iec_intf, "c1541")
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "PAL")
	MCFG_SOFTWARE_LIST_ADD("disk_list", "c64_flop")
	MCFG_SOFTWARE_LIST_FILTER("disk_list", "PAL")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pal_gs )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal_gs, c64gs_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6569_CLOCK)
	MCFG_CPU_PROGRAM_MAP(c64_mem)
	MCFG_CPU_CONFIG(c64gs_cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, c64_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(50))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(c64gs_state, screen_update)

	MCFG_PALETTE_INIT(c64)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(c64_palette) / 3)

	MCFG_VIC2_ADD(MOS6569_TAG, vic_pal_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6851_TAG, SID6581, VIC6569_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_1_TAG, VIC6569_CLOCK, cia1_intf)
	MCFG_MOS6526R1_ADD(MOS6526_2_TAG, VIC6569_CLOCK, cia2_intf)
	MCFG_CBM_IEC_BUS_ADD(iec_intf)
	MCFG_C64_EXPANSION_SLOT_ADD(C64_EXPANSION_SLOT_TAG, expansion_intf, c64_expansion_cards, NULL, NULL)
	MCFG_C64_USER_PORT_ADD(C64_USER_PORT_TAG, user_intf, c64_user_port_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list_c64", "c64_cart")
	MCFG_SOFTWARE_LIST_FILTER("cart_list_c64", "PAL")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( c64n )
//-------------------------------------------------

ROM_START( c64n )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.u3", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_DEFAULT_BIOS("r3")
	ROM_SYSTEM_BIOS(0, "r1", "Kernal rev. 1" )
	ROMX_LOAD( "901227-01.u4", 0x0000, 0x2000, CRC(dce782fa) SHA1(87cc04d61fc748b82df09856847bb5c2754a2033), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "r2", "Kernal rev. 2" )
	ROMX_LOAD( "901227-02.u4", 0x0000, 0x2000, CRC(a5c687b3) SHA1(0e2e4ee3f2d41f00bed72f9ab588b83e306fdb13), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "r3", "Kernal rev. 3" )
	ROMX_LOAD( "901227-03.u4", 0x0000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "jiffydos", "JiffyDOS v6.01" )
	ROMX_LOAD( "jiffydos c64.u4", 0x0000, 0x2000, CRC(2f79984c) SHA1(31e73e66eccb28732daea8ec3ad1addd9b39a017), ROM_BIOS(4) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901225-01.u5", 0x0000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )
	
	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.u17", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c64j )
//-------------------------------------------------

ROM_START( c64j )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.u3", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_LOAD( "906145-02.u4", 0x0000, 0x2000, CRC(3a9ef6f1) SHA1(4ff0f11e80f4b57430d8f0c3799ed0f0e0f4565d) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "906143-02.u5", 0x0000, 0x1000, CRC(1604f6c1) SHA1(0fad19dbcdb12461c99657b2979dbb5c2e47b527) )
	
	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.u17", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c64p )
//-------------------------------------------------

#define rom_c64p rom_c64n


//-------------------------------------------------
//  ROM( c64sw )
//-------------------------------------------------

ROM_START( c64sw )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.u3", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_LOAD( "kernel.u4",	0x0000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_SYSTEM_BIOS( 0, "default", "Swedish Characters" )
	ROMX_LOAD( "charswe.u5", 0x0000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "Swedish Characters (Alt)" )
	ROMX_LOAD( "charswe2.u5", 0x0000, 0x1000, CRC(377a382b) SHA1(20df25e0ba1c88f31689c1521397c96968967fac), ROM_BIOS(2) )
	
	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.u17", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( pet64 )
//-------------------------------------------------

ROM_START( pet64 )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.u3", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_LOAD( "901246-01.u4", 0x0000, 0x2000, CRC(789c8cc5) SHA1(6c4fa9465f6091b174df27dfe679499df447503c) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901225-01.u5", 0x0000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )
	
	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.u17", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( edu64 )
//-------------------------------------------------

#define rom_edu64	rom_c64n


//-------------------------------------------------
//  ROM( sx64n )
//-------------------------------------------------

ROM_START( sx64n )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.ud4", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_SYSTEM_BIOS(0, "cbm", "Original" )
	ROMX_LOAD( "251104-04.ud3", 0x0000, 0x2000, CRC(2c5965d4) SHA1(aa136e91ecf3c5ac64f696b3dbcbfc5ba0871c98), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "jiffydos", "JiffyDOS v6.01" )
	ROMX_LOAD( "jiffydos sx64.ud3", 0x0000, 0x2000, CRC(2b5a88f5) SHA1(942c2150123dc30f40b3df6086132ef0a3c43948), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "1541flash", "1541 FLASH!" )
	ROMX_LOAD( "1541 flash.ud3", 0x0000, 0x2000, CRC(0a1c9b85) SHA1(0bfcaab0ae453b663a6e01cd59a9764805419e00), ROM_BIOS(3) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901225-01.ud1", 0x0000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.ue4", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( rom_sx64p )
//-------------------------------------------------

#define rom_sx64p	rom_sx64n


//-------------------------------------------------
//  ROM( vip64 )
//-------------------------------------------------

ROM_START( vip64 )
	ROM_REGION( 0x2000, "basic", 0 )
	ROM_LOAD( "901226-01.ud4", 0x0000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e) )

	ROM_REGION( 0x2000, "kernal", 0 )
	ROM_LOAD( "kernelsx.ud3", 0x0000, 0x2000, CRC(7858d3d7) SHA1(097cda60469492a8916c2677b7cce4e12a944bc0) )
	
	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "charswe.ud1", 0x0000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a) )

	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "906114-01.ue4", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( dx64 )
//-------------------------------------------------

// ROM_LOAD( "dx64kern.bin", 0x0000, 0x2000, CRC(58065128) ) TODO where is this illusive ROM?
#define rom_dx64 	rom_sx64n


//-------------------------------------------------
//  ROM( c64cn )
//-------------------------------------------------

ROM_START( c64cn )
	ROM_REGION( 0x4000, M6510_TAG, 0 )
	ROM_LOAD( "251913-01.u4", 0x0000, 0x4000, CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901225-01.u5", 0x0000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "252715-01.u8", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c64cp )
//-------------------------------------------------

#define rom_c64cp		rom_c64cn


//-------------------------------------------------
//  ROM( c64g )
//-------------------------------------------------

#define rom_c64g		rom_c64cn


//-------------------------------------------------
//  ROM( c64csw )
//-------------------------------------------------

ROM_START( c64csw )
	ROM_REGION( 0x4000, M6510_TAG, 0 )
	ROM_LOAD( "325182-01.u4", 0x0000, 0x4000, CRC(2aff27d3) SHA1(267654823c4fdf2167050f41faa118218d2569ce) ) // 128/64 FI

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "cbm 64 skand.gen.u5", 0x0000, 0x1000, CRC(377a382b) SHA1(20df25e0ba1c88f31689c1521397c96968967fac) )

	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "252715-01.u8", 0x000, 0x100, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( c64gs )
//-------------------------------------------------

ROM_START( c64gs )
	ROM_REGION( 0x4000, M6510_TAG, 0 )
	ROM_LOAD( "390852-01.u4", 0x0000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901225-01.u5", 0x0000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa) )

	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "252715-01.u8", 0x000, 0x100, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT  MACHINE 	INPUT   	INIT    COMPANY                        FULLNAME    									FLAGS
COMP( 1982,	c64n,	0,  	0,		ntsc,		c64,		0,		"Commodore Business Machines", "Commodore 64 (NTSC)",  						0 )
COMP( 1982,	c64j,	c64n,  	0,		ntsc,		c64,		0,		"Commodore Business Machines", "Commodore 64 (Japan)", 						0 )
COMP( 1982,	c64p,	c64n,  	0,		pal,		c64,		0,		"Commodore Business Machines", "Commodore 64 (PAL)",  						0 )
COMP( 1982,	c64sw,	c64n,	0,		pal,		c64sw,		0,		"Commodore Business Machines", "Commodore 64 / VIC-64S (Sweden/Finland)", 	0 )
COMP( 1983, pet64,	c64n,  	0,    	pet64,   	c64,     	0,     	"Commodore Business Machines", "PET 64 / CBM 4064 (NTSC)", 					0 )
COMP( 1983, edu64,  c64n,  	0,    	pet64,   	c64,     	0,     	"Commodore Business Machines", "Educator 64 (NTSC)", 						0 )
COMP( 1984, sx64n,	c64n,	0,		ntsc_sx,	c64,		0,		"Commodore Business Machines", "SX-64 / Executive 64 (NTSC)", 				0 )
COMP( 1984, sx64p,	c64n,	0,		pal_sx,		c64,		0,		"Commodore Business Machines", "SX-64 / Executive 64 (PAL)", 				0 )
COMP( 1984, vip64,	c64n,	0,		pal_sx,		c64sw,		0,		"Commodore Business Machines", "VIP-64 (Sweden/Finland)",					0 )
COMP( 1984, dx64,	c64n,	0,		ntsc_dx,	c64,		0,		"Commodore Business Machines", "DX-64 (NTSC)", 								0 )
//COMP(1983, clipper,  c64,  0, c64pal,  clipper, c64pal,  "PDC", "Clipper", GAME_NOT_WORKING) // C64 in a briefcase with 3" floppy, electroluminescent flat screen, thermal printer
//COMP(1983, tesa6240, c64,  0, c64pal,  c64,     c64pal,  "Tesa", "6240", GAME_NOT_WORKING) // modified SX64 with label printer
COMP( 1986, c64cn,	c64n,	0,    	ntsc_c,		c64,		0,		"Commodore Business Machines", "Commodore 64C (NTSC)", 						0 )
COMP( 1986, c64cp,	c64n,	0,    	pal_c,		c64,		0,		"Commodore Business Machines", "Commodore 64C (PAL)", 						0 )
COMP( 1986, c64csw,	c64n,	0,    	pal_c,		c64sw,		0,		"Commodore Business Machines", "Commodore 64C (Sweden/Finland)", 			0 )
COMP( 1986, c64g,	c64n,	0,		pal_c,		c64,		0,		"Commodore Business Machines", "Commodore 64G (PAL)", 						0 )
CONS( 1990, c64gs,	c64n,	0,		pal_gs,		c64gs,		0,		"Commodore Business Machines", "Commodore 64 Games System (PAL)", 			0 )
