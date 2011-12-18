/***************************************************************************

    ACT Apricot F1 series

    preliminary driver by Angelo Salese


11/09/2011 - modernised. The portable doesn't seem to have
             scroll registers, and it sets the palette to black.
             I've added a temporary video output so that you can get
             an idea of what the screen should look like. [Robbbert]

****************************************************************************/

/*

	TODO:
	
	- CTC/SIO interrupt acknowledge
	- CTC clocks
	- sound
	- portable has wrong devices

*/

#include "includes/apricotf.h"



//**************************************************************************
//  VIDEO
//**************************************************************************

bool f1_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	int lines = m_200_256 ? 200 : 256;
	
	for (int y = 0; y < lines; y++)
	{
		offs_t addr = m_p_scrollram[y] << 1;
		
		for (int sx = 0; sx < 80; sx++)
		{
			UINT16 data = program->read_word(addr);
			
			if (m_40_80)
			{
				for (int x = 0; x < 8; x++)
				{
					int color = (BIT(data, 15) << 1) | BIT(data, 7);
					
					*BITMAP_ADDR16(&bitmap, y, (sx * 8) + x) = color;
					
					data <<= 1;
				}
			}
			else
			{
				for (int x = 0; x < 4; x++)
				{
					int color = (BIT(data, 15) << 3) | (BIT(data, 14) << 2) | (BIT(data, 7) << 1) | BIT(data, 6);
					
					*BITMAP_ADDR16(&bitmap, y, (sx * 8) + (x * 2)) = color;
					*BITMAP_ADDR16(&bitmap, y, (sx * 8) + (x * 2) + 1) = color;
					
					data <<= 2;
				}
			}
			
			addr += 2;
		}
	}

	return false;
}

bool f1p_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return false;
}

READ16_MEMBER( f1_state::palette_r )
{
	return m_p_paletteram[offset];
}

WRITE16_MEMBER( f1_state::palette_w )
{
	UINT8 i,r,g,b;
	COMBINE_DATA(&m_p_paletteram[offset]);

	if(ACCESSING_BITS_0_7 && offset) //TODO: offset 0 looks bogus
	{
		i = m_p_paletteram[offset] & 1;
		r = ((m_p_paletteram[offset] & 2)>>0) | i;
		g = ((m_p_paletteram[offset] & 4)>>1) | i;
		b = ((m_p_paletteram[offset] & 8)>>2) | i;

		palette_set_color_rgb(machine(), offset, pal2bit(r), pal2bit(g), pal2bit(b));
	}
}


static const gfx_layout charset_8x8 =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static GFXDECODE_START( act_f1 )
	GFXDECODE_ENTRY( I8086_TAG, 0x0800, charset_8x8, 0, 1 )
GFXDECODE_END


WRITE8_MEMBER( f1_state::system_w )
{
	switch(offset)
	{
	case 0: // centronics data port
		centronics_data_w(m_centronics, 0, data);
		break;
	
	case 1: // drive select
		wd17xx_set_drive(m_fdc, !BIT(data, 0));
		break;
	
	case 3: // drive head load
		break;
	
	case 5: // drive motor on
		floppy_mon_w(m_floppy0, !BIT(data, 0));
		break;
	
	case 7: // video lines (1=200, 0=256)
		m_200_256 = BIT(data, 0);
		break;
	
	case 9: // video columns (1=80, 0=40)
		m_40_80 = BIT(data, 0);
		break;
	
	case 0x0b: // LED 0 enable
		break;
	
	case 0x0d: // LED 1 enable
		break;
	
	case 0x0f: // centronics strobe output
		centronics_strobe_w(m_centronics, !BIT(data, 0));
		break;
	}
}



//**************************************************************************
//  KEYBOARD
//**************************************************************************

