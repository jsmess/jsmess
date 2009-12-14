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

/*

	TODO:

	- job flag never gets cleared
	- activity led
	- write G64
	- D64 to G64 conversion
	- accurate timing

*/

#include "driver.h"
#include "c1541.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/g64_dsk.h"
#include "machine/6522via.h"
#include "machine/cbmserial.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_TAG		"ucd5"
#define M6522_0_TAG		"uab1"
#define M6522_1_TAG		"ucd4"
#define TIMER_BIT_TAG	"ue7"

#define FLOPPY_TAG		"c1541_floppy"

static const double C1541_BITRATE[4] =
{
	XTAL_16MHz/13.0, XTAL_16MHz/14.0, XTAL_16MHz/15.0, XTAL_16MHz/16.0
};

#define SYNC_MARK		0x3ff

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1541_t c1541_t;
struct _c1541_t
{
	/* abstractions */
	int address;						/* serial bus address - 8 */
	UINT8 track_buffer[8192];			/* track data buffer */
	int track_len;						/* track length */
	int buffer_pos;						/* current byte position within track buffer */
	int bit_pos;						/* current bit position within track buffer byte */
	int bit_count;						/*  */
	UINT16 data;

	/* signals */
	UINT8 yb;							/* GCR data byte */
	int byte;							/* byte ready */
	int atna;							/* attention acknowledge */
	int ds;								/* density select */
	int stp;							/* stepping motor */
	int soe;							/* s? output enable */
	int mode;							/* mode (0 = write, 1 = read) */

	/* serial bus */
	int data_out;						/* serial bus data output */

	/* devices */
	const device_config *cpu;
	const device_config *via0;
	const device_config *via1;
	const device_config *serial_bus;
	const device_config *image;
	
	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1541_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == C1541);
	return (c1541_t *)device->token;
}

INLINE c1541_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == C1541);
	return (c1541_config *)device->inline_config;
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

		if (c1541->buffer_pos > c1541->track_len + 1)
		{
			/* loop to the start of the track */
			c1541->buffer_pos = 2;
		}
	}

	if ((c1541->data & SYNC_MARK) == SYNC_MARK)
	{
		/* SYNC detected */
		c1541->bit_count = 0;
	}

	if (c1541->bit_count > 7)
	{
		/* byte ready */
		c1541->bit_count = 0;
		byte = 1;
	}

	if (c1541->byte != byte)
	{
		int byte_ready = !(byte && c1541->soe);

		cpu_set_input_line(c1541->cpu, M6502_SET_OVERFLOW, byte_ready);
		via_ca1_w(c1541->via1, 0, byte_ready);
		
		c1541->byte = byte;
	}
}

/*-------------------------------------------------
    c1541_atn_w - serial bus attention
-------------------------------------------------*/

static CBMSERIAL_ATN( c1541 )
{
	c1541_t *c1541 = get_safe_token(device);
	int serial_data = !c1541->data_out && !(c1541->atna ^ !state);

	via_ca1_w(c1541->via0, 0, !state);

	cbmserial_data_w(c1541->serial_bus, device, serial_data);
}

/*-------------------------------------------------
    c1541_reset_w - serial bus reset
-------------------------------------------------*/

