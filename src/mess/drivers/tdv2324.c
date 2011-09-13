/*

    Tandberg TDV2324

    Skeleton driver
    By Curt Coder with some work by Lord Nightmare

	Status:
	* Main cpu is currently hacked to read i/o port 0xE6 as 0x10;
	  it then seems to copy code to a ram area then jumps there
	  (there may be some sort of overlay/banking mess going on to allow full 64kb of ram)
	  The cpu gets stuck reading i/o port 0x30 in a loop.
	  - interrupts and sio lines are not hooked up
	* Sub cpu does a bunch of unknown i/o accesses and also tries to
	  sequentially read chunk of address space which it never writes to;
	  this seems likely to be a shared ram or i/o mapped area especially since it seems
	  to write to i/o port 0x60 before trying to read there.
	  - interrupts and sio lines are not hooked up
	* Fdc cpu starts, does a rom checksum (which passes) and tests a ram area
	  
	
	Board Notes:
	Mainboard (pictures P1010036 & P1010038)
	*28-pin: D27128, L4267096S,...(occluded by sticker: "965268 1")
	*40-pin: TMS9937NL, DB 336, ENGLAND
	*40-pin: P8085AH-2, F4265030, C INTEL '80 (there are 2 of these)
	*28-pin: JAPAN 8442, 00009SS0, HN4827128G-25 (sticker: "962107")
	*22-pin: ER3400, GI 8401HHA
	*  -pin: MOSTEK C 8424, MK3887N-4 (Z80 Serial I/O Controller)
	*20-pin: (sticker: "961420 0")
	*24-pin: D2716, L3263271, INTEL '77 (sticker: "962058 1")
	*3 tiny 16-pins which look socketed
	*+B8412, DMPAL10L8NC
	*PAL... (can't remove the sticker to read the rest since there's electrical components soldered above the chip)
	*Am27S21DC, 835IDmm
	*AM27S13DC, 8402DM (x2)
	*TBP28L22N, GERMANY 406 A (x2)
	*PAL16L6CNS, 8406

	FD/HD Interface Board (pictures P1010031 & P1010033)
	*28-pin: TMS, 2764JL-25, GHP8414
	*40-pin: MC68802P, R1H 8340
	*40-pin: WDC '79, FD1797PL-02, 8342 16
	*14-pin: MC4024P, MG 8341


*/
/*
'subcpu' (17CD): unmapped i/o memory write to 23 = 36 & FF
'subcpu' (17D1): unmapped i/o memory write to 23 = 76 & FF
'subcpu' (17D5): unmapped i/o memory write to 23 = B6 & FF
'subcpu' (17DB): unmapped i/o memory write to 20 = 1A & FF
'subcpu' (17DE): unmapped i/o memory write to 20 = 00 & FF
'subcpu' (17E0): unmapped i/o memory write to 3E = 00 & FF
'subcpu' (17E2): unmapped i/o memory write to 3A = 00 & FF
'subcpu' (17E6): unmapped i/o memory write to 30 = 74 & FF
'subcpu' (17EA): unmapped i/o memory write to 31 = 7F & FF
'subcpu' (17EE): unmapped i/o memory write to 32 = 6D & FF
'subcpu' (17F2): unmapped i/o memory write to 33 = 18 & FF
'subcpu' (17F6): unmapped i/o memory write to 34 = 49 & FF
'subcpu' (17FA): unmapped i/o memory write to 35 = 20 & FF
'subcpu' (17FE): unmapped i/o memory write to 36 = 18 & FF
'subcpu' (1801): unmapped i/o memory write to 3C = 00 & FF
'subcpu' (1803): unmapped i/o memory write to 3C = 00 & FF
'subcpu' (1805): unmapped i/o memory write to 3E = 00 & FF
'subcpu' (0884): unmapped i/o memory write to 10 = 97 & FF
'subcpu' (0888): unmapped i/o memory write to 10 = 96 & FF

'fdccpu' (E004): unmapped program memory read from 3C05 & FF
'fdccpu' (E007): unmapped program memory read from C000 & FF
'fdccpu' (E00A): unmapped program memory read from A000 & FF
'fdccpu' (E012): unmapped program memory write to F000 = D0 & FF
'fdccpu' (E015): unmapped program memory read from 3801 & FF
'fdccpu' (E018): unmapped program memory read from 3C06 & FF
'fdccpu' (E01B): unmapped program memory read from 3C04 & FF

'fdccpu' (E070): unmapped program memory write to 2101 = 01 & FF
'fdccpu' (E07C): unmapped program memory read from 6000 & FF
'fdccpu' (E07F): unmapped program memory read from 380D & FF
'fdccpu' (E082): unmapped program memory read from 380F & FF
'fdccpu' (E085): unmapped program memory read from 3803 & FF
'fdccpu' (E08B): unmapped program memory write to 6000 = 08 & FF
'fdccpu' (E08E): unmapped program memory write to 8000 = 08 & FF
'fdccpu' (E091): unmapped program memory write to 6000 = 00 & FF
'fdccpu' (E099): unmapped program memory write to F800 = 55 & FF
...
*/


