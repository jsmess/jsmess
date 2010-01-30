/**********************************************************************

    ti99cart.c

    TI99 family cartridge management

    Michael Zapf, 2009

*********************************************************************/
#include "emu.h"
#include "ti99cart.h"
#include "cartslot.h"
#include "machine/ti99_4x.h"
#include "multcart.h"

typedef int assmfct(running_device *);

enum
{
	TI99CARTINFO_FCT_6000_R = DEVINFO_FCT_DEVICE_SPECIFIC,		/* read16_device_handler */
	TI99CARTINFO_FCT_6000_W,					/* write16_device_handler */
	TI99CARTINFO_FCT_6000_R8,					/* read8_device_handler */
	TI99CARTINFO_FCT_6000_W8,					/* write16_device_handler */
	TI99CARTINFO_FCT_CRU_R,
	TI99CARTINFO_FCT_CRU_W,
	TI99CART_FCT_ASSM,
	TI99CART_FCT_DISASSM
};

typedef enum _legacy_cart_t
{
	LEG_CART_NONE,
	LEG_CART_STD,
	LEG_CART_EXB,
	LEG_CART_MINI,
	LEG_CART_MBX
} legacy_cart_t;

struct _ti99_multicart_t
{
	/* Reserves space for all cartridges. This is also used in the legacy
    cartridge system, but only for slot 0. */
	cartridge_t cartridge[NUMBER_OF_CARTRIDGE_SLOTS];

	/* Determines which slot is currently active. This value is changed when there
    are accesses to other GROM base addresses. */
	int active_slot;

	/* Used in order to enforce a special slot. This value is retrieved
       from the dipswitch setting. A value of -1 means automatic, that is,
       the grom base switch is used. Values 0 .. max refer to the
       respective slot. */
	int fixed_slot;

	/* Holds the highest index of a cartridge being plugged in plus one.
       If we only have one cartridge inserted, we don't want to get a
       selection option, so we just mirror the memory contents. */
	int next_free_slot;

	/* Counts the number of slots which currently contain legacy format
       cartridge images. */
	int legacy_slots;

	/* Counts the number of slots which currently contain new format
       cartridge images. */
	int multi_slots;

	/* Legacy mode. Stores the slot number where the cartridge part is mounted.
       Index is from slotc_type_t. */
	int legacy_slotnumber[5];
};
typedef struct _ti99_multicart_t ti99_multicart_t;

#define AUTO -1

typedef enum _slotc_type_t
{
	SLOTC_EMPTY = -1,
	SLOTC_GROM = 0,
	SLOTC_CROM = 1,
	SLOTC_DROM = 2,
	SLOTC_MINIMEM = 3,
	SLOTC_MBX = 4
} slotc_type_t;

/* Function declaration; the function itself is in the later part of this file. */
static UINT8 cartridge_grom_read_legacy(running_device *cartsys, int cart_offset);
static void unload_legacy(running_device *image);
static int load_legacy(running_device *image);

/* Access to the pcb. Contained in the token of the pcb instance. */
struct _ti99_pcb_t
{
	/* Read function for this cartridge. */
	read16_device_func read;

	/* Write function for this cartridge. */
	write16_device_func write;

	/* Read function for this cartridge, TI-99/8. */
	read8_device_func read8;

	/* Write function for this cartridge, TI-99/8. */
	write8_device_func write8;

	/* Read function for this cartridge, CRU. */
	read8_device_func cruread;

	/* Write function for this cartridge, CRU. */
	write8_device_func cruwrite;

	/* Link up to the cartridge structure which contains this pcb. */
	cartridge_t *cartridge;

	/* Function to assemble this cartridge. */
	assmfct	*assemble;

	/* Function to disassemble this cartridge. */
	assmfct	*disassemble;
};
typedef struct _ti99_pcb_t ti99_pcb_t;

static int in_legacy_mode(running_device *device)
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;
	if ((cartslots->legacy_slots>0) &&  (cartslots->multi_slots==0))
		return TRUE;
	return FALSE;
}

/*
    Activates a slot in the multi-cartridge extender.
    Setting the slot is done by accessing the GROM ports using a
    specific address:

    slot n:  read data (read address, write data, write address)

    slot 0:  0x9800 (0x9802, 0x9c00, 0x9c02)   : cartridge1
    slot 1:  0x9804 (0x9806, 0x9c04, 0x9c06)   : cartridge2
    ...
    slot 15: 0x983c (0x983e, 0x9c3c, 0x9c3e)   : cartridge16

    The following addresses are theoretically available, but the
    built-in OS does not use them; i.e. cartridges will not be
    included in the selection list, and their features will not be
    found by lookup, but they could be accessed directly by user
    programs.
    slot 16: 0x9840 (0x9842, 0x9c40, 0x9c42)
    ...
    slot 255:  0x9bfc (0x9bfe, 0x9ffc, 0x9ffe)

    Setting the GROM base should select one cartridge, but the ROMs in the
    CPU space must also be switched. As there is no known special mechanism
    we assume that by switching the GROM base, the ROM is automatically
    switched.

    Caution: This means that cartridges which do not have at least one
    GROM cannot be switched with this mechanism.

    We assume that the slot number is already calculated in the caller:
    slotnumber>=0 && slotnumber<=255

    NOTE: The OS will stop searching when it finds slots 1 and 2 empty.
    Interestingly, cartridge subroutines are found nevertheless, even when
    the cartridge is plugged into a higher slot.
*/
void ti99_cartridge_slot_set(running_device *cartsys, int slotnumber)
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;
	assert(slotnumber>=0 && slotnumber<=255);
//  if (cartslots->active_slot != slotnumber) printf("Setting cartslot to %d\n", slotnumber);
	if (cartslots->fixed_slot==AUTO)
		cartslots->active_slot = slotnumber;
	else
		cartslots->active_slot = cartslots->fixed_slot;
}

/*
    Allows to manually lock the cartslot system to a specific slot.
    Called from the machine driver, taking the dipswitch setting.
    We take slot numbers from 0 (automatic mode) to the maximum.
*/
void ti99_lock_cartridge_slot(running_device *cartsys, int slotnumber)
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;
	assert(slotnumber>=0 && slotnumber<=255);
	cartslots->fixed_slot = slotnumber-1; /* auto = -1 */
}

static int slot_is_empty(running_device *cartsys, int slotnumber)
{
	cartridge_t *cart;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	cart = &cartslots->cartridge[slotnumber];

	if ((cart->rom_size==0) && (cart->ram_size==0) && (cart->grom_size==0))
		return TRUE;
	return FALSE;
}

static void clear_slot(running_device *cartsys, int slotnumber)
{
	cartridge_t *cart;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	cart = &cartslots->cartridge[slotnumber];

	cart->rom_size = cart->ram_size = cart->grom_size = 0;
	cart->rom_ptr = NULL;
	cart->rom2_ptr = NULL;
	cart->ram_ptr = NULL;
	cart->grom_ptr = NULL;
}

/*
    GROM access. Accesses the GROMs in the cartridge. GROM memory is
    accessed serially via a set of ports. The GRMWA port (bank 0: 0x9c02)
    takes two consecutive byte writes which represent the high and low byte
    of the memory address in the GROM space. Once set, bytes can be read
    by repeated read accesses on the GRMRD port (bank 0: 0x9800). The
    address pointer advances on each read. The pointer wraps at the end
    of each GROM chip memory space, i.e. every 8KiB.
    This function takes the current GROM address minus 0x6000 which is the
    lowest address for cartridge GROMs. Lower addresses access GROMs in the
    console only, which appear in all banks.
    Note that the GROM access mechanism is identical for all cartridge
    types.
*/
UINT8 ti99_cartridge_grom_read(running_device *device, int cart_offset)
{
	UINT8 value;
	int slot;
	cartridge_t *cartridge;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

	/* Handle the legacy mode. */
	if (in_legacy_mode(device))
		return cartridge_grom_read_legacy(device, cart_offset);

	slot = cartslots->active_slot;
	/*
        This fixes a minor issue: The selection mechanism of the console
        concludes that there are multiple cartridges plugged in when it
        checks the different GROM bases and finds different data. Empty
        slots should return 0 at all addresses, but when we have only
        one cartridge, this entails that the second cartridge is
        different (all zeros), and so we get a meaningless selection
        option between cartridge 1 and 1 and 1 ...

        So if next_free_slot==1, we have one cartridge in slot 0.
        In that case we trick the OS to believe that the addressed
        cartridge appears at all locations which causes it to assume a
        standard single cartslot.
    */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		value = 0;
	else
	{
		cartridge = &cartslots->cartridge[slot];

		if ((cartridge->grom_size>0) && (cartridge->pcb!=NULL))
		{
			/* This is the reason why we need the image size: Read
               access beyond the image size must be detected. */
			if (cart_offset > cartridge->grom_size)
				return 0;

			value = (UINT8)cartridge->grom_buffer;
			/* read ahead */
			cartridge->grom_buffer = cartridge->grom_ptr[cart_offset];
		}
		else
		{
			/* We assume that empty sockets return 0 */
			value = 0;
			logerror("Empty socket at address G(%d)>%04x\n", slot, 0x6000 + cart_offset);
		}
	}
//  printf("grom access(port = %x, address = %04x) = %02x\n", slot,  0x6000 + cart_offset*2, value);
	return value;
}

/*
    Find the index of the cartridge name. We assume the format
    <name><number>, i.e. the number is the longest string from the right
    which can be interpreted as a number.
*/
static int get_index_from_tagname(running_device *image)
{
	const char *tag = image->tag;
	int maxlen = strlen(tag);
	int i;
	for (i=maxlen-1; i >=0; i--)
		if (tag[i] < 48 || tag[i] > 57) break;

	return atoi(tag+i+1);
}