static CBMSERIAL_RESET( c1541 )
{
	if (!state)
	{
		device_reset(device);
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1541_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1541_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1800, 0x180f) AM_DEVREADWRITE(M6522_0_TAG, via_r, via_w)
	AM_RANGE(0x1c00, 0x1c0f) AM_DEVREADWRITE(M6522_1_TAG, via_r, via_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c1541", 0xc000)
ADDRESS_MAP_END

/*-------------------------------------------------
    via6522_interface c1541_via0_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( via0_pb_r )
{
	/*

		bit		description

		PB0		DATA IN
		PB1		DATA OUT
		PB2		CLK IN
		PB3		CLK OUT
		PB4		ATNA
		PB5		J1
		PB6		J2
		PB7		ATN IN

	*/

	c1541_t *c1541 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* data in */
	data = !cbmserial_data_r(c1541->serial_bus);

	/* clock in */
	data |= !cbmserial_clk_r(c1541->serial_bus) << 2;

	/* serial bus address */
	data |= c1541->address << 5;

	/* attention in */
	data |= !cbmserial_atn_r(c1541->serial_bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via0_pb_w )
{
	/*

		bit		description

		PB0		DATA IN
		PB1		DATA OUT
		PB2		CLK IN
		PB3		CLK OUT
		PB4		ATNA
		PB5		J1
		PB6		J2
		PB7		ATN IN

	*/

	c1541_t *c1541 = get_safe_token(device->owner);
	
	int data_out = BIT(data, 1);
	int clk_out = BIT(data, 3);
	int atna = BIT(data, 4);

	/* data out */
	int serial_data = !data_out && !(atna ^ !cbmserial_atn_r(c1541->serial_bus));
	cbmserial_data_w(c1541->serial_bus, device->owner, serial_data);
	c1541->data_out = data_out;

	/* clock out */
	cbmserial_clk_w(c1541->serial_bus, device->owner, !clk_out);

	/* attention acknowledge */
	c1541->atna = atna;
}

static const via6522_interface c1541_via0_intf =
{
	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_HANDLER(via0_pb_w),
	DEVCB_NULL, // ATN IN
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0)
};

/*-------------------------------------------------
    via6522_interface c1541_via1_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( yb_r )
{
	/*

		bit		description

		PA0		YB0
		PA1		YB1
		PA2		YB2
		PA3		YB3
		PA4		YB4
		PA5		YB5
		PA6		YB6
		PA7		YB7

	*/

	c1541_t *c1541 = get_safe_token(device->owner);

	return c1541->data & 0xff;
}

static WRITE8_DEVICE_HANDLER( yb_w )
{
	/*

		bit		description

		PA0		YB0
		PA1		YB1
		PA2		YB2
		PA3		YB3
		PA4		YB4
		PA5		YB5
		PA6		YB6
		PA7		YB7

	*/

	c1541_t *c1541 = get_safe_token(device->owner);

	c1541->yb = data;
}

static READ8_DEVICE_HANDLER( via1_pb_r )
{
	/*

		bit		signal		description

		PB0		STP0		stepping motor bit 0
		PB1		STP1		stepping motor bit 1
		PB2		MTR			motor ON/OFF
		PB3		ACT			drive 0 LED
		PB4		WPS			write protect sense
		PB5		DS0			density select 0
		PB6		DS1			density select 1
		PB7		SYNC		SYNC detect line

	*/

	c1541_t *c1541 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* write protect sense */
	data |= (floppy_drive_get_flag_state(c1541->image, FLOPPY_DRIVE_DISK_WRITE_PROTECTED) == FLOPPY_DRIVE_DISK_WRITE_PROTECTED) << 4;

	/* SYNC detect */
	data |= !(c1541->mode && ((c1541->data & SYNC_MARK) == SYNC_MARK)) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( via1_pb_w )
{
	/*

		bit		signal		description

		PB0		STP0		stepping motor bit 0
		PB1		STP1		stepping motor bit 1
		PB2		MTR			motor ON/OFF
		PB3		ACT			drive 0 LED
		PB4		WPS			write protect sense
		PB5		DS0			density select 0
		PB6		DS1			density select 1
		PB7		SYNC		SYNC detect line

	*/

	c1541_t *c1541 = get_safe_token(device->owner);

	int stp = data & 0x03;
	int ds = (data >> 5) & 0x03;
	int mtr = BIT(data, 2);

	/* stepper motor */
	if (c1541->stp != stp)
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
			c1541->track_len = 8192;
			c1541->buffer_pos = 2;
			c1541->bit_pos = 7;
			c1541->bit_count = 0;

			floppy_drive_seek(c1541->image, tracks);
			floppy_drive_read_track_data_info_buffer(c1541->image, 0, c1541->track_buffer, &c1541->track_len);

			c1541->track_len = (c1541->track_buffer[1] << 8) | c1541->track_buffer[0];
		}

		c1541->stp = stp;
	}

	/* spindle motor */
	floppy_drive_set_motor_state(c1541->image, mtr ? FLOPPY_DRIVE_MOTOR_ON : 0);
	timer_enable(c1541->bit_timer, mtr);

	/* activity LED */

	/* density select */
	if (c1541->ds != ds)
	{
		timer_adjust_periodic(c1541->bit_timer, attotime_zero, 0, ATTOTIME_IN_HZ(C1541_BITRATE[ds]/4));
		c1541->ds = ds;
	}
}

