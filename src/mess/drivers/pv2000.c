/*

CASIO PV-2000

(preliminary work by anondumper)
Thanks for the loaner (Ianoid)

NOTE:
The PCB has printed names of components, not ICXY, etc
but: "hn613128pc64.bin"

SEE
http://hou4gong1.mo-blog.jp/.shared/image.html?/photos/uncategorized/pv_2000_k1.jpg
http://hou4gong1.mo-blog.jp/.shared/image.html?/photos/uncategorized/pv_2000_14.jpg
http://hou4gong1.mo-blog.jp/.shared/image.html?/photos/uncategorized/pv_2000_15.jpg

Also See:
http://www2.odn.ne.jp/~haf09260/Pv2000/EnrPV.htm
For BIOS CRC confirmation
*/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "devices/cartslot.h"


static int last_state=0;

static WRITE8_HANDLER( pv2000_keys_w )
{
	switch(offset){
		default:break;
	}
}

static READ8_HANDLER( pv2000_keys_r )
{
	switch(offset){
		default:break;
	}

	return 0;
}


/* Memory Maps */

static ADDRESS_MAP_START( pv2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3FFF) AM_ROM

	AM_RANGE(0x4000, 0x4000) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x4001, 0x4001) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)

	AM_RANGE(0x7000, 0x7FFF) AM_RAM
  //AM_RANGE(0x8000, 0xBFFF) ext ram?
	AM_RANGE(0xC000, 0xFFFF) AM_ROM  //cart
ADDRESS_MAP_END


static ADDRESS_MAP_START( pv2000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)

	//theres also printer and tape I/O (TODO)

	//keyboard/joystick
	//AM_RANGE(0x10, 0x10) AM_READ(pv2000_keys_r)
	AM_RANGE(0x20, 0x20) AM_READWRITE(pv2000_keys_r, pv2000_keys_w)
	//AM_RANGE(0x40, 0x40) AM_READ(pv2000_keys_r)

	//sn76489a
	AM_RANGE(0x40, 0x40) AM_DEVWRITE("sn76489a", sn76496_w)

ADDRESS_MAP_END


static INPUT_PORTS_START( pv2000 )

INPUT_PORTS_END


static INTERRUPT_GEN( pv2000_interrupt )
{
   TMS9928A_interrupt(device->machine);
}

static void pv2000_vdp_interrupt(running_machine *machine, int state)
{
    // only if it goes up
	if (state && !last_state)
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);

	last_state = state;
}



/* Machine Initialization */

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	pv2000_vdp_interrupt
};

static MACHINE_START( pv2000 )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( pv2000 )
{
	last_state = 0;
	cpu_set_input_line_vector(devtag_get_device(machine, "maincpu"), INPUT_LINE_IRQ0, 0xff);
	memset(&memory_region(machine, "maincpu")[0x7000], 0xff, 0x1000);	// initialize RAM
}


/* Machine Drivers */

static MACHINE_DRIVER_START( pv2000 )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", Z80, XTAL_7_15909MHz/2)	// 3.579545 MHz
	MDRV_CPU_PROGRAM_MAP(pv2000_map)
	MDRV_CPU_IO_MAP(pv2000_io_map)
	MDRV_CPU_VBLANK_INT("screen", pv2000_interrupt)

	MDRV_MACHINE_START(pv2000)
	MDRV_MACHINE_RESET(pv2000)

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, XTAL_7_15909MHz/2)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,col,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END



/* ROMs */
ROM_START (pv2000)
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "hn613128pc64.bin", 0x0000, 0x4000, CRC(8f31f297) SHA1(94b5f54dd7bce321e377fdaaf592acd3870cf621) )
	ROM_CART_LOAD("cart", 0xC000, 0x4000, ROM_OPTIONAL)
ROM_END


/* System Drivers */

//    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT     INIT COMPANY   FULLNAME    FLAGS
CONS( 1983, pv2000,  0,      0,      pv2000,  pv2000,   0,   "Casio",  "PV-2000",  GAME_NOT_WORKING )
