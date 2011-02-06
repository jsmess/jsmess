/***************************************************************************

    MESS specific Atari init and Cartridge code for Atari 8 bit systems

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "includes/atari.h"
#include "ataridev.h"
#include "machine/ram.h"

static int a800_cart_loaded = 0;
static int a800_cart_is_16k = 0;
static int atari = 0;

#define LEFT_CARTSLOT_MOUNTED  1
#define RIGHT_CARTSLOT_MOUNTED 2

/*************************************
 *
 *  Generic code
 *
 *************************************/


// Currently, the drivers have fixed 40k RAM, however the function here is ready for different sizes too
static void a800_setbank(running_machine *machine, int cart_mounted)
{
	offs_t ram_top;
	
	// take care of 0x0000-0x7fff: RAM or NOP
	ram_top = MIN(ram_get_size(machine->device(RAM_TAG)), 0x8000) - 1;
	memory_install_readwrite_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, ram_top, 0, 0, "0000");
	memory_set_bankptr(machine, "0000", ram_get_ptr(machine->device(RAM_TAG)));

	// take care of 0x8000-0x9fff: A800 -> either right slot or RAM or NOP, others -> RAM or NOP
	// is there anything in the right slot?
	if (cart_mounted & RIGHT_CARTSLOT_MOUNTED)
	{
		memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, "8000");
		memory_set_bankptr(machine, "8000", machine->region("rslot")->base());
		memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0);
	}
	else
	{
		ram_top = MIN(ram_get_size(machine->device(RAM_TAG)), 0xa000) - 1;
		if (ram_top > 0x8000)
		{
			memory_install_readwrite_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, ram_top, 0, 0, "8000");
			memory_set_bankptr(machine, "8000", ram_get_ptr(machine->device(RAM_TAG)) + 0x8000);
		}
	}
	
	// take care of 0xa000-0xbfff: is there anything in the left slot?
	if (cart_mounted & LEFT_CARTSLOT_MOUNTED)
	{
		if (atari == ATARI_800XL)
		{
			// FIXME: this is an hack to keep XL working until we clean up its memory map as well!
			if (a800_cart_is_16k)
			{
				memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0, "8000");
				memory_set_bankptr(machine, "8000", machine->region("lslot")->base());
				memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0x9fff, 0, 0);

				memcpy(machine->region("maincpu")->base() + 0x10000, machine->region("lslot")->base() + 0x2000, 0x2000);
			}
			else
				memcpy(machine->region("maincpu")->base() + 0x10000, machine->region("lslot")->base(), 0x2000);
		}
		else if (a800_cart_is_16k)
		{
			memory_set_bankptr(machine, "8000", machine->region("lslot")->base());
			memory_set_bankptr(machine, "a000", machine->region("lslot")->base() + 0x2000);
			memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0);
		}
		else
		{
			memory_set_bankptr(machine, "a000", machine->region("lslot")->base());
			memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xa000, 0xbfff, 0, 0);
		}
	}
}

/* MESS specific parts that have to be started */
static void ms_atari_machine_start(running_machine *machine, int type, int has_cart)
{
	/* set atari type (temporarily not used) */
	atari = type;
	a800_setbank(machine, a800_cart_loaded);
}

