/**********************************************************************

    gromport.c

*********************************************************************/

#include "emu.h"
#include "gromport.h"
#include "imagedev/cartslot.h"
#include "imagedev/multcart.h"
#include "grom.h"

typedef UINT8	(*read8z_device_func)  (ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT8 *value);

/* We set the number of slots to 8, although we may have up to 16. From a
   logical point of view we could have 256, but the operating system only checks
   the first 16 banks. */
#define NUMBER_OF_CARTRIDGE_SLOTS 8

typedef int assmfct(device_t *);

DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_NONE, ti99_cartridge_pcb_none);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_STD, ti99_cartridge_pcb_std);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGED, ti99_cartridge_pcb_paged);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_MINIMEM, ti99_cartridge_pcb_minimem);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_SUPER, ti99_cartridge_pcb_super);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_MBX, ti99_cartridge_pcb_mbx);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGED379I, ti99_cartridge_pcb_paged379i);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGEDCRU, ti99_cartridge_pcb_pagedcru);
DECLARE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_GRAMKRACKER, ti99_cartridge_pcb_gramkracker);

DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_NONE, ti99_cartridge_pcb_none);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_STD, ti99_cartridge_pcb_std);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGED, ti99_cartridge_pcb_paged);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_MINIMEM, ti99_cartridge_pcb_minimem);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_SUPER, ti99_cartridge_pcb_super);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_MBX, ti99_cartridge_pcb_mbx);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGED379I, ti99_cartridge_pcb_paged379i);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_PAGEDCRU, ti99_cartridge_pcb_pagedcru);
DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_CARTRIDGE_PCB_GRAMKRACKER, ti99_cartridge_pcb_gramkracker);

/* Generic TI99 cartridge structure. */
struct _cartridge_t
{
	// PCB device associated to this cartridge. If NULL, the slot is empty.
	device_t *pcb;

	// ROM page.
	int rom_page;

	// RAM page.
	int ram_page;

	// GROM buffer size.
	int grom_size;

	// ROM buffer size. All banks have equal sizes.
	int rom_size;

	// RAM buffer size. All banks have equal sizes.
	int ram_size;

	// GROM buffer.
	UINT8 *grom_ptr;

	// ROM buffer.
	UINT8 *rom_ptr;

	// ROM buffer for the second bank of paged cartridges.
	UINT8 *rom2_ptr;

	// RAM buffer. The persistence service is done by the cartridge system.
	// The RAM space is a consecutive space; all banks are in one buffer.
	UINT8 *ram_ptr;

	/* GK support. */
	UINT16	grom_address;
	int		waddr_LSB;
};
typedef struct _cartridge_t cartridge_t;

enum
{
	TI99CARTINFO_FCT_6000_R = DEVINFO_FCT_DEVICE_SPECIFIC,
	TI99CARTINFO_FCT_6000_W,
	TI99CARTINFO_FCT_CRU_R,
	TI99CARTINFO_FCT_CRU_W,
	TI99CART_FCT_ASSM,
	TI99CART_FCT_DISASSM
};

typedef struct _ti99_multicart_state
{
	// Reserves space for all cartridges. This is also used in the legacy
	// cartridge system, but only for slot 0.
	cartridge_t cartridge[NUMBER_OF_CARTRIDGE_SLOTS];

	// Determines which slot is currently active. This value is changed when
	// there are accesses to other GROM base addresses.
	int 	active_slot;

	// Used in order to enforce a special slot. This value is retrieved
	// from the dipswitch setting. A value of -1 means automatic, that is,
	// the grom base switch is used. Values 0 .. max refer to the
	// respective slot.
	int 	fixed_slot;

	// Holds the highest index of a cartridge being plugged in plus one.
	// If we only have one cartridge inserted, we don't want to get a
	// selection option, so we just mirror the memory contents.
	int 	next_free_slot;

	/* GK support. */
	int		gk_slot;
	int		gk_guest_slot;

	/* Legacy support. */
	// Counts the number of slots which currently contain legacy format
	// cartridge images.
	int legacy_slots;

	// Counts the number of slots which currently contain new format
	// cartridge images.
	int multi_slots;

	/* Used to cache the switch settings. */
	UINT8 gk_switch[8];

	/* Ready line */
	devcb_resolved_write_line ready;

} ti99_multicart_state;

#define AUTO -1

/* Access to the pcb. Contained in the token of the pcb instance. */
struct _ti99_pcb_t
{
	/* Read function for this cartridge. */
	read8z_device_func read;

	/* Write function for this cartridge. */
	write8_device_func write;

	/* Read function for this cartridge, CRU. */
	read8z_device_func cruread;

	/* Write function for this cartridge, CRU. */
	write8_device_func cruwrite;

	/* Link up to the cartridge structure which contains this pcb. */
	cartridge_t *cartridge;

	/* Function to assemble this cartridge. */
	assmfct	*assemble;

	/* Function to disassemble this cartridge. */
	assmfct	*disassemble;

	/* Points to the (max 5) GROM devices. */
	device_t *grom[5];
};
typedef struct _ti99_pcb_t ti99_pcb_t;

INLINE ti99_multicart_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TI99_MULTICART);

	return (ti99_multicart_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99_multicart_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TI99_MULTICART);

	return (const ti99_multicart_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE ti99_pcb_t *get_safe_pcb_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TI99_CARTRIDGE_PCB_NONE ||
		device->type() == TI99_CARTRIDGE_PCB_STD ||
		device->type() == TI99_CARTRIDGE_PCB_PAGED ||
		device->type() == TI99_CARTRIDGE_PCB_MINIMEM ||
		device->type() == TI99_CARTRIDGE_PCB_SUPER ||
		device->type() == TI99_CARTRIDGE_PCB_MBX ||
		device->type() == TI99_CARTRIDGE_PCB_PAGED379I ||
		device->type() == TI99_CARTRIDGE_PCB_PAGEDCRU ||
		device->type() == TI99_CARTRIDGE_PCB_GRAMKRACKER);

	return (ti99_pcb_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE cartslot_t *get_safe_cartslot_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == CARTSLOT);

	return (cartslot_t *)downcast<legacy_device_base *>(device)->token();
}

static WRITE_LINE_DEVICE_HANDLER( cart_grom_ready )
{
	device_t *cartdev = device->owner()->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartdev);
	devcb_call_write_line( &cartslots->ready, state );
}

/*
    The console writes the values in this array to cache them.
*/
void set_gk_switches(device_t *cartsys, int number, int value)
{
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
	cartslots->gk_switch[number] = value;
}

static int get_gk_switch(device_t *cartsys, int number)
{
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
	return cartslots->gk_switch[number];
}

static int in_legacy_mode(device_t *device)
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	return ((cartslots->legacy_slots>0) && (cartslots->multi_slots==0));
}

