
#include "driver.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "sound/msm5205.h"
#include "image.h"

/* the largest possible cartridge image (street fighter 2 - 2.5MB) */
#define PCE_ROM_MAXSIZE  (0x280000)
#define PCE_BRAM_SIZE	0x800

/* system RAM */
unsigned char *pce_user_ram;    /* scratch RAM at F8 */

/* CD Unit RAM */
UINT8	*pce_cd_ram;			/* 64KB RAM from a CD unit */
static UINT8	*pce_cd_bram;

static struct {
	UINT8	regs[16];
	int		bram_locked;
	int		adpcm_read_ptr;
	int		adpcm_write_ptr;
	int		adpcm_length;
} pce_cd;

struct pce_struct pce;

static UINT8 *cartridge_ram;

/* joystick related data*/

#define JOY_CLOCK   0x01
#define JOY_RESET   0x02

static int joystick_port_select;        /* internal index of joystick ports */
static int joystick_data_select;        /* which nibble of joystick data we want */

/* prototypes */
static void pce_cd_init( void );


static WRITE8_HANDLER( pce_sf2_banking_w ) {
	memory_set_bankptr( 2, memory_region(REGION_USER1) + offset * 0x080000 + 0x080000 );
	memory_set_bankptr( 3, memory_region(REGION_USER1) + offset * 0x080000 + 0x088000 );
	memory_set_bankptr( 4, memory_region(REGION_USER1) + offset * 0x080000 + 0x0D0000 );
}

static WRITE8_HANDLER( pce_cartridge_ram_w ) {
	cartridge_ram[ offset ] = data;
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
	memory_set_bankptr( 3, ROM + 0x088000 );
	memory_set_bankptr( 4, ROM + 0x0D0000 );

	/* Check for Street fighter 2 */
	if ( size == PCE_ROM_MAXSIZE ) {
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x01ff0, 0x01ff3, 0, 0, pce_sf2_banking_w );
	}

	/* Check for Populous */
	if ( ! memcmp( ROM + 0x1F26, "POPULOUS", 8 ) ) {
		cartridge_ram = auto_malloc( 0x8000 );
		memory_set_bankptr( 2, cartridge_ram );
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x080000, 0x087FFF, 0, 0, pce_cartridge_ram_w );
	}

	/* Check for CD system card */
	if ( ! memcmp( ROM + 0x3FFB6, "PC Engine CD-ROM SYSTEM", 23 ) ) {
		/* Check if 192KB additional system card ram should be used */
		if ( ! memcmp( ROM + 0x29D1, "VER. 3.", 7 ) || ! memcmp( ROM + 0x29C4, "VER. 3.", 7 ) ) {
			cartridge_ram = auto_malloc( 0x30000 );
			memory_set_bankptr( 4, cartridge_ram );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0D0000, 0x0FFFFF, 0, 0, pce_cartridge_ram_w );
		}
	}
	return 0;
}

DRIVER_INIT( pce ) {
	pce_cd_init();
	pce.io_port_options = PCE_JOY_SIG | CONST_SIG;
}

DRIVER_INIT( tg16 ) {
	pce_cd_init();
	pce.io_port_options = TG_16_JOY_SIG | CONST_SIG;
}

DRIVER_INIT( sgx ) {
	pce_cd_init();
	pce.io_port_options = PCE_JOY_SIG | CONST_SIG;
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

NVRAM_HANDLER( pce )
{
	if (read_or_write)
	{
		mame_fwrite(file, pce_cd_bram, PCE_BRAM_SIZE);
	}
	else
	{
		/* load battery backed memory from disk */
		if (file)
			mame_fread(file, pce_cd_bram, PCE_BRAM_SIZE);
	}
}

static void pce_set_cd_bram( void ) {
	memory_set_bankptr( 10, pce_cd_bram + ( pce_cd.bram_locked ? PCE_BRAM_SIZE : 0 ) );
}

static void pce_cd_reset( void ) {
}

static void pce_cd_init( void ) {
	/* Initialize pce_cd struct */
	memset( &pce_cd, 0, sizeof(pce_cd) );

	/* Initialize BRAM */
	pce_cd_bram = auto_malloc( PCE_BRAM_SIZE * 2 );
	memset( pce_cd_bram, 0, PCE_BRAM_SIZE );
	memset( pce_cd_bram + PCE_BRAM_SIZE, 0xFF, PCE_BRAM_SIZE );
	pce_cd.bram_locked = 1;
	pce_set_cd_bram();
}

WRITE8_HANDLER( pce_cd_bram_w ) {
	if ( ! pce_cd.bram_locked ) {
		pce_cd_bram[ offset ] = data;
	}
}

WRITE8_HANDLER( pce_cd_intf_w ) {
	logerror("%04X: write to CD interface offset %02X, data %02X\n", activecpu_get_pc(), offset, data );

	switch( offset & 0x0F ) {
	case 0x00:	/* CDC status */
	case 0x01:	/* CDC command / status / data */
		break;
	case 0x02:	/* ADPCM / CD control */
		/* 00 is written here during booting */
		break;
	case 0x03:	/* BRAM lock / CD status - Read Only register */
		break;
	case 0x04:	/* CD reset */
		if ( data & 0x02 ) {
			pce_cd_reset();
		}
		break;
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
		break;
	case 0x07:	/* BRAM unlock / CD status */
		if ( data & 0x80 ) {
			pce_cd.bram_locked = 0;
			pce_set_cd_bram();
		}
		break;
	case 0x08:	/* ADPCM address (LSB) / CD data */
	case 0x09:	/* ADPCM address (MSB) */
	case 0x0A:	/* ADPCM RAM data port */
	case 0x0B:	/* ADPCM DMA control */
		if ( ! ( pce_cd.regs[0x0B] & 0x02 ) && ( data & 0x02 ) ) {
			/* Start CD to ADPCM transfer */
		}
		break;
	case 0x0C:	/* ADPCM status */
		break;
	case 0x0D:	/* ADPCM address control */
		if ( ( pce_cd.regs[0x0D] & 0x80 ) && ! ( data & 0x80 ) ) {
			/* Reset ADPCM hardware */
			pce_cd.adpcm_read_ptr = 0;
			pce_cd.adpcm_write_ptr = 0;
		}
		if ( data & 0x10 ) {
			pce_cd.adpcm_length = ( pce_cd.regs[0x09] << 8 ) | pce_cd.regs[0x08];
		}
		break;
	case 0x0E:	/* ADPCM playback rate */
		break;
	case 0x0F:	/* ADPCM and CD audio fade timer */
		break;
	}

	pce_cd.regs[offset] = data;
}

READ8_HANDLER( pce_cd_intf_r ) {
	UINT8 data = pce_cd.regs[offset];

	logerror("%04X: read from CD interface offset %02X\n", activecpu_get_pc(), offset );
	switch( offset & 0x0F ) {
	case 0x00:	/* CDC status */
	case 0x01:	/* CDC command / status / data */
	case 0x02:	/* ADPCM / CD control */
		break;
	case 0x03:	/* BRAM lock / CD status */
		/* bit 4 set when CD motor is on */
		/* bit 2 set when less than half of the ADPCM data is remaining */
		pce_cd.bram_locked = 1;
		pce_set_cd_bram();
		data = 0;
		break;
	case 0x04:	/* CD reset */
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
	case 0x07:	/* BRAM unlock / CD status */
		data = ( pce_cd.bram_locked ? ( data & 0x7F ) : ( data | 0x80 ) );
		break;
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

