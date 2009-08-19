#include "driver.h"
#include "includes/mikromik.h"
#include "formats/basicdsk.h"
#include "devices/basicdsk.h"
#include "devices/mflopimg.h"
#include "cpu/i8085/i8085.h"
#include "machine/8237dma.h"
#include "machine/nec765.h"
#include "machine/pit8253.h"
#include "video/i8275.h"
#include "video/upd7220.h"

/*

	Nokia Elektroniikka pj
	Controller ILC 9534

	Parts:

	6,144 MHz xtal
	18,720 MHz xtal
	16 MHz xtal
	Intel 8085AP (CPU)
	Intel 8253-5P (PIT)
	Intel 8275P (CRTC)
	Intel 8212P (I/OP)
	Intel 8237A-5P (DMAC)
	NEC µPD7220C (GDC)
	NEC µPD7201P (MPSC=uart)
	NEC µPD765 (FDC)
	TMS4116-15 (16Kx4 DRAM)*4 = 32KB RAM

	DMA channels:
	
	0	CRT
	1	MPSC transmit
	2	MPSC receive
	3	FDC

	Interrupts:

	INTR	MPSC
	RST5.5	FDC
	RST6.5	8212
	RST7.5	TC?

*/

/*

	TODO:

	- Intel 8212 emulation
	- PCB layout
	- character generator
	- memory mapped I/O
	- pixel display
	- system disks
	- hook up DMA channels
	- NEC µPD7220 emulation (Intel 82720 GDC)

*/

#define MM1_DMA_I8275	0
#define MM1_DMA_I8272	0
#define MM1_DMA_UPD7220	0

/* Video */

static I8275_DMA_REQUEST( crtc_dma_request )
{
	mm1_state *driver_state = device->machine->driver_data;
	//logerror("i8275 DMA\n");
	dma8237_drq_write(driver_state->i8237, MM1_DMA_I8275, state);
}

static I8275_INT_REQUEST( crtc_int_request )
{
	//logerror("i8275 INT\n");
}

static I8275_DISPLAY_PIXELS( crtc_display_pixels )
{
}

static const i8275_interface mm1_i8275_intf = 
{
	SCREEN_TAG,
	8,
	0,
	crtc_dma_request,
	crtc_int_request,
	crtc_display_pixels
};

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
}

static WRITE_LINE_DEVICE_HANDLER( hgdc_drq_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, MM1_DMA_UPD7220, state);
}

static UPD7220_INTERFACE( mm1_upd7220_intf )
{
	SCREEN_TAG,
	8,
	hgdc_display_pixels,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(hgdc_drq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static VIDEO_UPDATE( mm1 )
{
	mm1_state *state = screen->machine->driver_data;

	i8275_update(state->i8275, bitmap, cliprect);

	return 0;
}

static DMA8237_HRQ_CHANGED( dma_hrq_changed )
{
	cputag_set_input_line( device->machine, I8085_TAG, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE );

	/* Assert HLDA */
	dma8237_set_hlda( device, state );
}

static DMA8237_MEM_READ( memory_dma_r )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA read %04x\n", offset);
	return memory_read_byte(program, offset);
}

static DMA8237_MEM_WRITE( memory_dma_w )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA write %04x:%02x\n", offset, data);
	memory_write_byte(program, offset, data);
}

static DMA8237_CHANNEL_WRITE( crtc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	i8275_dack_set_data(state->i8275, data);
}

#ifdef UNUSED_CODE
static DMA8237_CHANNEL_WRITE( hgdc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	upd7220_dack_w(state->upd7220, 0, data);
}

static DMA8237_CHANNEL_READ( fdc_dack_r )
{
	mm1_state *state = device->machine->driver_data;

	return nec765_dack_r(state->i8272, 0);
}

static DMA8237_CHANNEL_WRITE( fdc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	nec765_dack_w(state->i8272, 0, data);
}
#endif

static DMA8237_OUT_EOP( dma_eop_w )
{
}