/*
    Activates a slot in the multi-cartridge extender.
*/
static void cartridge_slot_set(device_t *cartsys, UINT8 slotnumber)
{
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
//  if (cartslots->active_slot != slotnumber) printf("Setting cartslot to %d\n", slotnumber);
	if (cartslots->fixed_slot==AUTO)
		cartslots->active_slot = slotnumber;
	else
		cartslots->active_slot = cartslots->fixed_slot;
}

static int slot_is_empty(device_t *cartsys, int slotnumber)
{
	cartridge_t *cart;
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	cart = &cartslots->cartridge[slotnumber];

	if ((cart->rom_size==0) && (cart->ram_size==0) && (cart->grom_size==0))
		return TRUE;
	return FALSE;
}

static void clear_slot(device_t *cartsys, int slotnumber)
{
	cartridge_t *cart;
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	cart = &cartslots->cartridge[slotnumber];

	cart->rom_size = cart->ram_size = cart->grom_size = 0;
	cart->rom_ptr = NULL;
	cart->rom2_ptr = NULL;
	cart->ram_ptr = NULL;
}

/*
    Find the index of the cartridge name. We assume the format
    <name><number>, i.e. the number is the longest string from the right
    which can be interpreted as a number.
*/
static int get_index_from_tagname(device_t *image)
{
	const char *tag = image->tag();
	int maxlen = strlen(tag);
	int i;
	for (i=maxlen-1; i >=0; i--)
		if (tag[i] < 48 || tag[i] > 57) break;

	return atoi(tag+i+1);
}

/*
    Common routine to assemble cartridges from resources.
*/
static cartridge_t *assemble_common(device_t *cartslot)
{
	/* Pointer to the cartridge structure. */
	cartridge_t *cartridge;
	device_t *cartsys = cartslot->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
	device_t *pcb = cartslot->subdevice("pcb");
	ti99_pcb_t *pcb_def;

	void *socketcont;
	int reslength;
//  int i;

	int slotnumber = get_index_from_tagname(cartslot)-1;
	assert(slotnumber>=0 && slotnumber<NUMBER_OF_CARTRIDGE_SLOTS);

	/* There is a cartridge in this slot, check the maximum slot number. */
	if (cartslots->next_free_slot <= slotnumber)
	{
		cartslots->next_free_slot = slotnumber+1;
	}
	cartridge = &cartslots->cartridge[slotnumber];

	// Get the socket which is retrieved by the multicart instance in
	// the state of the cartslot instance.
	socketcont = cartslot_get_socket(cartslot, "grom_socket");
	reslength = cartslot_get_resource_length(cartslot, "grom_socket");
	if (socketcont != NULL)
	{
		cartridge->grom_ptr = (UINT8 *)socketcont;
//      printf("grom_socket = %lx\n", cartridge->grom_ptr);
		cartridge->grom_size = reslength;
	}

	socketcont = cartslot_get_socket(cartslot, "rom_socket");
	reslength = cartslot_get_resource_length(cartslot, "rom_socket");
	if (socketcont != NULL)
	{
		cartridge->rom_ptr = (UINT8 *)socketcont;
		cartridge->rom_size = reslength;
//      printf("set rom ptr = %lx, romlength = %04x, data = %02x %02x\n", cartridge->rom_ptr, cartridge->rom_size, cartridge->rom_ptr[0], cartridge->rom_ptr[1]);
	}

	socketcont = cartslot_get_socket(cartslot, "rom2_socket");
//  reslength = cartslot_get_resource_length(cartslot, "rom2_socket");  /* cannot differ from rom_socket */
	if (socketcont != NULL)
	{
		cartridge->rom2_ptr = (UINT8 *)socketcont;
	}

	socketcont = cartslot_get_socket(cartslot, "ram_socket");
	reslength = cartslot_get_resource_length(cartslot, "ram_socket");
	if (socketcont != NULL)
	{
		cartridge->ram_ptr = (UINT8 *)socketcont;
		cartridge->ram_size = reslength;
	}

	// Register the GROMs
	pcb_def = get_safe_pcb_token(pcb);
	for (int i=0; i < 5; i++) pcb_def->grom[i]= NULL;
	if (cartridge->grom_size > 0)      pcb_def->grom[0] = pcb->subdevice("grom_3");
	if (cartridge->grom_size > 0x2000) pcb_def->grom[1] = pcb->subdevice("grom_4");
	if (cartridge->grom_size > 0x4000) pcb_def->grom[2] = pcb->subdevice("grom_5");
	if (cartridge->grom_size > 0x6000) pcb_def->grom[3] = pcb->subdevice("grom_6");
	if (cartridge->grom_size > 0x8000) pcb_def->grom[4] = pcb->subdevice("grom_7");

	return cartridge;
}

static void set_pointers(device_t *pcb, int index)
{
	device_t *cartsys = pcb->owner()->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
	ti99_pcb_t *pcb_def = get_safe_pcb_token(pcb);

	pcb_def->read = (read8z_device_func)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CARTINFO_FCT_6000_R);
	pcb_def->write = (write8_device_func)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CARTINFO_FCT_6000_W);
	pcb_def->cruread = (read8z_device_func)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CARTINFO_FCT_CRU_R);
	pcb_def->cruwrite = (write8_device_func)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CARTINFO_FCT_CRU_W);

    pcb_def->assemble = (assmfct *)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CART_FCT_ASSM);
    pcb_def->disassemble = (assmfct *)downcast<const legacy_cart_slot_device_config_base *>(&pcb->baseconfig())->get_config_fct(TI99CART_FCT_DISASSM);

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
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*****************************************************************************
  Cartridge type: Standard
    Most cartridges are built in this type. Every cartridge may contain
    GROM between 0 and 40 KiB, and ROM with 8 KiB length, no banking.
******************************************************************************/

