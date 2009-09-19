/***************************************************************************

  snescart.c

  Machine file to handle cart loading in the Nintendo Super NES emulation.

***************************************************************************/

#include "driver.h"
#include "includes/snes.h"
#include "devices/cartslot.h"
#include "snescart.h"


/****** Strings for Cart Header Logging ******/

#define UNK "Unknown"

/* Some known type of cart */
static const char *const types[] =
{
	"ROM",
	"ROM, DSP-1",
	"ROM, DSP-2",
	"ROM, DSP-3",
	"ROM, DSP-4",
	"ROM, Super FX / FX2",
	"ROM, SA-1",
	"ROM, S-DD1",
	"ROM, OBC-1",
	"ROM, S-RTC",
	"ROM, Z80GB (Super Game Boy)",
	"ROM, C4",
	"ROM, Seta ST-010",
	"ROM, Seta ST-011",
	"ROM, Seta ST-018",
	"ROM, SPC7110",
	"ROM, SPC7110, RTC",
	UNK,							// to add: Satellaview BS-X detection
};

/* Some known countries */
static const char *const countries[] =
{
/* 0*/	"Japan (NTSC)", "USA & Canada (NTSC)", "Europe, Oceania & Asia (PAL)", "Sweden (PAL)",
/* 4*/	"Finland (PAL)", "Denmark (PAL)", "France (PAL)", "Holland (PAL)",
/* 8*/	"Spain (PAL)", "Germany, Austria & Switzerland (PAL)", "Italy (PAL)", "Hong Kong & China (PAL)",
/* c*/	"Indonesia (PAL)", "South Korea (NTSC)", UNK, UNK,
};

/* Some known companies (integrations to the list from Snes9x) */
static const char *const companies[] =
{
/* 0*/	"Invalid", "Nintendo", "Ajinomoto", "Imagineer-Zoom", "Chris Gray Enterprises Inc.", "Zamuse", "Falcom", UNK,
/* 8*/	"Capcom", "HOT-B", "Jaleco", "Coconuts", "Rage Software", "Micronet", "Technos", "Mebio Software",
/*10*/	"SHOUEi System", "Starfish", "Gremlin Graphics", "Electronic Arts", "NCS / Masaya", "COBRA Team", "Human/Field", "KOEI",
/*18*/	"Hudson Soft", "Game Village", "Yanoman", UNK, "Tecmo", UNK, "Open System", "Virgin Games",
/*20*/	"KSS", "Sunsoft", "POW", "Micro World", UNK, UNK, "Enix", "Loriciel/Electro Brain",
/*28*/	"Kemco", "Seta Co.,Ltd.", "Culture Brain", "Irem Japan", "Pal Soft", "Visit Co.,Ltd.", "INTEC Inc.", "System Sacom Corp.",
/*30*/	"Viacom New Media", "Carrozzeria", "Dynamic", "Nintendo", "Magifact", "Hect", UNK, UNK,
/*38*/	"Capcom Europe", "Accolade Europe", UNK, "Arcade Zone", "Empire Software", "Loriciel", "Gremlin Graphics", UNK,
/*40*/	"Seika Corp.", "UBI Soft", UNK, UNK, "LifeFitness Exertainment", UNK, "System 3", "Spectrum Holobyte",
/*48*/	UNK, "Irem", UNK, "Raya Systems/Sculptured Software", "Renovation Products", "Malibu Games/Black Pearl", UNK, "U.S. Gold",
/*50*/	"Absolute Entertainment", "Acclaim", "Activision", "American Sammy", "GameTek", "Hi Tech Expressions", "LJN Toys", UNK,
/*58*/	UNK, UNK, "Mindscape", "Romstar, Inc.", UNK, "Tradewest", UNK, "American Softworks Corp.",
/*60*/	"Titus", "Virgin Interactive Entertainment", "Maxis", "Origin/FCI/Pony Canyon", UNK, UNK, UNK, "Ocean",
/*68*/	UNK, "Electronic Arts", UNK, "Laser Beam", UNK, UNK, "Elite", "Electro Brain",
/*70*/	"Infogrames", "Interplay", "LucasArts", "Parker Brothers", "Konami", "STORM", UNK, UNK,
/*78*/	"THQ Software", "Accolade Inc.", "Triffix Entertainment", UNK, "Microprose", UNK, UNK, "Kemco",
/*80*/	"Misawa", "Teichio", "Namco Ltd.", "Lozc", "Koei", UNK, "Tokuma Shoten Intermedia", "Tsukuda Original",
/*88*/	"DATAM-Polystar", UNK, UNK, "Bullet-Proof Software", "Vic Tokai", UNK, "Character Soft", "I\'\'Max",
/*90*/	"Takara", "CHUN Soft", "Video System Co., Ltd.", "BEC", UNK, "Varie", "Yonezawa / S'Pal Corp.", "Kaneco",
/*98*/	UNK, "Pack in Video", "Nichibutsu", "TECMO", "Imagineer Co.", UNK, UNK, UNK,
/*a0*/	"Telenet", "Hori", UNK, UNK, "Konami", "K.Amusement Leasing Co.", UNK, "Takara",
/*a8*/	UNK, "Technos Jap.", "JVC", UNK, "Toei Animation", "Toho", UNK, "Namco Ltd.",
/*b0*/	"Media Rings Corp.", "ASCII Co. Activison", "Bandai", UNK, "Enix America", UNK, "Halken", UNK,
/*b8*/	UNK, UNK, "Culture Brain", "Sunsoft", "Toshiba EMI", "Sony Imagesoft", UNK, "Sammy",
/*c0*/	"Taito", UNK, "Kemco", "Square", "Tokuma Soft", "Data East", "Tonkin House", UNK,
/*c8*/	"KOEI", UNK, "Konami USA", "NTVIC", UNK, "Meldac", "Pony Canyon", "Sotsu Agency/Sunrise",
/*d0*/	"Disco/Taito", "Sofel", "Quest Corp.", "Sigma", "Ask Kodansha Co., Ltd.", UNK, "Naxat", UNK,
/*d8*/	"Capcom Co., Ltd.", "Banpresto", "Tomy", "Acclaim", UNK, "NCS", "Human Entertainment", "Altron",
/*e0*/	"Jaleco", UNK, "Yutaka", UNK, "T&ESoft", "EPOCH Co.,Ltd.", UNK, "Athena",
/*e8*/	"Asmik", "Natsume", "King Records", "Atlus", "Sony Music Entertainment", UNK, "IGS", UNK,
/*f0*/	UNK, "Motown Software", "Left Field Entertainment", "Beam Software", "Tec Magik", UNK, UNK, UNK,
/*f8*/	UNK, "Cybersoft", UNK, "Psygnosis", UNK, UNK, "Davidson", UNK,
};


