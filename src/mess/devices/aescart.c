/**********************************************************************

    aescart.c

    Neo-Geo AES cartridge management
    R. Belmont, 2009

    Based on ti99cart.c by Michael Zapf

    We leave the support in place for multiple slots in case MAME
    uses this at some future point for MVS multi-cart.

*********************************************************************/
#include "emu.h"
#include "aescart.h"
#include "cartslot.h"
#include "machine/ti99_4x.h"
#include "multcart.h"
#include "includes/neogeo.h"

typedef int assmfct(running_machine *machine, running_device *);

enum
{
	AESCART_FCT_ASSM = DEVINFO_FCT_DEVICE_SPECIFIC,
	AESCART_FCT_DISASSM
};

struct _aes_multicart_t
{
	/* Reserves space for all cartridges. This is also used in the legacy
       cartridge system, but only for slot 0. */
	aescartridge_t cartridge[AES_NUMBER_OF_CARTRIDGE_SLOTS];

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
};
typedef struct _aes_multicart_t aes_multicart_t;

#define AUTO -1

/* Access to the pcb. Contained in the token of the pcb instance. */
struct _aes_pcb_t
{
	/* Link up to the cartridge structure which contains this pcb. */
	aescartridge_t *cartridge;

	/* Function to assemble this cartridge. */
	assmfct	*assemble;

	/* Function to disassemble this cartridge. */
	assmfct	*disassemble;
};
typedef struct _aes_pcb_t aes_pcb_t;

/*
    Find the index of the cartridge name. We assume the format
    <name><number>, i.e. the number is the longest string from the right
    which can be interpreted as a number.
*/
static int get_index_from_tagname(running_device *image)
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
static aescartridge_t *assemble_common(running_machine *machine, running_device *cartslot)
{
	/* Pointer to the cartridge structure. */
	aescartridge_t *cartridge;
	running_device *cartsys = cartslot->owner;
	aes_multicart_t *cartslots = (aes_multicart_t *)cartsys->token;
	UINT8 *socketcont, *romrgn;
	int reslength, i, blockofs;
	char sprname1[16], sprname2[16];

	int slotnumber = get_index_from_tagname(cartslot)-1;
	assert(slotnumber>=0 && slotnumber<AES_NUMBER_OF_CARTRIDGE_SLOTS);

	/* There is a cartridge in this slot, check the maximum slot number. */
	if (cartslots->next_free_slot <= slotnumber)
	{
		cartslots->next_free_slot = slotnumber+1;
	}
	cartridge = &cartslots->cartridge[slotnumber];

	// check for up to 4 program ROMs
	romrgn = (UINT8 *)memory_region(machine, "maincpu");
	blockofs = 0;
	for (i = 0; i < 4; i++)
	{
		sprintf(sprname1, "p%d", i+1);

		socketcont = (UINT8*)cartslot_get_socket(cartslot, sprname1);
		reslength = cartslot_get_resource_length(cartslot, sprname1);

		if (socketcont != NULL)
		{
			memcpy(romrgn+blockofs, socketcont, reslength);
			blockofs += reslength;
		}
	}

	// 1 m1 ROM
	socketcont = (UINT8*)cartslot_get_socket(cartslot, "m1");
	reslength = cartslot_get_resource_length(cartslot, "m1");
	if (socketcont != NULL)
	{
		romrgn = (UINT8 *)memory_region(machine, "audiocpu");

		memcpy(romrgn, socketcont, reslength);
		// mirror (how does this really work?)
		memcpy(romrgn+0x10000, socketcont, reslength);
	}

	// up to 8 YM sample ROMs
	romrgn = (UINT8 *)memory_region(machine, "ymsnd");
	blockofs = 0;
	for (i = 0; i < 8; i++)
	{
		sprintf(sprname1, "v1%d", i+1);

		socketcont = (UINT8*)cartslot_get_socket(cartslot, sprname1);
		reslength = cartslot_get_resource_length(cartslot, sprname1);

		if (socketcont != NULL)
		{
			memcpy(romrgn+blockofs, socketcont, reslength);
			blockofs += reslength;
		}
	}

	// up to 8 YM delta-T sample ROMs
	romrgn = (UINT8 *)memory_region(machine, "ymsnd.deltat");
	blockofs = 0;
	for (i = 0; i < 8; i++)
	{
		sprintf(sprname1, "v2%d", i+1);

		socketcont = (UINT8*)cartslot_get_socket(cartslot, sprname1);
		reslength = cartslot_get_resource_length(cartslot, sprname1);

		if (socketcont != NULL)
		{
			memcpy(romrgn+blockofs, socketcont, reslength);
			blockofs += reslength;
		}
	}

	// 1 s1 ROM
	socketcont = (UINT8*)cartslot_get_socket(cartslot, "s1");
	reslength = cartslot_get_resource_length(cartslot, "s1");
	if (socketcont != NULL)
	{
		romrgn = (UINT8 *)memory_region(machine, "fixed");

		memcpy(romrgn, socketcont, reslength);
	}

	// up to 8 sprite ROMs in byte-interleaved pairs
	romrgn = (UINT8 *)memory_region(machine, "sprites");
	blockofs = 0;
	for (i = 0; i < 8; i+=2)
	{
		UINT8 *spr1, *spr2;
		int j;

		sprintf(sprname1, "c%d", i+1);
		sprintf(sprname2, "c%d", i+2);

		spr1 = (UINT8*)cartslot_get_socket(cartslot, sprname1);
		spr2 = (UINT8*)cartslot_get_socket(cartslot, sprname2);
		reslength = cartslot_get_resource_length(cartslot, sprname1);

		if ((spr1) && (spr2))
		{
			for (j = 0; j < reslength; j++)
			{
				romrgn[blockofs + (j*2)] = spr1[j];
				romrgn[blockofs + (j*2) + 1] = spr2[j];
			}

			blockofs += reslength*2;
		}
	}

	return cartridge;
}

