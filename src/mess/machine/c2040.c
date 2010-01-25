/**********************************************************************

    Commodore 2040/3040/4040/8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- find GCR rom region
	- write to disk

*/

#include "emu.h"
#include "c2040.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_DOS_TAG	"un1"
#define M6532_0_TAG		"uc1"
#define M6532_1_TAG		"ue1"

#define M6502_FDC_TAG	"uh3"
#define M6522_TAG		"um3"
#define M6530_TAG		"uk3"

static const double C2040_BITRATE[4] =
{
	XTAL_16MHz/13.0,	/* tracks 1-17 */
	XTAL_16MHz/14.0,	/* tracks 18-24 */
	XTAL_16MHz/15.0, 	/* tracks 25-30 */
	XTAL_16MHz/16.0		/* tracks 31-42 */
};

#define SYNC_MARK			0x3ff		/* 10 consecutive 1-bits */

#define TRACK_BUFFER_SIZE	8194		/* 2 bytes track length + maximum of 8192 bytes of GCR encoded data */
#define TRACK_DATA_START	2

#define SYNC	((c2040->sr & SYNC_MARK) == SYNC_MARK)
#define ERROR	(!c2040->ready & !BIT(c2040->gcr_data, 3))

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c2040_unit_t c2040_unit_t;
struct _c2040_unit_t
{
	int stp;								/* stepper motor */
	UINT8 track_buffer[TRACK_BUFFER_SIZE];	/* track data buffer */
	int track_len;							/* track length */
	int buffer_pos;							/* current byte position within track buffer */
	int bit_pos;							/* current bit position within track buffer byte */

	/* devices */
	const device_config *image;
};

typedef struct _c2040_t c2040_t;
struct _c2040_t
{
	/* abstractions */
	int address;						/* bus address - 8 */
	int drive;							/* selected drive */
	int side;							/* selected side */
	c2040_unit_t unit[2];				/* drive unit */

	/* track */
	int bit_count;						/* current data byte bit counter */
	UINT16 sr;							/* data shift register */
	int ds;								/* density select */
	UINT8* gcr;							/* GCR encoder/decoder ROM */
	UINT8 gcr_data;						/* GCR encoder/decoded ROM data */
	UINT8 pi;							/* parallel data input */

	/* IEEE-488 bus */
	int rfdo;							/* not ready for data output */
	int daco;							/* not data accepted output */
	int atna;							/* attention acknowledge */

	/* signals */
	int error;
	int ready;							/* byte ready */
	int mode;							/* mode select */
	int rw;								/* read/write select */

	/* devices */
	const device_config *cpu_dos;
	const device_config *cpu_fdc;
	const device_config *riot0;
	const device_config *riot1;
	const device_config *riot2;
	const device_config *via;
	const device_config *bus;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c2040_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == C2040) || (device->type == C3040) || (device->type == C4040) ||
		(device->type == C8050) || (device->type == C8250) || (device->type == SFD1001));
	return (c2040_t *)device->token;
}

INLINE c2040_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == C2040) || (device->type == C3040) || (device->type == C4040) || 
		(device->type == C8050) || (device->type == C8250) || (device->type == SFD1001));
	return (c2040_config *)device->inline_config;
}

INLINE void update_ieee_signals(const device_config *device)
{
	c2040_t *c2040 = get_safe_token(device);

	int atn = ieee488_atn_r(c2040->bus);
	int atna = c2040->atna;
	int rfdo = c2040->rfdo;
	int daco = c2040->daco;

	int nrfd = !(!(!(atn&atna)&rfdo)|!(atn|atna));
	int ndac = !(daco|!(atn|atna));

	ieee488_nrfd_w(c2040->bus, device, nrfd);
	ieee488_ndac_w(c2040->bus, device, ndac);
}