static DEVICE_START(ti99_pcb_std)
{
	/* device is ti99_cartslot:cartridge:pcb */
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ8Z_DEVICE_HANDLER( read_cart_std )
{
	/* device is pcb, owner is cartslot (cartridge is only a part of the state of cartslot) */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	if (cartridge->rom_ptr!=NULL)
	{
		// For TI-99/8 we should plan for 16K cartridges. However, none was produced. Well, just ignore this thought,
		*value = (cartridge->rom_ptr)[offset & 0x1fff];
//      printf("read cartridge rom space (length %04x) %04x = %02x\n", cartridge->rom_size, offset, *value);
	}
}

static WRITE8_DEVICE_HANDLER( write_cart_std )
{
	logerror("Write access to cartridge ROM at address %04x ignored\n", offset);
}

/*
    The standard cartridge assemble routine. We just call the common
    function here.
*/
static int assemble_std(device_t *image)
{
	assemble_common(image);
	return IMAGE_INIT_PASS;
}

/*
    Removes pointers and restores the state before plugging in the
    cartridge.
    The pointer to the location after the last cartridge is adjusted.
    As it seems, we can use the same function for the disassembling of all
    cartridge types.
*/
static int disassemble_std(device_t *image)
{
	int slotnumber;
	int i;
//  cartridge_t *cart;
	device_t *cartsys = image->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	slotnumber = get_index_from_tagname(image)-1;

	/* Search the highest remaining cartridge. */
	cartslots->next_free_slot = 0;
	for (i=NUMBER_OF_CARTRIDGE_SLOTS-1; i >= 0; i--)
	{
		if (i != slotnumber)
		{
			if (!slot_is_empty(cartsys, i))
			{
				cartslots->next_free_slot = i+1;
				break;
			}
		}
	}

	clear_slot(cartsys, slotnumber);

	return IMAGE_INIT_PASS;
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
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ8Z_DEVICE_HANDLER( read_cart_paged )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	UINT8 *rombank;

	if (cartridge->rom_page==0)
		rombank = (UINT8*)cartridge->rom_ptr;
	else
		rombank = (UINT8*)cartridge->rom2_ptr;

	if (rombank==NULL)
	{
		/* TODO: Check for consistency with the GROM memory handling. */
		*value = 0;
	}
	else
	{
		*value = rombank[offset & 0x1fff];
	}
}

/*
    Handle paging. Extended Basic switches between ROM banks by
    using the value of the LSB of the address where any value
    is written to.
*/
static WRITE8_DEVICE_HANDLER( write_cart_paged )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	cartridge->rom_page = (offset >> 1) & 1;
}

/*
    We require paged modules to have at least those two rom banks.
*/
static int assemble_paged(device_t *image)
{
	cartridge_t *cart;

	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for paged cartridge");
		return IMAGE_INIT_FAIL;
	}
	if (cart->rom2_ptr==NULL)
	{
		logerror("Missing second ROM for paged cartridge");
		return IMAGE_INIT_FAIL;
	}

	return IMAGE_INIT_PASS;
}

/*****************************************************************************
  Cartridge type: Mini Memory
    GROM: 6 KiB (occupies G>6000 to G>7800)
    ROM: 4 KiB (romfile is actually 8 K long, second half with zeros, 0x6000-0x6fff)
    persistent RAM: 4 KiB (0x7000-0x7fff)
******************************************************************************/

static DEVICE_START(ti99_pcb_minimem)
{
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

static READ8Z_DEVICE_HANDLER( read_cart_minimem )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset & 0x1000)==0x0000)
	{
		*value = (cartridge->rom_ptr==NULL)? 0 : (cartridge->rom_ptr)[offset & 0x0fff];
	}
	else
	{
		*value = (cartridge->ram_ptr==NULL)? 0 : (cartridge->ram_ptr)[offset & 0x0fff];
	}
}

/*
    Mini Memory cartridge write operation. RAM is located at 0x7000.
*/
static WRITE8_DEVICE_HANDLER( write_cart_minimem )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset & 0x1000)==0x0000)
	{
		logerror("Write access to cartridge ROM at address %04x ignored", offset);
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			logerror("No cartridge RAM at address %04x", offset);
			/* TODO: Check for consistency with the GROM memory handling. */
		}
		else
		{
			(cartridge->ram_ptr)[offset & 0x0fff] = data;
		}
	}
}

static int assemble_minimem(device_t *image)
{
	cartridge_t *cart;

	cart = assemble_common(image);
	if (cart->grom_size==0)
	{
		logerror("Missing GROM for Mini Memory");
		// should not fail here because there may be variations of
		// cartridges which do not use all parts
//      return IMAGE_INIT_FAIL;
	}
	if (cart->rom_size==0)
	{
		logerror("Missing ROM for Mini Memory");
//      return IMAGE_INIT_FAIL;
	}
	if (cart->ram_size==0)
	{
		logerror("Missing RAM for Mini Memory");
//      return IMAGE_INIT_FAIL;
	}

	return IMAGE_INIT_PASS;
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
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    The CRU read handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the SuperSpace cartridge.
*/
static READ8Z_DEVICE_HANDLER( read_cart_cru )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	// offset is the bit number. The CRU base address is already divided  by 2.

	// ram_page contains the bank number. We have a maximum of
	// 4 banks; the Super Space II manual says:
	//
	// Banks are selected by writing a bit pattern to CRU address >0800:
	//
	// Bank #   Value
	// 0        >02  = 0000 0010
	// 1        >08  = 0000 1000
	// 2        >20  = 0010 0000
	// 3        >80  = 1000 0000
	//
	// With the bank number (0, 1, 2, or 3) in R0:
	//
	// BNKSW   LI    R12,>0800   Set CRU address
	//         LI    R1,2        Load Shift Bit
	//         SLA   R0,1        Align Bank Number
	//         JEQ   BNKS1       Skip shift if Bank 0
	//         SLA   R1,0        Align Shift Bit
	// BNKS1   LDCR  R1,0        Switch Banks
	//         SRL   R0,1        Restore Bank Number (optional)
	//         RT
	logerror("Superspace: CRU accessed at %04x\n", offset);
	if ((offset & 1) == 0 || offset > 7)
		*value = 0;

	// CRU addresses are only 1 bit wide. Bytes are transferred from LSB
	// to MSB. That is, 8 bit are eight consecutive addresses. */
	*value = (cartridge->ram_page == (offset-1)/2);
}

static WRITE8_DEVICE_HANDLER( write_cart_cru )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	// data is bit
	// offset is address
	logerror("Superspace: CRU accessed at %04x\n", offset);
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
static READ8Z_DEVICE_HANDLER( read_cart_super )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		*value = 0;
	}
	else
	{
		boffset = cartridge->ram_page * 0x2000 + (offset & 0x1fff);
		*value = (cartridge->ram_ptr)[boffset];
	}
}

/*
    SuperSpace RAM write operation. RAM is located at 0x6000 to 0x7fff
    as 4 banks of 8 KiB each
*/
static WRITE8_DEVICE_HANDLER( write_cart_super )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;
	offs_t boffset;

	if (cartridge->ram_ptr==NULL)
	{
		logerror("No cartridge RAM at address %04x", offset);
	}
	else
	{
		boffset = cartridge->ram_page * 0x2000 + (offset & 0x1fff);
		(cartridge->ram_ptr)[boffset] = data;
	}
}

static int assemble_super(device_t *image)
{
	cartridge_t *cart;

	cart = assemble_common(image);
	if (cart->ram_size==0)
	{
		logerror("Missing RAM for SuperSpace");
		return IMAGE_INIT_FAIL;
	}
	return IMAGE_INIT_PASS;
}

/*****************************************************************************
  Cartridge type: MBX
    GROM: up to 40 KiB
    ROM: up to 16 KiB (in up to 2 banks of 8KiB each)
    RAM: 1022 B (0x6c00-0x6ffd, overrides ROM in that area)
    ROM mapper: 6ffe

    Note that some MBX cartridges assume the presence of the MBX system
    (additional console) and will not run without it. The MBX hardware is
    not emulated yet.
******************************************************************************/

