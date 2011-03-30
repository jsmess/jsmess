/**********************************************************************

    Commodore 1540/1541/1541C/1541-II/2031 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - some copy protections fail
    - more accurate timing
    - power/activity LEDs

*/

/*

    1540/1541/1541A/SX-64 Parts

    Location       Part Number    Description
                                  2016 2K x 8 bit Static RAM (short board)
    UA2-UB3                       2114 (4) 1K x 4 bit Static RAM (long board)
                   325572-01      64H105 40 pin Gate Array (short board)
                   325302-01      2364-130 ROM DOS 2.6 C000-DFFF
                   325303-01      2364-131 ROM DOS 2.6 (1540) E000-FFFF
                   901229-01      2364-173 ROM DOS 2.6 rev. 0 E000-FFFF
                   901229-03      2364-197 ROM DOS 2.6 rev. 1 E000-FFFF
                   901229-05      8K ROM DOS 2.6 rev. 2 E000-FFFF
                                  6502 CPU
                                  6522 (2) VIA
    drive                         Alps DFB111M25A
    drive                         Alps FDM2111-B2
    drive                         Newtronics D500

    1541B/1541C Parts

    Location       Part Number    Description
    UA3                           2016 2K x 8 bit Static RAM
    UC2                           6502 CPU
    UC1, UC3                      6522 (2) CIA
    UC4            251828-02      64H156 42 pin Gate Array
    UC5            251829-01      64H157 20 pin Gate Array
    UD1          * 251853-01      R/W Hybrid
    UD1          * 251853-02      R/W Hybrid
    UA2            251968-01      27128 EPROM DOS 2.6 rev. 3 C000-FFFF
    drive                         Newtronics D500
      * Not interchangeable.

    1541-II Parts

    Location       Part Number    Description
    U5                            2016-15 2K x 8 bit Static RAM
    U12                           SONY CX20185 R/W Amp.
    U3                            6502A CPU
    U6, U8                        6522 (2) CIA
    U10            251828-01      64H156 40 pin Gate Array
    U4             251968-03      16K ROM DOS 2.6 rev. 4 C000-FFFF
    drive                         Chinon FZ-501M REV A
    drive                         Digital System DS-50F
    drive                         Newtronics D500
    drive                         Safronic DS-50F

    ...

    PCB Assy # 1540008-01
    Schematic # 1540001
    Original "Long" Board
    Has 4 discreet 2114 RAMs
    ALPS Drive only

    PCB Assy # 1540048
    Schematic # 1540049
    Referred to as the CR board
    Changed to 2048 x 8 bit RAM pkg.
    A 40 pin Gate Array is used
    Alps Drive (-01)
    Newtronics Drive (-03)

    PCB Assy # 250442-01
    Schematic # 251748
    Termed the 1541 A
    Just one jumper change to accommodate both types of drive

    PCB Assy # 250446-01
    Schematic # 251748 (See Notes)
    Termed the 1541 A-2
    Just one jumper change to accommodate both types of drive

    ...

    VIC1541 1540001-01   Very early version, long board.
            1540001-03   As above, only the ROMs are different.
            1540008-01

    1541    1540048-01   Shorter board with a 40 pin gate array, Alps mech.
            1540048-03   As above, but Newtronics mech.
            1540049      Similar to above
            1540050      Similar to above, Alps mechanism.

    SX64    250410-01    Design most similar to 1540048-01, Alps mechanism.

    1541    251777       Function of bridge rects. reversed, Newtronics mech.
            251830       Same as above

    1541A   250442-01    Alps or Newtronics drive selected by a jumper.
    1541A2  250446-01    A 74LS123 replaces the 9602 at UD4.
    1541B   250448-01    Same as the 1541C, but in a case like the 1541.
    1541C   250448-01    Short board, new 40/42 pin gate array, 20 pin gate
                         array and a R/W hybrid chip replace many components.
                         Uses a Newtronics drive with optical trk 0 sensor.
    1541C   251854       As above, single DOS ROM IC, trk 0 sensor, 30 pin
                         IC for R/W ampl & stepper motor control (like 1571).

    1541-II              A complete redesign using the 40 pin gate array
                         from the 1451C and a Sony R/W hybrid, but not the
                         20 pin gate array, single DOS ROM IC.

    NOTE: These system boards are the 60 Hz versions.
          The -02 and -04 boards are probably the 50 Hz versions.

    The ROMs appear to be completely interchangeable. For instance, the
    first version of ROM for the 1541-II contained the same code as the
    last version of the 1541. I copy the last version of the 1541-II ROM
    into two 68764 EPROMs and use them in my original 1541 (long board).
    Not only do they work, but they work better than the originals.


    http://www.amiga-stuff.com/hardware/cbmboards.html

*/