/*
    Common routine to assemble cartridges from resources.
*/
static cartridge_t *assemble_common(running_device *cartslot)
{
	/* Pointer to the cartridge structure. */
	cartridge_t *cartridge;
	running_device *cartsys = cartslot->owner;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	void *socketcont;
	int reslength;
	int i;

	int slotnumber = get_index_from_tagname(cartslot)-1;
	assert(slotnumber>=0 && slotnumber<NUMBER_OF_CARTRIDGE_SLOTS);

	/* There is a cartridge in this slot, check the maximum slot number. */
	if (cartslots->next_free_slot <= slotnumber)
	{
		cartslots->next_free_slot = slotnumber+1;
	}
	cartridge = &cartslots->cartridge[slotnumber];

	/* Get the socket which is retrieved by the multicart instance in
       the state of the cartslot instance. */
	socketcont = cartslot_get_socket(cartslot, "grom_socket");
	reslength = cartslot_get_resource_length(cartslot, "grom_socket");
	if (socketcont != NULL)
	{
		cartridge->grom_ptr = (UINT8 *)socketcont;
		cartridge->grom_size = reslength;
	}

	socketcont = cartslot_get_socket(cartslot, "rom_socket");
	reslength = cartslot_get_resource_length(cartslot, "rom_socket");
	if (socketcont != NULL)
	{
		cartridge->rom_ptr = socketcont;
		cartridge->rom_size = reslength;
		if (!ti99_is_99_8())
		{
			/* Big-endianize it for 16 bit access. */
			UINT16 *cont16b = (UINT16 *)socketcont;
			for (i = 0; i < cartridge->rom_size/2; i++)
				cont16b[i] = BIG_ENDIANIZE_INT16(cont16b[i]);
		}
	}

	socketcont = cartslot_get_socket(cartslot, "rom2_socket");
//  reslength = cartslot_get_resource_length(cartslot, "rom2_socket");  /* cannot differ from rom_socket */
	if (socketcont != NULL)
	{
		cartridge->rom2_ptr = (UINT16 *)socketcont;
		assert(cartridge->rom2_ptr != NULL);
		if (!ti99_is_99_8())
		{
			/* Big-endianize it for 16 bit access. */
			UINT16 *cont16b = (UINT16 *)socketcont;
			for (i = 0; i < cartridge->rom_size/2; i++)
				cont16b[i] = BIG_ENDIANIZE_INT16(cont16b[i]);
		}
	}

	socketcont = cartslot_get_socket(cartslot, "ram_socket");
	reslength = cartslot_get_resource_length(cartslot, "ram_socket");
	if (socketcont != NULL)
	{
		cartridge->ram_ptr = socketcont;
		cartridge->ram_size = reslength;
		if (!ti99_is_99_8())
		{
			/* Big-endianize it for 16 bit access. */
			UINT16 *cont16b = (UINT16 *)socketcont;
			for (i = 0; i < cartridge->ram_size/2; i++)
				cont16b[i] = BIG_ENDIANIZE_INT16(cont16b[i]);
		}
	}
	return cartridge;
}

static void set_pointers(running_device *pcb, int index)
{
	running_device *cartsys = pcb->owner->owner;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;
	ti99_pcb_t *pcb_def = (ti99_pcb_t *)pcb->token;

	pcb_def->read = (read16_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_6000_R);
	pcb_def->write = (write16_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_6000_W);
	pcb_def->read8 = (read8_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_6000_R8);
	pcb_def->write8 = (write8_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_6000_W8);
	pcb_def->cruread = (read8_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_CRU_R);
	pcb_def->cruwrite = (write8_device_func) pcb->get_config_fct(TI99CARTINFO_FCT_CRU_W);

	pcb_def->assemble = (assmfct *)pcb->get_config_fct(TI99CART_FCT_ASSM);
	pcb_def->disassemble = (assmfct *)pcb->get_config_fct(TI99CART_FCT_DISASSM);

	pcb_def->cartridge = &cartslots->cartridge[index];
	pcb_def->cartridge->pcb = pcb;
}

/*****************************************************************************
  Cartridge type: None
    This PCB device is just a pseudo device; the legacy mode is handled
    by dedicated functions.
******************************************************************************/
static DEVICE_START(ti99_pcb_none)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_none), tag of device=%s\n", device->tag.cstr());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*****************************************************************************
  Cartridge type: Standard
    Most cartridges are built in this type. Every cartridge may contain
    GROM between 0 and 40 KiB, and ROM with 8 KiB length, no banking.
******************************************************************************/

static DEVICE_START(ti99_pcb_std)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_std), tag of device=%s\n", device->tag.cstr());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ16_DEVICE_HANDLER( read_cart_std )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if (cartridge->rom_ptr==NULL)
	{
//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		value = ((UINT16 *)cartridge->rom_ptr)[offset];
//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", 0, 0x6000+offset*2, value);
	return value;
}

static WRITE16_DEVICE_HANDLER( write_cart_std )
{
	logerror("Write access to cartridge ROM at address %04x ignored", 0x6000 + 2*offset);
}

/*
    TI-99/8 support:
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
    For comments see the READ16 handler.
*/
static READ8_DEVICE_HANDLER( read_cart_std8 )
{
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if (cartridge->rom_ptr==NULL)
		value = 0;
	else
		value = ((UINT8 *)cartridge->rom_ptr)[offset];
	return value;
}

/*
    TI-99/8 support:
    Write handler. No writes are allowed to standard cartridges.
*/
static WRITE8_DEVICE_HANDLER( write_cart_std8 )
{
	logerror("Write access to cartridge ROM at address %04x ignored", 0x6000 + offset);
}

/*
    The standard cartridge assemble routine. We just call the common
    function here.
*/
static int assemble_std(running_device *image)
{
	cartridge_t *cart;
//  printf("assemble_std, %s\n", image->tag);
	cart = assemble_common(image);

	return INIT_PASS;
}

/*
    Removes pointers and restores the state before plugging in the
    cartridge.
    The pointer to the location after the last cartridge is adjusted.
    As it seems, we can use the same function for the disassembling of all
    cartridge types.
*/
static int disassemble_std(running_device *image)
{
	int slotnumber;
	int i;
	cartridge_t *cart;
	running_device *cartsys = image->owner;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	slotnumber = get_index_from_tagname(image)-1;
//  printf("Disassemble cartridge %d\n", slotnumber);

	/* Search the highest remaining cartridge. */
	cartslots->next_free_slot = 0;
	for (i=NUMBER_OF_CARTRIDGE_SLOTS-1; i >= 0; i--)
	{
		if (i != slotnumber)
		{
			if (!slot_is_empty(cartsys, i))
			{
				cartslots->next_free_slot = i+1;
//              printf("Setting new next_free_slot to %d\n", cartslots->next_free_slot);
				break;
			}
		}
	}

	/* Do we have RAM? If so, swap the bytes (undo the BIG_ENDIANIZE) */
	cart = &cartslots->cartridge[slotnumber];
	if (!ti99_is_99_8())
	{
		for (i = 0; i < cart->ram_size/2; i++)
			((UINT16 *)cart->ram_ptr)[i] = BIG_ENDIANIZE_INT16(((UINT16 *)cart->ram_ptr)[i]);
	}

	clear_slot(cartsys, slotnumber);

	return INIT_PASS;
}

/*****************************************************************************
  Cartridge type: Paged (Extended Basic)
    This cartridge consists of GROM memory and 2 pages of standard ROM.
    The page is set by writing any value to a location in
    the address area, where an even word offset sets the page to 0 and an
    odd word offset sets the page to 1 (e.g. 6000 = bank 0, and
    6002 = bank 1).
******************************************************************************/

static DEVICE_START(ti99_pcb_paged)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_paged), tag of device=%s\n", device->tag.cstr());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ16_DEVICE_HANDLER( read_cart_paged )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	UINT16 *rombank;

	if (cartridge->rom_page==0)
		rombank = (UINT16*)cartridge->rom_ptr;
	else
		rombank = (UINT16*)cartridge->rom2_ptr;

	if (rombank==NULL)
	{
//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		/* TODO: Check for consistency with the GROM memory handling. */
		value = 0;
	}
	else
	{
		value = rombank[offset];
//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", rompage, 0x6000+offset*2, value);
	return value;
}


/*
    Handle paging. Extended Basic switches between ROM banks by
    using the value of the LSB of the address where any value
    is written to.
*/
static WRITE16_DEVICE_HANDLER( write_cart_paged )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	cartridge->rom_page = offset & 1;
//  printf("setting rompage to %d\n", cartridge->rom_page);
}

/*
    TI-99/8 support:
    Read handler for the CPU address space of the cartridge.
    Images for this area are found in the rom_sockets.
*/
static READ8_DEVICE_HANDLER( read_cart_paged8 )
{
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	UINT8 *rombank;

	if (cartridge->rom_page==0)
		rombank = (UINT8 *)cartridge->rom_ptr;
	else
		rombank = (UINT8 *)cartridge->rom2_ptr;

	if (rombank==NULL)
		value = 0;
	else
		value = rombank[offset];

	return value;
}


/*
    TI-99/8 support:
    Handle paging. Extended Basic switches between ROM banks by
    using the value of the LSB of the address where any value
    is written to. This is the TI-99/8 support.
    Note that we need to check whether offset/2 is even or odd.

    FIXME: Extended Basic crashes with TI-99/8 when changing the rom page.
    This may be an incompatibility of Extended Basic and TI-99/8.
*/
static WRITE8_DEVICE_HANDLER( write_cart_paged8 )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
//  printf("offset = %04x\n", offset);
	cartridge->rom_page = (offset>>1) & 1;
}