/* We use this to convert the company_id in the header to int value to be passed in companies[] */
static int char_to_int_conv( char id )
{
	int value;

	if (id == '1') value = 0x01;
	else if (id == '2') value = 0x02;
	else if (id == '3') value = 0x03;
	else if (id == '4') value = 0x04;
	else if (id == '5') value = 0x05;
	else if (id == '6') value = 0x06;
	else if (id == '7') value = 0x07;
	else if (id == '8') value = 0x08;
	else if (id == '9') value = 0x09;
	else if (id == 'A') value = 0x0a;
	else if (id == 'B') value = 0x0b;
	else if (id == 'C') value = 0x0c;
	else if (id == 'D') value = 0x0d;
	else if (id == 'E') value = 0x0e;
	else if (id == 'F') value = 0x0f;
	else value = 0x00;

	return value;
}


/***************************************************************************

  SRAM handling

***************************************************************************/

/* Loads the battery backed RAM into the appropriate memory area */
static void snes_load_sram(running_machine *machine)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = alloc_array_or_die(UINT8, snes_cart.sram_max);
	ptr = battery_ram;
	image_battery_load(devtag_get_device(machine, "cart"), battery_ram, snes_cart.sram_max);

	if (snes_cart.mode == SNES_MODE_20)
	{
		/* There could be some larger image needing banks 0x70 to 0x7f at address 0x8000 for ROM
		 * mirroring. These should be treated separately or data would be overwritten by SRAM */
		for (ii = 0; ii < 16; ii++)
		{
			/* loading */
			memmove(&snes_ram[0x700000 + (ii * 0x010000)], ptr, 0xffff);
			/* mirroring */
			memcpy(&snes_ram[0xf00000 + (ii * 0x010000)], &snes_ram[0x700000 + (ii * 0x010000)], 0xffff);
			ptr += 0xffff;
		}
	}
	else if (snes_cart.mode == SNES_MODE_21)
	{
		for (ii = 0; ii < 16; ii++)
		{
			/* loading */
			memmove(&snes_ram[0x306000 + (ii * 0x010000)], ptr, 0x1fff);
			/* mirroring */
			memcpy(&snes_ram[0xb06000 + (ii * 0x010000)], &snes_ram[0x306000 + (ii * 0x010000)], 0x1fff);
			ptr += 0x1fff;
		}
	}
	else if (snes_cart.mode == SNES_MODE_25)
	{
		for (ii = 0; ii < 16; ii++)
		{
			memmove(&snes_ram[0xb06000 + (ii * 0x010000)], ptr, 0x1fff);
			ptr += 0x1fff;
		}
	}

	free(battery_ram);
}

