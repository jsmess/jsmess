/******************************************************************************

SNK NeoGeo Pocket driver

The NeoGeo Pocket (Color) contains one big chip which contains the following
components:
- Toshiba TLCS-900/H cpu core with 64KB ROM
- Z80 cpu core
- K1GE (mono)/K2GE (color) graphics chip + lcd controller
- SN76489A compatible sound chip
- DAC for additional sound
- RTC
- RAM


The following TLCS-900/H input sources are known:
- INT0 - Alarm/RTC irq
- INT4 - VBlank irq
- INT5 - Interrupt from Z80
- AN0 - Analog input which is used to determine the battery level
- NMI - Triggered by power button
- TIO - HBlank signal


The following TLCS-900/H output destination are known:
- TO3 - connected to Z80 INT pin


The cartridges
==============

The cartridges used flash chips produced by Toshiba, Sharp or Samsung. These
are the only 3 manufacturers supported by the NeoGeo pocket bios. The device
IDs supported appear to be SNK exclusive. Most likely because of the factory
blocked game data areas on these chip.

These manufacturer IDs are supported: 0x98, 0xec, 0xb0
These device IDs are supported: 0xab, 0x2c, 0x2f

There is support for 3 different sizes of flash roms: 4Mbit, 8Mbit 16Mbit. The
32Mbit games appear to be 2 16Mbit flash chips in 2 different memory regions (?).

The flash chips have a couple of different sized blocks. When writing to a
cartridge the neogeo pocket bios will erase the proper block and write the data
to the block.

The relation between block number and flash chip is as follows:

 # |  16Mbit (2f)  |  8Mbit (2c) |  4Mbit (ab)
---+---------------+-------------+-------------
 0 | 000000-00ffff | 00000-0ffff | 00000-0ffff
 1 | 010000-01ffff | 10000-1ffff | 10000-1ffff
 2 | 020000-02ffff | 20000-2ffff | 20000-2ffff
 3 | 030000-03ffff | 30000-3ffff | 30000-3ffff
 4 | 040000-01ffff | 40000-4ffff | 40000-4ffff
 5 | 050000-01ffff | 50000-5ffff | 50000-5ffff
 6 | 060000-01ffff | 60000-6ffff | 60000-6ffff
 7 | 070000-01ffff | 70000-7ffff | 70000-77fff
 8 | 080000-01ffff | 80000-8ffff | 78000-79fff
 9 | 090000-01ffff | 90000-9ffff | 7a000-7bfff
10 | 0a0000-01ffff | a0000-affff | 7c000-7ffff
11 | 0b0000-01ffff | b0000-bffff | 
12 | 0c0000-01ffff | c0000-cffff | 
13 | 0d0000-01ffff | d0000-dffff | 
14 | 0e0000-01ffff | e0000-effff | 
15 | 0f0000-01ffff | f0000-f7fff | 
16 | 100000-10ffff | f8000-f9fff | 
17 | 110000-11ffff | fa000-fbfff | 
18 | 120000-12ffff | fc000-fffff | 
19 | 130000-13ffff |             | 
20 | 140000-14ffff |             |
21 | 150000-15ffff |             | 
22 | 160000-16ffff |             | 
23 | 170000-17ffff |             | 
24 | 180000-18ffff |             | 
25 | 190000-19ffff |             | 
26 | 1a0000-1affff |             | 
27 | 1b0000-1bffff |             | 
28 | 1c0000-1cffff |             | 
29 | 1d0000-1dffff |             | 
30 | 1e0000-1effff |             | 
31 | 1f0000-1f7fff |             | 
32 | 1f8000-1f9fff |             | 
33 | 1fa000-1fbfff |             | 
34 | 1fc000-1fffff |             | 

The last block is always reserved for use by the system. The Neogeo Pocket Color
bios does some tests on this last block to see if the flash functionality is
working. It does this on every boot!

The Toshiba TC58FVT004 seems to have an interface similar to what is used in
the Neogeo Pocket.


******************************************************************************/