static WRITE_LINE_DEVICE_HANDLER( soe_w )
{
	c1541_t *c1541 = get_safe_token(device->owner);
	int byte_ready = !(state && c1541->byte);

	c1541->soe = state;

	cpu_set_input_line(c1541->cpu, M6502_SET_OVERFLOW, byte_ready);
	via_ca1_w(device, 0, byte_ready);
}

static WRITE_LINE_DEVICE_HANDLER( mode_w )
{
	c1541_t *c1541 = get_safe_token(device->owner);

	c1541->mode = state;
}

static const via6522_interface c1541_via1_intf =
{
	DEVCB_HANDLER(yb_r),
	DEVCB_HANDLER(via1_pb_r),
	DEVCB_NULL, // BYTE REDY
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_HANDLER(yb_w),
	DEVCB_HANDLER(via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(soe_w),
	DEVCB_LINE(mode_w),

	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0)
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1541 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1541 )
//	FLOPPY_OPTION( c1541, "d64", "Commodore 1541 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c1541, "g64", "Commodore 1541 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
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
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c1541),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1541 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1541 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MDRV_CPU_PROGRAM_MAP(c1541_map)

	MDRV_VIA6522_ADD(M6522_0_TAG, XTAL_16MHz/16, c1541_via0_intf)
	MDRV_VIA6522_ADD(M6522_1_TAG, XTAL_16MHz/16, c1541_via1_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_TAG, c1541_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c1540 )
-------------------------------------------------*/