static DEVICE_START(ti99_pcb_mbx)
{
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

static READ8Z_DEVICE_HANDLER( read_cart_mbx )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset & 0x1c00)==0x0c00)
	{
		// This is the RAM area which overrides any ROM. There is no
		// known banking behavior for the RAM, so we must assume that
		// there is only one bank.
		if (cartridge->ram_ptr != NULL)
			*value = (cartridge->ram_ptr)[offset & 0x03ff];
	}
	else
	{
		if (cartridge->rom_ptr != NULL)
			*value = (cartridge->rom_ptr)[(offset & 0x1fff)+ cartridge->rom_page*0x2000];
	}
}

/*
    MBX write operation.
*/
static WRITE8_DEVICE_HANDLER( write_cart_mbx )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if (offset == 0x6ffe)
	{
		cartridge->rom_page = data & 1;
		return;
	}

	if ((offset & 0x1c00)==0x0c00)
	{
		if (cartridge->ram_ptr == NULL)	return;
		cartridge->ram_ptr[offset & 0x03ff] = data;
	}
}


static int assemble_mbx(device_t *image)
{
	assemble_common(image);
	return IMAGE_INIT_PASS;
}

/*****************************************************************************
  Cartridge type: paged379i
    This cartridge consists of one 16 KiB, 32 KiB, 64 KiB, or 128 KiB EEPROM
    which is organised in 2, 4, 8, or 16 pages of 8 KiB each. The complete
    memory contents must be stored in one dump file.
    The pages are selected by writing a value to some memory locations. Due to
    using the inverted outputs of the LS379 latch, setting the inputs of the
    latch to all 0 selects the highest bank, while setting to all 1 selects the
    lowest. There are some cartridges (16 KiB) which are using this scheme, and
    there are new hardware developments mainly relying on this scheme.

    Writing to       selects page (16K/32K/64K/128K)
    >6000            1 / 3 / 7 / 15
    >6002            0 / 2 / 6 / 14
    >6004            1 / 1 / 5 / 13
    >6006            0 / 0 / 4 / 12
    >6008            1 / 3 / 3 / 11
    >600A            0 / 2 / 2 / 10
    >600C            1 / 1 / 1 / 9
    >600E            0 / 0 / 0 / 8
    >6010            1 / 3 / 7 / 7
    >6012            0 / 2 / 6 / 6
    >6014            1 / 1 / 5 / 5
    >6016            0 / 0 / 4 / 4
    >6018            1 / 3 / 3 / 3
    >601A            0 / 2 / 2 / 2
    >601C            1 / 1 / 1 / 1
    >601E            0 / 0 / 0 / 0

******************************************************************************/

static DEVICE_START(ti99_pcb_paged379i)
{
	/* device is ti99_cartslot:cartridge:pcb */
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ8Z_DEVICE_HANDLER( read_cart_paged379i )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if (cartridge->rom_ptr!=NULL)
	{
		*value = (cartridge->rom_ptr)[cartridge->rom_page*0x2000 + (offset & 0x1fff)];
	}
}

/*
    Determines which bank to set, depending on the size of the ROM. This is
    some magic code that actually represents different PCB versions.
*/
static void set_paged379i_bank(cartridge_t *cartridge, int rompage)
{
	int mask = 0;
	if (cartridge->rom_size > 16384)
	{
		if (cartridge->rom_size > 32768)
		{
			if (cartridge->rom_size > 65536)
				mask = 15;
			else
				mask = 7;
		}
		else
			mask = 3;
	}
	else
		mask = 1;

	cartridge->rom_page = rompage & mask;
}

/*
    Handle paging. We use a LS379 latch for storing the page number. On this
    PCB, the inverted output of the latch is used, so the order of pages
    is reversed. (No problem as long as the memory dump is kept exactly
    in the way it is stored in the EPROM.)
    The latch can store a value of 4 bits. We adjust the number of
    significant bits by the size of the memory dump (16K, 32K, 64K).
*/
static WRITE8_DEVICE_HANDLER( write_cart_paged379i )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	// Bits: 011x xxxx xxxb bbbx
	// x = don't care, bbbb = bank
	set_paged379i_bank(cartridge, 15 - ((offset>>1) & 15));
}

/*
    Paged379i modules have one EPROM dump.
*/
static int assemble_paged379i(device_t *image)
{
	cartridge_t *cart;

	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for paged cartridge");
		return IMAGE_INIT_FAIL;
	}
	set_paged379i_bank(cart, 15);
	return IMAGE_INIT_PASS;
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
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    Read handler for the CPU address space of the cartridge
    Images for this area are found in the rom_sockets.
*/
static READ8Z_DEVICE_HANDLER( read_cart_pagedcru )
{
	/* device is pcb, owner is cartslot */
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if (cartridge->rom_ptr!=NULL)
	{
		*value = (cartridge->rom_ptr)[cartridge->rom_page*0x2000 + (offset & 0x1fff)];
	}
}

/*
    This type of cartridge does not support writing to the EPROM address
    space.
*/
static WRITE8_DEVICE_HANDLER( write_cart_pagedcru )
{
	return;
}

/*
    The CRU read handler. The CRU is a serial interface in the console.
    Using the CRU we can switch the banks in the SuperSpace cartridge.
    For documentation see the corresponding cru function above. Note we are
    using rom_page here.
*/
static READ8Z_DEVICE_HANDLER( read_cart_cru_paged )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	int page = cartridge->rom_page;
	if ((offset & 0xf800)==0x0800)
	{
		int bit = (offset & 0x001e)/2;
		if (bit != 0)
		{
			page = page-(bit/2);  // 4 page flags per 8 bits
		}
		*value = 1 << (page*2+1);
	}
}

static WRITE8_DEVICE_HANDLER( write_cart_cru_paged )
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	cartridge_t *cartridge = pcb->cartridge;

	if ((offset & 0xf800)==0x0800)
	{
		int bit = (offset & 0x001e)/2;
		if (data != 0 && bit > 0)
		{
			cartridge->rom_page = (bit-1)/2;
		}
	}
}

/*
    Pagedcru modules have one EPROM dump.
*/
static int assemble_pagedcru(device_t *image)
{
	cartridge_t *cart;

	cart = assemble_common(image);
	if (cart->rom_ptr==NULL)
	{
		logerror("Missing ROM for pagedcru cartridge");
		return IMAGE_INIT_FAIL;
	}
	cart->rom_page = 0;
	return IMAGE_INIT_PASS;
}

/*****************************************************************************
  Cartridge type: gramkracker
******************************************************************************/

static DEVICE_START(ti99_pcb_gramkracker)
{
	/* device is ti99_cartslot:cartridge:pcb */
//  printf("DEVICE_START(ti99_pcb_pagedcru), tag of device=%s\n", device->tag());
	set_pointers(device, get_index_from_tagname(device->owner())-1);
}

