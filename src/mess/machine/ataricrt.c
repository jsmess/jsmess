/***************************************************************************

    MESS specific Atari init and Cartridge code for Atari 8 bit systems

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "includes/atari.h"
#include "ataridev.h"
#include "devices/messram.h"

static int a800_cart_loaded = 0;
static int a800_cart_is_16k = 0;
static int atari = 0;

/*************************************
 *
 *  Generic code
 *
 *************************************/

#ifdef UNUSED_CODE
DRIVER_INIT( atari )
{
	offs_t ram_top;
	offs_t ram_size;

	if (!strcmp(machine->gamedrv->name, "a5200")
		|| !strcmp(machine->gamedrv->name, "a600xl"))
	{
		ram_size = 0x8000;
	}
	else
	{
		ram_size = 0xa000;
	}

	/* install RAM */
	ram_top = MIN(messram_get_size(devtag_get_device(machine, "messram")), ram_size) - 1;
	memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_set_bankptr(machine, 2, messram_get_ptr(devtag_get_device(machine, "messram")));
}
#endif


static void a800_setbank(running_machine *machine, int n)
{
	void *read_addr;
	void *write_addr;
	UINT8 *mem = memory_region(machine, "maincpu");

	switch (n)
	{
		case 1:
			read_addr = &mem[0x10000];
			write_addr = NULL;
			break;
		default:
			if( atari <= ATARI_400 )
			{
				/* Atari 400 has no RAM here, so install the NOP handler */
				read_addr = NULL;
				write_addr = NULL;
			}
			else
			{
				read_addr = &messram_get_ptr(devtag_get_device(machine, "messram"))[0x08000];
				write_addr = &messram_get_ptr(devtag_get_device(machine, "messram"))[0x08000];
			}
			break;
	}

	if (read_addr) {
		memory_set_bankptr(machine, "bank1", read_addr);
		memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0,"bank1");
	} else {
		memory_nop_read(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0);
	}
	if (write_addr) {
		memory_set_bankptr(machine, "bank1", write_addr);
		memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0,"bank1");
	} else {
		memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0);
	}
}


static void cart_reset(running_machine *machine)
{
	if (a800_cart_loaded)
		a800_setbank(machine, 1);
}

/* MESS specific parts that have to be started */
static void ms_atari_machine_start(running_machine *machine, int type, int has_cart)
{
	offs_t ram_top;
	offs_t ram_size;

	/* set atari type (needed for banks above) */
	atari = type;

	/* determine RAM */
	if (!strcmp(machine->gamedrv->name, "a400")
		|| !strcmp(machine->gamedrv->name, "a400pal")
		|| !strcmp(machine->gamedrv->name, "a800")
		|| !strcmp(machine->gamedrv->name, "a800pal")
		|| !strcmp(machine->gamedrv->name, "a800xl"))
	{
		ram_size = 0xA000;
	}
	else
	{
		ram_size = 0x8000;
	}

	/* install RAM */
	ram_top = MIN(messram_get_size(devtag_get_device(machine, "messram")), ram_size) - 1;
	memory_install_readwrite_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),0x0000, ram_top, 0, 0, "bank2");
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));

	/* cartridge */
	if (has_cart)
		add_reset_callback(machine, cart_reset);
}

static void ms_atari800xl_machine_start(running_machine *machine, int type, int has_cart)
{
	/* set atari type (needed for banks above) */
	atari = type;

	/* cartridge */
	if (has_cart)
		add_reset_callback(machine, cart_reset);
}

/*************************************
 *
 *  Atari 400
 *
 *************************************/

MACHINE_START( a400 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_400, TRUE);
}


/*************************************
 *
 *  Atari 800
 *
 *************************************/

MACHINE_START( a800 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_800, TRUE);
}


