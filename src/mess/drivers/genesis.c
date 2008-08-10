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
/*#include "includes/genesis.h"*/
#include "devices/cartslot.h"
#include "../../mame/drivers/megadriv.h"
#include "cpu/m68000/m68000.h"

static UINT16 squirrel_king_extra = 0;
static UINT16 lion2_prot1_data, lion2_prot2_data;
static UINT16 realtek_bank_addr=0, realtek_bank_size=0, realtek_old_bank_addr;
static UINT16 g_l3alt_pdat;
static UINT16 g_l3alt_pcmd;

static int genesis_last_loaded_image_length = -1;

//static int megadrive_region_export = 0;
//static int megadrive_region_pal = 0;

/* where a fresh copy of rom is stashed for reset and banking setup */
#define VIRGIN_COPY_GEN 0xd00000

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

    /* devilish Mahjong Tower */
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
	unsigned char *rawROM,fliptemp;
	int relocate;
	int length;
	int ptr, x;
	unsigned char *ROM;

	genesis_sram = NULL;
	genesis_sram_start = genesis_sram_len = genesis_sram_active = genesis_sram_readonly = 0;

	rawROM = memory_region(image->machine, "main");
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

#ifdef LSB_FIRST
		tmpROMnew = ROM;
		for (ptr = 0; ptr < length; ptr += 2)
		{
			fliptemp = tmpROMnew[ptr];
			tmpROMnew[ptr] = tmpROMnew[ptr+1];
			tmpROMnew[ptr+1] = fliptemp;
		}
#endif
		genesis_last_loaded_image_length = length - 512;
		memcpy(&ROM[VIRGIN_COPY_GEN],&ROM[0x000000],0x500000);  /* store a copy of data for MACHINE_RESET processing */

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

#ifdef LSB_FIRST
		for (ptr = 0; ptr < length; ptr += 2)
		{
			fliptemp = ROM[ptr];
			ROM[ptr] = ROM[ptr+1];
			ROM[ptr+1] = fliptemp;
		}
#endif
		genesis_last_loaded_image_length = length;
		memcpy(&ROM[VIRGIN_COPY_GEN],&ROM[0x000000],0x500000);  /* store a copy of data for MACHINE_RESET processing */
		relocate = 0;

	}
	else
	/* BIN it is, then */
	{
		relocate = 0x2000;
		genesis_last_loaded_image_length = length;


		ROM = memory_region(image->machine, "main");	/* 68000 ROM region */

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
		memcpy(&ROM[VIRGIN_COPY_GEN],&ROM[0x000000],0x500000);  /* store a copy of data for MACHINE_RESET processing */
	}
	return INIT_PASS;

bad:
	return INIT_FAIL;
}
/* we don't use the bios rom (its not needed and only provides security on early models) */

