/***************************************************************************

    A5105

    12/05/2009 Skeleton driver.

    http://www.robotrontechnik.de/index.htm?/html/computer/a5105.htm

	- this looks like "somehow" inspired by the MSX1 machine?

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class a5105_state : public driver_device
{
public:
	a5105_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 pcg_addr;
	UINT16 pcg_internal_addr;
	UINT8 key_mux;
};


static VIDEO_START( a5105 )
{
}

static VIDEO_UPDATE( a5105 )
{
	UINT8 *vram = screen->machine->region("vram")->base();
	//UINT8 *attr = screen->machine->region("attr")->base();
	int x,y;
	int count;
	const gfx_element *gfx = screen->machine->gfx[0];
	UINT8 tile;

	count = 0;

	for(y=0;y<32;y++)
	{
		for(x=0;x<40;x++)
		{
			tile = vram[count];

			drawgfx_opaque(bitmap,cliprect,gfx,tile,0,0,0,x*8,y*8);
			count++;
		}
	}


	return 0;
}

static ADDRESS_MAP_START(a5105_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_ROM //ROM bank
	AM_RANGE(0xc000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END


/*
VDP sequence is:
- polls the command ($99)
- polls the params ($98)
*/

static UINT8 vdp_cmd,vdp_index,vdp_param[0x10],vdp_stack[0x100];
static UINT32 vdp_offset,vdp_read_stack;

static READ8_HANDLER( vdp_status_r )
{
	/*
	[0]
	--x- ---- vblank
	---- --x- processed command?
	*/

	return 0xdd | input_port_read(space->machine,"VBLANK");
}

static READ8_HANDLER( vdp_data_r )
{
	//printf("Read data port\n");
	UINT8 res;
	//UINT8 *vram = space->machine->region("vram")->base();

	res = vdp_stack[vdp_read_stack];

	if(vdp_read_stack < 0x100)
		vdp_read_stack++;

	//res = vdp_stack[vdp_offset+vdp_read_stack];
	//vdp_read_stack++;
	//printf("%04x %04x %02x\n",vdp_offset,vram_read_stack,res);

	return res;
}

static WRITE8_HANDLER( vdp_cmd_w )
{
	/*

	*/

	vdp_cmd = data;
	vdp_index = 0;

	switch(data)
	{
		case 0x00: printf("VDP: Reset / screen param issued 0x00\n"); break;
		case 0x20: printf("VDP: Put char command issued 0x20\n"); break;
		case 0x49: printf("VDP: current videoram offset command issued 0x49\n"); break;
		case 0x4c: printf("VDP: repeat command issued? 0x4c\n"); break;
		case 0xa0:
			vdp_read_stack = 0;
			printf("VDP: get current videoram offset 0xa0 = %04x\n",vdp_offset);
			vdp_stack[0] = vdp_offset & 0xff;
			vdp_stack[1] = vdp_offset >> 8;
			break;

		default: printf("VDP: Unknown command issued %02x\n",data);
	}

}

static WRITE8_HANDLER( vdp_data_w )
{
	UINT8 *vram = space->machine->region("vram")->base();
	UINT8 *attr = space->machine->region("attr")->base();

	/**/

	vdp_param[vdp_index] = data;

	printf("%02x\n",data);

	if(vdp_index < 0x10)
		vdp_index++;

	switch(vdp_cmd)
	{
		case 0x20:
			if(vdp_index == 2)
			{
				vram[vdp_offset] = vdp_param[0];
				attr[vdp_offset] = vdp_param[1];
				vdp_offset++;
				vdp_offset&=0xffff; //TODO: this is wrong
			}
			break;
		case 0x49:
			if(vdp_index == 2)
			{
				vdp_offset = (vdp_param[1]<<8)|(vdp_param[0]);
				//vdp_offset++;
				//vdp_offset&=0xffff;
			}
			if(vdp_index == 3)
				vdp_offset = 0;
			break;
		case 0x4c:
			if(vdp_index == 3)
			{
				if(vdp_param[0] == 2 && vdp_param[1] == 0x27 && vdp_param[2] == 0x00)
				{
					int i;

					for(i=1;i<0x50;i++)
					{
						vram[vdp_offset+i] = vram[vdp_offset];
						attr[vdp_offset+i] = attr[vdp_offset];
					}
				}
			}
			break;
		default:
			break;
	}


}

static WRITE8_HANDLER( pcg_addr_w )
{
	a5105_state *state = space->machine->driver_data<a5105_state>();

	state->pcg_addr = data << 3;
	state->pcg_internal_addr = 0;
}

static WRITE8_HANDLER( pcg_val_w )
{
	UINT8 *gfx_data = space->machine->region("pcg")->base();
	a5105_state *state = space->machine->driver_data<a5105_state>();

	gfx_data[state->pcg_addr|state->pcg_internal_addr] = data;

	gfx_element_mark_dirty(space->machine->gfx[0], state->pcg_addr >> 3);

	state->pcg_internal_addr++;
	state->pcg_internal_addr&=7;
}

