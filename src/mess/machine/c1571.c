/**********************************************************************

    Commodore 1570/1571/1571CR Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - fast serial only works with PAL C128
    - 1541/1571 Alignment shows drive speed as 266 rpm, should be 310
    - CP/M disks
    - power/activity LEDs

*/

#include "emu.h"
#include "c1571.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6502_TAG		"u1"
#define M6522_0_TAG		"u9"
#define M6522_1_TAG		"u4"
#define M6526_TAG		"u20"
#define WD1770_TAG		"u11"
#define C64H156_TAG		"u6"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1571_t c1571_t;
struct _c1571_t
{
	/* IEC bus */
	int address;							/* device number */
	int data_out;							/* serial data out */
	int atn_ack;							/* attention acknowledge */
	int ser_dir;							/* fast serial direction */
	int sp_out;								/* fast serial data out */
	int cnt_out;							/* fast serial clock out */

	/* motors */
	int stp;								/* stepper motor phase */
	int mtr;								/* spindle motor on */

	/* track */
	UINT8 track_buffer[G64_BUFFER_SIZE];	/* track data buffer */
	int track_len;							/* track length */
	int buffer_pos;							/* current byte position within track buffer */
	int bit_pos;							/* current bit position within track buffer byte */
	int bit_count;							/* current data byte bit counter */
	UINT16 data;							/* data shift register */
	UINT8 yb;								/* GCR data byte to write */

	/* signals */
	int ds;									/* density select */
	int soe;								/* s? output enable */
	int byte;								/* byte ready */
	int mode;								/* mode (0 = write, 1 = read) */
	int side;								/* disk side select */
	int clock;								/* clock speed */

	/* interrupts */
	int via0_irq;							/* VIA #0 interrupt request */
	int via1_irq;							/* VIA #1 interrupt request */
	int cia_irq;							/* CIA interrupt request */

