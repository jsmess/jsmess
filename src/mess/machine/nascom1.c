/**********************************************************************

  machine/nascom1.c

**********************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/nascom1.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "machine/ay31015.h"

/* Devices */
#include "devices/snapquik.h"
#include "devices/cassette.h"

#define NASCOM1_KEY_RESET	0x02
#define NASCOM1_KEY_INCR	0x01



/*************************************
 *
 *  Global variables
 *
 *************************************/

static const device_config *nascom1_hd6402;
static int nascom1_tape_size = 0;
static UINT8 *nascom1_tape_image = NULL;
static int nascom1_tape_index = 0;

static	struct
{
	UINT8	stat_flags;
	UINT8	stat_count;
} nascom1_portstat;

static struct
{
	UINT8 select;
	UINT8 irq;
	UINT8 drq;
} nascom2_fdc;



/*************************************
 *
 *  Floppy
 *
 *************************************/

static WD17XX_CALLBACK( nascom2_fdc_callback )
{
	switch (state)
	{
	case WD17XX_IRQ_SET: nascom2_fdc.irq = 1; break;
	case WD17XX_IRQ_CLR: nascom2_fdc.irq = 0; break;
	case WD17XX_DRQ_SET: nascom2_fdc.drq = 1; break;
	case WD17XX_DRQ_CLR: nascom2_fdc.drq = 0; break;
	}
}


const wd17xx_interface nascom2_wd17xx_interface = { nascom2_fdc_callback, NULL, {FLOPPY_0,FLOPPY_1,FLOPPY_2,FLOPPY_3} };


READ8_HANDLER( nascom2_fdc_select_r )
{
	return nascom2_fdc.select | 0xa0;
}


WRITE8_HANDLER( nascom2_fdc_select_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd1793");
	nascom2_fdc.select = data;

	logerror("nascom2_fdc_select_w: %02x\n", data);

	if (data & 0x01) wd17xx_set_drive(fdc,0);
	if (data & 0x02) wd17xx_set_drive(fdc,1);
	if (data & 0x04) wd17xx_set_drive(fdc,2);
	if (data & 0x08) wd17xx_set_drive(fdc,3);
	if (data & 0x10) wd17xx_set_side(fdc,(data & 0x10) >> 4);
}


/*
 * D0 -- WD1793 IRQ
 * D1 -- NOT READY
 * D2 to D6 -- 0
 * D7 -- WD1793 DRQ
 *
 */
READ8_HANDLER( nascom2_fdc_status_r )
{
	return (nascom2_fdc.drq << 7) | nascom2_fdc.irq;
}

/*************************************
 *
 *  Keyboard
 *
 *************************************/

READ8_HANDLER ( nascom1_port_00_r )
{
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8" };

	if (nascom1_portstat.stat_count < 9)
		return (input_port_read(space->machine, keynames[nascom1_portstat.stat_count]) | ~0x7f);
	
	return (0xff);
}


WRITE8_HANDLER( nascom1_port_00_w )
{

	cassette_change_state( devtag_get_device(space->machine, "cassette"),
		( data & 0x10 ) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR );

	if (!(data & NASCOM1_KEY_RESET)) 
	{
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_RESET)
			nascom1_portstat.stat_count = 0;
	} 
	else 
		nascom1_portstat.stat_flags = NASCOM1_KEY_RESET;

	if (!(data & NASCOM1_KEY_INCR)) 
	{
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_INCR)
			nascom1_portstat.stat_count++;
	} 
	else 
		nascom1_portstat.stat_flags = NASCOM1_KEY_INCR;
}




/*************************************
 *
 *  Cassette
 *
 *************************************/


READ8_HANDLER( nascom1_port_01_r )
{
	return ay31015_get_received_data( nascom1_hd6402 );
}


WRITE8_HANDLER( nascom1_port_01_w )
{
	ay31015_set_transmit_data( nascom1_hd6402, data );
}

READ8_HANDLER( nascom1_port_02_r )
{
	UINT8 data = 0x31;

	ay31015_set_input_pin( nascom1_hd6402, AY31015_SWE, 0 );
	data |= ay31015_get_output_pin( nascom1_hd6402, AY31015_OR ) ? 0x02 : 0;
	data |= ay31015_get_output_pin( nascom1_hd6402, AY31015_PE ) ? 0x04 : 0;
	data |= ay31015_get_output_pin( nascom1_hd6402, AY31015_FE ) ? 0x08 : 0;
	data |= ay31015_get_output_pin( nascom1_hd6402, AY31015_TBMT ) ? 0x40 : 0;
	data |= ay31015_get_output_pin( nascom1_hd6402, AY31015_DAV ) ? 0x80 : 0;
	ay31015_set_input_pin( nascom1_hd6402, AY31015_SWE, 1 );

	return data;
}


READ8_DEVICE_HANDLER( nascom1_hd6402_si )
{
	return 1;
}


WRITE8_DEVICE_HANDLER( nascom1_hd6402_so )
{
}


DEVICE_IMAGE_LOAD( nascom1_cassette )
{
	nascom1_tape_size = image_length(image);
	nascom1_tape_image = image_ptr(image);
	if (!nascom1_tape_image)
		return INIT_FAIL;

	nascom1_tape_index = 0;
	return INIT_PASS;
}


DEVICE_IMAGE_UNLOAD( nascom1_cassette )
{
	nascom1_tape_image = NULL;
	nascom1_tape_size = nascom1_tape_index = 0;
}



/*************************************
 *
 *  Snapshots
 *
 *  ASCII .nas format
 *
 *************************************/

SNAPSHOT_LOAD( nascom1 )
{
	UINT8 line[35];

	while (image_fread(image, &line, sizeof(line)) == sizeof(line))
	{
		int addr, b0, b1, b2, b3, b4, b5, b6, b7, dummy;

		if (sscanf((char *)line, "%x %x %x %x %x %x %x %x %x %x\010\010\n",
			&addr, &b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7, &dummy) == 10)
		{
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b0);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b1);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b2);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b3);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b4);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b5);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b6);
			memory_write_byte(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM), addr++, b7);
		}
	}

	return INIT_PASS;
}



/*************************************
 *
 *  Initialization
 *
 *************************************/

MACHINE_RESET( nascom1 )
{
	nascom1_hd6402 = devtag_get_device(machine, "hd6402");

	/* Set up hd6402 pins */
	ay31015_set_input_pin( nascom1_hd6402, AY31015_SWE, 1 );

	ay31015_set_input_pin( nascom1_hd6402, AY31015_CS, 0 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_NP, 1 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_NB1, 1 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_NB2, 1 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_EPS, 1 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_TSB, 1 );
	ay31015_set_input_pin( nascom1_hd6402, AY31015_CS, 1 );
}

MACHINE_RESET( nascom2 )
{
	const device_config *fdc = devtag_get_device(machine, "wd1793");
	
	wd17xx_set_density(fdc,DEN_FM_HI);

	MACHINE_RESET_CALL(nascom1);
}

DRIVER_INIT( nascom1 )
{
	switch (mess_ram_size)
	{
	case 1 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x1400, 0x9000, 0, 0, SMH_NOP, SMH_NOP);
		break;

	case 16 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x1400, 0x4fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x5000, 0xafff, 0, 0, SMH_NOP, SMH_NOP);
		memory_set_bankptr(machine, 1, mess_ram);
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x1400, 0x8fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x9000, 0xafff, 0, 0, SMH_NOP, SMH_NOP);
		memory_set_bankptr(machine, 1, mess_ram);
		break;

	case 40 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
			0x1400, 0xafff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bankptr(machine, 1, mess_ram);
		break;
	}
}