static READ8_HANDLER( key_r )
{
	a5105_state *state = space->machine->driver_data<a5105_state>();
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3",
	                                        "KEY4", "KEY5", "KEY6", "KEY7",
	                                        "KEY8", "KEY9", "KEYA", "UNUSED",
	                                        "UNUSED", "UNUSED", "UNUSED", "UNUSED" };

	if((state->key_mux & 0xf0) == 0xf0)
		return input_port_read(space->machine, keynames[state->key_mux & 0x0f]);

	return 0xff;
}

static WRITE8_HANDLER( key_mux_w )
{
	a5105_state *state = space->machine->driver_data<a5105_state>();

	state->key_mux = data;
}

static ADDRESS_MAP_START( a5105_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x98, 0x98) AM_READ(vdp_status_r)
	AM_RANGE(0x99, 0x99) AM_READ(vdp_data_r)

	AM_RANGE(0x98, 0x98) AM_WRITE(vdp_data_w)
	AM_RANGE(0x99, 0x99) AM_WRITE(vdp_cmd_w)
	AM_RANGE(0x9c, 0x9c) AM_WRITE(pcg_val_w)
	//0x9d palette?
	AM_RANGE(0x9e, 0x9e) AM_WRITE(pcg_addr_w)

	//0xa8 is an i/o component (bankswitch lies there?)
	AM_RANGE(0xa9, 0xa9) AM_READ(key_r)
	AM_RANGE(0xaa, 0xaa) AM_READ(key_mux_w)

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a5105 )
	PORT_START("KEY0")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("KEY1")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)				PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)				PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("<") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("+") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	/* TODO: following three aren't marked correctly */
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) //PORT_NAME("ò") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) //PORT_NAME("à") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) //PORT_NAME("ù") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("#") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')

	PORT_START("KEY2")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("'") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("?") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(".") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(",") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("-") //PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Dead Key?") PORT_CODE(KEYCODE_TILDE)				\
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("a") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("b") PORT_CODE(KEYCODE_B) PORT_CHAR('B')

	PORT_START("KEY3")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')

	PORT_START("KEY4")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("KEY5")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("KEY6")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)						PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL)						PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_PGUP)		PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK)	PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CODE") PORT_CODE(KEYCODE_PGDN)		PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1  F6") PORT_CODE(KEYCODE_F1)		PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2  F7") PORT_CODE(KEYCODE_F2)		PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3  F8") PORT_CODE(KEYCODE_F3)		PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("KEY7")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4  F9") PORT_CODE(KEYCODE_F4)		PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5  F10") PORT_CODE(KEYCODE_F5)		PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)							PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)							PORT_CHAR('\t')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("STOP") PORT_CODE(KEYCODE_RCONTROL)	PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)					PORT_CHAR(8)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SELECT") PORT_CODE(KEYCODE_END)		PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)						PORT_CHAR(13)

	PORT_START("KEY8")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)					PORT_CHAR(' ')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_HOME)					PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_INSERT)				PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)	PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)					PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)					PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)					PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)					PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("KEY9")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)	PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)	PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)		PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)		PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)		PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)		PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)		PORT_CHAR(UCHAR_MAMEKEY(4_PAD))

	PORT_START("KEYA")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)		PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)		PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)		PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)	PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	PORT_START("UNUSED")
	PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("VBLANK")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK)
INPUT_PORTS_END


static MACHINE_RESET(a5105)
{
}


static const gfx_layout x1_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/* decoded for debugging purpose, this will be nuked in the end... */
static GFXDECODE_START( a5105 )
	GFXDECODE_ENTRY( "pcg",   0x00000, x1_chars_8x8,    0, 1 )
GFXDECODE_END

static INTERRUPT_GEN( a5105_vblank_irq )
{
	cpu_set_input_line_and_vector(device, 0, HOLD_LINE,0x44);
}

static MACHINE_CONFIG_START( a5105, a5105_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(a5105_mem)
	MCFG_CPU_IO_MAP(a5105_io)
	MCFG_CPU_VBLANK_INT("screen",a5105_vblank_irq)

	MCFG_MACHINE_RESET(a5105)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(40*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 32*8-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_GFXDECODE(a5105)

	MCFG_VIDEO_START(a5105)
	MCFG_VIDEO_UPDATE(a5105)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( a5105 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k1505_00.rom", 0x0000, 0x8000, CRC(0ed5f556) SHA1(5c33139db9f59e50da5c76729752f8e653ae34ae))
	ROM_LOAD( "k1505_80.rom", 0x8000, 0x2000, CRC(350e4958) SHA1(7e5b587c59676e8549561117ea0b70234f439a94))

	ROM_REGION( 0x800, "pcg", ROMREGION_ERASEFF )

	ROM_REGION( 0x10000, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x10000, "attr", ROMREGION_ERASE00 )

	ROM_REGION( 0x4000, "gfx2", ROMREGION_ERASEFF )
	ROM_LOAD( "k5651_40.rom", 0x0000, 0x2000, CRC(f4ad4739) SHA1(9a7bbe6f0d180dd513c7854f441cee986c8d9765))
	ROM_LOAD( "k5651_60.rom", 0x2000, 0x2000, CRC(c77dde3f) SHA1(7c16226be6c4c71013e8008fba9d2e9c5640b6a7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME           FLAGS */
COMP( 1989, a5105,  0,      0,       a5105,     a5105,   0,      "VEB Robotron",   "BIC A5105",		GAME_NOT_WORKING | GAME_NO_SOUND)
