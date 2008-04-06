/*

Megadrive / Genesis Rewrite, Take 65498465432356345250432.3  August 06

Thanks to:
Charles Macdonald for much useful information (cgfm2.emuviews.com)

Long Description names mostly taken from the GoodGen database

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
static int is_redcliff = 0;
static int is_radica = 0;
static int is_kof99 = 0;
static int is_soulb = 0;
static int is_mjlovr = 0;
static int is_squir = 0;
static int squirrel_king_extra = 0;
static int is_smous = 0;
static int is_smb = 0;
static int is_elfwor = 0;
static int is_lionk2 = 0;
static UINT16 lion2_prot1_data, lion2_prot2_data;
static int is_rx3 = 0;
static int is_bugsl = 0;
static int is_sbub = 0;
static int is_smb2 = 0;
static int is_kof98 = 0;
static int is_kaiju = 0;

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


static DEVICE_IMAGE_LOAD( genesis_cart )
{
	unsigned char *tmpROMnew, *tmpROM;
	unsigned char *secondhalf;
	unsigned char *rawROM;
	int relocate;
	int length;
	int ptr, x;
	unsigned char *ROM;

	genesis_sram = NULL;
	genesis_sram_start = genesis_sram_len = genesis_sram_active = genesis_sram_readonly = 0;
	is_ssf2 = is_redcliff = is_radica = is_kof99 = is_soulb = is_mjlovr = is_squir = is_smous = is_elfwor = is_lionk2 = is_rx3 = is_bugsl = is_sbub = is_smb2 = is_kof98 = is_kaiju = 0;

	rawROM = memory_region(REGION_CPU1);
        ROM = rawROM /*+ 512 */;

        length = image_fread(image, rawROM + 0x2000, 0x600000);
	logerror("image length = 0x%x\n", length);

	if (genesis_isSMD(&rawROM[0x2200],(unsigned)length))	/* is this a SMD file..? */
	{
		tmpROMnew = ROM;
		tmpROM = ROM + 0x2000 + 512;

		for (ptr = 0; ptr < (0x500000) / (8192); ptr += 2)
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

		if (length == 0x500000 && !strncmp((char *)&ROM[0x0120+relocate], "SUPER STREET FIGHTER2", 21))
		{
			is_ssf2 = 1;
		}
		// detect the 'Romance of the Three Kingdoms - Battle of Red Cliffs' rom, already decoded from .mdx format
		if (length == 0x200000)
		{
		 	static unsigned char redcliffsig[] = { 0x10, 0x39, 0x00, 0x40, 0x00, 0x04}; // move.b  ($400004).l,d0
		 	if (!memcmp(&ROM[0xce560+relocate],&redcliffsig[0],sizeof(redcliffsig)))
			{
				is_redcliff = 1;
			}
		}
		// detect the Radica TV games.. these probably should be a seperate driver since they are a seperate 'console'
		if (length == 0x400000)
		{
		 	static unsigned char radicasig[] = { 0xd0, 0x30, 0x39, 0x00, 0xa1, 0x30}; // jmp (a0) move.w ($a130xx),d0

		 	if (!memcmp(&ROM[0x3c031d+relocate],&radicasig[0],sizeof(radicasig))) // ssf+gng
			{
				is_radica = 1;
			}
		 	if (!memcmp(&ROM[0x3f031d+relocate],&radicasig[0],sizeof(radicasig))) // 6in1 vol 1
			{
				is_radica = 1;
			}

		}

		// detect the King of Fighters '99 unlicensed game
		if (length == 0x300000)
		{
		 	static unsigned char kof99sig[] = { 0x20, 0x3c, 0x30, 0x00, 0x00, 0xa1}; // move.l  #$300000A1,d0

		 	if (!memcmp(&ROM[0x1fd0d2+relocate],&kof99sig[0],sizeof(kof99sig)))
			{
				is_kof99 = 1;
			}
		}

		// detect the Soul Blade unlicensed game
		if (length == 0x400000)
		{
		 	static unsigned char soulbsig[] = { 0x33, 0xfc, 0x00, 0x0c, 0x00, 0xff}; // move.w  #$C,($FF020A).l (what happens if check fails)

		 	if (!memcmp(&ROM[0x028460+relocate],&soulbsig[0],sizeof(soulbsig)))
			{
				is_soulb = 1;
			}
		}

		// detect Mahjong Lover unlicensed game
		if (length == 0x100000)
		{
		 	static unsigned char mjlovrsig[] = { 0x13, 0xf9, 0x00, 0x40, 0x00, 0x00}; // move.b  ($400000).l,($FFFF0C).l (partial)

		 	if (!memcmp(&ROM[0x01b24+relocate],&mjlovrsig[0],sizeof(mjlovrsig)))
			{
				is_mjlovr = 1;
			}
		}
		// detect Squirrel King unlicensed game
		if (length == 0x100000)
		{
		 	static unsigned char squirsig[] = { 0x26, 0x79, 0x00, 0xff, 0x00, 0xfa};

		 	if (!memcmp(&ROM[0x03b4+relocate],&squirsig[0],sizeof(squirsig)))
			{
				is_squir = 1;
			}
		}
		if (length == 0x80000)
		{
		 	static unsigned char smoussig[] = { 0x4d, 0xf9, 0x00, 0x40, 0x00, 0x02};

		 	if (!memcmp(&ROM[0x08c8+relocate],&smoussig[0],sizeof(smoussig)))
			{
				is_smous = 1;
			}
		}
		if (length >= 0x200000)
		{
			static unsigned char smb2sig[] = { 0x4e, 0xb9, 0x00, 0x0f, 0x25, 0x84};

			if (!memcmp(&ROM[0xf24d6+relocate],&smb2sig[0],sizeof(smb2sig)))
			{
				is_smb2 = 1;
			}
		}
		if (length >= 0x200000)
		{
			static unsigned char smb2sig[] = { 0x4e, 0xb9, 0x00, 0x0f, 0x25, 0x84};

			if (!memcmp(&ROM[0xf24d6+relocate],&smb2sig[0],sizeof(smb2sig)))
			{
				is_smb2 = 1;
			}
		}

		if (length == 0x200000)
		{
			static unsigned char kaijusig[] = { 0x19, 0x7c, 0x00, 0x01, 0x00, 0x00};

			if (!memcmp(&ROM[0x674e + relocate],&kaijusig[0],sizeof(kaijusig)))
			{
				is_kaiju = 1;
			}
		}

		if (length == 0x200000)
		{
			static unsigned char lionk2sig[] = { 0x26, 0x79, 0x00, 0xff, 0x00, 0xf4};

			if (!memcmp(&ROM[0x03c2+relocate],&lionk2sig[0],sizeof(lionk2sig)))
			{
				is_lionk2 = 1;
			}
		}

		if (length == 0x100000)
		{
			static unsigned char bugslsig[] = { 0x20, 0x12, 0x13, 0xc0, 0x00, 0xff};

			if (!memcmp(&ROM[0xee0d0+relocate],&bugslsig[0],sizeof(bugslsig)))
			{
				is_bugsl = 1;
			}
		}

		if (length == 0x100000 && !strncmp((char *)&ROM[0x0172+relocate], "GAME : ELF WOR", 14))
		{
			is_elfwor = 1;
		}
		if (length == 0x200000)
		{
			static unsigned char rx3sig[] = { 0x66, 0x00, 0x00, 0x0e, 0x30, 0x3c};
			if (!memcmp(&ROM[0xc8b90+relocate],&rx3sig[0],sizeof(rx3sig)))
			{
				is_rx3 = 1;
			}
		}
		if (length == 0x100000)
		{
			static unsigned char sbubsig[] = { 0x0c, 0x39, 0x00, 0x55, 0x00, 0x40}; // 	cmpi.b  #$55,($400000).l
			if (!memcmp(&ROM[0x123e4+relocate],&sbubsig[0],sizeof(sbubsig)))
			{
				is_sbub = 1;
			}
		}
		if (length == 0x200000)
		{
			static unsigned char kof98sig[] = { 0x9b, 0xfc, 0x00, 0x00, 0x4a, 0x00};
			if (!memcmp(&ROM[0x56ae2+relocate],&kof98sig[0],sizeof(kof98sig)))
			{
				is_kof98 = 1;
			}
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
		memcpy(&ROM[0x800000],&ROM[0x400000],0x100000);
		memcpy(&ROM[0x400000],&ROM[0x000000],0x400000);
	}

	if (is_kaiju)
	{
		memcpy(&ROM[0x400000],&ROM[0x000000],0x200000);
		memcpy(&ROM[0x600000],&ROM[0x000000],0x200000);
	}
	if (is_radica)
	{
		memcpy(&ROM[0x400000], &ROM[0], 0x400000); // keep a copy for later banking.. making use of huge ROM_REGION allocated to genesis driver
		memcpy(&ROM[0x800000], &ROM[0], 0x400000); // wraparound banking (from hazemd code)
	}


        /* check if cart has battery save */
	genesis_sram_len = 0;
	genesis_sram = NULL;
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
		image_battery_load(image_from_devtype_and_index(IO_CARTSLOT,0), genesis_sram, genesis_sram_len);

                if (length - 0x200 < genesis_sram_start)
		{
                        genesis_sram_active = 1;
		}

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
//	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION(0x0a00000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

ROM_START(gensvp)
//	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION(0x0a00000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

ROM_START(megadriv)
//	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION(0x0a00000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

ROM_START(megadrij)
//	ROM_REGION(0x1415000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION(0x0a00000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, REGION_CPU2, ROMREGION_ERASEFF)
ROM_END

static DEVICE_IMAGE_UNLOAD( genesis_cart )
{
	/* Write out the battery file if necessary */
	if ((genesis_sram != NULL) && (genesis_sram_len > 0))
	{
		image_battery_save(image_from_devtype_and_index(IO_CARTSLOT, 0), genesis_sram, genesis_sram_len);
	}
}

static void genesis_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(genesis_cart); break;
		case MESS_DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = NULL;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(genesis_cart); break;

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
	static int lastoffset = -1,lastdata = -1;
	UINT8 *ROM = memory_region(REGION_CPU1);

	if ((lastoffset != offset) || (lastdata != data)) {
		lastoffset = offset; lastdata = data;
		switch (offset<<1)
		{
			case 0x00: // write protect register // this is not a write protect, but seems to do nothing useful but reset bank0 after the checksum test (red screen otherwise)
				if (data == 2) {
					memcpy(ROM + 0x000000, ROM + 0x400000+(((data&0xf)-2)*0x080000), 0x080000);
				}
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
}

static WRITE16_HANDLER( g_kaiju_bank_w )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memcpy(ROM + 0x000000, ROM + 0x400000+(data&0x7f)*0x8000, 0x8000);
}

// Soulblade handler from HazeMD
READ16_HANDLER( soulb_0x400006_r )
{
	return 0xf000;
}

READ16_HANDLER( soulb_0x400002_r )
{
	return 0x9800;
}

READ16_HANDLER( soulb_0x400004_r )
{
	return 0xc900;
}

// Mahjong Lover handler from HazeMD
READ16_HANDLER( mjlovr_prot_1_r )
{
	return 0x9000;
}

READ16_HANDLER( mjlovr_prot_2_r )
{
	return 0xd300;
}

// Super Mario Bros handler from HazeMD
READ16_HANDLER( smbro_prot_r )
{
	return 0xc;
}


// Smart Mouse handler from HazeMD
READ16_HANDLER( smous_prot_r )
{
	switch (offset)
	{
		case 0: return 0x5500;
		case 1: return 0x0f00;
		case 2: return 0xaa00;
		case 3: return 0xf000;
	}

	return -1;
}

// Super Bubble Bobble MD handler from HazeMD
READ16_HANDLER( sbub_extra1_r )
{
	return 0x5500;
}

READ16_HANDLER( sbub_extra2_r )
{
	return 0x0f00;
}

// KOF99 handler from HazeMD
READ16_HANDLER( kof99_0xA13002_r )
{
	// write 02 to a13002.. shift right 1?
	return 0x01;
}

READ16_HANDLER( kof99_00A1303E_r )
{
	// write 3e to a1303e.. shift right 1?
	return 0x1f;
}

READ16_HANDLER( kof99_0xA13000_r )
{
	// no write, startup check, chinese message if != 0
	return 0x0;
}

// Radica handler from HazeMD

READ16_HANDLER( radica_bank_select )
{
	int bank = offset&0x3f;
	UINT8 *ROM = memory_region(REGION_CPU1);
	memcpy(ROM, ROM +  (bank*0x10000)+0x400000, 0x400000);
	return 0;
}

// ROTK Red Cliff handler from HazeMD

READ16_HANDLER( redclif_prot_r )
{
	return -0x56 << 8;
}

READ16_HANDLER( redclif_prot2_r )
{
	return 0x55 << 8;
}

// Squirrel King handler from HazeMD, this does not give screen garbage like HazeMD compile If you reset it twice
READ16_HANDLER( squirrel_king_extra_r )
{
	return squirrel_king_extra;

}
WRITE16_HANDLER( squirrel_king_extra_w )
{
	squirrel_king_extra = data;
}

// Lion King 2 handler from HazeMD
READ16_HANDLER( lion2_prot1_r )
{
	return lion2_prot1_data;
}

READ16_HANDLER( lion2_prot2_r )
{
	return lion2_prot2_data;
}

WRITE16_HANDLER ( lion2_prot1_w )
{
	lion2_prot1_data = data;
}

WRITE16_HANDLER ( lion2_prot2_w )
{
	lion2_prot2_data = data;
}

// Rockman X3 handler from HazeMD
READ16_HANDLER( rx3_extra_r )
{
	return 0xc;
}

// King of Fighters 98 handler from HazeMD
READ16_HANDLER( g_kof98_aa_r )
{
	return 0xaa00;
}

READ16_HANDLER( g_kof98_0a_r )
{
	return 0x0a00;
}

READ16_HANDLER( g_kof98_00_r )
{
	return 0x0000;
}

// Super Mario Bros 2 handler from HazeMD
READ16_HANDLER( smb2_extra_r )
{
	return 0xa;
}

// Bug's Life handler from HazeMD
READ16_HANDLER( bugl_extra_r )
{
	return 0x28;
}

// Elf Wor handler from HazeMD (DFJustin says the title is 'Linghuan Daoshi Super Magician')
READ16_HANDLER( elfwor_0x400000_r )
{
	return 0x5500;
}

READ16_HANDLER( elfwor_0x400002_r )
{
	return 0x0f00;
}

READ16_HANDLER( elfwor_0x400004_r )
{
	return 0xc900;
}

READ16_HANDLER( elfwor_0x400006_r )
{
	return 0x1800;
}

static WRITE16_HANDLER( genesis_TMSS_bank_w )
{
	/* this probably should do more, like make Genesis V2 'die' if the SEGA string is not written promptly */
}

static void genesis_machine_stop(running_machine *machine)
{
}

static DRIVER_INIT( gencommon )
{
	if (genesis_sram)
	{
                add_exit_callback(machine, genesis_machine_stop);
	}

	if (is_ssf2)
	{
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA130F0, 0xA130FF, 0, 0, genesis_ssf2_bank_w);
	}
	if (is_kaiju)
	{
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_kaiju_bank_w );
	}

	if (is_radica)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa1307f, 0, 0, radica_bank_select );
	}

	if (is_kof99) {
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, kof99_0xA13000_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA13002, 0xA13003, 0, 0, kof99_0xA13002_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA1303e, 0xA1303f, 0, 0, kof99_00A1303E_r );
	}

	if (is_soulb) {
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, soulb_0x400006_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, soulb_0x400002_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, soulb_0x400004_r );
	}

	if (is_redcliff)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, redclif_prot2_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, redclif_prot_r );
	}

	if (is_mjlovr)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, mjlovr_prot_1_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x401000, 0x401001, 0, 0, mjlovr_prot_2_r );
	}

	if (is_squir)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_r);
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_w);
	}
	if (is_smous)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, smous_prot_r );
	}
	if (is_smb)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa13001, 0, 0, smbro_prot_r );
	}
	if (is_smb2)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, smb2_extra_r);
	}
	if (is_kof98)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x480000, 0x480001, 0, 0, g_kof98_aa_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800e0, 0x4800e1, 0, 0, g_kof98_aa_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4824a0, 0x4824a1, 0, 0, g_kof98_aa_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x488880, 0x488881, 0, 0, g_kof98_aa_r );

		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4a8820, 0x4a8821, 0, 0, g_kof98_0a_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4f8820, 0x4f8821, 0, 0, g_kof98_00_r );
	}

	if (is_elfwor)
	{
	/* is there more to this, i can't seem to get off the first level? */
	/*
	Elf Wor (Unl) - return (0×55@0×400000 OR 0xc9@0×400004) AND (0×0f@0×400002 OR 0×18@0×400006). It is probably best to add handlers for all 4 addresses.
	*/
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, elfwor_0x400000_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, elfwor_0x400004_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, elfwor_0x400002_r );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, elfwor_0x400006_r );
	}

	if (is_lionk2)
	{
		memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, lion2_prot1_w );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, lion2_prot1_r );
		memory_install_write16_handler(0,ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, lion2_prot2_w );
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, lion2_prot2_r );
	}

	if (is_rx3)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, rx3_extra_r);
	}

	if (is_bugsl)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, bugl_extra_r);
	}

	if (is_sbub)
	{
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, sbub_extra1_r);
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, sbub_extra2_r);
}

	/* install NOP handler for TMSS */
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xA14000, 0xA14003, 0, 0, genesis_TMSS_bank_w);

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
CONS( 1993, gensvp,   genesis,	0,	megdsvp,   megdsvp, megadsvp,	genesis, "Sega",   "Genesis (USA, NTSC, w/SVP)", 0)
CONS( 1990, megadriv, genesis,	0,	megadriv,  megadri6, geneur,	genesis, "Sega",   "Mega Drive (Europe, PAL)", 0)
CONS( 1988, megadrij, genesis,	0,	megadriv,  megadri6, genjpn,	genesis, "Sega",   "Mega Drive (Japan, NTSC)", 0)