INLINE void update_gcr_data(c2040_t *c2040)
{
	UINT16 gcr_addr;

	if (c2040->rw)
	{
		/*

			bit		description

			I0		SR0
			I1		SR1
			I2		SR2
			I3		SR3
			I4		SR4
			I5		SR5
			I6		SR6
			I7		SR7
			I8		SR8
			I9		SR9
			I10		R/_W SEL

		*/

		gcr_addr = (c2040->rw << 10) | (c2040->sr & 0x3ff);
	}
	else
	{
		/*

			bit		description

			I0		PI0
			I1		PI1
			I2		PI2
			I3		PI3
			I4		MODE SEL
			I5		PI4
			I6		PI5
			I7		PI6
			I8		PI7
			I9		0
			I10		R/_W SEL

		*/

		gcr_addr = (c2040->rw << 10) | ((c2040->pi & 0xf0) << 1) | (c2040->mode << 4) | (c2040->pi & 0x0f);
	}

	//	c2040->gcr_data = c2040->gcr[gcr_addr];

	via_cb1_w(c2040->via, ERROR);
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
	c2040_t *c2040 = get_safe_token(device);
	int ready = 1;

	/* shift in data from the read head */
	c2040->sr <<= 1;
	c2040->sr |= BIT(c2040->unit[c2040->drive].track_buffer[c2040->unit[c2040->drive].buffer_pos], c2040->unit[c2040->drive].bit_pos);
	c2040->unit[c2040->drive].bit_pos--;
	c2040->bit_count++;

	if (c2040->unit[c2040->drive].bit_pos < 0)
	{
		c2040->unit[c2040->drive].bit_pos = 7;
		c2040->unit[c2040->drive].buffer_pos++;

		if (c2040->unit[c2040->drive].buffer_pos > c2040->unit[c2040->drive].track_len + 1)
		{
			/* loop to the start of the track */
			c2040->unit[c2040->drive].buffer_pos = TRACK_DATA_START;
		}
	}

	if (SYNC)
	{
		/* SYNC detected */
		c2040->bit_count = 0;
	}

	if (c2040->bit_count > 7)
	{
		/* byte ready */
		c2040->bit_count = 0;
		ready = 0;

		if (!(c2040->sr & 0xff))
		{
			/* simulate weak bits with randomness */
			c2040->sr = (c2040->sr & 0xff00) | (mame_rand(machine) & 0xff);
		}
	}

	if (c2040->ready != ready)
	{
		/* set byte ready flag */
		c2040->ready = ready;
		via_ca1_w(c2040->via, ready);
	}

	/* update GCR data */
	update_gcr_data(c2040);
}

/*-------------------------------------------------
    c2040_ieee488_atn_w - IEEE-488 bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c2040_ieee488_atn_w )
{
	c2040_t *c2040 = get_safe_token(device);

	update_ieee_signals(device);

	/* set RIOT PA7 */
	riot6532_porta_in_set(c2040->riot1, !state << 7, 0x80);
}

/*-------------------------------------------------
    c2040_ieee488_ifc_w - IEEE-488 bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c2040_ieee488_ifc_w )
{
	if (!state)
	{
		device_reset(device);
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c2040_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c2040_dos_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x020f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x028f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x6000, 0x7fff) AM_MIRROR(0x0000) AM_ROM AM_REGION("c2040", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c2040_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c2040_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6522_TAG, via_r, via_w)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("c2040", 0x2000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c4040_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c4040_dos_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x020f) /*AM_MIRROR(0x0d70)*/ AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x028f) /*AM_MIRROR(0x0d70)*/ AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) /*AM_MIRROR(0x0c00)*/ AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) /*AM_MIRROR(0x0c00)*/ AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) /*AM_MIRROR(0x0c00)*/ AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) /*AM_MIRROR(0x0c00)*/ AM_RAM AM_SHARE("share4")
	AM_RANGE(0x5000, 0x7fff) AM_ROM AM_REGION("c4040", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c4040_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c4040_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0100) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) /*AM_MIRROR(0x0330)*/ AM_DEVREADWRITE(M6522_TAG, via_r, via_w)
	AM_RANGE(0x0080, 0x008f) /*AM_MIRROR(0x0330)*/ AM_DEVREADWRITE(M6530_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("c4040", 0x3000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c8050_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8050_dos_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0f00) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0f00) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x020f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x028f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c8050", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c8050_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8050_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6522_TAG, via_r, via_w)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("c8050", 0x4000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sfd1001_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sfd1001_dos_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0f00) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0f00) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x020f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x028f) AM_MIRROR(0x0d70) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("sfd1001", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sfd1001_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sfd1001_fdc_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6522_TAG, via_r, via_w)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1800, 0x1fff) AM_ROM AM_REGION("sfd1001", 0x4000)
ADDRESS_MAP_END