/*
    The GRAM Kracker contains one GROM and several buffered RAM chips
*/
static int assemble_gramkracker(device_t *image)
{
	cartridge_t *cart;
	int id,i;

	device_t *cartsys = image->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	cart = assemble_common(image);

	if (cart->grom_ptr==NULL)
	{
		logerror("Missing loader GROM for GRAM Kracker system");
		return IMAGE_INIT_FAIL;
	}

	if (cart->ram_size < 81920)
	{
		logerror("Missing or insufficient RAM for GRAM Kracker system");
		return IMAGE_INIT_FAIL;
	}

	// Get slot number
	id = get_index_from_tagname(image);
	cartslots->gk_slot = id-1;
	cartslots->gk_guest_slot = -1;
	// The first cartridge that is still plugged in is the "guest" of the
	// GRAM Kracker
	for (i=0; i < NUMBER_OF_CARTRIDGE_SLOTS; i++)
	{
		if (!slot_is_empty(cartsys, i) && (i != cartslots->gk_slot))
		{
			cartslots->gk_guest_slot = i;
			break;
		}
	}
	return IMAGE_INIT_PASS;
}

/*
    Unplugs the GRAM Kracker. The "guest" is still plugged in, but will be
    treated as normal.
*/
static int disassemble_gramkracker(device_t *cartslot)
{
	device_t *cartsys = cartslot->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	//int slotnumber = get_index_from_tagname(cartslot)-1;
	//assert(slotnumber>=0 && slotnumber<NUMBER_OF_CARTRIDGE_SLOTS);

	cartslots->gk_slot = -1;
	cartslots->gk_guest_slot = -1;

	return disassemble_std(cartslot);
}


/*****************************************************************************
  Device metadata
******************************************************************************/

/*
    Get the pointer to the GROM data from the cartridge. Called by the GROM.
*/
static UINT8 *get_grom_ptr(device_t *device)
{
	ti99_pcb_t *pcb = get_safe_pcb_token(device);
	return pcb->cartridge->grom_ptr;
}

static MACHINE_CONFIG_FRAGMENT(ti99_cart_common)
	MCFG_GROM_ADD_P( "grom_3", 3, get_grom_ptr, 0x0000, 0x1800, cart_grom_ready )
	MCFG_GROM_ADD_P( "grom_4", 4, get_grom_ptr, 0x2000, 0x1800, cart_grom_ready )
	MCFG_GROM_ADD_P( "grom_5", 5, get_grom_ptr, 0x4000, 0x1800, cart_grom_ready )
	MCFG_GROM_ADD_P( "grom_6", 6, get_grom_ptr, 0x6000, 0x1800, cart_grom_ready )
	MCFG_GROM_ADD_P( "grom_7", 7, get_grom_ptr, 0x8000, 0x1800, cart_grom_ready )
MACHINE_CONFIG_END

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

		case DEVINFO_PTR_MACHINE_CONFIG:
			info->machine_config = MACHINE_CONFIG_NAME(ti99_cart_common);
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

DEVICE_GET_INFO(ti99_cartridge_pcb_none)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_none);
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 empty cartridge");
			break;

		// Don't create GROMs
		case DEVINFO_PTR_MACHINE_CONFIG:
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_std)
{
	DEVICE_GET_INFO_CALL(ti99_cart_common);
}

DEVICE_GET_INFO(ti99_cartridge_pcb_paged)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_paged);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_paged;
			break;

		case TI99CARTINFO_FCT_6000_R:
			info->f = (genf *) read_cart_paged;
			break;

		case TI99CARTINFO_FCT_6000_W:
			info->f = (genf *) write_cart_paged;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 paged cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_minimem)
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

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 MiniMemory cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_super)
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
		case TI99CARTINFO_FCT_CRU_R:
			info->f = (genf *) read_cart_cru;
			break;
		case TI99CARTINFO_FCT_CRU_W:
			info->f = (genf *) write_cart_cru;
			break;

		// Don't create GROMs
		case DEVINFO_PTR_MACHINE_CONFIG:
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 SuperSpace cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_mbx)
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

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 MBX cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_paged379i)
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

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 paged379i cartridge pcb");
			break;

		default:
			DEVICE_GET_INFO_CALL(ti99_cart_common);
			break;
	}
}

DEVICE_GET_INFO(ti99_cartridge_pcb_pagedcru)
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

DEVICE_GET_INFO(ti99_cartridge_pcb_gramkracker)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(ti99_pcb_gramkracker);
			break;

		case TI99CART_FCT_ASSM:
			info->f = (genf *) assemble_gramkracker;
			break;

		case TI99CART_FCT_DISASSM:
			info->f = (genf *) disassemble_gramkracker;
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "TI99 GRAM Kracker system");
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
	cartslot_t *cart = get_safe_cartslot_token(device);

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
	device_t *pcbdev = cartslot_get_pcb(&image.device());
//  device_t *cartsys = image.device().owner();
//  ti99_multicart_state *cartslots = get_safe_token(cartsys);

	int result;

	if (pcbdev != NULL)
	{
		/* If we are here, we have a multicart. */
		ti99_pcb_t *pcb = get_safe_pcb_token(pcbdev);
		cartslot_t *cart = get_safe_cartslot_token(&image.device());

		/* try opening this as a multicart */
		// This line requires that cartslot_t be included in cartslot.h,
		// otherwise one cannot make use of multicart handling within such a
		// custom LOAD function.
		multicart_open_error me = multicart_open(image.device().machine().options(), image.filename(), image.device().machine().system().name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);

		// Now that we have loaded the image files, let the PCB put them all
		// together. This means we put the images in a structure which allows
		// for a quick access by the memory handlers. Every PCB defines an
		// own assembly method.
		if (me == MCERR_NONE)
			result = pcb->assemble(image);
		else
			fatalerror("Error loading multicart: %s", multicart_error_text(me));
	}
	else
	{
		fatalerror("Error loading multicart: no pcb found.");
	}
	return result;
}