#include "emu.h"
#include "c1541.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define M6502_TAG		"ucd5"
#define M6522_0_TAG		"uab1"
#define M6522_1_TAG		"ucd4"
#define TIMER_BIT_TAG	"ue7"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1541_t c1541_t;
struct _c1541_t
{
	/* IEC bus */
	int address;							/* device address - 8 */
	int atna;								/* attention acknowledge */
	int data_out;							/* serial bus data output */

	/* IEEE-488 bus */
	int nrfd_out;							/* not ready for data */
	int ndac_out;							/* not data accepted */

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

	/* interrupts */
	int via0_irq;							/* VIA #0 interrupt request */
	int via1_irq;							/* VIA #1 interrupt request */

	/* devices */
	device_t *cpu;
	via6522_device *via0;
	via6522_device *via1;
	device_t *bus;
	device_t *image;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1541_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1540) || (device->type() == C1541) || (device->type() == C1541C) || (device->type() == C1541II) ||
		(device->type() == SX1541) || (device->type() == OC118) || (device->type() == C2031));
	return (c1541_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE c1541_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == C1540) || (device->type() == C1541) || (device->type() == C1541C) || (device->type() == C1541II) ||
		(device->type() == SX1541) || (device->type() == OC118) || (device->type() == C2031));
	return (c1541_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
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
	c1541_t *c1541 = get_safe_token(device);
	int byte = 0;

	/* shift in data from the read head */
	c1541->data <<= 1;
	c1541->data |= BIT(c1541->track_buffer[c1541->buffer_pos], c1541->bit_pos);
	c1541->bit_pos--;
	c1541->bit_count++;

	if (c1541->bit_pos < 0)
	{
		c1541->bit_pos = 7;
		c1541->buffer_pos++;

		if (c1541->buffer_pos >= c1541->track_len)
		{
			/* loop to the start of the track */
			c1541->buffer_pos = 0;
		}
	}

	if ((c1541->data & G64_SYNC_MARK) == G64_SYNC_MARK)
	{
		/* SYNC detected */
		c1541->bit_count = 0;
	}

	if (c1541->bit_count > 7)
	{
		/* byte ready */
		c1541->bit_count = 0;
		byte = 1;

		if (!(c1541->data & 0xff))
		{
			/* simulate weak bits with randomness */
			c1541->data = (c1541->data & 0xff00) | (machine.rand() & 0xff);
		}
	}

	if (c1541->byte != byte)
	{
		int byte_ready = !(byte && c1541->soe);

		device_set_input_line(c1541->cpu, M6502_SET_OVERFLOW, byte_ready);
		c1541->via1->write_ca1(byte_ready);

		c1541->byte = byte;
	}
}

/*-------------------------------------------------
    read_current_track - read track from disk
-------------------------------------------------*/

static void read_current_track(c1541_t *c1541)
{
	c1541->track_len = G64_BUFFER_SIZE;
	c1541->buffer_pos = 0;
	c1541->bit_pos = 7;
	c1541->bit_count = 0;

	/* read track data */
	floppy_drive_read_track_data_info_buffer(c1541->image, 0, c1541->track_buffer, &c1541->track_len);

	/* get track length */
	c1541->track_len = floppy_drive_get_current_track_size(c1541->image, 0);
}

/*-------------------------------------------------
    on_disk_change - disk change handler
-------------------------------------------------*/

static void on_disk_change(device_image_interface &image)
{
	c1541_t *c1541 = get_safe_token(image.device().owner());

	read_current_track(c1541);
}

/*-------------------------------------------------
    spindle_motor - spindle motor control
-------------------------------------------------*/

static void spindle_motor(c1541_t *c1541, int mtr)
{
	if (c1541->mtr != mtr)
	{
		if (mtr)
		{
			/* read track data */
			read_current_track(c1541);
		}

		floppy_mon_w(c1541->image, !mtr);
		c1541->bit_timer->enable(mtr);

		c1541->mtr = mtr;
	}
}