/*
    We require paged modules to have at least those two rom banks.
*/
static int assemble_paged(running_device *image)
{
	cartridge_t *cart;

//  printf("assemble_paged, %s\n", image->tag);
	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for paged cartridge");
		return INIT_FAIL;
	}
	if (cart->rom2_ptr==NULL)
	{
		logerror("Missing second ROM for paged cartridge");
		return INIT_FAIL;
	}

	return INIT_PASS;
}

/*****************************************************************************
  Cartridge type: Mini Memory
    GROM: 6 KiB (occupies G>6000 to G>7800)
    ROM: 4 KiB (romfile is actually 8 K long, half with zeros, 0x6000-0x6fff)
    persistent RAM: 4 KiB (0x7000-0x7fff)
******************************************************************************/

static DEVICE_START(ti99_pcb_minimem)
{
//  printf("TI99 pcb minimem start\n");
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

static READ16_DEVICE_HANDLER( read_cart_minimem )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if (offset < 0x800)
	{
		if (cartridge->rom_ptr==NULL)
		{
			//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
			//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
			value = 0;
		}
		else
		{
			value = ((UINT16 *)cartridge->rom_ptr)[offset];
			//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
		}
		//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", rompage, 0x6000+offset*2, value);
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
			//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
			value = 0;
		}
		else
		{
			value = ((UINT16 *)cartridge->ram_ptr)[offset-0x800];
			//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
		}
		//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", rompage, 0x6000+offset*2, value);
	}
	return value;
}

/*
    Mini Memory cartridge write operation. RAM is located at offset + 4 KiB
*/
static WRITE16_DEVICE_HANDLER( write_cart_minimem )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	UINT16 *pos;

	if (offset < 0x800)
	{
		logerror("Write access to cartridge ROM at address %04x ignored", 0x6000 + 2*offset);
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			logerror("No cartridge RAM at address %04x", 0x6000 + 2*offset);

			//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
			//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
			/* TODO: Check for consistency with the GROM memory handling. */
		}
		else
		{
			pos = ((UINT16 *)cartridge->ram_ptr) + offset - 0x800;
			COMBINE_DATA(pos);
		}
	}
}

/*
    Read handler for MiniMemory in TI-99/8.
*/
static READ8_DEVICE_HANDLER( read_cart_minimem8 )
{
	/* device is pcb, owner is cartslot */
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if (offset < 0x1000)
	{
		if (cartridge->rom_ptr==NULL)
		{
			value = 0;
		}
		else
		{
			value = ((UINT8 *)cartridge->rom_ptr)[offset];
		}
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			value = 0;
		}
		else
		{
			value = ((UINT8 *)cartridge->ram_ptr)[offset-0x1000];
		}
	}
	return value;
}

/*
    Mini Memory cartridge write operation. RAM is located at offset + 4 KiB.
    This is the TI-99/8 support.
*/
static WRITE8_DEVICE_HANDLER( write_cart_minimem8 )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if (offset < 0x1000)
	{
		logerror("Write access to cartridge ROM at address %04x ignored", 0x6000 + offset);
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			logerror("No cartridge RAM at address %04x", 0x6000 + offset);

			//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
			//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
			/* TODO: Check for consistency with the GROM memory handling. */
		}
		else
		{
			((UINT8 *)cartridge->ram_ptr)[offset-0x1000] = data;
		}
	}
}


static int assemble_minimem(running_device *image)
{
	cartridge_t *cart;

//  printf("assemble_minimem, %s\n", image->tag);
	cart = assemble_common(image);
	if (cart->grom_size==0)
	{
		logerror("Missing GROM for Mini Memory");
		return INIT_FAIL;
	}
	if (cart->rom_size==0)
	{
		logerror("Missing ROM for Mini Memory");
		return INIT_FAIL;
	}
	if (cart->ram_size==0)
	{
		logerror("Missing RAM for Mini Memory");
		return INIT_FAIL;
	}

	return INIT_PASS;
}


/*****************************************************************************
  Cartridge type: SuperSpace II
    SuperSpace cartridge type. SuperSpace is intended as a user-definable
    blank cartridge containing buffered RAM
    It has an Editor/Assembler GROM which helps the user to load
    the user program into the cartridge. If the user program has a suitable
    header, the console recognizes the cartridge as runnable, and
    assigns a number in the selection screen.
    Switching the RAM banks in this cartridge is achieved by setting
    CRU bits (the system serial interface).

    GROM: Editor/Assembler GROM
    ROM: none
    persistent RAM: 32 KiB (0x6000-0x7fff, 4 banks)
    Switching the bank is done via CRU write
******************************************************************************/

static DEVICE_START(ti99_pcb_super)
{
//  printf("TI99 pcb super start\n");
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    The CRU read handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the SuperSpace cartridge.
*/
static READ8_DEVICE_HANDLER( read_cart_cru )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	/* offset is the bit number. The CRU base address is already divided
           by 2. */
        int reply = 0;

	/* ram_page contains the bank number. We have a maximum of
    4 banks; the Super Space II manual says:

    Banks are selected by writing a bit pattern to CRU address >0800:

    Bank #   Value
    0        >02  = 0000 0010
    1        >08  = 0000 1000
    2        >20  = 0010 0000
    3        >80  = 1000 0000

    With the bank number (0, 1, 2, or 3) in R0:

    BNKSW   LI    R12,>0800   Set CRU address
        LI    R1,2        Load Shift Bit
        SLA   R0,1        Align Bank Number
        JEQ   BNKS1       Skip shift if Bank 0
        SLA   R1,0        Align Shift Bit
    BNKS1   LDCR  R1,0        Switch Banks
        SRL   R0,1        Restore Bank Number (optional)
        RT
    */
	if ((offset & 1) == 0 || offset > 7)
		reply = 0;

	/* CRU addresses are only 1 bit wide. Bytes are transferred from LSB
    to MSB. That is, 8 bit are eight consecutive addresses. */
	reply = (cartridge->ram_page == (offset-1)/2);

	return reply;
}

static WRITE8_DEVICE_HANDLER( write_cart_cru )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	// data is bit
	// offset is address
        if (offset < 8)
	{
		if (data != 0)
			cartridge->ram_page = (offset-1)/2;
	}
}

/*
    SuperSpace RAM read operation. RAM is located at 0x6000 to 0x7fff
    as 4 banks of 8 KiB each
*/
static READ16_DEVICE_HANDLER( read_cart_super )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		boffset = cartridge->ram_page * 0x1000 + offset;
		value = ((UINT16 *)cartridge->ram_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
	//  printf("accessed cartridge ram[%02x:%04x] = %04x\n", cartridge->ram_page, 0x6000+offset*2, value);
	return value;
}

/*
    SuperSpace RAM write operation. RAM is located at 0x6000 to 0x7fff
    as 4 banks of 8 KiB each
*/
static WRITE16_DEVICE_HANDLER( write_cart_super )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	UINT16 *pos;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		logerror("No cartridge RAM at address %04x", 0x6000 + 2*offset);

		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
	}
	else
	{
		boffset = cartridge->ram_page * 0x1000 + offset;
		pos = ((UINT16 *)cartridge->ram_ptr) + boffset;
		COMBINE_DATA(pos);
	}
}

/*
    SuperSpace RAM read operation. RAM is located at 0x6000 to 0x7fff
    as 4 banks of 8 KiB each. This is the TI-99/8 support.
*/
static READ8_DEVICE_HANDLER( read_cart_super8 )
{
	/* device is pcb, owner is cartslot */
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		boffset = cartridge->ram_page * 0x2000 + offset;
		value = ((UINT8 *)cartridge->ram_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
	//  printf("accessed cartridge ram[%02x:%04x] = %04x\n", cartridge->ram_page, 0x6000+offset*2, value);
	return value;
}

/*
    SuperSpace RAM write operation. RAM is located at 0x6000 to 0x7fff
    as 4 banks of 8 KiB each. This is the TI-99/8 support.
*/
static WRITE8_DEVICE_HANDLER( write_cart_super8 )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		logerror("No cartridge RAM at address %04x", 0x6000 + offset);

		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
	}
	else
	{
		boffset = cartridge->ram_page * 0x2000 + offset;
		((UINT8 *)cartridge->ram_ptr)[boffset] = data;
	}
}


static int assemble_super(running_device *image)
{
	cartridge_t *cart;
//  printf("assemble_super, %s\n", image->tag);

	cart = assemble_common(image);
	if (cart->ram_size==0)
	{
		logerror("Missing RAM for SuperSpace");
		return INIT_FAIL;
	}
	return INIT_PASS;
}

/*****************************************************************************
  Cartridge type: MBX
    GROM: up to 40 KiB
    ROM: up to 16 KiB (in up to 2 banks of 8KiB each)
    RAM: 1022 B (0x6c00-0x6ffd, overrides ROM in that area)
    ROM mapper: 6ffe
******************************************************************************/

static DEVICE_START(ti99_pcb_mbx)
{
//  printf("TI99 pcb mbx start\n");
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

static READ16_DEVICE_HANDLER( read_cart_mbx )
{
	/* device is pcb, owner is cartslot */
	UINT16 value = 0;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset >= 0x0600) && (offset <= 0x07fe))
	{
		/* This is the RAM area which overrides any ROM. There is no
           known banking behavior for the RAM, so we must assume that
           there is only one bank. */
		if (cartridge->ram_ptr != NULL)
			value = ((UINT16 *)cartridge->ram_ptr)[offset - 0x0600];
	}
	else
	{
		if (cartridge->rom_ptr != NULL)
			value = ((UINT16 *)cartridge->rom_ptr)[offset + cartridge->rom_page * 0x1000];
	}
	return value;
}