/*-------------------------------------------------
    riot6532_interface c2040_riot0_intf uc1
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( dio_r )
{
	/*

        bit     description

        PA0     DI0
        PA1     DI1
        PA2     DI2
        PA3     DI3
        PA4     DI4
        PA5     DI5
        PA6     DI6
        PA7     DI7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	return ieee488_dio_r(c2040->bus, 0);
}

static WRITE8_DEVICE_HANDLER( dio_w )
{
	/*

        bit     description

        PB0     DO0
        PB1     DO1
        PB2     DO2
        PB3     DO3
        PB4     DO4
        PB5     DO5
        PB6     DO6
        PB7     DO7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	ieee488_dio_w(c2040->bus, device->owner, data);
}

static const riot6532_interface c2040_riot0_intf =
{
	DEVCB_HANDLER(dio_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(dio_w),
	DEVCB_NULL
};

/*-------------------------------------------------
    riot6532_interface c2040_riot1_intf ue1
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c2040_riot1_pa_r )
{
	/*

        bit     description

        PA0     ATNA
        PA1		DACO
        PA2		RFDO
        PA3		EOIO
        PA4		DAVO
        PA5		EOII
        PA6		DAVI
        PA7		_ATN

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT8 data = 0;

	/* end or identify in */
	data |= ieee488_eoi_r(c2040->bus) << 5;

	/* data valid in */
	data |= ieee488_dav_r(c2040->bus) << 6;

	/* attention */
	data |= !ieee488_atn_r(c2040->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c2040_riot1_pa_w )
{
	/*

        bit     description

        PA0     ATNA
        PA1		DACO
        PA2		RFDO
        PA3		EOIO
        PA4		DAVO
        PA5		EOII
        PA6		DAVI
        PA7		_ATN

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	/* attention acknowledge */
	c2040->atna = BIT(data, 0);

	/* data accepted output */
	c2040->daco = BIT(data, 1);

	/* not ready for data output */
	c2040->rfdo = BIT(data, 2);

	/* end or identify */
	ieee488_eoi_w(c2040->bus, device->owner, BIT(data, 3));

	/* data valid */
	ieee488_dav_w(c2040->bus, device->owner, BIT(data, 4));

	update_ieee_signals(device->owner);
}

static READ8_DEVICE_HANDLER( c2040_riot1_pb_r )
{
	/*

        bit     description

        PB0		DEVICE NUMBER SELECTION
        PB1		DEVICE NUMBER SELECTION
        PB2		DEVICE NUMBER SELECTION
        PB3		ACT LED 1
        PB4		ACT LED 0
        PB5		ERR LED
        PB6		DACI
        PB7		RFDI

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT8 data = 0;

	/* device number selection */
	data |= c2040->address;

	/* data accepted in */
	data |= ieee488_ndac_r(c2040->bus) << 6;

	/* ready for data in */
	data |= ieee488_nrfd_r(c2040->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c2040_riot1_pb_w )
{
	/*

        bit     description

        PB0		DEVICE NUMBER SELECTION
        PB1		DEVICE NUMBER SELECTION
        PB2		DEVICE NUMBER SELECTION
        PB3		ACT LED 1
        PB4		ACT LED 0
        PB5		ERR LED
        PB6		DACI
        PB7		RFDI

    */

	/* TODO active led 1 */

	/* TODO active led 0 */

	/* TODO error led */
}

static WRITE_LINE_DEVICE_HANDLER( c2040_riot1_irq_w )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	cpu_set_input_line(c2040->cpu_dos, M6502_IRQ_LINE, state);
}

static const riot6532_interface c2040_riot1_intf =
{
	DEVCB_HANDLER(c2040_riot1_pa_r),
	DEVCB_HANDLER(c2040_riot1_pb_r),
	DEVCB_HANDLER(c2040_riot1_pa_w),
	DEVCB_HANDLER(c2040_riot1_pb_w),
	DEVCB_LINE(c2040_riot1_irq_w)
};

/*-------------------------------------------------
    via6522_interface c2040_via_intf um3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c2040_via_pa_r )
{
	/*

        bit     description

        PA0     E0
        PA1		E1
        PA2		I2
        PA3		E2
        PA4		E4
        PA5		E5
        PA6		I7
        PA7		E6

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT16 i = c2040->sr;
	UINT8 e = c2040->gcr_data;

	return (BIT(e, 6) << 7) | (BIT(i, 7) << 6) | (e & 0x30) | (BIT(e, 2) << 3) | (i & 0x04) | (e & 0x03);
}

static READ8_DEVICE_HANDLER( c2040_via_pb_r )
{
	/*

        bit     description

        PB0		S1A
        PB1		S1B
        PB2		S0A
        PB3		S0B
        PB4		MTR1
        PB5		MTR0
        PB6		
        PB7		

    */

	return 0;
}

