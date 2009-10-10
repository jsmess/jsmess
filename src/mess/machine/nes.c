#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"
#include "sound/nes_apu.h"
#include "devices/cartslot.h"
#include "image.h"
#include "hash.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* Set to dump info about the inputs to the errorlog */
#define LOG_JOY		0

/* Set to generate prg (16k) & chr (8k) chunk files when the cart is loaded */
#define SPLIT_PRG	0
#define SPLIT_CHR	0

#define BATTERY_SIZE 0x2000

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _nes_input nes_input;
struct _nes_input
{
	UINT32 shift;
	UINT32 i0, i1, i2;
};

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

unsigned char *nes_battery_ram;
static UINT8 battery_data[BATTERY_SIZE];

struct nes_struct nes;
struct fds_struct nes_fds;

static nes_input in_0;
static nes_input in_1;

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void init_nes_core(running_machine *machine);
static void nes_machine_stop(running_machine *machine);

/***************************************************************************
    FUNCTIONS
***************************************************************************/

static const device_config *cartslot_image( running_machine *machine )
{
	nes_state *state = machine->driver_data;
	return state->cart;
}

static void init_nes_core( running_machine *machine )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* We set these here in case they weren't set in the cart loader */
	nes.rom = memory_region(machine, "maincpu");
	nes.vrom = memory_region(machine, "gfx1");
	nes.vram = memory_region(machine, "gfx2");
	nes.ciram = memory_region(machine, "gfx3");
	nes.wram = memory_region(machine, "user1");

	/* Brutal hack put in as a consequence of the new memory system; we really
       need to fix the NES code */
	memory_install_read8_handler(space, 0x0000, 0x07ff, 0, 0x1800, SMH_BANK(10));
	memory_install_write8_handler(space, 0x0000, 0x07ff, 0, 0x1800, SMH_BANK(10));

	memory_install_readwrite8_handler(cpu_get_address_space(cputag_get_cpu(machine, "ppu"), ADDRESS_SPACE_PROGRAM), 0, 0x1fff, 0, 0, nes_chr_r, nes_chr_w);
	memory_install_readwrite8_handler(cpu_get_address_space(cputag_get_cpu(machine, "ppu"), ADDRESS_SPACE_PROGRAM), 0x2000, 0x3eff, 0, 0, nes_nt_r, nes_nt_w);

	memory_set_bankptr(machine, 10, nes.rom);

	nes_battery_ram = nes.wram;

	/* Set up the memory handlers for the mapper */
	switch (nes.mapper)
	{
		case 20:
			nes.slow_banking = 0;
			/* If we are loading a disk we have already filled nes_fds.data and we don't want to overwrite it,
            if we are loading a cart image identified as mapper 20 (probably wrong mapper...) we need to alloc
            memory for the bank 2 pointer */
			if (nes_fds.data == NULL)
				nes_fds.data = auto_alloc_array(machine, UINT8, 0x8000 );
			memory_install_read8_handler(space, 0x4030, 0x403f, 0, 0, fds_r);
			memory_install_read8_handler(space, 0x6000, 0xdfff, 0, 0, SMH_BANK(2));
			memory_install_read8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(1));

			memory_install_write8_handler(space, 0x4020, 0x402f, 0, 0, fds_w);
			memory_install_write8_handler(space, 0x6000, 0xdfff, 0, 0, SMH_BANK(2));

			memory_set_bankptr(machine, 1, &nes.rom[0xe000]);
			memory_set_bankptr(machine, 2, nes_fds.data );
			break;
		case 50:
			memory_install_write8_handler(space, 0x4020, 0x403f, 0, 0, mapper50_add_w);
			memory_install_write8_handler(space, 0x40a0, 0x40bf, 0, 0, mapper50_add_w);
		default:
			nes.slow_banking = 0;
			memory_install_read8_handler(space, 0x6000, 0x7fff, 0, 0, SMH_BANK(5));
			memory_install_read8_handler(space, 0x8000, 0x9fff, 0, 0, SMH_BANK(1));
			memory_install_read8_handler(space, 0xa000, 0xbfff, 0, 0, SMH_BANK(2));
			memory_install_read8_handler(space, 0xc000, 0xdfff, 0, 0, SMH_BANK(3));
			memory_install_read8_handler(space, 0xe000, 0xffff, 0, 0, SMH_BANK(4));
			memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + 0x6000);
			memory_set_bankptr(machine, 2, memory_region(machine, "maincpu") + 0x8000);
			memory_set_bankptr(machine, 3, memory_region(machine, "maincpu") + 0xa000);
			memory_set_bankptr(machine, 4, memory_region(machine, "maincpu") + 0xc000);
			memory_set_bankptr(machine, 5, memory_region(machine, "maincpu") + 0xe000);

			memory_install_write8_handler(space, 0x6000, 0x7fff, 0, 0, nes_mid_mapper_w);
			memory_install_write8_handler(space, 0x8000, 0xffff, 0, 0, nes_mapper_w);
			break;
	}

	/* Set up the mapper callbacks */
	if (nes.format == 1)
	{
		const mmc *mapper;

		mapper = nes_mapper_lookup(nes.mapper);
		if (mapper)
		{
			mmc_write_low = mapper->mmc_write_low;
			mmc_read_low = mapper->mmc_read_low;
			mmc_write_mid = mapper->mmc_write_mid;
			mmc_write = mapper->mmc_write;
			ppu_latch = mapper->ppu_latch;
		}
		else
		{
			logerror("Mapper %d is not yet supported, defaulting to no mapper.\n",nes.mapper);
			mmc_write_low = mmc_write_mid = mmc_write = NULL;
			mmc_read_low = NULL;
			ppu_latch = NULL;
		}
	}
	else if (nes.format == 2)
	{
		const unif *board;

		board = nes_unif_lookup(nes.board);
		if (board)
		{
			mmc_write_low = board->mmc_write_low;
			mmc_read_low = board->mmc_read_low;
			mmc_write_mid = board->mmc_write_mid;
			mmc_write = board->mmc_write;
			ppu_latch = board->ppu_latch;
		}
		else
		{
			logerror("Board %s is not yet supported, defaulting to no mapper.\n", nes.board);
			mmc_write_low = mmc_write_mid = mmc_write = NULL;
			mmc_read_low = NULL;
			ppu_latch = NULL;
		}
	}

	/* Load a battery file, but only if there's no trainer since they share */
	/* overlapping memory. */
	if (nes.trainer)
		return;

	/* We need this because battery ram is loaded before the */
	/* memory subsystem is set up. When this routine is called */
	/* everything is ready, so we can just copy over the data */
	/* we loaded before. */
	memcpy(nes_battery_ram, battery_data, BATTERY_SIZE);
}