/*
    MBX write operation.
*/
static WRITE16_DEVICE_HANDLER( write_cart_mbx )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	UINT16 *pos;

	if ((offset >= 0x0600) && (offset <= 0x07fe))
	{
		if (cartridge->ram_ptr == NULL)
			return;
		pos = ((UINT16 *)cartridge->ram_ptr) + offset - 0x0600;
		COMBINE_DATA(pos);
	}
	else if ((offset == 0x07ff) && ACCESSING_BITS_8_15)
	{
		if (cartridge->rom_ptr==NULL)
			return;
		cartridge->rom_page = ((data >> 8) & 1);
	}
}

/*
    TI-99/8 support
*/
static READ8_DEVICE_HANDLER( read_cart_mbx8 )
{
	/* device is pcb, owner is cartslot */
	UINT8 value = 0;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset >= 0x0c00) && (offset <= 0x0ffd))
	{
		/* This is the RAM area which overrides any ROM. There is no
           known banking behavior for the RAM, so we must assume that
           there is only one bank. */
		if (cartridge->ram_ptr != NULL)
			value = ((UINT8 *)cartridge->ram_ptr)[offset - 0x0c00];
	}
	else
	{
		if (cartridge->rom_ptr != NULL)
			value = ((UINT8 *)cartridge->rom_ptr)[offset + cartridge->rom_page * 0x2000];
	}
	return value;
}

/*
    MBX write operation (TI-99/8)
*/
static WRITE8_DEVICE_HANDLER( write_cart_mbx8 )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset >= 0x0c00) && (offset <= 0x0ffd))
	{
		if (cartridge->ram_ptr == NULL)
			return;
		((UINT8 *)cartridge->ram_ptr)[offset - 0x0c00] = data;
	}
	else if (offset == 0x0ffe || offset == 0x0fff)
	{
		if (cartridge->rom_ptr==NULL)
			return;
		cartridge->rom_page = (data & 1); /* TODO: Check */
	}
}


static int assemble_mbx(running_device *image)
{
	cartridge_t *cart;

//  printf("assemble_mbx, %s\n", image->tag);
	cart = assemble_common(image);
	return INIT_PASS;
}

/*****************************************************************************
  Cartridge type: paged379i
    This cartridge consists of one 16 KiB, 32 KiB, or 64 KiB EEPROM which is
    organised in 2, 4, or 8 pages of 8 KiB each. We assume there is only one
    dump file of the respective size.
    The pages are selected by writing a value to some memory locations. Due to
    using the inverted outputs of the LS379 latch, setting the inputs of the
    latch to all 0 selects the highest bank, while setting to all 1 selects the
    lowest.There are some cartridges (16 KiB) which are using this scheme, and
    there are new hardware developments mainly relying on this scheme.

    Writing to       selects page (16K/32K/64K)
    >6000            1 / 3 / 7
    >6002            0 / 2 / 6
    >6004            1 / 1 / 5
    >6006            0 / 0 / 4
    >6008            1 / 3 / 3
    >600A            0 / 2 / 2
    >600C            1 / 1 / 1
    >600E            0 / 0 / 0

******************************************************************************/

static DEVICE_START(ti99_pcb_paged379i)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_paged379i), tag of device=%s\n", device->tag.cstr());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ16_DEVICE_HANDLER( read_cart_paged379i )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->rom_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		// rom_page is the stored latch value
		// 16bit access -> multiply by 0x1000
		boffset = cartridge->rom_page * 0x1000 + offset;
		value = ((UINT16 *)cartridge->rom_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
//    printf("accessed cartridge rom[%02x:%04x] = %04x\n", cartridge->rom_page, 0x6000+offset*2, value);
	return value;
}

static void set_paged379i_bank(cartridge_t *cartridge, int rompage)
{
	int mask = 0;
	if (cartridge->rom_size > 16384)
	{
		if (cartridge->rom_size > 32768)
			mask = 7;
		else
			mask = 3;
	}
	else
		mask = 1;

	cartridge->rom_page = rompage & mask;
//  printf("setting rompage to %d\n", cartridge->rom_page);
}

/*
    Handle paging. We use a LS379 latch for storing the page number. On this
    PCB, the inverted output of the latch is used, so the order of pages
    is reversed. (No problem as long as the memory dump is kept exactly
    in the way it is stored in the EPROM.)
    The latch can store a value of 4 bits. We adjust the number of
    significant bits by the size of the memory dump (16K, 32K, 64K).
*/
static WRITE16_DEVICE_HANDLER( write_cart_paged379i )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
/*
    >6000            1 / 3 / 7
    >6002            0 / 2 / 6
    >6004            1 / 1 / 5
    >6006            0 / 0 / 4
    >6008            1 / 3 / 3
    >600A            0 / 2 / 2
    >600C            1 / 1 / 1
    >600E            0 / 0 / 0

    Bits: 011x xxxx xxxx bbbx
    x = don't care, bbb = bank
    16bit access -> drop rightmost bit
*/
	set_paged379i_bank(cartridge, 7 - (offset & 7));
}

/*
    TI-99/8 support:
    Read handler for the CPU address space of the cartridge.
    Images for this area are found in the rom_sockets.
*/
static READ8_DEVICE_HANDLER( read_cart_paged379i8 )
{
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->rom_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		// rom_page is the stored latch value
		// 8bit access -> multiply by 0x2000
		boffset = cartridge->rom_page * 0x2000 + offset;
		value = ((UINT8 *)cartridge->rom_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
	//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", cartridge->rom_page, 0x6000+offset*2, value);
	return value;

}

/*
    TI-99/8 support: Handle paging.
*/
static WRITE8_DEVICE_HANDLER( write_cart_paged379i8 )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
/*
    >6000            1 / 3 / 7
    >6002            0 / 2 / 6
    >6004            1 / 1 / 5
    >6006            0 / 0 / 4
    >6008            1 / 3 / 3
    >600A            0 / 2 / 2
    >600C            1 / 1 / 1
    >600E            0 / 0 / 0

    Bits: 011x xxxx xxxx bbbx
    x = don't care, bbb = bank
    16bit access -> drop rightmost bit
*/
	set_paged379i_bank(cartridge, 7 - ((offset>>1) & 7));
}

/*
    Paged379i modules have one EPROM dump.
*/
static int assemble_paged379i(running_device *image)
{
	cartridge_t *cart;

//  printf("assemble_paged379i, %s\n", image->tag);
	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for paged cartridge");
		return INIT_FAIL;
	}
	set_paged379i_bank(cart, 15);
	return INIT_PASS;
}

/*****************************************************************************
  Cartridge type: pagedcru
    This cartridge consists of one 16 KiB, 32 KiB, or 64 KiB EEPROM which is
    organised in 2, 4, or 8 pages of 8 KiB each. We assume there is only one
    dump file of the respective size.
    The pages are selected by writing a value to the CRU. This scheme is
    similar to the one used for the SuperSpace cartridge, with the exception
    that we are using ROM only, and we can have up to 8 pages.

      Bank     Value written to CRU>0800
    0      >0002  = 0000 0000 0000 0010
    1      >0008  = 0000 0000 0000 1000
    2      >0020  = 0000 0000 0010 0000
    3      >0080  = 0000 0000 1000 0000
    4      >0200  = 0000 0010 0000 0000
    5      >0800  = 0000 1000 0000 0000
    6      >2000  = 0010 0000 0000 0000
    7      >8000  = 1000 0000 0000 0000

******************************************************************************/

static DEVICE_START(ti99_pcb_pagedcru)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_paged379i), tag of device=%s\n", device->tag.cstr());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ16_DEVICE_HANDLER( read_cart_pagedcru )
{
	/* device is pcb, owner is cartslot */
	UINT16 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->rom_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		// rom_page is the stored latch value
		// 16bit access -> multiply by 0x1000
		boffset = cartridge->rom_page * 0x1000 + offset;
		value = ((UINT16 *)cartridge->rom_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
//    printf("accessed cartridge rom[%02x:%04x] = %04x\n", cartridge->rom_page, 0x6000+offset*2, value);
	return value;
}

/*
    This type of cartridge does not support writing to the EPROM address
    space.
*/
static WRITE16_DEVICE_HANDLER( write_cart_pagedcru )
{
	return;
}

/*
    TI-99/8 support:
    Read handler for the CPU address space of the cartridge.
    Images for this area are found in the rom_sockets.
*/
static READ8_DEVICE_HANDLER( read_cart_pagedcru8 )
{
	UINT8 value;
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->rom_ptr==NULL)
	{
		//      printf("No cartridge ROM in rompage=%d of bank %d\n", rompage, cartslots->active_slot);
		//      printf("Cartridge address = %lx\n", (long)pcb->cartridge);
		value = 0;
	}
	else
	{
		// rom_page is the stored latch value
		// 8bit access -> multiply by 0x2000
		boffset = cartridge->rom_page * 0x2000 + offset;
		value = ((UINT8 *)cartridge->rom_ptr)[boffset];
		//      printf("stdcart/read: %4x = %4x\n", 0x6000 + 2*offset, value);
	}
	//  printf("accessed cartridge rom[%02x:%04x] = %04x\n", cartridge->rom_page, 0x6000+offset*2, value);
	return value;

}

/*
    TI-99/8 support: This type of cartridge does not support writing to the
    EPROM address space.
*/
static WRITE8_DEVICE_HANDLER( write_cart_pagedcru8 )
{
	return;
}