	/* devices */
	device_t *cpu;
	via6522_device *via0;
	via6522_device *via1;
	device_t *cia;
	device_t *wd1770;
	device_t *serial_bus;
	device_t *image;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1571_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1570) || (device->type() == C1571) || (device->type() == C1571CR));
	return (c1571_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c1571_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1570) || (device->type() == C1571) || (device->type() == C1571CR));
	return (c1571_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE void iec_data_w(device_t *device)
{
	c1571_t *c1571 = get_safe_token(device);

	int atn = cbm_iec_atn_r(c1571->serial_bus);
	int data = !c1571->data_out & !(c1571->atn_ack ^ !atn);

	/* fast serial data */
	if (c1571->ser_dir) data &= c1571->sp_out;

	cbm_iec_data_w(c1571->serial_bus, device, data);
}

INLINE void iec_srq_w(device_t *device)
{
	c1571_t *c1571 = get_safe_token(device);

	int srq = 1;

	/* fast serial clock */
	if (c1571->ser_dir) srq &= c1571->cnt_out;

	cbm_iec_srq_w(c1571->serial_bus, device, srq);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( bit_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( bit_tick )
{
	device_t *device = (device_t *)ptr;
	c1571_t *c1571 = get_safe_token(device);
	int byte = 0;

	/* shift in data from the read head */
	c1571->data <<= 1;
	c1571->data |= BIT(c1571->track_buffer[c1571->buffer_pos], c1571->bit_pos);
	c1571->bit_pos--;
	c1571->bit_count++;

	if (c1571->bit_pos < 0)
	{
		c1571->bit_pos = 7;
		c1571->buffer_pos++;

		if (c1571->buffer_pos >= c1571->track_len)
		{
			/* loop to the start of the track */
			c1571->buffer_pos = 0;
		}
	}

	if ((c1571->data & G64_SYNC_MARK) == G64_SYNC_MARK)
	{
		/* SYNC detected */
		c1571->bit_count = 0;
	}

	if (c1571->bit_count > 7)
	{
		/* byte ready */
		c1571->bit_count = 0;
		byte = 1;

		c1571->yb = c1571->data & 0xff;

		if (!c1571->yb)
		{
			/* simulate weak bits with randomness */
			c1571->yb = machine.rand() & 0xff;
		}
	}

	if (c1571->byte != byte)
	{
		int byte_ready = !(byte && c1571->soe);

		device_set_input_line(c1571->cpu, M6502_SET_OVERFLOW, byte_ready);
		c1571->via1->write_ca1(byte_ready);

		c1571->byte = byte;
	}
}

/*-------------------------------------------------
    read_current_track - read track from disk
-------------------------------------------------*/

static void read_current_track(c1571_t *c1571)
{
	c1571->track_len = G64_BUFFER_SIZE;
	c1571->buffer_pos = 0;
	c1571->bit_pos = 7;
	c1571->bit_count = 0;

	/* read track data */
	floppy_drive_read_track_data_info_buffer(c1571->image, c1571->side, c1571->track_buffer, &c1571->track_len);

	/* extract track length */
	c1571->track_len = floppy_drive_get_current_track_size(c1571->image, c1571->side);
}

/*-------------------------------------------------
    on_disk_change - disk change handler
-------------------------------------------------*/

static void on_disk_change(device_image_interface &image)
{
	c1571_t *c1571 = get_safe_token(image.device().owner());

	read_current_track(c1571);
}

/*-------------------------------------------------
    set_side - set disk side
-------------------------------------------------*/

static void set_side(c1571_t *c1571, int side)
{
	if (c1571->side != side)
	{
		c1571->side = side;
		wd17xx_set_side(c1571->wd1770, side);

		if (c1571->mtr)
		{
			/* read track data */
			read_current_track(c1571);
		}
	}
}

/*-------------------------------------------------
    spindle_motor - spindle motor control
-------------------------------------------------*/

static void spindle_motor(c1571_t *c1571, int mtr)
{
	if (c1571->mtr != mtr)
	{
		if (mtr)
		{
			/* read track data */
			read_current_track(c1571);
		}

		floppy_mon_w(c1571->image, !mtr);
		c1571->bit_timer->enable(mtr);

		c1571->mtr = mtr;
	}
}

/*-------------------------------------------------
    step_motor - stepper motor control
-------------------------------------------------*/

static void step_motor(c1571_t *c1571, int stp)
{
	if (c1571->mtr & (c1571->stp != stp))
	{
		int tracks = 0;

		switch (c1571->stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 3) tracks--; break;
		case 1:	if (stp == 2) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 3) tracks++; else if (stp == 1) tracks--; break;
		case 3: if (stp == 0) tracks++; else if (stp == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			/* step read/write head */
			floppy_drive_seek(c1571->image, tracks);

			/* read new track data */
			read_current_track(c1571);
		}

		c1571->stp = stp;
	}
}

/*-------------------------------------------------
    c1571_iec_atn_w - serial bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_atn_w )
{
	c1571_t *c1571 = get_safe_token(device);

	c1571->via0->write_ca1(!state);
	iec_data_w(device);
}

/*-------------------------------------------------
    c1571_iec_srq_w - serial bus fast clock
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_srq_w )
{
	c1571_t *c1571 = get_safe_token(device);

	if (!c1571->ser_dir)
	{
		mos6526_cnt_w(c1571->cia, state);
	}
}

/*-------------------------------------------------
    c1571_iec_data_w - serial bus fast data
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_data_w )
{
	c1571_t *c1571 = get_safe_token(device);

	if (!c1571->ser_dir)
	{
		mos6526_sp_w(c1571->cia, state);
	}
}

/*-------------------------------------------------
    c1571_iec_reset_w - serial bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_reset_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1570_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1570_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x2000, 0x2003) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x4000, 0x400f) AM_DEVREADWRITE(M6526_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1570:c1570", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1571_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1571_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x03f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x03f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x2000, 0x2003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x3ff0) AM_DEVREADWRITE(M6526_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1571:c1571", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1571cr_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1571cr_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
//  AM_RANGE(0x2000, 0x2005) 5710 FDC
//  AM_RANGE(0x400c, 0x400e) 5710 CIA
//  AM_RANGE(0x4010, 0x4017) 5710 CIA
//  AM_RANGE(0x6000, 0x7fff) RAM mirrors
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1571:c1571cr", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    via6522_interface via0_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via0_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->via0_irq = state;

	device_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( via0_pa_r )
{
	/*

        bit     description

        PA0     TRK0 SNS
        PA1     SER DIR
        PA2     SIDE
        PA3
        PA4
        PA5     _1/2 MHZ
        PA6     ATN OUT
        PA7     BYTE RDY

    */

	c1571_t *c1571 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* track 0 sense */
	data |= floppy_tk00_r(c1571->image);

	/* byte ready */
	data |= !(c1571->byte && c1571->soe) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pa_w )
{
	/*

        bit     description

        PA0     TRK0 SNS
        PA1     SER DIR
        PA2     SIDE
        PA3
        PA4
        PA5     _1/2 MHZ
        PA6     ATN OUT
        PA7     BYTE RDY

    */

	c1571_t *c1571 = get_safe_token(device->owner());

	/* 1/2 MHz */
	int clock_1_2 = BIT(data, 5);

	if (c1571->clock != clock_1_2)
	{
		UINT32 clock = clock_1_2 ? XTAL_16MHz/8 : XTAL_16MHz/16;

		c1571->cpu->set_unscaled_clock(clock);
		c1571->cia->set_unscaled_clock(clock);
		c1571->via0->set_unscaled_clock(clock);
		c1571->via1->set_unscaled_clock(clock);

		c1571->clock = clock_1_2;
	}

	/* fast serial direction */
	c1571->ser_dir = BIT(data, 1);
	iec_data_w(device->owner());
	iec_srq_w(device->owner());

	/* side select */
	int side = BIT(data, 2);
	set_side(c1571, side);

	/* attention out */
	cbm_iec_atn_w(c1571->serial_bus, device->owner(), !BIT(data, 6));
}

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     DEV# SEL
        PB6     DEV# SEL
        PB7     ATN IN

    */

	c1571_t *c1571 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* data in */
	data = !cbm_iec_data_r(c1571->serial_bus);

	/* clock in */
	data |= !cbm_iec_clk_r(c1571->serial_bus) << 2;

	/* serial bus address */
	data |= c1571->address << 5;

	/* attention in */
	data |= !cbm_iec_atn_r(c1571->serial_bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     DEV# SEL
        PB6     DEV# SEL
        PB7     ATN IN

    */

	c1571_t *c1571 = get_safe_token(device->owner());

	/* attention acknowledge */
	c1571->atn_ack = BIT(data, 4);

	/* data out */
	c1571->data_out = BIT(data, 1);
	iec_data_w(device->owner());

	/* clock out */
	cbm_iec_clk_w(c1571->serial_bus, device->owner(), !BIT(data, 3));
}

static READ_LINE_DEVICE_HANDLER( atn_in_r )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	return !cbm_iec_atn_r(c1571->serial_bus);
}

