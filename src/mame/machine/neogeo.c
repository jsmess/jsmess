#include "driver.h"
#include "machine/pd4990a.h"
#include "neogeo.h"
#include "sound/2610intf.h"


extern INT32 neogeo_sram_locked;

extern INT32 neogeo_rng;
extern INT32 neogeo_prot_data;

UINT16 *neogeo_ram16;
UINT16 *neogeo_sram16;


/***************** MEMCARD GLOBAL VARIABLES ******************/
UINT8 *neogeo_memcard;		/* Pointer to 2kb RAM zone */

UINT8 *neogeo_game_vectors;

static void neogeo_register_sub_savestate(void);


/* This function is called on every reset */
MACHINE_RESET( neogeo )
{
	mame_system_time systime;


	/* Reset variables & RAM */
	memset (neogeo_ram16, 0, 0x10000);



	mame_get_base_datetime(Machine, &systime);

	pd4990a.seconds = ((systime.local_time.second/10)<<4) + (systime.local_time.second%10);
	pd4990a.minutes = ((systime.local_time.minute/10)<<4) + (systime.local_time.minute%10);
	pd4990a.hours = ((systime.local_time.hour/10)<<4) + (systime.local_time.hour%10);
	pd4990a.days = ((systime.local_time.mday/10)<<4) + (systime.local_time.mday%10);
	pd4990a.month = (systime.local_time.month + 1);
	pd4990a.year = ((((systime.local_time.year - 1900) %100)/10)<<4) + ((systime.local_time.year - 1900)%10);
	pd4990a.weekday = systime.local_time.weekday;

	neogeo_rng = 0x2345;	/* seed for the protection RNG in KOF99 onwards */

	/* it boots up with the BIOS vectors selected - see fatfury3 which has a corrupt vector in the game cart */
	neogeo_select_bios_vectors(0,0,0); // data doesn't matter

	/* not ideal having these here, but they need to be checked every reset at least */
	/* the rom banking is tied directly to the dipswitch?, or is there a bank write somewhere? */
	if (!strcmp(Machine->gamedrv->name,"svcpcb"))
	{
		int harddip3 = readinputportbytag("HARDDIP")&1;
		memcpy(memory_region( REGION_USER1 ),memory_region( REGION_USER1 )+0x20000+harddip3*0x20000, 0x20000);
	}
	/* a jumper pad on th PCB acts as a ROM overlay and is used to select the game language as opposed to the BIOS */
	if (!strcmp(Machine->gamedrv->name,"kog"))
	{
		int jumper = readinputportbytag("JUMPER");
		memory_region(REGION_CPU1)[0x1FFFFC/2] = jumper;
	}

	neogeo_start_timers();
}


/* This function is only called once per game. */
DRIVER_INIT( neogeo )
{
	extern struct YM2610interface neogeo_ym2610_interface;
	UINT16 *mem16 = (UINT16 *)memory_region(REGION_CPU1);
	int tileno,numtiles;

	numtiles = memory_region_length(REGION_GFX3)/128;
	for (tileno = 0;tileno < numtiles;tileno++)
	{
		unsigned char swap[128];
		UINT8 *gfxdata;
		int x,y;
		unsigned int pen;

		gfxdata = &memory_region(REGION_GFX3)[128 * tileno];

		memcpy(swap,gfxdata,128);

		for (y = 0;y < 16;y++)
		{
			UINT32 dw;

			dw = 0;
			for (x = 0;x < 8;x++)
			{
				pen  = ((swap[64 + 4*y + 3] >> x) & 1) << 3;
				pen |= ((swap[64 + 4*y + 1] >> x) & 1) << 2;
				pen |= ((swap[64 + 4*y + 2] >> x) & 1) << 1;
				pen |=	(swap[64 + 4*y	  ] >> x) & 1;
				dw |= pen << 4*x;
			}
			*(gfxdata++) = dw>>0;
			*(gfxdata++) = dw>>8;
			*(gfxdata++) = dw>>16;
			*(gfxdata++) = dw>>24;

			dw = 0;
			for (x = 0;x < 8;x++)
			{
				pen  = ((swap[4*y + 3] >> x) & 1) << 3;
				pen |= ((swap[4*y + 1] >> x) & 1) << 2;
				pen |= ((swap[4*y + 2] >> x) & 1) << 1;
				pen |=	(swap[4*y	 ] >> x) & 1;
				dw |= pen << 4*x;
			}
			*(gfxdata++) = dw>>0;
			*(gfxdata++) = dw>>8;
			*(gfxdata++) = dw>>16;
			*(gfxdata++) = dw>>24;
		}
	}

	if (memory_region(REGION_SOUND2))
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND2);
		neogeo_ym2610_interface.pcmromb = REGION_SOUND2;
	}
	else
	{
		logerror("using memory region %d for Delta T samples\n",REGION_SOUND1);
		neogeo_ym2610_interface.pcmromb = REGION_SOUND1;
	}

	/* Allocate ram banks */
	neogeo_ram16 = auto_malloc (0x10000);
	memory_set_bankptr(1, neogeo_ram16);

	/* Set the biosbank */
	memory_set_bankptr(3, memory_region(REGION_USER1));

	/* Set the 2nd ROM bank */
	if (memory_region_length(REGION_CPU1) > 0x100000)
		neogeo_set_cpu1_second_bank(0x100000);
	else
		neogeo_set_cpu1_second_bank(0x000000);

	/* Set the sound CPU ROM banks */
	neogeo_init_cpu2_setbank();

	/* Allocate and point to the memcard - bank 5 */
	neogeo_memcard = auto_malloc(0x800);
	memset(neogeo_memcard, 0, 0x800);

	mem16 = (UINT16 *)memory_region(REGION_USER1);


	/* irritating maze uses a trackball */
	if (!strcmp(Machine->gamedrv->name,"irrmaze"))
		neogeo_has_trackball = 1;
	else
		neogeo_has_trackball = 0;


	{ /* info from elsemi, this is how nebula works, is there a better way in mame? */
		UINT8* gamerom = memory_region(REGION_CPU1);
		neogeo_game_vectors = auto_malloc (0x80);
		memcpy( neogeo_game_vectors, gamerom, 0x80 );
	}
}

