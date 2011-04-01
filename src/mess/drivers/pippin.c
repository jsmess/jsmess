/***************************************************************************

        Bandai Pippin

        17/07/2009 Skeleton driver.

    Apple ASICs identified:
    -----------------------
    343S1125    Grand Central (SWIM III, Sound, VIA)
    341S0060    Cuda (68HC05 MCU, handles ADB and parameter ("CMOS") RAM)
    343S1146    ??? (likely SCSI due to position on board)
    343S1191(x2) Athens Prime PLL Clock Generator


    Other chips
    -----------
    Z8530 SCC
    CS4217 audio DAC
    Bt856 video DAC

    PowerMac 6500 partial memory map (Pippin should be similar)
    These are base addresses; Apple typically mirrors each chip over at least a 0x1000 byte range

    F3010000 : SCSI
    F3012000 : 8530 SCC
    F3015000 : SWIM III (Apple custom NEC765 derivative with GCR support, supposedly)
    F3016000 : 6522 VIA (look at machine/mac.c for Apple's unique register mapping)

****************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"
#include "imagedev/chd_cd.h"
#include "sound/cdda.h"


class pippin_state : public driver_device
{
public:
	pippin_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 m_unk1_test;
	UINT8 m_portb_data;
};


static READ64_HANDLER( unk1_r )
{
	pippin_state *state = space->machine().driver_data<pippin_state>();
	state->m_unk1_test ^= 0x0400; //PC=ff808760

	return state->m_unk1_test << 16 | 0;
}

static READ64_HANDLER( unk2_r )
{
	if(ACCESSING_BITS_32_47)
		return (UINT64)0xe1 << 32; //PC=fff04810

	return 0;
}


static READ64_HANDLER( adb_portb_r )
{
	pippin_state *state = space->machine().driver_data<pippin_state>();
	if(ACCESSING_BITS_56_63)
	{
		if(state->m_portb_data == 0x10)
			return (UINT64)0x08 << 56;

		if(state->m_portb_data == 0x38)	//fff04c80
			return (UINT64)0x20 << 56; //almost sure this is wrong

		//printf("PORTB R %02x\n",state->m_portb_data);


		return 0;
	}

	return 0;
}

static WRITE64_HANDLER( adb_portb_w )
{
	pippin_state *state = space->machine().driver_data<pippin_state>();
	if(ACCESSING_BITS_56_63)
	{
		state->m_portb_data = (UINT64)data >> 56;
	}
}

static ADDRESS_MAP_START( pippin_mem, AS_PROGRAM, 64 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x005fffff) AM_RAM

	/* writes at 0x0*c01000 the string "Mr. Kesh" and wants it to be read back, true color VRAMs perhaps? */
	AM_RANGE(0x00c00000, 0x00c01007) AM_RAM
	AM_RANGE(0x01c00000, 0x01c01007) AM_RAM
	AM_RANGE(0x02c00000, 0x02c01007) AM_RAM
	AM_RANGE(0x03c00000, 0x03c01007) AM_RAM

	AM_RANGE(0xf00dfff8, 0xf00dffff) AM_READ(unk2_r)

	AM_RANGE(0xf3008800, 0xf3008807) AM_READ(unk1_r)
	AM_RANGE(0xf3016000, 0xf3016007) AM_READWRITE(adb_portb_r,adb_portb_w) // ADB PORTB
	AM_RANGE(0xf3016400, 0xf3016407) AM_NOP // ADB DDRB
	AM_RANGE(0xf3016600, 0xf3016607) AM_NOP // ?
	AM_RANGE(0xf3016800, 0xf3016807) AM_NOP // ?
	AM_RANGE(0xf3016a00, 0xf3016a07) AM_NOP // ?
	AM_RANGE(0xf3016c00, 0xf3016c07) AM_NOP // ?
	AM_RANGE(0xf3017400, 0xf3017407) AM_NOP // ADB SHR
	AM_RANGE(0xf3017600, 0xf3017607) AM_NOP // ADB ACR
	AM_RANGE(0xf3017800, 0xf3017807) AM_NOP // ADB PCR
	AM_RANGE(0xf3017a00, 0xf3017a07) AM_NOP // ADB IFR
	AM_RANGE(0xf3017c00, 0xf3017c07) AM_NOP // ADB IER
	AM_RANGE(0xf3017e00, 0xf3017e07) AM_NOP // ?
	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pippin )
INPUT_PORTS_END


static MACHINE_RESET(pippin)
{
}

static VIDEO_START( pippin )
{
}

static SCREEN_UPDATE( pippin )
{
    return 0;
}

static MACHINE_CONFIG_START( pippin, pippin_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",PPC603, 66000000)
    MCFG_CPU_PROGRAM_MAP(pippin_mem)

    MCFG_MACHINE_RESET(pippin)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(pippin)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(pippin)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_SOUND_ADD( "cdda", CDDA, 0 )
	MCFG_SOUND_ROUTE( 0, "lspeaker", 1.00 )
	MCFG_SOUND_ROUTE( 1, "rspeaker", 1.00 )

	MCFG_CDROM_ADD("cdrom")
MACHINE_CONFIG_END

/* ROM definition */
/*

    BIOS versions

    dev
    monitor 341S0241 - 245,247,248,250
    1.0     341S0251..254               U1-U4
    1.2     341S0297..300               U15,U20,U31,U35
    1.3     341S0328..331               U1/U31, U2/U35, U3/U15 and U4/U20

*/

ROM_START( pippin )
    ROM_REGION( 0x400000, "user1",  ROMREGION_64BIT | ROMREGION_BE )
	ROM_SYSTEM_BIOS(0, "v1", "Kinka v 1.0")
    ROMX_LOAD( "341s0251.u1", 0x000006, 0x100000, CRC(aaea2449) SHA1(2f63e215260a42fb7c5f2364682d5e8c0604646f),ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(6) | ROM_BIOS(1))
	ROMX_LOAD( "341s0252.u2", 0x000004, 0x100000, CRC(3d584419) SHA1(e29c764816755662693b25f1fb3c24faef4e9470),ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(6) | ROM_BIOS(1))
	ROMX_LOAD( "341s0253.u3", 0x000002, 0x100000, CRC(d8ae5037) SHA1(d46ce4d87ca1120dfe2cf2ba01451f035992b6f6),ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(6) | ROM_BIOS(1))
	ROMX_LOAD( "341s0254.u4", 0x000000, 0x100000, CRC(3e2851ba) SHA1(7cbf5d6999e890f5e9ab2bc4b10ca897c4dc2016),ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(6) | ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "pre", "Kinka pre-release")
    ROMX_LOAD( "kinka-pre.rom", 0x000000, 0x400000, CRC(4ff875e6) SHA1(eb8739cab1807c6c7c51acc7f4a3afc1f9c6ddbb),ROM_BIOS(2))
	ROM_REGION( 0x10000, "cdrom", 0 ) /* MATSUSHITA CR504-L OEM */
	ROM_LOAD( "504par4.0i.ic7", 0x0000, 0x10000, CRC(25f7dd46) SHA1(ec3b3031742807924c6259af865e701827208fec) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1996, pippin,  0,       0,	pippin, 	pippin, 	 0,  "Apple / Bandai",   "Pippin @mark",		GAME_NOT_WORKING)
