/***************************************************************************

        Amstrad PenPad PDA 600

        05/11/2009 Skeleton driver.

    Hardware info:
    U1: CPU
        Z8S180
        Enhanced Z80 CPU
        1 MB MMU
        2 DMAs
        2 UARTs (up to 512 Kbps)
        Two 16-Bit Timers
        Clock Serial I/O

    U2: ROM
        contains OS (binary and disassembled source)
        apparently compatible to 27C1001 (128K*8)

    U3: RAM
        128K RAM, static
    U4: RTC
    U5: Amstrad ASIC (unknown functionality )
    U6, U7, U8, U9: Multiplexer
        quad channel dual multiplexer
    U11: RAM
        32K RAM, static
    U12: LCD Controller
    U21: 5V StepUp converter voltage converter
    U27: OA quad operational amplifier
    U30: RS232 Driver
    U31: Amstrad ASIC
    U32: FlipFlop Octal D-Flip-Flop
    U33: RAM
        32K RAM, static
    U35: Tone Dialer
        generates DTMF frequencies
    U36: unknown
    U37: Nand
        Dual Quad-Input Nand
    U43: Bus Buffer
        Octal Bus Buffer

    Connectors:
        LCD
        Digitizer
        Serial
        PCMCIA

    Additional info:
        Two Z8 for power management and character recognition

****************************************************************************/

#include "emu.h"
#include "cpu/z180/z180.h"


class pda600_state : public driver_device
{
public:
	pda600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(pda600_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_ROM
	AM_RANGE(0x20000, 0x3ffff) AM_RAM
	//AM_RANGE(0x90000, 0x92fff) AM_RAM // video ram
	AM_RANGE(0xf0000, 0xfffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pda600_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x3f) AM_NOP /* Z180 internal registers */
	//AM_RANGE(0x40, 0x7f) AM_NOP   /* Z180 internal registers */
	//AM_RANGE(0x80, 0x8f) AM_NOP   /* RTC */
	//AM_RANGE(0xC0, 0xC1) AM_NOP   /* LCD */

ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pda600 )
INPUT_PORTS_END


static MACHINE_RESET(pda600)
{
}

static VIDEO_START( pda600 )
{
}

static SCREEN_UPDATE( pda600 )
{
    return 0;
}

static MACHINE_CONFIG_START( pda600, pda600_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z180, XTAL_14_31818MHz)
    MCFG_CPU_PROGRAM_MAP(pda600_mem)
    MCFG_CPU_IO_MAP(pda600_io)

    MCFG_MACHINE_RESET(pda600)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(240, 320)
    MCFG_SCREEN_VISIBLE_AREA(0, 240-1, 0, 320-1)
    MCFG_SCREEN_UPDATE(pda600)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(pda600)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pda600 )
    ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pdarom.bin", 0x00000, 0x20000, CRC(f793a6c5) SHA1(ab14b0fdcedb927c66357368a2bfff605ba758fb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY    FULLNAME       FLAGS */
COMP( 1993, pda600,  0,       0,	pda600, 	pda600,  0, 	 "Amstrad plc",   "PenPad PDA 600",		GAME_NOT_WORKING | GAME_NO_SOUND)
