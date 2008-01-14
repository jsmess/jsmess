/***************************************************************************

	a7800.c

	Machine file to handle emulation of the Atari 7800.

	 5-Nov-2003 npwoods		Cleanups

	14-May-2002	kubecj		Fixed Fatal Run - adding simple riot timer helped.
							maybe someone with knowledge should add full fledged
							riot emulation?

	13-May-2002 kubecj		Fixed a7800_cart_type not to be too short ;-D
							fixed for loading of bank6 cart (uh, I hope)
							fixed for loading 64k supercarts
							fixed for using PAL bios
							cart not needed when in PAL mode
							added F18 Hornet bank select type
							added Activision bank select type

***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiasound.h"
#include "hash.h"
#include "image.h"
#include "machine/6532riot.h"
#include "sound/pokey.h"
#include "sound/tiaintf.h"

#include "includes/a7800.h"

int a7800_lines;
int a7800_ispal;

/* local */
static unsigned char *a7800_cart_bkup = NULL;
static unsigned char *a7800_bios_bkup = NULL;
static int a7800_ctrl_lock;
static int a7800_ctrl_reg;
static int maria_flag;

static unsigned char *a7800_cartridge_rom;
static unsigned int a7800_cart_type;
static unsigned long a7800_cart_size;
static unsigned char a7800_stick_type;
static UINT8 *ROM;


/****** RIOT ****************************************/

/* TODO: Convert the definitions into a syntax accepting readinputportbytag */

static const struct riot6532_interface r6532_interface =
{
	input_port_0_r,
	input_port_3_r,
	NULL,
	NULL
};

static const struct riot6532_interface r6532_interface_pal =
{
	input_port_0_r,
	input_port_3_r,
	NULL,
	NULL
};

/* -----------------------------------------------------------------------
 * Driver/Machine Init
 * ----------------------------------------------------------------------- */

static void a7800_driver_init(int ispal, int lines)
{
	if (ispal)
	{
		r6532_config(0, &r6532_interface_pal),
		r6532_set_clock(0, 3546894/3);
		r6532_reset(0);
	}
	else
	{
		r6532_config(0, &r6532_interface),
		r6532_set_clock(0, 3579545/3);
		r6532_reset(0);
	}

	ROM = memory_region(REGION_CPU1);
	a7800_ispal = ispal;
	a7800_lines = lines;

	/* standard banks */
	memory_set_bankptr(5, &ROM[0x2040]);		/* RAM0 */
	memory_set_bankptr(6, &ROM[0x2140]);		/* RAM1 */
	memory_set_bankptr(7, &ROM[0x2000]);		/* MAINRAM */

	/* Brutal hack put in as a consequence of new memory system; fix this */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0480, 0x04FF, 0, 0, MRA8_BANK10);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0480, 0x04FF, 0, 0, MWA8_BANK10);
	memory_set_bankptr(10, memory_region(REGION_CPU1) + 0x0480);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1800, 0x27FF, 0, 0, MRA8_BANK11);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x1800, 0x27FF, 0, 0, MWA8_BANK11);
	memory_set_bankptr(11, memory_region(REGION_CPU1) + 0x1800);
}



DRIVER_INIT( a7800_ntsc )
{
	a7800_driver_init(FALSE, 262);
}



DRIVER_INIT( a7800_pal )
{
	a7800_driver_init(TRUE, 312);
}



MACHINE_RESET( a7800 )
{
	UINT8 *memory;

	a7800_ctrl_lock = 0;
	a7800_ctrl_reg = 0;
	maria_flag = 0;

	/* set banks to default states */
	memory = memory_region(REGION_CPU1);
	memory_set_bankptr( 1, memory + 0x4000 );
	memory_set_bankptr( 2, memory + 0x8000 );
	memory_set_bankptr( 3, memory + 0xA000 );
	memory_set_bankptr( 4, memory + 0xC000 );

	/* pokey cartridge */
	if (a7800_cart_type & 0x01)
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7FFF, 0, 0, pokey1_r);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7FFF, 0, 0, pokey1_w);
	}
}



/* ----------------------------------------------------------------------- */

#define MBANK_TYPE_ATARI 0x0000
#define MBANK_TYPE_ACTIVISION 0x0100
#define MBANK_TYPE_ABSOLUTE 0x0200

