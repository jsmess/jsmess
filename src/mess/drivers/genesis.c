/*

Megadrive / Genesis Rewrite, Take 65498465432356345250432.3  August 06

Thanks to:
Charles Macdonald for much useful information (cgfm2.emuviews.com)

Long Description names mostly taken from the Good Gen database

ToDo:

The Code here is terrible for now, this is just for testing
Fix HV Counter & Raster Implementation (One line errors in some games, others not working eg. Dracula)
Fix Horizontal timings (currently a kludge, currently doesn't change with resolution changes)
Add Real DMA timings (using a timer)
Add All VDP etc. Mirror addresses (not done yet as I prefer to catch odd cases for now)
Investigate other Bugs (see list below)
Rewrite (again) using cleaner, more readable and better optimized code with knowledge gained
Add support for other peripherals (6 player pad, Teamplay Adapters, Lightguns, Sega Mouse etc.)
Sort out set info, making sure all games have right manufacturers, dates etc.
Make sure everything that needs backup RAM has it setup and working
Fix Reset glitches
Add 32X / SegaCD support
Add Megaplay, Megatech support (needs SMS emulation too)
Add other obscure features (palette flicker for mid-screen CRAM changes, undocumented register behavior)
Figure out how sprite masking *really* works
Add EEprom support in games that need it

Known Issues:
    Bass Masters Classic Pro Edition (U) [!] - Sega Logo is corrupt
    Bram Stoker's Dracula (U) [!] - Doesn't Work (HV Timing)
    Double Dragon 2 - The Revenge (J) [!] - Too Slow?
    International Superstar Soccer Deluxe (E) [!] - Single line Raster Glitch
    Lemmings (JU) (REV01) [!] - Rasters off by ~7-8 lines (strange case)
    Mercs - Sometimes sound doesn't work..

    Some beta games without proper sound programs seem to crash because the z80 crashes

Known Non-Issues (confirmed on Real Genesis)
    Castlevania - Bloodlines (U) [!] - Pause text is missing on upside down level
    Blood Shot (E) (M4) [!] - corrupt texture in level 1 is correct...

----------------------

Cartridge support by Gareth S. Long

MESS adaptation by R. Belmont

*/

#include "driver.h"
#include "sound/2612intf.h"
#include "includes/genesis.h"
#include "devices/cartslot.h"
#include "../../mame/drivers/megadriv.h"

static int is_ssf2 = 0;

static UINT8 *genesis_sram;
static int genesis_sram_start;
static int genesis_sram_len = 0;
static int genesis_sram_active;
static int genesis_sram_readonly;

static READ16_HANDLER( genesis_sram_read )
{
        UINT16 retval = 0;

        offset <<= 1;

        if (genesis_sram_active)
        {
                retval |= genesis_sram[offset] << 8;
                retval |= genesis_sram[offset ^ 1] & 0xff;
        }

        return retval;
}


static WRITE16_HANDLER( genesis_sram_write )
{
        offset <<= 1;

        if (!genesis_sram_readonly)
        {
                genesis_sram[offset] = data >> 8;
                genesis_sram[offset ^ 1] = data & 0xff;
        }
}


static WRITE16_HANDLER( genesis_sram_toggle )
{
        /* unsure if this is actually supposed to toggle or just switch on?
          * Yet to encounter game that utilizes
          */
        genesis_sram_active = (data & 1) ? 1 : 0;
        genesis_sram_readonly = (data & 2) ? 1 : 0;
}

