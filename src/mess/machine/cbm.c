#include "driver.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"

#include "includes/cbm.h"
#include "formats/cbm_tap.h"


/***********************************************

    Input Reading - Common Components

***********************************************/

/* These are needed by c64, c65 and c128, each machine has also additional specific
components in its INTERRUPT_GEN */

/* keyboard lines */
UINT8 c64_keyline[10] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


static TIMER_CALLBACK( lightpen_tick )
{
	if ((input_port_read(machine, "CTRLSEL") & 0x07) == 0x04)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}
}

void cbm_common_interrupt( const device_config *device )
{
	int value, i;
	int controller1 = input_port_read(device->machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(device->machine, "CTRLSEL") & 0x70;
	static const char *const c64ports[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	/* Lines 0-7 : common keyboard */
	for (i = 0; i < 8; i++)
	{
		value = 0xff;
		value &= ~input_port_read(device->machine, c64ports[i]);

		/* Shift Lock is mapped on Left Shift */
		if ((i == 1) && (input_port_read(device->machine, "SPECIAL") & 0x40))
			value &= ~0x80;

		c64_keyline[i] = value;
	}


	value = 0xff;
	switch(controller1)
	{
		case 0x00:
			value &= ~input_port_read(device->machine, "JOY1_1B");			/* Joy1 Directions + Button 1 */
			break;

		case 0x01:
			if (input_port_read(device->machine, "OTHER") & 0x40)			/* Paddle2 Button */
				value &= ~0x08;
			if (input_port_read(device->machine, "OTHER") & 0x80)			/* Paddle1 Button */
				value &= ~0x04;
			break;

		case 0x02:
			if (input_port_read(device->machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(device->machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;

		case 0x03:
			value &= ~(input_port_read(device->machine, "JOY1_2B") & 0x1f);	/* Joy1 Directions + Button 1 */
			break;

		case 0x04:
/* was there any input on the lightpen? where is it mapped? */
//          if (input_port_read(device->machine, "OTHER") & 0x04)           /* Lightpen Signal */
//              value &= ?? ;
			break;

		case 0x07:
			break;

		default:
			logerror("Invalid Controller 1 Setting %d\n", controller1);
			break;
	}

	c64_keyline[8] = value;


	value = 0xff;
	switch(controller2)
	{
		case 0x00:
			value &= ~input_port_read(device->machine, "JOY2_1B");			/* Joy2 Directions + Button 1 */
			break;

		case 0x10:
			if (input_port_read(device->machine, "OTHER") & 0x10)			/* Paddle4 Button */
				value &= ~0x08;
			if (input_port_read(device->machine, "OTHER") & 0x20)			/* Paddle3 Button */
				value &= ~0x04;
			break;

		case 0x20:
			if (input_port_read(device->machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(device->machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;

		case 0x30:
			value &= ~(input_port_read(device->machine, "JOY2_2B") & 0x1f);	/* Joy2 Directions + Button 1 */
			break;

		case 0x40:
/* was there any input on the lightpen? where is it mapped? */
//          if (input_port_read(device->machine, "OTHER") & 0x04)           /* Lightpen Signal */
//              value &= ?? ;
			break;

		case 0x70:
			break;

		default:
			logerror("Invalid Controller 2 Setting %d\n", controller2);
			break;
	}

	c64_keyline[9] = value;

//  vic2_frame_interrupt does nothing so this is not necessary
//  vic2_frame_interrupt (device);

	/* check if lightpen has been chosen as input: if so, enable crosshair */
	timer_set(device->machine, attotime_zero, NULL, 0, lightpen_tick);

	set_led_status (device->machine, 1, input_port_read(device->machine, "SPECIAL") & 0x40 ? 1 : 0);		/* Shift Lock */
	set_led_status (device->machine, 0, input_port_read(device->machine, "CTRLSEL") & 0x80 ? 1 : 0);		/* Joystick Swap */
}


/***********************************************

    CIA Common Handlers

***********************************************/

/* These are shared by c64, c65 and c128. c65 and c128 also have additional specific
components (to select/read additional keyboard lines) */

/*
 *  CIA 0 - Port A
 * bits 7-0 keyboard line select
 * bits 7,6: paddle select( 01 port a, 10 port b)
 * bit 4: joystick a fire button
 * bits 3,2: Paddles port a fire button
 * bits 3-0: joystick a direction
 *
 *  CIA 0 - Port B
 * bits 7-0: keyboard raw values
 * bit 4: joystick b fire button, lightpen select
 * bits 3,2: paddle b fire buttons (left,right)
 * bits 3-0: joystick b direction
 *
 * flag cassette read input, serial request in
 * irq to irq connected
 */

UINT8 cbm_common_cia0_port_a_r( const device_config *device, UINT8 output_b )
{
	UINT8 value = 0xff;

	if (!(output_b & 0x80))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x80)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x80)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x80)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x80)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x80)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x80)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x80)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x80)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x40))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x40)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x40)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x40)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x40)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x40)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x40)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x40)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x40)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x20))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x20)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x20)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x20)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x20)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x20)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x20)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x20)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x20)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x10))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x10)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x10)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x10)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x10)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x10)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x10)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x10)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x10)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x08))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x08)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x08)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x08)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x08)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x08)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x08)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x08)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x08)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x04))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x04)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x04)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x04)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x04)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x04)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x04)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x04)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x04)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x02))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x02)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x02)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x02)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x02)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x02)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x02)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x02)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x02)) t &= ~0x01;
		value &= t;
	}

	if (!(output_b & 0x01))
	{
		UINT8 t = 0xff;
		if (!(c64_keyline[7] & 0x01)) t &= ~0x80;
		if (!(c64_keyline[6] & 0x01)) t &= ~0x40;
		if (!(c64_keyline[5] & 0x01)) t &= ~0x20;
		if (!(c64_keyline[4] & 0x01)) t &= ~0x10;
		if (!(c64_keyline[3] & 0x01)) t &= ~0x08;
		if (!(c64_keyline[2] & 0x01)) t &= ~0x04;
		if (!(c64_keyline[1] & 0x01)) t &= ~0x02;
		if (!(c64_keyline[0] & 0x01)) t &= ~0x01;
		value &= t;
	}

	if ( input_port_read(device->machine, "CTRLSEL") & 0x80 )
		value &= c64_keyline[8];
	else
		value &= c64_keyline[9];

	return value;
}