/*
    This is called when the cartridge is unplugged (or the emulator is
    stopped).
*/
static DEVICE_IMAGE_UNLOAD( ti99_cartridge )
{
	device_t *pcbdev;
	//ti99_multicart_state *cartslots = get_safe_token(image.device().owner());

	if (downcast<legacy_device_base *>(&image.device())->token() == NULL)
	{
		// This means something went wrong during the pcb
		// identification (e.g. one of the cartridge files was not
		// found). We do not need to (and cannot) unload the cartridge.
		return;
	}
	pcbdev = cartslot_get_pcb(image);

	if (pcbdev != NULL)
	{
		ti99_pcb_t *pcb = get_safe_pcb_token(pcbdev);
		cartslot_t *cart = get_safe_cartslot_token(&image.device());

		if (cart->mc != NULL)
		{
			/* Remove pointers and de-big-endianize RAM contents. */
			pcb->disassemble(image);

			// Close the multicart; all RAM resources will be
			// written to disk
			multicart_close(image.device().machine().options(),cart->mc);
			cart->mc = NULL;

		}
	}
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

static READ8Z_DEVICE_HANDLER( ti99_cart_gk_rz );
static WRITE8_DEVICE_HANDLER( ti99_cart_gk_w );
static void cartridge_gram_kracker_writeg(device_t *cartsys, int offset, UINT8 data);
static void cartridge_gram_kracker_readg(device_t *device, UINT8 *value);
static READ8Z_DEVICE_HANDLER( ti99_cart_cru_gk_rz );
static WRITE8_DEVICE_HANDLER( ti99_cart_cru_gk_w );

/*
    Instantiation of a multicart system for the TI family.
*/
static DEVICE_START(ti99_multicart)
{
	int i;
	ti99_multicart_state *cartslots = get_safe_token(device);
	const ti99_multicart_config* cartconf = (const ti99_multicart_config*)get_config(device);

	cartslots->active_slot = 0;
	cartslots->next_free_slot = 0;

	for (i=0; i < NUMBER_OF_CARTRIDGE_SLOTS; i++)
	{
		cartslots->cartridge[i].pcb = NULL;
		clear_slot(device, i);
	}

	devcb_write_line ready = DEVCB_LINE(cartconf->ready);

	devcb_resolve_write_line(&cartslots->ready, &ready, device);

	cartslots->gk_slot = -1;
	cartslots->gk_guest_slot = -1;

	// The cartslot system is initialized now. The cartridges themselves
	// need to check whether their parts are available.
}

static DEVICE_STOP(ti99_multicart)
{
//  printf("DEVICE_STOP(ti99_multicart)\n");
}

static DEVICE_RESET(ti99_multicart)
{
	// Consider to propagate RESET to cartridges.
	// However, schematics do not reveal any pin for resetting a cartridge;
	// the reset line is an input used when plugging in a cartridge.

	ti99_multicart_state *cartslots = get_safe_token(device);
	int slotnumber = input_port_read(device->machine(), "CARTSLOT");
	cartslots->fixed_slot = slotnumber-1; /* auto = -1 */
}

/*
    Accesses the ROM regions of the cartridge for reading.
*/
READ8Z_DEVICE_HANDLER( gromportr_rz )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	int slot = cartslots->active_slot;
	cartridge_t *cart;

	// Handle GRAM Kracker
	// We could also consider to use a flag, but input_port_read uses
	// a hashtable, so this should not be too slow
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		ti99_cart_gk_rz(device, offset, value);
		return;
	}

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	// Same as above (GROM read)
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
//      printf("try it on slot %d\n", slot);
		ti99_pcb_t *pcbdef = get_safe_pcb_token(cart->pcb);
		(*pcbdef->read)(cart->pcb, offset, value);
	}
}

/*
    Accesses the ROM regions of the cartridge for writing.
*/
WRITE8_DEVICE_HANDLER( gromportr_w )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	int slot = cartslots->active_slot;
	cartridge_t *cart;

	// Handle GRAM Kracker
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
		ti99_cart_gk_w(device, offset, data);

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
		ti99_pcb_t *pcbdef = get_safe_pcb_token(cart->pcb);
		(*pcbdef->write)(cart->pcb, offset, data);
	}
}

/*
    Set the address pointer value or write to GRAM (if available).
*/
WRITE8_DEVICE_HANDLER(gromportg_w)
{
	int slot;
	cartridge_t *cartridge;
	ti99_multicart_state *cartslots = get_safe_token(device);

	// Set the cart slot.
	// 1001 1wbb bbbb bbr0

	cartridge_slot_set(device, (UINT8)((offset>>2) & 0x00ff));
	slot = cartslots->active_slot;

	// The selection mechanism of the console checks the different GROM address
	// bases, and when it finds different data, it concludes that there is
	// indeed a multi-cartridge facility. Empty slots return 0 at all
	// addresses.
	// Unfortunately, TI's cartridge selection mechanism has an undesirable
	// side-effect which we work around in this implementation.
	// When we have only one cartridge we would like to get only the option to
	// select this cartridge and not an option to switch to the next one (which
	// would immediately cycle back to cartridge 1). However, the console
	// routines always show the switch option when any two slots have different
	// content, and this is the case with only one cartridge (slot 2 will then
	// be filled with zeros).
	// So if next_free_slot==1, we have one cartridge in slot 0.
	// In that case we trick the OS to believe that the addressed
	// cartridge appears at all locations which causes it to assume a
	// standard single cartslot.

	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	// Handle the GRAM Kracker
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		cartridge_gram_kracker_writeg(device, offset, data);
		return;
	}

	if (slot < NUMBER_OF_CARTRIDGE_SLOTS)
	{
		cartridge = &cartslots->cartridge[slot];
		// Send the request to all mounted GROMs
		if (cartridge->pcb!=NULL)
		{
			for (int i=0; i < 5; i++)
			{
				ti99_pcb_t *pcbdef = get_safe_pcb_token(cartridge->pcb);
				if (pcbdef->grom[i]!=NULL)
					ti99grom_w(pcbdef->grom[i], offset, data);
			}
		}
	}
}

/*
    GROM access. Nota bene: We only react to the request if there is a GROM
    at the selected bank with the currently selected ID. If not, just do
    nothing; do not return a value.
    (We must not return a value since all GROMs are connected in parallel, so
    returning a 0, for instance, will spoil the processing of other GROMs.)
*/
READ8Z_DEVICE_HANDLER(gromportg_rz)
{
	int slot;
	cartridge_t *cartridge;
	ti99_multicart_state *cartslots = get_safe_token(device);

	// Set the cart slot. Note that the port must be adjusted for 16-bit
	// systems, i.e. it should be shifted left by 1.
	// 1001 1wbb bbbb bbr0

	cartridge_slot_set(device, (UINT8)((offset>>2) & 0x00ff));

	// Handle the GRAM Kracker
	// BTW, the GK does not have a readable address counter, but the console
	// GROMs will keep our addess counter up to date. That is
	// exactly what happens in the real machine.
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		if ((offset & 0x0002)==0) cartridge_gram_kracker_readg(device, value);
		return;
	}

	slot = cartslots->active_slot;

	// This fixes a minor issue: The selection mechanism of the console
	// concludes that there are multiple cartridges plugged in when it
	// checks the different GROM bases and finds different data. Empty
	// slots should return 0 at all addresses, but when we have only
	// one cartridge, this entails that the second cartridge is
	// different (all zeros), and so we get a meaningless selection
	// option between cartridge 1 and 1 and 1 ...
	//
	// So if next_free_slot==1, we have one cartridge in slot 0.
	// In that case we trick the OS to believe that the addressed
	// cartridge appears at all locations which causes it to assume a
	// standard single cartslot.

	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	if (slot < NUMBER_OF_CARTRIDGE_SLOTS)
	{
		// Select the cartridge which the multicart system is currently pointing at
		cartridge = &cartslots->cartridge[slot];
		// Send the request to all mounted GROMs on this PCB
		if (cartridge->pcb!=NULL)
		{
			for (int i=0; i < 5; i++)
			{
				ti99_pcb_t *pcbdef = get_safe_pcb_token(cartridge->pcb);
				if (pcbdef->grom[i]!=NULL)
					ti99grom_rz(pcbdef->grom[i], offset, value);
			}
		}
	}
}

