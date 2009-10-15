/*************************************************************************

    machine/mtx.c

    Memotech MTX 500, MTX 512 and RS 128

**************************************************************************/


#include "driver.h"
#include "includes/mtx.h"
#include "cpu/z80/z80.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "video/tms9928a.h"
#include "machine/ctronics.h"
#include "devices/snapquik.h"
#include "devices/messram.h"


/*************************************
 *
 *  Global variables
 *
 *************************************/

static UINT8 key_sense;



/*************************************
 *
 *  TMS9929A video chip
 *
 *************************************/

static void mtx_tms9929a_interrupt(running_machine *machine, int data)
{
	z80ctc_trg0_w(devtag_get_device(machine, "z80ctc"), 0, data ? 0 : 1);
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
	TMS9928A_interrupt(device->machine);
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

READ8_DEVICE_HANDLER( mtx_strobe_r )
{
	/* set STROBE low */
	centronics_strobe_w(device, FALSE);

	return 0xff;
}


READ8_DEVICE_HANDLER( mtx_prt_r )
{
	UINT8 result = 0;

	/* reset STROBE to high */
	centronics_strobe_w(device, TRUE);

	/* fill in centronics printer status */
	result |= centronics_busy_r(device) << 0;
	result |= centronics_fault_r(device) << 1;
	result |= !centronics_pe_r(device) << 2;
	result |= centronics_vcc_r(device) << 3;

	return result;
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

	if (!(key_sense & 0x01)) data &= input_port_read(space->machine, "keyboard_low_0");
	if (!(key_sense & 0x02)) data &= input_port_read(space->machine, "keyboard_low_1");
	if (!(key_sense & 0x04)) data &= input_port_read(space->machine, "keyboard_low_2");
	if (!(key_sense & 0x08)) data &= input_port_read(space->machine, "keyboard_low_3");
	if (!(key_sense & 0x10)) data &= input_port_read(space->machine, "keyboard_low_4");
	if (!(key_sense & 0x20)) data &= input_port_read(space->machine, "keyboard_low_5");
	if (!(key_sense & 0x40)) data &= input_port_read(space->machine, "keyboard_low_6");
	if (!(key_sense & 0x80)) data &= input_port_read(space->machine, "keyboard_low_7");

	return data;
}

READ8_HANDLER( mtx_key_hi_r )
{
	UINT8 data = input_port_read(space->machine, "country_code");

	if (!(key_sense & 0x01)) data &= input_port_read(space->machine, "keyboard_high_0");
	if (!(key_sense & 0x02)) data &= input_port_read(space->machine, "keyboard_high_1");
	if (!(key_sense & 0x04)) data &= input_port_read(space->machine, "keyboard_high_2");
	if (!(key_sense & 0x08)) data &= input_port_read(space->machine, "keyboard_high_3");
	if (!(key_sense & 0x10)) data &= input_port_read(space->machine, "keyboard_high_4");
	if (!(key_sense & 0x20)) data &= input_port_read(space->machine, "keyboard_high_5");
	if (!(key_sense & 0x40)) data &= input_port_read(space->machine, "keyboard_high_6");
	if (!(key_sense & 0x80)) data &= input_port_read(space->machine, "keyboard_high_7");

	return data;
}



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
	UINT8 rom_page = data >> 4 & 0x07;
	UINT8 ram_page = data >> 0 & 0x0f;

	/* set rom bank (switches between basic and assembler rom or cartridges) */
	memory_set_bank(space->machine, 2, rom_page);

	/* set ram bank, for invalid pages a nop-handler will be installed */
	if (ram_page >= messram_get_size(devtag_get_device(space->machine, "messram"))/0x8000)
	{
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0x7fff, 0, 0, SMH_NOP, SMH_NOP);
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0, SMH_NOP, SMH_NOP);
	}
	else if (ram_page + 1 == messram_get_size(devtag_get_device(space->machine, "messram"))/0x8000)
	{
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0x7fff, 0, 0, SMH_NOP, SMH_NOP);
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bank(space->machine, 4, ram_page);
	}
	else
	{
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0x7fff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_install_readwrite8_handler(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bank(space->machine, 3, ram_page);
		memory_set_bank(space->machine, 4, ram_page);
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
	memory_set_bankptr(machine, 1, memory_region(machine, "user1"));
	memory_configure_bank(machine, 2, 0, 8, memory_region(machine, "user2"), 0x2000);
	memory_configure_bank(machine, 3, 0, messram_get_size(devtag_get_device(machine, "messram"))/0x4000/2, messram_get_ptr(devtag_get_device(machine, "messram")), 0x4000);
	memory_configure_bank(machine, 4, 0, messram_get_size(devtag_get_device(machine, "messram"))/0x4000/2, messram_get_ptr(devtag_get_device(machine, "messram")) + messram_get_size(devtag_get_device(machine, "messram"))/2, 0x4000);

	/* setup tms9928a */
	TMS9928A_configure(&tms9928a_interface);
}

DRIVER_INIT( rs128 )
{
	const device_config *device;
	const address_space *space;

	DRIVER_INIT_CALL(mtx512);

	/* install handlers for dart interface */
	device = devtag_get_device(machine, "z80dart");
	space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_IO);
	memory_install_readwrite8_device_handler(space, device, 0x0c, 0x0d, 0, 0, z80dart_d_r, z80dart_d_w);
	memory_install_readwrite8_device_handler(space, device, 0x0e, 0x0f, 0, 0, z80dart_c_r, z80dart_c_w);
}
