/***************************************************************************

        DEC VK 100

        12/05/2009 Skeleton driver.
        28/07/2009 added Guru-readme(TM)

        Todo: attach vblank int
              attach hd46505sp CRTC
              emulate vector generator hardware
              attach keyboard
              calculate or measure the correct beeper frequency
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
|      PR5        INTEL           ROM1                                 |
|         PR1 PR6 P8251A                           4116 4116 4116 4116 |
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

    LED meanings:
    The LEDS on the vk100 (there are 7) are set up above the keyboard as:
    Label: ON LINE   LOCAL     NO SCROLL BASIC     HARD-COPY L1        L2
    Bit:   !d5       d5        !d4       !d3       !d2       !d1       !d0 (of port 0x68)
according to manual from http://www.bitsavers.org/pdf/dec/terminal/gigi/EK-VK100-IN-002_GIGI_Terminal_Installation_and_Owners_Manual_Apr81.pdf
where X = on, 0 = off, ? = variable (- = off)
    - X 0 0 0 0 0 (0x1F) = Microprocessor error
    X - 0 X X X X (0x30) "

    - X 0 0 0 0 X (0x1E) = ROM error
    X - 0 0 ? ? ? (0x3x) "
1E 3F = rom error, rom 1 (0000-0fff)
1E 3E = rom error, rom 1 (1000-1fff)
1E 3D = rom error, rom 2 (2000-2fff)
1E 3C = rom error, rom 2 (3000-3fff)
1E 3B = rom error, rom 3 (4000-4fff)
1E 3A = rom error, rom 3 (5000-5fff)
1E 39 = rom error, rom 4 (6000-6fff)

    - X 0 0 0 X 0 (0x1D) = RAM error
    X - 0 ? ? ? ? (0x3x) "

    - X 0 0 0 X X (0x1C) = CRT Controller error
    X - 0 X X X X (0x30) "

    - X 0 0 X 0 0 (0x1B) = CRT Controller time-out
    X - 0 X X X X (0x30) "

    - X 0 0 X 0 X (0x1A) = Vector time-out error
    X - 0 X X X X (0x30) "

*/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sound/beep.h"
#include "video/mc6845.h"
#include "machine/i8251.h"

class vk100_state : public driver_device
{
public:
	vk100_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen")//,
		//m_crtc(*this, "hd46505sp"),
		//m_uart(*this, "i8251")
		{ }
	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;
	//required_device<mc6845_device> m_crtc;
	//required_device<device_t> m_uart;

	//UINT8 m_statusLED;
	UINT8 m_key_scan;
	DECLARE_WRITE8_MEMBER(led_beep_write);
};

WRITE8_MEMBER(vk100_state::led_beep_write)
{
	device_t *speaker = machine().device(BEEPER_TAG);
	beep_set_frequency( speaker, 318 ); //318 hz is a guess based on vt100
	/* The LEDS on the vk100 (there are 7) are set up above the keyboard as:
     * Label: ON LINE   LOCAL     NO SCROLL BASIC     HARD-COPY L1        L2
     * Bit:   !d5       d5        !d4       !d3       !d2       !d1       !d0
     * see header of file for information on error codes
     */
	/* TODO: make this work.
    output_set_value("online_led",BIT(data, 5) ? 0 : 1);
    output_set_value("local_led", BIT(data, 5));
    output_set_value("locked_led",BIT(data, 4) ? 0 : 1);
    output_set_value("noscroll_led", BIT(data, 3) ? 0 : 1);
    output_set_value("hardcopy_led", BIT(data, 2) ? 0 : 1);
    output_set_value("l1_led", BIT(data, 1) ? 0 : 1);
    output_set_value("l2_led", BIT(data, 0) ? 0 : 1);
    */
	m_key_scan = BIT(data, 6);
	beep_set_state( speaker, BIT(data, 7));
	//m_statusLED = data&0xFF;
	logerror("LED state: %02X: %s %s %s %s %s %s\n", data&0xFF, (data&0x20)?"------- LOCAL ":"ON LINE ----- ", (data&0x10)?"--------- ":"NO SCROLL ", (data&0x8)?"----- ":"BASIC ", (data&0x4)?"--------- ":"HARD-COPY ", (data&0x2)?"-- ":"L1 ", (data&0x1)?"-- ":"L2 ");
	//popmessage("LED write: %02X\n", data&0xFF);
	popmessage("LED state: %02X: %s %s %s %s %s %s\n", data&0xFF, (data&0x20)?"------- LOCAL ":"ON LINE ----- ", (data&0x10)?"--------- ":"NO SCROLL ", (data&0x8)?"----- ":"BASIC ", (data&0x4)?"--------- ":"HARD-COPY ", (data&0x2)?"-- ":"L1 ", (data&0x1)?"-- ":"L2 ");
	//popmessage("LED status: %x %x %x %x %x %x %x %x\n", (data&0x80), (data&0x40), (data&0x20), data&0x10, data&0x8, data&0x4, data&0x2, data&0x1);
}