static const struct dma8237_interface mm1_dma8237_intf =
{
	XTAL_18_720MHz/3, /* this needs to be verified */
	dma_hrq_changed,
	memory_dma_r,
	memory_dma_w,
	{ NULL, NULL, NULL, NULL },
	{ crtc_dack_w, NULL, NULL, NULL },
	dma_eop_w
};

static NEC765_INTERRUPT( fdc_irq )
{
}

static NEC765_DMA_REQUEST( fdc_drq )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, MM1_DMA_I8272, state);
}

static const nec765_interface mm1_nec765_intf =
{
	fdc_irq,
	fdc_drq,
	NULL,
	NEC765_RDY_PIN_CONNECTED // ???
};

static const struct pit8253_config mm1_pit8253_intf =
{
	{
		{
			XTAL_6_144MHz/2/2,
			NULL /* _ITxC */
		}, {
			XTAL_6_144MHz/2/2,
			NULL /* _IRxC */
		}, {
			XTAL_6_144MHz/2/2,
			NULL /* _AUXC */
		}
	}
};

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xfeff) AM_RAM
	AM_RANGE(0xff00, 0xff0f) AM_DEVREADWRITE(I8237_TAG, dma8237_r, dma8237_w)
//	AM_RANGE(0xff10, 0xff1f) µPD7201
    AM_RANGE(0xff20, 0xff21) AM_DEVREADWRITE(I8275_TAG, i8275_r, i8275_w)
	AM_RANGE(0xff30, 0xff33) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
//	AM_RANGE(0xff40, 0xff4f) 8212
	AM_RANGE(0xff50, 0xff50) AM_DEVREAD(UPD765_TAG, nec765_status_r)
	AM_RANGE(0xff51, 0xff51) AM_DEVREADWRITE(UPD765_TAG, nec765_data_r, nec765_data_w)
//	AM_RANGE(0xff60, 0xff6f)
//	AM_RANGE(0xff70, 0xff7f) µPD7220
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* look up devices */
	state->i8237 = devtag_get_device(machine, I8237_TAG);
	state->upd765 = devtag_get_device(machine, UPD765_TAG);
	state->i8275 = devtag_get_device(machine, I8275_TAG);

	/* register for state saving */
	//state_save_register_global(machine, state->);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mm1 )
	MDRV_DRIVER_DATA(mm1_state)

	/* basic system hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, XTAL_6_144MHz)
	MDRV_CPU_PROGRAM_MAP(mm1_map)

	MDRV_MACHINE_START(mm1)

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, RASTER )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 800, 327 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 327-1 )

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(mm1)

	/* peripheral hardware */
	MDRV_I8275_ADD(I8275_TAG, mm1_i8275_intf)
	MDRV_DMA8237_ADD(I8237_TAG, mm1_dma8237_intf)
	MDRV_PIT8253_ADD(I8253_TAG, mm1_pit8253_intf)
	MDRV_NEC765A_ADD(UPD765_TAG, mm1_nec765_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mm1m6 )
	MDRV_IMPORT_FROM(mm1)

	MDRV_UPD7220_ADD(UPD7220_TAG, 0, mm1_upd7220_intf)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x1000, I8085_TAG, 0 )
	ROM_LOAD( "mm1.bin", 0x0000, 0x1000, CRC(07400e72) SHA1(354ff97817a607ca38d296af8b2813878d092a08) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen", 0x0000, 0x1000, NO_DUMP )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Configuration */

static FLOPPY_OPTIONS_START( mm1 )
	FLOPPY_OPTION( mm1, "dsk", "Nokia MikroMikko 1 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([8])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

#ifdef UNUSED_CODE
static DEVICE_IMAGE_LOAD( mm2_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		switch (size)
		{
		case 40*2*9*512: // 360KB
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 9, 512, 1, 0, FALSE);
			break;

		case 40*2*18*512: // 720KB (number of sectors has been doubled, instead of tracks as in 720KB DOS floppies)
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 18, 512, 1, 0, FALSE);
			break;

		default:
			return INIT_FAIL;
		}

		return INIT_PASS;
	}

	return INIT_FAIL;
}
#endif

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
