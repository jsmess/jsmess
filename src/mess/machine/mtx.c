/*************************************************************************

    machine/mtx.c

    Memotech MTX 500, MTX 512 and RS 128

**************************************************************************/


/* Core includes */
#include "driver.h"
#include "deprecat.h"
#include "includes/mtx.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "video/tms9928a.h"

/* Devices */
#include "devices/printer.h"
#include "devices/snapquik.h"


#define MTX_PRT_BUSY		1
#define MTX_PRT_NOERROR		2
#define MTX_PRT_EMPTY		4
#define MTX_PRT_SELECTED	8



/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT8 key_sense;

static char mtx_prt_strobe = 0;
static char mtx_prt_data = 0;



/*************************************
 *
 *  TMS9929A video chip
 *
 *************************************/

static void mtx_tms9929a_interrupt(int data)
{
	z80ctc_0_trg0_w(0, data ? 0 : 1);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	mtx_tms9929a_interrupt
};

INTERRUPT_GEN( mtx_interrupt )
{
	TMS9928A_interrupt();
}



/*************************************
 *
 *  Snapshots
 *
 *************************************/

SNAPSHOT_LOAD( mtx )
{
	UINT8 header[18];
	UINT16 sys_addr;
	
	/* get the header */
	image_fread(image, &header, sizeof(header));

	if (header[0] == 0xff)
	{
		/* long header */
		sys_addr = pick_integer_le(header, 16, 2);
		image_fread(image, mtx_ram + (sys_addr - 0xc000), 599);
		image_fread(image, mtx_ram, snapshot_size - 599 - 18);
	}
	else
	{
		/* short header */
		sys_addr = pick_integer_le(header, 0, 2);
		image_fseek(image, 4, SEEK_SET);
		image_fread(image, mtx_ram + (sys_addr - 0xc000), 599);
		image_fread(image, mtx_ram, snapshot_size - 599 - 4);
	}
	
	return INIT_PASS;
}



/*************************************
 *
 *  Cassette
 *
 *************************************/

READ8_HANDLER( mtx_cst_r )
{
	return 0xff;
}

WRITE8_HANDLER( mtx_cst_w )
{
}



/*************************************
 *
 *  Printer
 *
 *************************************/

static mess_image *mtx_printer_image(void)
{
	return image_from_devtype_and_index(IO_PRINTER, 0);
}

READ8_HANDLER( mtx_strobe_r )
{
	if (mtx_prt_strobe == 0)
		printer_output (mtx_printer_image (), mtx_prt_data);

	mtx_prt_strobe = 1;

	return 0;
}


READ8_HANDLER( mtx_prt_r )
{
	mtx_prt_strobe = 0;

	return MTX_PRT_NOERROR | (printer_status (mtx_printer_image (), 0)
			? MTX_PRT_SELECTED : 0);
}

WRITE8_HANDLER( mtx_prt_w )
{
	mtx_prt_data = data;
}



/*************************************
 *
 *  Keyboard
 *
 *************************************/

WRITE8_HANDLER( mtx_sense_w )
{
	key_sense = data;
}

READ8_HANDLER( mtx_key_lo_r )
{
	UINT8 data = 0xff;

	if (!(key_sense & 0x01)) data &= readinputportbytag("keyboard_low_0");
	if (!(key_sense & 0x02)) data &= readinputportbytag("keyboard_low_1");
	if (!(key_sense & 0x04)) data &= readinputportbytag("keyboard_low_2");
	if (!(key_sense & 0x08)) data &= readinputportbytag("keyboard_low_3");
	if (!(key_sense & 0x10)) data &= readinputportbytag("keyboard_low_4");
	if (!(key_sense & 0x20)) data &= readinputportbytag("keyboard_low_5");
	if (!(key_sense & 0x40)) data &= readinputportbytag("keyboard_low_6");
	if (!(key_sense & 0x80)) data &= readinputportbytag("keyboard_low_7");

	return data;
}

READ8_HANDLER( mtx_key_hi_r )
{
	UINT8 data = readinputportbytag("country_code");

	if (!(key_sense & 0x01)) data &= readinputportbytag("keyboard_high_0");
	if (!(key_sense & 0x02)) data &= readinputportbytag("keyboard_high_1");
	if (!(key_sense & 0x04)) data &= readinputportbytag("keyboard_high_2");
	if (!(key_sense & 0x08)) data &= readinputportbytag("keyboard_high_3");
	if (!(key_sense & 0x10)) data &= readinputportbytag("keyboard_high_4");
	if (!(key_sense & 0x20)) data &= readinputportbytag("keyboard_high_5");
	if (!(key_sense & 0x40)) data &= readinputportbytag("keyboard_high_6");
	if (!(key_sense & 0x80)) data &= readinputportbytag("keyboard_high_7");

	return data;
}



