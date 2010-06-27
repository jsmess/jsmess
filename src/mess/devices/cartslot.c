/**********************************************************************

    cartslot.c

    Cartridge device

**********************************************************************/

#include <ctype.h>
#include "emu.h"
#include "cartslot.h"
#include "multcart.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

enum _process_mode
{
	PROCESS_CLEAR,
	PROCESS_LOAD
};
typedef enum _process_mode process_mode;


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cartslot_t *get_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == CARTSLOT);
	return (cartslot_t *) downcast<legacy_device_base *>(device)->token();
}


INLINE const cartslot_config *get_config(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == CARTSLOT);
	return (const cartslot_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE const cartslot_config *get_config_dev(const device_config *device)
{
	assert(device != NULL);
	assert(device->type() == CARTSLOT);
	return (const cartslot_config *)downcast<const legacy_device_config_base *>(device)->inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    load_cartridge
-------------------------------------------------*/

static int load_cartridge(device_image_interface *image, const rom_entry *romrgn, const rom_entry *roment, process_mode mode)
{
	const char *region;
	const char *type;
	UINT32 flags;
	offs_t offset, length, read_length, pos = 0, len;
	UINT8 *ptr;
	UINT8 clear_val;
	int datawidth, littleendian, i, j;
	running_device *cpu;

	region = ROMREGION_GETTAG(romrgn);
	offset = ROM_GETOFFSET(roment);
	length = ROM_GETLENGTH(roment);
	flags = ROM_GETFLAGS(roment);
	ptr = ((UINT8 *) memory_region(image->device().machine, region)) + offset;

	if (mode == PROCESS_LOAD)
	{
		/* must this be full size */
		if (flags & ROM_FULLSIZE)
		{
			if (image->length() != length)
				return IMAGE_INIT_FAIL;
		}

		/* read the ROM */
		pos = read_length = image->fread(ptr, length);

		/* reset the ROM to the initial point. */
		/* eventually, we could add a flag to allow the ROM to continue instead of restarting whenever a new cart region is present */
		image->fseek(0, SEEK_SET);

		/* do we need to mirror the ROM? */
		if (flags & ROM_MIRROR)
		{
			while(pos < length)
			{
				len = MIN(read_length, length - pos);
				memcpy(ptr + pos, ptr, len);
				pos += len;
			}
		}

		/* postprocess this region */
		type = ROMREGION_GETTAG(romrgn);
		littleendian = ROMREGION_ISLITTLEENDIAN(romrgn);
		datawidth = ROMREGION_GETWIDTH(romrgn) / 8;

		/* if the region is inverted, do that now */
		cpu = devtag_get_device(image->device().machine, type);
		if (cpu != NULL)
		{
			datawidth = device_memory(cpu)->space_config(AS_PROGRAM)->m_databus_width / 8;
			littleendian = (device_memory(cpu)->space_config()->m_endianness == ENDIANNESS_LITTLE);
		}

		/* swap the endianness if we need to */
#ifdef LSB_FIRST
		if (datawidth > 1 && !littleendian)
#else
		if (datawidth > 1 && littleendian)
#endif
		{
			for (i = 0; i < length; i += datawidth)
			{
				UINT8 temp[8];
				memcpy(temp, &ptr[i], datawidth);
				for (j = datawidth - 1; j >= 0; j--)
					ptr[i + j] = temp[datawidth - 1 - j];
			}
		}
	}

	/* clear out anything that remains */
	if (!(flags & ROM_NOCLEAR))
	{
		clear_val = (flags & ROM_FILL_FF) ? 0xFF : 0x00;
		memset(ptr + pos, clear_val, length - pos);
	}
	return IMAGE_INIT_PASS;
}



/*-------------------------------------------------
    process_cartridge
-------------------------------------------------*/

static int process_cartridge(device_image_interface *image, process_mode mode)
{
	const rom_source *source;
	const rom_entry *romrgn, *roment;
	int result = 0;

	for (source = rom_first_source(image->device().machine->gamedrv, image->device().machine->config); source != NULL; source = rom_next_source(image->device().machine->gamedrv, image->device().machine->config, source))
	{
		for (romrgn = rom_first_region(image->device().machine->gamedrv, source); romrgn != NULL; romrgn = rom_next_region(romrgn))
		{
			roment = romrgn + 1;
			while(!ROMENTRY_ISREGIONEND(roment))
			{
				if (ROMENTRY_GETTYPE(roment) == ROMENTRYTYPE_CARTRIDGE)
				{
					if (strcmp(roment->_hashdata,image->device().tag())==0)
					{
						result |= load_cartridge(image, romrgn, roment, mode);

						/* if loading failed in any cart region, stop loading */
						if (result)
							return result;
					}
				}
				roment++;
			}
		}
	}

	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
    cartslot_get_pcb
-------------------------------------------------*/

running_device *cartslot_get_pcb(running_device *device)
{
	cartslot_t *cart = get_token(device);
	return cart->pcb_device;
}


/*-------------------------------------------------
    cartslot_get_socket
-------------------------------------------------*/

void *cartslot_get_socket(running_device *device, const char *socket_name)
{
	cartslot_t *cart = get_token(device);
	device_image_interface *image = dynamic_cast<device_image_interface *>(device);
	void *result = NULL;

	if (cart->mc != NULL)
	{
		const multicart_socket *socket;
		for (socket = cart->mc->sockets; socket != NULL; socket = socket->next)
		{
			if (!strcmp(socket->id, socket_name))
				break;
		}
		result = socket ? socket->ptr : NULL;
	}
	else if (socket_name[0] == '\0')
	{
		result = image->ptr();
	}
	return result;
}

/*-------------------------------------------------
    cartslot_get_resource_length
-------------------------------------------------*/

int cartslot_get_resource_length(running_device *device, const char *socket_name)
{
	cartslot_t *cart = get_token(device);
	int result = 0;

	if (cart->mc != NULL)
	{
		const multicart_socket *socket;

		for (socket = cart->mc->sockets; socket != NULL; socket = socket->next)
		{
			if (!strcmp(socket->id, socket_name)) {
				break;
			}
		}
		if (socket != NULL)
			result = socket->resource->length;
	}
	else
		result = 0;

	return result;
}


/*-------------------------------------------------
    DEVICE_START( cartslot )
-------------------------------------------------*/

static DEVICE_START( cartslot )
{
	cartslot_t *cart = get_token(device);
	const cartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_START, use it */
	if (config->device_start != NULL)
	{
		(*config->device_start)(device);
		goto done;
	}

	/* find the PCB (if there is one) */
	cart->pcb_device = device->subdevice(TAG_PCB);

done:
	return;
}


/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( cartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( cartslot )
{
	int result;
	device_t *device = &image.device();
	cartslot_t *cart = get_token(device);
	const cartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_IMAGE_LOAD, use it */
	if (config->device_load != NULL)
		return (*config->device_load)(image);

	/* try opening this as if it were a multicart */
	multicart_open(image.filename(), device->machine->gamedrv->name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);
	if (cart->mc == NULL)
	{


		/* otherwise try the normal route */
		result = process_cartridge(&image, PROCESS_LOAD);
		if (result != IMAGE_INIT_PASS)
			return result;
	}

	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
    DEVICE_IMAGE_UNLOAD( cartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_UNLOAD( cartslot )
{
	device_t *device = &image.device();
	cartslot_t *cart = get_token(device);
	const cartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_IMAGE_UNLOAD, use it */
	if (config->device_unload != NULL)
	{
		(*config->device_unload)(image);
		return;
	}

	if (cart->mc != NULL)
	{
		multicart_close(cart->mc);
		cart->mc = NULL;
	}

	process_cartridge(&image, PROCESS_CLEAR);
}


/*-------------------------------------------------
    identify_pcb
-------------------------------------------------*/

static const cartslot_pcb_type *identify_pcb(device_image_interface &image)
{
	const cartslot_config *config = get_config(&image.device());
	astring pcb_name;
	const cartslot_pcb_type *pcb_type = NULL;
	multicart_t *mc;
	int i;

	if (image.software_entry() == NULL && image.exists())
	{
		/* try opening this as if it were a multicart */
		multicart_open_error me = multicart_open(image.filename(), image.device().machine->gamedrv->name, MULTICART_FLAGS_DONT_LOAD_RESOURCES, &mc);
		if (me == MCERR_NONE)
		{
			/* this was a multicart - read from it */
			astring_cpyc(&pcb_name, mc->pcb_type);
			multicart_close(mc);
		}
		else
		{
			if (me != MCERR_NOT_MULTICART)
				fatalerror("multicart error: %s", multicart_error_text(me));
			if (image.pcb() != NULL)
			{
				/* read from hash file */
				astring_cpyc(&pcb_name, image.pcb());
			}
		}

		/* look for PCB type with matching name */
		for (i = 0; (i < ARRAY_LENGTH(config->pcb_types)) && (config->pcb_types[i].name != NULL); i++)
		{
			if ((config->pcb_types[i].name[0] == '\0') || !strcmp(astring_c(&pcb_name), config->pcb_types[i].name))
			{
				pcb_type = &config->pcb_types[i];
				break;
			}
		}

		/* check for unknown PCB type */
		if ((mc != NULL) && (pcb_type == NULL))
			fatalerror("Unknown PCB type \"%s\"", astring_c(&pcb_name));
	}
	else
	{
		/* no device loaded; use the default */
		pcb_type = (config->pcb_types[0].name != NULL) ? &config->pcb_types[0] : NULL;
	}
	return pcb_type;
}

static machine_config *device_cfg = NULL;

static void memory_exit(running_machine *machine)
{
	machine_config_free(device_cfg);
}

/*-------------------------------------------------
    DEVICE_IMAGE_GET_DEVICES(cartslot)
-------------------------------------------------*/
static void add_device_with_subdevices(device_t *_owner, device_type _type, const char *_tag, UINT32 _clock)
{
	astring tempstring;
	device_list *devlist = &_owner->machine->devicelist;	
	const machine_config *config = _owner->machine->config;
	
	device_config *devconfig = _type(*config, _owner->subtag(tempstring,_tag), &_owner->baseconfig(), _clock);
	
	running_device *dev = devlist->append(_owner->subtag(tempstring,_tag), devconfig->alloc_device(*_owner->machine));
	const machine_config_token *tokens = dev->machine_config_tokens();
	if (tokens != NULL) 
    {		
        device_cfg = machine_config_alloc_owner(tokens,&dev->baseconfig());
        for (const device_config *config_dev = device_cfg->devicelist.first(); config_dev != NULL; config_dev = config_dev->next())
        {
			devlist->append(config_dev->tag(), config_dev->alloc_device(*_owner->machine));
        }
		add_exit_callback(_owner->machine, memory_exit);        
    }
}

static DEVICE_IMAGE_GET_DEVICES(cartslot)
{
	const cartslot_pcb_type *pcb_type;
	device_t *device = &image.device();

	pcb_type = identify_pcb(image);
	if (pcb_type != NULL)
	{
		add_device_with_subdevices(device,pcb_type->devtype,TAG_PCB,0);
	}
}


/*-------------------------------------------------
    DEVICE_GET_INFO( cartslot )
-------------------------------------------------*/

DEVICE_GET_INFO( cartslot )
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(cartslot_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = sizeof(cartslot_config); break;
		case DEVINFO_INT_IMAGE_TYPE:				info->i = IO_CARTSLOT; break;
		case DEVINFO_INT_IMAGE_READABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_RESET_ON_LOAD:		info->i = 1; break;
		case DEVINFO_INT_IMAGE_MUST_BE_LOADED:
			if ( device && downcast<const legacy_image_device_config_base *>(device)->inline_config()) {
				info->i = get_config_dev(device)->must_be_loaded;
			} else {
				info->i = 0;
			}
			break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(cartslot);					break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(cartslot);		break;
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(cartslot);		break;
		case DEVINFO_FCT_IMAGE_GET_DEVICES:			info->f = (genf *) DEVICE_IMAGE_GET_DEVICES_NAME(cartslot);	break;
		case DEVINFO_FCT_IMAGE_PARTIAL_HASH:
			if ( device && downcast<const legacy_image_device_config_base *>(device)->inline_config() && get_config_dev(device)->device_partialhash) {
				info->f = (genf *) get_config_dev(device)->device_partialhash;
			} else {
				info->f = NULL;
			}
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "Cartslot"); break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Cartslot"); break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:
			if ( device && downcast<const legacy_image_device_config_base *>(device)->inline_config() && get_config_dev(device)->extensions )
			{
				strcpy(info->s, get_config_dev(device)->extensions);
			}
			else
			{
				strcpy(info->s, "bin");
			}
			break;
		case DEVINFO_STR_IMAGE_INTERFACE:
			if ( device && downcast<const legacy_image_device_config_base *>(device)->inline_config() && get_config_dev(device)->interface )
			{
				strcpy(info->s, get_config_dev(device)->interface );
			}
			break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(CARTSLOT, cartslot);