int nes_ppu_vidaccess( const device_config *device, int address, int data )
{
	/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
	/* but, B-Wings, submath (j) seem to use this location differently... */
	if (nes.chr_chunks && (address & 0x3fff) < 0x2000)
	{
//      int vrom_loc;
//      vrom_loc = (nes_vram[(address & 0x1fff) >> 10] * 16) + (address & 0x3ff);
//      data = nes.vrom [vrom_loc];
	}
	return data;
}

MACHINE_RESET( nes )
{
	nes_state *state = machine->driver_data;

	state->ppu = devtag_get_device(machine, "ppu");
	state->sound = devtag_get_device(machine, "nessound");
	state->cart = devtag_get_device(machine, "cart");

	/* Some carts have extra RAM and require it on at startup, e.g. Metroid */
	nes.mid_ram_enable = 1;

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	if (nes.format == 1)
		mapper_reset(machine, nes.mapper);

	if (nes.format == 2)
		unif_reset(machine, nes.board);

	/* Reset the serial input ports */
	in_0.shift = 0;
	in_1.shift = 0;

	cputag_reset(machine, "maincpu");
}

MACHINE_START( nes )
{
	init_nes_core(machine);
	add_exit_callback(machine, nes_machine_stop);
}

static void nes_machine_stop( running_machine *machine )
{
	/* Write out the battery file if necessary */
	if (nes.battery)
		image_battery_save(cartslot_image(machine), nes_battery_ram, BATTERY_SIZE);
}