/* code taken directly from GoodGEN by Cowering */
static int genesis_isfunkySMD(unsigned char *buf,unsigned int len)
{
	/* aq quiz */
	if (!strncmp("UZ(-01  ", (const char *) &buf[0xf0], 8))
		return 1;

    /* Phelios USA redump */
	/* target earth */
	/* klax (namcot) */
	if (buf[0x2080] == ' ' && buf[0x0080] == 'S' && buf[0x2081] == 'E' && buf[0x0081] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("OL R-AEAL", (const char *) &buf[0xf0], 9))
		return 1;

    /* devilish Mahjonng Tower */
    if (!strncmp("optrEtranet", (const char *) &buf[0xf3], 11))
		return 1;

	/* golden axe 2 beta */
	if (buf[0x0100] == 0x3c && buf[0x0101] == 0 && buf[0x0102] == 0 && buf[0x0103] == 0x3c)
		return 1;

    /* omega race */
	if (!strncmp("OEARC   ", (const char *) &buf[0x90], 8))
		return 1;

    /* budokan beta */
	if ((len >= 0x6708+8) && !strncmp(" NTEBDKN", (const char *) &buf[0x6708], 8))
		return 1;

    /* cdx pro 1.8 bios */
	if (!strncmp("so fCXP", (const char *) &buf[0x2c0], 7))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("sio-Wyo ", (const char *) &buf[0x0090], 8))
		return 1;

    /* onslaught */
	if (!strncmp("SS  CAL ", (const char *) &buf[0x0088], 8))
		return 1;

    /* tram terror pirate */
	if ((len >= 0x3648 + 8) && !strncmp("SG NEPIE", (const char *) &buf[0x3648], 8))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x0007] == 0x1c && buf[0x0008] == 0x0a && buf[0x0009] == 0xb8 && buf[0x000a] == 0x0a)
		return 1;

    /*tetris pirate */
	if ((len >= 0x1cbe + 5) && !strncmp("@TTI>", (const char *) &buf[0x1cbe], 5))
		return 1;

	return 0;
}



/* code taken directly from GoodGEN by Cowering */
static int genesis_isSMD(unsigned char *buf,unsigned int len)
{
	if (buf[0x2080] == 'S' && buf[0x80] == 'E' && buf[0x2081] == 'G' && buf[0x81] == 'A')
		return 1;
	return genesis_isfunkySMD(buf,len);
}


static int device_load_genesis_cart(mess_image *image)
{
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;
	unsigned char *ROM;

	genesis_sram = NULL;
	genesis_sram_start = genesis_sram_len = genesis_sram_active = genesis_sram_readonly = is_ssf2 = 0;

	rawROM = memory_region(REGION_CPU1);
        ROM = rawROM /*+ 512 */;

        length = image_fread(image, rawROM + 0x2000, 0x600000);
	logerror("image length = 0x%x\n", length);

	if (genesis_isSMD(&rawROM[0x2200],(unsigned)length))	/* is this a SMD file..? */
	{
		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000 + 512;

		for (ptr = 0; ptr < (0x400000) / (8192); ptr += 2)
		{
			for (x = 0; x < 8192; x++)
			{
				*tmpROMnew++ = *(tmpROM + ((ptr + 1) * 8192) + x);
				*tmpROMnew++ = *(tmpROM + ((ptr + 0) * 8192) + x);
			}
		}

		relocate = 0;

	}
	else
	/* check if it's a MD file */
	if ((rawROM[0x2080] == 'E') &&
		(rawROM[0x2081] == 'A') &&
		(rawROM[0x2082] == 'M' || rawROM[0x2082] == 'G'))  /* is this a MD file..? */
	{
		tmpROMnew = malloc(length);
		secondhalf = &tmpROMnew[length >> 1];

		if (!tmpROMnew)
		{
			logerror("Memory allocation failed reading roms!\n");
			goto bad;
		}

		memcpy(tmpROMnew, ROM + 0x2000, length);

		for (ptr = 0; ptr < length; ptr += 2)
		{

			ROM[ptr] = secondhalf[ptr >> 1];
			ROM[ptr + 1] = tmpROMnew[ptr >> 1];
		}
		free(tmpROMnew);
		relocate = 0;

	}
	else
	/* BIN it is, then */
	{
		relocate = 0x2000;

		if (!strncmp((char *)&ROM[0x2120], "SUPER STREET FIGHTER2", 20))
		{
			is_ssf2 = 1;
		}
	}

	ROM = memory_region(REGION_CPU1);	/* 68000 ROM region */

 	for (ptr = 0; ptr < 0x502000; ptr += 2)		/* mangle bytes for littleendian machines */
	{
#ifdef LSB_FIRST
		int temp = ROM[relocate + ptr];

		ROM[ptr] = ROM[relocate + ptr + 1];
		ROM[ptr + 1] = temp;
#else
		ROM[ptr] = ROM[relocate + ptr];
		ROM[ptr + 1] = ROM[relocate + ptr + 1];
#endif
	}

	if (is_ssf2)
	{
		tmpROM = malloc(0x500000);
		memcpy(tmpROM, &ROM[0], 0x500000);
		memcpy(&ROM[0x400000], tmpROM, 0x500000);
		free(tmpROM);
	}

        /* check if cart has battery save */
	if (ROM[0x1b1] == 'R' && ROM[0x1b0] == 'A')
	{
		genesis_sram_start = (ROM[0x1b5] << 24 | ROM[0x1b4] << 16 | ROM[0x1b7] << 8 | ROM[0x1b6]);
		genesis_sram_len = (ROM[0x1b9] << 24 | ROM[0x1b8] << 16 | ROM[0x1bb] << 8 | ROM[0x1ba]);
	}

	if (genesis_sram_start != genesis_sram_len)
	{
                if (genesis_sram_start & 1)
                        genesis_sram_start -= 1;
                if (!(genesis_sram_len & 1))
                        genesis_sram_len += 1;
		genesis_sram_len -= (genesis_sram_start - 1);
		genesis_sram = auto_malloc (genesis_sram_len);
		memset(genesis_sram, 0, genesis_sram_len);
		printf("Attempting to load SRM . . .\n");
		image_battery_load(image, genesis_sram, genesis_sram_len);

                if (length - 0x200 < genesis_sram_start)
                        genesis_sram_active = 1;

                memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, (genesis_sram_start + genesis_sram_len - 1) & 0x3fffff, 0, 0, genesis_sram_read);

                memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, (genesis_sram_start + genesis_sram_len - 1) & 0x3fffff, 0, 0, genesis_sram_write);
                memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa130f0, 0xa130f1, 0, 0, genesis_sram_toggle);
	}

	return INIT_PASS;