static READ_LINE_DEVICE_HANDLER( wprt_r )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	return !floppy_wpt_r(c1571->image);
}

static const via6522_interface via0_intf =
{
	DEVCB_HANDLER(via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_LINE(atn_in_r),
	DEVCB_NULL,
	DEVCB_LINE(wprt_r),
	DEVCB_NULL,

	DEVCB_HANDLER(via0_pa_w),
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface via1_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->via1_irq = state;

	device_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( yb_r )
{
	/*

        bit     description

        PA0     YB0
        PA1     YB1
        PA2     YB2
        PA3     YB3
        PA4     YB4
        PA5     YB5
        PA6     YB6
        PA7     YB7

    */

	c1571_t *c1571 = get_safe_token(device->owner());

	return c1571->yb;
}

static WRITE8_DEVICE_HANDLER( yb_w )
{
	/*

        bit     description

        PA0     YB0
        PA1     YB1
        PA2     YB2
        PA3     YB3
        PA4     YB4
        PA5     YB5
        PA6     YB6
        PA7     YB7

    */

	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->yb = data;
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     signal      description

        PB0     STP0        stepping motor bit 0
        PB1     STP1        stepping motor bit 1
        PB2     MTR         motor ON/OFF
        PB3     ACT         drive 0 LED
        PB4     _WPRT       write protect sense
        PB5     DS0         density select 0
        PB6     DS1         density select 1
        PB7     _SYNC       SYNC detect line

    */

	c1571_t *c1571 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* write protect sense */
	data |= !floppy_wpt_r(c1571->image) << 4;

	/* SYNC detect line */
	data |= !((c1571->data & G64_SYNC_MARK) == G64_SYNC_MARK) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

        bit     signal      description

        PB0     STP0        stepping motor bit 0
        PB1     STP1        stepping motor bit 1
        PB2     MTR         motor ON/OFF
        PB3     ACT         drive 0 LED
        PB4     _WPRT       write protect sense
        PB5     DS0         density select 0
        PB6     DS1         density select 1
        PB7     _SYNC       SYNC detect line

    */

	c1571_t *c1571 = get_safe_token(device->owner());

	/* spindle motor */
	int mtr = BIT(data, 2);
	spindle_motor(c1571, mtr);

	/* stepper motor */
	int stp = data & 0x03;
	step_motor(c1571, stp);

	/* TODO activity LED */

	/* density select */
	int ds = (data >> 5) & 0x03;

	if (c1571->ds != ds)
	{
		c1571->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C2040_BITRATE[ds]/4));
		c1571->ds = ds;
	}
}

