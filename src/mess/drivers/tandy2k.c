/***************************************************************************

    Tandy 2000

    Skeleton driver.

****************************************************************************/

/*

    TODO:

    - 80186
    - CRT9007
    - keyboard ROM
    - hires graphics board
    - floppy 720K DSQD
    - DMA
    - WD1010
    - hard disk
    - mouse
    - save state

*/

#include "emu.h"
#include "includes/tandy2k.h"
#include "cpu/i86/i86.h"
#include "cpu/mcs48/mcs48.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/upd765.h"
#include "sound/speaker.h"
#include "video/crt9007.h"

/* Read/Write Handlers */

static void speaker_update(tandy2k_state *state)
{
	int level = !(state->spkrdata & state->outspkr);

	speaker_level_w(state->speaker, level);
}

static READ8_HANDLER( enable_r )
{
	/*

        bit     signal      description

        0                   RS-232 ring indicator
        1                   RS-232 carrier detect
        2
        3
        4
        5
        6
        7       _ACLOW

    */

	return 0x80;
}

static WRITE8_HANDLER( enable_w )
{
	/*

        bit     signal      description

        0       KBEN        keyboard enable
        1       EXTCLK      external baud rate clock
        2       SPKRGATE    enable periodic speaker output
        3       SPKRDATA    direct output to speaker
        4       RFRQGATE    enable refresh and baud rate clocks
        5       _FDCRESET   reset 8272
        6       TMRIN0      enable 80186 timer 0
        7       TMRIN1      enable 80186 timer 1

    */

	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();

	/* keyboard enable */
	state->kben = BIT(data, 0);

	/* external baud rate clock */
	state->extclk = BIT(data, 1);

	/* speaker gate */
	pit8253_gate0_w(state->pit, BIT(data, 2));

	/* speaker data */
	state->spkrdata = BIT(data, 3);
	speaker_update(state);

	/* refresh and baud rate clocks */
	pit8253_gate1_w(state->pit, BIT(data, 4));
	pit8253_gate2_w(state->pit, BIT(data, 4));

	/* FDC reset */
	upd765_reset_w(state->fdc, BIT(data, 5));

	/* TODO timer 0 enable */

	/* TODO timer 1 enable */
}

static WRITE8_HANDLER( dma_mux_w )
{
	/*

        bit     description

        0       DMA channel 0 enable
        1       DMA channel 1 enable
        2       DMA channel 2 enable
        3       DMA channel 3 enable
        4       DMA channel 0 select
        5       DMA channel 1 select
        6       DMA channel 2 select
        7       DMA channel 3 select

    */

	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();

	state->dma_mux = data;

	/* check for DMA error */
	int drq0 = 0;
	int drq1 = 0;

	for (int ch = 0; ch < 4; ch++)
	{
		if (BIT(data, ch)) { if (BIT(data, ch + 4)) drq1++; else drq0++; }
	}

	int dme = (drq0 > 2) || (drq1 > 2);
	pic8259_ir6_w(state->pic1, dme);
}

static void dma_request(running_machine *machine, int line, int state)
{

}

static READ8_DEVICE_HANDLER( fldtc_r )
{
	upd765_tc_w(device, 1);
	upd765_tc_w(device, 0);

	return 0;
}

static WRITE8_DEVICE_HANDLER( fldtc_w )
{
	upd765_tc_w(device, 1);
	upd765_tc_w(device, 0);
}

static WRITE8_HANDLER( addr_ctrl_w )
{
	/*

        bit     signal      description

        8       A15         A15 of video access
        9       A16         A16 of video access
        10      A17         A17 of video access
        11      A18         A18 of video access
        12      A19         A19 of video access
        13      CLKSPD      clock speed (0 = 22.4 MHz, 1 = 28 MHz)
        14      CLKCNT      dots/char (0 = 10 [800x400], 1 = 8 [640x400])
        15      VIDOUTS     selects the video source for display on monochrome monitor

    */

	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();

	/* video access */
	state->vram_base = data << 15;

	/* video clock speed */
	crt9007_set_clock(state->vpac, BIT(data, 5) ? XTAL_16MHz*28/16 : XTAL_16MHz*28/20);

	/* dots per char */
	crt9007_set_hpixels_per_column(state->vpac, BIT(data, 6) ? 8 : 10);

	/* video source select */
	state->vidouts = BIT(data, 7);

	logerror("Address Control %02x\n", data);
}