/*-------------------------------------------------
    step_motor - stepper motor control
-------------------------------------------------*/

static void step_motor(c1541_t *c1541, int mtr, int stp)
{
	if (mtr & (c1541->stp != stp))
	{
		int tracks = 0;

		switch (c1541->stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 3) tracks--; break;
		case 1:	if (stp == 2) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 3) tracks++; else if (stp == 1) tracks--; break;
		case 3: if (stp == 0) tracks++; else if (stp == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			/* step read/write head */
			floppy_drive_seek(c1541->image, tracks);

			/* read new track data */
			read_current_track(c1541);
		}

		c1541->stp = stp;
	}
}

/*-------------------------------------------------
    c1541_iec_atn_w - serial bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1541_iec_atn_w )
{
	c1541_t *c1541 = get_safe_token(device);
	int data_out = !c1541->data_out && !(c1541->atna ^ !state);

	c1541->via0->write_ca1(!state);

	cbm_iec_data_w(c1541->bus, device, data_out);
}

/*-------------------------------------------------
    c1541_iec_reset_w - serial bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1541_iec_reset_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    c2031_ieee488_atn_w - IEEE-488 bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_atn_w )
{
	c1541_t *c1541 = get_safe_token(device);
	int nrfd = c1541->nrfd_out;
	int ndac = c1541->ndac_out;

	c1541->via0->write_ca1(!state);

	if (!state ^ c1541->atna)
	{
		nrfd = ndac = 0;
	}

	ieee488_nrfd_w(c1541->bus, device, nrfd);
	ieee488_ndac_w(c1541->bus, device, ndac);
}

/*-------------------------------------------------
    c2031_ieee488_ifc_w - IEEE-488 bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c2031_ieee488_ifc_w )
{
	if (!state)
	{
		device->reset();
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1540_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1540_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c1540:c1540", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1541_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1541_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c1541:c1541", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1541c_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1541c_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c1541:c1541c", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1541ii_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1541ii_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c1541:c1541ii", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sx1541_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sx1541_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c1541:sx1541", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c2031_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c2031_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("c2031:c2031", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( oc118_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( oc118_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_0_TAG, via6522_device, read, write)
	AM_RANGE(0x1c00, 0x1c0f) AM_MIRROR(0x63f0) AM_DEVREADWRITE_MODERN(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0x8000, 0xbfff) AM_MIRROR(0x4000) AM_ROM AM_REGION("oc118:oc118", 0x0000)
ADDRESS_MAP_END

/*-------------------------------------------------
    via6522_interface c1541_via0_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via0_irq_w )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	c1541->via0_irq = state;

	device_set_input_line(c1541->cpu, INPUT_LINE_IRQ0, (c1541->via0_irq | c1541->via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( via0_pa_r )
{
	/* dummy read to acknowledge ATN IN interrupt */
	return 0;
}

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATNA
        PB5     J1
        PB6     J2
        PB7     ATN IN

    */

	c1541_t *c1541 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* data in */
	data = !cbm_iec_data_r(c1541->bus);

	/* clock in */
	data |= !cbm_iec_clk_r(c1541->bus) << 2;

	/* serial bus address */
	data |= c1541->address << 5;

	/* attention in */
	data |= !cbm_iec_atn_r(c1541->bus) << 7;

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
        PB4     ATNA
        PB5     J1
        PB6     J2
        PB7     ATN IN

    */

	c1541_t *c1541 = get_safe_token(device->owner());

	int data_out = BIT(data, 1);
	int clk_out = BIT(data, 3);
	int atna = BIT(data, 4);

	/* data out */
	int serial_data = !data_out && !(atna ^ !cbm_iec_atn_r(c1541->bus));
	cbm_iec_data_w(c1541->bus, device->owner(), serial_data);
	c1541->data_out = data_out;

	/* clock out */
	cbm_iec_clk_w(c1541->bus, device->owner(), !clk_out);

	/* attention acknowledge */
	c1541->atna = atna;
}

static READ_LINE_DEVICE_HANDLER( atn_in_r )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	return !cbm_iec_atn_r(c1541->bus);
}