UINT8 f1_state::read_keyboard()
{
	UINT8 data = 0xff;
	
	if (!BIT(m_kb_y, 0)) data &= input_port_read(machine(), "Y0");
	if (!BIT(m_kb_y, 1)) data &= input_port_read(machine(), "Y1");
	if (!BIT(m_kb_y, 2)) data &= input_port_read(machine(), "Y2");
	if (!BIT(m_kb_y, 3)) data &= input_port_read(machine(), "Y3");
	if (!BIT(m_kb_y, 4)) data &= input_port_read(machine(), "Y4");
	if (!BIT(m_kb_y, 5)) data &= input_port_read(machine(), "Y5");
	if (!BIT(m_kb_y, 6)) data &= input_port_read(machine(), "Y6");
	if (!BIT(m_kb_y, 7)) data &= input_port_read(machine(), "Y7");
	if (!BIT(m_kb_y, 8)) data &= input_port_read(machine(), "Y8");
	if (!BIT(m_kb_y, 9)) data &= input_port_read(machine(), "Y9");
	if (!BIT(m_kb_y, 10)) data &= input_port_read(machine(), "YA");
	if (!BIT(m_kb_y, 11)) data &= input_port_read(machine(), "YB");
	if (!BIT(m_kb_y, 12)) data &= input_port_read(machine(), "YC");
	
	return data;
}


READ8_MEMBER( f1_state::kb_lo_r )
{
	return read_keyboard() & 0x0f;
}


READ8_MEMBER( f1_state::kb_hi_r )
{
	return read_keyboard() >> 4;
}


READ8_MEMBER( f1_state::kb_p6_r )
{
	/*
	
		bit		description
		
		P60		
		P61		SHIFT
		P62		CONTROL
	
	*/

	UINT8 modifiers = input_port_read(machine(), "MODIFIERS");
	
	return modifiers << 1;
}


WRITE8_MEMBER( f1_state::kb_p3_w )
{
	// bit 1 controls the IR LEDs
}


WRITE8_MEMBER( f1_state::kb_y0_w )
{
	m_kb_y = (m_kb_y & 0xfff0) | (data & 0x0f);
}


WRITE8_MEMBER( f1_state::kb_y4_w )
{
	m_kb_y = (m_kb_y & 0xff0f) | ((data & 0x0f) << 4);
}


WRITE8_MEMBER( f1_state::kb_y8_w )
{
	m_kb_y = (m_kb_y & 0xf0ff) | ((data & 0x0f) << 8);
}