ROM_START(genesis)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(gensvp)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(megadriv)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(megadrij)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
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
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(genesis_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "smd,bin,md,gen"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(genesis)
	CONFIG_DEVICE(genesis_cartslot_getinfo)
SYSTEM_CONFIG_END

static WRITE16_HANDLER( genesis_ssf2_bank_w )
{
	static int lastoffset = -1,lastdata = -1;
	UINT8 *ROM = memory_region(machine, "main");

	if ((lastoffset != offset) || (lastdata != data)) {
		lastoffset = offset; lastdata = data;
		switch (offset<<1)
		{
			case 0x00: /* write protect register // this is not a write protect, but seems to do nothing useful but reset bank0 after the checksum test (red screen otherwise) */
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

/*static WRITE16_HANDLER( g_l3alt_pdat_w )
{
	g_l3alt_pdat = data;
}

static WRITE16_HANDLER( g_l3alt_pcmd_w )
{
	g_l3alt_pcmd = data;
}
*/

static READ16_HANDLER( g_l3alt_prot_r )
{
	int retdata = 0;

	offset &=0x7;

	switch (offset)
	{

		case 2:

			switch (g_l3alt_pcmd)
			{
				case 1:
					retdata = g_l3alt_pdat>>1;
					break;

				case 2:
					retdata = g_l3alt_pdat>>4;
					retdata |= (g_l3alt_pdat&0x0f)<<4;
					break;

				default:
					/* printf("unk prot case %d\n",g_l3alt_pcmd); */
					retdata = (((g_l3alt_pdat>>7)&1)<<0);
					retdata |=(((g_l3alt_pdat>>6)&1)<<1);
					retdata |=(((g_l3alt_pdat>>5)&1)<<2);
					retdata |=(((g_l3alt_pdat>>4)&1)<<3);
					retdata |=(((g_l3alt_pdat>>3)&1)<<4);
					retdata |=(((g_l3alt_pdat>>2)&1)<<5);
					retdata |=(((g_l3alt_pdat>>1)&1)<<6);
					retdata |=(((g_l3alt_pdat>>0)&1)<<7);
					break;
			}
			break;

		default:

			printf("protection read, unknown offset\n");
			break;
	}

/*	printf("%06x: g_l3alt_pdat_w %04x g_l3alt_pcmd_w %04x return %04x\n",activecpu_get_pc(), g_l3alt_pdat,g_l3alt_pcmd,retdata); */

	return retdata;

}

static WRITE16_HANDLER( g_l3alt_protw )
{
	offset &=0x7;

	switch (offset)
	{
		case 0x0:
			g_l3alt_pdat = data;
			break;
		case 0x1:
			g_l3alt_pcmd = data;
			break;
		default:
			printf("protection write, unknown offst\n");
			break;
	}
}

static WRITE16_HANDLER( g_l3alt_bank_w )
{
	offset &=0x7;

	switch (offset)
	{
		case 0:
		{
		UINT8 *ROM = memory_region(machine, "main");
		/* printf("%06x data %04x\n",activecpu_get_pc(), data); */
		memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN + (data&0xffff)*0x8000], 0x8000);
		}
		break;

		default:
		printf("unk bank w\n");
		break;

	}

}


static WRITE16_HANDLER( realtec_402000_w )
{
	realtek_bank_addr = 0;
	realtek_bank_size = (data>>8)&0x1f;
}

static WRITE16_HANDLER( realtec_400000_w )
{
	int bankdata = (data >> 9) & 0x7;

	UINT8 *ROM = memory_region(machine, "main");

	realtek_old_bank_addr = realtek_bank_addr;
	realtek_bank_addr = (realtek_bank_addr & 0x7) | bankdata<<3;

	memcpy(ROM, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
	memcpy(ROM+ realtek_bank_size*0x20000, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
}

static WRITE16_HANDLER( realtec_404000_w )
{
	int bankdata = (data >> 8) & 0x3;
	UINT8 *ROM = memory_region(machine, "main");

	realtek_old_bank_addr = realtek_bank_addr;
	realtek_bank_addr = (realtek_bank_addr & 0xf8)|bankdata;

	if (realtek_old_bank_addr != realtek_bank_addr)
	{
		memcpy(ROM, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
		memcpy(ROM+ realtek_bank_size*0x20000, ROM +  (realtek_bank_addr*0x20000)+0x400000, realtek_bank_size*0x20000);
	}
}

static WRITE16_HANDLER( g_chifi3_bank_w )
{
	UINT8 *ROM = memory_region(machine, "main");

	if (data==0xf100) // *hit player
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x410000, 0x10000);
		}
	}
	else if (data==0xd700) // title screen..
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x470000, 0x10000);
		}
	}
	else if (data==0xd300) // character hits floor
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x430000, 0x10000);
		}
	}
	else if (data==0x0000)
	{
		int x;
		for (x=0;x<0x100000;x+=0x10000)
		{
			memcpy(ROM + x, ROM + 0x400000+x, 0x10000);
		}
	}
	else
	{
		logerror("%06x chifi3, bankw? %04x %04x\n",activecpu_get_pc(), offset,data);
	}

}

static READ16_HANDLER( g_chifi3_prot_r )
{
	UINT32 retdat;

	/* not 100% correct, there may be some relationship between the reads here
	and the writes made at the start of the game.. */

	/*
	04dc10 chifi3, prot_r? 2800
	04cefa chifi3, prot_r? 65262
	*/

	if (activecpu_get_pc()==0x01782) // makes 'VS' screen appear
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x1c24) // background gfx etc.
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x10c4a) // unknown
	{
		return mame_rand(machine);
	}
	else if (activecpu_get_pc()==0x10c50) // unknown
	{
		return mame_rand(machine);
	}
	else if (activecpu_get_pc()==0x10c52) // relates to the game speed..
	{
		retdat = activecpu_get_reg(M68K_D4)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x061ae)
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else if (activecpu_get_pc()==0x061b0)
	{
		retdat = activecpu_get_reg(M68K_D3)&0xff;
		retdat<<=8;
		return retdat;
	}
	else
	{
		logerror("%06x chifi3, prot_r? %04x\n",activecpu_get_pc(), offset);
	}

	return 0;
}

static WRITE16_HANDLER( s19in1_bank )
{
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM + 0x000000, ROM + 0x400000+((offset << 1)*0x10000), 0x80000);
}

// Kaiju? (Pokemon Stadium) handler from HazeMD
static WRITE16_HANDLER( g_kaiju_bank_w )
{
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM + 0x000000, ROM + 0x400000+(data&0x7f)*0x8000, 0x8000);
}

// Soulblade handler from HazeMD
static READ16_HANDLER( soulb_0x400006_r )
{
	return 0xf000;
}

static READ16_HANDLER( soulb_0x400002_r )
{
	return 0x9800;
}

static READ16_HANDLER( soulb_0x400004_r )
{
	return 0xc900;
}

// Mahjong Lover handler from HazeMD
static READ16_HANDLER( mjlovr_prot_1_r )
{
	return 0x9000;
}

static READ16_HANDLER( mjlovr_prot_2_r )
{
	return 0xd300;
}