#include "driver.h"
#include "cpu/tlcs900/tlcs900.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"
#include "sound/sn76496.h"
#include "sound/dac.h"
#include "video/k1ge.h"


enum flash_state
{
	F_READ,						/* xxxx F0 or 5555 AA 2AAA 55 5555 F0 */
	F_PROG1,
	F_PROG2,
	F_COMMAND,
	F_ID_READ,					/* 5555 AA 2AAA 55 5555 90 */
	F_AUTO_PROGRAM,				/* 5555 AA 2AAA 55 5555 A0 address data */
	F_AUTO_CHIP_ERASE,			/* 5555 AA 2AAA 55 5555 80 5555 AA 2AAA 55 5555 10 */
	F_AUTO_BLOCK_ERASE,			/* 5555 AA 2AAA 55 5555 80 5555 AA 2AAA 55 block_address 30 */
	F_BLOCK_PROTECT,			/* 5555 AA 2AAA 55 5555 9A 5555 AA 2AAA 55 5555 9A */
};


static UINT8 io_reg[0x40];
static emu_timer* seconds_timer;

struct {
	int		present;
	UINT8	manufacturer_id;
	UINT8	device_id;
	UINT8	*data;
	UINT8	org_data[16];
	int		state;
	UINT8	command[2];
} flash_chip[2];


static TIMER_CALLBACK( ngp_seconds_callback )
{
	io_reg[0x16] += 1;
	if ( ( io_reg[0x16] & 0x0f ) == 0x0a )
	{
		io_reg[0x16] += 0x06;
	}

	if ( io_reg[0x16] >= 0x60 )
	{
		io_reg[0x16] = 0;
		io_reg[0x15] += 1;
		if ( ( io_reg[0x15] & 0x0f ) == 0x0a ) {
			io_reg[0x15] += 0x06;
		}

		if ( io_reg[0x15] >= 0x60 )
		{
			io_reg[0x15] = 0;
			io_reg[0x14] += 1;
			if ( ( io_reg[0x14] & 0x0f ) == 0x0a ) {
				io_reg[0x14] += 0x06;
			}

			if ( io_reg[0x14] == 0x24 )
			{
				io_reg[0x14] = 0;
			}
		}
	}
}


static READ8_HANDLER( ngp_io_r )
{
	UINT8	data = io_reg[offset];

	switch( offset )
	{
	case 0x30:	/* Read controls */
		data = input_port_read(space->machine, "Controls" );
		break;
	case 0x31:
		data = input_port_read( space->machine, "Power" ) & 0x01;
		/* Sub-batttery OK */
		data |= 0x02;
		break;
	}
	return data;
}


static WRITE8_HANDLER( ngp_io_w )
{
	switch( offset )
	{
	case 0x20:		/* sn76489 right */
	case 0x21:		/* sn76489 left */
		sn76496_w( devtag_get_device( space->machine, "sn76489a" ), 0, data );
		break;

	case 0x22:		/* DAC right */
		dac_w( devtag_get_device( space->machine, "dac_r" ), 0, data );
		break;
	case 0x23:		/* DAC left */
		dac_w( devtag_get_device( space->machine, "dac_l" ), 0, data );
		break;

	/* Internal eeprom related? */
	case 0x36:
	case 0x37:
		break;
	case 0x38:	/* Sound enable/disable. */
		switch( data )
		{
		case 0x55:		/* Enabled sound */
			break;
		case 0xAA:		/* Disable sound */
			break;
		}
		break;

	case 0x39:	/* Z80 enable/disable. */
		switch( data )
		{
		case 0x55:		/* Enable Z80 */
			cputag_resume( space->machine, "soundcpu", SUSPEND_REASON_HALT );
			cputag_reset( space->machine, "soundcpu" );
			cputag_set_input_line( space->machine, "soundcpu", 0, CLEAR_LINE );
			break;
		case 0xAA:		/* Disable Z80 */
			cputag_suspend( space->machine, "soundcpu", SUSPEND_REASON_HALT, 1 );
			break;
		}
		break;

	case 0x3a:	/* Trigger Z80 NMI */
		cputag_set_input_line( space->machine, "soundcpu", INPUT_LINE_NMI, PULSE_LINE );
		break;
	}
	io_reg[offset] = data;
}