#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "cpu/m6800/m6800.h"
#include "machine/s1410.h"
#include "video/tms9927.h"


class tdv2324_state : public driver_device
{
public:
	tdv2324_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_subcpu(*this, "subcpu"),
		  m_fdccpu(*this, "fdccpu")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_subcpu;
	required_device<cpu_device> m_fdccpu;
	DECLARE_READ8_MEMBER( tdv2324_main_io_e6 );
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

/* Read/Write handlers */

// Not sure what this is for, i/o read at 0xE6 on maincpu, post fails if it does not return bit 4 set
READ8_MEMBER( tdv2324_state::tdv2324_main_io_e6 )
{
	return 0x10; // TODO: this should actually return something meaningful, for now is enough to pass early boot test
}

/* Memory Maps */

static ADDRESS_MAP_START( tdv2324_mem, AS_PROGRAM, 8, tdv2324_state )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_ROM AM_REGION("maincpu", 0)
	/* when copying code to 4000 area it runs right off the end of rom;
	 * I'm not sure if its supposed to mirror or read as open bus */
	AM_RANGE(0x4000, 0x5fff) AM_RAM // 0x4000 has the boot code copied to it, 5fff and down are the stack
	AM_RANGE(0x6000, 0x6fff) AM_RAM // used by the relocated boot code; shared?
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdv2324_io, AS_IO, 8, tdv2324_state )
	//ADDRESS_MAP_GLOBAL_MASK(0xff)
	/* 0x30 is read by main code and if high bit isn't set at some point it will never get anywhere */
	/* e0, e2, e8, ea are written to */
	/* 30, e6 and e2 are readable */
	AM_RANGE(0xE6,0xE6) AM_READ(tdv2324_main_io_e6) // bit 4 is a status for something, post will not continue until it returns 1
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdv2324_sub_mem, AS_PROGRAM, 8, tdv2324_state )
	AM_RANGE(0x0000, 0x3fff) AM_ROM AM_REGION("subcpu", 0)
	AM_RANGE(0x4000, 0x47ff) AM_RAM // ram exists at least here, acts as stack?
	AM_RANGE(0x5000, 0x503f) AM_RAM // subcpu reads here (5000-5021) without writing first, implies this ram is shared
	AM_RANGE(0x6000, 0x7fff) AM_RAM // it floodfills this area with 0; is this a framebuffer? or shared ram?
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdv2324_sub_io, AS_IO, 8, tdv2324_state )
	//ADDRESS_MAP_GLOBAL_MASK(0xff)
	/* 20, 23, 30-36, 38, 3a, 3c, 3e, 60, 70 are written to */
	/* 60 may be a shared ram semaphore as it writes it immediately before reading the 5000 area */
	AM_RANGE(0x30, 0x3f) AM_DEVREADWRITE_LEGACY("tms9937", tms9927_r, tms9927_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tdv2324_fdc_mem, AS_PROGRAM, 8, tdv2324_state )
	AM_RANGE(0x0000, 0x001f) AM_RAM // on-6802-die ram (optinally battery backed)
	AM_RANGE(0x0020, 0x007f) AM_RAM // on-6802-die ram
	AM_RANGE(0x0080, 0x07ff) AM_RAM // the fdc cpu explicitly tests this area with A5,5A pattern after the rom check
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_REGION("fdccpu", 0)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tdv2324 )
INPUT_PORTS_END