// Super Mario Bros handler from HazeMD
static READ16_HANDLER( smbro_prot_r )
{
	return 0xc;
}


// Smart Mouse handler from HazeMD
static READ16_HANDLER( smous_prot_r )
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
static READ16_HANDLER( sbub_extra1_r )
{
	return 0x5500;
}

static READ16_HANDLER( sbub_extra2_r )
{
	return 0x0f00;
}

// KOF99 handler from HazeMD
static READ16_HANDLER( kof99_0xA13002_r )
{
	// write 02 to a13002.. shift right 1?
	return 0x01;
}

static READ16_HANDLER( kof99_00A1303E_r )
{
	// write 3e to a1303e.. shift right 1?
	return 0x1f;
}

static READ16_HANDLER( kof99_0xA13000_r )
{
	// no write, startup check, chinese message if != 0
	return 0x0;
}

// Radica handler from HazeMD

static READ16_HANDLER( radica_bank_select )
{
	int bank = offset&0x3f;
	UINT8 *ROM = memory_region(machine, "main");
	memcpy(ROM, ROM +  (bank*0x10000)+0x400000, 0x400000);
	return 0;
}

// ROTK Red Cliff handler from HazeMD

static READ16_HANDLER( redclif_prot_r )
{
	return -0x56 << 8;
}

static READ16_HANDLER( redclif_prot2_r )
{
	return 0x55 << 8;
}

// Squirrel King handler from HazeMD, this does not give screen garbage like HazeMD compile If you reset it twice
static READ16_HANDLER( squirrel_king_extra_r )
{
	return squirrel_king_extra;

}
static WRITE16_HANDLER( squirrel_king_extra_w )
{
	squirrel_king_extra = data;
}

// Lion King 2 handler from HazeMD
static READ16_HANDLER( lion2_prot1_r )
{
	return lion2_prot1_data;
}

static READ16_HANDLER( lion2_prot2_r )
{
	return lion2_prot2_data;
}

static WRITE16_HANDLER ( lion2_prot1_w )
{
	lion2_prot1_data = data;
}

static WRITE16_HANDLER ( lion2_prot2_w )
{
	lion2_prot2_data = data;
}

// Rockman X3 handler from HazeMD
static READ16_HANDLER( rx3_extra_r )
{
	return 0xc;
}

// King of Fighters 98 handler from HazeMD
static READ16_HANDLER( g_kof98_aa_r )
{
	return 0xaa00;
}

static READ16_HANDLER( g_kof98_0a_r )
{
	return 0x0a00;
}

static READ16_HANDLER( g_kof98_00_r )
{
	return 0x0000;
}

// Super Mario Bros 2 handler from HazeMD
static READ16_HANDLER( smb2_extra_r )
{
	return 0xa;
}

// Bug's Life handler from HazeMD
static READ16_HANDLER( bugl_extra_r )
{
	return 0x28;
}

// Elf Wor handler from HazeMD (DFJustin says the title is 'Linghuan Daoshi Super Magician')
static READ16_HANDLER( elfwor_0x400000_r )
{
	return 0x5500;
}

static READ16_HANDLER( elfwor_0x400002_r )
{
	return 0x0f00;
}

static READ16_HANDLER( elfwor_0x400004_r )
{
	return 0xc900;
}

static READ16_HANDLER( elfwor_0x400006_r )
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

// only looks for == in the compare for LSB systems
static int allendianmemcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *realbuf;

#ifdef LSB_FIRST
	unsigned char flippedbuf[64];
	unsigned int i;

	if ((n & 1) || (n > 63)) return -1 ; // don't want odd sized buffers or too large a compare
	for (i = 0; i < n; i++) flippedbuf[i] = *((char *)s2 + (i ^ 1));
	realbuf = flippedbuf;

#else

	realbuf = s2;

#endif
	return memcmp(s1,realbuf,n);
}

