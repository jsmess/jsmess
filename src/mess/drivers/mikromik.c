#include "driver.h"
#include "includes/mikromik.h"
#include "formats/basicdsk.h"
#include "devices/mflopimg.h"
#include "cpu/i8085/i8085.h"
#include "machine/8237dma.h"
#include "machine/i8212.h"
#include "machine/nec765.h"
#include "machine/pit8253.h"
#include "machine/upd7201.h"
#include "video/i8275.h"
#include "video/i82720.h"
#include "video/upd7220.h"

/*

	TODO:

	- ROM dumps
	- pixel display
	- floppy
	- keyboard
	- memory banking
	- PCB layout
	- NEC µPD7220 (devicify Intel 82720 GDC)
	- NEC µPD7201

*/

/*

	Nokia Elektroniikka pj
	Controller ILC 9534

	Parts:

	6,144 MHz xtal (CPU clock)
	18,720 MHz xtal (pixel clock)
	16 MHz xtal (FDC clock)
	Intel 8085AP (CPU)
	Intel 8253-5P (PIT)
	Intel 8275P (CRTC)
	Intel 8212P (I/OP)
	Intel 8237A-5P (DMAC)
	NEC µPD7220C (GDC)
	NEC µPD7201P (MPSC=uart)
	NEC µPD765 (FDC)
	TMS4116-15 (16Kx4 DRAM)*4 = 32KB Video RAM for 7220
	2164-6P (64Kx1 DRAM)*8 = 64KB Work RAM

	DMA channels:
	
	0	CRT
	1	MPSC transmit
	2	MPSC receive
	3	FDC

	Interrupts:

	INTR	MPSC INT
	RST5.5	FDC IRQ
	RST6.5	8212 INT
	RST7.5	DMA EOP

*/

INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return image_from_devtype_and_index(machine, IO_FLOPPY, drive);
}

/* Read/Write Handlers */

static WRITE8_HANDLER( cs6_w )
{
	mm1_state *state = space->machine->driver_data;
	int d = BIT(data, 0);

	switch (offset)
	{
	case 0: /* IC24 A8 */
		break;

	case 1: /* RECALL */
		state->recall = d;
		break;

	case 2: /* _RV28/RX21 */
		state->rx21 = d;
		break;

	case 3: /* _TX21 */
		state->tx21 = d;
		break;

	case 4: /* _RCL */
		state->rcl = d;
		break;

	case 5: /* _INTC */
		state->intc = d;
		break;

	case 6: /* LLEN */
		state->llen = d;
		break;

	case 7: /* MOTOR ON */
		floppy_drive_set_motor_state(get_floppy_image(space->machine, 0), d);
		floppy_drive_set_ready_state(get_floppy_image(space->machine, 0), d, 1);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xff00, 0xff0f) AM_DEVREADWRITE(I8237_TAG, dma8237_r, dma8237_w)
	AM_RANGE(0xff10, 0xff13) AM_DEVREADWRITE(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)
    AM_RANGE(0xff20, 0xff21) AM_DEVREADWRITE(I8275_TAG, i8275_r, i8275_w)
	AM_RANGE(0xff30, 0xff33) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xff40, 0xff40) AM_DEVREADWRITE(I8212_TAG, i8212_r, i8212_w)
	AM_RANGE(0xff50, 0xff50) AM_DEVREAD(UPD765_TAG, nec765_status_r)
	AM_RANGE(0xff51, 0xff51) AM_DEVREADWRITE(UPD765_TAG, nec765_data_r, nec765_data_w)
	AM_RANGE(0xff60, 0xff67) AM_WRITE(cs6_w)
//	AM_RANGE(0xff70, 0xff7f) AM_DEVREADWRITE(UPD7220_TAG, i82720_r, i82720_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( drqcrt_w )
{
	mm1_state *driver_state = device->machine->driver_data;
	//logerror("i8275 DMA\n");
	dma8237_drq_write(driver_state->i8237, DMA_CRT, state);
}

static I8275_DISPLAY_PIXELS( crtc_display_pixels )
{
	mm1_state *state = device->machine->driver_data;

	UINT8 data = state->char_rom[((charcode & 0x3f) << 4) | linecount];
	int i;
	
	for (i = 0; i < 6; i++)
	{
		*BITMAP_ADDR16(tmpbitmap, y, x + i) = BIT(data, i);
	}
}

static const i8275_interface mm1_i8275_intf = 
{
	SCREEN_TAG,
	8,
	0,
	DEVCB_LINE(drqcrt_w),
	DEVCB_NULL,
	crtc_display_pixels
};

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
}

