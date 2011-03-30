/**********************************************************************

    Commodore 2040/3040/4040/8050/8250/SFD-1001 Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - 2040 DOS 1 FDC rom (jumps to 104d while getting block header)

        FE70: jsr  $104D
        104D: m6502_brk#$00

    - Micropolis 8x50 stepper motor is same as 4040, except it takes 4 pulses to step a track instead of 1
    - error/activity LEDs

*/

#include "emu.h"
#include "c2040.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/mos6530.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6502_TAG		"un1"
#define M6532_0_TAG		"uc1"
#define M6532_1_TAG		"ue1"

#define M6504_TAG		"uh3"
#define M6522_TAG		"um3"
#define M6530_TAG		"uk3"

#define C2040_REGION	"c2040"
#define C4040_REGION	"c4040"
#define C8050_REGION	"c8050"
#define SFD1001_REGION	"sfd1001"

#define SYNC \
	(!(((c2040->sr & G64_SYNC_MARK) == G64_SYNC_MARK) & c2040->rw))

#define ERROR \
	(!(c2040->ready | BIT(c2040->e, 3)))

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c2040_unit_t c2040_unit_t;
struct _c2040_unit_t
{
	/* motors */
	int stp;								/* stepper motor phase */
	int mtr;								/* spindle motor on */

	/* track */
	UINT8 track_buffer[G64_BUFFER_SIZE];	/* track data buffer */
	int track_len;							/* track length */
	int buffer_pos;							/* byte position within track buffer */
	int bit_pos;							/* bit position within track buffer byte */

	/* devices */
	device_t *image;
};

typedef struct _c2040_t c2040_t;
struct _c2040_t
{
	/* hardware */
	c2040_unit_t unit[2];				/* drive unit */
	int drive;							/* selected drive */
	int side;							/* selected side */

	/* IEEE-488 bus */
	int address;						/* device address - 8 */
	int rfdo;							/* not ready for data output */
	int daco;							/* not data accepted output */
	int atna;							/* attention acknowledge */

	/* track */
	int ds;								/* density select */
	int bit_count;						/* GCR bit counter */
	UINT16 sr;							/* GCR data shift register */
	UINT8 pi;							/* parallel data input */
	UINT8* gcr;							/* GCR encoder/decoder ROM */
	UINT16 i;							/* GCR encoder/decoded ROM address */
	UINT8 e;							/* GCR encoder/decoded ROM data */

	/* signals */
	int ready;							/* byte ready */
	int mode;							/* mode select */
	int rw;								/* read/write select */
	int miot_irq;						/* MIOT interrupt */

	/* devices */
	device_t *cpu_dos;
	device_t *cpu_fdc;
	device_t *riot0;
	device_t *riot1;
	device_t *miot;
	via6522_device *via;
	device_t *bus;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c2040_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C2040) || (device->type() == C3040) || (device->type() == C4040) ||
		(device->type() == C8050) || (device->type() == C8250) || (device->type() == SFD1001));
	return (c2040_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c2040_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C2040) || (device->type() == C3040) || (device->type() == C4040) ||
		(device->type() == C8050) || (device->type() == C8250) || (device->type() == SFD1001));
	return (c2040_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE void update_ieee_signals(device_t *device)
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
	if (c2040->rw)
	{
		/*

            bit     description

            I0      SR0
            I1      SR1
            I2      SR2
            I3      SR3
            I4      SR4
            I5      SR5
            I6      SR6
            I7      SR7
            I8      SR8
            I9      SR9
            I10     R/_W SEL

        */

		c2040->i = (c2040->rw << 10) | (c2040->sr & 0x3ff);
	}
	else
	{
		/*

            bit     description

            I0      PI0
            I1      PI1
            I2      PI2
            I3      PI3
            I4      MODE SEL
            I5      PI4
            I6      PI5
            I7      PI6
            I8      PI7
            I9      0
            I10     R/_W SEL

        */

		c2040->i = (c2040->rw << 10) | ((c2040->pi & 0xf0) << 1) | (c2040->mode << 4) | (c2040->pi & 0x0f);
	}

	c2040->e = c2040->gcr[c2040->i];
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
	c2040_t *c2040 = get_safe_token(device);
	int ready = 1;

	/* shift in data from the read head */
	c2040->sr <<= 1;
	c2040->sr |= BIT(c2040->unit[c2040->drive].track_buffer[c2040->unit[c2040->drive].buffer_pos], c2040->unit[c2040->drive].bit_pos);

	/* update GCR data */
	update_gcr_data(c2040);

	/* update bit counters */
	c2040->unit[c2040->drive].bit_pos--;
	c2040->bit_count++;

	if (c2040->unit[c2040->drive].bit_pos < 0)
	{
		c2040->unit[c2040->drive].bit_pos = 7;
		c2040->unit[c2040->drive].buffer_pos++;

		if (c2040->unit[c2040->drive].buffer_pos >= c2040->unit[c2040->drive].track_len)
		{
			/* loop to the start of the track */
			c2040->unit[c2040->drive].buffer_pos = 0;
		}
	}

	if (!SYNC)
	{
		/* SYNC detected */
		c2040->bit_count = 0;
	}

	if (c2040->bit_count == 10)
	{
		/* byte ready */
		c2040->bit_count = 0;
		ready = 0;
	}

	if (c2040->ready != ready)
	{
		/* set byte ready flag */
		c2040->ready = ready;

		c2040->via->write_ca1(ready);
		c2040->via->write_cb1(ERROR);

		if ((device->type() == C8050) || (device->type() == C8250) || (device->type() == SFD1001))
		{
			device_set_input_line(c2040->cpu_fdc, M6502_SET_OVERFLOW, ready ? CLEAR_LINE : ASSERT_LINE);
		}
	}
}