void setup_megadriv_custom_mappers(running_machine *machine)
{
	static int relocate = VIRGIN_COPY_GEN;
	unsigned char *ROM;
	UINT32 mirroraddr;

	ROM = memory_region(machine, "main");

	if (genesis_last_loaded_image_length == 0x500000 && !allendianmemcmp((char *)&ROM[0x0120+relocate], "SUPER STREET FIGHTER2 ", 22))
	{
		memcpy(&ROM[0x800000],&ROM[VIRGIN_COPY_GEN+0x400000],0x100000);
		memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x400000);
		memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x400000);
		memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA130F0, 0xA130FF, 0, 0, genesis_ssf2_bank_w);
	}
	/* Lion King 3 */
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char lk3sig[] = { 0x0c, 0x01, 0x00, 0x30, 0x66, 0xe4};
		if (!allendianmemcmp(&ROM[0x18c6+relocate],&lk3sig[0],sizeof(lk3sig)))
		{

			memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */
			memcpy(&ROM[0x200000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */

			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_protw );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_l3alt_bank_w );
			memory_install_read16_handler( machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_prot_r );
		}
	}

	/* Super King Kong 99 */
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char sdksig[] = { 0x48, 0xe7, 0xff, 0xfe, 0x52, 0x79};
		if (!allendianmemcmp(&ROM[0x220+relocate],&sdksig[0],sizeof(sdksig)))
		{

			memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */
			memcpy(&ROM[0x200000], &ROM[VIRGIN_COPY_GEN], 0x200000); /* default rom */

			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_protw );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_l3alt_bank_w );
			memory_install_read16_handler( machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_prot_r );
		}
	}

	/* Super Donkey Kong 99 */
	if (genesis_last_loaded_image_length == 0x300000)
	{
		static unsigned char sdksig[] = { 0x48, 0xe7, 0xff, 0xfe, 0x52, 0x79};
		if (!allendianmemcmp(&ROM[0x220+relocate],&sdksig[0],sizeof(sdksig)))
		{

			memcpy(&ROM[0x000000], &ROM[VIRGIN_COPY_GEN], 0x300000); /* default rom */
			memcpy(&ROM[0x300000], &ROM[VIRGIN_COPY_GEN], 0x100000); /* default rom */

			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_protw );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_l3alt_bank_w );
			memory_install_read16_handler( machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_l3alt_prot_r );
		}
	}

	/* detect the 'Romance of the Three Kingdoms - Battle of Red Cliffs' rom, already decoded from .mdx format */
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char redcliffsig[] = { 0x10, 0x39, 0x00, 0x40, 0x00, 0x04}; // move.b  ($400004).l,d0
		if (!allendianmemcmp(&ROM[0xce560+relocate],&redcliffsig[0],sizeof(redcliffsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, redclif_prot2_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, redclif_prot_r );
		}
	}
	/* detect the Radica TV games.. these probably should be a seperate driver since they are a seperate 'console' */
	if (genesis_last_loaded_image_length == 0x400000)
	{
		static unsigned char radicasig[] = { 0x4e, 0xd0, 0x30, 0x39, 0x00, 0xa1}; // jmp (a0) move.w ($a130xx),d0

		if (!allendianmemcmp(&ROM[0x3c031c+relocate],&radicasig[0],sizeof(radicasig)) ||
		    !allendianmemcmp(&ROM[0x3f031c+relocate],&radicasig[0],sizeof(radicasig))) // ssf+gng + radica vol1
		{
			memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x400000); // keep a copy for later banking.. making use of huge ROM_REGION allocated to genesis driver
			memcpy(&ROM[0x800000],&ROM[VIRGIN_COPY_GEN],0x400000); // wraparound banking (from hazemd code)
			memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x400000);
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa13000, 0xa1307f, 0, 0, radica_bank_select );
		}

	}

	// detect the King of Fighters '99 unlicensed game
	if (genesis_last_loaded_image_length == 0x300000)
	{
		static unsigned char kof99sig[] = { 0x20, 0x3c, 0x30, 0x00, 0x00, 0xa1}; // move.l  #$300000A1,d0

		if (!allendianmemcmp(&ROM[0x1fd0d2+relocate],&kof99sig[0],sizeof(kof99sig)))
		{
			//memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x300000);
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, kof99_0xA13000_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13002, 0xA13003, 0, 0, kof99_0xA13002_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA1303e, 0xA1303f, 0, 0, kof99_00A1303E_r );
		}
	}

	// detect the Soul Blade unlicensed game
	if (genesis_last_loaded_image_length == 0x400000)
	{
		static unsigned char soulbsig[] = { 0x33, 0xfc, 0x00, 0x0c, 0x00, 0xff}; // move.w  #$C,($FF020A).l (what happens if check fails)

		if (!allendianmemcmp(&ROM[0x028460+relocate],&soulbsig[0],sizeof(soulbsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, soulb_0x400006_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, soulb_0x400002_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, soulb_0x400004_r );
		}
	}

	// detect Mahjong Lover unlicensed game
	if (genesis_last_loaded_image_length == 0x100000)
	{
		static unsigned char mjlovrsig[] = { 0x13, 0xf9, 0x00, 0x40, 0x00, 0x00}; // move.b  ($400000).l,($FFFF0C).l (partial)

		if (!allendianmemcmp(&ROM[0x01b24+relocate],&mjlovrsig[0],sizeof(mjlovrsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, mjlovr_prot_1_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x401000, 0x401001, 0, 0, mjlovr_prot_2_r );
		}
	}
	// detect Squirrel King unlicensed game
	if (genesis_last_loaded_image_length == 0x100000)
	{
		static unsigned char squirsig[] = { 0x26, 0x79, 0x00, 0xff, 0x00, 0xfa};

		if (!allendianmemcmp(&ROM[0x03b4+relocate],&squirsig[0],sizeof(squirsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_r);
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, squirrel_king_extra_w);
		}
	}
	if (genesis_last_loaded_image_length == 0x80000)
	{
		static unsigned char smoussig[] = { 0x4d, 0xf9, 0x00, 0x40, 0x00, 0x02};

		if (!allendianmemcmp(&ROM[0x08c8+relocate],&smoussig[0],sizeof(smoussig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400007, 0, 0, smous_prot_r );
		}
	}
	if (genesis_last_loaded_image_length >= 0x200000)
	{
		static unsigned char smbsig[] = { 0x20, 0x4d, 0x41, 0x52, 0x49, 0x4f};

		if (!allendianmemcmp(&ROM[0xc8cb0+relocate],&smbsig[0],sizeof(smbsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, smbro_prot_r );
		}
	}
	if (genesis_last_loaded_image_length >= 0x200000)
	{
		static unsigned char smb2sig[] = { 0x4e, 0xb9, 0x00, 0x0f, 0x25, 0x84};

		if (!allendianmemcmp(&ROM[0xf24d6+relocate],&smb2sig[0],sizeof(smb2sig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, smb2_extra_r);
		}
	}
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char kaijusig[] = { 0x19, 0x7c, 0x00, 0x01, 0x00, 0x00};

		if (!allendianmemcmp(&ROM[0x674e + relocate],&kaijusig[0],sizeof(kaijusig)))
		{
			memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x200000);
			memcpy(&ROM[0x600000],&ROM[VIRGIN_COPY_GEN],0x200000);
			memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x200000);

			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x700000, 0x7fffff, 0, 0, g_kaiju_bank_w );
		}
	}

	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char chifi3sig[] = { 0xb6, 0x16, 0x66, 0x00, 0x00, 0x4a};

		if (!allendianmemcmp(&ROM[0x1780 + relocate],&chifi3sig[0],sizeof(chifi3sig)))
		{
			memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x200000);
			memcpy(&ROM[0x600000],&ROM[VIRGIN_COPY_GEN],0x200000);
			memcpy(&ROM[0x000000],&ROM[VIRGIN_COPY_GEN],0x200000);

			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x4fffff, 0, 0, g_chifi3_prot_r );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x600000, 0x6fffff, 0, 0, g_chifi3_bank_w );
		}
	}


	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char lionk2sig[] = { 0x26, 0x79, 0x00, 0xff, 0x00, 0xf4};

		if (!allendianmemcmp(&ROM[0x03c2+relocate],&lionk2sig[0],sizeof(lionk2sig)))
		{
			memory_install_write16_handler(machine, 0,ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, lion2_prot1_w );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, lion2_prot1_r );
			memory_install_write16_handler(machine, 0,ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, lion2_prot2_w );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, lion2_prot2_r );
		}
	}

	if (genesis_last_loaded_image_length == 0x100000)
	{
		static unsigned char bugslsig[] = { 0x20, 0x12, 0x13, 0xc0, 0x00, 0xff};

		if (!allendianmemcmp(&ROM[0xee0d0+relocate],&bugslsig[0],sizeof(bugslsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, bugl_extra_r);
		}
	}

	if (genesis_last_loaded_image_length == 0x100000 && !allendianmemcmp((char *)&ROM[0x0172+relocate], "GAME : ELF WOR", 14))
	{
	/*
	Elf Wor (Unl) - return (0x55 @ 0x400000 OR 0xc9 @ 0x400004) AND (0x0f @ 0x400002 OR 0x18 @ 0x400006). It is probably best to add handlers for all 4 addresses.
	*/
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, elfwor_0x400000_r );
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400004, 0x400005, 0, 0, elfwor_0x400004_r );
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, elfwor_0x400002_r );
		memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400006, 0x400007, 0, 0, elfwor_0x400006_r );
	}
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char rx3sig[] = { 0x66, 0x00, 0x00, 0x0e, 0x30, 0x3c};
		if (!allendianmemcmp(&ROM[0xc8b90+relocate],&rx3sig[0],sizeof(rx3sig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13001, 0, 0, rx3_extra_r);
		}
	}
	if (genesis_last_loaded_image_length == 0x100000)
	{
		static unsigned char sbubsig[] = { 0x0c, 0x39, 0x00, 0x55, 0x00, 0x40}; // 	cmpi.b  #$55,($400000).l
		if (!allendianmemcmp(&ROM[0x123e4+relocate],&sbubsig[0],sizeof(sbubsig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, sbub_extra1_r);
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, sbub_extra2_r);
		}
	}
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char kof98sig[] = { 0x9b, 0xfc, 0x00, 0x00, 0x4a, 0x00};
		if (!allendianmemcmp(&ROM[0x56ae2+relocate],&kof98sig[0],sizeof(kof98sig)))
		{
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x480000, 0x480001, 0, 0, g_kof98_aa_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4800e0, 0x4800e1, 0, 0, g_kof98_aa_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4824a0, 0x4824a1, 0, 0, g_kof98_aa_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x488880, 0x488881, 0, 0, g_kof98_aa_r );

			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4a8820, 0x4a8821, 0, 0, g_kof98_0a_r );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4f8820, 0x4f8821, 0, 0, g_kof98_00_r );
		}
	}
	if (genesis_last_loaded_image_length == 0x80000)
	{
		if (!allendianmemcmp((char *)&ROM[0x7e30e + relocate],"SEGA",4) ||
		    !allendianmemcmp((char *)&ROM[0x7e100 + relocate],"SEGA",4) ||
		    !allendianmemcmp((char *)&ROM[0x7e1e6 + relocate],"SEGA",4))  { // Whac a Critter/mallet legend, Defend the Earth, Funnyworld/ballonboy
				realtek_old_bank_addr = -1;
				/* Realtec mapper!*/
				memcpy(&ROM[0x400000],&ROM[relocate],0x80000);
				for (mirroraddr = 0; mirroraddr < 0x400000;mirroraddr+=0x2000) {
					memcpy(ROM+mirroraddr, ROM +  relocate + 0x7e000, 0x002000); /* copy last 8kb across the whole rom region */
				}

				memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, realtec_400000_w);
				memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x402000, 0x402001, 0, 0, realtec_402000_w);
				memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x404000, 0x404001, 0, 0, realtec_404000_w);
			}
	}
	// detect 'Super 19 in 1'
	if (genesis_last_loaded_image_length == 0x400000)
	{
		static unsigned char s19in1sig[] = { 0x13, 0xc0, 0x00, 0xa1, 0x30, 0x38};

		if (!allendianmemcmp(&ROM[0x1e700+relocate],&s19in1sig[0],sizeof(s19in1sig))) // super19in1
		{
			memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x400000); // allow hard reset to menu
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13039, 0, 0, s19in1_bank);
		}

	}
	// detect 'Super 15 in 1'
	if (genesis_last_loaded_image_length == 0x200000)
	{
		static unsigned char s15in1sig[] = { 0x22, 0x3c, 0x00, 0xa1, 0x30, 0x00};

		if (!allendianmemcmp(&ROM[0x17bb2+relocate],&s15in1sig[0],sizeof(s15in1sig))) // super15in1
		{
			memcpy(&ROM[0x400000],&ROM[VIRGIN_COPY_GEN],0x200000); // allow hard reset to menu
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA13000, 0xA13039, 0, 0, s19in1_bank);
		}
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

                if (genesis_last_loaded_image_length - 0x200 < genesis_sram_start)
		{
                        genesis_sram_active = 1;
		}

        memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, (genesis_sram_start + genesis_sram_len - 1) & 0x3fffff, 0, 0, genesis_sram_read);
        memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, genesis_sram_start & 0x3fffff, (genesis_sram_start + genesis_sram_len - 1) & 0x3fffff, 0, 0, genesis_sram_write);
        memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa130f0, 0xa130f1, 0, 0, genesis_sram_toggle);
	}


	/* install NOP handler for TMSS */
	memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xA14000, 0xA14003, 0, 0, genesis_TMSS_bank_w);
}