static READ_LINE_DEVICE_HANDLER( byte_ready_r )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	return !(c1571->byte && c1571->soe);
}

static WRITE_LINE_DEVICE_HANDLER( soe_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());
	int byte_ready = !(state && c1571->byte);

	c1571->soe = state;

	device_set_input_line(c1571->cpu, M6502_SET_OVERFLOW, byte_ready);
	c1571->via1->write_ca1(byte_ready);
}

static WRITE_LINE_DEVICE_HANDLER( mode_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->mode = state;
}

static const via6522_interface via1_intf =
{
	DEVCB_HANDLER(yb_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_LINE(byte_ready_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(yb_w),
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(soe_w),
	DEVCB_LINE(mode_w),

	DEVCB_LINE(via1_irq_w)
};

/*-------------------------------------------------
    mos6526_interface cia_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( cia_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->cia_irq = state;

	device_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( cia_cnt_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	/* fast serial clock out */
	c1571->cnt_out = state;
	iec_srq_w(device->owner());
}

static WRITE_LINE_DEVICE_HANDLER( cia_sp_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	/* fast serial data out */
	c1571->sp_out = state;
	iec_data_w(device->owner());
}

static MOS6526_INTERFACE( cia_intf )
{
	0,
	DEVCB_LINE(cia_irq_w),
	DEVCB_NULL,
	DEVCB_LINE(cia_cnt_w),
	DEVCB_LINE(cia_sp_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

/*-------------------------------------------------
    wd17xx_interface wd1770_intf
-------------------------------------------------*/

static const wd17xx_interface wd1770_intf =
{
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1571 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1571 )
	FLOPPY_OPTION( c1571, "g64", "Commodore 1541 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
	FLOPPY_OPTION( c1571, "d64", "Commodore 1541 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c1571, "d71", "Commodore 1571 Disk Image", d71_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1570_floppy_config
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( wpt_w )
{
	c1571_t *c1571 = get_safe_token(device->owner());

	c1571->via0->write_ca2(!state);
}

static const floppy_config c1570_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(wpt_w),
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c1571),
	NULL
};

/*-------------------------------------------------
    floppy_config c1571_floppy_config
-------------------------------------------------*/

static const floppy_config c1571_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(wpt_w),
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(c1571),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1570 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1570 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1570_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, via1_intf)
	MCFG_MOS6526R1_ADD(M6526_TAG, XTAL_16MHz/16, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /* XTAL_16MHz/2, */ wd1770_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1570_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1571 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1571 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1571_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, via1_intf)
	MCFG_MOS6526R1_ADD(M6526_TAG, XTAL_16MHz/16, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /* XTAL_16MHz/2, */ wd1770_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1571_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1571cr )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1571cr )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1571cr_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, via1_intf)
	MCFG_MOS6526R1_ADD(M6526_TAG, XTAL_16MHz/16, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /* XTAL_16MHz/2, */ wd1770_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1571_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c1570 )
-------------------------------------------------*/

ROM_START( c1570 )
	ROM_REGION( 0x8000, "c1570", ROMREGION_LOADBYNAME )
	ROM_LOAD( "315090-01.u2", 0x0000, 0x8000, CRC(5a0c7937) SHA1(5fc06dc82ff6840f183bd43a4d9b8a16956b2f56) )
ROM_END

/*-------------------------------------------------
    ROM( c1571 )
-------------------------------------------------*/