DEVICE_IMAGE_LOAD( a800_cart )
{
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	int size;

	/* load an optional (dual) cartridge (e.g. basic.rom) */
	if( strcmp(image.device().tag(),"cart2") == 0 )
	{
		size = image.fread(&mem[0x12000], 0x2000);
		a800_cart_is_16k = (size == 0x2000);
		logerror("%s loaded right cartridge '%s' size 16K\n", image.device().machine->gamedrv->name, image.filename() );
	}
	else
	{
		size = image.fread(&mem[0x10000], 0x2000);
		a800_cart_loaded = size > 0x0000;
		size = image.fread(&mem[0x12000], 0x2000);
		a800_cart_is_16k = size > 0x2000;
		logerror("%s loaded left cartridge '%s' size %s\n", image.device().machine->gamedrv->name, image.filename() , (a800_cart_is_16k) ? "16K":"8K");
	}
	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( a800_cart )
{
	if( strcmp(image.device().tag(),"cart2") == 0 )
	{
		a800_cart_is_16k = 0;
		a800_setbank(image.device().machine, 1);
    }
	else
	{
		a800_cart_loaded = 0;
		a800_setbank(image.device().machine, 0);
    }
}


/*************************************
 *
 *  Atari 800XL
 *
 *************************************/

MACHINE_START( a800xl )
{
	atari_machine_start(machine);
	ms_atari800xl_machine_start(machine, ATARI_800XL, TRUE);
}

DEVICE_IMAGE_LOAD( a800xl_cart )
{
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	astring *fname;
	mame_file *basic_fp;
	file_error filerr;
	unsigned size;

	fname = astring_assemble_3(astring_alloc(), image.device().machine->gamedrv->name, PATH_SEPARATOR, "basic.rom");
	filerr = mame_fopen(SEARCHPATH_ROM, astring_c(fname), OPEN_FLAG_READ, &basic_fp);
	astring_free(fname);

	if (filerr != FILERR_NONE)
	{
		size = mame_fread(basic_fp, &mem[0x14000], 0x2000);
		if( size < 0x2000 )
		{
			logerror("%s image '%s' load failed (less than 8K)\n", image.device().machine->gamedrv->name, astring_c(fname));
			mame_fclose(basic_fp);
			return 2;
		}
	}

	/* load an optional (dual) cartidge (e.g. basic.rom) */
	if (filerr != FILERR_NONE)
	{
		{
			size = image.fread(&mem[0x14000], 0x2000);
			a800_cart_loaded = size / 0x2000;
			size = image.fread(&mem[0x16000], 0x2000);
			a800_cart_is_16k = size / 0x2000;
			logerror("%s loaded cartridge '%s' size %s\n",
					image.device().machine->gamedrv->name, image.filename(), (a800_cart_is_16k) ? "16K":"8K");
		}
		mame_fclose(basic_fp);
	}

	return IMAGE_INIT_PASS;
}


/*************************************
 *
 *  Atari 5200 console
 *
 *************************************/

MACHINE_START( a5200 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_800XL, TRUE);
}


DEVICE_IMAGE_LOAD( a5200_cart )
{
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	int size;

	/* load an optional (dual) cartidge */
	size = image.fread(&mem[0x4000], 0x8000);
	if (size<0x8000) memmove(mem+0x4000+0x8000-size, mem+0x4000, size);
	// mirroring of smaller cartridges
	if (size <= 0x1000) memcpy(mem+0xa000, mem+0xb000, 0x1000);
	if (size <= 0x2000) memcpy(mem+0x8000, mem+0xa000, 0x2000);
	if (size <= 0x4000)
	{
		const char *info;
		memcpy(&mem[0x4000], &mem[0x8000], 0x4000);
		info = image.extrainfo();
		if (info!=NULL && strcmp(info, "A13MIRRORING")==0)
		{
			memcpy(&mem[0x8000], &mem[0xa000], 0x2000);
			memcpy(&mem[0x6000], &mem[0x4000], 0x2000);
		}
	}
	logerror("%s loaded cartridge '%s' size %dK\n",
		image.device().machine->gamedrv->name, image.filename() , size/1024);
	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( a5200_cart )
{
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	/* zap the cartridge memory (again) */
	memset(&mem[0x4000], 0x00, 0x8000);
}

/*************************************
 *
 *  Atari XEGS
 *
 *************************************/

static UINT8 xegs_banks = 0;
static UINT8 xegs_cart = 0;

static WRITE8_HANDLER( xegs_bankswitch )
{
	UINT8 *cart = memory_region(space->machine, "user1");
	data &= xegs_banks - 1;
	memory_set_bankptr(space->machine, "bank0", cart + data * 0x2000);
}

MACHINE_START( xegs )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *cart = memory_region(space->machine, "user1");
	UINT8 *cpu  = memory_region(space->machine, "maincpu");
	
	atari_machine_start(machine);
	memory_install_write8_handler(space, 0xd500, 0xd5ff, 0, 0, xegs_bankswitch);

	if (xegs_cart)
	{
		memory_set_bankptr(machine, "bank0", cart);
		memory_set_bankptr(machine, "bank1", cart + (xegs_banks - 1) * 0x2000);
	}
	else
	{
		// point to built-in Missile Command (this does not work well, though... FIXME!!)
		memory_set_bankptr(machine, "bank0", cpu + 0x10000);
		memory_set_bankptr(machine, "bank1", cpu + 0x10000);
	}
}

DEVICE_IMAGE_LOAD( xegs_cart )
{
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, "user1");
	
	if (image.software_entry() == NULL)
	{
		// skip the header
		image.fseek(0x10, SEEK_SET);
		size = image.length() - 0x10;
		if (image.fread(ptr, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr, image.get_software_region("rom"), size);
	}

	xegs_banks = size / 0x2000;
	xegs_cart = 1;

	return IMAGE_INIT_PASS;
}