static void step_motor(c2040_t *c2040, int unit, int stp)
{
	if (c2040->unit[unit].stp != stp)
	{
		int tracks = 0;

		switch (c2040->unit[unit].stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 3) tracks--; break;
		case 1:	if (stp == 2) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 3) tracks++; else if (stp == 1) tracks--; break;
		case 3: if (stp == 0) tracks++; else if (stp == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			c2040->unit[unit].track_len = TRACK_BUFFER_SIZE;
			c2040->unit[unit].buffer_pos = TRACK_DATA_START;
			c2040->unit[unit].bit_pos = 7;
			c2040->bit_count = 0;

			/* step read/write head */
			floppy_drive_seek(c2040->unit[unit].image, tracks);

			/* read track data */
			floppy_drive_read_track_data_info_buffer(c2040->unit[unit].image, c2040->side, c2040->unit[unit].track_buffer, &c2040->unit[unit].track_len);

			/* extract track length */
			c2040->unit[unit].track_len = (c2040->unit[unit].track_buffer[1] << 8) | c2040->unit[unit].track_buffer[0];
		}
	}
}

static WRITE8_DEVICE_HANDLER( c2040_via_pb_w )
{
	/*

        bit     description

        PB0		S1A
        PB1		S1B
        PB2		S0A
        PB3		S0B
        PB4		MTR1
        PB5		MTR0
        PB6		
        PB7		

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	/* stepper motor 1 */
	int s1 = data & 0x03;
	step_motor(c2040, 1, s1);
	logerror("S1 %u\n", s1);

	/* stepper motor 0 */
	int s0 = (data >> 2) & 0x03;
	step_motor(c2040, 0, s0);
	logerror("S0 %u\n", s1);

	/* spindle motor 1 */
	int mtr1 = BIT(data, 4);
	floppy_mon_w(c2040->unit[1].image, !mtr1);
	logerror("MTR1 %u\n", mtr1);

	/* spindle motor 0 */
	int mtr0 = BIT(data, 5);
	floppy_mon_w(c2040->unit[0].image, !mtr0);
	logerror("MTR0 %u\n", mtr0);

	timer_enable(c2040->bit_timer, mtr1 | mtr0);
}

static READ8_DEVICE_HANDLER( c8050_via_pb_r )
{
	/*

        bit     description

        PB0		S1A
        PB1		S1B
        PB2		S0A
        PB3		S0B
        PB4		MTR1
        PB5		MTR0
        PB6		PULL SYNC
        PB7		SYNC

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT8 data = 0;

	/* SYNC detected */
	data |= !SYNC << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c8050_via_pb_w )
{
	/*

        bit     description

        PB0		S1A
        PB1		S1B
        PB2		S0A
        PB3		S0B
        PB4		MTR1
        PB5		MTR0
        PB6		PULL SYNC
        PB7		SYNC

    */

	c2040_via_pb_w(device, 0, data);
}

static READ_LINE_DEVICE_HANDLER( ready_r )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	return c2040->ready;
}

static READ_LINE_DEVICE_HANDLER( err_r )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	return ERROR;
}

static WRITE_LINE_DEVICE_HANDLER( mode_sel_w )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	/* mode select */
	c2040->mode = state;

	update_gcr_data(c2040);
}

static WRITE_LINE_DEVICE_HANDLER( rw_sel_w )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	/* read/write select */
	c2040->rw = state;

	update_gcr_data(c2040);
}

