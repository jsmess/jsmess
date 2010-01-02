/**********************************************************************

    Commodore 1570/1571/1571CR Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "c1571.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_TAG		"u1"
#define M6522_0_TAG		"u4"
#define M6522_1_TAG		"u9"
#define M6526_TAG		"u20"
#define WD1770_TAG		"u11"

static const double C1571_BITRATE[4] =
{
	XTAL_16MHz/13.0,	/* tracks 1-17 */
	XTAL_16MHz/14.0,	/* tracks 18-24 */
	XTAL_16MHz/15.0, 	/* tracks 25-30 */
	XTAL_16MHz/16.0		/* tracks 31-42 */
};

#define SYNC_MARK			0x3ff		/* 10 consecutive 1-bits */

#define TRACK_BUFFER_SIZE	8194		/* 2 bytes track length + maximum of 8192 bytes of GCR encoded data */
#define TRACK_DATA_START	2

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1571_t c1571_t;
struct _c1571_t
{
	/* abstractions */
	int address;
	UINT8 track_buffer[TRACK_BUFFER_SIZE];				/* track data buffer */
	int track_len;						/* track length */
	int buffer_pos;						/* current byte position within track buffer */
	int bit_pos;						/* current bit position within track buffer byte */
	int bit_count;						/* current data byte bit counter */
	UINT16 data;						/* data shift register */

	/* signals */
	UINT8 yb;							/* GCR data byte to write */
	int byte;							/* byte ready */
	int atna;							/* attention acknowledge */
	int ds;								/* density select */
	int stp;							/* stepping motor */
	int soe;							/* s? output enable */
	int mode;							/* mode (0 = write, 1 = read) */

	int data_out;
	int atn_ack;

	/* interrupts */
	int via0_irq;						/* VIA #0 interrupt request */
	int via1_irq;						/* VIA #1 interrupt request */
	int cia_irq;						/* CIA interrupt request */

	/* devices */
	const device_config *cpu;
	const device_config *via0;
	const device_config *via1;
	const device_config *cia;
	const device_config *wd1770;
	const device_config *serial_bus;
	const device_config *image;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1571_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == C1570) || (device->type == C1571) || (device->type == C1571CR));
	return (c1571_t *)device->token;
}

INLINE c1571_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == C1570) || (device->type == C1571) || (device->type == C1571CR));
	return (c1571_config *)device->inline_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( bit_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( bit_tick )
{
	const device_config *device = (device_config *)ptr;
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

		if (c1571->buffer_pos > c1571->track_len + 1)
		{
			/* loop to the start of the track */
			c1571->buffer_pos = TRACK_DATA_START;
		}
	}

	if ((c1571->data & SYNC_MARK) == SYNC_MARK)
	{
		/* SYNC detected */
		c1571->bit_count = 0;
	}

	if (c1571->bit_count > 7)
	{
		/* byte ready */
		c1571->bit_count = 0;
		byte = 1;

		if (!(c1571->data & 0xff))
		{
			/* simulate weak bits with randomness */
			c1571->data = (c1571->data & 0xff00) | (mame_rand(machine) & 0xff);
		}
	}

	if (c1571->byte != byte)
	{
		int byte_ready = !(byte && c1571->soe);

		cpu_set_input_line(c1571->cpu, M6502_SET_OVERFLOW, byte_ready);
		via_ca1_w(c1571->via1, 0, byte_ready);

		c1571->byte = byte;
	}
}