/*************************************
 *
 *  Z80 CTC
 *
 *************************************/

static void mtx_ctc_interrupt(int state)
{
//  logerror("mtx_ctc_interrupt: %02x\n", state);
	cpunum_set_input_line(Machine, 0, 0, state);
}

READ8_HANDLER( mtx_ctc_r )
{
	return z80ctc_0_r(offset);
}

WRITE8_HANDLER( mtx_ctc_w )
{
//  logerror("mtx_ctc_w: %02x\n", data);
	if (offset < 3)
		z80ctc_0_w(offset,data);
}

static z80ctc_interface mtx_ctc_intf =
{
	MTX_SYSTEM_CLOCK,
	0,
	mtx_ctc_interrupt,
	0,
	0,
	0
};



/*************************************
 *
 *  Z80 Dart
 *
 *************************************/

READ8_HANDLER( mtx_dart_data_r )
{
	return z80dart_d_r(0, offset);
}

READ8_HANDLER( mtx_dart_control_r )
{
	return z80dart_c_r(0, offset);
}

WRITE8_HANDLER( mtx_dart_data_w )
{
	z80dart_d_w(0, offset, data);
}

WRITE8_HANDLER( mtx_dart_control_w )
{
	z80dart_c_w(0, offset, data);
}

static const z80dart_interface mtx_dart_intf =
{
		MTX_SYSTEM_CLOCK,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
};



/*************************************
 *
 *  Bank switching
 *
 *************************************/

/*
    There are two memory models on the MTX, the standard one and a
    CBM mode. In standard mode, the memory map is defined as:

    0x0000 - 0x1fff  OSROM
    0x2000 - 0x3fff  Paged ROM
    0x4000 - 0x7fff  Paged RAM
    0x8000 - 0xbfff  Paged RAM
    0xc000 - 0xffff  RAM

    Banks are selected by output port 0. Bits 0-3 define the RAM page
    and bits 4-6 the ROM page.

    CBM mode is selected by bit 7 of output port 0. ROM is replaced
    by RAM in this mode.
*/

WRITE8_HANDLER( mtx_bankswitch_w )
{
//  UINT8 cbm_mode = data >> 7 & 0x01;
	UINT8 rom_page = data >> 4 & 0x03;
	UINT8 ram_page = data >> 0 & 0x0f;

	/* set rom bank (switches between basic and assembler rom or cartridges) */
	memory_set_bank(2, rom_page);

	/* set ram bank, for invalid pages a nop-handler will be installed */
	if (ram_page >= mess_ram_size/0x8000)
	{
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_NOP, MWA8_NOP);
	}
	else if (ram_page + 1 == mess_ram_size/0x8000)
	{
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK4, MWA8_BANK4);
		memory_set_bank(4, ram_page);
	}
	else
	{
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, MRA8_BANK3, MWA8_BANK3);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, MRA8_BANK4, MWA8_BANK4);
		memory_set_bank(3, ram_page);
		memory_set_bank(4, ram_page);
	}
}



/*************************************
 *
 *  Machine initialization
 *
 *************************************/

DRIVER_INIT( mtx512 )
{
	/* configure memory */
	memory_set_bankptr(1, memory_region(REGION_USER1));
	memory_configure_bank(2, 0, 8, memory_region(REGION_USER2), 0x2000);
	memory_configure_bank(3, 0, mess_ram_size/0x4000/2, mess_ram, 0x4000);
	memory_configure_bank(4, 0, mess_ram_size/0x4000/2, mess_ram + mess_ram_size/2, 0x4000);

	/* setup tms9928a */
	TMS9928A_configure(&tms9928a_interface);

	/* setup ctc */
	z80ctc_init(0, &mtx_ctc_intf);
}

DRIVER_INIT( rs128 )
{
	DRIVER_INIT_CALL(mtx512);

	z80dart_init(0, &mtx_dart_intf);

	/* install handlers for dart interface */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0x0c, 0x0d, 0, 0, mtx_dart_data_r, mtx_dart_data_w);
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_IO, 0x0e, 0x0f, 0, 0, mtx_dart_control_r, mtx_dart_control_w);
}

MACHINE_RESET( rs128 )
{
	z80dart_reset(0);
}
