/***************************************************************************

	Casio FP-1100

	TODO:
	- unimplemented instruction PER triggered

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/upd7810/upd7810.h"
#include "video/mc6845.h"


class fp1100_state : public driver_device
{
public:
	fp1100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_wram;
	UINT8 m_mem_bank;
};

static VIDEO_START( fp1100 )
{
}

static SCREEN_UPDATE( fp1100 )
{
	return 0;
}

static READ8_HANDLER( fp1100_mem_r )
{
	fp1100_state *state = space->machine().driver_data<fp1100_state>();

	if(state->m_mem_bank == 0 && offset <= 0x8fff)
	{
		UINT8 *ipl = space->machine().region("ipl")->base();
		return ipl[offset];
	}

	return state->m_wram[offset];
}

static WRITE8_HANDLER( fp1100_mem_w )
{
	fp1100_state *state = space->machine().driver_data<fp1100_state>();

	state->m_wram[offset] = data;
}

static WRITE8_HANDLER( main_bank_w )
{
	fp1100_state *state = space->machine().driver_data<fp1100_state>();

	state->m_mem_bank = data & 2; //(1) RAM (0) ROM
}

static ADDRESS_MAP_START(fp1100_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(fp1100_mem_r,fp1100_mem_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(fp1100_io, AS_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xffa0, 0xffa0) AM_WRITE(main_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(fp1100_slave_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xff80, 0xffff) AM_RAM		/* upd7801 internal RAM */
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_REGION("sub_ipl",0x0000)
	AM_RANGE(0x1000, 0x1fff) AM_ROM AM_REGION("sub_ipl",0x1000)
	AM_RANGE(0xe000, 0xe000) AM_DEVWRITE_MODERN("crtc", mc6845_device, address_w)
	AM_RANGE(0xe001, 0xe001) AM_DEVWRITE_MODERN("crtc", mc6845_device, register_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION("sub_ipl",0x2000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(fp1100_slave_io, AS_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( fp1100 )
INPUT_PORTS_END


static MACHINE_START(fp1100)
{
	fp1100_state *state = machine.driver_data<fp1100_state>();

	state->m_wram = auto_alloc_array(machine, UINT8, 0x10000);

	state_save_register_global_pointer(machine, state->m_wram, 0x10000);
}

static MACHINE_RESET(fp1100)
{
}

#if 0
static const gfx_layout fp1100_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP8(0,1) },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};
#endif

static GFXDECODE_START( fp1100 )
//	GFXDECODE_ENTRY( "chargen", 0x0000, fp1100_chars_8x8, 0, 8 )
GFXDECODE_END

static const UPD7810_CONFIG fp1100_slave_cpu_config = { TYPE_7801, NULL };
//static const upd1771_interface scv_upd1771c_config = { DEVCB_LINE( scv_upd1771_ack_w ) };

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

static MACHINE_CONFIG_START( fp1100, fp1100_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz) //unknown clock
	MCFG_CPU_PROGRAM_MAP(fp1100_map)
	MCFG_CPU_IO_MAP(fp1100_io)

	MCFG_CPU_ADD( "sub", UPD7801, XTAL_4MHz ) //unknown clock
	MCFG_CPU_PROGRAM_MAP( fp1100_slave_map )
	MCFG_CPU_IO_MAP( fp1100_slave_io )
	MCFG_CPU_CONFIG( fp1100_slave_cpu_config )

	MCFG_MACHINE_START(fp1100)
	MCFG_MACHINE_RESET(fp1100)

	MCFG_MC6845_ADD("crtc", H46505, XTAL_4MHz/2, mc6845_intf)	/* hand tuned to get ~60 fps */

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(fp1100)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(fp1100)
	MCFG_GFXDECODE(fp1100)

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( fp1100 )
	ROM_REGION( 0x9000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x9000, BAD_DUMP CRC(7c7dd17c) SHA1(985757b9c62abd17b0bd77db751d7782f2710ec3))

	ROM_REGION( 0x3000, "sub_ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "sub1.rom", 0x0000, 0x1000, CRC(8feda489) SHA1(917d5b398b9e7b9a6bfa5e2f88c5b99923c3c2a3))
	ROM_LOAD( "sub2.rom", 0x1000, 0x1000, CRC(359f007e) SHA1(0188d5a7b859075cb156ee55318611bd004128d7))
	ROM_LOAD( "sub3.rom", 0x2000, 0xf80, BAD_DUMP CRC(fb2b577a) SHA1(a9ae6b03e06ea2f5db30dfd51ebf5aede01d9672))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1983, fp1100,  0,      0,       fp1100,     fp1100,    0,     "Casio",   "FP-1100", GAME_NOT_WORKING | GAME_NO_SOUND)