static void flash_w( int which, offs_t offset, UINT8 data )
{
	if ( ! flash_chip[which].present )
		return;

	switch( flash_chip[which].state )
	{
	case F_READ:
		if ( offset == 0x5555 && data == 0xaa )
			flash_chip[which].state = F_PROG1;
		flash_chip[which].command[0] = 0;
		break;
	case F_PROG1:
		if ( offset == 0x2aaa && data == 0x55 )
			flash_chip[which].state = F_PROG2;
		else
			flash_chip[which].state = F_READ;
		break;
	case F_PROG2:
		if ( data == 0x30 )
		{
			if ( flash_chip[which].command[0] == 0x80 )
			{
				int	size = 0x10000;
				UINT8 *block = flash_chip[which].data;

				flash_chip[which].state = F_AUTO_BLOCK_ERASE;
				switch( flash_chip[which].device_id )
				{
				case 0xab:
					if ( offset < 0x70000 )
						block = flash_chip[which].data + ( offset & 0x70000 );
					else
					{
						if ( offset & 0x8000 )
						{
							if ( offset & 0x4000 )
							{
								block = flash_chip[which].data + ( offset & 0x7c000 );
								size = 0x4000;
							}
							else
							{
								block = flash_chip[which].data + ( offset & 0x7e000 );
								size = 0x2000;
							}
						}
						else
						{
							block = flash_chip[which].data + ( offset & 0x78000 );
							size = 0x8000;
						}
					}
					break;
				case 0x2c:
					if ( offset < 0xf0000 )
						block = flash_chip[which].data + ( offset & 0xf0000 );
					else
					{
						if ( offset & 0x8000 )
						{
							if ( offset & 0x4000 )
							{
								block = flash_chip[which].data + ( offset & 0xfc000 );
								size = 0x4000;
							}
							else
							{
								block = flash_chip[which].data + ( offset & 0xfe000 );
								size = 0x2000;
							}
						}
						else
						{
							block = flash_chip[which].data + ( offset & 0xf8000 );
							size = 0x8000;
						}
					}
					break;
				case 0x2f:
					if ( offset < 0x1f0000 )
						block = flash_chip[which].data + ( offset & 0x1f0000 );
					else
					{
						if ( offset & 0x8000 )
						{
							if ( offset & 0x4000 )
							{
								block = flash_chip[which].data + ( offset & 0x1fc000 );
								size = 0x4000;
							}
							else
							{
								block = flash_chip[which].data + ( offset & 0x1fe000 );
								size = 0x2000;
							}
						}
						else
						{
							block = flash_chip[which].data + ( offset & 0x1f8000 );
							size = 0x8000;
						}
					}
					break;
				}
				memset( block, 0xFF, size );
			}
			else
				flash_chip[which].state = F_READ;
		}
		else if ( offset == 0x5555 )
		{
			switch( data )
			{
			case 0x80:
				flash_chip[which].command[0] = 0x80;
				flash_chip[which].state = F_COMMAND;
				break;
			case 0x90:
				flash_chip[which].data[0x1fc000] = flash_chip[which].manufacturer_id;
				flash_chip[which].data[0xfc000] = flash_chip[which].manufacturer_id;
				flash_chip[which].data[0x7c000] = flash_chip[which].manufacturer_id;
				flash_chip[which].data[0] = flash_chip[which].manufacturer_id;
				flash_chip[which].data[0x1fc001] = flash_chip[which].device_id;
				flash_chip[which].data[0xfc001] = flash_chip[which].device_id;
				flash_chip[which].data[0x7c001] = flash_chip[which].device_id;
				flash_chip[which].data[1] = flash_chip[which].device_id;
				flash_chip[which].data[0x1fc002] = 0x02;
				flash_chip[which].data[0xfc002] = 0x02;
				flash_chip[which].data[0x7c002] = 0x02;
				flash_chip[which].data[2] = 0x02;
				flash_chip[which].data[0x1fc003] = 0x80;
				flash_chip[which].data[0xfc003] = 0x80;
				flash_chip[which].data[0x7c003] = 0x80;
				flash_chip[which].data[3] = 0x80;
				flash_chip[which].state = F_ID_READ;
				break;
			case 0x9a:
				if ( flash_chip[which].command[0] == 0x9a )
					flash_chip[which].state = F_BLOCK_PROTECT;
				else
				{
					flash_chip[which].command[0] = 0x9a;
					flash_chip[which].state = F_COMMAND;
				}
				break;
			case 0xa0:
				flash_chip[which].state = F_AUTO_PROGRAM;
				break;
			case 0xf0:
			default:
				flash_chip[which].state = F_READ;
				break;
			}
		}
		else
			flash_chip[which].state = F_READ;
		break;
	case F_COMMAND:
		if ( offset == 0x5555 && data == 0xaa )
			flash_chip[which].state = F_PROG1;
		else
			flash_chip[which].state = F_READ;
		break;
	case F_ID_READ:
		if ( offset == 0x5555 && data == 0xaa )
			flash_chip[which].state = F_PROG1;
		else
			flash_chip[which].state = F_READ;
		flash_chip[which].command[0] = 0;
		break;
	case F_AUTO_PROGRAM:
		/* Only 1 -> 0 changes can be programmed */
		flash_chip[which].data[offset] = flash_chip[which].data[offset] & data;
		flash_chip[which].state = F_READ;
		break;
	case F_AUTO_CHIP_ERASE:
		flash_chip[which].state = F_READ;
		break;
	case F_AUTO_BLOCK_ERASE:
		flash_chip[which].state = F_READ;
		break;
	case F_BLOCK_PROTECT:
		flash_chip[which].state = F_READ;
		break;
	}

	if ( flash_chip[which].state == F_READ )
	{
		/* Exit command/back to normal operation*/
		flash_chip[which].data[0] = flash_chip[which].org_data[0];
		flash_chip[which].data[1] = flash_chip[which].org_data[1];
		flash_chip[which].data[2] = flash_chip[which].org_data[2];
		flash_chip[which].data[3] = flash_chip[which].org_data[3];
		flash_chip[which].data[0x7c000] = flash_chip[which].org_data[4];
		flash_chip[which].data[0x7c001] = flash_chip[which].org_data[5];
		flash_chip[which].data[0x7c002] = flash_chip[which].org_data[6];
		flash_chip[which].data[0x7c003] = flash_chip[which].org_data[7];
		flash_chip[which].data[0xfc000] = flash_chip[which].org_data[8];
		flash_chip[which].data[0xfc001] = flash_chip[which].org_data[9];
		flash_chip[which].data[0xfc002] = flash_chip[which].org_data[10];
		flash_chip[which].data[0xfc003] = flash_chip[which].org_data[11];
		flash_chip[which].data[0x1fc000] = flash_chip[which].org_data[12];
		flash_chip[which].data[0x1fc001] = flash_chip[which].org_data[13];
		flash_chip[which].data[0x1fc002] = flash_chip[which].org_data[14];
		flash_chip[which].data[0x1fc003] = flash_chip[which].org_data[15];
		flash_chip[which].command[0] = 0;
	}
}