READ8_HANDLER( nes_IN0_r )
{
	nes_state *state = space->machine->driver_data;
	int ret;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	ret = 0x40;

	ret |= ((in_0.i0 >> in_0.shift) & 0x01);

	/* zapper */
	if ((input_port_read(space->machine, "CTRLSEL") & 0x000f) == 0x0002)
	{
		int x = in_0.i1;	/* read Zapper x-position */
		int y = in_0.i2;	/* read Zapper y-position */
		UINT32 pix, color_base;

		/* get the pixel at the gun position */
		pix = ppu2c0x_get_pixel(state->ppu, x, y);

		/* get the color base from the ppu */
		color_base = ppu2c0x_get_colorbase(state->ppu);

		/* look at the screen and see if the cursor is over a bright pixel */
		if ((pix == color_base + 0x20) || (pix == color_base + 0x30) ||
			(pix == color_base + 0x33) || (pix == color_base + 0x34))
		{
			ret &= ~0x08; /* sprite hit */
		}
		else
			ret |= 0x08;  /* no sprite hit */

		/* If button 1 is pressed, indicate the light gun trigger is pressed */
		ret |= ((in_0.i0 & 0x01) << 4);
	}

	if (LOG_JOY)
		logerror("joy 0 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", ret, cpu_get_pc(space->cpu), in_0.shift, in_0.i0);

	in_0.shift++;
	return ret;
}

READ8_HANDLER( nes_IN1_r )
{
	nes_state *state = space->machine->driver_data;
	int ret;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4017 leaves 0x40 there. */
	ret = 0x40;

	/* Handle data line 0's serial output */
	ret |= ((in_1.i0 >> in_1.shift) & 0x01);

	/* zapper */
	if ((input_port_read(space->machine, "CTRLSEL") & 0x00f0) == 0x0030)
	{
		int x = in_1.i1;	/* read Zapper x-position */
		int y = in_1.i2;	/* read Zapper y-position */
		UINT32 pix, color_base;

		/* get the pixel at the gun position */
		pix = ppu2c0x_get_pixel(state->ppu, x, y);

		/* get the color base from the ppu */
		color_base = ppu2c0x_get_colorbase(state->ppu);

		/* look at the screen and see if the cursor is over a bright pixel */
		if ((pix == color_base + 0x20) || (pix == color_base + 0x30) ||
			(pix == color_base + 0x33) || (pix == color_base + 0x34))
		{
			ret &= ~0x08; /* sprite hit */
		}
		else
			ret |= 0x08;  /* no sprite hit */

		/* If button 1 is pressed, indicate the light gun trigger is pressed */
		ret |= ((in_1.i0 & 0x01) << 4);
	}

	/* arkanoid dial */
	else if ((input_port_read(space->machine, "CTRLSEL") & 0x00f0) == 0x0040)
	{
		/* Handle data line 2's serial output */
		ret |= ((in_1.i2 >> in_1.shift) & 0x01) << 3;

		/* Handle data line 3's serial output - bits are reversed */
		/* NPW 27-Nov-2007 - there is no third subscript! commenting out */
		/* ret |= ((in_1[3] >> in_1.shift) & 0x01) << 4; */
		/* ret |= ((in_1[3] << in_1.shift) & 0x80) >> 3; */
	}

	if (LOG_JOY)
		logerror("joy 1 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", ret, cpu_get_pc(space->cpu), in_1.shift, in_1.i0);

	in_1.shift++;
	return ret;
}



static void nes_read_input_device( running_machine *machine, int cfg, nes_input *vals, int pad_port,
	int supports_zapper, int paddle_port )
{
	static const char *const padnames[] = { "PAD1", "PAD2", "PAD3", "PAD4" };

	vals->i0 = 0;
	vals->i1 = 0;
	vals->i2 = 0;

	switch(cfg & 0x0f)
	{
		case 0x01:	/* gamepad */
			if (pad_port >= 0)
				vals->i0 = input_port_read(machine, padnames[pad_port]);
			break;

		case 0x02:	/* zapper 1 */
			if (supports_zapper)
			{
				vals->i0 = input_port_read(machine, "ZAPPER1_T");
				vals->i1 = input_port_read(machine, "ZAPPER1_X");
				vals->i2 = input_port_read(machine, "ZAPPER1_Y");
			}
			break;

		case 0x03:	/* zapper 2 */
			if (supports_zapper)
			{
				vals->i0 = input_port_read(machine, "ZAPPER2_T");
				vals->i1 = input_port_read(machine, "ZAPPER2_X");
				vals->i2 = input_port_read(machine, "ZAPPER2_Y");
			}
			break;

		case 0x04:	/* arkanoid paddle */
			if (paddle_port >= 0)
				vals->i0 = (UINT8) ((UINT8) input_port_read(machine, "PADDLE") + (UINT8)0x52) ^ 0xff;
			break;
	}
}