/*-------------------------------------------------
    c1571_iec_atn_w - serial bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_atn_w )
{
	c1571_t *c1571 = get_safe_token(device);
	int data_out = !c1571->data_out && !(c1571->atn_ack ^ !state);

	via_ca1_w(c1571->via0, 0, !state);

	cbm_iec_data_w(c1571->serial_bus, device, data_out);
}

/*-------------------------------------------------
    c1571_iec_reset_w - serial bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1571_iec_reset_w )
{
	if (!state)
	{
		device_reset(device);
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1570_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1570_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE(M6522_0_TAG, via_r, via_w)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0x2000, 0x2003) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x4000, 0x400f) AM_DEVREADWRITE(M6526_TAG, cia_r, cia_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1570", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1571_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1571_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE(M6522_0_TAG, via_r, via_w)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0x2000, 0x2003) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x4000, 0x400f) AM_DEVREADWRITE(M6526_TAG, cia_r, cia_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1571", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1571cr_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1571cr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE(M6522_0_TAG, via_r, via_w)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
//  AM_RANGE(0x2000, 0x2005) 5710 FDC
//  AM_RANGE(0x400c, 0x400e) 5710 CIA
//  AM_RANGE(0x4010, 0x4017) 5710 CIA
//  AM_RANGE(0x6000, 0x7fff) RAM mirrors
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1571cr", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    via6522_interface c1571_via0_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via0_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner);

	c1571->via0_irq = state;

	cpu_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
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

	c1571_t *c1571 = get_safe_token(device->owner);

	return floppy_tk00_r(c1571->image);
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

	c1571_t *c1571 = get_safe_token(device->owner);

	/* 1/2 MHz */
	UINT32 clock = BIT(data, 5) ? XTAL_16MHz/8 : XTAL_16MHz/16;

	device_set_clock(c1571->cpu, clock);
	device_set_clock(c1571->cia, clock);
	device_set_clock(c1571->via0, clock);
	device_set_clock(c1571->via1, clock);

	/* side */
	wd17xx_set_side(c1571->wd1770, BIT(data, 2));
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

	c1571_t *c1571 = get_safe_token(device->owner);
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

	c1571_t *c1571 = get_safe_token(device->owner);

	int data_out = BIT(data, 1);
	int clk_out = BIT(data, 3);
	int atn_ack = BIT(data, 4);

	/* data out */
	int serial_data = !data_out && !(atn_ack ^ !cbm_iec_atn_r(c1571->serial_bus));
	cbm_iec_data_w(c1571->serial_bus, device->owner, serial_data);
	c1571->data_out = data_out;

	/* clock out */
	cbm_iec_clk_w(c1571->serial_bus, device->owner, !clk_out);

	/* attention acknowledge */
	c1571->atn_ack = atn_ack;
}

static const via6522_interface c1571_via0_intf =
{
	DEVCB_HANDLER(via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(via0_pa_w),
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL, /* ATN IN */
	DEVCB_NULL,
	DEVCB_NULL, /* _WPRT */
	DEVCB_NULL,

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface c1571_via1_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner);

	c1571->via1_irq = state;

	cpu_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
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

	return 0;
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

	c1571_t *c1571 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* write protect sense */
	data |= !floppy_wpt_r(c1571->image) << 4;

	/* SYNC detect line */

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

	c1571_t *c1571 = get_safe_token(device->owner);
	int stp = data & 0x03;
	int ds = (data >> 5) & 0x03;
	int mtr = BIT(data, 2);

	/* stepper motor */
	if (c1571->stp != stp)
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
			c1571->track_len = TRACK_BUFFER_SIZE;
			c1571->buffer_pos = TRACK_DATA_START;
			c1571->bit_pos = 7;
			c1571->bit_count = 0;

			/* step read/write head */
			floppy_drive_seek(c1571->image, tracks);

			/* read track data */
			floppy_drive_read_track_data_info_buffer(c1571->image, 0, c1571->track_buffer, &c1571->track_len);

			/* extract track length */
			c1571->track_len = (c1571->track_buffer[1] << 8) | c1571->track_buffer[0];
		}

		c1571->stp = stp;
	}

	/* spindle motor */
	floppy_mon_w(c1571->image, !mtr);
	timer_enable(c1571->bit_timer, mtr);

	/* activity LED */

	/* density select */
	if (c1571->ds != ds)
	{
		timer_adjust_periodic(c1571->bit_timer, attotime_zero, 0, ATTOTIME_IN_HZ(C1571_BITRATE[ds]/4));
		c1571->ds = ds;
	}
}

static WRITE_LINE_DEVICE_HANDLER( soe_w )
{
	c1571_t *c1571 = get_safe_token(device->owner);
	int byte_ready = !(state && c1571->byte);

	c1571->soe = state;

	cpu_set_input_line(c1571->cpu, M6502_SET_OVERFLOW, byte_ready);
	via_ca1_w(device, 0, byte_ready);
}

static WRITE_LINE_DEVICE_HANDLER( mode_w )
{
	c1571_t *c1571 = get_safe_token(device->owner);

	c1571->mode = state;
}