static WRITE8_HANDLER( flash0_w )
{
	flash_w( 0, offset, data );
}


static WRITE8_HANDLER( flash1_w )
{
	flash_w( 1, offset, data );
}


static ADDRESS_MAP_START( ngp_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x000080, 0x0000bf )	AM_READWRITE( ngp_io_r, ngp_io_w )								/* ngp/c specific i/o */
	AM_RANGE( 0x004000, 0x006fff )	AM_RAM															/* work ram */
	AM_RANGE( 0x007000, 0x007fff )	AM_RAM AM_SHARE(1)												/* shared with sound cpu */
	AM_RANGE( 0x008000, 0x0087ff )	AM_DEVREADWRITE( "k1ge", k1ge_r, k1ge_w )						/* video registers */
	AM_RANGE( 0x008800, 0x00bfff )	AM_RAM AM_REGION("vram", 0x800 )								/* Video RAM area */
	AM_RANGE( 0x200000, 0x3fffff )	AM_ROM AM_WRITE( flash0_w ) AM_REGION( "cart", 0 )				/* cart area #1 */
	AM_RANGE( 0x800000, 0x9fffff )	AM_ROM AM_WRITE( flash1_w ) AM_REGION( "cart", 0x200000 )		/* cart area #2 */
	AM_RANGE( 0xff0000, 0xffffff )	AM_ROM AM_REGION( "maincpu", 0 )								/* system rom */