static const via6522_interface c2040_via_intf =
{
	DEVCB_HANDLER(c2040_via_pa_r),
	DEVCB_HANDLER(c2040_via_pb_r),
	DEVCB_LINE(ready_r),
	DEVCB_LINE(err_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(c2040_via_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(mode_sel_w),
	DEVCB_LINE(rw_sel_w),

	DEVCB_NULL
};

static const via6522_interface c8050_via_intf =
{
	DEVCB_HANDLER(c2040_via_pa_r),
	DEVCB_HANDLER(c8050_via_pb_r),
	DEVCB_LINE(ready_r),
	DEVCB_LINE(err_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(c8050_via_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(mode_sel_w),
	DEVCB_LINE(rw_sel_w),

	DEVCB_NULL
};

/*-------------------------------------------------
    riot6532_interface c2040_riot2_intf uk3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( pi_r )
{
	/*

        bit     description

        PA0     PI0
        PA1		PI1
        PA2		PI2
        PA3		PI3
        PA4		PI4
        PA5		PI5
        PA6		PI6
        PA7		PI7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	return c2040->pi;
}

static WRITE8_DEVICE_HANDLER( pi_w )
{
	/*

        bit     description

        PA0     PI0
        PA1		PI1
        PA2		PI2
        PA3		PI3
        PA4		PI4
        PA5		PI5
        PA6		PI6
        PA7		PI7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	c2040->pi = data;
}

static READ8_DEVICE_HANDLER( c2040_riot2_pb_r )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1		DS0
        PB2		DS1
        PB3		WPS
        PB4		
        PB5
        PB6		SYNC
        PB7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT8 data = 0;

	/* write protect sense */
	data |= floppy_wpt_r(c2040->unit[c2040->drive].image) << 3;

	/* SYNC detected */
	data |= !SYNC << 6;

	return data;
}

static WRITE8_DEVICE_HANDLER( c2040_riot2_pb_w )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1		DS0
        PB2		DS1
        PB3		WPS
        PB4		
        PB5
        PB6		SYNC
        PB7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	/* drive select */
	c2040->drive = BIT(data, 0);

	/* density select */
	int ds = (data >> 1) & 0x03;

	if (c2040->ds != ds)
	{
		timer_adjust_periodic(c2040->bit_timer, attotime_zero, 0, ATTOTIME_IN_HZ(C2040_BITRATE[ds]/4));
		c2040->ds = ds;
	}
}

static READ8_DEVICE_HANDLER( c8050_riot2_pb_r )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1		DS0
        PB2		DS1
        PB3		WPS
        PB4		ODD HD, SIDE SELECT (0=78-154, 1=1-77)
        PB5
        PB6		(0=DS, 1=SS)
        PB7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	UINT8 data = 0;

	/* write protect sense */
	data |= floppy_wpt_r(c2040->unit[c2040->drive].image) << 3;

	/* single/dual sided */
	data |= (device->type == C8050) << 6;

	return data;
}

static WRITE8_DEVICE_HANDLER( c8050_riot2_pb_w )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1		DS0
        PB2		DS1
        PB3		WPS
        PB4		ODD HD, SIDE SELECT (0=78-154, 1=1-77)
        PB5
        PB6		(0=DS, 1=SS)
        PB7

    */

	c2040_t *c2040 = get_safe_token(device->owner);

	c2040_riot2_pb_w(device, 0, data);

	/* side select */
	c2040->side = !BIT(data, 4);
}

static WRITE_LINE_DEVICE_HANDLER( c2040_riot2_irq_w )
{
	c2040_t *c2040 = get_safe_token(device->owner);

	cpu_set_input_line(c2040->cpu_fdc, M6502_IRQ_LINE, state);
}

static const riot6532_interface c2040_riot2_intf =
{
	DEVCB_HANDLER(pi_r),
	DEVCB_HANDLER(c2040_riot2_pb_r),
	DEVCB_HANDLER(pi_w),
	DEVCB_HANDLER(c2040_riot2_pb_w),
	DEVCB_LINE(c2040_riot2_irq_w)
};

