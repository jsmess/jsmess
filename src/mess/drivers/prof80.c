/*

	PROF-80 (Prozessor RAM-Floppy Kontroller)
	GRIP-1/2/3 (Grafik-Interface-Prozessor)

	http://www.prof80.de/

*/

/*

	TODO:

	- GRIP video RAM banking

*/

#include "driver.h"
#include "includes/prof80.h"
#include "cpu/z80/z80.h"
#include "devices/basicdsk.h"
#include "video/mc6845.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "machine/upd1990a.h"
#include "machine/ctronics.h"

/* Read/Write Handlers */

static void prof80_bankswitch(running_machine *machine)
{
	prof80_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	int bank;

	for (bank = 0; bank < 16; bank++)
	{
		UINT16 start_addr = bank * 0x1000;
		UINT16 end_addr = start_addr + 0xfff;
		int block = state->mme ? (~state->mmu[bank] & 0x0f) : 15;

		memory_set_bank(machine, bank + 1, block);

		switch (block)
		{
		case 15:
		case 14:
		case 7:
		case 6:
			/* EPROM */
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_UNMAP);
			break;

		case 13:
		case 12:
		case 5:
		case 4:
			/* RAM */
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_BANK(bank + 1));
			break;

		default:
			memory_install_readwrite8_handler(program, start_addr, end_addr, 0, 0, SMH_BANK(bank + 1), SMH_UNMAP);
		}
		
		logerror("Segment %u address %04x-%04x block %u\n", bank, start_addr, end_addr, block);
	}
}

static WRITE8_HANDLER( flr_w )
{
	/*

		bit		description

		0		FB
		1		SB0
		2		SB1
		3		SB2
		4		SA0
		5		SA1
		6		SA2
		7		FA

	*/

	prof80_state *state = space->machine->driver_data;

	int fa = BIT(data, 7);
	int sa = (data >> 4) & 0x07;

	int fb = BIT(data, 0);
	int sb = (data >> 1) & 0x07;

	switch (sa)
	{
	case 0:
		upd1990a_data_w(state->upd1990a, fa);
		upd1990a_c0_w(state->upd1990a, fa);
		break;

	case 1:
		upd1990a_c1_w(state->upd1990a, fa);
		break;

	case 2:
		upd1990a_c2_w(state->upd1990a, fa);
		break;

	case 3:
		/* READY */
		break;

	case 4:
		upd1990a_clk_w(state->upd1990a, fa);
		break;

	case 5:
		/* IN USE */
		break;

	case 6:
		/* _MOTOR */
		break;

	case 7:
		/* SELECT */
		break;
	}

	switch (sb)
	{
	case 0: /* RESF */
		if (fb) nec765_reset(state->nec765, 0);
		break;

	case 1: /* MINI */
		break;

	case 2: /* _RTS */
		break;

	case 3: /* TX */
		break;

	case 4: /* _MSTOP */
		break;

	case 5: /* TXP */
		break;

	case 6: /* TSTB */
		upd1990a_stb_w(state->upd1990a, fb);
		break;

	case 7: /* MME */
		state->mme = fb;
		break;
	}
}

static READ8_HANDLER( status_r )
{
	/*

		bit		signal		description

		0		_RX
		1
		2
		3
		4		CTS
		5		_INDEX
		6		
		7		CTSP

	*/

	prof80_state *state = space->machine->driver_data;

	return (state->fdc_index << 5) | 0x01;
}

static READ8_HANDLER( status2_r )
{
	/*

		bit		signal		description

		0		MOTOR
		1
		2
		3
		4		x
		5		x
		6
		7		_TDO

	*/

	prof80_state *state = space->machine->driver_data;

	return (!state->rtc_data << 7);
}

static READ8_HANDLER( fdc_status_r )
{
	/*

		bit		signal		description

		0		FDB1
		1		FDB2
		2		FDB3
		3		FDB4
		4		CB
		5		EXM
		6		DIO
		7		RQM

	*/

	return 0;
}

static WRITE8_HANDLER( fdc_w )
{
}

static READ8_HANDLER( fdc_r )
{
	return 0;
}

static WRITE8_HANDLER( mmu_w )
{
	prof80_state *state = space->machine->driver_data;

	int bank = offset >> 12;

	state->mmu[bank] = data & 0x0f;

	prof80_bankswitch(space->machine);
}

static WRITE8_HANDLER( page_w )
{
//	memory_set_bank(space->machine, 17, BIT(data, 7));
}

/* Memory Maps */