ADDRESS_MAP_END


static READ8_HANDLER( ngp_z80_comm_r )
{
	return io_reg[0x3c];
}


static WRITE8_HANDLER( ngp_z80_comm_w )
{
	io_reg[0x3c] = data;
}


static WRITE8_HANDLER( ngp_z80_signal_main_w )
{
	cputag_set_input_line( space->machine, "maincpu", TLCS900_INT5, ASSERT_LINE );
}


static ADDRESS_MAP_START( z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0FFF )	AM_RAM AM_SHARE(1)								/* shared with tlcs900 */
	AM_RANGE( 0x4000, 0x4000 )	AM_DEVWRITE( "sn76489a", sn76496_w )			/* sound chip (right) */
	AM_RANGE( 0x4001, 0x4001 )	AM_DEVWRITE( "sn76489a", sn76496_w )			/* sound chip (left) */
	AM_RANGE( 0x8000, 0x8000 )	AM_READWRITE( ngp_z80_comm_r, ngp_z80_comm_w )	/* main-sound communication */
	AM_RANGE( 0xc000, 0xc000 )	AM_WRITE( ngp_z80_signal_main_w )				/* signal irq to main cpu */
ADDRESS_MAP_END


static WRITE8_HANDLER( ngp_z80_clear_irq )
{
	cputag_set_input_line( space->machine, "soundcpu", 0, CLEAR_LINE );

	/* I am not exactly sure what causes the maincpu INT5 signal to be cleared. This will do for now. */
	cputag_set_input_line( space->machine, "maincpu", TLCS900_INT5, CLEAR_LINE );
}


static ADDRESS_MAP_START( z80_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x0000, 0xffff )	AM_WRITE( ngp_z80_clear_irq )
ADDRESS_MAP_END


static INPUT_CHANGED( power_callback )
{
	if ( io_reg[0x33] & 0x04 )
	{
		cputag_set_input_line( field->port->machine, "maincpu", TLCS900_NMI,
			(input_port_read(field->port->machine, "Power") & 0x01 ) ? CLEAR_LINE : ASSERT_LINE );
	}
}


static INPUT_PORTS_START( ngp )
	PORT_START("Controls")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_NAME("Up")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_NAME("Down")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_NAME("Left")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_NAME("Right")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Button A")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Button B")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SELECT) PORT_NAME("Option")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("Power")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_NAME("Power") PORT_CHANGED(power_callback, NULL)
INPUT_PORTS_END


static WRITE8_DEVICE_HANDLER( ngp_vblank_pin_w )
{
	cputag_set_input_line( device->machine, "maincpu", TLCS900_INT4, data ? ASSERT_LINE : CLEAR_LINE );
}


static WRITE8_DEVICE_HANDLER( ngp_hblank_pin_w )
{
	cputag_set_input_line( device->machine, "maincpu", TLCS900_TIO, data ? ASSERT_LINE : CLEAR_LINE );
}


static UINT8 old_to3;

static WRITE8_DEVICE_HANDLER( ngp_tlcs900_to3 )
{
	if ( data && ! old_to3 )
		cputag_set_input_line( device->machine, "soundcpu", 0, ASSERT_LINE );

	old_to3 = data;
}


static MACHINE_START( ngp )
{
	seconds_timer = timer_alloc( machine, ngp_seconds_callback, NULL );
	timer_adjust_periodic( seconds_timer, ATTOTIME_IN_SEC(1), 0, ATTOTIME_IN_SEC(1) );
}