ROM_START( c1540 ) // schematic 1540008
	ROM_REGION( 0x1000, "c1540", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.uab4", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "325303-01.uab5", 0xe000, 0x2000, CRC(10b39158) SHA1(56dfe79b26f50af4e83fd9604857756d196516b9) )
ROM_END

/*-------------------------------------------------
    ROM( c1541 )
-------------------------------------------------*/

ROM_START( c1541 ) // schematic 1540008
	ROM_REGION( 0x10000, "c1541", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.uab4", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "901229-01.uab5", 0xe000, 0x2000, CRC(9a48d3f0) SHA1(7a1054c6156b51c25410caec0f609efb079d3a77) )
	ROM_LOAD( "901229-02.uab5", 0xe000, 0x2000, CRC(b29bab75) SHA1(91321142e226168b1139c30c83896933f317d000) )
	ROM_LOAD( "901229-03.uab5", 0xe000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744) )
	ROM_LOAD( "901229-04.uab5", 0xe000, 0x2000, NO_DUMP )
	ROM_LOAD( "901229-05 ae.uab5", 0xe000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
//	ROM_LOAD( "901229-06 aa.uab5", 0xe000, 0x2000, CRC(3a235039) SHA1(c7f94f4f51d6de4cdc21ecbb7e57bb209f0530c0) )
ROM_END

/*-------------------------------------------------
    ROM( c1541cr )
-------------------------------------------------*/

ROM_START( c1541cr ) // schematic 1540049
	ROM_REGION( 0x10000, "c1541cr", ROMREGION_LOADBYNAME )
	ROM_LOAD( "325302-01.ub3", 0xc000, 0x2000, CRC(29ae9752) SHA1(8e0547430135ba462525c224e76356bd3d430f11) )
	ROM_LOAD( "901229-01.ub4", 0xe000, 0x2000, CRC(9a48d3f0) SHA1(7a1054c6156b51c25410caec0f609efb079d3a77) )
	ROM_LOAD( "901229-02.ub4", 0xe000, 0x2000, CRC(b29bab75) SHA1(91321142e226168b1139c30c83896933f317d000) )
	ROM_LOAD( "901229-03.ub4", 0xe000, 0x2000, CRC(9126e74a) SHA1(03d17bd745066f1ead801c5183ac1d3af7809744) )
	ROM_LOAD( "901229-04.ub4", 0xe000, 0x2000, NO_DUMP )
	ROM_LOAD( "901229-05 ae.ub4", 0xe000, 0x2000, CRC(361c9f37) SHA1(f5d60777440829e46dc91285e662ba072acd2d8b) )
	ROM_LOAD( "901229-06 aa.ub4", 0xe000, 0x2000, CRC(3a235039) SHA1(c7f94f4f51d6de4cdc21ecbb7e57bb209f0530c0) )
ROM_END

/*-------------------------------------------------
    ROM( c1541a )
-------------------------------------------------*/

#define rom_c1541a rom_c1541cr // schematic 251748

/*-------------------------------------------------
    ROM( c1541a2 )
-------------------------------------------------*/

#define rom_c1541a2 rom_c1541cr // schematic 251748

/*-------------------------------------------------
    ROM( c1541b )
-------------------------------------------------*/

ROM_START( c1541b ) // schematic ?
	ROM_REGION( 0x10000, "c1541b", ROMREGION_LOADBYNAME )
	ROM_LOAD( "251968-01.ua2", 0xc000, 0x4000, CRC(1b3ca08d) SHA1(8e893932de8cce244117fcea4c46b7c39c6a7765) )
	ROM_LOAD( "251968-02.ua2", 0xc000, 0x4000, CRC(2d862d20) SHA1(38a7a489c7bbc8661cf63476bf1eb07b38b1c704) )
ROM_END

/*-------------------------------------------------
    ROM( c1541c )
-------------------------------------------------*/

#define rom_c1541c rom_c1541b // schematic ?

/*-------------------------------------------------
    ROM( c1541ii )
-------------------------------------------------*/

ROM_START( c1541ii ) // schematic 340503
	ROM_REGION( 0x10000, "c1541ii", ROMREGION_LOADBYNAME )
	ROM_LOAD( "251968-03.u4", 0xc000, 0x4000, CRC(899fa3c5) SHA1(d3b78c3dbac55f5199f33f3fe0036439811f7fb3) )
	ROM_LOAD( "355640-01.u4", 0xc000, 0x4000, CRC(57224cde) SHA1(ab16f56989b27d89babe5f89c5a8cb3da71a82f0) )
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
	c1541->cpu = device_find_child_by_tag(device, M6502_TAG);

	/* find devices */
	c1541->via0 = device_find_child_by_tag(device, M6522_0_TAG);
	c1541->via1 = device_find_child_by_tag(device, M6522_1_TAG);
	c1541->serial_bus = devtag_get_device(device->machine, config->serial_bus_tag);
	c1541->image = device_find_child_by_tag(device, FLOPPY_TAG);

	/* allocate data timer */
	c1541->bit_timer = timer_alloc(device->machine, bit_tick, (void *)device);
//	timer_adjust_periodic(c1541->bit_timer, attotime_zero, 0, ATTOTIME_IN_HZ(C1541_BITRATE[0]));
//	timer_enable(c1541->bit_timer, 0);

	/* register for state saving */
//	state_save_register_device_item(device, 0, c1541->);
}

/*-------------------------------------------------
    DEVICE_RESET( c1541 )
-------------------------------------------------*/

static DEVICE_RESET( c1541 )
{
	c1541_t *c1541 = get_safe_token(device);

	device_reset(c1541->cpu);
	device_reset(c1541->via0);
	device_reset(c1541->via1);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1541 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1541 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1541_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1541_config);								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1541);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1541);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1541);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1541);						break;
		case DEVINFO_FCT_CBM_SERIAL_ATN:				info->f = (genf *)CBMSERIAL_ATN_NAME(c1541);				break;
		case DEVINFO_FCT_CBM_SERIAL_RESET:				info->f = (genf *)CBMSERIAL_RESET_NAME(c1541);				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1541");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-1540");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