/*
    CRU interface to cartridges, used only by SuperSpace and Pagedcru
    style cartridges.
    The SuperSpace has a CRU-based memory mapper. It is accessed via the
    cartridge slot on CRU base >0800.
*/
READ8Z_DEVICE_HANDLER( gromportc_rz )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	// Handle GRAM Kracker
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		ti99_cart_cru_gk_rz(device, offset, value);
		return;
	}

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
		ti99_pcb_t *pcbdef = get_safe_pcb_token(cart->pcb);
		if (pcbdef->cruread != NULL)
			(*pcbdef->cruread)(cart->pcb, offset, value);
	}
}

/*
    Write cartridge mapper CRU interface (SuperSpace, Pagedcru)
*/
WRITE8_DEVICE_HANDLER( gromportc_w )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	cartridge_t *cart;

	int slot = cartslots->active_slot;

	// Handle GRAM Kracker
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		ti99_cart_cru_gk_w(device, offset, data);
		return;
	}

	/* Sanity check. Higher slots are always empty. */
	if (slot >= NUMBER_OF_CARTRIDGE_SLOTS)
		return;

	/* Same as above (READ16). */
	if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
		slot=0;

	cart = &cartslots->cartridge[slot];

	if (!slot_is_empty(device, slot))
	{
		ti99_pcb_t *pcbdef = get_safe_pcb_token(cart->pcb);
		if (pcbdef->cruwrite != NULL)
			(*pcbdef->cruwrite)(cart->pcb, offset, data);
	}
}

/*****************************************************************************
   GramKracker support
   The 80 KiB of the GK are allocated as follows
   00000 - 01fff: GRAM 0
   02000 - 03fff: GRAM 1  (alternatively, the loader GROM appears here)
   04000 - 05fff: GRAM 2
   06000 - 0ffff: GRAM 3-7 (alternatively, the guest GROMs appear here)
   10000 - 11fff: RAM 1
   12000 - 13fff: RAM 2

   The GK emulates GROMs, but without a readable address counter. Thus it
   relies on the console GROMs being still in place (compare with the HSGPL).
   Also, the GK does not implement a GROM buffer.
******************************************************************************/

/*
    Reads from the GRAM space of the GRAM Kracker.
*/
static void cartridge_gram_kracker_readg(device_t *cartsys, UINT8 *value)
{
	// gk_slot is the slot where the GK module is plugged in
	// gk_guest_slot is the slot where the cartridge is plugged in

	cartridge_t *gromkrackerbox, *gkguestcart;
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	// Not null, since this is only called when there is really a GK module
	gromkrackerbox = &cartslots->cartridge[cartslots->gk_slot];

	if ((gromkrackerbox->grom_address & 0xe000) == 0x0000)
	{
		// GRAM 0. Only return a value if switch 2 is in GRAM0 position.
		if (get_gk_switch(cartsys, 2)==GK_GRAM0)
			*value = (gromkrackerbox->ram_ptr)[gromkrackerbox->grom_address];
	}

	if ((gromkrackerbox->grom_address & 0xe000) == 0x2000)
	{
		// If the loader is turned on, return loader contents.
		if (get_gk_switch(cartsys, 5)==GK_LDON)
		{
			// The only ROM contained in the GK box is the loader
			*value = gromkrackerbox->grom_ptr[gromkrackerbox->grom_address & 0x1fff];
		}
		else
		{
			// Loader off
			// GRAM 1. Only return a value if switch 3 is in GRAM12 position.
			if (get_gk_switch(cartsys, 3)==GK_GRAM12)
				*value = (gromkrackerbox->ram_ptr)[gromkrackerbox->grom_address];
		}
	}

	if ((gromkrackerbox->grom_address & 0xe000) == 0x4000)
	{
		// GRAM 2. Only return a value if switch 3 is in GRAM12 position.
		if (get_gk_switch(cartsys, 3)==GK_GRAM12)
			*value = (gromkrackerbox->ram_ptr)[gromkrackerbox->grom_address];
	}

	if (((gromkrackerbox->grom_address & 0xe000) == 0x6000) ||
		((gromkrackerbox->grom_address & 0x8000) == 0x8000))
	{
		// Cartridge space
		// When a cartridge is installed, it overrides the GK contents
		// but only if it has GROMs
		int cartwithgrom = FALSE;
		gkguestcart = &cartslots->cartridge[cartslots->gk_guest_slot];

		// Send the request to all mounted GROMs
		if (gkguestcart->pcb!=NULL)
		{
			for (int i=0; i < 5; i++)
			{
				ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
				if (pcbdef->grom[i]!=NULL)
				{
					ti99grom_rz(pcbdef->grom[i], 0x9800, value);
					cartwithgrom = TRUE;
				}
			}
		}
		if (!cartwithgrom && get_gk_switch(cartsys, 1)==GK_NORMAL)
		{
			*value = (gromkrackerbox->ram_ptr)[gromkrackerbox->grom_address];
		}
	}

	// The GK GROM emulation does not wrap at 8K boundaries.
	gromkrackerbox->grom_address = (gromkrackerbox->grom_address+1) & 0xffff;

	/* Reset the write address flipflop. */
	gromkrackerbox->waddr_LSB = FALSE;
}

/*
    Writes to the GRAM space of the GRAM Kracker.
*/
static void cartridge_gram_kracker_writeg(device_t *cartsys, int offset, UINT8 data)
{
	// gk_slot is the slot where the GK module is plugged in
	// gk_guest_slot is the slot where the cartridge is plugged in
	cartridge_t *gromkrackerbox;
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	// This is only called when there is really a GK module
	gromkrackerbox = &cartslots->cartridge[cartslots->gk_slot];

	if ((offset & 0x0002)==0x0002)
	{
		// Set address
		if (gromkrackerbox->waddr_LSB)
		{
			/* Accept low byte (2nd write) */
			gromkrackerbox->grom_address = (gromkrackerbox->grom_address & 0xff00) | data;
			gromkrackerbox->waddr_LSB = FALSE;
		}
		else
		{
			/* Accept high byte (1st write) */
			gromkrackerbox->grom_address = ((data<<8) & 0xff00) | (gromkrackerbox->grom_address & 0x00ff);
			gromkrackerbox->waddr_LSB = TRUE;
		}
	}
	else
	{
		if (get_gk_switch(cartsys, 4) != GK_WP)
		{
			// Write protection off
			if (   (((gromkrackerbox->grom_address & 0xe000)==0x0000) && get_gk_switch(cartsys, 2)==GK_GRAM0)
				|| (((gromkrackerbox->grom_address & 0xe000)==0x2000) && get_gk_switch(cartsys, 3)==GK_GRAM12 && (get_gk_switch(cartsys, 5)==GK_LDOFF))
				|| (((gromkrackerbox->grom_address & 0xe000)==0x4000) && get_gk_switch(cartsys, 3)==GK_GRAM12)
				|| (((gromkrackerbox->grom_address & 0xe000)==0x6000) && get_gk_switch(cartsys, 1)==GK_NORMAL && (cartslots->gk_guest_slot==-1))
				|| (((gromkrackerbox->grom_address & 0x8000)==0x8000) && get_gk_switch(cartsys, 1)==GK_NORMAL && (cartslots->gk_guest_slot==-1)))
			{
				(gromkrackerbox->ram_ptr)[gromkrackerbox->grom_address] = data;
			}
		}
		// The GK GROM emulation does not wrap at 8K boundaries.
		gromkrackerbox->grom_address = (gromkrackerbox->grom_address+1) & 0xffff;

		/* Reset the write address flipflop. */
		gromkrackerbox->waddr_LSB = FALSE;
	}
}