//static READ8_HANDLER( keyboard_x0_r )
//{
//	/*
//
//        bit     description
//
//        0       X0
//        1       X1
//        2       X2
//        3       X3
//        4       X4
//        5       X5
//        6       X6
//        7       X7
//
//    */
//
//	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();
//
//	UINT8 data = 0xff;
//
//	if (!BIT(state->keylatch,  0)) data &= input_port_read(space->machine,  "Y0");
//	if (!BIT(state->keylatch,  1)) data &= input_port_read(space->machine,  "Y1");
//	if (!BIT(state->keylatch,  2)) data &= input_port_read(space->machine,  "Y2");
//	if (!BIT(state->keylatch,  3)) data &= input_port_read(space->machine,  "Y3");
//	if (!BIT(state->keylatch,  4)) data &= input_port_read(space->machine,  "Y4");
//	if (!BIT(state->keylatch,  5)) data &= input_port_read(space->machine,  "Y5");
//	if (!BIT(state->keylatch,  6)) data &= input_port_read(space->machine,  "Y6");
//	if (!BIT(state->keylatch,  7)) data &= input_port_read(space->machine,  "Y7");
//	if (!BIT(state->keylatch,  8)) data &= input_port_read(space->machine,  "Y8");
//	if (!BIT(state->keylatch,  9)) data &= input_port_read(space->machine,  "Y9");
//	if (!BIT(state->keylatch, 10)) data &= input_port_read(space->machine, "Y10");
//	if (!BIT(state->keylatch, 11)) data &= input_port_read(space->machine, "Y11");
//
//	return ~data;
//}
//
//static WRITE8_HANDLER( keyboard_y0_w )
//{
//	/*
//
//        bit     description
//
//        0       Y0
//        1       Y1
//        2       Y2
//        3       Y3
//        4       Y4
//        5       Y5
//        6       Y6
//        7       Y7
//
//    */
//
//	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();
//
//	/* keyboard row select */
//	state->keylatch = (state->keylatch & 0xff00) | data;
//}
//
//static WRITE8_HANDLER( keyboard_y8_w )
//{
//	/*
//
//        bit     description
//
//        0       Y8
//        1       Y9
//        2       Y10
//        3       Y11
//        4       LED 2
//        5       LED 1
//        6
//        7
//
//    */
//
//	tandy2k_state *state = space->machine->driver_data<tandy2k_state>();
//
//	/* keyboard row select */
//	state->keylatch = ((data & 0x0f) << 8) | (state->keylatch & 0xff);
//}

/* Memory Maps */

static ADDRESS_MAP_START( tandy2k_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
//  AM_RANGE(0x00000, 0xdffff) AM_RAM
//  AM_RANGE(0xe0000, 0xf7fff) AM_RAM // hires graphics
	AM_RANGE(0xf8000, 0xfbfff) AM_RAM AM_BASE_MEMBER(tandy2k_state, char_ram)
	AM_RANGE(0xfc000, 0xfdfff) AM_MIRROR(0x2000) AM_ROM AM_REGION(I80186_TAG, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tandy2k_io, ADDRESS_SPACE_IO, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x00001) AM_READWRITE8(enable_r, enable_w, 0x00ff)
	AM_RANGE(0x00002, 0x00003) AM_WRITE8(dma_mux_w, 0x00ff)
	AM_RANGE(0x00004, 0x00005) AM_DEVREADWRITE8(I8272A_TAG, fldtc_r, fldtc_w, 0x00ff)
	AM_RANGE(0x00010, 0x00013) AM_DEVREADWRITE8(I8251A_TAG, msm8251_data_r, msm8251_data_w, 0x00ff)
	AM_RANGE(0x00030, 0x00031) AM_DEVREAD8(I8272A_TAG, upd765_status_r, 0x00ff)
	AM_RANGE(0x00032, 0x00033) AM_DEVREADWRITE8(I8272A_TAG, upd765_data_r, upd765_data_w, 0x00ff)
	AM_RANGE(0x00040, 0x00047) AM_DEVREADWRITE8(I8253_TAG, pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x00050, 0x00057) AM_DEVREADWRITE8(I8255A_TAG, i8255a_r, i8255a_w, 0x00ff)
	AM_RANGE(0x00060, 0x00063) AM_DEVREADWRITE8(I8259A_0_TAG, pic8259_r, pic8259_w, 0x00ff)
	AM_RANGE(0x00070, 0x00073) AM_DEVREADWRITE8(I8259A_1_TAG, pic8259_r, pic8259_w, 0x00ff)
	AM_RANGE(0x00080, 0x00081) AM_DEVREADWRITE8(I8272A_TAG, upd765_dack_r, upd765_dack_w, 0x00ff)
	AM_RANGE(0x00100, 0x0017f) AM_DEVREADWRITE8(CRT9007_TAG, crt9007_r, crt9007_w, 0x00ff)
	AM_RANGE(0x00100, 0x0017f) AM_WRITE8(addr_ctrl_w, 0xff00)
//  AM_RANGE(0x00180, 0x00180) AM_READ8(hires_status_r, 0x00ff)
//  AM_RANGE(0x00180, 0x001bf) AM_WRITE(hires_palette_w)
//  AM_RANGE(0x001a0, 0x001a0) AM_READ8(hires_plane_w, 0x00ff)
//  AM_RANGE(0x0ff00, 0x0ffff) AM_READWRITE(i186_internal_port_r, i186_internal_port_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tandy2k_hd_io, ADDRESS_SPACE_IO, 16 )
	AM_IMPORT_FROM(tandy2k_io)
//  AM_RANGE(0x000e0, 0x000ff) AM_WRITE8(hdc_dack_w, 0x00ff)
//  AM_RANGE(0x0026c, 0x0026d) AM_DEVREADWRITE8(WD1010_TAG, hdc_reset_r, hdc_reset_w, 0x00ff)
//  AM_RANGE(0x0026e, 0x0027f) AM_DEVREADWRITE8(WD1010_TAG, wd1010_r, wd1010_w, 0x00ff)
ADDRESS_MAP_END

//static ADDRESS_MAP_START( keyboard_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_WRITE(keyboard_y0_w)
//	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_WRITE(keyboard_y8_w)
//	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_READ(keyboard_x0_r)
//ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tandy2k )
	PORT_START("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW9")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW10")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW11")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )
INPUT_PORTS_END

/* Video */

static VIDEO_START( tandy2k )
{
	tandy2k_state *state = machine->driver_data<tandy2k_state>();

	/* find devices */
	state->vpac = machine->device(CRT9007_TAG);
}

static VIDEO_UPDATE( tandy2k )
{
	tandy2k_state *state = screen->machine->driver_data<tandy2k_state>();

	//if (state->vidouts)
	{
		crt9007_update(state->vpac, bitmap, cliprect);
	}

	return 0;
}

static CRT9007_DRAW_SCANLINE( tandy2k_crt9007_display_pixels )
{
	tandy2k_state *state = device->machine->driver_data<tandy2k_state>();
	address_space *program = cputag_get_address_space(device->machine, I80186_TAG, ADDRESS_SPACE_PROGRAM);

	for (int sx = 0; sx < x_count; sx++)
	{
		UINT32 videoram_addr = (state->vram_base | (va << 1)) + sx;
		UINT8 videoram_data = program->read_word(videoram_addr);
		UINT16 charram_addr = (videoram_data << 4) | sl;
		UINT8 charram_data = state->char_ram[charram_addr] & 0xff;

		for (int bit = 0; bit < 10; bit++)
		{
			if (BIT(charram_data, 7))
			{
				*BITMAP_ADDR16(bitmap, y, x + (sx * 10) + bit) = 1;
			}

			charram_data <<= 1;
		}
	}
}