static MACHINE_RESET( ms_megadriv )
{
	MACHINE_RESET_CALL( megadriv );

	setup_megadriv_custom_mappers(machine);
}


static MACHINE_DRIVER_START( ms_megadriv )
	MDRV_IMPORT_FROM(megadriv)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ms_megdsvp )
	MDRV_IMPORT_FROM(megdsvp)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END

/***************************************************************************
 Pico Related

***************************************************************************/

ROM_START(picoe)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(picou)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

ROM_START(picoj)
	ROM_REGION(0x1415000, "main", ROMREGION_ERASEFF)
	ROM_REGION( 0x10000, "sound", ROMREGION_ERASEFF)
ROM_END

/* static DRIVER_INIT( picoeur ) */
/* { */
/* 	DRIVER_INIT_CALL(picoe); */
/* } */

/* static DRIVER_INIT( picousa ) */
/* { */
/* 	DRIVER_INIT_CALL(picou); */
/* } */

/* static DRIVER_INIT( picojpn ) */
/* { */
/* 	DRIVER_INIT_CALL(picoj); */
/* } */

/* static MACHINE_DRIVER_START( ms_pico ) */
/* 	MDRV_IMPORT_FROM(pico) */

/* 	MDRV_MACHINE_RESET( ms_megadriv ) */
/* MACHINE_DRIVER_END */