static TIMER_CALLBACK( lightgun_tick )
{
	if ((input_port_read(machine, "CTRLSEL") & 0x000f) == 0x0002)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}

	if ((input_port_read(machine, "CTRLSEL") & 0x00f0) == 0x0030)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 1, CROSSHAIR_SCREEN_NONE);
	}
}

WRITE8_HANDLER( nes_IN0_w )
{
	int cfg;
	nes_input in_2;
	nes_input in_3;

	if (data & 0x01)
		return;

	if (LOG_JOY)
		logerror("joy 0 bits read: %d\n", in_0.shift);

	/* Toggling bit 0 high then low resets both controllers */
	in_0.shift = 0;
	in_1.shift = 0;

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	timer_set(space->machine, attotime_zero, NULL, 0, lightgun_tick);

	/* Check the configuration to see what's connected */
	cfg = input_port_read(space->machine, "CTRLSEL");

	/* Read the input devices */
	nes_read_input_device(space->machine, cfg >>  0, &in_0, 0,  TRUE, -1);
	nes_read_input_device(space->machine, cfg >>  4, &in_1, 1,  TRUE,  1);
	nes_read_input_device(space->machine, cfg >>  8, &in_2, 2, FALSE, -1);
	nes_read_input_device(space->machine, cfg >> 12, &in_3, 3, FALSE, -1);

	if (cfg & 0x0f00)
		in_0.i0 |= (in_2.i0 << 8) | (0x08 << 16);
	if (cfg & 0xf000)
		in_1.i0 |= (in_3.i0 << 8) | (0x04 << 16);
}



WRITE8_HANDLER( nes_IN1_w )
{
}