/*
    The CRU read handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the SuperSpace cartridge.
    For documentation see the corresponding cru function above. Note we are
    using rom_page here.
*/
static READ8_DEVICE_HANDLER( read_cart_cru_paged )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
        int reply = 0;

	if ((offset & 1) == 0 || offset > 15)
		reply = 0;

	reply = (cartridge->rom_page == (offset-1)/2);

	return reply;
}

static WRITE8_DEVICE_HANDLER( write_cart_cru_paged )
{
	ti99_pcb_t *pcb = (ti99_pcb_t *)device->token;
	cartridge_t *cartridge = pcb->cartridge;
	// data is bit
	// offset is address
	/* Note that CRU >0F00 also gets in here ... should check
    whether it is intended to go somewhere else */
        if (offset < 16)
	{
		if (data != 0)
		{
			cartridge->rom_page = (offset-1)/2;
			printf("Setting bit %4x of CRU base >0800\n", offset);
		}
	}
//  printf("setting rompage to %d\n", cartridge->rom_page);
}

/*
    Pagedcru modules have one EPROM dump.
*/
static int assemble_pagedcru(running_device *image)
{
	cartridge_t *cart;

//  printf("assemble_paged379i, %s\n", image->tag);
	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for pagedcru cartridge");
		return INIT_FAIL;
	}
	cart->rom_page = 0;
	return INIT_PASS;
}

/*****************************************************************************
  Device metadata
******************************************************************************/

static DEVICE_GET_INFO(ti99_cart_common)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:
			info->i = sizeof(ti99_pcb_t);
			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:
			info->i = 0;
			break;
		case DEVINFO_INT_CLASS:
			info->i = DEVICE_CLASS_PERIPHERAL;
			break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_std);
			break;
		case DEVINFO_FCT_STOP:
			/* Nothing */
			break;
		case DEVINFO_FCT_RESET:
			/* Nothing */
			break;
		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_std; break;

		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_std; break;

		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_std8; break;

		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_std8; break;

		case TI99CARTINFO_FCT_CRU_R:
			/* Nothing */
			break;

		case TI99CARTINFO_FCT_CRU_W:
			/* Nothing */
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_std;
			break;

		case TI99CART_FCT_DISASSM:
			info->f = (genf *) disassemble_std;
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 standard cartridge pcb");
			break;
		case DEVINFO_STR_FAMILY:
			strcpy(info->s, "TI99 cartridge pcb");
			break;
		case DEVINFO_STR_VERSION:
			strcpy(info->s, "1.0");
			break;
		case DEVINFO_STR_SOURCE_FILE:
			strcpy(info->s, __FILE__);
			break;
		case DEVINFO_STR_CREDITS:
			/* Nothing */
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_none)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_none);
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 empty cartridge");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_std)
{
	DEVICE_GET_INFO_CALL(ti99_cart_common);
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_paged)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_paged);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_paged; break;

		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_paged;
			break;

		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_paged;
			break;

		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_paged8;
			break;

		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_paged8;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 paged cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_minimem)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_minimem);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_minimem;
			break;
		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_minimem;
			break;
		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_minimem;
			break;
		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_minimem8;
			break;
		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_minimem8;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 MiniMemory cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_super)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_super);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_super;
			break;
		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_super;
			break;
		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_super;
			break;
		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_super8;
			break;
		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_super8;
			break;
		case TI99CARTINFO_FCT_CRU_R:
			info->f = (genf *) read_cart_cru;
			break;
		case TI99CARTINFO_FCT_CRU_W:
			info->f = (genf *) write_cart_cru;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 SuperSpace cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_mbx)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_mbx);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_mbx;
			break;
		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_mbx;
			break;
		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_mbx;
			break;
		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_mbx8;
			break;
		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_mbx8;
			break;


		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 MBX cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_paged379i)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_paged379i);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_paged379i; break;

		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_paged379i;
			break;

		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_paged379i;
			break;

		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_paged379i8;
			break;

		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_paged379i8;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 paged379i cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

static DEVICE_GET_INFO(ti99_cartridge_pcb_pagedcru)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_pagedcru);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_pagedcru;
			break;

		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_pagedcru;
			break;

		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_pagedcru;
			break;

		case TI99CARTINFO_FCT_6000_R8:
			info->f = (genf *) read_cart_pagedcru8;
			break;

		case TI99CARTINFO_FCT_6000_W8:
			info->f = (genf *) write_cart_pagedcru8;
			break;

		case TI99CARTINFO_FCT_CRU_R:
			info->f = (genf *) read_cart_cru_paged;
			break;
		case TI99CARTINFO_FCT_CRU_W:
			info->f = (genf *) write_cart_cru_paged;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 pagedcru cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

/*****************************************************************************
  The cartridge handling of the multi-cartridge system.
  Every cartridge contains a PCB device. The memory handlers delegate the calls
  to the respective handlers of the cartridges.
******************************************************************************/
/*
    Initialize a cartridge. Each cartridge contains a PCB device.
*/
static DEVICE_START( ti99_cartridge )
{
	cartslot_t *cart = (cartslot_t *)device->token;
	astring tempstring;

	/* find the PCB device */
	cart->pcb_device = device->subdevice(TAG_PCB);
}

/*
    Load the cartridge image files. Apart from reading, we set pointers
    to the image files so that during runtime we do not need search
    operations.
*/
static DEVICE_IMAGE_LOAD( ti99_cartridge )
{
	running_device *pcbdev = cartslot_get_pcb(image);
	running_device *cartsys = image->owner;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	int result;

	result = load_legacy(image);

	if (result != INIT_FAIL)
		cartslots->legacy_slots++;
	else
	{
		if (pcbdev != NULL)
		{
			/* If we are here, we have a multicart. */
			ti99_pcb_t *pcb = (ti99_pcb_t *)pcbdev->token;
			cartslot_t *cart = (cartslot_t *)image->token;

			/* try opening this as a multicart */
			/* This line requires that cartslot_t be included in cartslot.h,
            otherwise one cannot make use of multicart handling within such a
            custom LOAD function. */
			multicart_open_error me = multicart_open(image_filename(image), image->machine->gamedrv->name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);

			/* Now that we have loaded the image files, let the PCB put them all
            together. This means we put the images in a structure which allows
            for a quick access by the memory handlers. Every PCB defines an
            own assembly method. */
			if (me == MCERR_NONE)
				result = pcb->assemble(image);
			else
				fatalerror("Error loading multicart: %s\n", multicart_error_text(me));

			/* This is for legacy support. If we have no multicart left
            but there are still legacy dumps, we switch to legacy mode. */
			if (result != INIT_FAIL)
				cartslots->multi_slots++;
		}
		else
		{
			fatalerror("Error loading multicart: no pcb found.\n");
		}
	}
	return result;
}

/*
    This is called when the cartridge is unplugged (or the emulator is
    stopped).
*/
static DEVICE_IMAGE_UNLOAD( ti99_cartridge )
{
	running_device *pcbdev;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)image->owner->token;

	if (image->token == NULL)
	{
		/* This means something went wrong during the pcb
        identification (e.g. one of the cartridge files was not
        found). We do not need to (and cannot) unload the cartridge. */
		return;
	}
	pcbdev = cartslot_get_pcb(image);

	if (pcbdev != NULL)
	{
		ti99_pcb_t *pcb = (ti99_pcb_t *)pcbdev->token;
		cartslot_t *cart = (cartslot_t *)image->token;

		//  printf("unload\n");
		if (cart->mc != NULL)
		{
			/* Remove pointers and de-big-endianize RAM contents. */
			pcb->disassemble(image);

			/* Close the multicart; all RAM resources will be
            written to disk */
			multicart_close(cart->mc);
			cart->mc = NULL;

			/* See above: Legacy support. */
			cartslots->multi_slots--;
		}
//      else
//          fatalerror("Lost pointer to multicart in cartridge. Report bug.\n");
	}
	else
	{
		unload_legacy(image);
		cartslots->legacy_slots--;
	}
}

/*****************************************************************************
  The legacy cartridge slot system.

  This is the system which was used for the TI family until MESS 0.130.
  Here, plain ROM dumps were mounted on the slots, e.g. the GROM on slot 1,
  ROM on slot 2. No multiple cartridges were supported.

  This functionality is deprecated.

  In order to allow for a smooth transition, this feature is included in this
  version. It is strongly recommmended to create suitable multicart packs.

  This implementation tolerates both cartridge systems in the following way:
  - If there are only single ROM dumps, the system switched to legacy mode.
  - If there is at least one multicart pack in any slot, the system switches
    to the multicart mode. All plain dumps become invisible; the slots appear
    empty to the system
  - Both modes may change on plugging and unplugging.
******************************************************************************/

static UINT8 cartridge_grom_read_legacy(running_device *cartsys, int cart_offset)
{
	int slot;
	cartridge_t *cartridge;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	UINT8 value;

	/* Get the GROM slot number. */
	slot = cartslots->legacy_slotnumber[SLOTC_GROM];
	if (slot<0)
		/* No GROM available. */
		return 0;

	cartridge = &cartslots->cartridge[slot];

	if (cartridge->grom_size>0)
	{
		/* This is the reason why we need the image size: Read
        access beyond the image size must be detected. */
		if (cart_offset > cartridge->grom_size)
			return 0;

		value = (UINT8)cartridge->grom_buffer;
		/* read ahead */
		cartridge->grom_buffer = cartridge->grom_ptr[cart_offset];
	}
	else
	{
		/* We assume that empty sockets return 0 */
		value = 0;
		logerror("Empty socket at address G(%d)>%04x\n", slot, 0x6000 + cart_offset);
	}

	//  printf("grom access(port = %x, address = %04x) = %02x\n", slot,  0x6000 + cart_offset*2, value);
	return value;
}