static UPD7220_INTERFACE( mm1_upd7220_intf )
{
	SCREEN_TAG,
	8,
	hgdc_display_pixels,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static VIDEO_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");

	VIDEO_START_CALL(generic_bitmapped);
}

static VIDEO_UPDATE( mm1 )
{
	mm1_state *state = screen->machine->driver_data;

	i8275_update(state->i8275, bitmap, cliprect);
	VIDEO_UPDATE_CALL(generic_bitmapped);

	return 0;
}

/* 8212 Interface */

static READ8_DEVICE_HANDLER( kb_r )
{
	/* keyboard data */
	return 0;
}

static I8212_INTERFACE( mm1_i8212_intf )
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST65_LINE),
	DEVCB_HANDLER(kb_r),
	DEVCB_NULL
};

/* 8237 Interface */

static DMA8237_HRQ_CHANGED( dma_hrq_changed )
{
	cputag_set_input_line(device->machine, I8085A_TAG, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	dma8237_set_hlda(device, state);
}

static DMA8237_MEM_READ( memory_dma_r )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA read %04x\n", offset);
	return memory_read_byte(program, offset);
}

static DMA8237_MEM_WRITE( memory_dma_w )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085A_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA write %04x:%02x\n", offset, data);
	memory_write_byte(program, offset, data);
}

static DMA8237_CHANNEL_WRITE( crtc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	i8275_dack_w(state->i8275, 0, data);
}

static DMA8237_CHANNEL_READ( mpsc_dack_r )
{
	mm1_state *state = device->machine->driver_data;
	
	return upd7201_hai_r(state->upd7201, 0);
}

static DMA8237_CHANNEL_WRITE( mpsc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	upd7201_hai_w(state->upd7201, 0, data);
}

static DMA8237_CHANNEL_READ( fdc_dack_r )
{
	mm1_state *state = device->machine->driver_data;

	return nec765_dack_r(state->upd765, 0);
}

static DMA8237_CHANNEL_WRITE( fdc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	nec765_dack_w(state->upd765, 0, data);
}

static DMA8237_OUT_EOP( tc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	/* floppy terminal count */
	nec765_tc_w(driver_state->upd765, !state);

	cputag_set_input_line(device->machine, I8085A_TAG, I8085_RST75_LINE, state ? CLEAR_LINE : ASSERT_LINE);
}

static const struct dma8237_interface mm1_dma8237_intf =
{
	XTAL_6_144MHz/2/2,
	dma_hrq_changed,
	memory_dma_r,
	memory_dma_w,
	{ NULL, NULL, mpsc_dack_r, fdc_dack_r },
	{ crtc_dack_w, mpsc_dack_w, NULL, fdc_dack_w },
	tc_w
};

/* µPD765 Interface */

static NEC765_DMA_REQUEST( drq_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, DMA_FDC, state);
}

static const nec765_interface mm1_nec765_intf =
{
	DEVCB_CPU_INPUT_LINE(I8085A_TAG, I8085_RST55_LINE),
	drq_w,
	NULL,
	NEC765_RDY_PIN_CONNECTED // ???
};

/* 8253 Interface */

static PIT8253_OUTPUT_CHANGED( itxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (!driver_state->intc)
	{
		upd7201_txca_w(driver_state->upd7201, state);
	}
}

static PIT8253_OUTPUT_CHANGED( irxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	if (!driver_state->intc)
	{
		upd7201_rxca_w(driver_state->upd7201, state);
	}
}

static PIT8253_OUTPUT_CHANGED( auxc_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	upd7201_txcb_w(driver_state->upd7201, state);
	upd7201_rxcb_w(driver_state->upd7201, state);
}

static const struct pit8253_config mm1_pit8253_intf =
{
	{
		{
			XTAL_6_144MHz/2/2,
			itxc_w
		}, {
			XTAL_6_144MHz/2/2,
			irxc_w
		}, {
			XTAL_6_144MHz/2/2,
			auxc_w
		}
	}
};

/* µPD7201 Interface */

static WRITE_LINE_DEVICE_HANDLER( drq2_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, 2, state);
}

static WRITE_LINE_DEVICE_HANDLER( drq1_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, 1, state);
}