static void set_pointers(running_device *pcb, int index)
{
	running_device *cartsys = pcb->owner->owner;
	aes_multicart_t *cartslots = (aes_multicart_t *)cartsys->token;
	aes_pcb_t *pcb_def = (aes_pcb_t *)pcb->token;

	pcb_def->assemble = (assmfct *)pcb->get_config_fct(AESCART_FCT_ASSM);
	pcb_def->disassemble = (assmfct *)pcb->get_config_fct(AESCART_FCT_DISASSM);

	pcb_def->cartridge = &cartslots->cartridge[index];
	pcb_def->cartridge->pcb = pcb;
}

/*****************************************************************************
  Cartridge type: None
    This PCB device is just a pseudo device; the legacy mode is handled
    by dedicated functions.
******************************************************************************/
static DEVICE_START(aes_pcb_none)
{
	/* device is aes_cartslot:cartridge:pcb */
//  printf("DEVICE_START(aes_pcb_none), tag of device=%s\n", device->tag());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*****************************************************************************
  Cartridge type: Standard
    Most cartridges are built in this type. This includes word-swapped program,
    z80 program, YM samples, delta-T samples, sprites, and sfix.
******************************************************************************/

static DEVICE_START(aes_pcb_std)
{
	/* device is aes_cartslot:cartridge:pcb */
//  printf("DEVICE_START(aes_pcb_std), tag of device=%s\n", device->tag());
	set_pointers(device, get_index_from_tagname(device->owner)-1);
}

/*
    The standard cartridge assemble routine. We just call the common
    function here.
*/
static int assemble_std(running_machine *machine, running_device *image)
{
//  aescartridge_t *cart;
//  printf("assemble_std, %s\n", image->tag);
	/*cart = */assemble_common(machine, image);

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
//  int slotnumber;
//  int i;
//  aescartridge_t *cart;
//  running_device *cartsys = image->owner;
//  aes_multicart_t *cartslots = (aes_multicart_t *)cartsys->token;

//  slotnumber = get_index_from_tagname(image)-1;
//  printf("Disassemble cartridge %d\n", slotnumber);
#if 0
	/* Search the highest remaining cartridge. */
	cartslots->next_free_slot = 0;
	for (i=AES_NUMBER_OF_CARTRIDGE_SLOTS-1; i >= 0; i--)
	{
		if (i != slotnumber)
		{
			if (0) //!slot_is_empty(cartsys, i))    (for future use?)
			{
				cartslots->next_free_slot = i+1;
//              printf("Setting new next_free_slot to %d\n", cartslots->next_free_slot);
				break;
			}
		}
	}
#endif
	/* Do we have RAM? If so, swap the bytes (undo the BIG_ENDIANIZE) */
//  cart = &cartslots->cartridge[slotnumber];

//  clear_slot(cartsys, slotnumber);

	return INIT_PASS;
}

/*****************************************************************************
  Device metadata
******************************************************************************/

static DEVICE_GET_INFO(aes_cart_common)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:
			info->i = sizeof(aes_pcb_t);
			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:
			info->i = 0;
			break;
		case DEVINFO_INT_CLASS:
			info->i = DEVICE_CLASS_PERIPHERAL;
			break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(aes_pcb_std);
			break;
		case DEVINFO_FCT_STOP:
			/* Nothing */
			break;
		case DEVINFO_FCT_RESET:
			/* Nothing */
			break;

		case AESCART_FCT_ASSM:
			info->f = (genf *) assemble_std;
			break;

		case AESCART_FCT_DISASSM:
			info->f = (genf *) disassemble_std;
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:
			strcpy(info->s, "AES standard cartridge pcb");
			break;
		case DEVINFO_STR_FAMILY:
			strcpy(info->s, "AES cartridge pcb");
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

static DEVICE_GET_INFO(aes_cartridge_pcb_none)
{
	switch(state)
	{
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(aes_pcb_none);
			break;

		case DEVINFO_STR_NAME:
			strcpy(info->s, "AES empty cartridge");
			break;

		default:
			DEVICE_GET_INFO_CALL(aes_cart_common);
			break;
	}
}

DEVICE_GET_INFO(aes_cartridge_pcb_std)
{
	DEVICE_GET_INFO_CALL(aes_cart_common);
}

/*****************************************************************************
  The cartridge handling of the multi-cartridge system.
  Every cartridge contains a PCB device. The memory handlers delegate the calls
  to the respective handlers of the cartridges.
******************************************************************************/
/*
    Initialize a cartridge. Each cartridge contains a PCB device.
*/
static DEVICE_START( aes_cartridge )
{
	cartslot_t *cart = (cartslot_t *)device->token;

	/* find the PCB device */
	cart->pcb_device = device->subdevice(TAG_PCB);
}

// handle protected carts
static void install_protection(running_device* image)
{
	neogeo_state *state = (neogeo_state *)image->machine->driver_data;

	if(image_get_feature(image) == NULL)
		return;

	if(strcmp(image_get_feature(image),"fatfury2_prot") == 0)
	{
		fatfury2_install_protection(image->machine);
		logerror("Installed Fatal Fury 2 protection\n");
	}
	if(strcmp(image_get_feature(image),"mslug3_crypt") == 0)
	{
		state->fixed_layer_bank_type = 1;
		kof99_neogeo_gfx_decrypt(image->machine, 0xad);
		logerror("Decrypted Metal Slug 3 graphics\n");
	}
	if(strcmp(image_get_feature(image),"matrim_crypt") == 0)
	{
		matrim_decrypt_68k(image->machine);
		neo_pcm2_swap(image->machine, 1);
		state->fixed_layer_bank_type = 2;
		neogeo_cmc50_m1_decrypt(image->machine);
		kof2000_neogeo_gfx_decrypt(image->machine, 0x6a);
		logerror("Decrypted Matrimelee code, sound and graphics\n");
	}
	if(strcmp(image_get_feature(image),"svc_crypt") == 0)
	{
		svc_px_decrypt(image->machine);
		neo_pcm2_swap(image->machine, 3);
		state->fixed_layer_bank_type = 2;
		neogeo_cmc50_m1_decrypt(image->machine);
		kof2000_neogeo_gfx_decrypt(image->machine, 0x57);
		install_pvc_protection(image->machine);
	}
	if(strcmp(image_get_feature(image),"samsho5_crypt") == 0)
	{
		samsho5_decrypt_68k(image->machine);
		neo_pcm2_swap(image->machine, 4);
		state->fixed_layer_bank_type = 1;
		neogeo_cmc50_m1_decrypt(image->machine);
		kof2000_neogeo_gfx_decrypt(image->machine, 0x0f);
	}
}

/*
    Load the cartridge image files. Apart from reading, we set pointers
    to the image files so that during runtime we do not need search
    operations.
*/
static DEVICE_IMAGE_LOAD( aes_cartridge )
{
	running_device *pcbdev = cartslot_get_pcb(image);
	aes_pcb_t *pcb;
	cartslot_t *cart;
	multicart_open_error me;
	UINT32 size;
	running_device* ym = devtag_get_device(image->machine,"ymsnd");

	// first check software list
	if(image_software_entry(image) != NULL)
	{
		// create memory regions
		size = image_get_software_region_length(image,"maincpu");
		memory_region_free(image->machine,"maincpu");
		memory_region_alloc(image->machine,"maincpu",size,0);
		memcpy(memory_region(image->machine,"maincpu"),image_get_software_region(image,"maincpu"),size);
		size = image_get_software_region_length(image,"fixed");
		memory_region_free(image->machine,"fixed");
		memory_region_alloc(image->machine,"fixed",size,ROMREGION_ERASEFF);
		memcpy(memory_region(image->machine,"fixed"),image_get_software_region(image,"fixed"),size);
		size = image_get_software_region_length(image,"audiocpu");
		memory_region_free(image->machine,"audiocpu");
		memory_region_alloc(image->machine,"audiocpu",size,ROMREGION_ERASEFF);
		memcpy(memory_region(image->machine,"audiocpu"),image_get_software_region(image,"audiocpu"),size);
		size = image_get_software_region_length(image,"ymsnd");
		memory_region_free(image->machine,"ymsnd");
		memory_region_alloc(image->machine,"ymsnd",size,ROMREGION_ERASEFF);
		memcpy(memory_region(image->machine,"ymsnd"),image_get_software_region(image,"ymsnd"),size);
		if(image_get_software_region(image,"ymsnd.deltat") != NULL)
		{
			size = image_get_software_region_length(image,"ymsnd.deltat");
			memory_region_free(image->machine,"ymsnd.deltat");
			memory_region_alloc(image->machine,"ymsnd.deltat",size,ROMREGION_ERASEFF);
			memcpy(memory_region(image->machine,"ymsnd.deltat"),image_get_software_region(image,"ymsnd.deltat"),size);
		}
		else
			memory_region_free(image->machine,"ymsnd.deltat");  // removing the region will fix sound glitches in non-Delta-T games
		ym->reset();
		size = image_get_software_region_length(image,"sprites");
		memory_region_free(image->machine,"sprites");
		memory_region_alloc(image->machine,"sprites",size,ROMREGION_ERASEFF);
		memcpy(memory_region(image->machine,"sprites"),image_get_software_region(image,"sprites"),size);
		if(image_get_software_region(image,"audiocrypt") != NULL)  // encrypted Z80 code
		{
			size = image_get_software_region_length(image,"audiocrypt");
			memory_region_alloc(image->machine,"audiocrypt",size,ROMREGION_ERASEFF);
			memcpy(memory_region(image->machine,"audiocrypt"),image_get_software_region(image,"audiocrypt"),size);
		}
		
		// setup cartridge ROM area
		memory_install_read_bank(cputag_get_address_space(image->machine,"maincpu",ADDRESS_SPACE_PROGRAM),0x000080,0x0fffff,0,0,"cart_rom");
		memory_set_bankptr(image->machine,"cart_rom",&memory_region(image->machine,"maincpu")[0x80]);
		
		// handle possible protection
		install_protection(image);
		
		return INIT_PASS;
	}

	if (pcbdev == NULL)
		fatalerror("Error loading multicart: no pcb found.");

	/* If we are here, we have a multicart. */
	pcb = (aes_pcb_t *)pcbdev->token;
	cart = (cartslot_t *)image->token;

	/* try opening this as a multicart */
	/* This line requires that cartslot_t be included in cartslot.h,
    otherwise one cannot make use of multicart handling within such a
    custom LOAD function. */
	me = multicart_open(image_filename(image), image->machine->gamedrv->name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);

	/* Now that we have loaded the image files, let the PCB put them all
    together. This means we put the images in a structure which allows
    for a quick access by the memory handlers. Every PCB defines an
    own assembly method. */
    if (me != MCERR_NONE)
		fatalerror("Error loading multicart: %s", multicart_error_text(me));

	return pcb->assemble(pcbdev->machine, image);
}

/*
    This is called when the cartridge is unplugged (or the emulator is
    stopped).
*/
static DEVICE_IMAGE_UNLOAD( aes_cartridge )
{
	running_device *pcbdev;

	if (image->token == NULL)
	{
		/* This means something went wrong during the pcb
           identification (e.g. one of the cartridge files was not
           found). We do not need to (and cannot) unload
           the cartridge. */
		return;
	}
	pcbdev = cartslot_get_pcb(image);

	if (pcbdev != NULL)
	{
		aes_pcb_t *pcb = (aes_pcb_t *)pcbdev->token;
		cartslot_t *cart = (cartslot_t *)image->token;

		//  printf("unload\n");
		if (cart->mc != NULL)
		{
			/* Remove pointers and de-big-endianize RAM contents. */
			pcb->disassemble(pcbdev->machine, image);

			/* Close the multicart; all RAM resources will be
               written to disk */
			multicart_close(cart->mc);
			cart->mc = NULL;
		}
//      else
//          fatalerror("Lost pointer to multicart in cartridge. Report bug.");
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
/*
    Instantiation of a multicart system for the Neo Geo AES.
*/
static DEVICE_START(aes_multicart)
{
	int i;
//  printf("DEVICE_START(aes_multicart)\n");
	aes_multicart_t *cartslots = (aes_multicart_t *)device->token;

	/* Save this in the shortcut; we don't want to look for it each time
       that we have a memory access. And currently we do not plan for
       multiple multicart instances. */
	cartslots->active_slot = 0;
	cartslots->next_free_slot = 0;

	for (i=0; i < AES_NUMBER_OF_CARTRIDGE_SLOTS; i++)
	{
		cartslots->cartridge[i].pcb = NULL;
	}

	cartslots->multi_slots = 0;

	/* The cartslot system is initialized now. The cartridges themselves
       need to check whether their parts are available. */
}

static DEVICE_STOP(aes_multicart)
{
//  printf("DEVICE_STOP(aes_multicart)\n");
}

static MACHINE_DRIVER_START(aes_multicart)
	MDRV_CARTSLOT_ADD("cartridge1")
	MDRV_CARTSLOT_EXTENSION_LIST("rpk,bin")
	MDRV_CARTSLOT_PCBTYPE(0, "none", AES_CARTRIDGE_PCB_NONE)
	MDRV_CARTSLOT_PCBTYPE(1, "standard", AES_CARTRIDGE_PCB_STD)

	MDRV_CARTSLOT_START(aes_cartridge)
	MDRV_CARTSLOT_LOAD(aes_cartridge)
	MDRV_CARTSLOT_UNLOAD(aes_cartridge)
	MDRV_CARTSLOT_INTERFACE("aes_cart")
	MDRV_CARTSLOT_MANDATORY
MACHINE_DRIVER_END


DEVICE_GET_INFO(aes_multicart)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data --- */
		case DEVINFO_PTR_MACHINE_CONFIG:
			info->machine_config = MACHINE_DRIVER_NAME(aes_multicart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:
			strcpy(info->s, "AES Cartridge handler");
			break;

		case DEVINFO_INT_TOKEN_BYTES: /* private storage, automatically allocated */
			info->i = sizeof(aes_multicart_t);
			break;
		case DEVINFO_INT_CLASS:
			info->i = DEVICE_CLASS_PERIPHERAL;
			break;

		case DEVINFO_STR_FAMILY:
			strcpy(info->s, "Cartridge slot");
			break;
		case DEVINFO_STR_VERSION:
			strcpy(info->s, "1.0");
			break;
		case DEVINFO_STR_SOURCE_FILE:
			strcpy(info->s, __FILE__);
			break;
		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:
			info->start = DEVICE_START_NAME(aes_multicart);
			break;
		case DEVINFO_FCT_STOP:
			info->stop = DEVICE_STOP_NAME(aes_multicart);
			break;
		case DEVINFO_FCT_RESET:
			/* Nothing */
			break;
	}
}

