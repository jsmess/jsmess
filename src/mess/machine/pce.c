
#include "driver.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "image.h"

/* the largest possible cartridge image (street fighter 2 - 2.5MB) */
#define PCE_ROM_MAXSIZE  (0x280000)

/* system RAM */
unsigned char *pce_user_ram;    /* scratch RAM at F8 */
UINT8	*pce_nvram;

struct pce_struct pce;

/* joystick related data*/

#define JOY_CLOCK   0x01
#define JOY_RESET   0x02

static int joystick_port_select;        /* internal index of joystick ports */
static int joystick_data_select;        /* which nibble of joystick data we want */

static WRITE8_HANDLER( pce_sf2_banking_w ) {
	memory_set_bankptr( 2, memory_region(REGION_USER1) + ( offset + 1 ) * 0x080000 );
}

DEVICE_LOAD(pce_cart)
{
	int size;
	int split_rom = 0;
	const char *extrainfo;
	unsigned char *ROM;
	logerror("*** DEVICE_LOAD(pce_cart) : %s\n", image_filename(image));

	/* open file to get size */
	new_memory_region(Machine, REGION_USER1,PCE_ROM_MAXSIZE,0);
	ROM = memory_region(REGION_USER1);

	size = image_length( image );

	/* handle header accordingly */
	if((size/512)&1)
	{
		logerror("*** DEVICE_LOAD(pce_cart) : Header present\n");
		size -= 512;
		image_fseek(image, 512, SEEK_SET);
	}
	if ( size > PCE_ROM_MAXSIZE )
		size = PCE_ROM_MAXSIZE;

	image_fread(image, ROM, size);

	extrainfo = image_extrainfo( image );
	if ( extrainfo )
	{
		logerror( "extrainfo: %s\n", extrainfo );
		if ( strstr( extrainfo, "ROM_SPLIT" ) )
			split_rom = 1;
	}

	if ( ROM[0x1FFF] < 0xE0 )
	{
		int i;
		UINT8 decrypted[256];

		logerror( "*** DEVICE_LOAD(pce_cart) : ROM image seems encrypted, decrypting...\n" );

		/* Initialize decryption table */
		for( i = 0; i < 256; i++ )
			decrypted[i] = ( ( i & 0x01 ) << 7 ) | ( ( i & 0x02 ) << 5 ) | ( ( i & 0x04 ) << 3 ) | ( ( i & 0x08 ) << 1 ) | ( ( i & 0x10 ) >> 1 ) | ( ( i & 0x20 ) >> 3 ) | ( ( i & 0x40 ) >> 5 ) | ( ( i & 0x80 ) >> 7 );

		/* Decrypt ROM image */
		for( i = 0; i < size; i++ )
			ROM[i] = decrypted[ROM[i]];
	}

	/* check if we're dealing with a split rom image */
	if ( size == 384 * 1024 )
	{
		split_rom = 1;
		/* Mirror the upper 128KB part of the image */
		memcpy( ROM + 0x060000, ROM + 0x040000, 0x020000 );	/* Set up 060000 - 07FFFF mirror */
	}

	/* set up the memory for a split rom image */
	if ( split_rom )
	{
		logerror( "Split rom detected, setting up memory accordingly\n" );
		/* Set up ROM address space as follows:          */
		/* 000000 - 03FFFF : ROM data 000000 - 03FFFF    */
		/* 040000 - 07FFFF : ROM data 000000 - 03FFFF    */
		/* 080000 - 0BFFFF : ROM data 040000 - 07FFFF    */
		/* 0C0000 - 0FFFFF : ROM data 040000 - 07FFFF    */
		memcpy( ROM + 0x080000, ROM + 0x040000, 0x040000 );	/* Set up 080000 - 0BFFFF region */
		memcpy( ROM + 0x0C0000, ROM + 0x040000, 0x040000 );	/* Set up 0C0000 - 0FFFFF region */
		memcpy( ROM + 0x040000, ROM, 0x040000 );		/* Set up 040000 - 07FFFF region */
	}
	else
	{
		/* mirror 256KB rom data */
		if ( size <= 0x040000 )
			memcpy( ROM + 0x040000, ROM, 0x040000 );

		/* mirror 512KB rom data */
		if ( size <= 0x080000 )
			memcpy( ROM + 0x080000, ROM, 0x080000 );
	}

	memory_set_bankptr( 1, ROM );
	memory_set_bankptr( 2, ROM + 0x080000 );
	memory_set_bankptr( 3, ROM + 0x100000 );

	/* Check for Street fighter 2 */
	if ( size == PCE_ROM_MAXSIZE ) {
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x01ff0, 0x01ff3, 0, 0, pce_sf2_banking_w );
	}
	return 0;
}