static MACHINE_RESET( ngp )
{
	cputag_suspend( machine, "soundcpu", SUSPEND_REASON_HALT, 1 );
	cputag_set_input_line( machine, "soundcpu", 0, CLEAR_LINE );
}


static VIDEO_UPDATE( ngp )
{
	k1ge_update( devtag_get_device( screen->machine, "k1ge" ), bitmap, cliprect );
	return 0;
}


static DEVICE_START( ngp_cart )
{
	UINT8 *cart = memory_region( device->machine, "cart" );

	flash_chip[0].present = 0;
	flash_chip[0].state = F_READ;
	flash_chip[0].data = cart;

	flash_chip[1].present = 0;
	flash_chip[1].state = F_READ;
	flash_chip[1].data = cart + 0x200000;
}


static DEVICE_IMAGE_LOAD( ngp_cart )
{
	int filesize = image_length( image );

	if ( filesize != 0x80000 && filesize != 0x100000 && filesize != 0x200000 && filesize != 0x400000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Incorrect or not support cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, memory_region(image->machine, "cart"), filesize ) != filesize )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Error loading file" );
		return INIT_FAIL;
	}

	flash_chip[0].manufacturer_id = 0x98;
	switch( filesize )
	{
	case 0x80000:
		flash_chip[0].device_id = 0xab;
		break;
	case 0x100000:
		flash_chip[0].device_id = 0x2c;
		break;
	case 0x200000:
		flash_chip[0].device_id = 0x2f;
		break;
	case 0x400000:
		flash_chip[0].device_id = 0x2f;
		flash_chip[1].manufacturer_id = 0x98;
		flash_chip[1].device_id = 0x2f;
		flash_chip[1].present = 0;
		flash_chip[1].state = F_READ;
		break;
	}

	flash_chip[0].org_data[0] = flash_chip[0].data[0];
	flash_chip[0].org_data[1] = flash_chip[0].data[1];
	flash_chip[0].org_data[2] = flash_chip[0].data[2];
	flash_chip[0].org_data[3] = flash_chip[0].data[3];
	flash_chip[0].org_data[4] = flash_chip[0].data[0x7c000];
	flash_chip[0].org_data[5] = flash_chip[0].data[0x7c001];
	flash_chip[0].org_data[6] = flash_chip[0].data[0x7c002];
	flash_chip[0].org_data[7] = flash_chip[0].data[0x7c003];
	flash_chip[0].org_data[8] = flash_chip[0].data[0xfc000];
	flash_chip[0].org_data[9] = flash_chip[0].data[0xfc001];
	flash_chip[0].org_data[10] = flash_chip[0].data[0xfc002];
	flash_chip[0].org_data[11] = flash_chip[0].data[0xfc003];
	flash_chip[0].org_data[12] = flash_chip[0].data[0x1fc000];
	flash_chip[0].org_data[13] = flash_chip[0].data[0x1fc001];
	flash_chip[0].org_data[14] = flash_chip[0].data[0x1fc002];
	flash_chip[0].org_data[15] = flash_chip[0].data[0x1fc003];

	flash_chip[1].org_data[0] = flash_chip[1].data[0];
	flash_chip[1].org_data[1] = flash_chip[1].data[1];
	flash_chip[1].org_data[2] = flash_chip[1].data[2];
	flash_chip[1].org_data[3] = flash_chip[1].data[3];
	flash_chip[1].org_data[4] = flash_chip[1].data[0x7c000];
	flash_chip[1].org_data[5] = flash_chip[1].data[0x7c001];
	flash_chip[1].org_data[6] = flash_chip[1].data[0x7c002];
	flash_chip[1].org_data[7] = flash_chip[1].data[0x7c003];
	flash_chip[1].org_data[8] = flash_chip[1].data[0xfc000];
	flash_chip[1].org_data[9] = flash_chip[1].data[0xfc001];
	flash_chip[1].org_data[10] = flash_chip[1].data[0xfc002];
	flash_chip[1].org_data[11] = flash_chip[1].data[0xfc003];
	flash_chip[1].org_data[12] = flash_chip[1].data[0x1fc000];
	flash_chip[1].org_data[13] = flash_chip[1].data[0x1fc001];
	flash_chip[1].org_data[14] = flash_chip[1].data[0x1fc002];
	flash_chip[1].org_data[15] = flash_chip[1].data[0x1fc003];

	flash_chip[0].present = 1;
	flash_chip[0].state = F_READ;

	return INIT_PASS;
}