static const via6522_interface c1541_via0_intf =
{
	DEVCB_HANDLER(via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_LINE(atn_in_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface c1541c_via0_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c1541c_via0_pa_r )
{
	/*

        bit     description

        PA0     TR00 SENCE
        PA1
        PA2
        PA3
        PA4
        PA5
        PA6
        PA7

    */

	c1541_t *c1541 = get_safe_token(device->owner());

	return !floppy_tk00_r(c1541->image);
}

static const via6522_interface c1541c_via0_intf =
{
	DEVCB_HANDLER(c1541c_via0_pa_r),
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_LINE(atn_in_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface c2031_via0_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c2031_via0_pa_r )
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

	c1541_t *c1541 = get_safe_token(device->owner());

	return ieee488_dio_r(c1541->bus, 0);
}

static WRITE8_DEVICE_HANDLER( c2031_via0_pa_w )
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

	c1541_t *c1541 = get_safe_token(device->owner());

	ieee488_dio_w(c1541->bus, device->owner(), data);
}

static READ8_DEVICE_HANDLER( c2031_via0_pb_r )
{
	/*

        bit     description

        PB0     ATNA
        PB1     NRFD
        PB2     NDAC
        PB3     EOI
        PB4     T/_R
        PB5     HD SEL
        PB6     DAV
        PB7     _ATN

    */

	c1541_t *c1541 = get_safe_token(device->owner());

	UINT8 data = 0;

	/* not ready for data */
	data |= ieee488_nrfd_r(c1541->bus) << 1;

	/* not data accepted */
	data |= ieee488_ndac_r(c1541->bus) << 2;

	/* end or identify */
	data |= ieee488_eoi_r(c1541->bus) << 3;

	/* data valid */
	data |= ieee488_dav_r(c1541->bus) << 6;

	/* attention */
	data |= !ieee488_atn_r(c1541->bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c2031_via0_pb_w )
{
	/*

        bit     description

        PB0     ATNA
        PB1     NRFD
        PB2     NDAC
        PB3     EOI
        PB4     T/_R
        PB5     HD SEL
        PB6     DAV
        PB7     _ATN

    */

	c1541_t *c1541 = get_safe_token(device->owner());

	int atna = BIT(data, 0);
	int nrfd = BIT(data, 1);
	int ndac = BIT(data, 2);

	/* not ready for data */
	c1541->nrfd_out = nrfd;

	/* not data accepted */
	c1541->ndac_out = ndac;

	/* end or identify */
	ieee488_eoi_w(c1541->bus, device->owner(), BIT(data, 3));

	/* data valid */
	ieee488_dav_w(c1541->bus, device->owner(), BIT(data, 6));

	/* attention acknowledge */
	c1541->atna = atna;

	if (!ieee488_atn_r(c1541->bus) ^ atna)
	{
		nrfd = ndac = 0;
	}

	ieee488_nrfd_w(c1541->bus, device->owner(), nrfd);
	ieee488_ndac_w(c1541->bus, device->owner(), ndac);
}

static READ_LINE_DEVICE_HANDLER( c2031_via0_ca1_r )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	return !ieee488_atn_r(c1541->bus);
}

static READ_LINE_DEVICE_HANDLER( c2031_via0_ca2_r )
{
	c1541_t *c1541 = get_safe_token(device->owner());
	int state = 0;

	/* device # selection */
	switch (c1541->address)
	{
	case 0: state = (c1541->atna | c1541->nrfd_out);	break;
	case 1: state = c1541->atna;						break;
	case 2: state = c1541->nrfd_out;					break;
	case 3: state = 0;									break;
	}

	return state;
}

static const via6522_interface c2031_via0_intf =
{
	DEVCB_HANDLER(c2031_via0_pa_r),
	DEVCB_HANDLER(c2031_via0_pb_r),
	DEVCB_LINE(c2031_via0_ca1_r),
	DEVCB_NULL,
	DEVCB_LINE(c2031_via0_ca2_r),
	DEVCB_NULL,

	DEVCB_HANDLER(c2031_via0_pa_w),
	DEVCB_HANDLER(c2031_via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL, /* PLL SYN */

	DEVCB_LINE(via0_irq_w)
};

/*-------------------------------------------------
    via6522_interface c1541_via1_intf
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( via1_irq_w )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	c1541->via1_irq = state;

	device_set_input_line(c1541->cpu, INPUT_LINE_IRQ0, (c1541->via0_irq | c1541->via1_irq) ? ASSERT_LINE : CLEAR_LINE);
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

	c1541_t *c1541 = get_safe_token(device->owner());

	return c1541->data & 0xff;
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

	c1541_t *c1541 = get_safe_token(device->owner());

	c1541->yb = data;
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

        bit     signal      description

        PB0     STP0        stepping motor bit 0
        PB1     STP1        stepping motor bit 1
        PB2     MTR         motor ON/OFF
        PB3     ACT         drive 0 LED
        PB4     WPS         write protect sense
        PB5     DS0         density select 0
        PB6     DS1         density select 1
        PB7     SYNC        SYNC detect line

    */

	c1541_t *c1541 = get_safe_token(device->owner());
	UINT8 data = 0;

	/* write protect sense */
	data |= !floppy_wpt_r(c1541->image) << 4;

	/* SYNC detect line */
	data |= !(c1541->mode && ((c1541->data & G64_SYNC_MARK) == G64_SYNC_MARK)) << 7;

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
        PB4     WPS         write protect sense
        PB5     DS0         density select 0
        PB6     DS1         density select 1
        PB7     SYNC        SYNC detect line

    */

	c1541_t *c1541 = get_safe_token(device->owner());

	/* spindle motor */
	int mtr = BIT(data, 2);
	spindle_motor(c1541, mtr);

	/* stepper motor */
	int stp = data & 0x03;
	step_motor(c1541, mtr, stp);

	/* activity LED */

	/* density select */
	int ds = (data >> 5) & 0x03;

	if (c1541->ds != ds)
	{
		c1541->bit_timer->adjust(attotime::zero, 0, attotime::from_hz(C2040_BITRATE[ds]/4));
		c1541->ds = ds;
	}
}

static READ_LINE_DEVICE_HANDLER( byte_ready_r )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	return !(c1541->byte && c1541->soe);
}

