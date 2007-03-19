/******************************************************************************

    kc.c
	system driver

	A big thankyou to Torsten Paul for his great help with this
	driver!


	Kevin Thacker [MESS driver]

 ******************************************************************************/

#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "includes/kc.h"
#include "devices/cassette.h"

/* pio is last in chain and therefore has highest priority */

static struct z80_irq_daisy_chain kc85_daisy_chain[] =
{
	{z80pio_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{0,0,0,0,-1}
};

static READ8_HANDLER(kc85_4_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			return kc85_module_r(offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);

	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE8_HANDLER(kc85_4_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			kc85_module_w(offset,data);
//			return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


ADDRESS_MAP_START(kc85_4_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE( kc85_4_port_r, kc85_4_port_w )
ADDRESS_MAP_END


static  READ8_HANDLER(kc85_4d_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x080:
			return kc85_module_r(offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);
		case 0x0f0:
		case 0x0f1:
		case 0x0f2:
		case 0x0f3:
			return kc85_disc_interface_ram_r(offset);

	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE8_HANDLER(kc85_4d_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
		case 0x080:
			kc85_module_w(offset,data);
			return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;

		case 0x0f0:
		case 0x0f1:
		case 0x0f2:
		case 0x0f3:
			kc85_disc_interface_ram_w(offset,data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}

ADDRESS_MAP_START(kc85_4d_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE(kc85_4d_port_r, kc85_4d_port_w)
ADDRESS_MAP_END

ADDRESS_MAP_START(kc85_4_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK7)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK8)
	AM_RANGE(0x8000, 0xa7ff) AM_READWRITE(MRA8_BANK3, MWA8_BANK9)
//	AM_RANGE(0xa800, 0xbfff) AM_RAM
	AM_RANGE(0xa800, 0xbfff) AM_READWRITE(MRA8_BANK4, MWA8_BANK10)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK5)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_BANK6)
ADDRESS_MAP_END

ADDRESS_MAP_START(kc85_3_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK6)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK7)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK8)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK4)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_BANK5)
ADDRESS_MAP_END

static READ8_HANDLER(kc85_3_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			return kc85_module_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(port-0x08c);
	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE8_HANDLER(kc85_3_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//		case 0x080:
//			kc85_module_w(offset,data);
//			return;

		case 0x088:
		case 0x089:
			kc85_3_pio_data_w(port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


ADDRESS_MAP_START(kc85_3_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) ) 
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE(kc85_3_port_r, kc85_3_port_w)
ADDRESS_MAP_END



INPUT_PORTS_START( kc85 )
	KC_KEYBOARD
INPUT_PORTS_END


/********************/
/** DISC INTERFACE **/

ADDRESS_MAP_START(kc85_disc_hw_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START(kc85_disc_hw_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0f0, 0x0f0) AM_READ(nec765_status_r)
	AM_RANGE(0x0f1, 0x0f1) AM_READWRITE(nec765_data_r, nec765_data_w)
	AM_RANGE(0x0f2, 0x0f3) AM_READWRITE(nec765_dack_r, nec765_dack_w)
	AM_RANGE(0x0f4, 0x0f5) AM_READ(kc85_disc_hw_input_gate_r)
	/*{0x0f6, 0x0f7, MRA8_NOP},*/		/* for controller */
	AM_RANGE(0x0f8, 0x0f9) AM_WRITE( kc85_disc_hw_terminal_count_w) /* terminal count */
	AM_RANGE(0x0fc, 0x0ff) AM_READWRITE(kc85_disk_hw_ctc_r, kc85_disk_hw_ctc_w)
ADDRESS_MAP_END

MACHINE_DRIVER_START( cpu_kc_disc )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(kc85_disc_hw_mem, 0)
	MDRV_CPU_IO_MAP(kc85_disc_hw_io, 0)
MACHINE_DRIVER_END



static MACHINE_DRIVER_START( kc85_3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, KC85_3_CLOCK)
	MDRV_CPU_PROGRAM_MAP(kc85_3_mem, 0)
	MDRV_CPU_IO_MAP(kc85_3_io, 0)
	MDRV_CPU_CONFIG(kc85_daisy_chain)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( kc85_3 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(KC85_SCREEN_WIDTH, KC85_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1))
	MDRV_PALETTE_LENGTH(KC85_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(KC85_PALETTE_SIZE)
	MDRV_PALETTE_INIT( kc85 )

	MDRV_VIDEO_START( kc85_3 )
	MDRV_VIDEO_UPDATE( kc85_3 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4 )
	MDRV_IMPORT_FROM( kc85_3 )

	MDRV_CPU_REPLACE("main", Z80, KC85_4_CLOCK)
	MDRV_CPU_PROGRAM_MAP(kc85_4_mem, 0)
	MDRV_CPU_IO_MAP(kc85_4_io, 0)

	MDRV_MACHINE_RESET( kc85_4 )
	MDRV_VIDEO_START( kc85_4 )
	MDRV_VIDEO_UPDATE( kc85_4 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4d )
	MDRV_IMPORT_FROM( kc85_4 )
	MDRV_IMPORT_FROM( cpu_kc_disc )
	MDRV_INTERLEAVE( 2 )
	MDRV_MACHINE_RESET( kc85_4d )
MACHINE_DRIVER_END


ROM_START(kc85_4)
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b) SHA1(809cde2d189c1976d838e3e614468705183de800))
ROM_END


ROM_START(kc85_4d)
	ROM_REGION(0x015000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b) SHA1(809cde2d189c1976d838e3e614468705183de800))

	ROM_REGION(0x010000, REGION_CPU2,0)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x014000, REGION_CPU1,0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
	ROM_LOAD("caos__e0.853", 0x12000, 0x2000, CRC(52bc2199) SHA1(207d3e1c4ebf82ac7553ed0a0850b627b9796d4b))
ROM_END

static void kc85_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void kc85_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "kcc"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_kc; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(kc85)
	CONFIG_RAM_DEFAULT		(64 * 1024)
	CONFIG_DEVICE(kc85_cassette_getinfo)
	CONFIG_DEVICE(kc85_quickload_getinfo)
SYSTEM_CONFIG_END

static void kc85d_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_kc85_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(kc85d)
	CONFIG_IMPORT_FROM(kc85)
	CONFIG_DEVICE(kc85d_floppy_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT	COMPAT	MACHINE  INPUT     INIT  CONFIG  COMPANY   FULLNAME */
COMP( 1987, kc85_3,   0,		0,		kc85_3,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMP( 1989, kc85_4,   kc85_3,  0,		kc85_4,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMP( 1989, kc85_4d,  kc85_3,  0,		kc85_4d, kc85,     0,    kc85d,  "VEB Mikroelektronik", "KC 85/4 + Disk Interface Module (D004)", GAME_NOT_WORKING)