static UPD7201_INTERFACE( mm1_upd7201_intf )
{
	DEVCB_NULL,					/* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_LINE(drq2_w),	/* receive DRQ */
			DEVCB_LINE(drq1_w),	/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}, {
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/* 8085A Interface */

static READ_LINE_DEVICE_HANDLER( dsra_r )
{
	return 1;
}

static WRITE_LINE_DEVICE_HANDLER( bell_w )
{
}

static I8085_CONFIG( mm1_i8085_config )
{
	DEVCB_NULL,			/* STATUS changed callback */
	DEVCB_NULL,			/* INTE changed callback */
	DEVCB_LINE(dsra_r),	/* SID changed callback (8085A only) */
	DEVCB_LINE(bell_w)	/* SOD changed callback (8085A only) */
};

/* Machine Initialization */

static MACHINE_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* look up devices */
	state->i8212 = devtag_get_device(machine, I8212_TAG);
	state->i8237 = devtag_get_device(machine, I8237_TAG);
	state->i8275 = devtag_get_device(machine, I8275_TAG);
	state->upd765 = devtag_get_device(machine, UPD765_TAG);
	state->upd7201 = devtag_get_device(machine, UPD7201_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->llen);
	state_save_register_global(machine, state->intc);
	state_save_register_global(machine, state->rx21);
	state_save_register_global(machine, state->tx21);
	state_save_register_global(machine, state->rcl);
	state_save_register_global(machine, state->recall);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mm1 )
	MDRV_DRIVER_DATA(mm1_state)

	/* basic system hardware */
	MDRV_CPU_ADD(I8085A_TAG, 8085A, XTAL_6_144MHz)
	MDRV_CPU_PROGRAM_MAP(mm1_map)
	MDRV_CPU_CONFIG(mm1_i8085_config)

	MDRV_MACHINE_START(mm1)

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, RASTER )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 800, 327 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 327-1 )
	//MDRV_SCREEN_RAW_PARAMS(XTAL_18_720MHz, ...)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(mm1)
	MDRV_VIDEO_UPDATE(mm1)

	/* peripheral hardware */
	MDRV_I8212_ADD(I8212_TAG, mm1_i8212_intf)
	MDRV_I8275_ADD(I8275_TAG, mm1_i8275_intf)
	MDRV_DMA8237_ADD(I8237_TAG, /* XTAL_6_144MHz/2, */ mm1_dma8237_intf)
	MDRV_PIT8253_ADD(I8253_TAG, mm1_pit8253_intf)
	MDRV_NEC765A_ADD(UPD765_TAG, /* XTAL_16MHz/2/2, */ mm1_nec765_intf)
	MDRV_UPD7201_ADD(UPD7201_TAG, XTAL_6_144MHz/2, mm1_upd7201_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mm1m6 )
	MDRV_IMPORT_FROM(mm1)

	MDRV_UPD7220_ADD(UPD7220_TAG, 0, mm1_upd7220_intf)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x4000, I8085A_TAG, 0 )
	ROM_LOAD( "mm1.ic2",  0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "mm1.ic10", 0x2000, 0x2000, NO_DUMP )
	ROM_LOAD( "mm1.bin",  0x0000, 0x1000, BAD_DUMP CRC(07400e72) SHA1(354ff97817a607ca38d296af8b2813878d092a08) )

	ROM_REGION( 0x200, "prom", 0 )
	ROM_LOAD( "mmi6349-1.ic24", 0x0000, 0x0200, NO_DUMP )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "chargen.ic51", 0x0000, 0x0800, NO_DUMP )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Configuration */

static FLOPPY_OPTIONS_START( mm1 )
	FLOPPY_OPTION( mm1_640kb, "dsk", "Nokia MikroMikko 1 640KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([8])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static FLOPPY_OPTIONS_START( mm2 )
	FLOPPY_OPTION( mm2_360kb, "dsk", "Nokia MikroMikko 2 360KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))

	FLOPPY_OPTION( mm2_720kb, "dsk", "Nokia MikroMikko 2 720KB disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static void dual_640kb_floppy(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:	info->p = (void *) floppyoptions_mm1; break;

		default:								floppy_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( mm1m6 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( mm1m7 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
	// 5MB Winchester
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT  COMPAT	MACHINE		INPUT		INIT	CONFIG    COMPANY			FULLNAME				FLAGS
COMP( 1981, mm1m6,		0,		0,		mm1m6,		mm1,		0, 		mm1m6,	  "Nokia Data",		"MikroMikko 1 M6",		GAME_NOT_WORKING )
COMP( 1981, mm1m7,		mm1m6,	0,		mm1m6,		mm1,		0, 		mm1m7,	  "Nokia Data",		"MikroMikko 1 M7",		GAME_NOT_WORKING )
