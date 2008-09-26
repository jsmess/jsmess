/***************************************************************************

  snes.c

  Machine file to handle cart loading in the Nintendo Super NES emulation.

***************************************************************************/

#include "driver.h"
#include "includes/snes.h"
#include "devices/cartslot.h"

static int cart_type;

/***************************************************************************

  SRAM handling

***************************************************************************/

/* Loads the battery backed RAM into the appropriate memory area */
static void snes_load_sram(void)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = malloc_or_die( snes_cart.sram_max );
	ptr = battery_ram;
	image_battery_load( image_from_devtype_and_index(IO_CARTSLOT,0), battery_ram, snes_cart.sram_max );

	if( snes_cart.mode == SNES_MODE_20 )
	{
		for( ii = 0; ii < 8; ii++ )
		{
			memmove( &snes_ram[0x700000 + (ii * 0x010000)], ptr, 0x7fff );
			ptr += 0x7fff;
		}
	}
	else
	{
		for( ii = 0; ii < 16; ii++ )
		{
			memmove( &snes_ram[0x306000 + (ii * 0x010000)], ptr, 0x1fff );
			ptr += 0x1fff;
		}
	}

	free( battery_ram );
}

/* Saves the battery backed RAM from the appropriate memory area */
static void snes_save_sram(void)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = malloc_or_die( snes_cart.sram_max );
	ptr = battery_ram;

	if( snes_cart.mode == SNES_MODE_20 )
	{
		for( ii = 0; ii < 8; ii++ )
		{
			memmove( ptr, &snes_ram[0x700000 + (ii * 0x010000)], 0x8000 );
			ptr += 0x8000;
		}
	}
	else
	{
		for( ii = 0; ii < 16; ii++ )
		{
			memmove( ptr, &snes_ram[0x306000 + (ii * 0x010000)], 0x2000 );
			ptr += 0x2000;
		}
	}

	image_battery_save( image_from_devtype_and_index(IO_CARTSLOT,0), battery_ram, snes_cart.sram_max );

	free( battery_ram );
}

static void snes_machine_stop(running_machine *machine)
{
	/* Save SRAM */
	if( snes_cart.sram > 0 )
		snes_save_sram();
}

MACHINE_START( snes_mess )
{
	add_exit_callback(machine, snes_machine_stop);
	MACHINE_START_CALL(snes);
}


/***************************************************************************

  Cart handling

***************************************************************************/

