/***************************************************************************

        DEC VK 100

        12/05/2009 Skeleton driver.
        28/07/2009 added Guru-readme(TM)

        Todo: attach vblank int
              attach hd46505sp CRTC
              emulate vector generator hardware
              attach keyboard
****************************************************************************/
/*
DEC VK100
DEC, 1982

This is a VK100 terminal, otherwise known as a DEC Gigi graphics terminal.
There's a technical manual dated 1982 here:
http://www.computer.museum.uq.edu.au/./pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf

PCB Layout
----------

VK100 LOGICBOARD
    |-------|    |---------|  |---------|    |-| |-| |-|  |-|
|---|-POWER-|----|---EIA---|--|HARD-COPY|----|B|-|G|-|R|--|-|----------|
|                                                         BW   DSW(8)  |
|                                                             POWER    |
|                 PR2                                                  |
|                           HD46505SP              4116 4116 4116 4116 |
|                                                                      |
|                                                  4116 4116 4116 4116 |
|                 INTEL           ROM1                                 |
|          PR1    P8251A                           4116 4116 4116 4116 |
|                     45.6192MHz  ROM2                                 |
|                                                  4116 4116 4116 4116 |
| 4116 4116 4116  INTEL           ROM3  PR3                            |
|                 D8202A                                               |
| 4116 4116 4116       5.0688MHz  ROM4                       PR4       |
|                                                                      |
| 4116 4116       INTEL    SMC_5016T                            PIEZO  |
|                 D8085A                        IDC40   LM556   75452  |
|----------------------------------------------------------------------|
Notes:
      ROM1 - TP-01 (C) DEC 23-031E4-00 (M) SCM91276L 8114
      ROM2 - TP-01 (C) DEC 1980 23-017E4-00 MOSTEK MK36444N 8116
      ROM3 - TP-01 (C) MICROSOFT 1979 23-018E4-00 MOSTEK MK36445N 8113
      ROM4 - TP-01 (C) DEC 1980 23-190E2-00 P8316E AMD 35517 8117DPP
*/

#include "emu.h"
#include "cpu/i8085/i8085.h"