/****************************************** PICO related ****************************************/

/*
   Pico Implementation By ElBarto (Emmanuel Vadot, elbarto@megadrive.org)
   Still missing the PCM custom chip
   Some game will not boot due to this

 Pico Info from Notaz (http://notaz.gp2x.de/docs/picodoc.txt)

 addr   acc   description
-------+-----+------------
800001  byte  Version register.
              ?vv? ????, where v can be:
                00 - hardware is for Japan
                01 - European version
                10 - USA version
                11 - ?
800003  byte  Buttons, 0 for pressed, 1 for released:
                bit 0: UP (white)
                bit 1: DOWN (orange)
                bit 2: LEFT (blue)
                bit 3: RIGHT (green)
                bit 4: red button
                bit 5: unused?
                bit 6: unused?
                bit 7: pen button
800005  byte  Most significant byte of pen x coordinate.
800007  byte  Least significant byte of pen x coordinate.
800009  byte  Most significant byte of pen y coordinate.
80000b  byte  Least significant byte of pen y coordinate.
80000d  byte  Page register. One bit means one uncovered page sensor.
                00 - storyware closed
                01, 03, 07, 0f, 1f, 3f - pages 1-6
                either page 5 or page 6 is often unused.
800010  word  PCM data register.
        r/w   read returns free bytes left in PCM FIFO buffer
              writes write data to buffer.
800012  word  PCM control register.
        r/w   For writes, it has following possible meanings:
              ?p?? ???? ???? ?rrr
                p - set to enable playback?
                r - sample rate / PCM data type?
                  0: 8kHz 4bit ADPCM?
                  1-7: 16kHz variants?
              For reads, if bit 15 is cleared, it means PCM is 'busy' or
              something like that, as games sometimes wait for it to become 1.
800019  byte  Games write 'S'
80001b  byte  Games write 'E'
80001d  byte  Games write 'G'
80001f  byte  Games write 'A'

*/