MACHINE_START( neogeo )
{
	neogeo_create_timers();

	/* register state save */
	neogeo_register_main_savestate();
	neogeo_register_sub_savestate();

	return 0;
}


/******************************************************************************/

WRITE16_HANDLER (neogeo_select_bios_vectors)
{
	UINT8* gamerom = memory_region(REGION_CPU1);
	UINT8* biosrom = memory_region(REGION_USER1);

	memcpy( gamerom, biosrom, 0x80 );
}

WRITE16_HANDLER (neogeo_select_game_vectors)
{
	UINT8* gamerom = memory_region(REGION_CPU1);
	memcpy( gamerom, neogeo_game_vectors, 0x80 );
}

/******************************************************************************/



NVRAM_HANDLER( neogeo )
{
	if (read_or_write)
	{
		/* Save the SRAM settings */
		mame_fwrite(file,neogeo_sram16,0x2000);
	}
	else
	{
		/* Load the SRAM settings for this game */
		if (file)
			mame_fread(file,neogeo_sram16,0x2000);
		else
			memset(neogeo_sram16,0,0x10000);
	}
}



/*
    INFORMATION:

    Memory card is a 2kb battery backed RAM.
    It is accessed thru 0x800000-0x800FFF.
    Even bytes are always 0xFF
    Odd bytes are memcard data (0x800 bytes)

    Status byte at 0x380000: (BITS ARE ACTIVE *LOW*)

    0 PAD1 START
    1 PAD1 SELECT
    2 PAD2 START
    3 PAD2 SELECT
    4 --\  MEMORY CARD
    5 --/  INSERTED
    6 MEMORY CARD WRITE PROTECTION
    7 UNUSED (?)
*/




/********************* MEMCARD ROUTINES **********************/
READ16_HANDLER( neogeo_memcard16_r )
{
	if (memcard_present() != -1)
		return neogeo_memcard[offset] | 0xff00;
	else
		return ~0;
}

WRITE16_HANDLER( neogeo_memcard16_w )
{
	if (ACCESSING_LSB)
	{
		if (memcard_present() != -1)
			neogeo_memcard[offset] = data & 0xff;
	}
}

MEMCARD_HANDLER( neogeo )
{
	char buf[0x800];

	switch (action)
	{
		case MEMCARD_CREATE:
			memset(buf, 0, sizeof(buf));
			mame_fwrite(file, neogeo_memcard, 0x800);
			break;

		case MEMCARD_INSERT:
			mame_fread(file, neogeo_memcard, 0x800);
			break;

		case MEMCARD_EJECT:
			mame_fwrite(file, neogeo_memcard, 0x800);
			break;
	}
}

/******************************************************************************/

static void neogeo_register_sub_savestate(void)
{
	UINT8* gamevector = memory_region(REGION_CPU1);

	state_save_register_global(neogeo_sram_locked);
	state_save_register_global_pointer(neogeo_ram16, 0x10000/2);
	state_save_register_global_pointer(neogeo_memcard, 0x800);
	state_save_register_global_pointer(gamevector, 0x80);
	state_save_register_global(neogeo_prot_data);
}