static ADDRESS_MAP_START(vk100_mem, AS_PROGRAM, 8, vk100_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x6fff ) AM_ROM
	//AM_RANGE( 0x7000, 0x700f) AM_DEVREADWRITE(vk100_keyboard) AM_MIRROR(0x0ff0)
	AM_RANGE( 0x8000, 0xbfff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(vk100_io, AS_IO, 8, vk100_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_RAM // hack to pass crtc test
	//AM_RANGE(0x00, 0x00) AM_WRITE(crtc_addr_w)
	//AM_RANGE(0x01, 0x01) AM_READWRITE(crtc_data_r, crtc_data_w)
	// Comments are from page 118 (5-14) of http://web.archive.org/web/20091015205827/http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf
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
	AM_RANGE (0x68, 0x68) AM_WRITE(led_beep_write) //keyboard LEDs and beeper
	//AM_RANGE (0x6C, 0x6C) AM_WRITE(baud)   //LD BAUD (baud rate clock divider setting for i8251 tx and rx clocks)
	//AM_RANGE (0x70, 0x70) AM_WRITE(comd)   //LD COMD (one of the i8251 regs)
	//AM_RANGE (0x71, 0x71) AM_WRITE(com)    //LD COM (the other i8251 reg)
	//AM_RANGE (0x74, 0x74) AM_WRITE(unknown_74)
	//AM_RANGE (0x78, 0x78) AM_WRITE(kbdw)   //KBDW
	//AM_RANGE (0x7C, 0x7C) AM_WRITE(unknown_7C)
	//AM_RANGE (0x40, 0x40) AM_READ(systat_a) // SYSTAT A (dipswitches?)
	//AM_RANGE (0x48, 0x48) AM_READ(systat_b) // SYSTAT B (dipswitches?)
	//AM_RANGE (0x50, 0x50) AM_READ(uart_0)   // UART O , low 2 bits control the i8251 dest: 00 = rs232/eia, 01 = 20ma, 02 = hardcopy, 03 = test/loopback
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


static MACHINE_RESET( vk100 )
{
}

static VIDEO_START( vk100 )
{
}

static SCREEN_UPDATE_IND16( vk100 )
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
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_VIDEO_START(vk100)
	MCFG_SCREEN_UPDATE_STATIC(vk100)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( BEEPER_TAG, BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 0.25 )
MACHINE_CONFIG_END

/* ROM definition */
/* according to http://www.computer.museum.uq.edu.au/pdf/EK-VK100-TM-001%20VK100%20Technical%20Manual.pdf page 5-10 (pdf pg 114),
The 4 firmware roms should go from 0x0000-0x1fff, 0x2000-0x3fff, 0x4000-0x5fff and 0x6000-0x63ff; The last rom is actually a little bit longer and goes to 6fff.
*/
ROM_START( vk100 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-031e4-00.rom1.ic51", 0x0000, 0x2000, CRC(c8596398) SHA1(a8dc833dcdfb7550c030ac3d4143e266b1eab03a))
	ROM_LOAD( "23-017e4-00.rom2.ic52", 0x2000, 0x2000, CRC(e857a01e) SHA1(914b2c51c43d0d181ffb74e3ea59d74e70ab0813))
	ROM_LOAD( "23-018e4-00.rom3.ic53", 0x4000, 0x2000, CRC(b3e7903b) SHA1(8ad6ed25cd9b04a9968aa09ab69ba526d35ca550))
	ROM_LOAD( "23-190e2-00.rom4.ic54", 0x6000, 0x1000, CRC(ad596fa5) SHA1(b30a24155640d32c1b47a3a16ea33cd8df2624f6))

	ROM_REGION( 0x10000, "proms", ROMREGION_ERASEFF )
	// this is either the "SYNC ROM" or the "VECTOR ROM" which handles timing related stuff. (256*4, 82s129)
	ROM_LOAD( "wb8151_573a2.6301.pr3.ic44", 0x0000, 0x0100, CRC(75885a9f) SHA1(c721dad6a69c291dd86dad102ed3a8ddd620ecc4))
	// this is probably the "DIRECTION ROM", but might not be. (256*8, 82s135)
	ROM_LOAD( "wb8146_058b1.6309.pr1.ic99", 0x0100, 0x0100, CRC(71b01864) SHA1(e552f5b0bc3f443299282b1da7e9dbfec60e12bf))
	// this is definitely the "TRANSLATOR ROM" described in figure 5-17 on page 5-27 (256*8, 82s135)
	ROM_LOAD( "wb---0_090b1.6309.pr2.ic77", 0x0200, 0x0100, CRC(198317fc) SHA1(00e97104952b3fbe03a4f18d800d608b837d10ae)) // label is horribly unclear, could be 090b1 or 000b1 or 060b1
	// this is definitely the "PATTERN ROM", (1k*4, 82s137)
	ROM_LOAD( "wb8201_655a-.m1-7643-5.pr4.ic17", 0x0300, 0x0400, CRC(e8ecf59f) SHA1(49e9d109dad3d203d45471a3f4ca4985d556161f)) // label is unclear, may be type a4 (= 1024x4 assuming a3 = 512x4 and a2 = 256x4)
	// the following == mb6309 (256x8, 82s135)
	ROM_LOAD( "wb8141_059b1.tbp18s22n.pr5.ic108", 0x0700, 0x0100, NO_DUMP)
	// the following = mb6331 (32x8, 82s123)
	ROM_LOAD( "wb8214_297a1.74s288.pr6.ic89", 0x0800, 0x0100, NO_DUMP)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                       FULLNAME       FLAGS */
COMP( 1980, vk100,  0,      0,       vk100,     vk100,   0,  "Digital Equipment Corporation", "VK 100", GAME_NOT_WORKING | GAME_NO_SOUND)