/*	Header format
0	  Header version	 - 1 byte
1..16  "ATARI7800	  "  - 16 bytes
17..48 Cart title		 - 32 bytes
49..52 data length		- 4 bytes
53..54 cart type		  - 2 bytes
	bit 0 0x01 - pokey cart
	bit 1 0x02 - supercart bank switched
	bit 2 0x04 - supercart RAM at $4000
	bit 3 0x08 - additional ROM at $4000

	bit 8-15 - Special
		0 = Normal cart
		1 = Absolute (F18 Hornet)
		2 = Activision

55	 controller 1 type  - 1 byte
56	 controller 2 type  - 1 byte
	0 = None
	1 = Joystick
	2 = Light Gun
57  0 = NTSC/1 = PAL

100..127 "ACTUAL CART DATA STARTS HERE" - 28 bytes

Versions:
	Version 0: Initial release
	Version 1: Added PAL/NTSC bit. Added Special cart byte.
			   Changed 53 bit 2, added bit 3

*/

void a7800_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 128)
		return;
	hash_compute(dest, &data[128], length - 128, functions);
}



static int a7800_verify_cart(char header[128])
{
	const char* tag = "ATARI7800";

	if( strncmp( tag, header + 1, 9 ) )
	{
		logerror("Not a valid A7800 image\n");
		return IMAGE_VERIFY_FAIL;
	}

	logerror("returning ID_OK\n");
	return IMAGE_VERIFY_PASS;
}

DEVICE_INIT( a7800_cart )
{
	UINT8	*memory;

	memory = memory_region(REGION_CPU1);
	a7800_bios_bkup = NULL;
	a7800_cart_bkup = NULL;

	/* Allocate memory for BIOS bank switching */
	a7800_bios_bkup = (UINT8*) auto_malloc(0x4000);
	if (!a7800_bios_bkup)
	{
		logerror("Could not allocate ROM memory\n");
		return INIT_FAIL;
	}

	a7800_cart_bkup = (UINT8*) auto_malloc(0x4000);
	if (!a7800_cart_bkup)
	{
		logerror("Could not allocate ROM memory\n");
		return INIT_FAIL;
	}

	/* save the BIOS so we can switch it in and out */
	memcpy( a7800_bios_bkup, memory + 0xC000, 0x4000 );

	/* defaults for PAL bios without cart */
	a7800_cart_type = 0;
	a7800_stick_type = 1;

	return INIT_PASS;
}

DEVICE_LOAD( a7800_cart )
{
	long len,start;
	unsigned char header[128];
	UINT8 *memory;

	memory = memory_region(REGION_CPU1);

	/* Load and decode the header */
	image_fread( image, header, 128 );

	/* Check the cart */
	if( a7800_verify_cart((char *)header) == IMAGE_VERIFY_FAIL)
		return INIT_FAIL;

	len =(header[49] << 24) |(header[50] << 16) |(header[51] << 8) | header[52];
	a7800_cart_size = len;

	a7800_cart_type =(header[53] << 8) | header[54];
	a7800_stick_type = header[55];
	logerror( "Cart type: %x\n", a7800_cart_type );

	/* For now, if game support stick and gun, set it to stick */
	if( a7800_stick_type == 3 )
		a7800_stick_type = 1;

	if( a7800_cart_type == 0 || a7800_cart_type == 1 )
	{
		/* Normal Cart */

		start = 0x10000 - len;
		a7800_cartridge_rom = memory + start;
		image_fread(image, a7800_cartridge_rom, len);
	}
	else if( a7800_cart_type & 0x02 )
	{
		/* Super Cart */

		/* Extra ROM at $4000 */
		if( a7800_cart_type & 0x08 )
		{
			image_fread(image, memory + 0x4000, 0x4000 );
			len -= 0x4000;
		}

		a7800_cartridge_rom = memory + 0x10000;
		image_fread(image, a7800_cartridge_rom, len);

		/* bank 0 */
		memcpy( memory + 0x8000, memory + 0x10000, 0x4000);

		/* last bank */
		memcpy( memory + 0xC000, memory + 0x10000 + len - 0x4000, 0x4000);

		/* fixed 2002/05/13 kubecj
			there was 0x08, I added also two other cases.
			Now, it loads bank n-2 to $4000 if it's empty.
		*/

		/* bank n-2	*/
		if( ! ( a7800_cart_type & 0x0D ) )
		{
			memcpy( memory + 0x4000, memory + 0x10000 + len - 0x8000, 0x4000);
		}
	}
	else if( a7800_cart_type == MBANK_TYPE_ABSOLUTE )
	{
		/* F18 Hornet */

		logerror( "Cart type: %x Absolute\n",a7800_cart_type );

		a7800_cartridge_rom = memory + 0x10000;
		image_fread(image, a7800_cartridge_rom, len );

		/* bank 0 */
		memcpy( memory + 0x4000, memory + 0x10000, 0x4000 );

		/* last bank */
		memcpy( memory + 0x8000, memory + 0x18000, 0x8000);
	}
	else if( a7800_cart_type == MBANK_TYPE_ACTIVISION )
	{
		/* Activision */

		logerror( "Cart type: %x Activision\n",a7800_cart_type );

		a7800_cartridge_rom = memory + 0x10000;
		image_fread(image, a7800_cartridge_rom, len );

		/* bank 0 */
		memcpy( memory + 0xA000, memory + 0x10000, 0x4000 );

		/* bank6 hi */
		memcpy( memory + 0x4000, memory + 0x2A000, 0x2000 );

		/* bank6 lo */
		memcpy( memory + 0x6000, memory + 0x28000, 0x2000 );

		/* bank7 hi */
		memcpy( memory + 0x8000, memory + 0x2E000, 0x2000 );

		/* bank7 lo */
		memcpy( memory + 0xE000, memory + 0x2C000, 0x2000 );

	}

	memcpy( a7800_cart_bkup, memory + 0xC000, 0x4000 );
	memcpy( memory + 0xC000, a7800_bios_bkup, 0x4000 );
	return INIT_PASS;
}