static WRITE_LINE_DEVICE_HANDLER( soe_w )
{
	c1541_t *c1541 = get_safe_token(device->owner());
	int byte_ready = !(state && c1541->byte);

	c1541->soe = state;

	device_set_input_line(c1541->cpu, M6502_SET_OVERFLOW, byte_ready);
	c1541->via1->write_ca1(byte_ready);
}

static WRITE_LINE_DEVICE_HANDLER( mode_w )
{
	c1541_t *c1541 = get_safe_token(device->owner());

	c1541->mode = state;
}

static const via6522_interface c1541_via1_intf =
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
    FLOPPY_OPTIONS( c1541 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1541 )
	FLOPPY_OPTION( c1541, "g64", "Commodore 1541 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
	FLOPPY_OPTION( c1541, "d64", "Commodore 1541 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1541_floppy_config
-------------------------------------------------*/

static const floppy_config c1541_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c1541),
	NULL
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1540 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1540 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1540_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c1541_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1541_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1541 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1541 )
	MCFG_FRAGMENT_ADD(c1540)

	MCFG_CPU_MODIFY(M6502_TAG)
	MCFG_CPU_PROGRAM_MAP(c1541_map)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1541c )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1541c )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c1541c_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c1541c_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1541_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1541ii )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c1541ii )
	MCFG_FRAGMENT_ADD(c1540)

	MCFG_CPU_MODIFY(M6502_TAG)
	MCFG_CPU_PROGRAM_MAP(c1541ii_map)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( sx1541 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( sx1541 )
	MCFG_FRAGMENT_ADD(c1540)

	MCFG_CPU_MODIFY(M6502_TAG)
	MCFG_CPU_PROGRAM_MAP(sx1541_map)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( c2031 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( c2031 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(c2031_map)

	MCFG_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c2031_via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1541_floppy_config)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( oc118 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( oc118 )
	MCFG_FRAGMENT_ADD(c1540)

	MCFG_CPU_MODIFY(M6502_TAG)
	MCFG_CPU_PROGRAM_MAP(oc118_map)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( c1540 )
-------------------------------------------------*/

ROM_START( c1540 ) // schematic 1540008
	ROM_REGION( 0x4000, "c1540", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.uab4", 0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "325303-01.uab5", 0x2000, 0x2000, CRC(10b39158) SHA1(56dfe79b26f50af4e83fd9604857756d196516b9) )
ROM_END

/*-------------------------------------------------
    ROM( c1541 )
-------------------------------------------------*/

ROM_START( c1541 ) // schematic 1540008
	ROM_REGION( 0x6000, "c1541", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.uab4", 0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD_OPTIONAL( "901229-01.uab5", 0x2000, 0x2000, CRC(9a48d3f0) SHA1(7a1054c6156b51c25410caec0f609efb079d3a77) )
	ROM_LOAD_OPTIONAL( "901229-02.uab5", 0x2000, 0x2000, CRC(b29bab75) SHA1(91321142e226168b1139c30c83896933f317d000) )
	ROM_LOAD_OPTIONAL( "901229-03.uab5", 0x2000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744) )
	ROM_LOAD_OPTIONAL( "901229-04.uab5", 0x2000, 0x2000, NO_DUMP )
	ROM_LOAD_OPTIONAL( "901229-05 ae.uab5", 0x2000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
	ROM_LOAD( "901229-06 aa.uab5", 0x2000, 0x2000, CRC(3a235039) SHA1(c7f94f4f51d6de4cdc21ecbb7e57bb209f0530c0) )
	ROM_LOAD_OPTIONAL( "jiffydos 1541.uab5", 0x4000, 0x2000, CRC(bc7e4aeb) SHA1(db6cfaa6d9b78d58746c811de29f3b1f44d99ddf) )
ROM_END

/*-------------------------------------------------
    ROM( c1541c )
-------------------------------------------------*/

ROM_START( c1541c ) // schematic ?
	ROM_REGION( 0x4000, "c1541c", ROMREGION_LOADBYNAME )
	ROM_LOAD_OPTIONAL( "251968-01.ua2", 0x0000, 0x4000, CRC(1b3ca08d) SHA1(8e893932de8cce244117fcea4c46b7c39c6a7765) )
	ROM_LOAD( "251968-02.ua2", 0x0000, 0x4000, CRC(2d862d20) SHA1(38a7a489c7bbc8661cf63476bf1eb07b38b1c704) )
ROM_END

/*-------------------------------------------------
    ROM( c1541ii )
-------------------------------------------------*/

ROM_START( c1541ii ) // schematic 340503
	ROM_REGION( 0x8000, "c1541ii", ROMREGION_LOADBYNAME )
	ROM_LOAD_OPTIONAL( "251968-03.u4", 0x0000, 0x4000, CRC(899fa3c5) SHA1(d3b78c3dbac55f5199f33f3fe0036439811f7fb3) )
	ROM_LOAD( "355640-01.u4", 0x0000, 0x4000, CRC(57224cde) SHA1(ab16f56989b27d89babe5f89c5a8cb3da71a82f0) )
	ROM_LOAD_OPTIONAL( "jiffydos 1541-ii.u4", 0x4000, 0x4000, CRC(dd409902) SHA1(b1a5b826304d3df2a27d7163c6a81a532e040d32) )
ROM_END

/*-------------------------------------------------
    ROM( sx1541 )
-------------------------------------------------*/

ROM_START( sx1541 ) // schematic 314001-05
	ROM_REGION( 0xa000, "sx1541", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.uab4",    0x0000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "901229-05 ae.uab5", 0x2000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
	ROM_LOAD_OPTIONAL( "jiffydos sx1541",   0x4000, 0x4000, CRC(783575f6) SHA1(36ccb9ff60328c4460b68522443ecdb7f002a234) )
	ROM_LOAD_OPTIONAL( "1541 flash.uab5",   0x8000, 0x2000, CRC(22f7757e) SHA1(86a1e43d3d22b35677064cca400a6bd06767a3dc) )
ROM_END

/*-------------------------------------------------
    ROM( c2031 )
-------------------------------------------------*/

ROM_START( c2031 ) // schematic 1540039
	ROM_REGION( 0x4000, "c2031", ROMREGION_LOADBYNAME )
	ROM_LOAD( "901484-03.u5f", 0x0000, 0x2000, CRC(ee4b893b) SHA1(54d608f7f07860f24186749f21c96724dd48bc50) )
	ROM_LOAD( "901484-05.u5h", 0x2000, 0x2000, CRC(6a629054) SHA1(ec6b75ecfdd4744e5d57979ef6af990444c11ae1) )
ROM_END

/*-------------------------------------------------
    ROM( oc118 )
-------------------------------------------------*/

ROM_START( oc118 ) // schematic 1540039
	ROM_REGION( 0x8000, "oc118", ROMREGION_LOADBYNAME )
	ROM_LOAD( "oc118.bin", 0x0000, 0x4000, NO_DUMP )
	ROM_LOAD_OPTIONAL( "jiffydos oc118.bin", 0x4000, 0x4000, CRC(46c3302c) SHA1(e3623658cb7af30c9d3bce2ba3b6ad5ee89ac1b8) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1541 )
-------------------------------------------------*/

static DEVICE_START( c1541 )
{
	c1541_t *c1541 = get_safe_token(device);
	const c1541_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 12));
	c1541->address = config->address - 8;

	/* find our CPU */
	c1541->cpu = device->subdevice(M6502_TAG);

	/* find devices */
	c1541->via0 = device->subdevice<via6522_device>(M6522_0_TAG);
	c1541->via1 = device->subdevice<via6522_device>(M6522_1_TAG);
	c1541->bus = device->machine().device(config->bus_tag);
	c1541->image = device->subdevice(FLOPPY_0);

	/* install image callbacks */
	floppy_install_unload_proc(c1541->image, on_disk_change);
	floppy_install_load_proc(c1541->image, on_disk_change);

	/* allocate track buffer */
//  c1541->track_buffer = auto_alloc_array(device->machine(), UINT8, G64_BUFFER_SIZE);

	/* allocate data timer */
	c1541->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_tick), (void *)device);

	/* register for state saving */
//  device->save_pointer(NAME(c1541->track_buffer), G64_BUFFER_SIZE);
	device->save_item(NAME(c1541->address));
	device->save_item(NAME(c1541->track_len));
	device->save_item(NAME(c1541->buffer_pos));
	device->save_item(NAME(c1541->bit_pos));
	device->save_item(NAME(c1541->bit_count));
	device->save_item(NAME(c1541->data));
	device->save_item(NAME(c1541->yb));
	device->save_item(NAME(c1541->byte));
	device->save_item(NAME(c1541->atna));
	device->save_item(NAME(c1541->ds));
	device->save_item(NAME(c1541->stp));
	device->save_item(NAME(c1541->soe));
	device->save_item(NAME(c1541->mode));
	device->save_item(NAME(c1541->via0_irq));
	device->save_item(NAME(c1541->via1_irq));
	device->save_item(NAME(c1541->data_out));
	device->save_item(NAME(c1541->nrfd_out));
	device->save_item(NAME(c1541->ndac_out));
}

/*-------------------------------------------------
    DEVICE_RESET( c1541 )
-------------------------------------------------*/

static DEVICE_RESET( c1541 )
{
	c1541_t *c1541 = get_safe_token(device);

	c1541->cpu->reset();
	c1541->via0->reset();
	c1541->via1->reset();
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1540 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1540 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1541_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1541_config);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1540);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1540);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1541);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1541);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore VIC-1540");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "vic1540");									break;		
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-1540");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1541 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1541 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1541);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1541);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore VIC-1541");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "vic1541");									break;		
		
		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1541c )