ROM_START( c1571 )
	ROM_REGION( 0x10000, "c1571", ROMREGION_LOADBYNAME )
	ROM_LOAD_OPTIONAL( "310654-03.u2", 0x0000, 0x8000, CRC(3889b8b8) SHA1(e649ef4419d65829d2afd65e07d31f3ce147d6eb) )
	ROM_LOAD( "310654-05.u2", 0x0000, 0x8000, CRC(5755bae3) SHA1(f1be619c106641a685f6609e4d43d6fc9eac1e70) )
	ROM_LOAD_OPTIONAL( "jiffydos 1571.u2", 0x8000, 0x8000, CRC(fe6cac6d) SHA1(d4b79b60cf1eaa399d0932200eb7811e00455249) )
ROM_END

/*-------------------------------------------------
    ROM( c1571cr )
-------------------------------------------------*/

ROM_START( c1571cr )
	ROM_REGION( 0x10000, "c1571cr", ROMREGION_LOADBYNAME )
	ROM_LOAD( "318047-01.u102", 0x0000, 0x8000, CRC(f24efcc4) SHA1(14ee7a0fb7e1c59c51fbf781f944387037daa3ee) )
	ROM_LOAD_OPTIONAL( "jiffydos 1571d.u102", 0x8000, 0x8000, CRC(9cba146d) SHA1(823b178561302b403e6bfd8dd741d757efef3958) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1571 )
-------------------------------------------------*/

static DEVICE_START( c1571 )
{
	c1571_t *c1571 = get_safe_token(device);
	const c1571_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 12));
	c1571->address = config->address - 8;

	/* find our CPU */
	c1571->cpu = device->subdevice(M6502_TAG);

	/* find devices */
	c1571->via0 = device->subdevice<via6522_device>(M6522_0_TAG);
	c1571->via1 = device->subdevice<via6522_device>(M6522_1_TAG);
	c1571->cia = device->subdevice(M6526_TAG);
	c1571->wd1770 = device->subdevice(WD1770_TAG);
	c1571->serial_bus = device->machine().device(config->serial_bus_tag);
	c1571->image = device->subdevice(FLOPPY_0);

	/* install image callbacks */
	floppy_install_unload_proc(c1571->image, on_disk_change);
	floppy_install_load_proc(c1571->image, on_disk_change);

	/* allocate data timer */
	c1571->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_tick), (void *)device);

	/* register for state saving */
	device->save_item(NAME(c1571->address));
	device->save_item(NAME(c1571->data_out));
	device->save_item(NAME(c1571->atn_ack));
	device->save_item(NAME(c1571->ser_dir));
	device->save_item(NAME(c1571->sp_out));
	device->save_item(NAME(c1571->cnt_out));
	device->save_item(NAME(c1571->stp));
	device->save_item(NAME(c1571->mtr));
	device->save_item(NAME(c1571->track_len));
	device->save_item(NAME(c1571->buffer_pos));
	device->save_item(NAME(c1571->bit_pos));
	device->save_item(NAME(c1571->bit_count));
	device->save_item(NAME(c1571->data));
	device->save_item(NAME(c1571->yb));
	device->save_item(NAME(c1571->ds));
	device->save_item(NAME(c1571->soe));
	device->save_item(NAME(c1571->byte));
	device->save_item(NAME(c1571->mode));
	device->save_item(NAME(c1571->side));
	device->save_item(NAME(c1571->via0_irq));
	device->save_item(NAME(c1571->via1_irq));
	device->save_item(NAME(c1571->cia_irq));
}

/*-------------------------------------------------
    DEVICE_RESET( c1571 )
-------------------------------------------------*/

static DEVICE_RESET( c1571 )
{
	c1571_t *c1571 = get_safe_token(device);

	c1571->cpu->reset();
	c1571->via0->reset();
	c1571->via1->reset();
	c1571->cia->reset();
	c1571->wd1770->reset();

	c1571->sp_out = 1;
	c1571->cnt_out = 1;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1570 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1570 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1571_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1571_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1570);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1570);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1571);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1571);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1570");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1570");									break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 1571");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1571 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1571 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1571);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1571);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1571");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1571");									break;
		
		default:										DEVICE_GET_INFO_CALL(c1570);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1571cr )
-------------------------------------------------*/

DEVICE_GET_INFO( c1571cr )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1571cr);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1571cr);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1571CR");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c1571cr");									break;
		
		default:										DEVICE_GET_INFO_CALL(c1570);								break;
	}
}

DEFINE_LEGACY_DEVICE(C1570,  c1570);
DEFINE_LEGACY_DEVICE(C1571, c1571);
DEFINE_LEGACY_DEVICE(C1571CR, c1571cr);