WRITE8_MEMBER( f1_state::kb_yc_w )
{
	m_kb_y = (m_kb_y & 0xf0ff) | ((data & 0x0f) << 12);
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( act_f1_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( act_f1_mem, AS_PROGRAM, 16, f1_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x01dff) AM_RAM
	AM_RANGE(0x01e00, 0x01fff) AM_RAM AM_BASE(m_p_scrollram)
	AM_RANGE(0x02000, 0x3ffff) AM_RAM
	AM_RANGE(0xe0000, 0xe001f) AM_READWRITE(palette_r, palette_w) AM_BASE(m_p_paletteram)
	AM_RANGE(0xf8000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( act_f1p_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( act_f1p_mem, AS_PROGRAM, 16, f1p_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0xeffff) AM_RAM// AM_BASE(m_p_videoram)
	AM_RANGE(0xf0000, 0xf7fff) AM_RAM
	AM_RANGE(0xf8000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( act_f1_io )
//-------------------------------------------------

static ADDRESS_MAP_START( act_f1_io, AS_IO, 16, f1_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_WRITE8(system_w, 0xffff)
	AM_RANGE(0x0010, 0x0017) AM_DEVREADWRITE8_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w, 0x00ff)
	AM_RANGE(0x0020, 0x0027) AM_DEVREADWRITE8_LEGACY(Z80SIO2_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w, 0x00ff)
//  AM_RANGE(0x0030, 0x0031) AM_WRITE8_LEGACY(ctc_ack_w, 0x00ff)
	AM_RANGE(0x0040, 0x0047) AM_DEVREADWRITE8_LEGACY(WD2797_TAG, wd17xx_r, wd17xx_w, 0x00ff)
//  AM_RANGE(0x01e0, 0x01ff) winchester
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( kb_io )
//-------------------------------------------------
/*
static ADDRESS_MAP_START( kb_io, AS_IO, 8, f1_state )
	AM_RANGE(0x00, 0x00) AM_READ(kb_lo_r)
	AM_RANGE(0x01, 0x01) AM_READ(kb_hi_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(kb_p3_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(kb_y0_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(kb_y4_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(kb_p6_r, kb_yc_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(kb_y8_w)
ADDRESS_MAP_END
*/


//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( act )
//-------------------------------------------------

static INPUT_PORTS_START( act )
	PORT_START("Y0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("Y9")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("YA")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("YB")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
	
	PORT_START("YC")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  Z80DART_INTERFACE( sio_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( f1_state::sio_int_w )
{
	m_sio_int = state;
	
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, m_ctc_int | m_sio_int);
}

static Z80DART_INTERFACE( sio_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(f1_state, sio_int_w)
};


//-------------------------------------------------
//  Z80CTC_INTERFACE( ctc_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( f1_state::ctc_int_w )
{
	m_ctc_int = state;
	
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, m_ctc_int | m_sio_int);
}

WRITE_LINE_MEMBER( f1_state::ctc_z1_w )
{
	z80dart_rxcb_w(m_sio, state);
	z80dart_txcb_w(m_sio, state);
}

WRITE_LINE_MEMBER( f1_state::ctc_z2_w )
{
	z80dart_txca_w(m_sio, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,					// timer disables
	DEVCB_DRIVER_LINE_MEMBER(f1_state, ctc_int_w),		// interrupt handler
	DEVCB_NULL,		// ZC/TO0 callback
	DEVCB_DRIVER_LINE_MEMBER(f1_state, ctc_z1_w),	// ZC/TO1 callback
	DEVCB_DRIVER_LINE_MEMBER(f1_state, ctc_z2_w),	// ZC/TO2 callback
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static LEGACY_FLOPPY_OPTIONS_START( act )
	LEGACY_FLOPPY_OPTION( img2hd, "dsk", "2HD disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface act_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD, // Sony OA-D32W
	LEGACY_FLOPPY_OPTIONS_NAME(act),
	"floppy_3_5",
	NULL
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_NMI),
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_TEST), // TODO inverted?
	{ FLOPPY_0, NULL, NULL, NULL }
};


//-------------------------------------------------
//  centronics_interface centronics_intf
//-------------------------------------------------

static const centronics_interface centronics_intf =
{
	0,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z80SIO2_TAG, z80dart_ctsa_w),
	DEVCB_NULL
};



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( act_f1 )
//-------------------------------------------------

static MACHINE_CONFIG_START( act_f1, f1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(I8086_TAG, I8086, XTAL_14MHz/4)
	MCFG_CPU_PROGRAM_MAP(act_f1_mem)
	MCFG_CPU_IO_MAP(act_f1_io)

	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 256-1)
	MCFG_PALETTE_LENGTH(16)
	MCFG_GFXDECODE(act_f1)

	/* Devices */
	MCFG_Z80DART_ADD(Z80SIO2_TAG, 2500000, sio_intf)
	MCFG_Z80CTC_ADD(Z80CTC_TAG, 2500000, ctc_intf)
	MCFG_WD2797_ADD(WD2797_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(act_floppy_interface)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( act_f1p )
//-------------------------------------------------

static MACHINE_CONFIG_START( act_f1p, f1p_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(I8086_TAG, I8086, XTAL_15MHz/3)
	MCFG_CPU_PROGRAM_MAP(act_f1p_mem)
	MCFG_CPU_IO_MAP(act_f1_io)
/*
	MCFG_CPU_ADD(UPD7507C_TAG, UPD7507, XTAL_32_768kHz)
	MCFG_CPU_IO_MAP(kb_io)
*/
	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 256)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 256-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
	MCFG_GFXDECODE(act_f1)
	MCFG_DEFAULT_LAYOUT( layout_lcd )

	/* Devices */
	MCFG_Z80DART_ADD(Z80SIO2_TAG, 2500000, sio_intf)
	MCFG_Z80CTC_ADD(Z80CTC_TAG, 2500000, ctc_intf)
	MCFG_WD2797_ADD(WD2797_TAG, fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(act_floppy_interface)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( f1 )
//-------------------------------------------------

ROM_START( f1 )
	ROM_REGION( 0x8000, I8086_TAG, 0 )
	ROM_LOAD16_BYTE( "lo_f1_1.6.8f",  0x0000, 0x4000, CRC(be018be2) SHA1(80b97f5b2111daf112c69b3f58d1541a4ba69da0) )	// Labelled F1 - LO Vr. 1.6
	ROM_LOAD16_BYTE( "hi_f1_1.6.10f", 0x0001, 0x4000, CRC(bbba77e2) SHA1(e62bed409eb3198f4848f85fccd171cd0745c7c0) )	// Labelled F1 - HI Vr. 1.6

	ROM_REGION( 0x800, UPD7507C_TAG, 0 )
	ROM_LOAD( "keyboard controller upd7507c.ic2", 0x000, 0x800, NO_DUMP )
ROM_END

#define rom_f1e rom_f1
#define rom_f2 rom_f1


//-------------------------------------------------
//  ROM( f10 )
//-------------------------------------------------

ROM_START( f10 )
	ROM_REGION( 0x8000, I8086_TAG, 0 )
	ROM_LOAD16_BYTE( "lo_f10_3.1.1.8f",  0x0000, 0x4000, CRC(bfd46ada) SHA1(0a36ef379fa9af7af9744b40c167ce6e12093485) )	// Labelled LO-FRange Vr3.1.1
	ROM_LOAD16_BYTE( "hi_f10_3.1.1.10f", 0x0001, 0x4000, CRC(67ad5b3a) SHA1(a5ececb87476a30167cf2a4eb35c03aeb6766601) )	// Labelled HI-FRange Vr3.1.1

	ROM_REGION( 0x800, UPD7507C_TAG, 0 )
	ROM_LOAD( "keyboard controller upd7507c.ic2", 0x000, 0x800, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( fp )
//-------------------------------------------------

ROM_START( fp )
	ROM_REGION( 0x8000, I8086_TAG, 0 )
	ROM_LOAD16_BYTE( "lo_fp_3.1.ic20", 0x0000, 0x4000, CRC(0572add2) SHA1(c7ab0e5ced477802e37f9232b5673f276b8f5623) )	// Labelled 11212721 F97E PORT LO VR 3.1
	ROM_LOAD16_BYTE( "hi_fp_3.1.ic9",  0x0001, 0x4000, CRC(3903674b) SHA1(8418682dcc0c52416d7d851760fea44a3cf2f914) )	// Labelled 11212721 BD2D PORT HI VR 3.1

	ROM_REGION( 0x1000, "hd63b01v1", 0 )
	ROM_LOAD( "voice interface hd63b01v01.ic29", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x800, UPD7507C_TAG, 0 )
	ROM_LOAD( "keyboard controller upd7507c.ic2", 0x000, 0x800, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT     COMPANY             FULLNAME        FLAGS
COMP( 1984, f1,    0,      0,      act_f1,    act,    0,     "ACT",   "Apricot F1",            GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1984, f1e,   f1,     0,      act_f1,    act,    0,     "ACT",   "Apricot F1e",           GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1984, f2,    f1,     0,      act_f1,    act,    0,     "ACT",   "Apricot F2",            GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1985, f10,   f1,     0,      act_f1,    act,    0,     "ACT",   "Apricot F10",           GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1984, fp,    0,      0,      act_f1p,   act,    0,     "ACT",   "Apricot Portable / FP", GAME_NOT_WORKING | GAME_NO_SOUND )
