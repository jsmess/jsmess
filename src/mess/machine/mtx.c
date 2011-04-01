/*************************************************************************

    machine/mtx.c

    Memotech MTX 500, MTX 512 and RS 128

**************************************************************************/

#include "emu.h"
#include "imageutl.h"
#include "includes/mtx.h"
#include "cpu/z80/z80.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "imagedev/snapquik.h"
#include "machine/ctronics.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "video/tms9928a.h"
#include "sound/sn76496.h"

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*-------------------------------------------------
    mtx_strobe_r - centronics strobe
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mtx_strobe_r )
{
	/* set STROBE low */
	centronics_strobe_w(device, FALSE);

	return 0xff;
}

/*-------------------------------------------------
    mtx_bankswitch_w - bankswitch
-------------------------------------------------*/

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

static void bankswitch(running_machine &machine, UINT8 data)
{
	/*

        bit     description

        0       P0
        1       P1
        2       P2
        3       P3
        4       R0
        5       R1
        6       R2
        7       RELCPMH

    */

	address_space *program = machine.device(Z80_TAG)->memory().space(AS_PROGRAM);
	device_t *messram = machine.device(RAM_TAG);

//  UINT8 cbm_mode = data >> 7 & 0x01;
	UINT8 rom_page = data >> 4 & 0x07;
	UINT8 ram_page = data >> 0 & 0x0f;

	/* set rom bank (switches between basic and assembler rom or cartridges) */
	memory_set_bank(machine, "bank2", rom_page);

	/* set ram bank, for invalid pages a nop-handler will be installed */
	if (ram_page >= ram_get_size(messram)/0x8000)
	{
		program->nop_readwrite(0x4000, 0x7fff);
		program->nop_readwrite(0x8000, 0xbfff);
	}
	else if (ram_page + 1 == ram_get_size(messram)/0x8000)
	{
		program->nop_readwrite(0x4000, 0x7fff);
		program->install_readwrite_bank(0x8000, 0xbfff, "bank4");
		memory_set_bank(machine, "bank4", ram_page);
	}
	else
	{
		program->install_readwrite_bank(0x4000, 0x7fff, "bank3");
		program->install_readwrite_bank(0x8000, 0xbfff, "bank4");
		memory_set_bank(machine, "bank3", ram_page);
		memory_set_bank(machine, "bank4", ram_page);
	}
}

WRITE8_HANDLER( mtx_bankswitch_w )
{
	bankswitch(space->machine(), data);
}

/*-------------------------------------------------
    mtx_sound_strobe_r - sound strobe
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mtx_sound_strobe_r )
{
	mtx_state *state = device->machine().driver_data<mtx_state>();

	sn76496_w(device, 0, state->m_sound_latch);

	return 0xff;
}

/*-------------------------------------------------
    mtx_sound_latch_w - sound latch write
-------------------------------------------------*/

WRITE8_HANDLER( mtx_sound_latch_w )
{
	mtx_state *state = space->machine().driver_data<mtx_state>();

	state->m_sound_latch = data;
}

/*-------------------------------------------------
    mtx_cst_w - cassette write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( mtx_cst_w )
{
	cassette_output(device, BIT(data, 0) ? -1 : 1);
}

/*-------------------------------------------------
    mtx_prt_r - centronics status
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mtx_prt_r )
{
	/*

        bit     description

        0       BUSY
        1       ERROR
        2       PE
        3       SLCT
        4
        5
        6
        7

    */

	UINT8 data = 0;

	/* reset STROBE to high */
	centronics_strobe_w(device, TRUE);

	/* busy */
	data |= centronics_busy_r(device) << 0;

	/* fault */
	data |= centronics_fault_r(device) << 1;

	/* paper empty */
	data |= !centronics_pe_r(device) << 2;

	/* select */
	data |= centronics_vcc_r(device) << 3;

	return data;
}

/*-------------------------------------------------
    mtx_sense_w - keyboard sense write
-------------------------------------------------*/

WRITE8_HANDLER( mtx_sense_w )
{
	mtx_state *state = space->machine().driver_data<mtx_state>();

	state->m_key_sense = data;
}

/*-------------------------------------------------
    mtx_key_lo_r - keyboard low read
-------------------------------------------------*/

READ8_HANDLER( mtx_key_lo_r )
{
	mtx_state *state = space->machine().driver_data<mtx_state>();

	UINT8 data = 0xff;

	if (!(state->m_key_sense & 0x01)) data &= input_port_read(space->machine(), "ROW0");
	if (!(state->m_key_sense & 0x02)) data &= input_port_read(space->machine(), "ROW1");
	if (!(state->m_key_sense & 0x04)) data &= input_port_read(space->machine(), "ROW2");
	if (!(state->m_key_sense & 0x08)) data &= input_port_read(space->machine(), "ROW3");
	if (!(state->m_key_sense & 0x10)) data &= input_port_read(space->machine(), "ROW4");
	if (!(state->m_key_sense & 0x20)) data &= input_port_read(space->machine(), "ROW5");
	if (!(state->m_key_sense & 0x40)) data &= input_port_read(space->machine(), "ROW6");
	if (!(state->m_key_sense & 0x80)) data &= input_port_read(space->machine(), "ROW7");

	return data;
}