bad:
	return INIT_FAIL;
}

/* we don't use the bios rom (its not needed and only provides security on early models) */

ROM_START(genesis)
	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

ROM_START(megadriv)
	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

ROM_START(megadrij)
	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

static void genesis_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_genesis_cart; break;
		case MESS_DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = NULL;	/*genesis_partialhash*/ break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smd,bin,md,gen"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(genesis)
	CONFIG_DEVICE(genesis_cartslot_getinfo)
SYSTEM_CONFIG_END

static WRITE16_HANDLER( genesis_ssf2_bank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	switch (offset<<1)
	{
		case 0x00: // write protect register
			break;
		case 0x02: /* 0x080000 - 0x0FFFFF */
			memcpy(ROM + 0x080000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x04: /* 0x100000 - 0x17FFFF */
			memcpy(ROM + 0x100000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x06: /* 0x180000 - 0x1FFFFF */
			memcpy(ROM + 0x180000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x08: /* 0x200000 - 0x27FFFF */
			memcpy(ROM + 0x200000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x0a: /* 0x280000 - 0x2FFFFF */
			memcpy(ROM + 0x280000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x0c: /* 0x300000 - 0x37FFFF */
			memcpy(ROM + 0x300000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;
		case 0x0e: /* 0x380000 - 0x3FFFFF */
			memcpy(ROM + 0x380000, ROM + 0x400000+((data&0xf)*0x080000), 0x080000);
			break;

	}
}

static void genesis_machine_stop(running_machine *machine)
{
	/* Write out the battery file if necessary */
	if ((genesis_sram != NULL) && (genesis_sram_len > 0))
		image_battery_save(image_from_devtype_and_index(IO_CARTSLOT, 0), genesis_sram, genesis_sram_len);
}


static DRIVER_INIT( gencommon )
{
	genesis_sram_len = 0;

        if (genesis_sram)
	{
                add_exit_callback(machine, genesis_machine_stop);
	}

	if (is_ssf2)
	{
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA130F0, 0xA130FF, 0, 0, genesis_ssf2_bank_w);
	}
}

static DRIVER_INIT( genusa )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadriv);
}

static DRIVER_INIT( geneur )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadrie);
}

static DRIVER_INIT( genjpn )
{
	DRIVER_INIT_CALL(gencommon);
	DRIVER_INIT_CALL(megadrij);
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE    INPUT     INIT   CONFIG   COMPANY   FULLNAME */
CONS( 1989, genesis,  0,		0,	megadriv,  megadri6, genusa,	genesis, "Sega",   "Genesis (USA, NTSC)", 0)
CONS( 1990, megadriv, genesis,	0,	megadriv,  megadri6, geneur,	genesis, "Sega",   "Mega Drive (Europe, PAL)", 0)
CONS( 1988, megadrij, genesis,	0,	megadriv,  megadri6, genjpn,	genesis, "Sega",   "Mega Drive (Japan, NTSC)", 0)