UINT8 cbm_common_cia0_port_b_r( const device_config *device, UINT8 output_a )
{
	UINT8 value = 0xff;

	if (!(output_a & 0x80)) value &= c64_keyline[7];
	if (!(output_a & 0x40)) value &= c64_keyline[6];
	if (!(output_a & 0x20)) value &= c64_keyline[5];
	if (!(output_a & 0x10)) value &= c64_keyline[4];
	if (!(output_a & 0x08)) value &= c64_keyline[3];
	if (!(output_a & 0x04)) value &= c64_keyline[2];
	if (!(output_a & 0x02)) value &= c64_keyline[1];
	if (!(output_a & 0x01)) value &= c64_keyline[0];

	if ( input_port_read(device->machine, "CTRLSEL") & 0x80 )
		value &= c64_keyline[9];
	else
		value &= c64_keyline[8];

	return value;
}


/***********************************************

    CBM Quickloads

***********************************************/


static int general_cbm_loadsnap( const device_config *image, const char *file_type, int snapshot_size,
	offs_t offset, void (*cbm_sethiaddress)(running_machine *machine, UINT16 hiaddress) )
{
	char buffer[7];
	UINT8 *data = NULL;
	UINT32 bytesread;
	UINT16 address = 0;
	int i;
	const address_space *space = cpu_get_address_space(image->machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	if (!file_type)
		goto error;

	if (!mame_stricmp(file_type, "prg"))
	{
		/* prg files */
	}
	else if (!mame_stricmp(file_type, "p00"))
	{
		/* p00 files */
		if (image_fread(image, buffer, sizeof(buffer)) != sizeof(buffer))
			goto error;
		if (memcmp(buffer, "C64File", sizeof(buffer)))
			goto error;
		image_fseek(image, 26, SEEK_SET);
		snapshot_size -= 26;
	}
	else if (!mame_stricmp(file_type, "t64"))
	{
		/* t64 files - for GB64 Single T64s loading to x0801 - header is always the same size */
		if (image_fread(image, buffer, sizeof(buffer)) != sizeof(buffer))
			goto error;
		if (memcmp(buffer, "C64 tape image file", sizeof(buffer)))
			goto error;
		image_fseek(image, 94, SEEK_SET);
		snapshot_size -= 94;
	}
	else
	{
		goto error;
	}

	image_fread(image, &address, 2);
	address = LITTLE_ENDIANIZE_INT16(address);
	if (!mame_stricmp(file_type, "t64"))
		address = 2049;
	snapshot_size -= 2;

	data = malloc(snapshot_size);
	if (!data)
		goto error;

	bytesread = image_fread(image, data, snapshot_size);
	if (bytesread != snapshot_size)
		goto error;

	for (i = 0; i < snapshot_size; i++)
		memory_write_byte(space, address + i + offset, data[i]);

	cbm_sethiaddress(image->machine, address + snapshot_size);
	free(data);
	return INIT_PASS;

error:
	if (data)
		free(data);
	return INIT_FAIL;
}

static void cbm_quick_sethiaddress( running_machine *machine, UINT16 hiaddress )
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	memory_write_byte(space, 0x31, hiaddress & 0xff);
	memory_write_byte(space, 0x2f, hiaddress & 0xff);
	memory_write_byte(space, 0x2d, hiaddress & 0xff);
	memory_write_byte(space, 0x32, hiaddress >> 8);
	memory_write_byte(space, 0x30, hiaddress >> 8);
	memory_write_byte(space, 0x2e, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_c16 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

QUICKLOAD_LOAD( cbm_c64 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

QUICKLOAD_LOAD( cbm_vc20 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_quick_sethiaddress);
}

static void cbm_pet_quick_sethiaddress( running_machine *machine, UINT16 hiaddress )
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	memory_write_byte(space, 0x2e, hiaddress & 0xff);
	memory_write_byte(space, 0x2c, hiaddress & 0xff);
	memory_write_byte(space, 0x2a, hiaddress & 0xff);
	memory_write_byte(space, 0x2f, hiaddress >> 8);
	memory_write_byte(space, 0x2d, hiaddress >> 8);
	memory_write_byte(space, 0x2b, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_pet )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_pet_quick_sethiaddress);
}