DEVICE_IMAGE_LOAD( nes_cart )
{
	const char *mapinfo;
	int mapint1 = 0, mapint2 = 0, mapint3 = 0, mapint4 = 0, goodcrcinfo = 0;
	char magic[4];
	char skank[8];
	int local_options = 0;
	char m;
	int i;
	const char *filetype = image_filetype(image);

	if (!mame_stricmp(filetype, "nes"))
	{
		nes.format = 1;	// TODO: use this to select between mapper_reset / unif_reset
		/* Verify the file is in iNES format */
		memset(magic, '\0', sizeof(magic));
		image_fread(image, magic, 4);

		if ((magic[0] != 'N') || (magic[1] != 'E') || (magic[2] != 'S'))
		{
			logerror("%s is NOT a file in iNES format.\n", image_filename(image));
			return INIT_FAIL;
		}

		mapinfo = image_extrainfo(image);

		if (mapinfo)
		{
			if (4 == sscanf(mapinfo,"%d %d %d %d", &mapint1, &mapint2, &mapint3, &mapint4))
			{
				nes.mapper = mapint1;
				local_options = mapint2 & 0x0f;
				nes.crc_hack = mapint2 & 0xf0;	// this is used to differentiate among variants of the same Mapper (see nes_mmc.c)
				nes.prg_chunks = mapint3;
				nes.chr_chunks = mapint4;
				logerror("NES.HSI info: %d %d %d %d\n", mapint1, mapint2, mapint3, mapint4);
				goodcrcinfo = 1;
			}
			else
			{
				logerror("NES: [%s], Invalid mapinfo found\n",mapinfo);
			}
		}
		else
		{
			logerror("NES: No extrainfo found\n");
		}

		if (!goodcrcinfo)
		{
			// image_extrainfo() resets the file position back to start.
			// Let's skip past the magic header once again.
			image_fseek(image, 4, SEEK_SET);

			image_fread(image, &nes.prg_chunks, 1);
			image_fread(image, &nes.chr_chunks, 1);
			/* Read the first ROM option byte (offset 6) */
			image_fread(image, &m, 1);

			/* Interpret the iNES header flags */
			nes.mapper = (m & 0xf0) >> 4;
			local_options = m & 0x0f;

			/* Read the second ROM option byte (offset 7) */
			image_fread(image, &m, 1);

			/* Check for skanky headers */
			image_fread(image, &skank, 8);

			/* If the header has junk in the unused bytes, assume the extra mapper byte is also invalid */
			/* We only check the first 4 unused bytes for now */
			for (i = 0; i < 4; i ++)
			{
				logerror("%02x ", skank[i]);
				if (skank[i] != 0x00)
				{
					logerror("(skank: %d)", i);
				}
			}
			logerror("\n");

			nes.mapper = nes.mapper | (m & 0xf0);
		}

		nes.hard_mirroring = local_options & 0x01;
		nes.battery = local_options & 0x02;
		nes.trainer = local_options & 0x04;
		nes.four_screen_vram = local_options & 0x08;

		if (nes.battery)
			logerror("-- Battery found\n");

		if (nes.trainer)
			logerror("-- Trainer found\n");

		if (nes.four_screen_vram)
			logerror("-- 4-screen VRAM\n");

		/* Free the regions that were allocated by the ROM loader */
		memory_region_free(image->machine, "maincpu");
		memory_region_free(image->machine, "gfx1");

		/* Allocate them again with the proper size */
		memory_region_alloc(image->machine, "maincpu", 0x10000 + (nes.prg_chunks + 1) * 0x4000,0);
		if (nes.chr_chunks)
			memory_region_alloc(image->machine, "gfx1", nes.chr_chunks * 0x2000,0);

		nes.rom = memory_region(image->machine, "maincpu");
		nes.vrom = memory_region(image->machine, "gfx1");
		nes.vram = memory_region(image->machine, "gfx2");
		nes.wram = memory_region(image->machine, "user1");

		/* Position past the header */
		image_fseek(image, 16, SEEK_SET);

		/* Load the 0x200 byte trainer at 0x7000 if it exists */
		if (nes.trainer)
			image_fread(image, &nes.wram[0x1000], 0x200);

		/* Read in the program chunks */
		if (nes.prg_chunks == 1)
		{
			image_fread(image, &nes.rom[0x14000], 0x4000);
			/* Mirror this bank into $8000 */
			memcpy(&nes.rom[0x10000], &nes.rom [0x14000], 0x4000);
		}
		else
			image_fread(image, &nes.rom[0x10000], 0x4000 * nes.prg_chunks);

#if SPLIT_PRG
		{
			FILE *prgout;
			char outname[255];

			for (i = 0; i < nes.prg_chunks; i ++)
			{
				sprintf(outname, "%s.p%d", image_filename(image), i);
				prgout = fopen(outname, "wb");
				if (prgout)
				{
					fwrite(&nes.rom[0x10000 + 0x4000 * i], 1, 0x4000, prgout);
					fclose(prgout);
				}
			}
		}
#endif /* SPLIT_PRG */

		logerror("**\n");
		logerror("Mapper: %d\n", nes.mapper);
		logerror("PRG chunks: %02x, size: %06x\n", nes.prg_chunks, 0x4000 * nes.prg_chunks);
      printf("Mapper: %d\n", nes.mapper);
      printf("PRG chunks: %02x, size: %06x\n", nes.prg_chunks, 0x4000 * nes.prg_chunks);

		/* Read in any chr chunks */
		if (nes.chr_chunks > 0)
		{
			image_fread(image, nes.vrom, nes.chr_chunks * 0x2000);
			if (nes.mapper == 2)
				logerror("Warning: VROM has been found in VRAM-based mapper. Either the mapper is set wrong or the ROM image is incorrect.\n");
		}

#if SPLIT_CHR
		{
			FILE *chrout;
			char outname[255];

			for (i = 0; i < nes.chr_chunks; i ++)
			{
				sprintf(outname, "%s.c%d", image_filename(image), i);
				chrout= fopen(outname, "wb");
				if (chrout)
				{
					fwrite(&nes.rom[0x10000 + 0x2000 * i], 1, 0x2000, chrout);
					fclose(chrout);
				}
			}
		}
#endif /* SPLIT_CHR */

		logerror("CHR chunks: %02x, size: %06x\n", nes.chr_chunks, 0x4000 * nes.chr_chunks);
		logerror("**\n");
      printf("CHR chunks: %02x, size: %06x\n", nes.chr_chunks, 0x4000 * nes.chr_chunks);
      printf("**\n");

		/* Attempt to load a battery file for this ROM. If successful, we */
		/* must wait until later to move it to the system memory. */
		if (nes.battery)
			image_battery_load(image, battery_data, BATTERY_SIZE);

	}
	else if (!mame_stricmp(filetype, "unf"))
	{
		UINT32 unif_ver = 0;
		char magic2[4];
		UINT8 buffer[4];
		UINT32 chunk_length = 0, read_length = 0x20;
		UINT32 prg_start = 0, chr_start = 0;
		UINT32 prg_left = 0, chr_left = 0;	// used to verify integrity of our unif image
		char unif_mapr[32];	// here we should store MAPR chunks
		UINT32 size = image_length(image);
		const unif *unif_board;

		nes.format = 2;	// TODO: use this to select between mapper_reset / unif_reset
		/* Verify the file is in iNES format */
		memset(magic, '\0', sizeof(magic));
		image_fread(image, magic, 4);

		if ((magic[0] != 'U') || (magic[1] != 'N') || (magic[2] != 'I') || (magic[3] != 'F'))
		{
			logerror("%s is NOT a file in UNIF format.\n", image_filename(image));
			return INIT_FAIL;
		}

		image_fread(image, &buffer, 4);
		unif_ver = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
		logerror("UNIF file found, version %d\n", unif_ver);

		if (size <= 0x20)
		{
			logerror("%s only contains the UNIF header and no data.\n", image_filename(image));
			return INIT_FAIL;
		}

		do
		{
			image_fseek(image, read_length, SEEK_SET);

			memset(magic2, '\0', sizeof(magic2));
			image_fread(image, &magic2, 4);

			/* Preliminary checks: the first chunk MUST be MAPR! */
			if (read_length == 0x20 && ((magic2[0] != 'M') || (magic2[1] != 'A') || (magic2[2] != 'P') || (magic2[3] != 'R')))
				fatalerror("First chunk of data in UNIF should be [MAPR]. Check if your image has been corrupted\n");

			/* Preliminary checks: multiple MAPR chunks are FORBIDDEN! */
			if (read_length > 0x20 && ((magic2[0] == 'M') && (magic2[1] == 'A') && (magic2[2] == 'P') && (magic2[3] == 'R')))
				fatalerror("UNIF should not have multiple [MAPR] chunks. Check if your image has been corrupted\n");

			/* What kind of chunk do we have here? */
			if ((magic2[0] == 'M') && (magic2[1] == 'A') && (magic2[2] == 'P') && (magic2[3] == 'R'))
			{
//				int i;
				logerror("[MAPR] chunk found: ");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				if (chunk_length <= 0x20)
					image_fread(image, &unif_mapr, chunk_length);

				read_length += (chunk_length + 8);

				unif_board = nes_unif_lookup(unif_mapr);

//				for(i = 0; i < chunk_length; i++)
//					logerror("%c", unif_mapr[i]);
//				logerror("\n");
				logerror("%s\n", unif_mapr);

				if (unif_board == NULL)
				{
					fatalerror("Unsupported UNIF board.\n");
//                  logerror("Unsupported UNIF board.\n");
				}

				nes.mapper = 0;	// this allows us to set up memory handlers without duplicating code (for the moment)
				nes.board = unif_board->board;
				nes.prg_chunks = unif_board->prgrom;
				nes.chr_chunks = unif_board->chrrom;
				nes.battery = unif_board->wram;	// we should implement WRAM banks...
				nes.hard_mirroring = unif_board->nt;
//				nes.four_screen_vram = ;

				/* Free the regions that were allocated by the ROM loader */
				memory_region_free(image->machine, "maincpu");
				memory_region_free(image->machine, "gfx1");

				/* Allocate them again with the proper size */
				memory_region_alloc(image->machine, "maincpu", 0x10000 + (nes.prg_chunks + 1) * 0x4000, 0);
				if (nes.chr_chunks)
					memory_region_alloc(image->machine, "gfx1", nes.chr_chunks * 0x2000, 0);

				nes.rom = memory_region(image->machine, "maincpu");
				nes.vrom = memory_region(image->machine, "gfx1");
				nes.vram = memory_region(image->machine, "gfx2");
				nes.wram = memory_region(image->machine, "user1");

				/* for validation purposes */
				prg_left = unif_board->prgrom * 0x4000;
				chr_left = unif_board->chrrom * 0x2000;
			}
			else if ((magic2[0] == 'R') && (magic2[1] == 'E') && (magic2[2] == 'A') && (magic2[3] == 'D'))
			{
				logerror("[READ] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'N') && (magic2[1] == 'A') && (magic2[2] == 'M') && (magic2[3] == 'E'))
			{
				logerror("[NAME] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'T') && (magic2[1] == 'V') && (magic2[2] == 'C') && (magic2[3] == 'I'))
			{
				logerror("[TVCI] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'D') && (magic2[1] == 'I') && (magic2[2] == 'N') && (magic2[3] == 'F'))
			{
				logerror("[DINF] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'C') && (magic2[1] == 'T') && (magic2[2] == 'R') && (magic2[3] == 'L'))
			{
				logerror("[CTRL] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'B') && (magic2[1] == 'A') && (magic2[2] == 'T') && (magic2[3] == 'R'))
			{
				logerror("[BATR] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'V') && (magic2[1] == 'R') && (magic2[2] == 'O') && (magic2[3] == 'R'))
			{
				logerror("[VROR] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'M') && (magic2[1] == 'I') && (magic2[2] == 'R') && (magic2[3] == 'R'))
			{
				logerror("[MIRR] chunk found. No support yet.\n");
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'P') && (magic2[1] == 'C') && (magic2[2] == 'K'))
			{
				logerror("[PCK%c] chunk found. No support yet.\n", magic2[3]);
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'C') && (magic2[1] == 'C') && (magic2[2] == 'K'))
			{
				logerror("[CCK%c] chunk found. No support yet.\n", magic2[3]);
					image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'P') && (magic2[1] == 'R') && (magic2[2] == 'G'))
			{
				logerror("[PRG%c] chunk found. ", magic2[3]);
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				logerror("It consists of %d 16K-blocks.\n", chunk_length / 0x4000);
				/* Validation */
				prg_left -= chunk_length;
				if (prg_left < 0)
					fatalerror("PRG chunks larger than expected by board %s!\n", unif_mapr);

				/* Read in the program chunks */
				if (nes.prg_chunks == 1)
				{
					image_fread(image, &nes.rom[0x14000], chunk_length);
					/* Mirror the only bank into $8000 */
					memcpy(&nes.rom[0x10000], &nes.rom[0x14000], chunk_length);
				}
				else
					image_fread(image, &nes.rom[0x10000 + prg_start], chunk_length);

				prg_start += chunk_length;
				read_length += (chunk_length + 8);
			}
			else if ((magic2[0] == 'C') && (magic2[1] == 'H') && (magic2[2] == 'R'))
			{
				logerror("[CHR%c] chunk found. ", magic2[3]);
				image_fread(image, &buffer, 4);
				chunk_length = buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);

				logerror("It consists of %d 8K-blocks.\n", chunk_length / 0x2000);
				/* validation */
				chr_left -= chunk_length;
				if (chr_left < 0)
					fatalerror("CHR chunks larger than expected by board %s!\n", unif_mapr);

				image_fread(image, nes.vrom + chr_start, chunk_length);

				chr_start += chunk_length;
				read_length += (chunk_length + 8);
			}
			else
			{
				printf("Unsupported UNIF chunk. Please report the problem at MESS Board.\n");
				read_length = size;
			}
		} while (size > read_length);

	logerror("UNIF support is only very preliminary.\n");
	}

	return INIT_PASS;
}



void nes_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 16)
		return;
	hash_compute(dest, &data[16], length - 16, functions);
}


DEVICE_START(nes_disk)
{
	/* clear some of the cart variables we don't use */
	nes.trainer = 0;
	nes.battery = 0;
	nes.prg_chunks = nes.chr_chunks = 0;

	nes.mapper = 20;
	nes.four_screen_vram = 0;
	nes.hard_mirroring = 0;

	nes_fds.sides = 0;
	nes_fds.data = NULL;
}


DEVICE_IMAGE_LOAD(nes_disk)
{
	unsigned char magic[4];

	/* See if it has a  redundant header on it */
	image_fread(image, magic, 4);
	if ((magic[0] == 'F') && (magic[1] == 'D') && (magic[2] == 'S'))
	{
		/* Skip past the redundant header */
		image_fseek(image, 0x10, SEEK_SET);
	}
	else
		/* otherwise, point to the start of the image */
		image_fseek(image, 0, SEEK_SET);

	/* read in all the sides */
	while (!image_feof(image))
	{
		nes_fds.sides ++;
		nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);
		if (!nes_fds.data)
			return INIT_FAIL;
		image_fread(image, nes_fds.data + ((nes_fds.sides - 1) * 65500), 65500);
	}

	logerror("Number of sides: %d\n", nes_fds.sides);

	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(nes_disk)
{
	/* TODO: should write out changes here as well */
	nes_fds.data = NULL;
}
