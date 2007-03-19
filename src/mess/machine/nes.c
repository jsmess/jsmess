#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "video/generic.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"
#include "sound/nes_apu.h"
#include "zlib.h"
#include "image.h"
#include "hash.h"



/* Uncomment this to dump info about the inputs to the errorlog */
//#define LOG_JOY

/* Uncomment this to generate prg chunk files when the cart is loaded */
//#define SPLIT_PRG

#define BATTERY_SIZE 0x2000
UINT8 battery_data[BATTERY_SIZE];

struct nes_struct nes;
struct fds_struct nes_fds;

static UINT32 in_0[3];
static UINT32 in_1[3];
static UINT32 in_0_shift;
static UINT32 in_1_shift;

/* local prototypes */
static void init_nes_core(void);
static void nes_machine_stop(running_machine *machine);

static mess_image *cartslot_image(void)
{
	return image_from_devtype_and_index(IO_CARTSLOT, 0);
}

static void init_nes_core (void)
{
	/* We set these here in case they weren't set in the cart loader */
	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	/* Brutal hack put in as a consequence of the new memory system; we really
	 * need to fix the NES code */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0x1800, MRA8_BANK10);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x07ff, 0, 0x1800, MWA8_BANK10);
	memory_set_bankptr(10, nes.rom);

	battery_ram = nes.wram;

	/* Set up the memory handlers for the mapper */
	switch (nes.mapper)
	{
		case 20:
			nes.slow_banking = 0;
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4030, 0x403f, 0, 0, fds_r);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0xdfff, 0, 0, MRA8_RAM);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, MRA8_BANK1);
			memory_set_bankptr(1, &nes.rom[0xe000]);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4020, 0x402f, 0, 0, fds_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0xdfff, 0, 0, MWA8_RAM);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, MWA8_ROM);
			break;
		case 40:
			nes.slow_banking = 1;
			/* Game runs code in between banks, so we do things different */
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MRA8_RAM);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, MRA8_ROM);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, nes_mid_mapper_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, nes_mapper_w);
			break;
		default:
			nes.slow_banking = 0;
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MRA8_BANK5);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x9fff, 0, 0, MRA8_BANK1);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, MRA8_BANK2);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, MRA8_BANK3);
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, MRA8_BANK4);

			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, nes_mid_mapper_w);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, nes_mapper_w);
			break;
	}

	/* Set up the mapper callbacks */
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
//			mmc_irq = mmc_list[i].mmc_irq;
		}
		else
		{
			logerror ("Mapper %d is not yet supported, defaulting to no mapper.\n",nes.mapper);
			mmc_write_low = mmc_write_mid = mmc_write = NULL;
			mmc_read_low = NULL;
		}
	}

	/* Load a battery file, but only if there's no trainer since they share */
	/* overlapping memory. */
	if (nes.trainer) return;

	/* We need this because battery ram is loaded before the */
	/* memory subsystem is set up. When this routine is called */
	/* everything is ready, so we can just copy over the data */
	/* we loaded before. */
	memcpy (battery_ram, battery_data, BATTERY_SIZE);
}

int nes_ppu_vidaccess( int num, int address, int data )
{
	/* TODO: this is a bit of a hack, needed to get Argus, ASO, etc to work */
	/* but, B-Wings, submath (j) seem to use this location differently... */
	if (nes.chr_chunks && (address & 0x3fff) < 0x2000)
	{
//		int vrom_loc;
//		vrom_loc = (nes_vram[(address & 0x1fff) >> 10] * 16) + (address & 0x3ff);
//		data = nes.vrom [vrom_loc];
	}
	return data;
}

static void nes_machine_reset(running_machine *machine)
{
	/* Some carts have extra RAM and require it on at startup, e.g. Metroid */
	nes.mid_ram_enable = 1;

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset (nes.mapper);

	/* Reset the serial input ports */
	in_0_shift = 0;
	in_1_shift = 0;
}

MACHINE_START( nes )
{
	init_nes_core();
	add_reset_callback(machine, nes_machine_reset);
	add_exit_callback(machine, nes_machine_stop);

	if (!image_exists(image_from_devtype_and_index(IO_CARTSLOT, 0)))
	{
		/* NPW 05-Mar-2006 - Hack to keep the Famicom from crashing */
		static const UINT8 infinite_loop[] = { 0x4C, 0xF9, 0xFF, 0xF9, 0xFF }; /* JMP $FFF9, DC.W $FFF9 */
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xFFF9, 0xFFFD, 0, 0, MRA8_BANK11);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xFFF9, 0xFFFD, 0, 0, MWA8_BANK11);
		memory_set_bankptr(11, (void *) infinite_loop);
	}
	return 0;
}