static void cbm_pet1_quick_sethiaddress(running_machine *machine, UINT16 hiaddress)
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	memory_write_byte(space, 0x80, hiaddress & 0xff);
	memory_write_byte(space, 0x7e, hiaddress & 0xff);
	memory_write_byte(space, 0x7c, hiaddress & 0xff);
	memory_write_byte(space, 0x81, hiaddress >> 8);
	memory_write_byte(space, 0x7f, hiaddress >> 8);
	memory_write_byte(space, 0x7d, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_pet1 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_pet1_quick_sethiaddress);
}

static void cbmb_quick_sethiaddress(running_machine *machine, UINT16 hiaddress)
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	memory_write_byte(space, 0xf0046, hiaddress & 0xff);
	memory_write_byte(space, 0xf0047, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbmb )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0x10000, cbmb_quick_sethiaddress);
}

QUICKLOAD_LOAD( p500 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbmb_quick_sethiaddress);
}

static void cbm_c65_quick_sethiaddress( running_machine *machine, UINT16 hiaddress )
{
	const address_space *space = cpu_get_address_space(machine->firstcpu, ADDRESS_SPACE_PROGRAM);

	memory_write_byte(space, 0x82, hiaddress & 0xff);
	memory_write_byte(space, 0x83, hiaddress >> 8);
}

QUICKLOAD_LOAD( cbm_c65 )
{
	return general_cbm_loadsnap(image, file_type, quickload_size, 0, cbm_c65_quick_sethiaddress);
}


/***********************************************

    CBM Cartridges

***********************************************/


/*  All the cartridge specific code has been moved
    to machine/ drivers. Once more informations
    surface about the cart expansions for systems
    in c65.c, c128.c, cbmb.c and pet.c, the shared
    code could be refactored to have here the
    common functions                                */



/***********************************************

    CBM Datasette Tapes

***********************************************/

const cassette_config cbm_cassette_config =
{
	cbm_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};