-------------------------------------------------*/

DEVICE_GET_INFO( c1541c )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1541c);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1541c);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1541C");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "vic1541c");								break;		

		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1541ii )
-------------------------------------------------*/

DEVICE_GET_INFO( c1541ii )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1541ii);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c1541ii);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1541-II");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "vic1541ii");								break;		

		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( sx1541 )
-------------------------------------------------*/

DEVICE_GET_INFO( sx1541 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(sx1541);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(sx1541);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore SX1541");						break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "sx1541");									break;		

		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c2031 )
-------------------------------------------------*/

DEVICE_GET_INFO( c2031 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c2031);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(c2031);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 2031");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "c2031");									break;		

		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( oc118 )
-------------------------------------------------*/

DEVICE_GET_INFO( oc118 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(oc118);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(oc118);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Oceanic OC-118");							break;
		case DEVINFO_STR_SHORTNAME:						strcpy(info->s, "oc118");									break;		

		default:										DEVICE_GET_INFO_CALL(c1540);								break;
	}
}

DEFINE_LEGACY_DEVICE(C1540, c1540);
DEFINE_LEGACY_DEVICE(C1541, c1541);
DEFINE_LEGACY_DEVICE(C1541C, c1541c);
DEFINE_LEGACY_DEVICE(C1541II, c1541ii);
DEFINE_LEGACY_DEVICE(SX1541, sx1541);
DEFINE_LEGACY_DEVICE(C2031, c2031);
DEFINE_LEGACY_DEVICE(OC118, oc118);