static const via6522_interface c1571_via1_intf =
{
	DEVCB_HANDLER(yb_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_NULL, /* BYTE READY */
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
    cia6526_interface c1571_cia_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( cia_irq_w )
{
	c1571_t *c1571 = get_safe_token(device->owner);

	c1571->cia_irq = state;

	cpu_set_input_line(c1571->cpu, INPUT_LINE_IRQ0, (c1571->via0_irq | c1571->via1_irq | c1571->cia_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static const cia6526_interface c1571_cia_intf =
{
	DEVCB_LINE(cia_irq_w),
	DEVCB_NULL,
	10, /* 1/10 second */
	{
		{ DEVCB_NULL, DEVCB_NULL },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

/*-------------------------------------------------
    wd17xx_interface c1571_wd1770_intf
-------------------------------------------------*/

static const wd17xx_interface c1571_wd1770_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1571 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1571 )
	FLOPPY_OPTION( c1571, "g64", "Commodore 1571 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
//  FLOPPY_OPTION( c1571, "d64", "Commodore 1571 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
//  FLOPPY_OPTION( c1571, "d71", "Commodore 1571 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1570_floppy_config
-------------------------------------------------*/

static const floppy_config c1570_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c1571),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    floppy_config c1571_floppy_config
-------------------------------------------------*/

static const floppy_config c1571_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(c1571),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1570 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1570 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(c1570_map)

	MDRV_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/8, c1571_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/8, c1571_via1_intf)
	MDRV_CIA6526_ADD(M6526_TAG, CIA6526R1, XTAL_16MHz/8, c1571_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, c1571_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1570_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1571 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1571 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(c1571_map)

	MDRV_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/8, c1571_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/8, c1571_via1_intf)
	MDRV_CIA6526_ADD(M6526_TAG, CIA6526R1, XTAL_16MHz/8, c1571_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, c1571_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1571_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1571cr )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1571cr )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(c1571cr_map)

	MDRV_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/8, c1571_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/8, c1571_via1_intf)
	MDRV_CIA6526_ADD(M6526_TAG, CIA6526R1, XTAL_16MHz/8, c1571_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, c1571_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1571_floppy_config)
MACHINE_DRIVER_END

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
	ROM_REGION( 0x8000, "c1571", ROMREGION_LOADBYNAME )
	ROM_LOAD( "310654-03.u2", 0x0000, 0x8000, CRC(3889b8b8) SHA1(e649ef4419d65829d2afd65e07d31f3ce147d6eb) )
	ROM_LOAD( "310654-05.u2", 0x0000, 0x8000, CRC(5755bae3) SHA1(f1be619c106641a685f6609e4d43d6fc9eac1e70) )
ROM_END

/*-------------------------------------------------
    ROM( c1571cr )
-------------------------------------------------*/

ROM_START( c1571cr )
	ROM_REGION( 0x8000, "c1571cr", ROMREGION_LOADBYNAME )
	ROM_LOAD( "318047-01.", 0x0000, 0x8000, CRC(f24efcc4) SHA1(14ee7a0fb7e1c59c51fbf781f944387037daa3ee) )
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
	c1571->cpu = device_find_child_by_tag(device, M6502_TAG);

	/* find devices */
	c1571->via0 = device_find_child_by_tag(device, M6522_0_TAG);
	c1571->via1 = device_find_child_by_tag(device, M6522_1_TAG);
	c1571->cia = device_find_child_by_tag(device, M6526_TAG);
	c1571->wd1770 = device_find_child_by_tag(device, WD1770_TAG);
	c1571->serial_bus = devtag_get_device(device->machine, config->serial_bus_tag);
	c1571->image = device_find_child_by_tag(device, FLOPPY_0);

	/* set floppy density */
	wd17xx_set_density(c1571->wd1770, DEN_MFM_LO);

	/* allocate data timer */
	c1571->bit_timer = timer_alloc(device->machine, bit_tick, (void *)device);

	/* register for state saving */
}

/*-------------------------------------------------
    DEVICE_RESET( c1571 )
-------------------------------------------------*/

static DEVICE_RESET( c1571 )
{
	c1571_t *c1571 = get_safe_token(device);

	device_reset(c1571->cpu);
	device_reset(c1571->via0);
	device_reset(c1571->via1);
	device_reset(c1571->cia);
	device_reset(c1571->wd1770);
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
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1570);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1570);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1571);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1571);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1570");							break;
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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1571);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1571");							break;

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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1571cr);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1571CR");						break;

		default:										DEVICE_GET_INFO_CALL(c1570);								break;
	}
}