/*-------------------------------------------------
    read_current_track - read track from disk
-------------------------------------------------*/

static void read_current_track(c2040_t *c2040, int unit)
{
	c2040->unit[unit].track_len = G64_BUFFER_SIZE;
	c2040->unit[unit].buffer_pos = 0;
	c2040->unit[unit].bit_pos = 7;
	c2040->bit_count = 0;

	/* read track data */
	floppy_drive_read_track_data_info_buffer(c2040->unit[unit].image, c2040->side, c2040->unit[unit].track_buffer, &c2040->unit[unit].track_len);

	/* extract track length */
	c2040->unit[unit].track_len = floppy_drive_get_current_track_size(c2040->unit[unit].image, c2040->side);
}

/*-------------------------------------------------
    on_disk_0_change - disk 0 change handler
-------------------------------------------------*/

static void on_disk_0_change(device_image_interface &image)
{
	c2040_t *c2040 = get_safe_token(image.device().owner());

	read_current_track(c2040, 0);
}

/*-------------------------------------------------
    on_disk_1_change - disk 1 change handler
-------------------------------------------------*/

static void on_disk_1_change(device_image_interface &image)
{
	c2040_t *c2040 = get_safe_token(image.device().owner());

	read_current_track(c2040, 1);
}

/*-------------------------------------------------
    spindle_motor - spindle motor control
-------------------------------------------------*/

static void spindle_motor(c2040_t *c2040, int unit, int mtr)
{
	if (c2040->unit[unit].mtr != mtr)
	{
		if (!mtr)
		{
			/* read track data */
			read_current_track(c2040, unit);
		}

		floppy_mon_w(c2040->unit[unit].image, mtr);

		c2040->unit[unit].mtr = mtr;
	}
}

/*-------------------------------------------------
    micropolis_step_motor - Micropolis stepper
    motor control
-------------------------------------------------*/

static void micropolis_step_motor(c2040_t *c2040, int unit, int mtr, int stp)
{
	if (!mtr && (c2040->unit[unit].stp != stp))
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
			/* step read/write head */
			floppy_drive_seek(c2040->unit[unit].image, tracks);

			/* read new track data */
			read_current_track(c2040, unit);
		}

		c2040->unit[unit].stp = stp;
	}
}

/*-------------------------------------------------
    mpi_step_motor - MPI stepper motor control
-------------------------------------------------*/