#define PICO_PENX	1
#define PICO_PENY	2

static UINT16 pico_read_penpos(int pen)
{
  UINT16 penpos;

  switch (pen)
    {
    case PICO_PENX:
      penpos = input_port_read_safe(Machine, "PENX", 0);
      penpos |= 0x6;
      penpos = penpos * 320 / 255;
      penpos += 0x3d;
      break;
    case PICO_PENY:
      penpos = input_port_read_safe(Machine, "PENY", 0);
      penpos |= 0x6;
      penpos = penpos * 251 / 255;
      penpos += 0x1fc;
      break;
    }
  return penpos;
}

static READ16_HANDLER( pico_68k_io_read )
{
	UINT8 retdata;
	static UINT8 page_register = 0;


	retdata = 0;

	switch (offset)
	  {
	  case 0:	/* Version register ?XX?????? where ?? is 00 for japan, 01 for europe and 10 for USA*/
	    retdata = megadrive_region_export<<6 |
		      megadrive_region_pal<<5;
	    break;
	  case 1:
	    retdata = input_port_read_safe(Machine, "PAD", 0);
	    break;

	    /*
	       Still notes from notaz for the pen :

	       The pen can be used to 'draw' either on the drawing pad or on the storyware
	       itself. Both storyware and drawing pad are mapped on single virtual plane, where
	       coordinates range:

	       x: 0x03c - 0x17c
	       y: 0x1fc - 0x2f7 (drawing pad)
	          0x2f8 - 0x3f3 (storyware)
	     */
	  case 2:
	    retdata = pico_read_penpos(PICO_PENX) >> 8;
	    break;
	  case 3:
	    retdata = pico_read_penpos(PICO_PENX) & 0x00ff;
	    break;
	  case 4:
	    retdata = pico_read_penpos(PICO_PENY) >> 8;
	    break;
	  case 5:
	    retdata = pico_read_penpos(PICO_PENY) & 0x00ff;
	    break;
	  case 6:
	    /* Page register :
	       00 - storyware closed
	       01, 03, 07, 0f, 1f, 3f - pages 1-6
	       either page 5 or page 6 is often unused.
	    */
	    {
	      UINT8 tmp;

	      tmp = input_port_read_safe(Machine, "PAGE", 0);
	      if (tmp == 2 && page_register != 0x3f)
		{
		  page_register <<= 1;
		  page_register |= 1;
		}
	      if (tmp == 1 && page_register != 0x00)
		page_register >>= 1;
	      retdata = page_register;
	      break;
	    }
	  case 7:
	    /* Returns free bytes left in the PCM FIFO buffer */
	    retdata = 0x00;
	    break;
	  case 8:
	    /*
	       For reads, if bit 15 is cleared, it means PCM is 'busy' or
	       something like that, as games sometimes wait for it to become 1.
	    */
	    retdata = 0x00;
	  }
	return retdata | retdata <<8;
}

static WRITE16_HANDLER( pico_68k_io_write )
{
	switch (offset)
	  {
	  }
}