/******  TIA  *****************************************/

 READ8_HANDLER( a7800_TIA_r )
{
	switch(offset)
	{
		case 0x08:
			  return((readinputportbytag("buttons") & 0x02) << 6);
		case 0x09:
			  return((readinputportbytag("buttons") & 0x08) << 4);
		case 0x0A:
			  return((readinputportbytag("buttons") & 0x01) << 7);
		case 0x0B:
			  return((readinputportbytag("buttons") & 0x04) << 5);
		case 0x0c:
			if((readinputportbytag("buttons") & 0x08) ||(readinputportbytag("buttons") & 0x02))
				return 0x00;
			else
				return 0x80;
		case 0x0d:
			if((readinputportbytag("buttons") & 0x01) ||(readinputportbytag("buttons") & 0x04))
				return 0x00;
			else
				return 0x80;
		default:
			logerror("undefined TIA read %x\n",offset);

	}
	return 0xFF;
}

WRITE8_HANDLER( a7800_TIA_w )
{
	switch(offset) {
	case 0x01:
		if(data & 0x01)
		{
			maria_flag=1;
		}
		if(!a7800_ctrl_lock)
		{
			a7800_ctrl_lock = data & 0x01;
			a7800_ctrl_reg = data;

			if (data & 0x04)
				memcpy( ROM + 0xC000, a7800_cart_bkup, 0x4000 );
			else
				memcpy( ROM + 0xC000, a7800_bios_bkup, 0x4000 );
		}
		break;
	}
	tia_sound_w(offset,data);
	ROM[offset] = data;
}



/****** RAM Mirroring ******************************/

WRITE8_HANDLER( a7800_RAM0_w )
{
	ROM[0x2040 + offset] = data;
	ROM[0x40 + offset] = data;
}

WRITE8_HANDLER( a7800_cart_w )
{
	if(offset < 0x4000)
	{
		if(a7800_cart_type & 0x04)
		{
			ROM[0x4000 + offset] = data;
		}
		else if(a7800_cart_type & 0x01)
		{
			pokey1_w(offset,data);
		}
		else
		{
			logerror("Undefined write A: %x",offset + 0x4000);
		}
	}

	if(( a7800_cart_type & 0x02 ) &&( offset >= 0x4000 ) )
	{
		/* fix for 64kb supercart */
		if( a7800_cart_size == 0x10000 )
		{
			data &= 0x03;
		}
		else
		{
			data &= 0x07;
		}
		memory_set_bankptr(2,memory_region(REGION_CPU1) + 0x10000 + (data << 14));
		memory_set_bankptr(3,memory_region(REGION_CPU1) + 0x12000 + (data << 14));
	/*	logerror("BANK SEL: %d\n",data); */
	}
	else if(( a7800_cart_type == MBANK_TYPE_ABSOLUTE ) &&( offset == 0x4000 ) )
	{
		/* F18 Hornet */
		/*logerror( "F18 BANK SEL: %d\n", data );*/
		if( data & 1 )
		{
			memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x10000 );
		}
		else if( data & 2 )
		{
			memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x14000 );
		}
	}
	else if(( a7800_cart_type == MBANK_TYPE_ACTIVISION ) &&( offset >= 0xBF80 ) )
	{
		/* Activision */
		data = offset & 7;

		/*logerror( "Activision BANK SEL: %d\n", data );*/

		memory_set_bankptr( 3, memory_region( REGION_CPU1 ) + 0x10000 + ( data << 14 ) );
		memory_set_bankptr( 4, memory_region( REGION_CPU1 ) + 0x12000 + ( data << 14 ) );
	}
}