static ADDRESS_MAP_START( prof80_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
    AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(2)
    AM_RANGE(0x2000, 0x2fff) AM_RAMBANK(3)
    AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(4)
    AM_RANGE(0x4000, 0x4fff) AM_RAMBANK(5)
    AM_RANGE(0x5000, 0x5fff) AM_RAMBANK(6)
    AM_RANGE(0x6000, 0x6fff) AM_RAMBANK(7)
    AM_RANGE(0x7000, 0x7fff) AM_RAMBANK(8)
    AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(9)
    AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(10)
    AM_RANGE(0xa000, 0xafff) AM_RAMBANK(11)
    AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(12)
    AM_RANGE(0xc000, 0xcfff) AM_RAMBANK(13)
    AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(14)
    AM_RANGE(0xe000, 0xefff) AM_RAMBANK(15)
    AM_RANGE(0xf000, 0xffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( prof80_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xd8, 0xd8) AM_MIRROR(0xff00) AM_WRITE(flr_w)
	AM_RANGE(0xda, 0xda) AM_MIRROR(0xff00) AM_READ(status_r)
	AM_RANGE(0xdb, 0xdb) AM_MIRROR(0xff00) AM_READ(status2_r)
	AM_RANGE(0xdc, 0xdc) AM_MIRROR(0xff00) AM_READ(fdc_status_r)
	AM_RANGE(0xdd, 0xdd) AM_MIRROR(0xff00) AM_READWRITE(fdc_r, fdc_w)
	AM_RANGE(0xde, 0xde) AM_MIRROR(0xff01) AM_MASK(0xff00) AM_WRITE(mmu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_mem, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x3fff) AM_ROM
    AM_RANGE(0x4000, 0x47ff) AM_RAM
    AM_RANGE(0x8000, 0xffff) AM_RAMBANK(17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( grip_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x00, 0x00) AM_READWRITE(stb_r, stb_w)
//	AM_RANGE(0x10, 0x10) AM_WRITE(cc3_w)
//	AM_RANGE(0x11, 0x11) AM_WRITE(vol0_w)
//	AM_RANGE(0x12, 0x12) AM_WRITE(rts_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(page_w)
//	AM_RANGE(0x14, 0x14) AM_WRITE(cc1_w)
//	AM_RANGE(0x15, 0x15) AM_WRITE(cc2_w)
//	AM_RANGE(0x16, 0x16) AM_WRITE(flash_w)
//	AM_RANGE(0x17, 0x17) AM_WRITE(vol1_w)
//	AM_RANGE(0x20, 0x2f) AM_DEVREADWRITE(Z80STI_TAG, z80sti_r, z80sti_w)
//	AM_RANGE(0x30, 0x30) AM_READWRITE(lrs_r, lrs_w)
//	AM_RANGE(0x40, 0x40) AM_READ(status_r)
	AM_RANGE(0x50, 0x50) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x52, 0x52) AM_DEVWRITE(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x53, 0x53) AM_DEVREAD(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x70, 0x73) AM_DEVREADWRITE("ppi8255", ppi8255_r, ppi8255_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( prof80 )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( grip_update_row )
{
}

static MC6845_ON_VSYNC_CHANGED( grip_vsync_changed )
{
	prof80_state *state = device->machine->driver_data;

	state->vsync = vsync;
}

static const mc6845_interface grip_mc6845_interface = 
{
	SCREEN_TAG,
	8,
	NULL,
	grip_update_row,
	NULL,
	NULL,
	NULL,
	grip_vsync_changed
};

static VIDEO_START( prof80 )
{
}

static VIDEO_UPDATE( prof80 )
{
	prof80_state *state = screen->machine->driver_data;

	mc6845_update(state->mc6845, bitmap, cliprect);

	return 0;
}

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( prof80_upd1990a_data_w )
{
	prof80_state *driver_state = device->machine->driver_data;

	driver_state->rtc_data = state;
}

static UPD1990A_INTERFACE( prof80_upd1990a_intf )
{
	DEVCB_LINE(prof80_upd1990a_data_w),
	DEVCB_NULL
};

/* NEC765 Interface */

static void prof80_fdc_index_callback(const device_config *controller, const device_config *img, int state)
{
	prof80_state *driver_state = img->machine->driver_data;

	driver_state->fdc_index = state;
}

static const struct nec765_interface prof80_nec765_interface =
{
	NULL,
	NULL,
	NULL,
	NEC765_RDY_PIN_CONNECTED
};

/* PPI8255 Interface */

static const ppi8255_interface grip_ppi8255_interface =
{
	DEVCB_NULL,	// Port A read
	DEVCB_NULL,	// Port B read
	DEVCB_NULL,	// Port C read
	DEVCB_NULL,	// Port A write
	DEVCB_NULL,	// Port B write
	DEVCB_NULL,	// Port C write
};

/* Machine Initialization */

static MACHINE_START( prof80 )
{
	prof80_state *state = machine->driver_data;
	int bank;

	/* find devices */
	state->nec765 = devtag_get_device(machine, NEC765_TAG);
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	/* configure FDC */
	floppy_drive_set_index_pulse_callback(image_from_devtype_and_index(machine, IO_FLOPPY, 0), prof80_fdc_index_callback);

	/* allocate video RAM */
	state->video_ram = auto_alloc_array(machine, UINT8, GRIP_VIDEORAM_SIZE);

	/* setup PROF-80 memory banking */
	for (bank = 0; bank < 16; bank++)
	{
		UINT32 offset = bank * 0x1000;
		memory_configure_bank(machine, bank + 1, 0, 16, memory_region(machine, Z80_TAG) + offset, 0x10000);
	}

	/* setup GRIP memory banking */
//	memory_configure_bank(machine, 17, 0, 2, state->video_ram, 0x8000);
//	memory_set_bank(machine, 17, 0);

	/* bank switch */
	prof80_bankswitch(machine);

	/* register for state saving */
	state_save_register_global_array(machine, state->mmu);
	state_save_register_global(machine, state->mme);
	state_save_register_global_pointer(machine, state->video_ram, GRIP_VIDEORAM_SIZE);
	state_save_register_global(machine, state->vsync);
	state_save_register_global(machine, state->rtc_data);
	state_save_register_global(machine, state->fdc_index);
}

static MACHINE_RESET( prof80 )
{
	int i;

	for (i = 0; i < 8; i++)
	{
	// write all zeroes 0 flr_W
		//UINT8 data = (i << 4) | (i << 1);
	}
}

/* Machine Drivers */

static MACHINE_DRIVER_START( prof80 )
	MDRV_DRIVER_DATA(prof80_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_6MHz)
    MDRV_CPU_PROGRAM_MAP(prof80_mem)
    MDRV_CPU_IO_MAP(prof80_io)
 
	MDRV_MACHINE_START(prof80)
	MDRV_MACHINE_RESET(prof80)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)
    MDRV_PALETTE_LENGTH(2)
 
    MDRV_VIDEO_START(prof80)
    MDRV_VIDEO_UPDATE(prof80)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz, grip_mc6845_interface)

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, prof80_upd1990a_intf)
	MDRV_NEC765A_ADD(NEC765_TAG, prof80_nec765_interface)
	MDRV_PPI8255_ADD(PPI8255_TAG, grip_ppi8255_interface)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( prof80 )
	ROM_REGION( 0x100000, Z80_TAG, 0 )
	ROM_DEFAULT_BIOS( "v17" )
	
	ROM_SYSTEM_BIOS( 0, "v15", "v1.5" )
	ROMX_LOAD( "prof80v15.z7", 0xf0000, 0x00800, CRC(8f74134c) SHA1(83f9dcdbbe1a2f50006b41d406364f4d580daa1f), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v16", "v1.6" )
	ROMX_LOAD( "prof80v16.z7", 0xf0000, 0x00800, CRC(7d3927b3) SHA1(bcc15fd04dbf1d6640115be595255c7b9d2a7281), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v17", "v1.7" )
	ROMX_LOAD( "prof80v17.z7", 0xf0000, 0x00800, CRC(53305ff4) SHA1(3ea209093ac5ac8a5db618a47d75b705965cdf44), ROM_BIOS(3) )

	ROM_COPY( Z80_TAG, 0xf0000, 0xf0800, 0x00800 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf1000, 0x01000 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf2000, 0x02000 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf4000, 0x04000 )
	ROM_COPY( Z80_TAG, 0xf0000, 0xf8000, 0x08000 ) // block 15
	ROM_COPY( Z80_TAG, 0xf0000, 0xe0000, 0x10000 ) // block 14
	ROM_COPY( Z80_TAG, 0xf0000, 0x70000, 0x10000 ) // block 7
	ROM_COPY( Z80_TAG, 0xf0000, 0x60000, 0x10000 ) // block 6

	ROM_REGION( 0x10000, "grip", 0 )
	ROM_LOAD( "grip.z2", 0x0000, 0x4000, NO_DUMP )
ROM_END

/* System Configurations */

static DEVICE_IMAGE_LOAD( prof80_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	return INIT_FAIL;
}

static void prof80_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(prof80_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( prof80 )
	CONFIG_RAM_DEFAULT	(128 * 1024)
	CONFIG_DEVICE(prof80_floppy_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE INPUT   INIT  CONFIG COMPANY  FULLNAME   FLAGS */
COMP( 1984, prof80,     0,      0, prof80, prof80, 0, prof80,  "Conitec Datensysteme", "PROF-80", GAME_NO_SOUND | GAME_NOT_WORKING)
