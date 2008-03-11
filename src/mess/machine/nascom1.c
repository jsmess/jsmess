/**********************************************************************

  machine/nascom1.c

**********************************************************************/

/* Core includes */
#include <stdio.h>
#include "driver.h"
#include "includes/nascom1.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/snapquik.h"


#define NASCOM1_KEY_RESET	0x02
#define NASCOM1_KEY_INCR	0x01
#define NASCOM1_CAS_ENABLE	0x10



/*************************************
 *
 *  Global variables
 *
 *************************************/

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

static void nascom2_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param)
{
	switch (event)
	{
	case WD17XX_IRQ_SET: nascom2_fdc.irq = 1; break;
	case WD17XX_IRQ_CLR: nascom2_fdc.irq = 0; break;
	case WD17XX_DRQ_SET: nascom2_fdc.drq = 1; break;
	case WD17XX_DRQ_CLR: nascom2_fdc.drq = 0; break;
	}
}


READ8_HANDLER( nascom2_fdc_select_r )
{
	return nascom2_fdc.select | 0xa0;
}


WRITE8_HANDLER( nascom2_fdc_select_w )
{
	nascom2_fdc.select = data;

	logerror("nascom2_fdc_select_w: %02x\n", data);

	if (data & 0x01) wd17xx_set_drive(0);
	if (data & 0x02) wd17xx_set_drive(1);
	if (data & 0x04) wd17xx_set_drive(2);
	if (data & 0x08) wd17xx_set_drive(3);
	if (data & 0x10) wd17xx_set_side((data & 0x10) >> 4);
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


DEVICE_LOAD( nascom2_floppy )
{
	int sides, sectors;

	if (!image_has_been_created(image))
	{
		switch (image_length(image))
		{
		case 80 * 2 * 2 * 8 * 256:
			sides = 2;
			sectors = 2 * 8;
			wd17xx_set_density(DEN_FM_HI);
			break;

		case 80 * 1 * 2 * 8 * 256:
			sides = 1;
			sectors = 2 * 8;
			wd17xx_set_density(DEN_FM_HI);
			break;

		default:
			return INIT_FAIL;
		}
	}
	else
	{
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image) != INIT_PASS)
	{
		return INIT_FAIL;
	}

	basicdsk_set_geometry(image, 80, sides, sectors, 256, 1, 0, FALSE);

	return INIT_PASS;
}



/*************************************
 *
 *  Keyboard
 *
 *************************************/

READ8_HANDLER ( nascom1_port_00_r )
{
	if (nascom1_portstat.stat_count < 9)
		return (readinputport (nascom1_portstat.stat_count) | ~0x7f);

	return (0xff);
}


WRITE8_HANDLER( nascom1_port_00_w )
{
	nascom1_portstat.stat_flags &= ~NASCOM1_CAS_ENABLE;
	nascom1_portstat.stat_flags |= (data & NASCOM1_CAS_ENABLE);

	if (!(data & NASCOM1_KEY_RESET)) {
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_RESET)
			nascom1_portstat.stat_count = 0;
	} else nascom1_portstat.stat_flags = NASCOM1_KEY_RESET;

	if (!(data & NASCOM1_KEY_INCR)) {
		if (nascom1_portstat.stat_flags & NASCOM1_KEY_INCR)
			nascom1_portstat.stat_count++;
	} else nascom1_portstat.stat_flags = NASCOM1_KEY_INCR;
}


WRITE8_HANDLER( nascom1_port_01_w )
{
}



/*************************************
 *
 *  Cassette
 *
 *************************************/

int	nascom1_read_cassette(void)
{
	if (nascom1_tape_image && (nascom1_tape_index < nascom1_tape_size))
		return (nascom1_tape_image[nascom1_tape_index++]);

	return 0;
}


READ8_HANDLER( nascom1_port_01_r )
{
	if (nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE)
		return (nascom1_read_cassette());

	return 0;
}


READ8_HANDLER( nascom1_port_02_r )
{
	return nascom1_portstat.stat_flags & NASCOM1_CAS_ENABLE ? 0x80 : 0x00;
}


DEVICE_LOAD( nascom1_cassette )
{
	nascom1_tape_size = image_length(image);
	nascom1_tape_image = image_ptr(image);
	if (!nascom1_tape_image)
		return INIT_FAIL;

	nascom1_tape_index = 0;
	return INIT_PASS;
}


DEVICE_UNLOAD( nascom1_cassette )
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
			program_write_byte_8(addr++, b0);
			program_write_byte_8(addr++, b1);
			program_write_byte_8(addr++, b2);
			program_write_byte_8(addr++, b3);
			program_write_byte_8(addr++, b4);
			program_write_byte_8(addr++, b5);
			program_write_byte_8(addr++, b6);
			program_write_byte_8(addr++, b7);
		}
	}

	return INIT_PASS;
}



/*************************************
 *
 *  Initialization
 *
 *************************************/

DRIVER_INIT( nascom1 )
{
	switch (mess_ram_size)
	{
	case 1 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x9000, 0, 0, MRA8_NOP, MWA8_NOP);
		break;

	case 16 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x4fff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x5000, 0xafff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_set_bankptr(1, mess_ram);
		break;

	case 32 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0x8fff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x9000, 0xafff, 0, 0, MRA8_NOP, MWA8_NOP);
		memory_set_bankptr(1, mess_ram);
		break;

	case 40 * 1024:
		memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM,
			0x1400, 0xafff, 0, 0, MRA8_BANK1, MWA8_BANK1);
		memory_set_bankptr(1, mess_ram);
		break;
	}

	wd17xx_init(machine, WD_TYPE_1793, nascom2_fdc_callback, NULL);
}