static READ16_DEVICE_HANDLER( ti99_cart_r_legacy )
{
	int slotmbx, slotmini, slotebr2, slotrom;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

#if 0
	if (hsgpl_crdena)
		return ti99_hsgpl_rom6_r(space, offset, mem_mask);
#endif

	slotmbx = cartslots->legacy_slotnumber[SLOTC_MBX];

	if ((slotmbx != -1) && (offset >= 0x0600) && (offset <= 0x07fe))
	{
		/* We are in the RAM area of MBX cartridges. */
		return ((UINT16 *)(cartslots->cartridge[slotmbx].ram_ptr))[offset-0x0600];
	}

	slotmini = cartslots->legacy_slotnumber[SLOTC_MINIMEM];
	if ((slotmini != -1) && (offset >= 0x0800) && (offset <= 0x0fff))
	{
		/* We are in the RAM area of the Mini Memory cartridge. */
		return ((UINT16 *)(cartslots->cartridge[slotmini].ram_ptr))[offset-0x0800];
	}

	/* Standard */
	slotrom = cartslots->legacy_slotnumber[SLOTC_CROM];
	if (slotrom < 0)
		return 0;

	slotebr2 = cartslots->legacy_slotnumber[SLOTC_DROM];
	if (slotebr2 < 0)
		return (((UINT16 *)cartslots->cartridge[slotrom].rom_ptr)[offset]);
	else
	{
		/* Extended Basic or other paged cartridge */
		if (cartslots->cartridge[slotrom].rom_page!=0)
			return (((UINT16 *)cartslots->cartridge[slotebr2].rom2_ptr)[offset]);
		else
			return (((UINT16 *)cartslots->cartridge[slotrom].rom_ptr)[offset]);
	}
	/* SuperSpace, paged379i, and pagedcru are not supported in
    legacy mode. */
}

static WRITE16_DEVICE_HANDLER( ti99_cart_w_legacy )
{
	int slotmbx, slotmini, slotebr2, slotrom;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

#if 0
	if (hsgpl_crdena)
	{
		ti99_hsgpl_rom6_w(space, offset, data, mem_mask);
		return;
	}
#endif
	slotmbx = cartslots->legacy_slotnumber[SLOTC_MBX];
	slotrom = cartslots->legacy_slotnumber[SLOTC_CROM];
	slotebr2 = cartslots->legacy_slotnumber[SLOTC_DROM];
	slotmini = cartslots->legacy_slotnumber[SLOTC_MINIMEM];

	if (slotmbx != -1)
	{
		if ((offset >= 0x0600) && (offset <= 0x07fe))
		{
			/* We are in the RAM area of MBX cartridges. */
			/* RAM in 0x6c00-0x6ffd (presumably non-paged) */
			/* mapper at 0x6ffe */
			COMBINE_DATA(((UINT16 *)cartslots->cartridge[slotmbx].ram_ptr) + offset - 0x0600);
		}
		else
		{
			if ((offset == 0x07ff) && ACCESSING_BITS_8_15)
			{
				if (slotebr2 < 0 || slotrom < 0)
					return; /* just ignore if the cartridge is not paged. */
				else
					cartslots->cartridge[slotrom].rom_page = ((data >> 8) & 1);
			}
		}
		return;
	}
	if (slotmini != -1)
	{
		if ((offset >= 0x0800) && (offset <= 0x0fff))
			/* We are in the RAM area of a MiniMemory cartridge. */
			COMBINE_DATA(((UINT16 *)cartslots->cartridge[slotmini].ram_ptr) + offset - 0x0800);
		return;
	}
	if (slotebr2 != -1 && slotrom != -1)
	{
		cartslots->cartridge[slotrom].rom_page = (offset & 1);
	}
}

/*
    TI-99/8 support.
*/
static READ8_DEVICE_HANDLER( ti99_cart_r_legacy8 )
{
	int slotmbx, slotmini, slotebr2, slotrom;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

#if 0
	if (hsgpl_crdena)
		return ti99_hsgpl_rom6_r(space, offset, mem_mask);
#endif

	slotmbx = cartslots->legacy_slotnumber[SLOTC_MBX];

	if ((slotmbx != -1) && (offset >= 0x0c00) && (offset <= 0x0ffd))
	{
		/* We are in the RAM area of MBX cartridges. */
		return ((UINT8 *)(cartslots->cartridge[slotmbx].ram_ptr))[offset-0x0c00];
	}

	slotmini = cartslots->legacy_slotnumber[SLOTC_MINIMEM];
	if ((slotmini != -1) && (offset >= 0x1000) && (offset <= 0x1fff))
	{
		/* We are in the RAM area of the Mini Memory cartridge. */
		return ((UINT8 *)(cartslots->cartridge[slotmini].ram_ptr))[offset-0x1000];
	}

	/* Standard */
	slotrom = cartslots->legacy_slotnumber[SLOTC_CROM];
	if (slotrom < 0)
		return 0;

	slotebr2 = cartslots->legacy_slotnumber[SLOTC_DROM];
	if (slotebr2 < 0)
		return (((UINT8 *)cartslots->cartridge[slotrom].rom_ptr)[offset]);
	else
	{
		/* Extended Basic or other paged cartridge */
		if (cartslots->cartridge[slotrom].rom_page!=0)
			return (((UINT8 *)cartslots->cartridge[slotebr2].rom2_ptr)[offset]);
		else
			return (((UINT8 *)cartslots->cartridge[slotrom].rom_ptr)[offset]);
	}
	/* Super Space, paged379i, and pagedcru are not supported in
    legacy mode. */
}

/*
    TI-99/8 support.
*/
static WRITE8_DEVICE_HANDLER( ti99_cart_w_legacy8 )
{
	int slotmbx, slotmini, slotebr2, slotrom;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

#if 0
	if (hsgpl_crdena)
	{
		ti99_hsgpl_rom6_w(space, offset, data, mem_mask);
		return;
	}
#endif
	slotmbx = cartslots->legacy_slotnumber[SLOTC_MBX];
	slotrom = cartslots->legacy_slotnumber[SLOTC_CROM];
	slotebr2 = cartslots->legacy_slotnumber[SLOTC_DROM];
	slotmini = cartslots->legacy_slotnumber[SLOTC_MINIMEM];

	if (slotmbx != -1)
	{
		if ((offset >= 0x0c00) && (offset <= 0x0ffd))
		{
			/* We are in the RAM area of MBX cartridges. */
			/* RAM in 0x6c00-0x6ffd (presumably non-paged) */
			/* mapper at 0x6ffe */
			((UINT8 *)cartslots->cartridge[slotmbx].ram_ptr)[offset - 0x0c00] = data;
		}
		else
		{
			if (offset == 0x0ffe || offset == 0x0fff)
			{
				if (slotebr2 < 0 || slotrom < 0)
					return; /* just ignore if the cartridge is not paged. */
				else
					cartslots->cartridge[slotrom].rom_page = ((data >> 7) & 1);
			}
		}
		return;
	}
	if (slotmini != -1)
	{
		if ((offset >= 0x1000) && (offset <= 0x1fff))
			/* We are in the RAM area of a MiniMemory cartridge. */
			((UINT8 *)cartslots->cartridge[slotmini].ram_ptr)[offset - 0x1000] = data;
		return;
	}
	if (slotebr2 != -1 && slotrom != -1)
	{
		cartslots->cartridge[slotrom].rom_page = (offset & 1);
	}
}