/*-------------------------------------------------
    mtx_key_lo_r - keyboard high read
-------------------------------------------------*/

READ8_HANDLER( mtx_key_hi_r )
{
	mtx_state *state = space->machine().driver_data<mtx_state>();

	UINT8 data = input_port_read(space->machine(), "country_code");

	if (!(state->m_key_sense & 0x01)) data &= input_port_read(space->machine(), "ROW0") >> 8;
	if (!(state->m_key_sense & 0x02)) data &= input_port_read(space->machine(), "ROW1") >> 8;
	if (!(state->m_key_sense & 0x04)) data &= input_port_read(space->machine(), "ROW2") >> 8;
	if (!(state->m_key_sense & 0x08)) data &= input_port_read(space->machine(), "ROW3") >> 8;
	if (!(state->m_key_sense & 0x10)) data &= input_port_read(space->machine(), "ROW4") >> 8;
	if (!(state->m_key_sense & 0x20)) data &= input_port_read(space->machine(), "ROW5") >> 8;
	if (!(state->m_key_sense & 0x40)) data &= input_port_read(space->machine(), "ROW6") >> 8;
	if (!(state->m_key_sense & 0x80)) data &= input_port_read(space->machine(), "ROW7") >> 8;

	return data;
}

/*-------------------------------------------------
    hrx_address_w - HRX video RAM address
-------------------------------------------------*/

WRITE8_HANDLER( hrx_address_w )
{
	if (offset)
	{
		/*

            bit     description

            0       A8
            1       A9
            2       A10
            3
            4
            5       attribute memory write enable
            6       ASCII memory write enable
            7       cycle (0=read/1=write)

        */
	}
	else
	{
		/*

            bit     description

            0       A0
            1       A1
            2       A2
            3       A3
            4       A4
            5       A5
            6       A6
            7       A7

        */
	}
}

/*-------------------------------------------------
    hrx_data_r - HRX data read
-------------------------------------------------*/

READ8_HANDLER( hrx_data_r )
{
	return 0;
}

/*-------------------------------------------------
    hrx_data_w - HRX data write
-------------------------------------------------*/

WRITE8_HANDLER( hrx_data_w )
{
}

/*-------------------------------------------------
    hrx_attr_r - HRX attribute read
-------------------------------------------------*/

READ8_HANDLER( hrx_attr_r )
{
	return 0;
}

/*-------------------------------------------------
    hrx_attr_r - HRX attribute write
-------------------------------------------------*/

WRITE8_HANDLER( hrx_attr_w )
{
	/*

        bit     description

        0
        1
        2
        3
        4
        5
        6
        7

    */
}

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    TMS9928a_interface tms9928a_interface
-------------------------------------------------*/

static void mtx_tms9929a_interrupt(running_machine &machine, int data)
{
	mtx_state *state = machine.driver_data<mtx_state>();

	z80ctc_trg0_w(state->m_z80ctc, data ? 0 : 1);
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
	TMS9928A_interrupt(device->machine());
}

/***************************************************************************
    SNAPSHOT
***************************************************************************/

SNAPSHOT_LOAD( mtx )
{
	address_space *program = image.device().machine().device(Z80_TAG)->memory().space(AS_PROGRAM);

	UINT8 header[18];
	UINT16 addr;

	/* get the header */
	image.fread( &header, sizeof(header));

	if (header[0] == 0xff)
	{
		/* long header */
		addr = pick_integer_le(header, 16, 2);
		void *ptr = program->get_write_ptr(addr);
		image.fread( ptr, 599);
		ptr = program->get_write_ptr(0xc000);
		image.fread( ptr, snapshot_size - 599 - 18);
	}
	else
	{
		/* short header */
		addr = pick_integer_le(header, 0, 2);
		image.fseek(4, SEEK_SET);
		void *ptr = program->get_write_ptr(addr);
		image.fread( ptr, 599);
		ptr = program->get_write_ptr(0xc000);
		image.fread( ptr, snapshot_size - 599 - 4);
	}

	return IMAGE_INIT_PASS;
}

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    MACHINE_START( mtx512 )
-------------------------------------------------*/

MACHINE_START( mtx512 )
{
	mtx_state *state = machine.driver_data<mtx_state>();
	device_t *messram = machine.device(RAM_TAG);

	/* find devices */
	state->m_z80ctc = machine.device(Z80CTC_TAG);
	state->m_z80dart = machine.device(Z80DART_TAG);
	state->m_cassette = machine.device(CASSETTE_TAG);

	/* configure memory */
	memory_set_bankptr(machine, "bank1", machine.region("user1")->base());
	memory_configure_bank(machine, "bank2", 0, 8, machine.region("user2")->base(), 0x2000);
	memory_configure_bank(machine, "bank3", 0, ram_get_size(messram)/0x4000/2, ram_get_ptr(messram), 0x4000);
	memory_configure_bank(machine, "bank4", 0, ram_get_size(messram)/0x4000/2, ram_get_ptr(messram) + ram_get_size(messram)/2, 0x4000);

	/* setup tms9928a */
	TMS9928A_configure(&tms9928a_interface);
}

/*-------------------------------------------------
    MACHINE_RESET( mtx512 )
-------------------------------------------------*/

MACHINE_RESET( mtx512 )
{
	/* bank switching */
	bankswitch(machine, 0);
}