static k1ge_interface ngp_k1ge_interface =
{
	"screen",
	"vram",
	DEVCB_HANDLER( ngp_vblank_pin_w ),
	DEVCB_HANDLER( ngp_hblank_pin_w )
};


static tlcs900_interface ngp_tlcs900_interface =
{
	DEVCB_NULL,
	DEVCB_HANDLER( ngp_tlcs900_to3 )
};


static MACHINE_DRIVER_START( ngp_common )
	MDRV_CPU_ADD( "maincpu", TLCS900H, XTAL_6_144MHz )
	MDRV_CPU_PROGRAM_MAP( ngp_mem, 0 )
	MDRV_CPU_CONFIG( ngp_tlcs900_interface )

	MDRV_CPU_ADD( "soundcpu", Z80, XTAL_6_144MHz/2 )
	MDRV_CPU_PROGRAM_MAP( z80_mem, 0 )
	MDRV_CPU_IO_MAP( z80_io, 0 )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS( XTAL_6_144MHz, 515, 0, 160 /*480*/, 199, 0, 152 )

	MDRV_MACHINE_START( ngp )
	MDRV_MACHINE_RESET( ngp )

	MDRV_VIDEO_UPDATE( ngp )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO( "lspeaker","rspeaker" )

	MDRV_SOUND_ADD( "sn76489a", SN76489A, XTAL_6_144MHz/2 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 0.50 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 0.50 )

	MDRV_SOUND_ADD( "dac_l", DAC, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "lspeaker", 0.50 )
	MDRV_SOUND_ADD( "dac_r", DAC, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "rspeaker", 0.50 )

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin,ngp,npc")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START( ngp_cart )
	MDRV_CARTSLOT_LOAD( ngp_cart )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ngp )
	MDRV_IMPORT_FROM( ngp_common )

	MDRV_PALETTE_LENGTH( 8 )
	MDRV_PALETTE_INIT( k1ge )

	MDRV_K1GE_ADD( "k1ge", XTAL_6_144MHz, ngp_k1ge_interface )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ngpc )
	MDRV_IMPORT_FROM( ngp_common )

	MDRV_PALETTE_LENGTH( 4096 )
	MDRV_PALETTE_INIT( k2ge )

	MDRV_K2GE_ADD( "k1ge", XTAL_6_144MHz, ngp_k1ge_interface )
MACHINE_DRIVER_END


ROM_START( ngp )
	ROM_REGION( 0x10000, "maincpu" , 0 )
	ROM_LOAD( "ngp_bios.ngp", 0x0000, 0x10000, CRC(6232df8d) SHA1(2f6429b68446536d8b03f35d02f1e98beb6460a0) )

	ROM_REGION( 0x4000, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x400000, "cart", ROMREGION_ERASEFF )
ROM_END


ROM_START( ngpc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "ngpcbios.rom", 0x0000, 0x10000, CRC(6eeb6f40) SHA1(edc13192054a59be49c6d55f83b70e2510968e86) )

	ROM_REGION( 0x4000, "vram", ROMREGION_ERASE00 )

	ROM_REGION( 0x400000, "cart", ROMREGION_ERASEFF )
ROM_END


CONS( 1998, ngp, 0, 0, ngp, ngp, 0, 0, "SNK", "NeoGeo Pocket", GAME_IMPERFECT_SOUND )
CONS( 1999, ngpc, ngp, 0, ngpc, ngp, 0, 0, "SNK", "NeoGeo Pocket Color", GAME_IMPERFECT_SOUND )