static void ms_atari800xl_machine_start(running_machine *machine, int type, int has_cart)
{
	/* set atari type (temporarily not used) */
	atari = type;
	a800_setbank(machine, a800_cart_loaded);
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


/* PCB */
enum
{
    A800_UNKNOWN = 0,
	A800_4K, A800_8K, A800_12K, A800_16K,
	A800_RIGHT_4K, A800_RIGHT_8K,
	OSS_034M, OSS_M091, PHOENIX_8K, XEGS_32K,
	BBSB, DIAMOND_64K, WILLIAMS_64K, EXPRESS_64,
	SPARTADOS_X
};

typedef struct _a800_pcb  a800_pcb;
struct _a800_pcb
{
	const char              *pcb_name;
	int                     pcb_id;
};

// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
// WARNING: most of these are still unsupported by the driver
static const a800_pcb pcb_list[] =
{
	{"standard 4k", A800_4K},
	{"standard 8k", A800_8K},
	{"standard 12k", A800_12K},
	{"standard 16k", A800_16K},
	{"right slot 4k", A800_RIGHT_4K},
	{"right slot 8k", A800_RIGHT_8K},
	
	{"oss 034m", OSS_034M},
	{"oss m091", OSS_M091},
	{"phoenix 8k", PHOENIX_8K},
	{"xegs 32k", XEGS_32K},
	{"bbsb", BBSB},	
	{"diamond 64k", DIAMOND_64K},
	{"williams 64k", WILLIAMS_64K},
	{"express 64", EXPRESS_64},
	{"spartados x", SPARTADOS_X},
	{"N/A", A800_UNKNOWN}
};

static int a800_get_pcb_id(const char *pcb)
{
	int	i;
	
	for (i = 0; i < ARRAY_LENGTH(pcb_list); i++)
	{
		if (!mame_stricmp(pcb_list[i].pcb_name, pcb))
			return pcb_list[i].pcb_id;
	}
	
	return A800_UNKNOWN;
}

// currently this does nothing, but it will eventually install the memory handlers required by the mappers
static void a800_setup_mappers(running_machine *machine, int type)
{
	switch (type)
	{
		case A800_4K:
		case A800_RIGHT_4K:
		case A800_12K:
		case A800_8K:
		case A800_16K:
		case A800_RIGHT_8K:
		case OSS_034M:
		case OSS_M091:
		case PHOENIX_8K:
		case XEGS_32K:
		case BBSB:
		case DIAMOND_64K:
		case WILLIAMS_64K:
		case EXPRESS_64:
		case SPARTADOS_X:
		default:
			break;
	}
}

static int a800_get_type(device_image_interface &image)
{
	UINT8 header[16];
	image.fread(header, 0x10);
	int hdr_type, cart_type = A800_UNKNOWN;

	// add check of CART format
	if (strncmp((const char *)header, "CART", 4))
		fatalerror("Invalid header detected!\n");

	hdr_type = (header[4] << 24) + (header[5] << 16) +  (header[6] << 8) + (header[7] << 0);
	switch (hdr_type)
	{
		case 1:
			cart_type = A800_8K;
			break;
		case 2:
			cart_type = A800_16K;
			break;
		case 3:
			cart_type = OSS_034M;
			break;
		case 8:
			cart_type = WILLIAMS_64K;
			break;
		case 9:
			cart_type = DIAMOND_64K;
			break;
		case 10:
			cart_type = EXPRESS_64;
			break;
		case 11:
			cart_type = SPARTADOS_X;
			break;
		case 12:
			cart_type = XEGS_32K;
			break;
		case 15:
			cart_type = OSS_M091;
			break;
		case 18:
			cart_type = BBSB;
			break;
		case 21:
			cart_type = A800_RIGHT_8K;
			break;
		case 39:
			cart_type = PHOENIX_8K;
			break;
		case 4:
		case 6:
		case 7:
		case 16:
		case 19:
		case 20:
			fatalerror("Cart type \"%d\" means this is an Atari 5200 cart.\n", hdr_type);
			break;
		default:
			mame_printf_info("Cart type \"%d\" is currently unsupported.\n", hdr_type);
			break;
	}	
	return cart_type;
}

static int a800_check_cart_type(device_image_interface &image)
{
	const char	*pcb_name;
	int type = A800_UNKNOWN;

	if (image.software_entry() == NULL)
	{
		UINT32 size = image.length();
		
		// check if there is an header, if so extract cart_type from it, otherwise 
		// try to guess the cart_type from the file size (notice that after the 
		// a800_get_type call, we point at the start of the data)
		if ((size % 0x1000) == 0x10)
			type = a800_get_type(image);
		else if (size == 0x4000)
			type = A800_16K;
		else if (size == 0x2000)
		{
			if (strcmp(image.device().tag(),"cart2") == 0)
				type = A800_RIGHT_8K;
			else
				type = A800_8K;
		}
		
		if ((strcmp(image.device().tag(),"cart2") == 0) && (type != A800_RIGHT_8K))
			fatalerror("You cannot load this image '%s' in the right slot", image.filename());
	}
	else
	{
		if ((pcb_name = image.get_feature("cart_type")) != NULL)
			type = a800_get_pcb_id(pcb_name);
		
		switch (type)
		{
			case A800_UNKNOWN:
			case A800_4K:
			case A800_RIGHT_4K:
			case A800_12K:
			case A800_8K:
			case A800_16K:
			case A800_RIGHT_8K:
				break;
			default:
				mame_printf_info("Cart type \"%s\" currently unsupported.\n", pcb_name);
				break;
		}
	}
	return type;
}

DEVICE_IMAGE_LOAD( a800_cart )
{
	int cart_type;
	UINT32 size, start = 0;

	a800_cart_loaded = a800_cart_loaded & ~LEFT_CARTSLOT_MOUNTED;
	a800_cart_is_16k = 0;
	cart_type = a800_check_cart_type(image);

	a800_setup_mappers(image.device().machine, cart_type);

	if (image.software_entry() == NULL)
	{
		size = image.length();
		// if there is an header, skip it
		if ((size % 0x1000) == 0x10)
		{
			size -= 0x10;
			start = 0x10;
		}
		image.fread(image.device().machine->region("lslot")->base(), size - start);
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(image.device().machine->region("lslot")->base(), image.get_software_region("rom"), size);
	}	

	a800_cart_loaded |= (size > 0x0000) ? 1 : 0;
	a800_cart_is_16k = (size > 0x2000) ? 1 : 0;

	logerror("%s loaded left cartridge '%s' size %s\n", image.device().machine->gamedrv->name, image.filename(), (a800_cart_is_16k) ? "16K":"8K");
	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_LOAD( a800_cart_right )
{
	int cart_type;
	UINT32 size, start = 0;
	
	a800_cart_loaded = a800_cart_loaded & ~RIGHT_CARTSLOT_MOUNTED;
	cart_type = a800_check_cart_type(image);
	
	a800_setup_mappers(image.device().machine, cart_type);
	
	if (image.software_entry() == NULL)
	{
		size = image.length();
		// if there is an header, skip it
		if ((size % 0x1000) == 0x10)
		{
			size -= 0x10;
			start = 0x10;
		}
		image.fread(image.device().machine->region("rslot")->base(), size - start);
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(image.device().machine->region("rslot")->base(), image.get_software_region("rom"), size);
	}	

	a800_cart_loaded |= (size > 0x0000) ? 2 : 0;

	logerror("%s loaded right cartridge '%s' size 8K\n", image.device().machine->gamedrv->name, image.filename());
	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( a800_cart )
{
	a800_cart_is_16k = 0;
	a800_cart_loaded = a800_cart_loaded & ~LEFT_CARTSLOT_MOUNTED;
	a800_setbank(image.device().machine, a800_cart_loaded);
}

DEVICE_IMAGE_UNLOAD( a800_cart_right )
{
	a800_cart_loaded = a800_cart_loaded & ~RIGHT_CARTSLOT_MOUNTED;
	a800_setbank(image.device().machine, a800_cart_loaded);
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
	UINT8 *mem = image.device().machine->region("maincpu")->base();
	int size;
	if (image.software_entry() == NULL)
	{
		/* load an optional (dual) cartidge */
		size = image.fread(&mem[0x4000], 0x8000);
	} else {
		size = image.get_software_region_length("rom");
		memcpy(mem + 0x4000, image.get_software_region("rom"), size);
	}
	if (size<0x8000) memmove(mem+0x4000+0x8000-size, mem+0x4000, size);
	// mirroring of smaller cartridges
	if (size <= 0x1000) memcpy(mem+0xa000, mem+0xb000, 0x1000);
	if (size <= 0x2000) memcpy(mem+0x8000, mem+0xa000, 0x2000);
	if (size <= 0x4000)
	{
		const char *info;
		memcpy(&mem[0x4000], &mem[0x8000], 0x4000);
		info = image.extrainfo();
		if (strcmp(info, "") && !strcmp(info, "A13MIRRORING"))
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
	UINT8 *mem = image.device().machine->region("maincpu")->base();
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
	UINT8 *cart = space->machine->region("user1")->base();
	data &= xegs_banks - 1;
	memory_set_bankptr(space->machine, "bank0", cart + data * 0x2000);
}

MACHINE_START( xegs )
{
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *cart = space->machine->region("user1")->base();
	UINT8 *cpu  = space->machine->region("maincpu")->base();

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
	UINT8 *ptr = image.device().machine->region("user1")->base();

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

DEVICE_IMAGE_UNLOAD( xegs_cart )
{
	xegs_cart = 0;
	xegs_banks = 0;
}