NVRAM_HANDLER( pce )
{
	if (read_or_write)
	{
		mame_fwrite(file, pce_nvram, 0x2000);
	}
	else
	{
	    /* load battery backed memory from disk */
		memset(pce_nvram, 0, 0x2000);
		if (file)
			mame_fread(file, pce_nvram, 0x2000);
	}
}

DRIVER_INIT( pce )
{
	pce.io_port_options = NO_CD_SIG | PCE_JOY_SIG | CONST_SIG;
}

DRIVER_INIT( tg16 )
{
	pce.io_port_options = NO_CD_SIG | TG_16_JOY_SIG | CONST_SIG;
}

/* todo: how many input ports does the PCE have? */
WRITE8_HANDLER ( pce_joystick_w )
{
	h6280io_set_buffer(data);
    /* bump counter on a low-to-high transition of bit 1 */
    if((!joystick_data_select) && (data & JOY_CLOCK))
    {
        joystick_port_select = (joystick_port_select + 1) & 0x07;
    }

    /* do we want buttons or direction? */
    joystick_data_select = data & JOY_CLOCK;

    /* clear counter if bit 2 is set */
    if(data & JOY_RESET)
    {
        joystick_port_select = 0;
    }
}

READ8_HANDLER ( pce_joystick_r )
{
	UINT8 ret;
	int data = readinputport(0);
	if(joystick_data_select) data >>= 4;
	ret = (data & 0x0F) | pce.io_port_options;
#ifdef UNIFIED_PCE
	ret &= ~0x40;
#endif
	return (ret);
}

WRITE8_HANDLER( pce_cd_intf_w ) {
	switch( offset & 0x0F ) {
	case 0x00:	/* CDC status */
	case 0x01:	/* CDC command / status / data */
	case 0x02:	/* ADPCM / CD control */
	case 0x03:	/* BRAM lock / CD status */
	case 0x04:	/* CD reset */
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
	case 0x07:	/* BRAM unlock / CD status */
	case 0x08:	/* ADPCM address (LSB) / CD data */
	case 0x09:	/* ADPCM address (MSB) */
	case 0x0A:	/* ADPCM RAM data port */
	case 0x0B:	/* ADPCM DMA control */
	case 0x0C:	/* ADPCM status */
	case 0x0D:	/* ADPCM address control */
	case 0x0E:	/* ADPCM playback rate */
	case 0x0F:	/* ADPCM and CD audio fade timer */
		break;
	}
}

READ8_HANDLER( pce_cd_intf_r ) {
	UINT8 data = 0xFF;

	switch( offset & 0x0F ) {
	case 0x00:	/* CDC status */
	case 0x01:	/* CDC command / status / data */
	case 0x02:	/* ADPCM / CD control */
	case 0x03:	/* BRAM lock / CD status */
	case 0x04:	/* CD reset */
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
	case 0x07:	/* BRAM unlock / CD status */
	case 0x08:	/* ADPCM address (LSB) / CD data */
	case 0x09:	/* ADPCM address (MSB) */
	case 0x0A:	/* ADPCM RAM data port */
	case 0x0B:	/* ADPCM DMA control */
	case 0x0C:	/* ADPCM status */
	case 0x0D:	/* ADPCM address control */
	case 0x0E:	/* ADPCM playback rate */
	case 0x0F:	/* ADPCM and CD audio fade timer */
		break;
	}

	return data;
}

