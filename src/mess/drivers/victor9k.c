/***************************************************************************

    Victor 9000

    - very exciting hardware, disk controller is a direct descendant
      of the Commodore drives (designed by Chuck Peddle)

    Skeleton driver

***************************************************************************/

/*

    TODO:

    - floppy 8048
    - keyboard
    - add HD46505S variant to mc6845.c
    - hires graphics
    - character attributes
    - brightness/contrast
    - MC6852
    - codec sound

*/

#include "emu.h"
#include "includes/victor9k.h"
#include "cpu/i86/i86.h"
#include "cpu/mcs48/mcs48.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"
#include "machine/mc6852.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/upd7201.h"
#include "sound/hc55516.h"
#include "video/mc6845.h"

/* Memory Maps */

static ADDRESS_MAP_START( victor9k_mem, ADDRESS_SPACE_PROGRAM, 8 )
//  AM_RANGE(0x00000, 0xdffff) AM_RAM
	AM_RANGE(0xe0000, 0xe0001) AM_DEVREADWRITE(I8259A_TAG, pic8259_r, pic8259_w)
	AM_RANGE(0xe0020, 0xe0023) AM_DEVREADWRITE(I8253_TAG, pit8253_r, pit8253_w)
	AM_RANGE(0xe0040, 0xe0043) AM_DEVREADWRITE(UPD7201_TAG, upd7201_cd_ba_r, upd7201_cd_ba_w)
	AM_RANGE(0xe8000, 0xe8000) AM_DEVREADWRITE(HD46505S_TAG, mc6845_status_r, mc6845_address_w)
	AM_RANGE(0xe8001, 0xe8001) AM_DEVREADWRITE(HD46505S_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0xe8020, 0xe802f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0xe8040, 0xe804f) AM_DEVREADWRITE(M6522_2_TAG, via_r, via_w)
	AM_RANGE(0xe8060, 0xe8061) AM_DEVREADWRITE(MC6852_TAG, mc6852_r, mc6852_w)
	AM_RANGE(0xe8080, 0xe808f) AM_DEVREADWRITE(M6522_3_TAG, via_r, via_w)
	AM_RANGE(0xe80a0, 0xe80af) AM_DEVREADWRITE(M6522_4_TAG, via_r, via_w)
	AM_RANGE(0xe80c0, 0xe80cf) AM_DEVREADWRITE(M6522_6_TAG, via_r, via_w)
	AM_RANGE(0xe80e0, 0xe80ef) AM_DEVREADWRITE(M6522_5_TAG, via_r, via_w)
	AM_RANGE(0xf0000, 0xf0fff) AM_MIRROR(0x1000) AM_RAM AM_BASE_MEMBER(victor9k_state, video_ram)
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION(I8088_TAG, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( floppy_io, ADDRESS_SPACE_IO, 8 )
//  AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1)
//  AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2)
//  AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1)
//  AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS)
ADDRESS_MAP_END

//static ADDRESS_MAP_START( keyboard_io, ADDRESS_SPACE_IO, 8 )
//  AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1)
//  AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2)
//  AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1)
//  AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS)
//ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( victor9k )
INPUT_PORTS_END

/* Video */

#define CODE_NON_DISPLAY	0x1000
#define CODE_UNDERLINE		0x2000
#define CODE_LOW_INTENSITY	0x4000
#define CODE_REVERSE_VIDEO	0x8000

static MC6845_UPDATE_ROW( victor9k_update_row )
{
	victor9k_state *state = device->machine->driver_data<victor9k_state>();
	const address_space *program = cputag_get_address_space(device->machine, I8088_TAG, ADDRESS_SPACE_PROGRAM);

	if (BIT(ma, 13))
	{
		fatalerror("Graphics mode not supported!");
	}
	else
	{
		UINT16 video_ram_addr = (ma & 0xfff) << 1;

		for (int sx = 0; sx < x_count; sx++)
		{
			UINT16 code = (state->video_ram[video_ram_addr + 1] << 8) | state->video_ram[video_ram_addr];
			UINT32 char_ram_addr = (BIT(ma, 12) << 16) | ((code & 0xff) << 5) | (ra << 1);
			UINT16 data = memory_read_word_16le(program, char_ram_addr);

			for (int x = 0; x <= 10; x++)
			{
				int color = BIT(data, x);

				if (sx == cursor_x) color = 1;

				*BITMAP_ADDR16(bitmap, y, x + sx*10) = color;
			}

			video_ram_addr += 2;
			video_ram_addr &= 0xfff;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( vsync_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	pic8259_ir7_w(driver_state->pic, state);
	driver_state->vert = state;
}

static const mc6845_interface hd46505s_intf =
{
	SCREEN_TAG,
	10,
	NULL,
	victor9k_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(vsync_w),
	NULL
};

static VIDEO_START( victor9k )
{
	victor9k_state *state = machine->driver_data<victor9k_state>();

	/* find devices */
	state->crt = machine->device(HD46505S_TAG);
}

static VIDEO_UPDATE( victor9k )
{
	victor9k_state *state = screen->machine->driver_data<victor9k_state>();

	mc6845_update(state->crt, bitmap, cliprect);

	return 0;
}

/* Intel 8253 Interface */

static WRITE_LINE_DEVICE_HANDLER( mux_serial_b_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( mux_serial_a_w )
{
}

static const struct pit8253_config pit_intf =
{
	{
		{
			2500000,
			DEVCB_LINE_VCC,
			DEVCB_LINE(mux_serial_b_w)
		}, {
			2500000,
			DEVCB_LINE_VCC,
			DEVCB_LINE(mux_serial_a_w)
		}, {
			100000,
			DEVCB_LINE_VCC,
			DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir2_w)
		}
	}
};

/* Intel 8259 Interfaces */

/*

    pin     signal      description

    IR0     SYN         sync detect
    IR1     COMM        serial communications (7201)
    IR2     TIMER       8253 timer
    IR3     PARALLEL    all 6522 IRQ (including disk)
    IR4     IR4         expansion IR4
    IR5     IR5         expansion IR5
    IR6     KBINT       keyboard data ready
    IR7     VINT        vertical sync or nonspecific interrupt

*/

static const struct pic8259_interface pic_intf =
{
	DEVCB_CPU_INPUT_LINE(I8088_TAG, INPUT_LINE_IRQ0)
};

/* NEC uPD7201 Interface */

static UPD7201_INTERFACE( mpsc_intf )
{
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir1_w), /* interrupt */
	{
		{
			0,					/* receive clock */
			0,					/* transmit clock */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
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
			DEVCB_NULL,			/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output */
		}
	}
};

/* MC6852 Interface */

static WRITE_LINE_DEVICE_HANDLER( ssda_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via6_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static MC6852_INTERFACE( ssda_intf )
{
	0,
	0,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(HC55516_TAG, hc55516_digit_w),
	DEVCB_LINE(ssda_irq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/* M6522 Interface */

static READ8_DEVICE_HANDLER( via1_pa_r )
{
	/*

        bit     description

        PA0     DIO1
        PA1     DIO2
        PA2     DIO3
        PA3     DIO4
        PA4     DIO5
        PA5     DIO6
        PA6     DIO7
        PA7     DIO8

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	return ieee488_dio_r(state->ieee488, 0);
}

static WRITE8_DEVICE_HANDLER( via1_pa_w )
{
	/*

        bit     description

        PA0     DIO1
        PA1     DIO2
        PA2     DIO3
        PA3     DIO4
        PA4     DIO5
        PA5     DIO6
        PA6     DIO7
        PA7     DIO8

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	ieee488_dio_w(state->ieee488, device, data);
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     description

        PB0     DAV
        PB1     EOI
        PB2     REN
        PB3     ATN
        PB4     IFC
        PB5     SRQ
        PB6     NRFD
        PB7     NDAC

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	UINT8 data = 0;

	/* data valid */
	data |= ieee488_dav_r(state->ieee488);

	/* end or identify */
	data |= ieee488_eoi_r(state->ieee488) << 1;

	/* remote enable */
	data |= ieee488_ren_r(state->ieee488) << 2;

	/* attention */
	data |= ieee488_atn_r(state->ieee488) << 3;

	/* interface clear */
	data |= ieee488_ifc_r(state->ieee488) << 4;

	/* service request */
	data |= ieee488_srq_r(state->ieee488) << 5;

	/* not ready for data */
	data |= ieee488_nrfd_r(state->ieee488) << 6;

	/* data not accepted */
	data |= ieee488_ndac_r(state->ieee488) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     description

        PB0     DAV
        PB1     EOI
        PB2     REN
        PB3     ATN
        PB4     IFC
        PB5     SRQ
        PB6     NRFD
        PB7     NDAC

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	/* data valid */
	ieee488_dav_w(state->ieee488, device, BIT(data, 0));

	/* end or identify */
	ieee488_eoi_w(state->ieee488, device, BIT(data, 1));

	/* remote enable */
	ieee488_ren_w(state->ieee488, device, BIT(data, 2));

	/* attention */
	ieee488_atn_w(state->ieee488, device, BIT(data, 3));

	/* interface clear */
	ieee488_ifc_w(state->ieee488, device, BIT(data, 4));

	/* service request */
	ieee488_srq_w(state->ieee488, device, BIT(data, 5));

	/* not ready for data */
	ieee488_nrfd_w(state->ieee488, device, BIT(data, 6));

	/* data not accepted */
	ieee488_ndac_w(state->ieee488, device, BIT(data, 7));
}

static WRITE_LINE_DEVICE_HANDLER( codec_vol_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via1_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via1_intf =
{
	DEVCB_HANDLER(via1_pa_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_DEVICE_LINE(IEEE488_TAG, ieee488_nrfd_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(IEEE488_TAG, ieee488_ndac_r),
	DEVCB_NULL,

	DEVCB_HANDLER(via1_pa_w),
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(codec_vol_w),

	DEVCB_LINE(via1_irq_w)
};

static READ8_DEVICE_HANDLER( via2_pa_r )
{
	/*

        bit     description

        PA0     _INT/EXTA
        PA1     _INT/EXTB
        PA2     RIA
        PA3     DSRA
        PA4     RIB
        PA5     DSRB
        PA6     KBDATA
        PA7     VERT

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	UINT8 data = 0;

	/* vertical sync */
	data |= state->vert << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via2_pa_w )
{
	/*

        bit     description

        PA0     _INT/EXTA
        PA1     _INT/EXTB
        PA2     RIA
        PA3     DSRA
        PA4     RIB
        PA5     DSRB
        PA6     KBDATA
        PA7     VERT

    */
}

static WRITE8_DEVICE_HANDLER( via2_pb_w )
{
	/*

        bit     description

        PB0     TALK/LISTEN
        PB1     KBACKCTL
        PB2     BRT0
        PB3     BRT1
        PB4     BRT2
        PB5     CONT0
        PB6     CONT1
        PB7     CONT2

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	/* brightness */
	state->brt = (data >> 2) & 0x07;

	/* contrast */
	state->cont = data >> 5;
}

static WRITE_LINE_DEVICE_HANDLER( via2_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via2_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via2_intf =
{
	DEVCB_HANDLER(via2_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // KBRDY
	DEVCB_NULL, // SRQ/BUSY
	DEVCB_NULL, // KBDATA

	DEVCB_HANDLER(via2_pa_w),
	DEVCB_HANDLER(via2_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via2_irq_w)
};

static READ8_DEVICE_HANDLER( via3_pa_r )
{
	/*

        bit     description

        PA0     J5-16
        PA1     J5-18
        PA2     J5-20
        PA3     J5-22
        PA4     J5-24
        PA5     J5-26
        PA6     J5-28
        PA7     J5-30

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via3_pa_w )
{
	/*

        bit     description

        PA0     J5-16
        PA1     J5-18
        PA2     J5-20
        PA3     J5-22
        PA4     J5-24
        PA5     J5-26
        PA6     J5-28
        PA7     J5-30

    */
}

static READ8_DEVICE_HANDLER( via3_pb_r )
{
	/*

        bit     description

        PB0     J5-32
        PB1     J5-34
        PB2     J5-36
        PB3     J5-38
        PB4     J5-40
        PB5     J5-42
        PB6     J5-44
        PB7     J5-46

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via3_pb_w )
{
	/*

        bit     description

        PB0     J5-32
        PB1     J5-34
        PB2     J5-36
        PB3     J5-38
        PB4     J5-40
        PB5     J5-42
        PB6     J5-44
        PB7     J5-46

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	/* codec clock output */
	mc6852_rx_clk_w(state->ssda, BIT(data, 7));
	mc6852_tx_clk_w(state->ssda, BIT(data, 7));
}

static WRITE_LINE_DEVICE_HANDLER( via3_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via3_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via3_intf =
{
	DEVCB_HANDLER(via3_pa_r),
	DEVCB_HANDLER(via3_pb_r),
	DEVCB_NULL, // J5-12
	DEVCB_NULL, // J5-48
	DEVCB_NULL, // J5-14
	DEVCB_NULL, // J5-50

	DEVCB_HANDLER(via3_pa_w),
	DEVCB_HANDLER(via3_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via3_irq_w)
};

static WRITE8_DEVICE_HANDLER( via4_pa_w )
{
	/*

        bit     description

        PA0     L0MS0
        PA1     L0MS1
        PA2     L0MS2
        PA3     L0MS3
        PA4     ST0A
        PA5     ST0B
        PA6     ST0C
        PA7     ST0D

    */
}

static WRITE8_DEVICE_HANDLER( via4_pb_w )
{
	/*

        bit     description

        PB0     L1MS0
        PB1     L1MS1
        PB2     L1MS2
        PB3     L1MS3
        PB4     ST1A
        PB5     ST1B
        PB6     ST1C
        PB7     ST1D

    */
}

static WRITE_LINE_DEVICE_HANDLER( mode_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( via4_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via4_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via4_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, // DS0
	DEVCB_NULL, // DS1
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(via4_pa_w),
	DEVCB_HANDLER(via4_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(mode_w),
	DEVCB_NULL,

	DEVCB_LINE(via4_irq_w)
};

static READ8_DEVICE_HANDLER( via5_pa_r )
{
	/*

        bit     description

        PA0     E0
        PA1     E1
        PA2     I1
        PA3     E2
        PA4     E4
        PA5     E5
        PA6     I7
        PA7     E6

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( via5_pb_w )
{
	/*

        bit     description

        PB0     WD0
        PB1     WD1
        PB2     WD2
        PB3     WD3
        PB4     WD4
        PB5     WD5
        PB6     WD6
        PB7     WD7

    */
}

static WRITE_LINE_DEVICE_HANDLER( via5_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via5_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via5_intf =
{
	DEVCB_HANDLER(via5_pa_r),
	DEVCB_NULL,
	DEVCB_NULL, // BRDY
	DEVCB_NULL,
	DEVCB_NULL, // RDY0
	DEVCB_NULL, // RDY1

	DEVCB_NULL,
	DEVCB_HANDLER(via5_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via5_irq_w)
};

static READ8_DEVICE_HANDLER( via6_pa_r )
{
	/*

        bit     description

        PA0     LED0A
        PA1     TRK0D0
        PA2     LED1A
        PA3     TRK0D1
        PA4     SIDE SELECT
        PA5     DRIVE SELECT
        PA6     WPS
        PA7     SYNC

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	UINT8 data = 0;

	/* drive 0 track 0 sense */
	data |= floppy_tk00_r(state->floppy[0].image) << 1;

	/* drive 1 track 0 sense */
	data |= floppy_tk00_r(state->floppy[1].image) << 3;

	/* write protect sense */
	data |= floppy_wpt_r(state->floppy[state->drive].image) << 6;

	return data;
}

static WRITE8_DEVICE_HANDLER( via6_pa_w )
{
	/*

        bit     description

        PA0     LED0A
        PA1     TRK0D0
        PA2     LED1A
        PA3     TRK0D1
        PA4     SIDE SELECT
        PA5     DRIVE SELECT
        PA6     WPS
        PA7     SYNC

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	/* side select */
	state->side = BIT(data, 4);

	/* drive select */
	state->drive = BIT(data, 5);
}

static READ8_DEVICE_HANDLER( via6_pb_r )
{
	/*

        bit     description

        PB0     RDY0
        PB1     RDY1
        PB2     SCRESET
        PB3     _DS1
        PB4     _DS0
        PB5     SINGLE/_DOUBLE SIDED
        PB6     stepper enable A
        PB7     stepper enable B

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	UINT8 data = 0;

	/* drive 0 ready */
	data |= !(floppy_drive_get_flag_state(state->floppy[0].image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 4;

	/* drive 1 ready */
	data |= !(floppy_drive_get_flag_state(state->floppy[0].image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 3;

	/* single/double sided */
	data |= floppy_twosid_r(state->floppy[state->drive].image) << 5;

	return data;
}

static WRITE8_DEVICE_HANDLER( via6_pb_w )
{
	/*

        bit     description

        PB0     RDY0
        PB1     RDY1
        PB2     SCRESET
        PB3     _DS1
        PB4     _DS0
        PB5     SINGLE/_DOUBLE SIDED
        PB6     stepper enable A
        PB7     stepper enable B

    */

	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	/* motor speed controller reset */
	cputag_set_input_line(device->machine, I8048_TAG, INPUT_LINE_RESET, BIT(data, 2));

	/* stepper enable A */
	state->floppy[0].se = BIT(data, 6);

	/* stepper enable B */
	state->floppy[1].se = BIT(data, 7);
}

static WRITE_LINE_DEVICE_HANDLER( drw_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( erase_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( via6_irq_w )
{
	victor9k_state *driver_state = device->machine->driver_data<victor9k_state>();

	driver_state->via6_irq = state;

	pic8259_ir3_w(driver_state->pic, driver_state->ssda_irq | driver_state->via1_irq | driver_state->via2_irq | driver_state->via3_irq | driver_state->via4_irq | driver_state->via5_irq | driver_state->via6_irq);
}

static const via6522_interface via6_intf =
{
	DEVCB_HANDLER(via6_pa_r),
	DEVCB_HANDLER(via6_pb_r),
	DEVCB_NULL, // GCRERR
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(via6_pa_w),
	DEVCB_HANDLER(via6_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(drw_w),
	DEVCB_LINE(erase_w),

	DEVCB_LINE(via6_irq_w)
};

/* Floppy Configuration */

static const floppy_config victor9k_floppy_config =
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

/* IEEE-488 Interface */

static IEEE488_DAISY( ieee488_daisy )
{
	{ M6522_1_TAG, DEVCB_NULL, DEVCB_NULL, DEVCB_DEVICE_LINE(M6522_1_TAG, via_ca1_w), DEVCB_DEVICE_LINE(M6522_1_TAG, via_ca2_w) },
	{ NULL }
};

/* Machine Initialization */

static IRQ_CALLBACK( victor9k_irq_callback )
{
	victor9k_state *state = device->machine->driver_data<victor9k_state>();

	return pic8259_acknowledge(state->pic);
}

static MACHINE_START( victor9k )
{
	victor9k_state *state = machine->driver_data<victor9k_state>();

	/* set interrupt callback */
	cpu_set_irq_callback(machine->firstcpu, victor9k_irq_callback);

	/* find devices */
	state->ieee488 = machine->device(IEEE488_TAG);
	state->pic = machine->device(I8259A_TAG);
	state->ssda = machine->device(MC6852_TAG);
	state->cvsd = machine->device(HC55516_TAG);
	state->floppy[0].image = machine->device(FLOPPY_0);
	state->floppy[1].image = machine->device(FLOPPY_1);

	/* memory banking */
	const address_space *program = cputag_get_address_space(machine, I8088_TAG, ADDRESS_SPACE_PROGRAM);
	UINT8 *ram = messram_get_ptr(machine->device("messram"));
	int ram_size = messram_get_size(machine->device("messram"));

	memory_install_ram(program, 0x00000, ram_size - 1, 0, 0, ram);
}

/* Machine Driver */

static MACHINE_DRIVER_START( victor9k )
	MDRV_DRIVER_DATA(victor9k_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(I8088_TAG, I8088, XTAL_30MHz/6)
	MDRV_CPU_PROGRAM_MAP(victor9k_mem)

	MDRV_CPU_ADD(I8048_TAG, I8048, XTAL_30MHz/6)
	MDRV_CPU_IO_MAP(floppy_io)

//  MDRV_CPU_ADD(I8022_TAG, I8022, 100000)
//  MDRV_CPU_IO_MAP(keyboard_io)

	MDRV_MACHINE_START(victor9k)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(monochrome_green)

	MDRV_VIDEO_START(victor9k)
	MDRV_VIDEO_UPDATE(victor9k)

	MDRV_MC6845_ADD(HD46505S_TAG, H46505, 1000000, hd46505s_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(HC55516_TAG, HC55516, 100000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_IEEE488_ADD(IEEE488_TAG, ieee488_daisy)
	MDRV_PIC8259_ADD(I8259A_TAG, pic_intf)
	MDRV_PIT8253_ADD(I8253_TAG, pit_intf)
	MDRV_UPD7201_ADD(UPD7201_TAG, XTAL_30MHz/30, mpsc_intf)
	MDRV_MC6852_ADD(MC6852_TAG, XTAL_30MHz/30, ssda_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, XTAL_30MHz/30, via1_intf)
	MDRV_VIA6522_ADD(M6522_2_TAG, XTAL_30MHz/30, via2_intf)
	MDRV_VIA6522_ADD(M6522_3_TAG, XTAL_30MHz/30, via3_intf)
	MDRV_VIA6522_ADD(M6522_4_TAG, XTAL_30MHz/30, via4_intf)
	MDRV_VIA6522_ADD(M6522_5_TAG, XTAL_30MHz/30, via5_intf)
	MDRV_VIA6522_ADD(M6522_6_TAG, XTAL_30MHz/30, via6_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(victor9k_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("256K,384K,512K,640K,768K,896K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( victor9k )
	ROM_REGION( 0x2000, I8088_TAG, 0 )
	ROM_DEFAULT_BIOS( "univ" )
	ROM_SYSTEM_BIOS( 0, "old", "Older" )
	ROMX_LOAD( "102320.7j", 0x0000, 0x1000, CRC(3d615fd7) SHA1(b22f7e5d66404185395d8effbf57efded0079a92), ROM_BIOS(1) )
	ROMX_LOAD( "102322.8j", 0x1000, 0x1000, CRC(9209df0e) SHA1(3ee8e0c15186bbd5768b550ecc1fa3b6b1dbb928), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "univ", "Universal" )
	ROMX_LOAD( "v9000 univ. fe f3f7 13db.7j", 0x0000, 0x1000, CRC(25c7a59f) SHA1(8784e9aa7eb9439f81e18b8e223c94714e033911), ROM_BIOS(2) )
	ROMX_LOAD( "v9000 univ. ff f3f7 39fe.8j", 0x1000, 0x1000, CRC(496c7467) SHA1(eccf428f62ef94ab85f4a43ba59ae6a066244a66), ROM_BIOS(2) )

	ROM_REGION( 0x800, "gcr", 0 )
	ROM_LOAD( "100836-001.4k", 0x000, 0x800, CRC(adc601bd) SHA1(6eeff3d2063ae2d97452101aa61e27ef83a467e5) )

	ROM_REGION( 0x400, I8048_TAG, 0)
	ROM_LOAD( "36080.5d", 0x000, 0x400, CRC(9bf49f7d) SHA1(b3a11bb65105db66ae1985b6f482aab6ea1da38b) )

	ROM_REGION( 0x800, I8022_TAG, 0)
	ROM_LOAD( "keyboard controller i8022", 0x000, 0x800, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY                     FULLNAME        FLAGS */
COMP( 1982, victor9k, 0,      0,      victor9k, victor9k, 0,    "Victor Business Products", "Victor 9000",	GAME_NOT_WORKING )
