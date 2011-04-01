/***************************************************************************

    CPS Changer

    Although this is split off from MAME, it still uses machine/kabuki.c
    This in turn includes cps1.h.
    Therefore we must call our include by the same name, in order to
    override the mame include, and thereby remove any compilation errors.
    It also means we can get rid of the various bodges of the past, and
    no more ifdefs are needed in the mame code.

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"
#include "machine/eeprom.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"
#include "sound/qsound.h"
#include "includes/cpschngr.h"

static READ16_HANDLER( cps1_dsw_r )
{
	static const char *const dswname[] = { "IN0", "DSWA", "DSWB", "DSWC" };
	int in = input_port_read(space->machine(), dswname[offset]);
	return (in << 8) | 0xff;
}

static WRITE8_HANDLER( cps1_snd_bankswitch_w )
{
	UINT8 *RAM = space->machine().region("audiocpu")->base();
	int bankaddr;

	bankaddr = ((data & 1) * 0x4000);
	memory_set_bankptr(space->machine(), "bank1",&RAM[0x10000 + bankaddr]);
}

static WRITE8_DEVICE_HANDLER( cps1_oki_pin7_w )
{
	okim6295_device *oki6295 = device->machine().device<okim6295_device>("oki");
	oki6295->set_pin7((data & 1));
}

static WRITE16_HANDLER( cps1_soundlatch_w )
{
	if (ACCESSING_BITS_0_7)
		soundlatch_w(space,0,data & 0xff);
}

static WRITE16_HANDLER( cps1_soundlatch2_w )
{
	if (ACCESSING_BITS_0_7)
		soundlatch2_w(space,0,data & 0xff);
}

static WRITE16_HANDLER( cps1_coinctrl_w )
{
	if (ACCESSING_BITS_8_15)
	{
		coin_counter_w(space->machine(),0,data & 0x0100);
		coin_counter_w(space->machine(),1,data & 0x0200);
		coin_lockout_w(space->machine(),0,~data & 0x0400);
		coin_lockout_w(space->machine(),1,~data & 0x0800);
	}
}

static WRITE16_HANDLER( cpsq_coinctrl2_w )
{
	if (ACCESSING_BITS_0_7)
	{
		coin_counter_w(space->machine(),2,data & 0x01);
		coin_lockout_w(space->machine(),2,~data & 0x02);
		coin_counter_w(space->machine(),3,data & 0x04);
		coin_lockout_w(space->machine(),3,~data & 0x08);
	}
}

static INTERRUPT_GEN( cps1_interrupt )
{
	device_set_input_line(device, 2, HOLD_LINE);
}

/********************************************************************
*
*  Q Sound
*  =======
*
********************************************************************/


static INTERRUPT_GEN( cps1_qsound_interrupt )
{
	device_set_input_line(device, 2, HOLD_LINE);
}


static READ16_HANDLER( qsound_rom_r )
{
	UINT8 *rom = space->machine().region("user1")->base();

	if (rom)
		return rom[offset] | 0xff00;
	else
		return 0;
}

static READ16_HANDLER( qsound_sharedram1_r )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	return state->m_qsound_sharedram1[offset] | 0xff00;
}

static WRITE16_HANDLER( qsound_sharedram1_w )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	if (ACCESSING_BITS_0_7)
		state->m_qsound_sharedram1[offset] = data;
}

static READ16_HANDLER( qsound_sharedram2_r )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	return state->m_qsound_sharedram2[offset] | 0xff00;
}

static WRITE16_HANDLER( qsound_sharedram2_w )
{
	cpschngr_state *state = space->machine().driver_data<cpschngr_state>();
	if (ACCESSING_BITS_0_7)
		state->m_qsound_sharedram2[offset] = data;
}

static WRITE8_HANDLER( qsound_banksw_w )
{
	UINT8 *RAM = space->machine().region("audiocpu")->base();
	int bankaddress=0x10000+((data&0x0f)*0x4000);

	if (bankaddress >= space->machine().region("audiocpu")->bytes())
		bankaddress=0x10000;

	memory_set_bankptr(space->machine(), "bank1", &RAM[bankaddress]);
}


/********************************************************************
*
*  EEPROM
*  ======
*
*   The EEPROM is accessed by a serial protocol using the register
*   0xf1c006
*
********************************************************************/