/* Video */

void tdv2324_state::video_start()
{
}

bool tdv2324_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return 0;
}

static const tms9927_interface vtac_intf =
{
	"screen",
	8,
	NULL
};

static I8085_CONFIG( i8085_intf )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_NULL,	/* SID changed callback (I8085A only) */
	DEVCB_NULL	/* SOD changed callback (I8085A only) */
};

static I8085_CONFIG( i8085_sub_intf )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_NULL,	/* SID changed callback (I8085A only) */
	DEVCB_NULL	/* SOD changed callback (I8085A only) */
};

/* Machine Drivers */

static MACHINE_CONFIG_START( tdv2324, tdv2324_state )
	/* basic system hardware */
	MCFG_CPU_ADD("maincpu", I8085A, 8700000/2) // ???
	MCFG_CPU_PROGRAM_MAP(tdv2324_mem)
	MCFG_CPU_IO_MAP(tdv2324_io)
	MCFG_CPU_CONFIG(i8085_intf)

	MCFG_CPU_ADD("subcpu", I8085A, 8000000/2) // ???
	MCFG_CPU_PROGRAM_MAP(tdv2324_sub_mem)
	MCFG_CPU_IO_MAP(tdv2324_sub_io)
	MCFG_CPU_CONFIG(i8085_sub_intf)

	MCFG_CPU_ADD("fdccpu", M6802, 4000000)
	MCFG_CPU_PROGRAM_MAP(tdv2324_fdc_mem)

	/* video hardware */
	MCFG_SCREEN_ADD( "screen", RASTER )
	MCFG_SCREEN_REFRESH_RATE( 50 )
	MCFG_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MCFG_SCREEN_SIZE( 800, 400 )
	MCFG_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 400-1 )

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)
	
	MCFG_TMS9927_ADD("tms9937", XTAL_25_39836MHz, vtac_intf)

	// devices
	MCFG_S1410_ADD() // 104521-F ROM

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "tdv2324")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( tdv2324 )
	ROM_REGION( 0x800, "maincpu", 0 )
	ROM_LOAD( "962058-1.bin", 0x000, 0x800, CRC(3771aece) SHA1(36d3f03235f327d6c8682e5c167aed6dddfaa6ec) )

	ROM_REGION( 0x4000, "subcpu", 0 )
	ROM_LOAD( "962107-1.bin", 0x0000, 0x4000, CRC(29c1a139) SHA1(f55fa9075fdbfa6a3e94e5120270179f754d0ea5) )

	ROM_REGION( 0x2000, "fdccpu", 0 )
	ROM_LOAD( "962014-1.bin", 0x0000, 0x2000, CRC(d01c32cd) SHA1(1f00f5f5ff0c035eec6af820b5acb6d0c207b6db) )

	ROM_REGION( 0x4000, "chargen", 0 )
	ROM_LOAD( "965268-1.bin", 0x0000, 0x4000, CRC(7222a85f) SHA1(e94074b68d90698734ab1fc38d156407846df47c) )
ROM_END

#define rom_tdv2324m7 rom_tdv2324

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY           FULLNAME                FLAGS
COMP( 1983, tdv2324,		0,		0,		tdv2324,		tdv2324,		0,		"Tandberg",		"TDV 2324",		GAME_NOT_WORKING|GAME_NO_SOUND)