static int load_legacy(running_device *image)
{
	running_device *cartsys = image->owner;
	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	/* Some comments originally in machine/ti99_4x.c */

	/* We identify file types according to their extension */
	/* Note that if we do not recognize the extension, we revert to the
    slot location <-> type scheme. */
	int id = 0;
	int i;
	int filesize;
	int maxrom = 0x2000;
	slotc_type_t type;
	int savembx;

	const char *ch, *ch2;
	const char *name = image_filename(image);

	UINT8 *grom = memory_region(image->machine, region_grom) + 0x6000;
	UINT16 *cartrom = (UINT16 *) (memory_region(image->machine, "maincpu") + offset_cart);
	UINT16 *cartrom2 = (UINT16 *) (memory_region(image->machine, "maincpu") + offset_cart + 0x2000);
	UINT8 *cart8rom = memory_region(image->machine, "maincpu") + offset_cart_8;
	UINT8 *cart8rom2  = memory_region(image->machine, "maincpu") + offset_cart_8 + 0x2000;

	UINT16 *cartmbx;
	UINT8 *cartmbx8;

	type = SLOTC_EMPTY;

	/* There is a circuitry in TI99/4(a) that resets the console when a
    cartridge is inserted or removed.  We emulate this instead of resetting
    the emulator (which is the default in MESS). */
#if 0
	cpu_set_input_line(machine->firstcpu, INPUT_LINE_RESET, PULSE_LINE);
	tms9901_reset(0);
	if (! has_evpc)
		TMS9928A_reset();
	if (has_evpc)
		v9938_reset(0);
#endif

	ch = strrchr(name, '.');
	ch2 = (ch-1 >= name) ? ch-1 : "";

	if (ch)
	{
		if (! mame_stricmp(ch2, "g.bin"))
		{
			/* grom */
			type = SLOTC_GROM;
		}
		else if (! mame_stricmp(ch2, "c.bin"))
		{
			/* rom first page */
			type = SLOTC_CROM;
		}
		else if (! mame_stricmp(ch2, "d.bin"))
		{
			/* rom second page */
			type = SLOTC_DROM;
		}
		else if (! mame_stricmp(ch2, "m.bin"))
		{
			/* rom minimemory  */
			type = SLOTC_MINIMEM;
		}
		else if (! mame_stricmp(ch2, "b.bin"))
		{
			/* rom MBX  */
			type = SLOTC_MBX;
		}
	}

	if (type == SLOTC_EMPTY)
	{
		return INIT_FAIL;
	}

	id = get_index_from_tagname(image)-1;

	/* Remember where the cartridge part is plugged in. */
	cartslots->legacy_slotnumber[type] = id;

	savembx = FALSE;

	/* We will use the already allocated buffers in the memory regions,
       and the pointers in the cartridge structure will point to them. */
	switch (type)
	{
		case SLOTC_EMPTY:
			break;

		case SLOTC_GROM:
			filesize = image_fread(image, grom, 0xA000);
			cartslots->cartridge[id].grom_ptr = grom;
			cartslots->cartridge[id].grom_size = filesize;
			// printf("loaded GROM, size = %d\n", filesize);
			break;

		case SLOTC_CROM:
			maxrom = 0x2000;
			if (cartslots->legacy_slotnumber[SLOTC_MINIMEM] != -1)
			{
				/* This means the NVRAM has already been loaded.
                So we just load 4K of ROM to avoid overwriting
                the NVRAM. */
				maxrom = 0x1000;
			}

			if (cartslots->legacy_slotnumber[SLOTC_MBX] != -1)
			{
				/* This means the NVRAM has already been loaded.
                We need to temporarily save the RAM contents and
                write them back because the RAM overrides the
                ROM in an intermediate area. */
				savembx = TRUE;
			}

			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				if (savembx==TRUE)
				{
					cartmbx8 = (UINT8*)malloc(0x400);
					memcpy(cartmbx8, cart8rom + 0x0c00, 0x400);
					filesize = image_fread(image, cart8rom, maxrom);
					memcpy(cart8rom + 0xc00, cartmbx8, 0x400);
					free(cartmbx8);
				}
				else
				{
					filesize = image_fread(image, cart8rom, maxrom);
				}
				cartslots->cartridge[id].rom_ptr = cart8rom;
				cartslots->cartridge[id].rom_size = filesize;
			}
			else
			{
				if (savembx==TRUE)
				{
					cartmbx = (UINT16*)malloc(0x400);
					memcpy(cartmbx, cartrom + 0x0600, 0x400);
					filesize = image_fread(image, cartrom, maxrom);
					memcpy(cartrom + 0x0600, cartmbx, 0x400);
					free(cartmbx);
				}
				filesize = image_fread(image, cartrom, maxrom);
				for (i = 0; i < maxrom/2; i++)
					((UINT16 *)cartrom)[i] = BIG_ENDIANIZE_INT16(((UINT16 *)cartrom)[i]);
				cartslots->cartridge[id].rom_ptr = cartrom;
				cartslots->cartridge[id].rom_size = filesize;
			}
			// printf("loaded ROM, size = %d\n", filesize);
			break;

		case SLOTC_MINIMEM:
			/* Load the NVRAM. Need to BIG_ENDIANIZE it.
            MiniMemory has only one cartridge page. */
			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				image_battery_load(image, cart8rom+0x1000,0x1000);
				cartslots->cartridge[id].ram_ptr = cart8rom +  0x1000;
			}
			else
			{
				image_battery_load(image, cartrom+0x800,0x1000);
				for (i = 0x800; i < 0x1000; i++)
					cartrom[i] = BIG_ENDIANIZE_INT16(cartrom[i]);
				cartslots->cartridge[id].ram_ptr = (UINT16 *)cartrom +  0x800;
			}
			cartslots->cartridge[id].ram_size = 0x1000;
			// printf("loaded NVRAM\n");
			break;

		case SLOTC_MBX:
			/* Load the NVRAM. Need to BIG_ENDIANIZE it. */
			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				image_battery_load(image, cart8rom+0x0c00, 0x0400);
				cartslots->cartridge[id].ram_ptr = cart8rom +  0xc00;
			}
			else
			{
				image_battery_load(image, cartrom+0x0600, 0x0400);
				for (i = 0x0600; i < 0x0800; i++)
					cartrom[i] = BIG_ENDIANIZE_INT16(cartrom[i]);
				cartslots->cartridge[id].ram_ptr = (UINT16 *)cartrom +  0x600;
			}
			cartslots->cartridge[id].ram_size = 0x0400;
			// printf("loaded NVRAM\n");
			break;

		case SLOTC_DROM:
			maxrom = 0x2000;
			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				filesize = image_fread(image, cart8rom2, maxrom);
				cartslots->cartridge[id].rom2_ptr = cart8rom2;
			}
			else
			{
				filesize = image_fread(image, cartrom2, maxrom);
				for (i = 0; i < maxrom/2; i++)
					cartrom2[i] = BIG_ENDIANIZE_INT16(cartrom2[i]);
				cartslots->cartridge[id].rom2_ptr = cartrom2;
			}
			cartslots->cartridge[id].rom_size = filesize;
			// printf("loaded second ROM, size = %d\n", filesize);
			break;
	}
	return INIT_PASS;
}

static void unload_legacy(running_device *image)
{
	int i;
	running_device *cartsys = image->owner;

	ti99_multicart_t *cartslots = (ti99_multicart_t *)cartsys->token;

	slotc_type_t type = SLOTC_EMPTY;
	int slot = get_index_from_tagname(image)-1;

	// printf("non-rpk unload\n");

	for (i=0; (i < 5) && (type==SLOTC_EMPTY); i++)
	{
		if (cartslots->legacy_slotnumber[i]==slot)
		{
			cartslots->legacy_slotnumber[i] = -1;
			type = (slotc_type_t)i;
		}
	}

	switch (type)
	{
		case SLOTC_EMPTY:
			break;

		case SLOTC_GROM:
			memset(cartslots->cartridge[slot].grom_ptr, 0, 0xA000);
			break;

		case SLOTC_MINIMEM:
			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				image_battery_save(image, cartslots->cartridge[slot].ram_ptr, 0x1000);
				memset(cartslots->cartridge[slot].ram_ptr, 0, 0x1000);
			}
			else
			{
				/* We BIG_ENDIANIZE before saving. This is
                consistent with the cartridge save format. */
				UINT16 *ramcont = (UINT16 *)cartslots->cartridge[slot].ram_ptr;
				for (i = 0; i < 0x0800; i++)
					ramcont[i] = BIG_ENDIANIZE_INT16(ramcont[i]);
				image_battery_save(image, ramcont, 0x1000);
				memset(ramcont, 0, 0x1000);
			}
			break;
		case SLOTC_MBX:
			if (ti99_is_99_8())
			{
				// printf("** is 99/8\n");
				image_battery_save(image, cartslots->cartridge[slot].ram_ptr, 0x0400);
				memset(cartslots->cartridge[slot].ram_ptr, 0, 0x0400);
			}
			else
			{
				/* We BIG_ENDIANIZE before saving. This is
                consistent with the cartridge save format. */
				UINT16 *ramcont = (UINT16 *)cartslots->cartridge[slot].ram_ptr;
				for (i = 0; i < 0x0200; i++)
					ramcont[i] = BIG_ENDIANIZE_INT16(ramcont[i]);
				image_battery_save(image, ramcont, 0x0400);
				memset(ramcont, 0, 0x0400);
			}
			break;
		case SLOTC_CROM:
			if (cartslots->legacy_slotnumber[SLOTC_MINIMEM]!=-1)
				/* Don't wipe the RAM before it is saved. */
				memset(cartslots->cartridge[slot].rom_ptr, 0, 0x1000);
			else
				memset(cartslots->cartridge[slot].rom_ptr, 0, 0x2000);
			break;

		case SLOTC_DROM:
			cartslots->cartridge[slot].rom_page = 0;
			memset(cartslots->cartridge[slot].rom2_ptr, 0, 0x2000);
			break;
	}

	clear_slot(cartsys, slot);
}

/*****************************************************************************
  The overall multi-cartridge slot system. It contains instances of
  cartridges which contain PCB devices. The memory handlers delegate the calls
  to the respective handlers of the cartridges.

  Note that the term "multi-cartridge system" and "multicart" are not the same:
  A "multicart" may contain multiple resources, organized on a PCB. The multi-
  cart system may thus host multiple multicarts.

  Actually, the name of the device should be changed (however, the device name
  length is limited)
******************************************************************************/
/*
    Instantiation of a multicart system for the TI family.
*/
static DEVICE_START(ti99_multicart)
{
	int i;
//  printf("DEVICE_START(ti99_multicart)\n");
	ti99_multicart_t *cartslots = (ti99_multicart_t *)device->token;

	cartslots->active_slot = 0;
	cartslots->next_free_slot = 0;

	for (i=0; i < NUMBER_OF_CARTRIDGE_SLOTS; i++)
	{
		cartslots->cartridge[i].pcb = NULL;
		clear_slot(device, i);
	}

	/* Initialize the legacy system. */
	cartslots->legacy_slotnumber[SLOTC_GROM] = -1;
	cartslots->legacy_slotnumber[SLOTC_CROM] = -1;
	cartslots->legacy_slotnumber[SLOTC_DROM] = -1;
	cartslots->legacy_slotnumber[SLOTC_MINIMEM] = -1;
	cartslots->legacy_slotnumber[SLOTC_MBX] = -1;

	cartslots->legacy_slots = 0;
	cartslots->multi_slots = 0;

	/* The cartslot system is initialized now. The cartridges themselves
       need to check whether their parts are available. */
}