static void nes_machine_stop(running_machine *machine)
{
	/* Write out the battery file if necessary */
	if (nes.battery)
		image_battery_save(cartslot_image(), battery_ram, BATTERY_SIZE);
}



static int zapper_hit_pixel(const UINT32 *input)
{
	UINT16 pix = 0;
	rgb_t col;
	UINT8 r, g, b;
	extern mame_bitmap *nes_zapper_hack;

	if (nes_zapper_hack)
		pix = read_pixel(nes_zapper_hack, input[1], input[2]);

	col = palette_get_color(Machine, pix);
	r = (UINT8) (col >> 16);
	g = (UINT8) (col >> 8);
	b = (UINT8) (col >> 0);

	return (((UINT16) r) + ((UINT16) g) + ((UINT16) b)) >= 240*3;
}



 READ8_HANDLER ( nes_IN0_r )
{
	int cfg;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4016 leaves 0x40 there. */
	retVal = 0x40;

	retVal |= ((in_0[0] >> in_0_shift) & 0x01);

	/* Check the configuration to see what's connected */
	cfg = readinputport(PORT_CONFIG1);

	if (((cfg & 0x000f) == 0x0002) || ((cfg & 0x000f) == 0x0003))
	{
		/* If button 1 is pressed, indicate the light gun trigger is pressed */
		retVal |= ((in_0[0] & 0x01) << 4);

		/* Look at the screen and see if the cursor is over a bright pixel */
		if (zapper_hit_pixel(in_0))
			retVal &= ~0x08; /* sprite hit */
		else
			retVal |= 0x08;  /* no sprite hit */
	}

#ifdef LOG_JOY
	logerror ("joy 0 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_0_shift, in_0[0]);
#endif

	in_0_shift ++;
	return retVal;
}

 READ8_HANDLER ( nes_IN1_r )
{
	int cfg;
	int retVal;

	/* Some games expect bit 6 to be set because the last entry on the data bus shows up */
	/* in the unused upper 3 bits, so typically a read from $4017 leaves 0x40 there. */
	retVal = 0x40;

	/* Handle data line 0's serial output */
	retVal |= ((in_1[0] >> in_1_shift) & 0x01);

	/* Check the fake dip to see what's connected */
	cfg = readinputport(PORT_CONFIG1);

	if (((cfg & 0x00f0) == 0x0020) || ((cfg & 0x00f0) == 0x0030))
	{
		/* zapper */
		/* If button 1 is pressed, indicate the light gun trigger is pressed */
		retVal |= ((in_1[0] & 0x01) << 4);

		/* Look at the screen and see if the cursor is over a bright pixel */
		if (zapper_hit_pixel(in_1))
			retVal &= ~0x08; /* sprite hit */
		else
			retVal |= 0x08;  /* no sprite hit */
	}
	else if ((cfg & 0x00f0) == 0x0040)
	{
		/* arkanoid dial */
		/* Handle data line 2's serial output */
		retVal |= ((in_1[2] >> in_1_shift) & 0x01) << 3;

		/* Handle data line 3's serial output - bits are reversed */
//		retVal |= ((in_1[3] >> in_1_shift) & 0x01) << 4;
		retVal |= ((in_1[3] << in_1_shift) & 0x80) >> 3;
	}

#ifdef LOG_JOY
	logerror ("joy 1 read, val: %02x, pc: %04x, bits read: %d, chan0: %08x\n", retVal, activecpu_get_pc(), in_1_shift, in_1[0]);
#endif

	in_1_shift ++;
	return retVal;
}



static void nes_read_input_device(int cfg, UINT32 *vals, int pad_port,
	int supports_zapper, int paddle_port)
{
	vals[0] = 0;
	vals[1] = 0;
	vals[2] = 0;
	
	switch(cfg & 0x0f)
	{
		case 0x01:	/* gamepad */
			if (pad_port >= 0)
				vals[0] = readinputport(pad_port);
			break;

		case 0x02:	/* zapper 1 */
			if (supports_zapper)
			{
				vals[0] = readinputport(PORT_ZAPPER0_T);
				vals[1] = readinputport(PORT_ZAPPER0_X);
				vals[2] = readinputport(PORT_ZAPPER0_Y);
			}
			break;

		case 0x03:	/* zapper 2 */
			if (supports_zapper)
			{
				vals[0] = readinputport(PORT_ZAPPER1_T);
				vals[1] = readinputport(PORT_ZAPPER1_X);
				vals[2] = readinputport(PORT_ZAPPER1_Y);
			}
			break;

		case 0x04:	/* arkanoid paddle */
			if (paddle_port >= 0)
				vals[0] = (UINT8) ((UINT8) readinputport (paddle_port) + (UINT8)0x52) ^ 0xff;
			break;
	}
}



WRITE8_HANDLER ( nes_IN0_w )
{
	int cfg;
	UINT32 in_2[3];
	UINT32 in_3[3];

	if (data & 0x01) return;
#ifdef LOG_JOY
	logerror ("joy 0 bits read: %d\n", in_0_shift);
#endif

	/* Toggling bit 0 high then low resets both controllers */
	in_0_shift = 0;
	in_1_shift = 0;

	/* Check the configuration to see what's connected */
	cfg = readinputport(PORT_CONFIG1);

	/* Read the input devices */
	nes_read_input_device(cfg >>  0, in_0, PORT_PAD0,  TRUE, -1);
	nes_read_input_device(cfg >>  4, in_1, PORT_PAD1,  TRUE, PORT_PADDLE1);
	nes_read_input_device(cfg >>  8, in_2, PORT_PAD2, FALSE, -1);
	nes_read_input_device(cfg >> 12, in_3, PORT_PAD3, FALSE, -1);

	if (cfg & 0x0f00)
		in_0[0] |= (in_2[0] << 8) | (0x08 << 16);
	if (cfg & 0xf000)
		in_1[0] |= (in_3[0] << 8) | (0x04 << 16);
}



WRITE8_HANDLER ( nes_IN1_w )
{
}



DEVICE_LOAD(nes_cart)
{
	const char *mapinfo;
	int mapint1=0,mapint2=0,mapint3=0,mapint4=0,goodcrcinfo = 0;
	char magic[4];
	char skank[8];
	int local_options = 0;
	char m;
	int i;

	/* Verify the file is in iNES format */
	memset(magic, '\0', sizeof(magic));
	image_fread(image, magic, 4);

	if ((magic[0] != 'N') ||
		(magic[1] != 'E') ||
		(magic[2] != 'S'))
		goto bad;

	mapinfo = image_extrainfo(image);
	if (mapinfo)
	{
		if (4 == sscanf(mapinfo,"%d %d %d %d",&mapint1,&mapint2,&mapint3,&mapint4))
		{
			nes.mapper = mapint1;
			local_options = mapint2;
			nes.prg_chunks = mapint3;
			nes.chr_chunks = mapint4;
			logerror("NES.CRC info: %d %d %d %d\n",mapint1,mapint2,mapint3,mapint4);
			goodcrcinfo = 1;
		} else
		{
			logerror("NES: [%s], Invalid mapinfo found\n",mapinfo);
		}
	} else
	{
		logerror("NES: No extrainfo found\n");
	}
	if (!goodcrcinfo)
	{
		// image_extrainfo() resets the file position back to start.
		// Let's skip past the magic header once again.
		image_fseek (image, 4, SEEK_SET);

		image_fread (image, &nes.prg_chunks, 1);
		image_fread (image, &nes.chr_chunks, 1);
		/* Read the first ROM option byte (offset 6) */
		image_fread (image, &m, 1);

		/* Interpret the iNES header flags */
		nes.mapper = (m & 0xf0) >> 4;
		local_options = m & 0x0f;


		/* Read the second ROM option byte (offset 7) */
		image_fread (image, &m, 1);

		/* Check for skanky headers */
		image_fread (image, &skank, 8);

		/* If the header has junk in the unused bytes, assume the extra mapper byte is also invalid */
		/* We only check the first 4 unused bytes for now */
		for (i = 0; i < 4; i ++)
		{
			logerror("%02x ", skank[i]);
			if (skank[i] != 0x00)
			{
				logerror("(skank: %d)", i);
//				m = 0;
			}
		}
		logerror("\n");

		nes.mapper = nes.mapper | (m & 0xf0);
	}

	nes.hard_mirroring = local_options & 0x01;
	nes.battery = local_options & 0x02;
	nes.trainer = local_options & 0x04;
	nes.four_screen_vram = local_options & 0x08;

	if (nes.battery) logerror("-- Battery found\n");
	if (nes.trainer) logerror("-- Trainer found\n");
	if (nes.four_screen_vram) logerror("-- 4-screen VRAM\n");

	/* Free the regions that were allocated by the ROM loader */
	free_memory_region (Machine, REGION_CPU1);
	free_memory_region (Machine, REGION_GFX1);

	/* Allocate them again with the proper size */
	new_memory_region(Machine, REGION_CPU1, 0x10000 + (nes.prg_chunks+1) * 0x4000,0);
	if (nes.chr_chunks)
		new_memory_region(Machine, REGION_GFX1, nes.chr_chunks * 0x2000,0);

	nes.rom = memory_region(REGION_CPU1);
	nes.vrom = memory_region(REGION_GFX1);
	nes.vram = memory_region(REGION_GFX2);
	nes.wram = memory_region(REGION_USER1);

	/* Position past the header */
	image_fseek (image, 16, SEEK_SET);

	/* Load the 0x200 byte trainer at 0x7000 if it exists */
	if (nes.trainer)
	{
		image_fread (image, &nes.wram[0x1000], 0x200);
	}

	/* Read in the program chunks */
	if (nes.prg_chunks == 1)
	{
		image_fread (image, &nes.rom[0x14000], 0x4000);
		/* Mirror this bank into $8000 */
		memcpy (&nes.rom[0x10000], &nes.rom [0x14000], 0x4000);
	}
	else
		image_fread (image, &nes.rom[0x10000], 0x4000 * nes.prg_chunks);

#ifdef SPLIT_PRG
{
	FILE *prgout;
	char outname[255];

	for (i = 0; i < nes.prg_chunks; i ++)
	{
		sprintf (outname, "%s.p%d", battery_name, i);
		prgout = fopen (outname, "wb");
		if (prgout)
		{
			fwrite (&nes.rom[0x10000 + 0x4000 * i], 1, 0x4000, prgout);
			fclose (prgout);
		}
	}
}
#endif

	logerror("**\n");
	logerror("Mapper: %d\n", nes.mapper);
	logerror("PRG chunks: %02x, size: %06x\n", nes.prg_chunks, 0x4000*nes.prg_chunks);

	/* Read in any chr chunks */
	if (nes.chr_chunks > 0)
	{
		image_fread (image, nes.vrom, nes.chr_chunks * 0x2000);
		if (nes.mapper == 2)
			logerror("Warning: VROM has been found in VRAM-based mapper. Either the mapper is set wrong or the ROM image is incorrect.\n");
	}

	logerror("CHR chunks: %02x, size: %06x\n", nes.chr_chunks, 0x4000*nes.chr_chunks);
	logerror("**\n");

	/* Attempt to load a battery file for this ROM. If successful, we */
	/* must wait until later to move it to the system memory. */
	if (nes.battery)
		image_battery_load(image, battery_data, BATTERY_SIZE);

	return INIT_PASS;

bad:
	logerror("BAD section hit during LOAD ROM.\n");
	return INIT_FAIL;
}



void nes_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 16)
		return;
	hash_compute(dest, &data[16], length - 16, functions);
}