static CRT9007_INTERFACE( crt9007_intf )
{
	SCREEN_TAG,
	10,
	tandy2k_crt9007_display_pixels,
	DEVCB_DEVICE_LINE(I8259A_1_TAG, pic8259_ir1_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* Intel 8251A Interface */

static WRITE_LINE_DEVICE_HANDLER( rxrdy_w )
{
	tandy2k_state *driver_state = device->machine->driver_data<tandy2k_state>();

	driver_state->rxrdy = state;
	pic8259_ir2_w(driver_state->pic0, driver_state->rxrdy | driver_state->txrdy);
}

static WRITE_LINE_DEVICE_HANDLER( txrdy_w )
{
	tandy2k_state *driver_state = device->machine->driver_data<tandy2k_state>();

	driver_state->txrdy = state;
	pic8259_ir2_w(driver_state->pic0, driver_state->rxrdy | driver_state->txrdy);
}

static const msm8251_interface i8251_intf =
{
	txrdy_w,
	NULL,
	rxrdy_w
};

/* Intel 8253 Interface */

static WRITE_LINE_DEVICE_HANDLER( outspkr_w )
{
	tandy2k_state *driver_state = device->machine->driver_data<tandy2k_state>();

	driver_state->outspkr = state;
	speaker_update(driver_state);
}

static WRITE_LINE_DEVICE_HANDLER( intbrclk_w )
{
	tandy2k_state *driver_state = device->machine->driver_data<tandy2k_state>();

	if (!driver_state->extclk && state)
	{
		msm8251_transmit_clock(driver_state->uart);
		msm8251_receive_clock(driver_state->uart);
	}
}

static WRITE_LINE_DEVICE_HANDLER( rfrqpulse_w )
{
}

static const struct pit8253_config i8253_intf =
{
	{
		{
			XTAL_16MHz/16,
			DEVCB_NULL,
			DEVCB_LINE(outspkr_w)
		}, {
			XTAL_16MHz/8,
			DEVCB_NULL,
			DEVCB_LINE(intbrclk_w)
		}, {
			XTAL_16MHz/8,
			DEVCB_NULL,
			DEVCB_LINE(rfrqpulse_w)
		}
	}
};

/* Intel 8255A Interface */

enum
{
	LPINEN = 0,
	KBDINEN,
	PORTINEN
};

static READ8_DEVICE_HANDLER( ppi_pb_r )
{
	/*

        bit     signal          description

        0       LPRIN0          auxiliary input 0
        1       LPRIN1          auxiliary input 1
        2       LPRIN2          auxiliary input 2
        3       _LPRACK         acknowledge
        4       _LPRFLT         fault
        5       _LPRSEL         select
        6       LPRPAEM         paper empty
        7       LPRBSY          busy

    */

	tandy2k_state *state = device->machine->driver_data<tandy2k_state>();

	UINT8 data = 0;

	switch (state->pb_sel)
	{
	case LPINEN:
		/* printer acknowledge */
		data |= centronics_ack_r(state->centronics) << 3;

		/* printer fault */
		data |= centronics_fault_r(state->centronics) << 4;

		/* printer select */
		data |= centronics_vcc_r(state->centronics) << 5;

		/* paper empty */
		data |= centronics_pe_r(state->centronics) << 6;

		/* printer busy */
		data |= centronics_busy_r(state->centronics) << 7;
		break;

	case KBDINEN:
		/* keyboard data */
		break;

	case PORTINEN:
		/* PCB revision */
		data = 0x03;
		break;
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( ppi_pc_w )
{
	/*

        bit     signal          description

        0                       port A direction
        1                       port B input select bit 0
        2                       port B input select bit 1
        3       LPRINT13        interrupt
        4       STROBE IN
        5       INBUFFULL
        6       _LPRACK
        7       _LPRDATSTB

    */

	tandy2k_state *state = device->machine->driver_data<tandy2k_state>();

	/* input select */
	state->pb_sel = (data >> 1) & 0x03;

	/* interrupt */
	pic8259_ir3_w(state->pic1, BIT(data, 3));

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 7));
}

static I8255A_INTERFACE( i8255_intf )
{
	DEVCB_NULL,							// Port A read
	DEVCB_HANDLER(ppi_pb_r),			// Port B write
	DEVCB_NULL,							// Port C read
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	/* port A write callback */
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(ppi_pc_w)				// Port C write
};

/* Intel 8259 Interfaces */

/*

    IR0     MEMINT00
    IR1     TMOINT01
    IR2     SERINT02
    IR3     BUSINT03
    IR4     FLDINT04
    IR5     BUSINT05
    IR6     HDCINT06
    IR7     BUSINT07

*/

static const struct pic8259_interface i8259_0_intf =
{
	DEVCB_CPU_INPUT_LINE(I80186_TAG, 0)
};

/*

    IR0     KBDINT10
    IR1     VIDINT11
    IR2     RATINT12
    IR3     LPRINT13
    IR4     MCPINT14
    IR5     MEMINT15
    IR6     DMEINT16
    IR7     BUSINT17

*/

static const struct pic8259_interface i8259_1_intf =
{
	/* TODO: INT1 is not implemented in the 80186 core */
	DEVCB_CPU_INPUT_LINE(I80186_TAG, 1)
};

/* Floppy Configuration */

static const floppy_config tandy2k_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

/* Intel 8272 Interface */

static UPD765_DMA_REQUEST( busdmarq0_w )
{
	dma_request(device->machine, 0, state);
}

static const struct upd765_interface i8272_intf =
{
	DEVCB_DEVICE_LINE(I8259A_0_TAG, pic8259_ir4_w),
	busdmarq0_w,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* Centronics Interface */

static const centronics_interface centronics_intf =
{
	0,												/* is IBM PC? */
	DEVCB_DEVICE_LINE(I8255A_TAG, i8255a_pc6_w),	/* ACK output */
	DEVCB_NULL,										/* BUSY output */
	DEVCB_NULL										/* NOT BUSY output */
};

/* Machine Initialization */

static MACHINE_START( tandy2k )
{
	tandy2k_state *state = machine->driver_data<tandy2k_state>();

	/* find devices */
	state->uart = machine->device(I8251A_TAG);
	state->pit = machine->device(I8253_TAG);
	state->pic0 = machine->device(I8259A_0_TAG);
	state->pic1 = machine->device(I8259A_1_TAG);
	state->fdc = machine->device(I8272A_TAG);
	state->speaker = machine->device(SPEAKER_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);

	/* memory banking */
	address_space *program = cputag_get_address_space(machine, I80186_TAG, ADDRESS_SPACE_PROGRAM);
	UINT8 *ram = messram_get_ptr(machine->device("messram"));
	int ram_size = messram_get_size(machine->device("messram"));

	memory_install_ram(program, 0x00000, ram_size - 1, 0, 0, ram);

	/* patch out i186 relocation register check */
	UINT8 *rom = memory_region(machine, I80186_TAG);
	rom[0x1f16] = 0x90;
	rom[0x1f17] = 0x90;

	/* register for state saving */
//  state_save_register_global(machine, state->);
}

static MACHINE_RESET( tandy2k )
{
}

/* Machine Driver */

static MACHINE_CONFIG_START( tandy2k, tandy2k_state )

    /* basic machine hardware */
	MDRV_CPU_ADD(I80186_TAG, I80186, XTAL_16MHz)
    MDRV_CPU_PROGRAM_MAP(tandy2k_mem)
    MDRV_CPU_IO_MAP(tandy2k_io)

//  MDRV_CPU_ADD(I8048_TAG, I8048, 1000000) // ?
//  MDRV_CPU_IO_MAP(keyboard_io)

    MDRV_MACHINE_START(tandy2k)
    MDRV_MACHINE_RESET(tandy2k)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(tandy2k)
    MDRV_VIDEO_UPDATE(tandy2k)
	MDRV_CRT9007_ADD(CRT9007_TAG, XTAL_16MHz*28/16, crt9007_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8255A_ADD(I8255A_TAG, i8255_intf)
	MDRV_MSM8251_ADD(I8251A_TAG, i8251_intf)
	MDRV_PIT8253_ADD(I8253_TAG, i8253_intf)
	MDRV_PIC8259_ADD(I8259A_0_TAG, i8259_0_intf)
	MDRV_PIC8259_ADD(I8259A_1_TAG, i8259_1_intf)
	MDRV_UPD765A_ADD(I8272A_TAG, i8272_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(tandy2k_floppy_config)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("256K,384K,512K,640K,768K,896K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( tandy2k_hd, tandy2k )

    /* basic machine hardware */
	MDRV_CPU_MODIFY(I80186_TAG)
    MDRV_CPU_IO_MAP(tandy2k_hd_io)

	/* Tandon TM502 hard disk */
	//MDRV_WD1010_ADD(WD1010_TAG, wd1010_intf)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( tandy2k )
	ROM_REGION( 0x2000, I80186_TAG, 0 )
	ROM_LOAD16_BYTE( "484a00.u48", 0x0000, 0x1000, CRC(a5ee3e90) SHA1(4b1f404a4337c67065dd272d62ff88dcdee5e34b) )
	ROM_LOAD16_BYTE( "474600.u47", 0x0001, 0x1000, CRC(345701c5) SHA1(a775cbfa110b7a88f32834aaa2a9b868cbeed25b) )

	ROM_REGION( 0x400, I8048_TAG, 0 )
	ROM_LOAD( "keyboard.m1", 0x0000, 0x0400, NO_DUMP )
ROM_END

#define rom_tandy2khd rom_tandy2k

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY                 FULLNAME        FLAGS */
COMP( 1983, tandy2k,	0,			0,		tandy2k,	tandy2k,	0,		"Tandy Radio Shack",	"Tandy 2000",	GAME_NOT_WORKING)
COMP( 1983, tandy2khd,	tandy2k,	0,		tandy2k_hd,	tandy2k,	0,		"Tandy Radio Shack",	"Tandy 2000HD",	GAME_NOT_WORKING)