/* Saves the battery backed RAM from the appropriate memory area */
static void snes_save_sram(running_machine *machine)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = alloc_array_or_die(UINT8, snes_cart.sram_max);
	ptr = battery_ram;

	if (snes_cart.mode == SNES_MODE_20)
	{
		for (ii = 0; ii < 16; ii++)
		{
			memmove(ptr, &snes_ram[0x700000 + (ii * 0x010000)], 0x10000);
			ptr += 0x10000;
		}
	}
	else if (snes_cart.mode == SNES_MODE_21)
	{
		for (ii = 0; ii < 16; ii++)
		{
			memmove(ptr, &snes_ram[0x306000 + (ii * 0x010000)], 0x2000);
			ptr += 0x2000;
		}
	}
	else if (snes_cart.mode == SNES_MODE_25)
	{
		for (ii = 0; ii < 16; ii++)
		{
			memmove(ptr, &snes_ram[0xb06000 + (ii * 0x010000)], 0x2000);
			ptr += 0x2000;
		}
	}

	image_battery_save(devtag_get_device(machine, "cart"), battery_ram, snes_cart.sram_max);

	free(battery_ram);
}

static void snes_machine_stop(running_machine *machine)
{
	/* Save SRAM */
	if( snes_cart.sram > 0 )
		snes_save_sram(machine);
}

MACHINE_START( snes_mess )
{
	add_exit_callback(machine, snes_machine_stop);
	MACHINE_START_CALL(snes);
}


/***************************************************************************

  Cart handling

***************************************************************************/

/* This function assign a 'score' to data immediately after 'offset' to measure how valid they are
as information block (to decide if the image is HiRom, LoRom, ExLoRom or ExHiRom) */
/* Code from bsnes, courtesy of byuu - http://byuu.cinnamonpirate.com/ */
static int snes_validate_infoblock( UINT8 *infoblock, UINT32 offset )
{
	int score = 0;
	UINT16 reset_vector = infoblock[offset + 0x3c] | (infoblock[offset + 0x3d] << 8);
	UINT16 checksum     = infoblock[offset + 0x1e] | (infoblock[offset + 0x1f] << 8);
	UINT16 ichecksum    = infoblock[offset + 0x1c] | (infoblock[offset + 0x1d] << 8);
	UINT8 reset_opcode  = infoblock[(offset & ~0x7fff) | (reset_vector & 0x7fff)];	//first opcode executed upon reset
	UINT8 mapper        = infoblock[offset + 0x15] & ~0x10;							//mask off irrelevent FastROM-capable bit

	/* $00:[000-7fff] contains uninitialized RAM and MMIO.
	reset vector must point to ROM at $00:[8000-ffff] to be considered valid. */
	if (reset_vector < 0x8000)
		return 0;

	/* some images duplicate the header in multiple locations, and others have completely
	invalid header information that cannot be relied upon. The code below will analyze
	the first opcode executed at the specified reset vector to determine the probability
	that this is the correct header. Score is assigned accordingly. */

	/* most likely opcodes */
	if (reset_opcode == 0x78		//sei
		|| reset_opcode == 0x18		//clc (clc; xce)
		|| reset_opcode == 0x38		//sec (sec; xce)
		|| reset_opcode == 0x9c		//stz $nnnn (stz $4200)
		|| reset_opcode == 0x4c		//jmp $nnnn
		|| reset_opcode == 0x5c		//jml $nnnnnn
	)
		score += 8;

	/* plausible opcodes */
	if (reset_opcode == 0xc2		//rep #$nn
		|| reset_opcode == 0xe2		//sep #$nn
		|| reset_opcode == 0xad		//lda $nnnn
		|| reset_opcode == 0xae		//ldx $nnnn
		|| reset_opcode == 0xac		//ldy $nnnn
		|| reset_opcode == 0xaf		//lda $nnnnnn
		|| reset_opcode == 0xa9		//lda #$nn
		|| reset_opcode == 0xa2		//ldx #$nn
		|| reset_opcode == 0xa0		//ldy #$nn
		|| reset_opcode == 0x20		//jsr $nnnn
		|| reset_opcode == 0x22		//jsl $nnnnnn
	)
		score += 4;

	/* implausible opcodes */
	if (reset_opcode == 0x40		//rti
		|| reset_opcode == 0x60		//rts
		|| reset_opcode == 0x6b		//rtl
		|| reset_opcode == 0xcd		//cmp $nnnn
		|| reset_opcode == 0xec		//cpx $nnnn
		|| reset_opcode == 0xcc		//cpy $nnnn
	)
		score -= 4;

	/* least likely opcodes */
	if (reset_opcode == 0x00		//brk #$nn
		|| reset_opcode == 0x02		//cop #$nn
		|| reset_opcode == 0xdb		//stp
		|| reset_opcode == 0x42		//wdm
		|| reset_opcode == 0xff		//sbc $nnnnnn,x
	)
		score -= 8;

	/* Sometimes, both the header and reset vector's first opcode will match ...
	fallback and rely on info validity in these cases to determine more likely header. */

	/* a valid checksum is the biggest indicator of a valid header. */
	if ((checksum + ichecksum) == 0xffff && (checksum != 0) && (ichecksum != 0))
		score += 4;

	/* then there are the expected mapper values */
	if (offset == 0x007fc0 && mapper == 0x20)	// 0x20 is usually LoROM
		score += 2;

	if (offset == 0x00ffc0 && mapper == 0x21)	// 0x21 is usually HiROM
		score += 2;

	if (offset == 0x007fc0 && mapper == 0x22)	// 0x22 is usually ExLoROM
		score += 2;

	if (offset == 0x40ffc0 && mapper == 0x25)	// 0x25 is usually ExHiROM
		score += 2;

	/* finally, there are valid values in the Company, Region etc. fields */
	if (infoblock[offset + 0x1a] == 0x33)			// Company field: 0x33 indicates extended header
		score += 2;

	if (infoblock[offset + 0x16] < 0x08)			// ROM Type field
		score++;

	if (infoblock[offset + 0x17] < 0x10)			// ROM Size field
		score++;

	if (infoblock[offset + 0x18] < 0x08)			// SRAM Size field
		score++;

	if (infoblock[offset + 0x19] < 14)				// Region field
		score++;

	/* do we still have a positive score? */
	if (score < 0)
		score = 0;

	return score;
}