static DEVICE_IMAGE_LOAD( snes_cart )
{
	int i, supported_type = 0;
	running_machine *machine = image->machine;
	UINT16 totalblocks, readblocks;
	UINT32 offset;
	UINT8 header[512], sample[0xffff];
	UINT8 valid_mode20, valid_mode21;

	enum
	{
		STANDARD = 0,
		STANDARD_RAM,
		STANDARD_SRAM,
		DSP1,
		DSP1_RAM,
		DSP1_SRAM,
		DSP2,
		DSP3,
		DSP4,
		SUPERFX,
		SUPERFX_SRAM,
		SA1,
		SA1_SRAM,
		SDD1,
		SDD1_SRAM,
		OBC1,
		RTC,
		SGB,	
		C4,
		ST010,
		ST011,
		ST018,
		SPC7110,
		SPC7110_RTC,
		UNKNOWN,
	};


	/* Some known type of cart */
	const char *types[] =
	{
		"ROM",
		"ROM, RAM",
		"ROM, SRAM",
		"ROM, DSP-1",
		"ROM, RAM, DSP-1",
		"ROM, SRAM, DSP-1",
		"ROM, SRAM, DSP-2",
		"ROM, SRAM, DSP-3",
		"ROM, DSP-4",
		"ROM, Super FX / FX2",
		"ROM, SRAM, Super FX / FX2",
		"ROM, SA-1",
		"ROM, SRAM, SA-1",
		"ROM, S-DD1",
		"ROM, SRAM, S-DD1",
		"ROM, SRAM, OBC-1",
		"ROM, SRAM, S-RTC",
		"ROM, Z80GB (Super Game Boy)",
		"ROM, C4",
		"ROM, SRAM, Seta ST-010",
		"ROM, SRAM, Seta ST-011",
		"ROM, SRAM, Seta ST-018",
		"ROM, SRAM, SPC7110",
		"ROM, SRAM, SPC7110, RTC",
		"Unknown",							// to add: Satellaview BS-X detection
	};

	/* Some known countries */
	const char *countries[] =
	{
		"Japan (NTSC)",
		"USA (NTSC)",
		"Australia, Europe, Oceania & Asia (PAL)",
		"Sweden (PAL)",
		"Finland (PAL)",
		"Denmark (PAL)",
		"France (PAL)",
		"Holland (PAL)",
		"Spain (PAL)",
		"Germany, Austria & Switzerland (PAL)",
		"Italy (PAL)",
		"Hong Kong & China (PAL)",
		"Indonesia (PAL)",
		"South Korea (NTSC)",
		"UNKNOWN"
	};

	memory_region_alloc(machine, "main", 0x1000000,0);

	snes_ram = memory_region( machine, "main" );
	memset( snes_ram, 0, 0x1000000 );

	/* Check for a header (512 bytes) */
	offset = 512;
	image_fread( image, header, 512 );
	if( (header[8] == 0xaa) && (header[9] == 0xbb) && (header[10] == 0x04) )
	{
		/* Found an SWC identifier */
		logerror( "Found header(SWC) - Skipped\n" );
	}
	else if( (header[0] | (header[1] << 8)) == (((image_length(image) - 512) / 1024) / 8) )
	{
		/* Some headers have the rom size at the start, if this matches with the
         * actual rom size, we probably have a header */
		logerror( "Found header(size) - Skipped\n" );
	}
	else if( (image_length(image) % 0x8000) == 512 )
	{
		/* As a last check we'll see if there's exactly 512 bytes extra to this
         * image. */
		logerror( "Found header(extra) - Skipped\n" );
	}
	else
	{
		/* No header found so go back to the start of the file */
		logerror( "No header found.\n" );
		offset = 0;
		image_fseek( image, offset, SEEK_SET );
	}

	/* We need to take a sample of 128kb to test what mode we need to be in */
	image_fread( image, sample, 0xffff );
	image_fseek( image, offset, SEEK_SET );	/* Rewind */

	/* Now to determine if this is a lo-ROM or a hi-ROM */
	valid_mode20 = snes_validate_infoblock( sample, 0x7fc0 );
	valid_mode21 = snes_validate_infoblock( sample, 0xffc0 );
	if( valid_mode20 >= valid_mode21 )
	{
		snes_cart.mode = SNES_MODE_20;
		snes_cart.sram_max = 0x40000;
	}
	else if( valid_mode21 > valid_mode20 )
	{
		snes_cart.mode = SNES_MODE_21;
		snes_cart.sram_max = 0x20000;
	}

	/* Find the number of blocks in this ROM */
	totalblocks = ((image_length(image) - offset) >> (snes_cart.mode == SNES_MODE_20 ? 15 : 16));

	/* FIXME: Insert crc check here */

	readblocks = 0;
	if( snes_cart.mode == SNES_MODE_20 )
	{
		/* In mode 20, all blocks are 32kb. There are upto 96 blocks, giving a
         * total of 24mbit(3mb) of ROM.
         * The first 48 blocks are located in banks 0x00 to 0x2f at address
         * 0x8000.  They are mirrored in banks 0x80 to 0xaf.
         * The next 16 blocks are located in banks 0x30 to 0x3f at address
         * 0x8000.  They are mirrored in banks 0xb0 to 0xbf.
         * The final 32 blocks are located in banks 0x40 - 0x5f at address
         * 0x8000.  They are mirrored in banks 0xc0 to 0xdf.
         */
		i = 0;
		while( i < 96 && readblocks <= totalblocks )
		{
			image_fread( image, &snes_ram[(i++ * 0x10000) + 0x8000], 0x8000);
			readblocks++;
		}
	}
	else	/* Mode 21 */
	{
		/* In mode 21, all blocks are 64kb. There are upto 96 blocks, giving a
         * total of 48mbit(6mb) of ROM.
         * The first 64 blocks are located in banks 0xc0 to 0xff. The top 32k of
         * each bank is mirrored in banks 0x00 to 0x3f.
         * The final 32 blocks are located in banks 0x40 to 0x5f.
         */

		/* read first 64 blocks */
		i = 0;
		while( i < 64 && readblocks <= totalblocks )
		{
			image_fread( image, &snes_ram[0xc00000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
		/* read the next 32 blocks */
		i = 0;
		while( i < 32 && readblocks <= totalblocks )
		{
			image_fread( image, &snes_ram[0x400000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
	}


	/* Find the amount of sram */
	snes_cart.sram = snes_r_bank1(machine, 0x00ffd8);
	if( snes_cart.sram > 0 )
	{
		snes_cart.sram = ((1 << (snes_cart.sram + 3)) / 8);
		if( snes_cart.sram > snes_cart.sram_max )
			snes_cart.sram = snes_cart.sram_max;
	}

	/* Find the type of cart and detect special chips */
	/* Info mostly taken from http://snesemu.black-ship.net/misc/-from%20nsrt.edgeemu.com-chipinfo.htm */
	switch (snes_r_bank1(machine, 0x00ffd6))
	{
			case 0x00:
				cart_type = STANDARD;
				supported_type = 1;
				break;

			case 0x01:
				cart_type = STANDARD_RAM;
				supported_type = 1;
				break;

			case 0x02:
				cart_type = STANDARD_SRAM;
				supported_type = 1;
				break;

			case 0x03:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x30)
					cart_type = DSP4;
				else
				{
					cart_type = DSP1;
					supported_type = 1;
				}
				break;

			case 0x04:
				cart_type = DSP1_RAM;
				supported_type = 1;
				break;

			case 0x05:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x20)
					cart_type = DSP2;
				/* DSP-3 is hard to detect. We exploit the fact that the only game has been manufactured by Bandai */
				else if ((snes_r_bank1(machine, 0x00ffd5) == 0x30) && (snes_r_bank1(machine, 0x00ffda) == 0xb2))
					cart_type = DSP3;
				else
				{
					cart_type = DSP1_SRAM;
					supported_type = 1;
				}
				break;

			case 0x13:	// Mario Chip 1
			case 0x14:	// GSU-x
				if (snes_r_bank1(machine, 0x00ffd5) == 0x20)
					cart_type = SUPERFX;
				break;

			case 0x15:	// GSU-x
			case 0x1a:	// GSU-1 (21 Mhz at start)
				if (snes_r_bank1(machine, 0x00ffd5) == 0x20)
					cart_type = SUPERFX_SRAM;
				break;

			case 0x25:
				cart_type = OBC1;
				break;

			case 0x34:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x23)
					cart_type = SA1;
				break;

			case 0x32:	// needed by a Sample game (according to ZSNES)
			case 0x35:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x23)
					cart_type = SA1_SRAM;
				break;

			case 0x43:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x32)
					cart_type = SDD1;
				break;

			case 0x45:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x32)
					cart_type = SDD1_SRAM;
				break;

			case 0x55:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x35)
					cart_type = RTC;
				break;

			case 0xe3:
				cart_type = SGB;
				break;

			case 0xf3:
				cart_type = C4;
				break;
			
			case 0xf5:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x30)
					cart_type = ST018;
				else if (snes_r_bank1(machine, 0x00ffd5) == 0x3a)
					cart_type = SPC7110;
				break;

			case 0xf6:
				/* These Seta ST-01X chips have both 0x30 at 0x00ffd5, 
				they only differ for the 'size' at 0x00ffd7 */
				if (snes_r_bank1(machine, 0x00ffd7) < 0x0a)
					cart_type = ST011;
				else
					cart_type = ST010;
				break;

			case 0xf9:
				if (snes_r_bank1(machine, 0x00ffd5) == 0x3a)
					cart_type = SPC7110_RTC;
				break;
			
			default:
				cart_type = UNKNOWN;
				break;
	}

	/* Log snes_cart information */
	{
		char title[21], rom_id[4], company_id[2];
		UINT8 country;
		logerror( "ROM DETAILS\n" );
		logerror( "\tTotal blocks:  %d (%dmb)\n", totalblocks, totalblocks / (snes_cart.mode == SNES_MODE_20 ? 32 : 16) );
		logerror( "\tROM bank size: %s (LoROM: %d , HiROM: %d)\n", (snes_cart.mode == SNES_MODE_20) ? "LoROM" : "HiROM", valid_mode20, valid_mode21 );
		for( i = 0; i < 2; i++ )
			company_id[i] = snes_r_bank1(machine, 0x00ffb0 + i);
		logerror( "\tCompany ID:    %s\n", company_id );
		for( i = 0; i < 4; i++ )
			rom_id[i] = snes_r_bank1(machine, 0x00ffb2 + i);
		logerror( "\tROM ID:        %s\n", rom_id );
		logerror( "HEADER DETAILS\n" );
		for( i = 0; i < 21; i++ )
			title[i] = snes_r_bank1(machine, 0x00ffc0 + i);
		logerror( "\tName:          %s\n", title );
		logerror( "\tSpeed:         %s [%d]\n", ((snes_r_bank1(machine, 0x00ffd5) & 0xf0)) ? "FastROM" : "SlowROM", (snes_r_bank1(machine, 0x00ffd5) & 0xf0) >> 4 );
		logerror( "\tBank size:     %s [%d]\n", (snes_r_bank1(machine, 0x00ffd5) & 0xf) ? "HiROM" : "LoROM", snes_r_bank1(machine, 0x00ffd5) & 0xf );
		logerror( "\tType:          %s [%d]\n", types[cart_type], snes_r_bank1(machine, 0x00ffd6) );
		logerror( "\tSize:          %d megabits [%d]\n", 1 << (snes_r_bank1(machine, 0x00ffd7) - 7), snes_r_bank1(machine, 0x00ffd7) );
		logerror( "\tSRAM:          %d kilobits [%d]\n", snes_cart.sram * 8, snes_ram[0xffd8] );
		country = snes_r_bank1(machine, 0x00ffd9);
		if( country > 14 )
			country = 14;
		logerror( "\tCountry:       %s [%d]\n", countries[country], snes_r_bank1(machine, 0x00ffd9) );
		logerror( "\tLicense:       %s [%X]\n", "", snes_r_bank1(machine, 0x00ffda) );
		logerror( "\tVersion:       1.%d\n", snes_r_bank1(machine, 0x00ffdb) );
		logerror( "\tInv Checksum:  %X %X\n", snes_r_bank1(machine, 0x00ffdd), snes_r_bank1(machine, 0x00ffdc) );
		logerror( "\tChecksum:      %X %X\n", snes_r_bank1(machine, 0x00ffdf), snes_r_bank1(machine, 0x00ffde) );
		logerror( "\tNMI Address:   %2X%2Xh\n", snes_r_bank1(machine, 0x00fffb), snes_r_bank1(machine, 0x00fffa) );
		logerror( "\tStart Address: %2X%2Xh\n", snes_r_bank1(machine, 0x00fffd), snes_r_bank1(machine, 0x00fffc) );

		if (!supported_type)
			logerror("WARNING: This cart type \"%s\" is not supported yet!\n", types[cart_type]);
	}

	/* Load SRAM */
	snes_load_sram();

	/* All done */
	return INIT_PASS;
}


void snes_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(snes_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smc,sfc,fig,swc"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}