static const eeprom_interface qsound_eeprom_interface =
{
	7,		/* address bits */
	8,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static READ16_HANDLER( cps1_eeprom_port_r )
{
	device_t *eeprom = space->machine().device("eeprom");
	return eeprom_read_bit(eeprom);
}

static WRITE16_HANDLER( cps1_eeprom_port_w )
{
	device_t *eeprom = space->machine().device("eeprom");
	if (ACCESSING_BITS_0_7)
	{
		/*
        bit 0 = data
        bit 6 = clock
        bit 7 = cs
        */
		eeprom_write_bit(eeprom,data & 0x01);
		eeprom_set_cs_line(eeprom,(data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
		eeprom_set_clock_line(eeprom,(data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}


/***************************************

    Address Maps

****************************************/

static ADDRESS_MAP_START( main_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_ROM
	AM_RANGE(0x800000, 0x800007) AM_READ_PORT("IN1")			/* Player input ports */
	AM_RANGE(0x800018, 0x80001f) AM_READ(cps1_dsw_r)			/* System input ports / Dip Switches */
	AM_RANGE(0x800020, 0x800021) AM_READNOP
	AM_RANGE(0x800030, 0x800037) AM_WRITE(cps1_coinctrl_w)
	AM_RANGE(0x800100, 0x80013f) AM_WRITE(cps1_cps_a_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_cps_a_regs)	/* CPS-A custom */
	AM_RANGE(0x800140, 0x80017f) AM_READWRITE(cps1_cps_b_r, cps1_cps_b_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_cps_b_regs)
	AM_RANGE(0x800180, 0x800187) AM_WRITE(cps1_soundlatch_w)	/* Sound command */
	AM_RANGE(0x800188, 0x80018f) AM_WRITE(cps1_soundlatch2_w)	/* Sound timer fade */
	AM_RANGE(0x900000, 0x92ffff) AM_RAM_WRITE(cps1_gfxram_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_gfxram) AM_SIZE_MEMBER(cpschngr_state, m_cps1_gfxram_size)	/* SF2CE executes code from here */
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank1")
	AM_RANGE(0xd000, 0xd7ff) AM_RAM
	AM_RANGE(0xf000, 0xf001) AM_DEVREADWRITE("2151", ym2151_r, ym2151_w)
	AM_RANGE(0xf002, 0xf002) AM_DEVREADWRITE_MODERN("oki", okim6295_device, read, write)
	AM_RANGE(0xf004, 0xf004) AM_WRITE(cps1_snd_bankswitch_w)
	AM_RANGE(0xf006, 0xf006) AM_DEVWRITE("oki", cps1_oki_pin7_w) /* controls pin 7 of OKI chip */
	AM_RANGE(0xf008, 0xf008) AM_READ(soundlatch_r)	/* Sound command */
	AM_RANGE(0xf00a, 0xf00a) AM_READ(soundlatch2_r) /* Sound timer fade */
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_main_map, AS_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x800000, 0x800007) AM_READ_PORT("IN1")			/* Player input ports */
	AM_RANGE(0x800018, 0x80001f) AM_READ(cps1_dsw_r)			/* System input ports / Dip Switches */
	AM_RANGE(0x800030, 0x800037) AM_WRITE(cps1_coinctrl_w)
	AM_RANGE(0x800100, 0x80013f) AM_WRITE(cps1_cps_a_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_cps_a_regs)	/* CPS-A custom */
	AM_RANGE(0x800140, 0x80017f) AM_READWRITE(cps1_cps_b_r, cps1_cps_b_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_cps_b_regs)	/* CPS-B custom (mapped by LWIO/IOB1 PAL on B-board) */
	AM_RANGE(0x900000, 0x92ffff) AM_RAM_WRITE(cps1_gfxram_w) AM_BASE_MEMBER(cpschngr_state, m_cps1_gfxram) AM_SIZE_MEMBER(cpschngr_state, m_cps1_gfxram_size)	/* SF2CE executes code from here */
	AM_RANGE(0xf00000, 0xf0ffff) AM_READ(qsound_rom_r)			/* Slammasters protection */
	AM_RANGE(0xf18000, 0xf19fff) AM_READWRITE(qsound_sharedram1_r, qsound_sharedram1_w)  /* Q RAM */
	AM_RANGE(0xf1c000, 0xf1c001) AM_READ_PORT("IN2")			/* Player 3 controls (later games) */
	AM_RANGE(0xf1c002, 0xf1c003) AM_READ_PORT("IN3")			/* Player 4 controls ("Muscle Bombers") */
	AM_RANGE(0xf1c004, 0xf1c005) AM_WRITE(cpsq_coinctrl2_w)		/* Coin control2 (later games) */
	AM_RANGE(0xf1c006, 0xf1c007) AM_READWRITE(cps1_eeprom_port_r, cps1_eeprom_port_w)
	AM_RANGE(0xf1e000, 0xf1ffff) AM_READWRITE(qsound_sharedram2_r, qsound_sharedram2_w)  /* Q RAM */
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_sub_map, AS_PROGRAM, 8 )	// used by cps2.c too
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank1")	/* banked (contains music data) */
	AM_RANGE(0xc000, 0xcfff) AM_RAM AM_BASE_MEMBER(cpschngr_state, m_qsound_sharedram1)
	AM_RANGE(0xd000, 0xd002) AM_DEVWRITE("qsound", qsound_w)
	AM_RANGE(0xd003, 0xd003) AM_WRITE(qsound_banksw_w)
	AM_RANGE(0xd007, 0xd007) AM_DEVREAD("qsound", qsound_r)
	AM_RANGE(0xf000, 0xffff) AM_RAM AM_BASE_MEMBER(cpschngr_state, m_qsound_sharedram2)
ADDRESS_MAP_END

/***********************************************************

    Inputs and Dips

***********************************************************/

static INPUT_PORTS_START( sfzch )
	PORT_START("IN0")     /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_F1)	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE  )	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_PLAYER(2)

	PORT_START("DSWA")
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSWB")
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSWC")
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("IN1")     /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(1)

	PORT_START("IN2")      /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2)

	PORT_START("IN3")      /* Player 4 - not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/***************************************

    Graphics Decode

****************************************/

static const gfx_layout cps1_layout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static const gfx_layout cps1_layout8x8_2 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8
};

static GFXLAYOUT_RAW( cps1_layout16x16, 4, 16, 16, 8*8, 128*8 )
static GFXLAYOUT_RAW( cps1_layout32x32, 4, 32, 32, 16*8, 512*8 )

static GFXDECODE_START( cps1 )
	GFXDECODE_ENTRY( "gfx", 0, cps1_layout8x8,   0, 0x100 )
	GFXDECODE_ENTRY( "gfx", 0, cps1_layout8x8_2, 0, 0x100 )
	GFXDECODE_ENTRY( "gfx", 0, cps1_layout16x16, 0, 0x100 )
	GFXDECODE_ENTRY( "gfx", 0, cps1_layout32x32, 0, 0x100 )
GFXDECODE_END



static void cps1_irq_handler_mus(device_t *device, int irq)
{
	cputag_set_input_line(device->machine(), "audiocpu", 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static const ym2151_interface ym2151_config =
{
	cps1_irq_handler_mus
};


/********************************************************************
*
*  Machine Driver macro
*  ====================
*
********************************************************************/

static MACHINE_CONFIG_START( cps1_10MHz, cpschngr_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 10000000)
	MCFG_CPU_PROGRAM_MAP(main_map)
	MCFG_CPU_VBLANK_INT("screen", cps1_interrupt)

	MCFG_CPU_ADD("audiocpu", Z80, 3579545)
	MCFG_CPU_PROGRAM_MAP(sub_map)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(59.61) /* verified on one of the input gates of the 74ls08@4J on GNG romboard 88620-b-2 */
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(8*8, (64-8)*8-1, 2*8, 30*8-1 )
	MCFG_SCREEN_UPDATE(cps1)
	MCFG_SCREEN_EOF(cps1)

	MCFG_GFXDECODE(cps1)
	MCFG_PALETTE_LENGTH(0xc00)

	MCFG_VIDEO_START(cps1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("2151", YM2151, 3579545)
	MCFG_SOUND_CONFIG(ym2151_config)
	MCFG_SOUND_ROUTE(0, "mono", 0.35)
	MCFG_SOUND_ROUTE(1, "mono", 0.35)

	MCFG_SOUND_ADD("oki", OKIM6295, 1000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( cps1_12MHz, cps1_10MHz )

	/* basic machine hardware */

	MCFG_CPU_REPLACE("maincpu", M68000, 12000000)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( qsound, cps1_12MHz )

	/* basic machine hardware */

	MCFG_CPU_REPLACE("maincpu", M68000, 12000000)	// 12MHz verified
	MCFG_CPU_PROGRAM_MAP(qsound_main_map)
	MCFG_CPU_VBLANK_INT("screen", cps1_qsound_interrupt)  /* ??? interrupts per frame */

	MCFG_CPU_REPLACE("audiocpu", Z80, 8000000)
	MCFG_CPU_PROGRAM_MAP(qsound_sub_map)
	MCFG_CPU_PERIODIC_INT(irq0_line_hold, 250)	/* ?? */

	/* sound hardware */
	MCFG_DEVICE_REMOVE("mono")
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_DEVICE_REMOVE("2151")
	MCFG_DEVICE_REMOVE("oki")

	MCFG_SOUND_ADD("qsound", QSOUND, QSOUND_CLOCK)
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_EEPROM_ADD("eeprom", qsound_eeprom_interface)
MACHINE_CONFIG_END

/***************************************

    Driver Initialisation

****************************************/


static DRIVER_INIT( wof )
{
	wof_decode(machine);
	DRIVER_INIT_CALL(cps1);
}

/***************************************

    Roms

***************************************/

#define CODE_SIZE 0x400000

ROM_START( sfach )
	ROM_REGION( CODE_SIZE, "maincpu", 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfach23",        0x000000, 0x80000, CRC(02a1a853) SHA1(d92b9e774844fdcc9d9946b3e892b021e672d876) )
	ROM_LOAD16_WORD_SWAP( "sfza22",         0x080000, 0x80000, CRC(8d9b2480) SHA1(405305c1572908d00eab735f28676fbbadb4fac6) )
	ROM_LOAD16_WORD_SWAP( "sfzch21",        0x100000, 0x80000, CRC(5435225d) SHA1(6b1156fd82d0710e244ede39faaae0847c598376) )
	ROM_LOAD16_WORD_SWAP( "sfza20",         0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION( 0x800000, "gfx", 0 )
	ROMX_LOAD( "sfz01",         0x000000, 0x80000, CRC(0dd53e62) SHA1(5f3bcf5ca0fd564d115fe5075a4163d3ee3226df) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz02",         0x000002, 0x80000, CRC(94c31e3f) SHA1(2187b3d4977514f2ae486eb33ed76c86121d5745) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz03",         0x000004, 0x80000, CRC(9584ac85) SHA1(bbd62d66b0f6909630e801ce5d6331d43f44d741) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz04",         0x000006, 0x80000, CRC(b983624c) SHA1(841106bb9453e3dfb7869c4b0e9149cc610d515a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz05",         0x200000, 0x80000, CRC(2b47b645) SHA1(bc6426eff5df9417f32666586744626fa544f7b5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz06",         0x200002, 0x80000, CRC(74fd9fb1) SHA1(7945472591f3c06970e96611a0363ed8f3d52c36) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz07",         0x200004, 0x80000, CRC(bb2c734d) SHA1(97a06935f86f31755d2ffdc5b56bef53944bdecd) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz08",         0x200006, 0x80000, CRC(454f7868) SHA1(eecccba7542d893bc41676246a20aa4914b79bbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz10",         0x400000, 0x80000, CRC(2a7d675e) SHA1(0144ba34a29fb08b41c780ce65bb06d25724e88f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz11",         0x400002, 0x80000, CRC(e35546c8) SHA1(7b08aa3413494d12c5c550263a5f00b64b98e6ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz12",         0x400004, 0x80000, CRC(f122693a) SHA1(71ce901d8d30207e506b6a8d6a4e0fcf3a1b0eac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz13",         0x400006, 0x80000, CRC(7cf942c8) SHA1(a7109facb97a8a11ddf1b4e07de6ff3164d713a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz14",         0x600000, 0x80000, CRC(09038c81) SHA1(3461d70902fbfb92ce40f804be6388276a01d153) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz15",         0x600002, 0x80000, CRC(1aa17391) SHA1(b4d0f760a430b7fc4443b6c94da2659315c5b926) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz16",         0x600004, 0x80000, CRC(19a5abd6) SHA1(73ba1de15c883fdc69fd7dccdb58d00ca512d4ea) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz17",         0x600006, 0x80000, CRC(248b3b73) SHA1(95810a17b1caf6372b33ed3e4ee8a7e51482c70d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, "stars", 0 )
	ROM_COPY( "gfx", 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, "audiocpu", 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz09",         0x00000, 0x08000, CRC(c772628b) SHA1(ebc5b7c173caf1e151f733f23c1b20abec24e16d) )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, "oki", 0 )	/* Samples */
	ROM_LOAD( "sfz18",         0x00000, 0x20000, CRC(61022b2d) SHA1(6369d0c1d08a30ee19b94e52ab1463a7784b9de5) )
	ROM_LOAD( "sfz19",         0x20000, 0x20000, CRC(3b5886d5) SHA1(7e1b7d40ef77b5df628dd663d45a9a13c742cf58) )
ROM_END

ROM_START( sfzbch )
	ROM_REGION( CODE_SIZE, "maincpu", 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzbch23",       0x000000, 0x80000, CRC(53699f68) SHA1(d7f132faf8c31b5e79c32e6b0cce45377ec8d474) )
	ROM_LOAD16_WORD_SWAP( "sfza22",         0x080000, 0x80000, CRC(8d9b2480) SHA1(405305c1572908d00eab735f28676fbbadb4fac6) )
	ROM_LOAD16_WORD_SWAP( "sfzch21",        0x100000, 0x80000, CRC(5435225d) SHA1(6b1156fd82d0710e244ede39faaae0847c598376) )
	ROM_LOAD16_WORD_SWAP( "sfza20",         0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb) )

	ROM_REGION( 0x800000, "gfx", 0 )
	ROMX_LOAD( "sfz01",         0x000000, 0x80000, CRC(0dd53e62) SHA1(5f3bcf5ca0fd564d115fe5075a4163d3ee3226df) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz02",         0x000002, 0x80000, CRC(94c31e3f) SHA1(2187b3d4977514f2ae486eb33ed76c86121d5745) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz03",         0x000004, 0x80000, CRC(9584ac85) SHA1(bbd62d66b0f6909630e801ce5d6331d43f44d741) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz04",         0x000006, 0x80000, CRC(b983624c) SHA1(841106bb9453e3dfb7869c4b0e9149cc610d515a) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz05",         0x200000, 0x80000, CRC(2b47b645) SHA1(bc6426eff5df9417f32666586744626fa544f7b5) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz06",         0x200002, 0x80000, CRC(74fd9fb1) SHA1(7945472591f3c06970e96611a0363ed8f3d52c36) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz07",         0x200004, 0x80000, CRC(bb2c734d) SHA1(97a06935f86f31755d2ffdc5b56bef53944bdecd) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz08",         0x200006, 0x80000, CRC(454f7868) SHA1(eecccba7542d893bc41676246a20aa4914b79bbc) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz10",         0x400000, 0x80000, CRC(2a7d675e) SHA1(0144ba34a29fb08b41c780ce65bb06d25724e88f) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz11",         0x400002, 0x80000, CRC(e35546c8) SHA1(7b08aa3413494d12c5c550263a5f00b64b98e6ab) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz12",         0x400004, 0x80000, CRC(f122693a) SHA1(71ce901d8d30207e506b6a8d6a4e0fcf3a1b0eac) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz13",         0x400006, 0x80000, CRC(7cf942c8) SHA1(a7109facb97a8a11ddf1b4e07de6ff3164d713a1) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz14",         0x600000, 0x80000, CRC(09038c81) SHA1(3461d70902fbfb92ce40f804be6388276a01d153) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz15",         0x600002, 0x80000, CRC(1aa17391) SHA1(b4d0f760a430b7fc4443b6c94da2659315c5b926) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz16",         0x600004, 0x80000, CRC(19a5abd6) SHA1(73ba1de15c883fdc69fd7dccdb58d00ca512d4ea) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz17",         0x600006, 0x80000, CRC(248b3b73) SHA1(95810a17b1caf6372b33ed3e4ee8a7e51482c70d) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, "stars", 0 )
	ROM_COPY( "gfx", 0x000000, 0x000000, 0x8000 )	/* stars */

	ROM_REGION( 0x18000, "audiocpu", 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz09",         0x00000, 0x08000, CRC(c772628b) SHA1(ebc5b7c173caf1e151f733f23c1b20abec24e16d) )
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, "oki", 0 )	/* Samples */
	ROM_LOAD( "sfz18",         0x00000, 0x20000, CRC(61022b2d) SHA1(6369d0c1d08a30ee19b94e52ab1463a7784b9de5) )
	ROM_LOAD( "sfz19",         0x20000, 0x20000, CRC(3b5886d5) SHA1(7e1b7d40ef77b5df628dd663d45a9a13c742cf58) )
ROM_END

ROM_START( sfzch )
	ROM_REGION( CODE_SIZE, "maincpu",0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzch23",        0x000000, 0x80000, CRC(1140743f) SHA1(10bcedb5cca266f2aa3ed99ede6f9a64fc877539))
	ROM_LOAD16_WORD_SWAP( "sfza22",         0x080000, 0x80000, CRC(8d9b2480) SHA1(405305c1572908d00eab735f28676fbbadb4fac6))
	ROM_LOAD16_WORD_SWAP( "sfzch21",        0x100000, 0x80000, CRC(5435225d) SHA1(6b1156fd82d0710e244ede39faaae0847c598376))
	ROM_LOAD16_WORD_SWAP( "sfza20",         0x180000, 0x80000, CRC(806e8f38) SHA1(b6d6912aa8f2f590335d7ff9a8214648e7131ebb))

	ROM_REGION( 0x800000, "gfx", 0 )
	ROMX_LOAD( "sfz01",         0x000000, 0x80000, CRC(0dd53e62) SHA1(5f3bcf5ca0fd564d115fe5075a4163d3ee3226df), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz02",         0x000002, 0x80000, CRC(94c31e3f) SHA1(2187b3d4977514f2ae486eb33ed76c86121d5745), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz03",         0x000004, 0x80000, CRC(9584ac85) SHA1(bbd62d66b0f6909630e801ce5d6331d43f44d741), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz04",         0x000006, 0x80000, CRC(b983624c) SHA1(841106bb9453e3dfb7869c4b0e9149cc610d515a), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz05",         0x200000, 0x80000, CRC(2b47b645) SHA1(bc6426eff5df9417f32666586744626fa544f7b5), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz06",         0x200002, 0x80000, CRC(74fd9fb1) SHA1(7945472591f3c06970e96611a0363ed8f3d52c36), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz07",         0x200004, 0x80000, CRC(bb2c734d) SHA1(97a06935f86f31755d2ffdc5b56bef53944bdecd), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz08",         0x200006, 0x80000, CRC(454f7868) SHA1(eecccba7542d893bc41676246a20aa4914b79bbc), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz10",         0x400000, 0x80000, CRC(2a7d675e) SHA1(0144ba34a29fb08b41c780ce65bb06d25724e88f), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz11",         0x400002, 0x80000, CRC(e35546c8) SHA1(7b08aa3413494d12c5c550263a5f00b64b98e6ab), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz12",         0x400004, 0x80000, CRC(f122693a) SHA1(71ce901d8d30207e506b6a8d6a4e0fcf3a1b0eac), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz13",         0x400006, 0x80000, CRC(7cf942c8) SHA1(a7109facb97a8a11ddf1b4e07de6ff3164d713a1), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz14",         0x600000, 0x80000, CRC(09038c81) SHA1(3461d70902fbfb92ce40f804be6388276a01d153), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz15",         0x600002, 0x80000, CRC(1aa17391) SHA1(b4d0f760a430b7fc4443b6c94da2659315c5b926), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz16",         0x600004, 0x80000, CRC(19a5abd6) SHA1(73ba1de15c883fdc69fd7dccdb58d00ca512d4ea), ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz17",         0x600006, 0x80000, CRC(248b3b73) SHA1(95810a17b1caf6372b33ed3e4ee8a7e51482c70d), ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x8000, "stars", 0 )
	ROM_COPY( "gfx", 0x000000, 0x000000, 0x8000 )

	ROM_REGION( 0x28000, "audiocpu",0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz09",         0x00000, 0x08000, CRC(c772628b) SHA1(ebc5b7c173caf1e151f733f23c1b20abec24e16d))
	ROM_CONTINUE(              0x10000, 0x08000 )

	ROM_REGION( 0x40000, "oki",0 )	/* Samples */
	ROM_LOAD( "sfz18",         0x00000, 0x20000, CRC(61022b2d) SHA1(6369d0c1d08a30ee19b94e52ab1463a7784b9de5))
	ROM_LOAD( "sfz19",         0x20000, 0x20000, CRC(3b5886d5) SHA1(7e1b7d40ef77b5df628dd663d45a9a13c742cf58))
ROM_END

ROM_START( wofch )
	ROM_REGION( CODE_SIZE, "maincpu", 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "wofch_23.rom",  0x000000, 0x80000, CRC(4e0b8dee) SHA1(d2fb716d62b7a259f46bbc74c1976a18d56696ea) )
	ROM_LOAD16_WORD_SWAP( "wofch_22.rom",  0x080000, 0x80000, CRC(d0937a8d) SHA1(01d7be446e2e3ef8ca767f59c178240dfd52dd93) )

	ROM_REGION( 0x400000, "gfx", 0 )
	ROMX_LOAD( "tk2_gfx1.rom",   0x000000, 0x80000, CRC(0d9cb9bf) SHA1(cc7140e9a01a14b252cb1090bcea32b0de461928) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx3.rom",   0x000002, 0x80000, CRC(45227027) SHA1(b21afc593f0d4d8909dfa621d659cbb40507d1b2) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx2.rom",   0x000004, 0x80000, CRC(c5ca2460) SHA1(cbe14867f7b94b638ca80db7c8e0c60881183469) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk2_gfx4.rom",   0x000006, 0x80000, CRC(e349551c) SHA1(1d977bdf256accf750ad9930ec4a0a19bbf86964) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk205.bin",      0x200000, 0x80000, CRC(e4a44d53) SHA1(b747679f4d63e5e62d9fd81b3120fba0401fadfb) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk206.bin",      0x200002, 0x80000, CRC(58066ba8) SHA1(c93af968e21094d020e4b2002e0c6fc0d746af0b) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "wofch_06.rom",   0x200004, 0x80000, CRC(cc9006c9) SHA1(cfcbec3a67052268a7739538aa28a6391fe5400e) , ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tk208.bin",      0x200006, 0x80000, CRC(d4a19a02) SHA1(ff396b1d33d9b4842140f2c6d085fe05748e3244) , ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( 0x28000, "audiocpu", 0 ) /* QSound Z80 code */
	ROM_LOAD( "tk2_qa.rom",     0x00000, 0x08000, CRC(c9183a0d) SHA1(d8b1d41c572f08581f8ab9eb878de77d6ea8615d) )
	ROM_CONTINUE(               0x10000, 0x18000 )

	ROM_REGION( 0x200000, "qsound", 0 ) /* QSound samples */
	ROM_LOAD( "tk2_q1.rom",     0x000000, 0x80000, CRC(611268cf) SHA1(83ab059f2110fb25fdcff928d56b790fc1f5c975) )
	ROM_LOAD( "tk2_q2.rom",     0x080000, 0x80000, CRC(20f55ca9) SHA1(90134e9a9c4749bb65c728b66ea4dac1fd4d88a4) )
	ROM_LOAD( "tk2_q3.rom",     0x100000, 0x80000, CRC(bfcf6f52) SHA1(2a85ff3fc89b4cbabd20779ec12da2e116333c7c) )
	ROM_LOAD( "tk2_q4.rom",     0x180000, 0x80000, CRC(36642e88) SHA1(8ab25b19e2b67215a5cb1f3aa81b9d26009cfeb8) )
ROM_END

/***************************************************************************

    Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT     INIT     COMPANY     FULLNAME */
CONS( 1995, sfach,    sfzch,    0,	cps1_10MHz, sfzch,    cps1,        "Capcom", "CPS Changer - Street Fighter Alpha - Warriors' Dreams (Publicity US 950727)", 0 )
CONS( 1995, sfzch,    0,        0,	cps1_10MHz, sfzch,    cps1,        "Capcom", "CPS Changer - Street Fighter Zero (Japan 951020)", 0 )
CONS( 1995, sfzbch,   sfzch,    0,	cps1_10MHz, sfzch,    cps1,        "Capcom", "CPS Changer - Street Fighter Zero (Brazil 950727)", 0 )
CONS( 1995, wofch,    0,        0,	qsound,     sfzch,    wof,         "Capcom", "CPS Changer - Tenchi Wo Kurau II (Japan 921031)", 0 )