static DEVICE_IMAGE_LOAD( snes_cart )
{
	int i, supported_type = 1;
	running_machine *machine = image->machine;
	const address_space *space = cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM );
	int total_blocks, read_blocks;
	UINT32 offset;
	UINT8 header[512];
	UINT8 *temp_buffer = auto_alloc_array(machine, UINT8, 0x410000);
	UINT8 valid_mode20, valid_mode21, valid_mode25;
	unsigned char *ROM;
	int length;

	memory_region_alloc(machine, "maincpu", 0x1000000, 0);

	snes_ram = memory_region(machine, "maincpu");
	memset( snes_ram, 0, 0x1000000 );

	/* Check for a header (512 bytes) */
	offset = 512;
	image_fread(image, header, 512);
	if ((header[8] == 0xaa) && (header[9] == 0xbb) && (header[10] == 0x04))
	{
		/* Found an SWC identifier */
		logerror("Found header(SWC) - Skipped\n");
	}
	else if ((header[0] | (header[1] << 8)) == (((image_length(image) - 512) / 1024) / 8))
	{
		/* Some headers have the rom size at the start, if this matches with the
         * actual rom size, we probably have a header */
		logerror("Found header(size) - Skipped\n");
	}
	else if ((image_length(image) % 0x8000) == 512)
	{
		/* As a last check we'll see if there's exactly 512 bytes extra to this
         * image. */
		logerror("Found header(extra) - Skipped\n");
	}
	else
	{
		/* No header found so go back to the start of the file */
		logerror("No header found.\n");
		offset = 0;
		image_fseek(image, offset, SEEK_SET);
	}

	/* We need to take a sample to test what mode we need to be in (the
	sample has to be quite large to cope with large carts in ExHiRom) */
	image_fread(image, temp_buffer, 0x40ffff);
	image_fseek(image, offset, SEEK_SET);	/* Rewind */

	/* Now to determine if this is a lo-ROM, a hi-ROM or an extended lo/hi-ROM */
	valid_mode20 = snes_validate_infoblock(temp_buffer, 0x007fc0);
	valid_mode21 = snes_validate_infoblock(temp_buffer, 0x00ffc0);
	valid_mode25 = snes_validate_infoblock(temp_buffer, 0x40ffc0);

	/* Images larger than 32mbits are likely ExHiRom */
	if (valid_mode25)
		valid_mode25 += 4;

	if ((valid_mode20 >= valid_mode21) && (valid_mode20 >= valid_mode25))
	{
		if ((temp_buffer[0x007fd5] == 0x32) || ((image_length(image) - offset) > 0x401000))
			snes_cart.mode = SNES_MODE_22;	// ExLoRom
		else
			snes_cart.mode = SNES_MODE_20;	// LoRom

		/* a few games require 512k, however we store twice as much to be sure to cover the various mirrors */
		snes_cart.sram_max = 0x100000;
	}
	else if( valid_mode21 >= valid_mode25 )
	{
		snes_cart.mode = SNES_MODE_21;	// HiRom
		snes_cart.sram_max = 0x20000;
	}
	else
	{
		snes_cart.mode = SNES_MODE_25;	// ExHiRom
		snes_cart.sram_max = 0x20000;
	}

	/* Loading data */
	ROM = memory_region(image->machine, "cart");
	length = image_fread(image, ROM, image_length(image) - offset);

	/* FIXME: Insert crc check here */

	/* How many blocks of data are available to be loaded? */
	total_blocks = ((image_length(image) - offset) / (snes_cart.mode & 0x05 ? 0x8000 : 0x10000));
	read_blocks = 0;

	/* Loading all the data blocks from cart, we only partially cover banks 0x00 to 0x7f. Therefore, we
	 * have to mirror the blocks until we reach the end. E.g. for a 11Mbits image (44 blocks), we proceed
	 * as follows:
	 * 11 Mbits = 8 Mbits (blocks 1->32) + 2 Mbits (blocks 33->40) + 1 Mbit (blocks 41->44).
	 * Hence, we fill memory up to 16 Mbits (banks 0x00 to 0x3f) mirroring as follows
	 * 8 Mbits (blocks 1->32) + 2 Mbits (blocks 33->40) + 1 Mbit (blocks 41->44) + 1 Mbit (blocks 41->44)
	 *   + 2 Mbits (blocks 33->40) + 1 Mbit (blocks 41->44) + 1 Mbit (blocks 41->44)
	 * and we repeat the same blocks in the second half of the banks (banks 0x40 to 0x7f).
	 * This is likely what happens in the real SNES as well, because the unit cannot be aware of the exact
	 * size of data in the cart (procedure confirmed by byuu)
	 */
	switch (snes_cart.mode)
	{
		case SNES_MODE_21:
		/* HiROM carts load data in banks 0xc0 to 0xff. Each bank is fully mirrored in banks 0x40 to 0x7f
		 * (actually up to 0x7d, because 0x7e and 0x7f are overwritten by WRAM). The top half (address
		 * range 0x8000 - 0xffff) of each bank is also mirrored in banks 0x00 to 0x3f and 0x80 to 0xbf.
		 */
			while (read_blocks < 64 && read_blocks < total_blocks)
			{
				/* Loading data */
				memcpy(&snes_ram[0xc00000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x10000], 0x10000);
				/* Mirroring */
				memcpy(&snes_ram[0x008000 + read_blocks * 0x10000], &snes_ram[0xc08000 + read_blocks * 0x10000], 0x8000);
				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &snes_ram[0xc00000 + read_blocks * 0x10000], 0x10000);
				memcpy(&snes_ram[0x808000 + read_blocks * 0x10000], &snes_ram[0xc08000 + read_blocks * 0x10000], 0x8000);
				read_blocks++;
			}
			/* Filling banks up to 0xff and their mirrors */
			while (read_blocks % 64)
			{
				int j = 0, repeat_blocks;
				while ((read_blocks % (64 >> j)) && j < 6)
					j++;
				repeat_blocks = read_blocks % (64 >> (j - 1));

				memcpy(&snes_ram[0xc00000 + read_blocks * 0x10000], &snes_ram[0xc00000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[read_blocks * 0x10000], &snes_ram[(read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &snes_ram[0x400000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[0x800000 + read_blocks * 0x10000], &snes_ram[0x800000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				read_blocks += repeat_blocks;
			}
			break;

		case SNES_MODE_25:
		/* Extendend HiROM carts start to load data in banks 0xc0 to 0xff. However, they exceed the
		 * available space in these banks, and continue loading at banks 0x40 to 0x7f. The top half
		 * (address range 0x8000 - 0xffff) of each bank is also mirrored either to banks 0x00 to 0x3f
		 * (for data in banks 0x40 to 0x7f) or to banks 0x80 to 0xbf (for data in banks 0xc0 to 0xff).
		 * Notice that banks 0x7e and 0x7f are overwritten by WRAM, but they could contain data at
		 * address > 0x8000 because the mirrors at 0x3e and 0x3f are not overwritten.
		 */
			/* Reading the first 64 blocks */
			while (read_blocks < 64 && read_blocks < total_blocks)
			{
				/* Loading data */
				memcpy(&snes_ram[0xc00000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x10000], 0x10000);
				/* Mirroring */
				memcpy( &snes_ram[0x808000 + read_blocks * 0x10000], &snes_ram[0xc08000 + read_blocks * 0x10000], 0x8000);
				read_blocks++;
			}
			/* ExHiROMs are supposed to be larger than 32Mbits! */
			if (read_blocks < 64)
			{
				logerror("This image has been identified as ExHiROM, but it's too small!\n");
				logerror("Please verify if it's corrupted. If it's not please report this as a bug.\n");
				return INIT_FAIL;
			}
			/* Scaling down the counter */
			read_blocks -= 64;
			/* Reading the next blocks */
			while (read_blocks < 64  && read_blocks < (total_blocks - 64))
			{
				/* Loading data */
				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &ROM[0x400000 + read_blocks * 0x10000], 0x10000);
				/* Mirroring */
				memcpy(&snes_ram[0x8000 + read_blocks * 0x10000], &snes_ram[0x408000 + read_blocks * 0x10000], 0x8000);
				read_blocks++;
			}
			/* Filling banks up to 0x7f and their mirrors */
			while (read_blocks % 64)
			{
				int j = 0, repeat_blocks;
				while ((read_blocks % (64 >> j)) && j < 6)
					j++;
				repeat_blocks = read_blocks % (64 >> (j - 1));

				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &snes_ram[0x400000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[read_blocks * 0x10000], &snes_ram[(read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				read_blocks += repeat_blocks;
			}
			break;

		case SNES_MODE_22:
		/* "Extendend LoROM" carts have their data loaded in banks 0x00 to 0x3f at address 0x8000.
		 * These are then mirrored in banks 0x40 to 0x7f, both at address 0x0000 and at address
		 * 0x8000, in banks 0x80 to 0xbf at address 0x8000 and in banks 0xc0 to 0xff, both at address
		 * 0x0000 and at address 0x8000. Notice that SDD-1 games (Star Ocean and Street Fighter Zero 2)
		 * use SNES_MODE_22
		 */
			while (read_blocks < 64 && read_blocks < total_blocks)
			{
				/* Loading and mirroring data */
				memcpy(&snes_ram[0x008000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x8000], 0x8000);
				memcpy(&snes_ram[0x808000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x8000], 0x8000);
				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x10000], 0x10000);
				memcpy(&snes_ram[0xc00000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x10000], 0x10000);
				read_blocks++;
			}
			/* Filling banks up to 0x3f and their mirrors */
			while (read_blocks % 64)
			{
				int j = 0, repeat_blocks;
				while ((read_blocks % (64 >> j)) && j < 6)
					j++;
				repeat_blocks = read_blocks % (64 >> (j - 1));

				memcpy(&snes_ram[read_blocks * 0x10000], &snes_ram[(read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[0x800000 + read_blocks * 0x10000], &snes_ram[0x800000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[0x400000 + read_blocks * 0x10000], &snes_ram[0x400000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				memcpy(&snes_ram[0xc00000 + read_blocks * 0x10000], &snes_ram[0xc00000 + (read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
				read_blocks += repeat_blocks;
			}
			break;

		default:
		case SNES_MODE_20:
		/* LoROM carts load data in banks 0x00 to 0x7f at address 0x8000 (actually up to 0x7d, because 0x7e and
		 * 0x7f are overwritten by WRAM). Each block is also mirrored in banks 0x80 to 0xff (up to 0xff for real)
		 */
			/* SuperFX games needs this different loading routine */
			if(((temp_buffer[0x007fd6] >= 0x13 && temp_buffer[0x007fd6] <= 0x15) || temp_buffer[0x007fd6] == 0x1a) && (temp_buffer[0x007fd5] == 0x20))
			{
				while (read_blocks < 64 && read_blocks < total_blocks)
				{
					/* Loading data primary: banks 0-3f from 8000-ffff*/
					memcpy(&snes_ram[0x008000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x8000], 0x8000);
					/* Mirroring at banks 80-bf from 8000-ffff */
					memcpy(&snes_ram[0x808000 + read_blocks * 0x10000], &snes_ram[0x8000 + (read_blocks * 0x10000)], 0x8000);
					read_blocks++;
				}
				/* Loading data secondary, filling full 64k banks from 0x400000 to 600000 */
				memcpy(&snes_ram[0x400000], &ROM[0x000000], total_blocks * 0x8000);
				/* mirror at c00000-e00000 */
				memcpy(&snes_ram[0xc00000], &snes_ram[0x400000], total_blocks * 0x8000);
			}
			else
			{
				while (read_blocks < 128 && read_blocks < total_blocks)
				{
					/* Loading data */
					memcpy(&snes_ram[0x008000 + read_blocks * 0x10000], &ROM[0x000000 + read_blocks * 0x8000], 0x8000);
					/* Mirroring */
					memcpy(&snes_ram[0x808000 + read_blocks * 0x10000], &snes_ram[0x8000 + (read_blocks * 0x10000)], 0x8000);
					read_blocks++;
				}
				/* Filling banks up to 0x7f and their mirrors */
				while (read_blocks % 128)
				{
					int j = 0, repeat_blocks;
					while ((read_blocks % (128 >> j)) && j < 7)
						j++;
					repeat_blocks = read_blocks % (128 >> (j - 1));

					memcpy( &snes_ram[read_blocks * 0x10000], &snes_ram[(read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
					memcpy( &snes_ram[0x800000 + read_blocks * 0x10000], &snes_ram[(read_blocks - repeat_blocks) * 0x10000], repeat_blocks * 0x10000);
					read_blocks += repeat_blocks;
				}
			}
			break;
	}

	/* Find the amount of sram */
	snes_cart.sram = snes_r_bank1(space, 0x00ffd8);
	if( snes_cart.sram > 0 )
	{
		snes_cart.sram = ((1 << (snes_cart.sram + 3)) / 8);
		if( snes_cart.sram > snes_cart.sram_max )
			snes_cart.sram = snes_cart.sram_max;
	}

	/* Find the type of cart and detect special chips */
	/* Info mostly taken from http://snesemu.black-ship.net/misc/-from%20nsrt.edgeemu.com-chipinfo.htm */
	switch (snes_r_bank1(space, 0x00ffd6))
	{
			case 0x00:
			case 0x01:
			case 0x02:
				snes_has_addon_chip = HAS_NONE;
				break;

			case 0x03:
				if (snes_r_bank1(space, 0x00ffd5) == 0x30)
					snes_has_addon_chip = HAS_DSP4;
				else
					snes_has_addon_chip = HAS_DSP1;
				break;

			case 0x04:
				snes_has_addon_chip = HAS_DSP1;
				break;

			case 0x05:
				if (snes_r_bank1(space, 0x00ffd5) == 0x20)
					snes_has_addon_chip = HAS_DSP2;
				/* DSP-3 is hard to detect. We exploit the fact that the only game has been manufactured by Bandai */
				else if ((snes_r_bank1(space, 0x00ffd5) == 0x30) && (snes_r_bank1(space, 0x00ffda) == 0xb2))
					snes_has_addon_chip = HAS_DSP3;
				else
					snes_has_addon_chip = HAS_DSP1;
				break;

			case 0x13:	// Mario Chip 1
			case 0x14:	// GSU-x
			case 0x15:	// GSU-x
			case 0x1a:	// GSU-1 (21 MHz at start)
				if (snes_r_bank1(space, 0x00ffd5) == 0x20)
					snes_has_addon_chip = HAS_SUPERFX;
				break;

			case 0x25:
				snes_has_addon_chip = HAS_OBC1;
				break;

			case 0x32:	// needed by a Sample game (according to ZSNES)
			case 0x34:
			case 0x35:
				if (snes_r_bank1(space, 0x00ffd5) == 0x23)
				{	
					snes_has_addon_chip = HAS_SA1;
					supported_type = 0;
				}
				break;

			case 0x43:
			case 0x45:
				if (snes_r_bank1(space, 0x00ffd5) == 0x32)
					snes_has_addon_chip = HAS_SDD1;
				break;

			case 0x55:
				if (snes_r_bank1(space, 0x00ffd5) == 0x35)
				{	
					snes_has_addon_chip = HAS_RTC;
					supported_type = 0;
				}
				break;

			case 0xe3:
				snes_has_addon_chip = HAS_Z80GB;
				supported_type = 0;
				break;

			case 0xf3:
				snes_has_addon_chip = HAS_CX4;
				break;

			case 0xf5:
				if (snes_r_bank1(space, 0x00ffd5) == 0x30)
				{
					snes_has_addon_chip = HAS_ST018;
					supported_type = 0;
				}
				else if (snes_r_bank1(space, 0x00ffd5) == 0x3a)
				{
					snes_has_addon_chip = HAS_SPC7110;
					supported_type = 0;
				}
				break;

			case 0xf6:
				/* These Seta ST-01X chips have both 0x30 at 0x00ffd5,
				they only differ for the 'size' at 0x00ffd7 */
				if (snes_r_bank1(space, 0x00ffd7) < 0x0a)
				{
					snes_has_addon_chip = HAS_ST011;
					supported_type = 0;
				}
				else
				{
					snes_has_addon_chip = HAS_ST010;
					supported_type = 0;
				}
				break;

			case 0xf9:
				if (snes_r_bank1(space, 0x00ffd5) == 0x3a)
				{
					snes_has_addon_chip = HAS_SPC7110_RTC;
					supported_type = 0;
				}
				break;

			default:
				snes_has_addon_chip = HAS_UNK;
				supported_type = 0;
				break;
	}

	/* Log snes_cart information */
	{
		char title[21], rom_id[4], company_id[2];
		int company, has_ram = 0, has_sram = 0;

		/* Company */
		for( i = 0; i < 2; i++ )
			company_id[i] = snes_r_bank1(space, 0x00ffb0 + i);
		company = (char_to_int_conv(company_id[0]) << 4) + char_to_int_conv(company_id[1]);
		if (company == 0)
			company = snes_r_bank1(space, 0x00ffda);

		/* ROM ID */
		for( i = 0; i < 4; i++ )
			rom_id[i] = snes_r_bank1(space, 0x00ffb2 + i);

		/* Title */
		for( i = 0; i < 21; i++ )
			title[i] = snes_r_bank1(space, 0x00ffc0 + i);

		/* RAM */
		if (((snes_r_bank1(space, 0x00ffd6) & 0xf) == 1) ||
				((snes_r_bank1(space, 0x00ffd6) & 0xf) == 2) ||
				((snes_r_bank1(space, 0x00ffd6) & 0xf) == 4) ||
				((snes_r_bank1(space, 0x00ffd6) & 0xf) == 5))
			has_ram = 1;

		/* SRAM */
		if (((snes_r_bank1(space, 0x00ffd6) & 0xf) == 2) ||
				((snes_r_bank1(space, 0x00ffd6) & 0xf) == 5) ||
				((snes_r_bank1(space, 0x00ffd6) & 0xf) == 6))
			has_sram = 1;

		logerror( "ROM DETAILS\n" );
		logerror( "===========\n\n" );
		logerror( "\tTotal blocks:  %d (%dmb)\n", total_blocks, total_blocks / (snes_cart.mode & 5 ? 32 : 16) );
		logerror( "\tROM bank size: %s (LoROM: %d , HiROM: %d, ExHiROM: %d)\n",
								(snes_cart.mode == SNES_MODE_20) ? "LoROM" : 
								(snes_cart.mode == SNES_MODE_21) ? "HiROM" :
								(snes_cart.mode == SNES_MODE_22) ? "ExLoROM" : "ExHiROM",
										valid_mode20, valid_mode21, valid_mode25);
		logerror( "\tCompany:       %s [%.2s]\n", companies[company], company_id );
		logerror( "\tROM ID:        %.4s\n\n", rom_id );

		logerror( "HEADER DETAILS\n" );
		logerror( "==============\n\n" );
		logerror( "\tName:          %.21s\n", title );
		logerror( "\tSpeed:         %s [%d]\n", ((snes_r_bank1(space, 0x00ffd5) & 0xf0)) ? "FastROM" : "SlowROM", (snes_r_bank1(space, 0x00ffd5) & 0xf0) >> 4 );
		logerror( "\tBank size:     %s [%d]\n", (snes_r_bank1(space, 0x00ffd5) & 0xf) ? "HiROM" : "LoROM", snes_r_bank1(space, 0x00ffd5) & 0xf );

		logerror( "\tType:          %s", types[snes_has_addon_chip]);
		if (has_ram)
			logerror( ", RAM");
		if (has_sram)
			logerror( ", SRAM");
		logerror( " [%d]\n", snes_r_bank1(space, 0x00ffd6) );

		logerror( "\tSize:          %d megabits [%d]\n", 1 << (snes_r_bank1(space, 0x00ffd7) - 7), snes_r_bank1(space, 0x00ffd7) );
		logerror( "\tSRAM:          %d kilobits [%d]\n", snes_cart.sram * 8, snes_ram[0xffd8] );
		logerror( "\tCountry:       %s [%d]\n", countries[snes_r_bank1(space, 0x00ffd9)], snes_r_bank1(space, 0x00ffd9) );
		logerror( "\tLicense:       %s [%X]\n", companies[snes_r_bank1(space, 0x00ffda)], snes_r_bank1(space, 0x00ffda) );
		logerror( "\tVersion:       1.%d\n", snes_r_bank1(space, 0x00ffdb) );
		logerror( "\tInv Checksum:  %X %X\n", snes_r_bank1(space, 0x00ffdd), snes_r_bank1(space, 0x00ffdc) );
		logerror( "\tChecksum:      %X %X\n", snes_r_bank1(space, 0x00ffdf), snes_r_bank1(space, 0x00ffde) );
		logerror( "\tNMI Address:   %2X%2Xh\n", snes_r_bank1(space, 0x00fffb), snes_r_bank1(space, 0x00fffa) );
		logerror( "\tStart Address: %2X%2Xh\n\n", snes_r_bank1(space, 0x00fffd), snes_r_bank1(space, 0x00fffc) );

		logerror( "\tMode: %d\n", snes_cart.mode);

		if (!supported_type)
			logerror("WARNING: This cart type \"%s\" is not supported yet!\n", types[snes_has_addon_chip]);
	}

	/* Load SRAM */
	snes_load_sram(space->machine);

	/* All done */
	return INIT_PASS;
}

MACHINE_DRIVER_START( snes_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("smc,sfc,fig,swc")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD(snes_cart)
MACHINE_DRIVER_END