static ADDRESS_MAP_START( _pico_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000 , 0x3fffff) AM_READ(SMH_ROM)

	AM_RANGE(0x800000 , 0x80001f) AM_READ(pico_68k_io_read)

	AM_RANGE(0xc00000 , 0xc0001f) AM_READ(megadriv_vdp_r)
	AM_RANGE(0xe00000 , 0xe0ffff) AM_READ(SMH_RAM) AM_MIRROR(0x1f0000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( _pico_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000 , 0x3fffff) AM_WRITE(SMH_ROM)

	AM_RANGE(0x800000 , 0x80001f) AM_WRITE(pico_68k_io_write)

	AM_RANGE(0xc00000 , 0xc0001f) AM_WRITE(megadriv_vdp_w)
	AM_RANGE(0xe00000 , 0xe0ffff) AM_WRITE(SMH_RAM) AM_MIRROR(0x1f0000) AM_BASE(&megadrive_ram)
ADDRESS_MAP_END

INPUT_PORTS_START( pico )
	PORT_START("PAD")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("RED BUTTON")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("PEN BUTTON")

	PORT_START("PAGE")
        PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("Increment Page")
        PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("Decrement Page")

	PORT_START("PENX")
        PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0, 255) PORT_CATEGORY(5) PORT_PLAYER(1) PORT_NAME("PEN X")
	PORT_START("PENY")
        PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 ) PORT_CATEGORY(5) PORT_PLAYER(1) PORT_NAME("PEN Y")

	PORT_START("REGION")	/* Buttons on Genesis Console */
	/* Region setting for Console */
	PORT_DIPNAME( 0x000f, 0x0000, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0000, "Use HazeMD Default Choice" )
	PORT_DIPSETTING(      0x0001, "US (NTSC, 60fps)" )
	PORT_DIPSETTING(      0x0002, "JAPAN (NTSC, 60fps)" )
	PORT_DIPSETTING(      0x0003, "EUROPE (PAL, 50fps)" )
INPUT_PORTS_END

MACHINE_DRIVER_START( pico )
/* 	MDRV_CPU_ADD_TAG("main", M68000, MASTER_CLOCK / 7) /\* 7.67 MHz *\/ */
/* 	MDRV_CPU_PROGRAM_MAP(_pico_readmem,_pico_writemem) */
/* 	/\* IRQs are handled via the timers *\/ */

/* 	MDRV_CPU_ADD_TAG("sound", Z80, MASTER_CLOCK / 15) /\* 3.58 MHz *\/ */
/* 	MDRV_CPU_PROGRAM_MAP(z80_readmem,z80_writemem) */
/* 	MDRV_CPU_IO_MAP(z80_portmap,0) */
/* 	/\* IRQ handled via the timers *\/ */

/* 	MDRV_MACHINE_RESET(megadriv) */

/* 	MDRV_SCREEN_ADD("pico", RASTER) */
/* 	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15) */
/* 	MDRV_SCREEN_REFRESH_RATE(60) */
/* 	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0)) // Vblank handled manually. */
/* 	MDRV_SCREEN_SIZE(64*8, 64*8) */
/* 	MDRV_SCREEN_VISIBLE_AREA(0, 32*8-1, 0, 28*8-1) */

/* #ifndef MESS */
/* 	MDRV_NVRAM_HANDLER(megadriv) */
/* #endif */

/* 	MDRV_PALETTE_LENGTH(0x200) */

/* 	MDRV_VIDEO_START(megadriv) */
/* 	MDRV_VIDEO_UPDATE(megadriv) /\* Copies a bitmap *\/ */
/* 	MDRV_VIDEO_EOF(megadriv) /\* Used to Sync the timing *\/ */

/* 	MDRV_SPEAKER_STANDARD_STEREO("left", "right") */

/* 	/\* sound hardware *\/ */
/* 	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15) */
/* 	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25) /\* 3.58 MHz *\/ */
/* 	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right",0.25) /\* 3.58 MHz *\/ */
	MDRV_IMPORT_FROM(megadriv)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(_pico_readmem,_pico_writemem)

	MDRV_MACHINE_RESET( ms_megadriv )
MACHINE_DRIVER_END

DRIVER_INIT( picoe )
{
  DRIVER_INIT_CALL(gencommon);
  DRIVER_INIT_CALL(megadrie);
}

DRIVER_INIT( picou )
{
  DRIVER_INIT_CALL(gencommon);
  DRIVER_INIT_CALL(megadriv);
}

DRIVER_INIT( picoj )
{
  DRIVER_INIT_CALL(gencommon);
  DRIVER_INIT_CALL(megadrij);
}

/****************************************** END PICO related ************************************/


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE    	  INPUT     INIT  		CONFIG   COMPANY   FULLNAME */
CONS( 1989, genesis,  0,		0,      ms_megadriv,  megadri6, genusa,		genesis, "Sega",   "Genesis (USA, NTSC)", 0)
CONS( 1993, gensvp,   genesis,	0,      ms_megdsvp,   megdsvp,  megadsvp,	genesis, "Sega",   "Genesis (USA, NTSC, w/SVP)", 0)
CONS( 1990, megadriv, genesis,	0,      ms_megadriv,  megadri6, geneur,		genesis, "Sega",   "Mega Drive (Europe, PAL)", 0)
CONS( 1988, megadrij, genesis,	0,      ms_megadriv,  megadri6, genjpn,		genesis, "Sega",   "Mega Drive (Japan, NTSC)", 0)
CONS( 1994, picoe,    genesis,  0,      pico,	      pico,		picoe,		genesis, "Sega",   "Pico (Europe, PAL)", 0)
CONS( 1994, picou,    genesis,  0,      pico,	      pico,		picou,		genesis, "Sega",   "Pico (USA, NTSC)", 0)
CONS( 1993, picoj,    genesis,  0,      pico,	      pico,		picoj,		genesis, "Sega",   "Pico (Japan, NTSC)", 0)