/*
    Reads from the RAM space of the GRAM Kracker.
*/
static READ8Z_DEVICE_HANDLER( ti99_cart_gk_rz )
{
	// gk_slot is the slot where the GK module is plugged in
	// gk_guest_slot is the slot where the cartridge is plugged in
	cartridge_t *gromkrackerbox, *gkguestcart;
	ti99_multicart_state *cartslots = get_safe_token(device);

	gromkrackerbox = &cartslots->cartridge[cartslots->gk_slot];

	if (cartslots->gk_guest_slot != -1)
	{
		gkguestcart = &cartslots->cartridge[cartslots->gk_guest_slot];
		// Legacy mode is not supported with the GK
		if (in_legacy_mode(device))
		{
			return;
		}
		//      printf("accessing cartridge in slot %d\n", slot);
		ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
		//      printf("address=%lx, offset=%lx\n", pcbdef, offset);
		(*pcbdef->read)(gkguestcart->pcb, offset, value);
		return;
	}
	else
	{
		if (get_gk_switch(device, 1)==GK_OFF) return;
		if (get_gk_switch(device, 4)==GK_BANK1)
		{
			*value = (gromkrackerbox->ram_ptr)[offset+0x10000];
		}
		else
		{
			if (get_gk_switch(device, 4)==GK_BANK2)
			{
				*value = (gromkrackerbox->ram_ptr)[offset+0x12000];
			}
			else
			{
				// Write protection on; auto-bank
				if (gromkrackerbox->ram_page==0)
				{
					*value = (gromkrackerbox->ram_ptr)[offset+0x10000];
				}
				else
				{
					*value = (gromkrackerbox->ram_ptr)[offset+0x12000];
				}
			}
		}
	}
}

/*
    Writes to the RAM space of the GRAM Kracker.
*/
static WRITE8_DEVICE_HANDLER( ti99_cart_gk_w )
{
	// gk_slot is the slot where the GK module is plugged in
	// gk_guest_slot is the slot where the cartridge is plugged in
	cartridge_t *gromkrackerbox, *gkguestcart;
	ti99_multicart_state *cartslots = get_safe_token(device);

	gromkrackerbox = &cartslots->cartridge[cartslots->gk_slot];

	if (cartslots->gk_guest_slot != -1)
	{
		gkguestcart = &cartslots->cartridge[cartslots->gk_guest_slot];
		// Legacy mode is not supported with the GK
		if (in_legacy_mode(device))
		{
			return;
		}

		ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
			(*pcbdef->write)(gkguestcart->pcb, offset, data);
	}
	else
	{
		if (get_gk_switch(device, 1)==GK_OFF)
			return;

		if (get_gk_switch(device, 4)==GK_BANK1)
		{
			(gromkrackerbox->ram_ptr)[offset + 0x10000] = data;
		}
		else
		{
			if (get_gk_switch(device, 4)==GK_BANK2)
			{
				(gromkrackerbox->ram_ptr)[offset + 0x12000] = data;
			}
			else
			{
				// Write protection on
				// This is handled like in Extended Basic
				gromkrackerbox->ram_page = (offset & 2)>>1;
			}
		}
	}
}

/*
    Read cartridge mapper CRU interface (Guest in GRAM Kracker)
*/
static READ8Z_DEVICE_HANDLER( ti99_cart_cru_gk_rz )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	cartridge_t *gkguestcart;

	if (cartslots->gk_guest_slot != -1)
	{
		gkguestcart = &cartslots->cartridge[cartslots->gk_guest_slot];
		ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
		if (pcbdef->cruread != NULL)
			(*pcbdef->cruread)(gkguestcart->pcb, offset, value);
	}
}

/*
    Write cartridge mapper CRU interface (Guest in GRAM Kracker)
*/
static WRITE8_DEVICE_HANDLER( ti99_cart_cru_gk_w )
{
	ti99_multicart_state *cartslots = get_safe_token(device);
	cartridge_t *gkguestcart;

	if (cartslots->gk_guest_slot != -1)
	{
		gkguestcart = &cartslots->cartridge[cartslots->gk_guest_slot];
		ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
		if (pcbdef->cruwrite != NULL)
			(*pcbdef->cruwrite)(gkguestcart->pcb, offset, data);
	}
}

/*****************************************************************************/
#define TI99_CARTRIDGE_SLOT(p)  MCFG_CARTSLOT_ADD(p) \
	MCFG_CARTSLOT_EXTENSION_LIST("rpk,bin") \
	MCFG_CARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE) \
	MCFG_CARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD) \
	MCFG_CARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED) \
	MCFG_CARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM) \
	MCFG_CARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER) \
	MCFG_CARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX) \
	MCFG_CARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I) \
	MCFG_CARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU) \
	MCFG_CARTSLOT_PCBTYPE(8, "gramkracker", TI99_CARTRIDGE_PCB_GRAMKRACKER) \
	MCFG_CARTSLOT_START(ti99_cartridge) \
	MCFG_CARTSLOT_LOAD(ti99_cartridge) \
	MCFG_CARTSLOT_UNLOAD(ti99_cartridge)

static MACHINE_CONFIG_FRAGMENT(ti99_multicart)
	TI99_CARTRIDGE_SLOT("cartridge1")
	TI99_CARTRIDGE_SLOT("cartridge2")
	TI99_CARTRIDGE_SLOT("cartridge3")
	TI99_CARTRIDGE_SLOT("cartridge4")
MACHINE_CONFIG_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_multicart##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 Multi-Cartridge Extender"
#define DEVTEMPLATE_FAMILY              "Multi-cartridge system"
#include "devtempl.h"

DEFINE_LEGACY_CART_SLOT_DEVICE(TI99_MULTICART, ti99_multicart);