static DEVICE_STOP(ti99_multicart)
{
//  printf("DEVICE_STOP(ti99_multicart)\n");
}

static DEVICE_RESET(ti99_multicart)
{
	/* Consider to propagate RESET to cartridges.
       However, schematics do not reveal any pin for resetting a cartridge;
       the reset line is an input used when plugging in a cartridge. */
//    printf("DEVICE_RESET(ti99_multicart)\n");
}

/*
    Accesses the ROM regions of the cartridge for reading.
    This is the area from 0x6000 to 0x7fff. Each cartridge is reponsible
    for all kinds of magic which is done inside this area, like swapping
    banks or doing other control actions. These activities beyond simple
    reading are cartridge-specific. Each kind of cartridge requires a
    new type.
    The currently set GROM bank determines which cartridge is actually
    accessed, also for ROM reads.
*/
READ16_DEVICE_HANDLER( ti99_multicart_r )
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	int slot = cartslots->active_slot;
	cartridge_t *cart;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return 0;

	/* Same as above (GROM read): If there is only one cartridge there
       is no use of trying other banks. There is one issue with the
       selection mechanism: When the console probes the GROM banks and
       we have a ROM-only cartridge plugged in, it checks bank 0 and bank 1,
       finds no GROM, and then fails to dummy-check bank 0 again which would
       have set our cartridge expander to slot 0. But in slot 1 there is no
       ROM cartridge, so nothing shows up in the selection list. We work
       around this with the following line. */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	/* Idle the CPU */
        cpu_adjust_icount(devtag_get_device(device->machine, "maincpu"),-4);

	// Alternatively:
	// cpu_spinuntil_time(device->machine->firstcpu, ATTOTIME_IN_USEC(6));

//  printf("rpk = %d, bin = %d\n", cartslots->multi_slots,cartslots->legacy_slots);

	/* Handle legacy mode. */
	if (in_legacy_mode(device))
	{
		return ti99_cart_r_legacy(device, offset, mem_mask);
	}

	if (!slot_is_empty(device, slot))
	{
//      printf("accessing cartridge in slot %d\n", slot);
		ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
//      printf("address=%lx, offset=%lx\n", pcbdef, offset);
		return (*pcbdef->read)(cart->pcb, offset, mem_mask);
	}
	else
	{
//      printf("No cartridge in slot %d\n", slot);
		return 0;
	}
}

/*
    Accesses the ROM regions of the cartridge for writing.
    This is the area from 0x6000 to 0x7fff. Each cartridge is reponsible
    for all kinds of magic which is done inside this area. Specifically,
    writing is often used to swap banks for subsequent reads. The actual
    effect of writing (beyond setting values in a RAM chip) are
    cartridge-specific and must be handled within the cartridge pcb.
    The currently set GROM bank determines which cartridge is actually
    accessed, also for ROM reads.
*/
WRITE16_DEVICE_HANDLER( ti99_multicart_w )
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	int slot = cartslots->active_slot;
	cartridge_t *cart;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	/* Idle the CPU */
        cpu_adjust_icount(devtag_get_device(device->machine, "maincpu"),-4);
//  cpu_spinuntil_time(device->machine->firstcpu, ATTOTIME_IN_USEC(6));

	/* Handle legacy mode. */
	if (in_legacy_mode(device))
	{
		ti99_cart_w_legacy(device, offset, data, mem_mask);
	}
	else
	{
		if (!slot_is_empty(device, slot))
		{
//          printf("writing to cartridge in slot %d\n", slot);
			ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
			(*pcbdef->write)(cart->pcb, offset, data, mem_mask);
		}
	}
}

/*
    CRU interface to cartridges, used only by SuperSpace and Pagedcru
    style cartridges.
        The SuperSpace has a CRU-based memory mapper. It is accessed via the
        cartridge slot on CRU base >0800.
*/
READ8_DEVICE_HANDLER( ti99_multicart_cru_r )
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	/* We don't support this cartridge type in the legacy mode. */
	if (in_legacy_mode(device))
		return 0;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return 0;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
		ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
		if (pcbdef->cruread != NULL)
			return (*pcbdef->cruread)(cart->pcb, offset);
	}
	return 0;
}

/*
        Write cartridge mapper CRU interface (SuperSpace, Pagedcru)
*/
WRITE8_DEVICE_HANDLER( ti99_multicart_cru_w )
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	/* We don't support this cartridge type in the legacy mode. */
	if (in_legacy_mode(device))
		return;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
		ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
		if (pcbdef->cruwrite != NULL)
			(*pcbdef->cruwrite)(cart->pcb, offset, data);
	}
}

/*
    TI-99/8 support. We have only 8 bit memory access, so we adapt all
    functions to this width.
*/
READ8_DEVICE_HANDLER(ti99_multicart8_r)
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return 0;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	/* Idle the CPU. TODO: Check whether this is correct for this machine. */
        cpu_adjust_icount(devtag_get_device(device->machine, "maincpu"),-4);

	/* Handle legacy mode. */
	if (in_legacy_mode(device))
	{
		return ti99_cart_r_legacy8(device, offset);
	}

	if (!slot_is_empty(device, slot))
	{
//      printf("accessing cartridge in slot %d\n", slot);
		ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
		return (*pcbdef->read8)(cart->pcb, offset);
	}
	else
	{
//      printf("No cartridge in slot %d\n", slot);
		return 0;
	}
}

WRITE8_DEVICE_HANDLER(ti99_multicart8_w)
{
	ti99_multicart_t *cartslots = (ti99_multicart_t *) device->token;
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	/* Idle the CPU. TODO: Check whether this is correct for this machine. */
        cpu_adjust_icount(devtag_get_device(device->machine, "maincpu"),-4);

	/* Handle legacy mode. */
	if (in_legacy_mode(device))
	{
		ti99_cart_w_legacy8(device, offset, data);
	}
	else
	{
		if (!slot_is_empty(device, slot))
		{
			ti99_pcb_t *pcbdef = (ti99_pcb_t *)cart->pcb->token;
			(*pcbdef->write8)(cart->pcb, offset, data);
		}
	}
}

static MACHINE_DRIVER_START(ti99_multicart)
 	MDRV_CARTSLOT_ADD("cartridge1")
	MDRV_CARTSLOT_EXTENSION_LIST("rpk,bin")
	MDRV_CARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE)
	MDRV_CARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD)
	MDRV_CARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED)
	MDRV_CARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM)
	MDRV_CARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER)
	MDRV_CARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX)
	MDRV_CARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I)
	MDRV_CARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU)

	MDRV_CARTSLOT_START(ti99_cartridge)
	MDRV_CARTSLOT_LOAD(ti99_cartridge)
	MDRV_CARTSLOT_UNLOAD(ti99_cartridge)

	MDRV_CARTSLOT_ADD("cartridge2")
	MDRV_CARTSLOT_EXTENSION_LIST("rpk,bin")
	MDRV_CARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE)
	MDRV_CARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD)
	MDRV_CARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED)
	MDRV_CARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM)
	MDRV_CARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER)
	MDRV_CARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX)
	MDRV_CARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I)
	MDRV_CARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU)

	MDRV_CARTSLOT_START(ti99_cartridge)
	MDRV_CARTSLOT_LOAD(ti99_cartridge)
	MDRV_CARTSLOT_UNLOAD(ti99_cartridge)

	MDRV_CARTSLOT_ADD("cartridge3")
	MDRV_CARTSLOT_EXTENSION_LIST("rpk,bin")
	MDRV_CARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE)
	MDRV_CARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD)
	MDRV_CARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED)
	MDRV_CARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM)
	MDRV_CARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER)
	MDRV_CARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX)
	MDRV_CARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I)
	MDRV_CARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU)

	MDRV_CARTSLOT_START(ti99_cartridge)
	MDRV_CARTSLOT_LOAD(ti99_cartridge)
	MDRV_CARTSLOT_UNLOAD(ti99_cartridge)

	MDRV_CARTSLOT_ADD("cartridge4")
	MDRV_CARTSLOT_EXTENSION_LIST("rpk,bin")
	MDRV_CARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE)
	MDRV_CARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD)
	MDRV_CARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED)
	MDRV_CARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM)
	MDRV_CARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER)
	MDRV_CARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX)
	MDRV_CARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I)
	MDRV_CARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU)

	MDRV_CARTSLOT_START(ti99_cartridge)
	MDRV_CARTSLOT_LOAD(ti99_cartridge)
	MDRV_CARTSLOT_UNLOAD(ti99_cartridge)
MACHINE_DRIVER_END


DEVICE_GET_INFO(ti99_multicart)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:
			info->machine_config = MACHINE_DRIVER_NAME(ti99_multicart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 Multi-Cartridge Extender");
			break;

		case DEVINFO_INT_TOKEN_BYTES: /* private storage, automatically allocated */
			info->i = sizeof(ti99_multicart_t);
			break;
		case DEVINFO_INT_CLASS:
			info->i = DEVICE_CLASS_PERIPHERAL;
			break;

		case DEVINFO_STR_FAMILY:
			strcpy(info->s, "Multi-cartridge system");
			break;
		case DEVINFO_STR_VERSION:
			strcpy(info->s, "1.0");
			break;
		case DEVINFO_STR_SOURCE_FILE:
			strcpy(info->s, __FILE__);
			break;
		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_multicart);
			break;
		case DEVINFO_FCT_STOP:
			info->stop = DEVICE_STOP_NAME(ti99_multicart);
			break;
		case DEVINFO_FCT_RESET:
			info->reset = DEVICE_RESET_NAME(ti99_multicart);
			break;
	}
}