DEVICE_LOAD(nes_disk)
{
	unsigned char magic[4];

	/* See if it has a fucking redundant header on it */
	image_fread(image, magic, 4);
	if ((magic[0] == 'F') &&
		(magic[1] == 'D') &&
		(magic[2] == 'S'))
	{
		/* Skip past the fucking redundant header */
		image_fseek (image, 0x10, SEEK_SET);
	}
	else
		/* otherwise, point to the start of the image */
		image_fseek (image, 0, SEEK_SET);

	/* clear some of the cart variables we don't use */
	nes.trainer = 0;
	nes.battery = 0;
	nes.prg_chunks = nes.chr_chunks = 0;

	nes.mapper = 20;
	nes.four_screen_vram = 0;
	nes.hard_mirroring = 0;

	nes_fds.sides = 0;
	nes_fds.data = NULL;

	/* read in all the sides */
	while (!image_feof (image))
	{
		nes_fds.sides ++;
		nes_fds.data = image_realloc(image, nes_fds.data, nes_fds.sides * 65500);
		if (!nes_fds.data)
			return INIT_FAIL;
		image_fread (image, nes_fds.data + ((nes_fds.sides-1) * 65500), 65500);
	}

	logerror ("Number of sides: %d\n", nes_fds.sides);

	return INIT_PASS;

//bad:
	logerror("BAD section hit during disk load.\n");
	return 1;
}

DEVICE_UNLOAD(nes_disk)
{
	/* TODO: should write out changes here as well */
	nes_fds.data = NULL;
}

void ppu_mirror_custom (int page, int address)
{
	fatalerror("Unimplemented");
}

void ppu_mirror_custom_vrom (int page, int address)
{
	fatalerror("Unimplemented");
}