static const riot6532_interface c8050_riot2_intf =
{
	DEVCB_HANDLER(pi_r),
	DEVCB_HANDLER(c8050_riot2_pb_r),
	DEVCB_HANDLER(pi_w),
	DEVCB_HANDLER(c8050_riot2_pb_w),
	DEVCB_LINE(c2040_riot2_irq_w)
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c2040 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c2040 )
	FLOPPY_OPTION( c2040, "d67", "Commodore 2040/3040 (DOS 1) Disk Image", d67_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    FLOPPY_OPTIONS( c4040 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c4040 )
	FLOPPY_OPTION( c4040, "d64", "Commodore 4040 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c4040, "g64", "Commodore 4040 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    FLOPPY_OPTIONS( c8050 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c8050 )
	FLOPPY_OPTION( c8050, "d80", "Commodore 8050 Disk Image", d80_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    FLOPPY_OPTIONS( c8050 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c8250 )
	FLOPPY_OPTION( c8250, "d80", "Commodore 8050 Disk Image", d80_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c8250, "d82", "Commodore 8250/SFD1001 Disk Image", d82_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c2040_floppy_config
-------------------------------------------------*/

static const floppy_config c2040_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c2040),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    floppy_config c4040_floppy_config
-------------------------------------------------*/

static const floppy_config c4040_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c4040),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    floppy_config c8050_floppy_config
-------------------------------------------------*/

static const floppy_config c8050_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c8050),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    floppy_config c8250_floppy_config
-------------------------------------------------*/

static const floppy_config c8250_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(c8250),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c2040 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c2040 )
	/* DOS */
	MDRV_CPU_ADD(M6502_DOS_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c2040_dos_map)
	
	MDRV_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, c2040_riot0_intf)
	MDRV_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, c2040_riot1_intf)

	/* controller */
	MDRV_CPU_ADD(M6502_FDC_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c2040_fdc_map)
	
	MDRV_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, c2040_via_intf)
	MDRV_RIOT6532_ADD(M6530_TAG, XTAL_16MHz/16, c2040_riot2_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(c2040_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c4040 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c4040 )
	/* DOS */
	MDRV_CPU_ADD(M6502_DOS_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c4040_dos_map)

	MDRV_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, c2040_riot0_intf)
	MDRV_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, c2040_riot1_intf)

	/* controller */
	MDRV_CPU_ADD(M6502_FDC_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c4040_fdc_map)

	MDRV_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, c2040_via_intf)
	MDRV_RIOT6532_ADD(M6530_TAG, XTAL_16MHz/16, c2040_riot2_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(c4040_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c8050 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c8050 )
	/* DOS */
	MDRV_CPU_ADD(M6502_DOS_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(c8050_dos_map)
	
	MDRV_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/3, c2040_riot0_intf)
	MDRV_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/3, c2040_riot1_intf)

	/* controller */
	MDRV_CPU_ADD(M6502_FDC_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(c8050_fdc_map)
	
	MDRV_VIA6522_ADD(M6522_TAG, XTAL_12MHz/3, c8050_via_intf)
	MDRV_RIOT6532_ADD(M6530_TAG, XTAL_12MHz/3, c8050_riot2_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(c8050_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c8250 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c8250 )
	/* DOS */
	MDRV_CPU_ADD(M6502_DOS_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(c8050_dos_map)
	
	MDRV_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/3, c2040_riot0_intf)
	MDRV_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/3, c2040_riot1_intf)

	/* controller */
	MDRV_CPU_ADD(M6502_FDC_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(c8050_fdc_map)
	
	MDRV_VIA6522_ADD(M6522_TAG, XTAL_12MHz/3, c8050_via_intf)
	MDRV_RIOT6532_ADD(M6530_TAG, XTAL_12MHz/3, c8050_riot2_intf)

	MDRV_FLOPPY_2_DRIVES_ADD(c8250_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( sfd1001 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( sfd1001 )
	/* DOS */
	MDRV_CPU_ADD(M6502_DOS_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(sfd1001_dos_map)
	
	MDRV_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/3, c2040_riot0_intf)
	MDRV_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/3, c2040_riot1_intf)

	/* controller */
	MDRV_CPU_ADD(M6502_FDC_TAG, M6502, XTAL_12MHz/3)
	MDRV_CPU_PROGRAM_MAP(sfd1001_fdc_map)
	
	MDRV_VIA6522_ADD(M6522_TAG, XTAL_12MHz/3, c8050_via_intf)
	MDRV_RIOT6532_ADD(M6530_TAG, XTAL_12MHz/3, c8050_riot2_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c8250_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c2040 )
-------------------------------------------------*/

ROM_START( c2040 )
	ROM_REGION( 0x2c00, "c2040", ROMREGION_LOADBYNAME )
	/* DOS 1.0 */
	ROM_LOAD( "901468-06.ul1", 0x0000, 0x1000, CRC(25b5eed5) SHA1(4d9658f2e6ff3276e5c6e224611a66ce44b16fc7) )
	ROM_LOAD( "901468-07.uh1", 0x1000, 0x1000, CRC(9b09ae83) SHA1(6a51c7954938439ca8342fc295bda050c06e1791) )

	/* RIOT DOS 1 */
	ROM_LOAD( "901466-02.uk3", 0x2000, 0x0400, NO_DUMP )

	/* ROM GCR */
	ROM_LOAD( "901467.uk6", 0x2400, 0x0800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( c4040 )
-------------------------------------------------*/

ROM_START( c4040 )
	ROM_REGION( 0x3c00, "c4040", ROMREGION_LOADBYNAME )
	/* DOS 2.1 */
	ROM_LOAD( "901468-11.uj1", 0x0000, 0x1000, CRC(b7157458) SHA1(8415f3159dea73161e0cef7960afa6c76953b6f8) )
	ROM_LOAD( "901468-12.ul1", 0x1000, 0x1000, CRC(02c44ff9) SHA1(e8a94f239082d45f64f01b2d8e488d18fe659cbb) )
	ROM_LOAD( "901468-13.uh1", 0x2000, 0x1000, CRC(cbd785b3) SHA1(6ada7904ac9d13c3f1c0a8715f9c4be1aa6eb0bb) )
	/* DOS 2 rev */
	ROM_LOAD( "901468-14.uj1", 0x0000, 0x1000, CRC(bc4d4872) SHA1(ffb992b82ec913ddff7be964d7527aca3e21580c) )
	ROM_LOAD( "901468-15.ul1", 0x1000, 0x1000, CRC(b6970533) SHA1(f702d6917fe8a798740ba4d467b500944ae7b70a) )
	ROM_LOAD( "901468-16.uh1", 0x2000, 0x1000, CRC(1f5eefb7) SHA1(04b918cf4adeee8015b43383d3cea7288a7d0aa8) )

	/* RIOT DOS 2 */
	ROM_LOAD( "901466-04.uk3", 0x3000, 0x0400, CRC(0ab338dc) SHA1(6645fa40b81be1ff7d1384e9b52df06a26ab0bfb) )

	/* ROM GCR */
	ROM_LOAD( "901467.uk6", 0x3400, 0x800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( c8050 )
-------------------------------------------------*/

/*

	DOS/CONTROLLER ROMS FOR DIGITAL PCB #8050002

	DESCRIPTION		PCB PART NO.		UL1			UH1			UK3

	2.5 Micropolis	8050002-01		901482-07	901482-06	901483-03
	2.5 Tandon		8050002-02		901482-07	901482-06	901483-04
	2.7 Tandon		8050002-03		901887-01	901888-01	901884-01
	2.7 Micropolis	8050002-04		901887-01	901888-01	901885-04
	2.7 MPI 8050	8050002-05		901887-01	901888-01	901869-01
	2.7 MPI 8250	8050002-06		901887-01	901888-01	901869-01

*/

ROM_START( c8050 ) // schematic 8050001
	ROM_REGION( 0x4c00, "c8050", ROMREGION_LOADBYNAME )
	/* 2364 ROM DOS 2.5 */
	ROM_LOAD( "901482-03.ul1", 0x0000, 0x2000, CRC(09a609b9) SHA1(166d8bfaaa9c4767f9b17ad63fc7ae77c199a64e) )
	ROM_LOAD( "901482-04.uh1", 0x2000, 0x2000, CRC(1bcf9df9) SHA1(217f4a8b348658bb365f4a1de21ecbaa6402b1c0) )
	/* 2364-091 ROM DOS 2.5 rev */
	ROM_LOAD( "901482-07.ul1", 0x0000, 0x2000, CRC(c7532d90) SHA1(0b6d1e55afea612516df5f07f4a6dccd3bd73963) )
	ROM_LOAD( "901482-06.uh1", 0x2000, 0x2000, CRC(3cbd2756) SHA1(7f5fbed0cddb95138dd99b8fe84fddab900e3650) )
	/* 2364 ROM DOS 2.7 */
	ROM_LOAD( "901887-01.ul1", 0x0000, 0x2000, CRC(0073b8b2) SHA1(b10603195f240118fe5fb6c6dfe5c5097463d890) )
	ROM_LOAD( "901888-01.uh1", 0x2000, 0x2000, CRC(de9b6132) SHA1(2e6c2d7ca934e5c550ad14bd5e9e7749686b7af4) )

	/* controller */
	ROM_LOAD( "901483-03.uk3", 0x4000, 0x400, CRC(9e83fa70) SHA1(e367ea8a5ddbd47f13570088427293138a10784b) )
	ROM_LOAD( "901483-04.uk3", 0x4000, 0x400, NO_DUMP )
	ROM_LOAD( "901884-01.uk3", 0x4000, 0x400, NO_DUMP )
	ROM_LOAD( "901885-04.uk3", 0x4000, 0x400, CRC(bab998c9) SHA1(0dc9a3b60f1b866c63eebd882403532fc59fe57f) )
	ROM_LOAD( "901869-01.uk3", 0x4000, 0x400, CRC(2915327a) SHA1(3a9a80f72ce76e5f5c72513f8ef7553212912ae3) )
	
	/* GCR decoder */
	ROM_LOAD( "901467.uk6", 0x4400, 0x800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( sfd1001 )
-------------------------------------------------*/

ROM_START( sfd1001 ) // schematic 
	ROM_REGION( 0x5000, "sfd1001", ROMREGION_LOADBYNAME )
	ROM_LOAD( "901887-01.1j", 0x0000, 0x2000, CRC(0073b8b2) SHA1(b10603195f240118fe5fb6c6dfe5c5097463d890) )
	ROM_LOAD( "901888-01.3j", 0x2000, 0x2000, CRC(de9b6132) SHA1(2e6c2d7ca934e5c550ad14bd5e9e7749686b7af4) )

	/* controller */
	ROM_LOAD( "251257-02a.u2", 0x4000, 0x0800, CRC(b51150de) SHA1(3b954eb34f7ea088eed1d33ebc6d6e83a3e9be15) )

	/* GCR decoder */
	ROM_LOAD( "901467.5c", 0x4800, 0x800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c2040 )
-------------------------------------------------*/

static DEVICE_START( c2040 )
{
	c2040_t *c2040 = get_safe_token(device);
	const c2040_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 16));
	c2040->address = config->address - 8;

	/* find our CPU */
	c2040->cpu_dos = device_find_child_by_tag(device, M6502_DOS_TAG);
	c2040->cpu_fdc = device_find_child_by_tag(device, M6502_FDC_TAG);

	/* find devices */
	c2040->riot0 = device_find_child_by_tag(device, M6532_0_TAG);
	c2040->riot1 = device_find_child_by_tag(device, M6532_1_TAG);
	c2040->riot2 = device_find_child_by_tag(device, M6530_TAG);
	c2040->via = device_find_child_by_tag(device, M6522_TAG);
	c2040->bus = devtag_get_device(device->machine, config->bus_tag);
	c2040->unit[0].image = device_find_child_by_tag(device, FLOPPY_0);
	c2040->unit[1].image = device_find_child_by_tag(device, FLOPPY_1);

	/* find GCR ROM */

	/* allocate data timer */
	c2040->bit_timer = timer_alloc(device->machine, bit_tick, (void *)device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, c2040->);
}

/*-------------------------------------------------
    DEVICE_RESET( c2040 )
-------------------------------------------------*/

static DEVICE_RESET( c2040 )
{
	c2040_t *c2040 = get_safe_token(device);

	/* reset devices */
	device_reset(c2040->cpu_dos);
	device_reset(c2040->cpu_fdc);
	device_reset(c2040->riot0);
	device_reset(c2040->riot1);
	device_reset(c2040->riot2);
	device_reset(c2040->via);

	/* toggle SO */
	cpu_set_input_line(c2040->cpu_dos, M6502_SET_OVERFLOW, ASSERT_LINE);
	cpu_set_input_line(c2040->cpu_dos, M6502_SET_OVERFLOW, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c2040 )
-------------------------------------------------*/

DEVICE_GET_INFO( c2040 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c2040_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c2040_config);								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c2040);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c2040);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c2040);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c2040);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 2040");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 2040");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c3040 )
-------------------------------------------------*/

DEVICE_GET_INFO( c3040 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c2040);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c2040);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 3040");							break;

		default:										DEVICE_GET_INFO_CALL(c2040);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c4040 )
-------------------------------------------------*/

DEVICE_GET_INFO( c4040 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c4040);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c4040);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 4040");							break;

		default:										DEVICE_GET_INFO_CALL(c3040);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c8050 )
-------------------------------------------------*/

DEVICE_GET_INFO( c8050 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c8050);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c8050);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 8050");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 8050");							break;

		default:										DEVICE_GET_INFO_CALL(c2040);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c8250 )
-------------------------------------------------*/

DEVICE_GET_INFO( c8250 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c8050);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c8250);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 8250");							break;

		default:										DEVICE_GET_INFO_CALL(c8050);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( sfd1001 )
-------------------------------------------------*/

DEVICE_GET_INFO( sfd1001 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(sfd1001);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(sfd1001);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore SFD-1001");						break;

		default:										DEVICE_GET_INFO_CALL(c8050);								break;
	}
}