class vk100_state : public driver_device
{
public:
	vk100_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(vk100_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE( 0x0000, 0x67ff ) AM_ROM
	//AM_RANGE( 0x7000, 0x700f) AM_DEVREADWRITE(vk100_keyboard) AM_MIRROR(0x0ff0)
	AM_RANGE( 0x8000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vk100_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	// Comments are from page 118 (5-14) of http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf
	//AM_RANGE (0x40, 0x40) AM_WRITE(X_low)  //LD X LO \_ 12 bits
	//AM_RANGE (0x41, 0x41) AM_WRITE(X_high) //LD X HI /
	//AM_RANGE (0x42, 0x42) AM_WRITE(Y_low)  //LD Y LO \_ 12 bits
	//AM_RANGE (0x43, 0x43) AM_WRITE(Y_high) //LD Y HI /
	//AM_RANGE (0x44, 0x44) AM_WRITE(err)    //LD ERR
	//AM_RANGE (0x45, 0x45) AM_WRITE(sops)   //LD SOPS (screen options)
	//AM_RANGE (0x46, 0x46) AM_WRITE(pat)    //LD PAT (pattern register)
	//AM_RANGE (0x47, 0x47) AM_READWRITE(pmul)   //LD PMUL (pattern multiplier) (should this be write only?)
	//AM_RANGE (0x60, 0x60) AM_WRITE(du)     //LD DU
	//AM_RANGE (0x61, 0x61) AM_WRITE(dvm)    //LD DVM
	//AM_RANGE (0x62, 0x62) AM_WRITE(dir)    //LD DIR (direction)
	//AM_RANGE (0x63, 0x63) AM_WRITE(wops)   //LD WOPS (write options)
	//AM_RANGE (0x64, 0x64) AM_WRITE(mov)    //EX MOV
	//AM_RANGE (0x65, 0x65) AM_WRITE(dot)    //EX DOT
	//AM_RANGE (0x66, 0x66) AM_WRITE(vec)    //EX VEC
	//AM_RANGE (0x67, 0x67) AM_WRITE(er)     //EX ER
	//AM_RANGE (0x68, 0x68) AM_WRITE(unknown_68) // lots of writes go here; interrupt related?
	//AM_RANGE (0x6C, 0x6C) AM_WRITE(baud)   //LD BAUD
	//AM_RANGE (0x70, 0x70) AM_WRITE(comd)   //LD COMD
	//AM_RANGE (0x71, 0x71) AM_WRITE(com)    //LD COM
	//AM_RANGE (0x74, 0x74) AM_WRITE(unknown_74)
	//AM_RANGE (0x78, 0x78) AM_WRITE(kbdw)   //KBDW
	//AM_RANGE (0x7C, 0x7C) AM_WRITE(unknown_7C)
	//AM_RANGE (0x40, 0x40) AM_READ(systat_a) // SYSTAT A (dipswitches?)
	//AM_RANGE (0x48, 0x48) AM_READ(systat_b) // SYSTAT B (dipswitches?)
	//AM_RANGE (0x50, 0x50) AM_READ(uart_0)   // UART O
	//AM_RANGE (0x51, 0x51) AM_READ(uart_1)   // UAR
	//AM_RANGE (0x58, 0x58) AM_READ(unknown_58)
	//AM_RANGE (0x60, 0x60) AM_READ(unknown_60)
	//AM_RANGE (0x68, 0x68) AM_READ(unknown_68) // NOT USED
	//AM_RANGE (0x70, 0x70) AM_READ(unknown_70)
	//AM_RANGE (0x78, 0x7f) AM_READ(unknown_78)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vk100 )
INPUT_PORTS_END


static MACHINE_RESET(vk100)
{
}

static VIDEO_START( vk100 )
{
}

static SCREEN_UPDATE( vk100 )
{
    return 0;
}

static MACHINE_CONFIG_START( vk100, vk100_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I8085A, XTAL_5_0688MHz)
    MCFG_CPU_PROGRAM_MAP(vk100_mem)
    MCFG_CPU_IO_MAP(vk100_io)
    //MCFG_CPU_VBLANK_INT("screen", vk100_vertical_interrupt) // hook me up please

    MCFG_MACHINE_RESET(vk100)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(vk100)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(vk100)
MACHINE_CONFIG_END

/* ROM definition */
/* according to http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf page 5-10 (pdf pg 114),
The 4 firmware roms should go from 0x0000-0x1fff, 0x2000-0x3fff, 0x4000-0x5fff and 0x6000-0x63ff; The last rom is actually a little bit longer and goes to 67ff.
*/
ROM_START( vk100 )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "23-031e4-00.rom1", 0x0000, 0x2000, CRC(c8596398) SHA1(a8dc833dcdfb7550c030ac3d4143e266b1eab03a))
  ROM_LOAD( "23-017e4-00.rom2", 0x2000, 0x2000, CRC(e857a01e) SHA1(914b2c51c43d0d181ffb74e3ea59d74e70ab0813))
  ROM_LOAD( "23-018e4-00.rom3", 0x4000, 0x2000, CRC(b3e7903b) SHA1(8ad6ed25cd9b04a9968aa09ab69ba526d35ca550))
  ROM_LOAD( "23-190e2-00.rom4", 0x6000, 0x1000, CRC(ad596fa5) SHA1(b30a24155640d32c1b47a3a16ea33cd8df2624f6))
  ROM_REGION( 0x10000, "proms", ROMREGION_ERASEFF )
  ROM_LOAD( "6301.pr3", 0x0000, 0x0100, CRC(75885a9f) SHA1(c721dad6a69c291dd86dad102ed3a8ddd620ecc4)) // this is either the "SYNC ROM" or the "VECTOR ROM" which handles timing related stuff. or possibly both. (256*4)
  ROM_LOAD( "6309.pr1", 0x0100, 0x0100, CRC(71b01864) SHA1(e552f5b0bc3f443299282b1da7e9dbfec60e12bf)) // this is probably the "DIRECTION ROM", but might not be. (256*8)
  ROM_LOAD( "6309.pr2", 0x0200, 0x0100, CRC(198317fc) SHA1(00e97104952b3fbe03a4f18d800d608b837d10ae)) // this is definitely the "TRANSLATOR ROM" described in figure 5-17 on page 5-27 (256*8)
  ROM_LOAD( "7643.pr4", 0x0300, 0x0400, CRC(e8ecf59f) SHA1(49e9d109dad3d203d45471a3f4ca4985d556161f)) // this is definitely the "PATTERN ROM", (1k*4)

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1980, vk100,  0,       0, 	vk100,	vk100,	 0, 		 "Digital Equipment Corporation",   "VK 100",		GAME_NOT_WORKING | GAME_NO_SOUND)

