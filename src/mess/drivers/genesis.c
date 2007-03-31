/*

Megadrive / Genesis Rewrite, Take 65498465432356345250432.3  August 06

Thanks to:
Charles Macdonald for much useful information (cgfm2.emuviews.com)

Long Decription names mostly taken from the Good Gen database

ToDo:

The Code here is terrible for now, this is just for testing
Fix HV Counter & Raster Implementation (One line errors in some games, others not working eg. Dracula)
Fix Horizontal timings (currently a kludge, currently doesn't change with resolution changes)
Add Real DMA timings (using a timer)
Add All VDP etc. Mirror addresses (not done yet as I prefer to catch odd cases for now)
Investigate other Bugs (see list bloew)
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
#include "video/generic.h"
#include "includes/genesis.h"
#include "devices/cartslot.h"
#include "inputx.h"
#include "../../mame/drivers/megadriv.h"

static int is_ssf2 = 0;

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



static int genesis_isfunkyBIN(unsigned char *buf,unsigned int len)
{
	/* all the special cases for crappy headered roms */
	/* aq quiz */
	if (!strncmp("QUIZ (G-3051", (const char *) &buf[0x1e0], 12))
		return 1;

	/* phelios USA redump */
	/* target earth */
	/* klax namcot */
	if (buf[0x0104] == 'A' && buf[0x0101] == 'S' && buf[0x0102] == 'E' && buf[0x0103] == 'G')
		return 1;

    /* jap baseball 94 */
	if (!strncmp("WORLD PRO-B", (const char *) &buf[0x1e0], 11))
		return 1;

    /* devlish mahj tower */
	if (!strncmp("DEVILISH MAH", (const char *) &buf[0x120], 12))
		return 1;

    /* golden axe 2 beta */
	if ((len >= 0xe40a+4) && !strncmp("SEGA", (const char *) &buf[0xe40a], 4))
		return 1;

    /* omega race */
	if (!strncmp(" OMEGA RAC", (const char *) &buf[0x120], 10))
		return 1;

    /* budokan beta */
	if ((len >= 0x4e18+8) && !strncmp("BUDOKAN.", (const char *) &buf[0x4e18], 8))
		return 1;

    /* cdx 1.8 bios */
	if ((len >= 0x588+8) && !strncmp(" CDX PRO", (const char *) &buf[0x588], 8))
		return 1;

    /* ishido (hacked) */
	if (!strncmp("Ishido - ", (const char *) &buf[0x120], 9))
		return 1;

    /* onslaught */
	if (!strncmp("(C)ACLD 1991", (const char *) &buf[0x118], 12))
		return 1;

    /* trampoline terror pirate */
	if ((len >= 0x2c70+9) && !strncmp("DREAMWORK", (const char *) &buf[0x2c70], 9))
		return 1;

    /* breath of fire 3 chinese */
	if (buf[0x000f] == 0x1c && buf[0x0010] == 0x00 && buf[0x0011] == 0x0a && buf[0x0012] == 0x5c)
		return 1;

    /* tetris pirate */
	if ((len >= 0x397f+6) && !strncmp("TETRIS", (const char *) &buf[0x397f], 6))
		return 1;

    /* Ghostbusters (JUE) (REV 00) [p1] and similar */
    if ((len == 524288) && !strncmp("GHOSTBUSTERS", (const char *)&buf[0x18020], 12))
    		return 1;

    /* generic BIN, part 1 */
    if ((buf[0] == 00) && (buf[1] == 0xff) && (buf[0x100] == 0x20) && (buf[0x200] == 0x4e) && (buf[0x201] == 0x71))
		return 1;

    /* generic BIN, part 2 */
    if ((buf[4] == 0x00) && (buf[5] == 0x00) && (buf[6] == 0x02))
    		return 1;

    return 0;
}



static int genesis_isBIN(unsigned char *buf,unsigned int len)
{
	if (buf[0x0100] == 'S' && buf[0x0101] == 'E' && buf[0x0102] == 'G' && buf[0x0103] == 'A')
		return 1;
	return genesis_isfunkyBIN(buf,len);
}



static int genesis_verify_cart(unsigned char *temp,unsigned int len)
{
	int retval = IMAGE_VERIFY_FAIL;

	/* is this an SMD file..? */
	if (genesis_isSMD(&temp[0x200],len))
		retval = IMAGE_VERIFY_PASS;

	/* How about a BIN file..? */
	if ((retval == IMAGE_VERIFY_FAIL) && genesis_isBIN(&temp[0],len))
		retval = IMAGE_VERIFY_PASS;

	/* maybe a .md file? (rare) */
	if ((retval == IMAGE_VERIFY_FAIL) && (temp[0x080] == 'E') && (temp[0x081] == 'A') && (temp[0x082] == 'M' || temp[0x082] == 'G'))
		retval = IMAGE_VERIFY_PASS;

	if (retval == IMAGE_VERIFY_FAIL)
		logerror("Invalid Image!\n");

	return retval;
}

int device_load_genesis_cart(mess_image *image)
{
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;
	unsigned char *ROM;

	is_ssf2 = 0;

	rawROM = memory_region(REGION_CPU1);
        ROM = rawROM /*+ 512 */;

        length = image_fread(image, rawROM + 0x2000, 0x500200);
	logerror("image length = 0x%x\n", length);

	if (length < 1024 + 512)
	{
		goto bad;						/* smallest known rom is 1.7K */
	}

	if (genesis_verify_cart(&rawROM[0x2000],(unsigned)length) == IMAGE_VERIFY_FAIL)
	{
		goto bad;
	}

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

	return INIT_PASS;

bad:
	return INIT_FAIL;
}

/* we don't use the bios rom (its not needed and only provides security on early models) */

ROM_START(genesis)
	ROM_REGION(0x1415000, REGION_CPU1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

ROM_START(megadriv)
	ROM_REGION(0x1415000, REGION_CPU1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

ROM_START(megadrij)
	ROM_REGION(0x1415000, REGION_CPU1, 0)
	ROM_REGION( 0x10000, REGION_CPU2, 0)
ROM_END

static void genesis_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_genesis_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = NULL;	/*genesis_partialhash*/ break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smd,bin,md"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(genesis)
	CONFIG_DEVICE(genesis_cartslot_getinfo)
SYSTEM_CONFIG_END

WRITE16_HANDLER( genesis_ssf2_bank_w )
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

static DRIVER_INIT( gencommon )
{
	if (is_ssf2)
	{
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA130F0, 0xA130FF, 0, 0, genesis_ssf2_bank_w);
	}
}

static DRIVER_INIT( genusa )
{
	init_gencommon(machine);
	init_megadriv(machine);
}

static DRIVER_INIT( geneur )
{
	init_gencommon(machine);
	init_megadrie(machine);
}

static DRIVER_INIT( genjpn )
{
	init_gencommon(machine);
	init_megadrij(machine);
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE    INPUT     INIT	CONFIG	 COMPANY   FULLNAME */
CONS( 1988, genesis,  0,	0,	megadriv,  megadri6, genusa,	genesis, "Sega",   "Genesis (USA, NTSC)" , 0)
CONS( 1988, megadriv,  genesis,	0,	megadriv,  megadri6, geneur,	genesis, "Sega",   "Megadrive (Europe, PAL)" , 0)
CONS( 1988, megadrij,  genesis,	0,	megadriv,  megadri6, genjpn,	genesis, "Sega",   "Megadrive (Japan, NTSC)" , 0)