static void mpi_step_motor(c2040_t *c2040, int unit, int mtr, int stp)
{
	if (!mtr && (c2040->unit[unit].stp != stp))
	{
		int tracks = 0;

		switch (c2040->unit[unit].stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 2) tracks--; break;
		case 1:	if (stp == 3) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 0) tracks++; else if (stp == 3) tracks--; break;
		case 3: if (stp == 2) tracks++; else if (stp == 1) tracks--; break;
		}

		if (tracks != 0)
		{
			/* step read/write head */
			floppy_drive_seek(c2040->unit[unit].image, tracks);

			/* read new track data */
			read_current_track(c2040, unit);
		}

		c2040->unit[unit].stp = stp;
	}
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
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c2040_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c2040_dos_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x6000, 0x7fff) AM_MIRROR(0x0000) AM_ROM AM_REGION("drive:c2040", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c2040_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c2040_fdc_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE_MODERN(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, mos6530_r, mos6530_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("drive:c2040", 0x2000) // 6530
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c4040_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c4040_dos_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x5000, 0x7fff) AM_ROM AM_REGION("drive:c4040", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c4040_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c4040_fdc_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE_MODERN(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, mos6530_r, mos6530_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("drive:c4040", 0x3000) // 6530
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c8050_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8050_dos_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("drive:c8050", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c8050_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c8050_fdc_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE_MODERN(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, mos6530_r, mos6530_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1c00, 0x1fff) AM_ROM AM_REGION("drive:c8050", 0x4000) // 6530
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sfd1001_dos_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sfd1001_dos_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_MIRROR(0x0100) AM_RAM // 6532 #1
	AM_RANGE(0x0080, 0x00ff) AM_MIRROR(0x0100) AM_RAM // 6532 #2
	AM_RANGE(0x0200, 0x021f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_0_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x0280, 0x029f) AM_MIRROR(0x0d60) AM_DEVREADWRITE(M6532_1_TAG, riot6532_r, riot6532_w)
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0c00) AM_RAM AM_SHARE("share4")
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("drive:sfd1001", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sfd1001_fdc_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sfd1001_fdc_map, AS_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x0000, 0x003f) AM_MIRROR(0x0300) AM_RAM // 6530
	AM_RANGE(0x0040, 0x004f) AM_MIRROR(0x0330) AM_DEVREADWRITE_MODERN(M6522_TAG, via6522_device, read, write)
	AM_RANGE(0x0080, 0x008f) AM_MIRROR(0x0330) AM_DEVREADWRITE(M6530_TAG, mos6530_r, mos6530_w)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_SHARE("share1")
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_SHARE("share2")
	AM_RANGE(0x0c00, 0x0fff) AM_RAM AM_SHARE("share3")
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_SHARE("share4")
	AM_RANGE(0x1800, 0x1fff) AM_ROM AM_REGION("drive:sfd1001", 0x4000)
ADDRESS_MAP_END

/*-------------------------------------------------
    riot6532_interface riot0_intf uc1
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

	c2040_t *c2040 = get_safe_token(device->owner());

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

	c2040_t *c2040 = get_safe_token(device->owner());

	ieee488_dio_w(c2040->bus, device->owner(), data);
}

static const riot6532_interface riot0_intf =
{
	DEVCB_HANDLER(dio_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(dio_w),
	DEVCB_NULL
};

/*-------------------------------------------------
    riot6532_interface riot1_intf ue1
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( riot1_pa_r )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* end or identify in */
	data |= ieee488_eoi_r(c2040->bus) << 5;

	/* data valid in */
	data |= ieee488_dav_r(c2040->bus) << 6;

	/* attention */
	data |= !ieee488_atn_r(c2040->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( riot1_pa_w )
{
	/*

        bit     description

        PA0     ATNA
        PA1     DACO
        PA2     RFDO
        PA3     EOIO
        PA4     DAVO
        PA5     EOII
        PA6     DAVI
        PA7     _ATN

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	/* attention acknowledge */
	c2040->atna = BIT(data, 0);

	/* data accepted out */
	c2040->daco = BIT(data, 1);

	/* not ready for data out */
	c2040->rfdo = BIT(data, 2);

	/* end or identify out */
	ieee488_eoi_w(c2040->bus, device->owner(), BIT(data, 3));

	/* data valid out */
	ieee488_dav_w(c2040->bus, device->owner(), BIT(data, 4));

	update_ieee_signals(device->owner());
}

static READ8_DEVICE_HANDLER( riot1_pb_r )
{
	/*

        bit     description

        PB0     DEVICE NUMBER SELECTION
        PB1     DEVICE NUMBER SELECTION
        PB2     DEVICE NUMBER SELECTION
        PB3     ACT LED 1
        PB4     ACT LED 0
        PB5     ERR LED
        PB6     DACI
        PB7     RFDI

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* device number selection */
	data |= c2040->address;

	/* data accepted in */
	data |= ieee488_ndac_r(c2040->bus) << 6;

	/* ready for data in */
	data |= ieee488_nrfd_r(c2040->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( riot1_pb_w )
{
	/*

        bit     description

        PB0     DEVICE NUMBER SELECTION
        PB1     DEVICE NUMBER SELECTION
        PB2     DEVICE NUMBER SELECTION
        PB3     ACT LED 1
        PB4     ACT LED 0
        PB5     ERR LED
        PB6     DACI
        PB7     RFDI

    */

	/* TODO activity led 1 */

	/* TODO activity led 0 */

	/* TODO error led */
}

static WRITE_LINE_DEVICE_HANDLER( riot1_irq_w )
{
	c2040_t *c2040 = get_safe_token(device->owner());

	device_set_input_line(c2040->cpu_dos, M6502_IRQ_LINE, state);
}

static const riot6532_interface riot1_intf =
{
	DEVCB_HANDLER(riot1_pa_r),
	DEVCB_HANDLER(riot1_pb_r),
	DEVCB_HANDLER(riot1_pa_w),
	DEVCB_HANDLER(riot1_pb_w),
	DEVCB_LINE(riot1_irq_w)
};

/*-------------------------------------------------
    via6522_interface via_intf um3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( via_pa_r )
{
	/*

        bit     description

        PA0     E0
        PA1     E1
        PA2     I2
        PA3     E2
        PA4     E4
        PA5     E5
        PA6     I7
        PA7     E6

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT16 i = c2040->i;
	UINT8 e = c2040->e;
	UINT8 data = (BIT(e, 6) << 7) | (BIT(i, 7) << 6) | (e & 0x33) | (BIT(e, 2) << 3) | (i & 0x04);

	return data;
}

static WRITE8_DEVICE_HANDLER( via_pb_w )
{
	/*

        bit     description

        PB0     S1A
        PB1     S1B
        PB2     S0A
        PB3     S0B
        PB4     MTR1
        PB5     MTR0
        PB6
        PB7

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	/* spindle motor 1 */
	int mtr1 = BIT(data, 4);
	spindle_motor(c2040, 1, mtr1);

	/* spindle motor 0 */
	int mtr0 = BIT(data, 5);
	spindle_motor(c2040, 0, mtr0);

	/* stepper motor 1 */
	int s1 = data & 0x03;
	micropolis_step_motor(c2040, 1, mtr1, s1);

	/* stepper motor 0 */
	int s0 = (data >> 2) & 0x03;
	micropolis_step_motor(c2040, 0, mtr0, s0);

	c2040->bit_timer->enable(!mtr1 | !mtr0);
}

static READ_LINE_DEVICE_HANDLER( ready_r )
{
	c2040_t *c2040 = get_safe_token(device->owner());

	return c2040->ready;
}

static READ_LINE_DEVICE_HANDLER( err_r )
{
	c2040_t *c2040 = get_safe_token(device->owner());

	return ERROR;
}

static WRITE_LINE_DEVICE_HANDLER( mode_sel_w )
{
	c2040_t *c2040 = get_safe_token(device->owner());

	/* mode select */
	c2040->mode = state;

	update_gcr_data(c2040);
	c2040->via->write_cb1(ERROR);
}

static WRITE_LINE_DEVICE_HANDLER( rw_sel_w )
{
	c2040_t *c2040 = get_safe_token(device->owner());

	/* read/write select */
	c2040->rw = state;

	update_gcr_data(c2040);
	c2040->via->write_cb1(ERROR);
}

static const via6522_interface via_intf =
{
	DEVCB_HANDLER(via_pa_r),
	DEVCB_NULL,
	DEVCB_LINE(ready_r),
	DEVCB_LINE(err_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(mode_sel_w),
	DEVCB_LINE(rw_sel_w),

	DEVCB_NULL
};

/*-------------------------------------------------
    via6522_interface c8050_via_intf um3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c8050_via_pb_r )
{
	/*

        bit     description

        PB0     S1A
        PB1     S1B
        PB2     S0A
        PB3     S0B
        PB4     MTR1
        PB5     MTR0
        PB6     PULL SYNC
        PB7     SYNC

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* SYNC detected */
	data |= SYNC << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c8050_via_pb_w )
{
	/*

        bit     description

        PB0     S1A
        PB1     S1B
        PB2     S0A
        PB3     S0B
        PB4     MTR1
        PB5     MTR0
        PB6     PULL SYNC
        PB7     SYNC

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	/* spindle motor 1 */
	int mtr1 = BIT(data, 4);
	spindle_motor(c2040, 1, mtr1);

	/* spindle motor 0 */
	int mtr0 = BIT(data, 5);
	spindle_motor(c2040, 0, mtr0);

	/* stepper motor 1 */
	int s1 = data & 0x03;
	mpi_step_motor(c2040, 1, mtr1, s1);

	/* stepper motor 0 */
	int s0 = (data >> 2) & 0x03;
	mpi_step_motor(c2040, 0, mtr0, s0);

	c2040->bit_timer->enable(!mtr1 | !mtr0);
}

static const via6522_interface c8050_via_intf =
{
	DEVCB_HANDLER(via_pa_r),
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
    mos6530_interface miot_intf uk3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( pi_r )
{
	/*

        bit     description

        PA0     PI0
        PA1     PI1
        PA2     PI2
        PA3     PI3
        PA4     PI4
        PA5     PI5
        PA6     PI6
        PA7     PI7

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	return c2040->pi;
}

static WRITE8_DEVICE_HANDLER( pi_w )
{
	/*

        bit     description

        PA0     PI0
        PA1     PI1
        PA2     PI2
        PA3     PI3
        PA4     PI4
        PA5     PI5
        PA6     PI6
        PA7     PI7

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	c2040->pi = data;
}

static READ8_DEVICE_HANDLER( miot_pb_r )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1     DS0
        PB2     DS1
        PB3     WPS
        PB4
        PB5
        PB6     SYNC
        PB7     M6504 IRQ

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* write protect sense */
	data |= floppy_wpt_r(c2040->unit[c2040->drive].image) << 3;

	/* SYNC detected */
	data |= SYNC << 6;

	return data;
}

static WRITE8_DEVICE_HANDLER( miot_pb_w )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1     DS0
        PB2     DS1
        PB3     WPS
        PB4
        PB5
        PB6     SYNC
        PB7     M6504 IRQ

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	/* drive select */
	c2040->drive = BIT(data, 0);

	/* density select */
	int ds = (data >> 1) & 0x03;

	if (c2040->ds != ds)
	{
		c2040->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C2040_BITRATE[ds]/4));
		c2040->ds = ds;
	}

	/* interrupt */
	if (c2040->miot_irq != BIT(data, 7))
	{
		device_set_input_line(c2040->cpu_fdc, M6502_IRQ_LINE, BIT(data, 7) ? CLEAR_LINE : ASSERT_LINE);
		c2040->miot_irq = BIT(data, 7);
	}
}

static MOS6530_INTERFACE( miot_intf )
{
	DEVCB_HANDLER(pi_r),
	DEVCB_HANDLER(pi_w),
	DEVCB_HANDLER(miot_pb_r),
	DEVCB_HANDLER(miot_pb_w)
};

/*-------------------------------------------------
    mos6530_interface c8050_miot_intf uk3
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c8050_miot_pb_r )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1     DS0
        PB2     DS1
        PB3     WPS
        PB4     DRIVE TYPE (0=2A, 1=2C)
        PB5
        PB6     (0=DS, 1=SS)
        PB7     M6504 IRQ

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* write protect sense */
	data |= floppy_wpt_r(c2040->unit[c2040->drive].image) << 3;

	/* drive type */
	data |= 0x10;

	/* single/dual sided */
	if (device->owner()->type() == C8050)
	{
		data |= 0x40;
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( c8050_miot_pb_w )
{
	/*

        bit     description

        PB0     DRV SEL
        PB1     DS0
        PB2     DS1
        PB3     WPS
        PB4     ODD HD (0=78-154, 1=1-77)
        PB5
        PB6     (0=DS, 1=SS)
        PB7     M6504 IRQ

    */

	c2040_t *c2040 = get_safe_token(device->owner());

	/* drive select */
	if ((device->owner()->type() == C8050) || (device->owner()->type() == C8250))
	{
		c2040->drive = BIT(data, 0);
	}

	/* density select */
	int ds = (data >> 1) & 0x03;

	if (c2040->ds != ds)
	{
		c2040->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C8050_BITRATE[ds]));
		c2040->ds = ds;
	}

	/* side select */
	if ((device->owner()->type() == C8250) || (device->owner()->type() == SFD1001))
	{
		c2040->side = !BIT(data, 4);
	}

	/* interrupt */
	if (c2040->miot_irq != BIT(data, 7))
	{
		device_set_input_line(c2040->cpu_fdc, M6502_IRQ_LINE, BIT(data, 7) ? CLEAR_LINE : ASSERT_LINE);
		c2040->miot_irq = BIT(data, 7);
	}
}

static MOS6530_INTERFACE( c8050_miot_intf )
{
	DEVCB_HANDLER(pi_r),
	DEVCB_HANDLER(pi_w),
	DEVCB_HANDLER(c8050_miot_pb_r),
	DEVCB_HANDLER(c8050_miot_pb_w)
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c2040 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c2040 )
	FLOPPY_OPTION( c2040, "d67", "Commodore 2040/3040 Disk Image", d67_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c2040, "g64", "Commodore 2040/3040 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
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
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c2040),
	NULL
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
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c4040),
	NULL
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
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c8050),
	NULL
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
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(c8250),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c2040 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c2040 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c2040_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c2040_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, via_intf)
	MCFG_MOS6530_ADD(M6530_TAG, XTAL_16MHz/16, miot_intf)

	MCFG_FLOPPY_2_DRIVES_ADD(c2040_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c4040 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c4040 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c4040_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_16MHz/16, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_16MHz/16, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c4040_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, via_intf)
	MCFG_MOS6530_ADD(M6530_TAG, XTAL_16MHz/16, miot_intf)

	MCFG_FLOPPY_2_DRIVES_ADD(c4040_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c8050 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c8050 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(c8050_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/12, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/12, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(c8050_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_12MHz/12, c8050_via_intf)
	MCFG_MOS6530_ADD(M6530_TAG, XTAL_12MHz/12, c8050_miot_intf)

	MCFG_FLOPPY_2_DRIVES_ADD(c8050_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c8250 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c8250 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(c8050_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/12, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/12, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(c8050_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_12MHz/12, c8050_via_intf)
	MCFG_MOS6530_ADD(M6530_TAG, XTAL_12MHz/12, c8050_miot_intf)

	MCFG_FLOPPY_2_DRIVES_ADD(c8250_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( sfd1001 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( sfd1001 )
	/* DOS */
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(sfd1001_dos_map)

	MCFG_RIOT6532_ADD(M6532_0_TAG, XTAL_12MHz/12, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, XTAL_12MHz/12, riot1_intf)

	/* controller */
	MCFG_CPU_ADD(M6504_TAG, M6504, XTAL_12MHz/12)
	MCFG_CPU_PROGRAM_MAP(sfd1001_fdc_map)

	MCFG_VIA6522_ADD(M6522_TAG, XTAL_12MHz/12, c8050_via_intf)
	MCFG_MOS6530_ADD(M6530_TAG, XTAL_12MHz/12, c8050_miot_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c8250_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c2040 )
-------------------------------------------------*/

ROM_START( c2040 ) // schematic 320806
	ROM_REGION( 0x2c00, C2040_REGION, ROMREGION_LOADBYNAME )
	/* DOS 1.0 */
	ROM_LOAD( "901468-06.ul1", 0x0000, 0x1000, CRC(25b5eed5) SHA1(4d9658f2e6ff3276e5c6e224611a66ce44b16fc7) )
	ROM_LOAD( "901468-07.uh1", 0x1000, 0x1000, CRC(9b09ae83) SHA1(6a51c7954938439ca8342fc295bda050c06e1791) )

	/* RIOT DOS 1 */
	ROM_LOAD( "901466-02.uk3", 0x2000, 0x0400, BAD_DUMP /* parsed in from disassembly */ CRC(e1c86c43) SHA1(d8209c66fde3f2937688ba934ba968678a9d2ebb) )

	/* ROM GCR */
	ROM_LOAD( "901467.uk6",    0x2400, 0x0800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( c4040 )
-------------------------------------------------*/

ROM_START( c4040 ) // schematic ?
	ROM_REGION( 0x3c00, C4040_REGION, ROMREGION_LOADBYNAME )
	/* DOS 2.1 */
	ROM_LOAD_OPTIONAL( "901468-11.uj1", 0x0000, 0x1000, CRC(b7157458) SHA1(8415f3159dea73161e0cef7960afa6c76953b6f8) )
	ROM_LOAD_OPTIONAL( "901468-12.ul1", 0x1000, 0x1000, CRC(02c44ff9) SHA1(e8a94f239082d45f64f01b2d8e488d18fe659cbb) )
	ROM_LOAD_OPTIONAL( "901468-13.uh1", 0x2000, 0x1000, CRC(cbd785b3) SHA1(6ada7904ac9d13c3f1c0a8715f9c4be1aa6eb0bb) )
	/* DOS 2 rev */
	ROM_LOAD( "901468-14.uj1", 0x0000, 0x1000, CRC(bc4d4872) SHA1(ffb992b82ec913ddff7be964d7527aca3e21580c) )
	ROM_LOAD( "901468-15.ul1", 0x1000, 0x1000, CRC(b6970533) SHA1(f702d6917fe8a798740ba4d467b500944ae7b70a) )
	ROM_LOAD( "901468-16.uh1", 0x2000, 0x1000, CRC(1f5eefb7) SHA1(04b918cf4adeee8015b43383d3cea7288a7d0aa8) )

	/* RIOT DOS 2 */
	ROM_LOAD( "901466-04.uk3", 0x3000, 0x0400, CRC(0ab338dc) SHA1(6645fa40b81be1ff7d1384e9b52df06a26ab0bfb) )

	/* ROM GCR */
	ROM_LOAD( "901467.uk6",    0x3400, 0x0800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( c8050 )
-------------------------------------------------*/

/*

    DOS/CONTROLLER ROMS FOR DIGITAL PCB #8050002

    DESCRIPTION     PCB PART NO.        UL1         UH1         UK3

    2.5 Micropolis  8050002-01      901482-07   901482-06   901483-03
    2.5 Tandon      8050002-02      901482-07   901482-06   901483-04
    2.7 Tandon      8050002-03      901887-01   901888-01   901884-01
    2.7 Micropolis  8050002-04      901887-01   901888-01   901885-04
    2.7 MPI 8050    8050002-05      901887-01   901888-01   901869-01
    2.7 MPI 8250    8050002-06      901887-01   901888-01   901869-01

*/

ROM_START( c8050 ) // schematic 8050001
	ROM_REGION( 0x4c00, C8050_REGION, ROMREGION_LOADBYNAME )
	/* 2364 ROM DOS 2.5 */
	ROM_LOAD_OPTIONAL( "901482-03.ul1", 0x0000, 0x2000, CRC(09a609b9) SHA1(166d8bfaaa9c4767f9b17ad63fc7ae77c199a64e) )
	ROM_LOAD_OPTIONAL( "901482-04.uh1", 0x2000, 0x2000, CRC(1bcf9df9) SHA1(217f4a8b348658bb365f4a1de21ecbaa6402b1c0) )
	/* 2364-091 ROM DOS 2.5 rev */
	ROM_LOAD_OPTIONAL( "901482-06.ul1", 0x0000, 0x2000, CRC(3cbd2756) SHA1(7f5fbed0cddb95138dd99b8fe84fddab900e3650) )
	ROM_LOAD_OPTIONAL( "901482-07.uh1", 0x2000, 0x2000, CRC(c7532d90) SHA1(0b6d1e55afea612516df5f07f4a6dccd3bd73963) )
	/* 2364 ROM DOS 2.7 */
	ROM_LOAD( "901887-01.ul1", 0x0000, 0x2000, CRC(0073b8b2) SHA1(b10603195f240118fe5fb6c6dfe5c5097463d890) )
	ROM_LOAD( "901888-01.uh1", 0x2000, 0x2000, CRC(de9b6132) SHA1(2e6c2d7ca934e5c550ad14bd5e9e7749686b7af4) )

	/* controller */
	ROM_LOAD_OPTIONAL( "901483-03.uk3", 0x4000, 0x0400, CRC(9e83fa70) SHA1(e367ea8a5ddbd47f13570088427293138a10784b) )
	ROM_LOAD_OPTIONAL( "901483-04.uk3", 0x4000, 0x0400, NO_DUMP )
	ROM_LOAD_OPTIONAL( "901884-01.uk3", 0x4000, 0x0400, NO_DUMP )
	ROM_LOAD_OPTIONAL( "901885-04.uk3", 0x4000, 0x0400, CRC(bab998c9) SHA1(0dc9a3b60f1b866c63eebd882403532fc59fe57f) )
	ROM_LOAD( "901869-01.uk3", 0x4000, 0x0400, CRC(2915327a) SHA1(3a9a80f72ce76e5f5c72513f8ef7553212912ae3) )

	/* GCR decoder */
	ROM_LOAD( "901467.uk6",    0x4400, 0x0800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
ROM_END

/*-------------------------------------------------
    ROM( sfd1001 )
-------------------------------------------------*/

ROM_START( sfd1001 ) // schematic 251406
	ROM_REGION( 0x5000, SFD1001_REGION, ROMREGION_LOADBYNAME )
	ROM_LOAD( "901887-01.1j",  0x0000, 0x2000, CRC(0073b8b2) SHA1(b10603195f240118fe5fb6c6dfe5c5097463d890) )
	ROM_LOAD( "901888-01.3j",  0x2000, 0x2000, CRC(de9b6132) SHA1(2e6c2d7ca934e5c550ad14bd5e9e7749686b7af4) )

	/* controller */
	ROM_LOAD( "251257-02a.u2", 0x4000, 0x0800, CRC(b51150de) SHA1(3b954eb34f7ea088eed1d33ebc6d6e83a3e9be15) )

	/* GCR decoder */
	ROM_LOAD( "901467.5c",     0x4800, 0x0800, CRC(a23337eb) SHA1(97df576397608455616331f8e837cb3404363fa2) )
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
	c2040->cpu_dos = device->subdevice(M6502_TAG);
	c2040->cpu_fdc = device->subdevice(M6504_TAG);

	/* find devices */
	c2040->riot0 = device->subdevice(M6532_0_TAG);
	c2040->riot1 = device->subdevice(M6532_1_TAG);
	c2040->miot = device->subdevice(M6530_TAG);
	c2040->via = device->subdevice<via6522_device>(M6522_TAG);
	c2040->bus = device->machine().device(config->bus_tag);
	c2040->unit[0].image = device->subdevice(FLOPPY_0);
	c2040->unit[1].image = device->subdevice(FLOPPY_1);

	/* install image callbacks */
	floppy_install_unload_proc(c2040->unit[0].image, on_disk_0_change);
	floppy_install_load_proc(c2040->unit[0].image, on_disk_0_change);
	floppy_install_unload_proc(c2040->unit[1].image, on_disk_1_change);
	floppy_install_load_proc(c2040->unit[1].image, on_disk_1_change);

	/* find GCR ROM */
	const memory_region *region = device->subregion(C4040_REGION);

	if ((device->type() == C2040) || (device->type() == C3040))
	{
		region = device->subregion(C2040_REGION);
	}
	else if (device->type() == C4040)
	{
		region = device->subregion(C4040_REGION);
	}
	else if ((device->type() == C8050) || (device->type() == C8250))
	{
		region = device->subregion(C8050_REGION);
	}
	else if (device->type() == SFD1001)
	{
		region = device->subregion(SFD1001_REGION);
	}

	c2040->gcr = region->base() + region->bytes() - 0x800;

	/* allocate data timer */
	c2040->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_tick), (void *)device);

	/* register for state saving */
	device->save_item(NAME(c2040->drive));
	device->save_item(NAME(c2040->side));
	device->save_item(NAME(c2040->address));
	device->save_item(NAME(c2040->rfdo));
	device->save_item(NAME(c2040->daco));
	device->save_item(NAME(c2040->atna));
	device->save_item(NAME(c2040->ds));
	device->save_item(NAME(c2040->bit_count));
	device->save_item(NAME(c2040->sr));
	device->save_item(NAME(c2040->pi));
	device->save_item(NAME(c2040->i));
	device->save_item(NAME(c2040->e));
	device->save_item(NAME(c2040->ready));
	device->save_item(NAME(c2040->mode));
	device->save_item(NAME(c2040->rw));
	device->save_item(NAME(c2040->miot_irq));
	device->save_item(NAME(c2040->unit[0].stp));
	device->save_item(NAME(c2040->unit[0].mtr));
	device->save_item(NAME(c2040->unit[0].track_len));
	device->save_item(NAME(c2040->unit[0].buffer_pos));
	device->save_item(NAME(c2040->unit[0].bit_pos));
	device->save_item(NAME(c2040->unit[1].stp));
	device->save_item(NAME(c2040->unit[1].mtr));
	device->save_item(NAME(c2040->unit[1].track_len));
	device->save_item(NAME(c2040->unit[1].buffer_pos));
	device->save_item(NAME(c2040->unit[1].bit_pos));
}

/*-------------------------------------------------
    DEVICE_RESET( c2040 )
-------------------------------------------------*/

static DEVICE_RESET( c2040 )
{
	c2040_t *c2040 = get_safe_token(device);

	/* reset devices */
	c2040->cpu_dos->reset();
	c2040->cpu_fdc->reset();
	c2040->riot0->reset();
	c2040->riot1->reset();
	c2040->miot->reset();
	c2040->via->reset();

	/* toggle M6502 SO */
	device_set_input_line(c2040->cpu_dos, M6502_SET_OVERFLOW, ASSERT_LINE);
	device_set_input_line(c2040->cpu_dos, M6502_SET_OVERFLOW, CLEAR_LINE);

	/* turn off spindle motors */
	c2040->unit[0].mtr = c2040->unit[1].mtr = 1;
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

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c2040);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c2040);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c2040);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c2040);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 2040");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c2040");									break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore PET");							break;
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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c2040);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 3040");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c3040");									break;

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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c4040);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 4040");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c4040");									break;

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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c8050);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 8050");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c8050");									break;		

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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c8250);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 8250");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c8250");									break;
	
		default:										DEVICE_GET_INFO_CALL(c2040);								break;
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
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(sfd1001);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore SFD-1001");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "sfd1001");									break;

		default:										DEVICE_GET_INFO_CALL(c2040);								break;
	}
}

DEFINE_LEGACY_DEVICE(C2040, c2040);
DEFINE_LEGACY_DEVICE(C3040, c3040);
DEFINE_LEGACY_DEVICE(C4040, c4040);
DEFINE_LEGACY_DEVICE(C8050, c8050);
DEFINE_LEGACY_DEVICE(C8250, c8250);
DEFINE_LEGACY_DEVICE(SFD1001, sfd1001);
