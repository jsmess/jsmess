/**********************************************************************

    gromport.c

*********************************************************************/

#include "emu.h"
#include "gromport.h"
#include "grom.h"
#include "pool.h"
#include "unzip.h"
#include "corestr.h"
#include "xmlfile.h"
#include "hash.h"
#include "machine/ram.h"

/* start of multicart */
#define TAG_PCB		"pcb"

DECLARE_LEGACY_IMAGE_DEVICE(MULTICARTSLOT, multicartslot);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum _multicart_load_flags
{
	MULTICART_FLAGS_DONT_LOAD_RESOURCES	= 0x00,
	MULTICART_FLAGS_LOAD_RESOURCES		= 0x01
};
typedef enum _multicart_load_flags multicart_load_flags;

enum _multicart_resource_type
{
	MULTICART_RESOURCE_TYPE_INVALID,
	MULTICART_RESOURCE_TYPE_ROM,
	MULTICART_RESOURCE_TYPE_RAM
};
typedef enum _multicart_resource_type multicart_resource_type;

typedef struct _multicart_resource multicart_resource;
struct _multicart_resource
{
	const char *				id;
	const char *				filename;
	multicart_resource *		next;
	multicart_resource_type		type;
	UINT32						length;
	void *						ptr;
};


typedef struct _multicart_socket multicart_socket;
struct _multicart_socket
{
	const char *				id;
	multicart_socket *			next;
	const multicart_resource *	resource;
	void *						ptr;
};


typedef struct _multicart_private multicart_private;

typedef struct _multicart_t multicart_t;
struct _multicart_t
{
	const multicart_resource *	resources;
	const multicart_socket *	sockets;
	const char *				pcb_type;
	const char *				gamedrv_name; /* need this to find the path to the nvram files */
	multicart_private *			data;
};

typedef struct _multicartslot_t multicartslot_t;
struct _multicartslot_t
{
	device_t *pcb_device;
	multicart_t *mc;
};


typedef struct _multicartslot_pcb_type multicartslot_pcb_type;
struct _multicartslot_pcb_type
{
	const char *					name;
	device_type						devtype;
};


typedef struct _multicartslot_config multicartslot_config;
struct _multicartslot_config
{
	const char *					extensions;
	device_start_func				device_start;
	device_image_load_func			device_load;
	device_image_unload_func		device_unload;
	multicartslot_pcb_type			pcb_types[16];
};

enum _multicart_open_error
{
	MCERR_NONE,
	MCERR_NOT_MULTICART,
	MCERR_CORRUPT,
	MCERR_OUT_OF_MEMORY,
	MCERR_XML_ERROR,
	MCERR_INVALID_FILE_REF,
	MCERR_ZIP_ERROR,
	MCERR_MISSING_RAM_LENGTH,
	MCERR_INVALID_RAM_SPEC,
	MCERR_UNKNOWN_RESOURCE_TYPE,
	MCERR_INVALID_RESOURCE_REF,
	MCERR_INVALID_FILE_FORMAT,
	MCERR_MISSING_LAYOUT,
	MCERR_NO_PCB_OR_RESOURCES
};
typedef enum _multicart_open_error multicart_open_error;

const char *multicart_error_text(multicart_open_error error);

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* opens a multicart */
multicart_open_error multicart_open(emu_options &options, const char *filename, const char *drvname, multicart_load_flags load_flags, multicart_t **cart);

/* closes a multicart */
void multicart_close(emu_options &options, multicart_t *cart);

/* accesses the PCB associated with this cartslot */
device_t *cartslot_get_pcb(device_t *device);

/* accesses a particular socket */
void *cartslot_get_socket(device_t *device, const char *socket_name);

/* accesses a particular socket; gets the length of the associated resource */
int cartslot_get_resource_length(device_t *device, const char *socket_name);

#define DECLARE_LEGACY_CART_SLOT_DEVICE(name, basename) _DECLARE_LEGACY_DEVICE(name, basename, basename##_device, legacy_cart_slot_device_base)
#define DEFINE_LEGACY_CART_SLOT_DEVICE(name, basename) _DEFINE_LEGACY_DEVICE(name, basename, basename##_device, legacy_cart_slot_device_base)

#define MCFG_MULTICARTSLOT_ADD(_tag)										\
	MCFG_DEVICE_ADD(_tag, MULTICARTSLOT, 0)									\

#define MCFG_MULTICARTSLOT_MODIFY(_tag)										\
	MCFG_DEVICE_MODIFY(_tag)									\

#define MCFG_MULTICARTSLOT_PCBTYPE(_index, _pcb_type_name, _pcb_devtype)			\
	MCFG_DEVICE_CONFIG_DATAPTR_ARRAY_MEMBER(multicartslot_config, pcb_types, _index, multicartslot_pcb_type, name, _pcb_type_name) \
	MCFG_DEVICE_CONFIG_DATAPTR_ARRAY_MEMBER(multicartslot_config, pcb_types, _index, multicartslot_pcb_type, devtype, _pcb_devtype)

#define MCFG_MULTICARTSLOT_START(_start)										\
	MCFG_DEVICE_CONFIG_DATAPTR(multicartslot_config, device_start, DEVICE_START_NAME(_start))

#define MCFG_MULTICARTSLOT_LOAD(_load)										\
	MCFG_DEVICE_CONFIG_DATAPTR(multicartslot_config, device_load, DEVICE_IMAGE_LOAD_NAME(_load))

#define MCFG_MULTICARTSLOT_UNLOAD(_unload)									\
	MCFG_DEVICE_CONFIG_DATAPTR(multicartslot_config, device_unload, DEVICE_IMAGE_UNLOAD_NAME(_unload))

#define MCFG_MULTICARTSLOT_EXTENSION_LIST(_extensions)						\
	MCFG_DEVICE_CONFIG_DATAPTR(multicartslot_config, extensions, _extensions)


// ======================> device_cart_slot_interface

// class representing interface-specific live cart_slot
class device_cart_slot_interface : public device_interface
{
public:
	// construction/destruction
	device_cart_slot_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_cart_slot_interface();
};


// ======================> legacy_cart_slot_device

// legacy_cart_slot_device is a legacy_device_base with a cart_slot interface
class legacy_cart_slot_device_base :	public legacy_device_base,
										public device_cart_slot_interface
{
protected:
	// construction/destruction
	legacy_cart_slot_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, UINT32 clock, device_get_config_func get_config);

public:
	using legacy_device_base::get_legacy_int;
	using legacy_device_base::get_legacy_fct;
	using legacy_device_base::get_legacy_ptr;

	// device_cart_slot_interface overrides
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _multicart_private
{
	object_pool *	pool;
	zip_file *		zip;
};


typedef struct _multicart_load_state multicart_load_state;
struct _multicart_load_state
{
	multicart_t *			multicart;
	zip_file *			zip;
	xml_data_node *			layout_xml;
	xml_data_node *			resources_node;
	xml_data_node *			pcb_node;
	multicart_resource *		resources;
	multicart_socket *		sockets;
};

static const char error_text[14][30] =
{
	"no error",
	"not a multicart",
	"module definition corrupt",
	"out of memory",
	"xml format error",
	"invalid file reference",
	"zip file error",
	"missing ram length",
	"invalid ram specification",
	"unknown resource type",
	"invalid resource reference",
	"invalid file format",
	"missing layout",
	"no pcb or resource found"
};

const char *multicart_error_text(multicart_open_error error)
{
	return error_text[(int)error];
}

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE multicartslot_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MULTICARTSLOT);
	return (multicartslot_t *) downcast<legacy_device_base *>(device)->token();
}


INLINE const multicartslot_config *get_config(const device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MULTICARTSLOT);
	return (const multicartslot_config *) downcast<const legacy_device_base *>(device)->inline_config();
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    cartslot_get_pcb
-------------------------------------------------*/

device_t *cartslot_get_pcb(device_t *device)
{
	multicartslot_t *cart = get_token(device);
	return cart->pcb_device;
}


/*-------------------------------------------------
    cartslot_get_socket
-------------------------------------------------*/

void *cartslot_get_socket(device_t *device, const char *socket_name)
{
	multicartslot_t *cart = get_token(device);
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

int cartslot_get_resource_length(device_t *device, const char *socket_name)
{
	multicartslot_t *cart = get_token(device);
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
    identify_pcb
-------------------------------------------------*/

static const multicartslot_pcb_type *identify_pcb(device_image_interface &image)
{
	const multicartslot_config *config = get_config(&image.device());
	astring pcb_name;
	const multicartslot_pcb_type *pcb_type = NULL;
	multicart_t *mc;
	int i;

	if (image.exists())
	{
		/* try opening this as if it were a multicart */
		multicart_open_error me = multicart_open(image.device().machine().options(), image.filename(), image.device().machine().system().name, MULTICART_FLAGS_DONT_LOAD_RESOURCES, &mc);
		if (me == MCERR_NONE)
		{
			/* this was a multicart - read from it */
			astring_cpyc(&pcb_name, mc->pcb_type);
			multicart_close(image.device().machine().options(), mc);
		}
		else
		{
			if (me != MCERR_NOT_MULTICART)
				fatalerror("multicart error: %s", multicart_error_text(me));
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

/*-------------------------------------------------
    DEVICE_IMAGE_GET_DEVICES(cartslot)
-------------------------------------------------*/
static DEVICE_IMAGE_GET_DEVICES(multicartslot)
{
	const multicartslot_pcb_type *pcb_type;
	device_t *device = &image.device();

	pcb_type = identify_pcb(image);
	if (pcb_type != NULL)
	{
		image_add_device_with_subdevices(device,pcb_type->devtype,TAG_PCB,0);
	}
}


/*-------------------------------------------------
    find_file
-------------------------------------------------*/

static const zip_file_header *find_file(zip_file *zip, const char *filename)
{
	const zip_file_header *header;
	for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
	{
		if (!core_stricmp(header->filename, filename))
			return header;
	}
	return NULL;
}

static const zip_file_header *find_file_crc(zip_file *zip, const char *filename, UINT32 crc)
{
	const zip_file_header *header;
	for (header = zip_file_first_file(zip); header != NULL; header = zip_file_next_file(zip))
	{
		// if the CRC and name both match, we're good
		// if the CRC matches and the name doesn't, we're still good
		if ((!core_stricmp(header->filename, filename)) && (header->crc == crc))
		{
			return header;
		}
		else if (header->crc == crc)
		{
			return header;
		}
	}
	return NULL;
}

/*-------------------------------------------------
    find_pcb_and_resource_nodes
-------------------------------------------------*/

static int find_pcb_and_resource_nodes(xml_data_node *layout_xml,
	xml_data_node **pcb_node, xml_data_node **resource_node)
{
	xml_data_node *romset_node;
	xml_data_node *configuration_node;

	*pcb_node = NULL;
	*resource_node = NULL;

	romset_node = xml_get_sibling(layout_xml->child, "romset");
	if (romset_node == NULL)
		return FALSE;

	configuration_node = xml_get_sibling(romset_node->child, "configuration");
	if (configuration_node == NULL)
		return FALSE;

	*pcb_node = xml_get_sibling(configuration_node->child, "pcb");
	if (*pcb_node == NULL)
		return FALSE;

	*resource_node = xml_get_sibling(romset_node->child, "resources");
	if (*resource_node == NULL)
		return FALSE;

	return TRUE;
}


/*-------------------------------------------------
    get_resource_type
-------------------------------------------------*/

static multicart_resource_type get_resource_type(const char *s)
{
	multicart_resource_type result;
	if (!strcmp(s, "rom"))
		result = MULTICART_RESOURCE_TYPE_ROM;
	else if (!strcmp(s, "ram"))
		result = MULTICART_RESOURCE_TYPE_RAM;
	else
		result = MULTICART_RESOURCE_TYPE_INVALID;
	return result;
}


/*-------------------------------------------------
    load_rom_resource
-------------------------------------------------*/

static multicart_open_error load_rom_resource(multicart_load_state *state, xml_data_node *resource_node,
	multicart_resource *resource)
{
	const char *file, *crcstr, *sha1;
	const zip_file_header *header;
	zip_error ziperr;
	UINT32 crc;

	/* locate the 'file' attribute */
	file = xml_get_attribute_string(resource_node, "file", NULL);
	if (file == NULL)
		return MCERR_XML_ERROR;

	if (!(crcstr = xml_get_attribute_string(resource_node, "crc", NULL)))
	{
		/* locate the file in the ZIP file */
		header = find_file(state->zip, file);
	}
	else	/* CRC tag is present, use it */
	{
		crc = strtoul(crcstr, NULL, 16);
		header = find_file_crc(state->zip, file, crc);
	}
	if (header == NULL)
		return MCERR_INVALID_FILE_REF;
	resource->length = header->uncompressed_length;

	/* allocate bytes for this resource */
	resource->ptr = pool_malloc_lib(state->multicart->data->pool, resource->length);
	if (resource->ptr == NULL)
		return MCERR_OUT_OF_MEMORY;

	/* and decompress it */
	ziperr = zip_file_decompress(state->zip, resource->ptr, resource->length);
	if (ziperr != ZIPERR_NONE)
		return MCERR_ZIP_ERROR;

	/* check SHA1 now */
	if ((sha1 = xml_get_attribute_string(resource_node, "sha1", NULL)))
	{
		hash_collection actual_hashes;
		actual_hashes.compute((const UINT8 *)resource->ptr, resource->length, hash_collection::HASH_TYPES_CRC_SHA1);

		hash_collection expected_hashes;
		expected_hashes.add_from_string(hash_collection::HASH_SHA1, sha1, strlen(sha1));

		if (actual_hashes != expected_hashes)
		{
			return MCERR_INVALID_FILE_REF;
		}
	}

	return MCERR_NONE;
}


/*-------------------------------------------------
    load_ram_resource
-------------------------------------------------*/

static multicart_open_error load_ram_resource(emu_options &options, multicart_load_state *state, xml_data_node *resource_node,
	multicart_resource *resource)
{
	const char *length_string;
	const char *ram_type;
	const char *ram_filename;

	astring *ram_pathname;

	/* locate the 'length' attribute */
	length_string = xml_get_attribute_string(resource_node, "length", NULL);
	if (length_string == NULL)
		return MCERR_MISSING_RAM_LENGTH;

	/* ...and parse it */
	resource->length = ram_device::parse_string(length_string);
	if (resource->length <= 0)
		return MCERR_INVALID_RAM_SPEC;

	/* allocate bytes for this resource */
	resource->ptr = pool_malloc_lib(state->multicart->data->pool, resource->length);
	if (resource->ptr == NULL)
		return MCERR_OUT_OF_MEMORY;

	/* Is this a persistent RAM resource? Then try to load it. */
	ram_type = xml_get_attribute_string(resource_node, "type", NULL);
	if (ram_type != NULL)
	{
		if (strcmp(ram_type, "persistent")==0)
		{
			astring tmp;

			/* Get the file name. */
			ram_filename = xml_get_attribute_string(resource_node, "file", NULL);
			if (ram_filename==NULL)
				return MCERR_XML_ERROR;

			ram_pathname = astring_assemble_3(&tmp, state->multicart->gamedrv_name, PATH_SEPARATOR, ram_filename);

			/* Save the file name so that we can write the contents on unloading.
               If the RAM resource has no filename, we know that it was volatile only. */
			resource->filename = pool_strdup_lib(state->multicart->data->pool, astring_c(ram_pathname));

			if (resource->filename == NULL)
				return MCERR_OUT_OF_MEMORY;

			image_battery_load_by_name(options, resource->filename, resource->ptr, resource->length, 0x00);
		}
		/* else this type is volatile, in which case we just have
            a memory expansion */
	}
	return MCERR_NONE;
}


/*-------------------------------------------------
    load_resource
-------------------------------------------------*/

static multicart_open_error load_resource(emu_options &options, multicart_load_state *state, xml_data_node *resource_node,
	multicart_resource_type resource_type)
{
	const char *id;
	multicart_open_error err;
	multicart_resource *resource;
	multicart_resource **next_resource;

	/* get the 'id' attribute; error if not present */
	id = xml_get_attribute_string(resource_node, "id", NULL);
	if (id == NULL)
		return MCERR_XML_ERROR;

	/* allocate memory for the resource */
	resource = (multicart_resource *)pool_malloc_lib(state->multicart->data->pool, sizeof(*resource));
	if (resource == NULL)
		return MCERR_OUT_OF_MEMORY;
	memset(resource, 0, sizeof(*resource));
	resource->type = resource_type;

	/* copy id */
	resource->id = pool_strdup_lib(state->multicart->data->pool, id);
	if (resource->id == NULL)
		return MCERR_OUT_OF_MEMORY;

	switch(resource->type)
	{
		case MULTICART_RESOURCE_TYPE_ROM:
			err = load_rom_resource(state, resource_node, resource);
			if (err != MCERR_NONE)
				return err;
			break;

		case MULTICART_RESOURCE_TYPE_RAM:
			err = load_ram_resource(options, state, resource_node, resource);
			if (err != MCERR_NONE)
				return err;
			break;

		default:
			return MCERR_UNKNOWN_RESOURCE_TYPE;
	}

	/* append the resource */
	for (next_resource = &state->resources; *next_resource; next_resource = &(*next_resource)->next)
		;
	*next_resource = resource;

	return MCERR_NONE;
}


/*-------------------------------------------------
    load_all_resources
-------------------------------------------------*/

static multicart_open_error load_all_resources(emu_options &options, multicart_load_state *state)
{
	multicart_open_error err;
	xml_data_node *resource_node;
	multicart_resource_type resource_type;

	for (resource_node = state->resources_node->child; resource_node != NULL; resource_node = resource_node->next)
	{
		resource_type = get_resource_type(resource_node->name);
		if (resource_type != MULTICART_RESOURCE_TYPE_INVALID)
		{
			err = load_resource(options, state, resource_node, resource_type);
			if (err != MCERR_NONE)
				return err;
		}
	}

	state->multicart->resources = state->resources;
	return MCERR_NONE;
}

/*-------------------------------------------------
    save_ram_resources. This is important for persistent RAM. All
    resources were allocated within the memory pool of this device and will
    be freed on multicart_close.
-------------------------------------------------*/

static multicart_open_error save_ram_resources(emu_options &options, multicart_t *cart)
{
	const multicart_resource *resource;

	for (resource = cart->resources; resource != NULL; resource = resource->next)
	{
		if ((resource->type == MULTICART_RESOURCE_TYPE_RAM) && (resource->filename != NULL))
		{
			image_battery_save_by_name(options, resource->filename, resource->ptr, resource->length);
		}
	}
	return MCERR_NONE;
}

/*-------------------------------------------------
    load_socket
-------------------------------------------------*/

static multicart_open_error load_socket(multicart_load_state *state, xml_data_node *socket_node)
{
	const char *id;
	const char *uses;
	const multicart_resource *resource;
	multicart_socket *socket;
	multicart_socket **next_socket;

	/* get the 'id' and 'uses' attributes; error if not present */
	id = xml_get_attribute_string(socket_node, "id", NULL);
	uses = xml_get_attribute_string(socket_node, "uses", NULL);
	if ((id == NULL) || (uses == NULL))
		return MCERR_XML_ERROR;

	/* find the resource */
	for (resource = state->multicart->resources; resource != NULL; resource = resource->next)
	{
		if (!strcmp(uses, resource->id))
			break;
	}
	if (resource == NULL)
		return MCERR_INVALID_RESOURCE_REF;

	/* create the socket */
	socket = (multicart_socket *)pool_malloc_lib(state->multicart->data->pool, sizeof(*socket));
	if (socket == NULL)
		return MCERR_OUT_OF_MEMORY;
	memset(socket, 0, sizeof(*socket));
	socket->resource = resource;
	socket->ptr = resource->ptr;

	/* copy id */
	socket->id = pool_strdup_lib(state->multicart->data->pool, id);
	if (socket->id == NULL)
		return MCERR_OUT_OF_MEMORY;

	/* which pointer should I use? */
	if (resource->ptr != NULL)
	{
		/* use the resource's ptr */
		socket->ptr = resource->ptr;
	}
	else
	{
		/* allocate bytes for this socket */
		socket->ptr = pool_malloc_lib(state->multicart->data->pool, resource->length);
		if (socket->ptr == NULL)
			return MCERR_OUT_OF_MEMORY;

		/* ...and clear it */
		memset(socket->ptr, 0xCD, resource->length);
	}

	/* append the resource */
	for (next_socket = &state->sockets; *next_socket; next_socket = &(*next_socket)->next)
		;
	*next_socket = socket;

	return MCERR_NONE;
}


/*-------------------------------------------------
    load_all_sockets
-------------------------------------------------*/

static multicart_open_error load_all_sockets(multicart_load_state *state)
{
	multicart_open_error err;
	xml_data_node *socket_node;

	for (socket_node = xml_get_sibling(state->pcb_node->child, "socket"); socket_node != NULL;
		socket_node = xml_get_sibling(socket_node->next, "socket"))
	{
		err = load_socket(state, socket_node);
		if (err != MCERR_NONE)
			return err;
	}

	state->multicart->sockets = state->sockets;
	return MCERR_NONE;
}


/*-------------------------------------------------
    multicart_open - opens a multicart
-------------------------------------------------*/

multicart_open_error multicart_open(emu_options &options, const char *filename, const char *gamedrv, multicart_load_flags load_flags, multicart_t **cart)
{
	multicart_open_error err;
	zip_error ziperr;
	object_pool *pool;
	multicart_load_state state = {0, };
	const zip_file_header *header;
	const char *pcb_type;
	char *layout_text = NULL;

	/* allocate an object pool */
	pool = pool_alloc_lib(NULL);
	if (pool == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* allocate the multicart */
	state.multicart = (multicart_t*)pool_malloc_lib(pool, sizeof(*state.multicart));
	if (state.multicart == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}
	memset(state.multicart, 0, sizeof(*state.multicart));

	/* allocate the multicart's private data */
	state.multicart->data = (multicart_private*)pool_malloc_lib(pool, sizeof(*state.multicart->data));
	if (state.multicart->data == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}
	memset(state.multicart->data, 0, sizeof(*state.multicart->data));
	state.multicart->data->pool = pool;
	pool = NULL;

	/* open the ZIP file */
	ziperr = zip_file_open(filename, &state.zip);
	if (ziperr != ZIPERR_NONE)
	{
		err = MCERR_NOT_MULTICART;
		goto done;
	}

	/* find the layout.xml file */
	header = find_file(state.zip, "layout.xml");
	if (header == NULL)
	{
		err = MCERR_MISSING_LAYOUT;
		goto done;
	}

	/* reserve space for the layout text */
	layout_text = (char*)malloc(header->uncompressed_length + 1);
	if (layout_text == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* uncompress the layout text */
	ziperr = zip_file_decompress(state.zip, layout_text, header->uncompressed_length);
	if (ziperr != ZIPERR_NONE)
	{
		err = MCERR_ZIP_ERROR;
		goto done;
	}
	layout_text[header->uncompressed_length] = '\0';

	/* parse the layout text */
	state.layout_xml = xml_string_read(layout_text, NULL);
	if (state.layout_xml == NULL)
	{
		err = MCERR_XML_ERROR;
		goto done;
	}

	/* locate the PCB node */
	if (!find_pcb_and_resource_nodes(state.layout_xml, &state.pcb_node, &state.resources_node))
	{
		err = MCERR_NO_PCB_OR_RESOURCES;
		goto done;
	}

	/* get the PCB resource_type */
	pcb_type = xml_get_attribute_string(state.pcb_node, "type", "");
	state.multicart->pcb_type = pool_strdup_lib(state.multicart->data->pool, pcb_type);
	if (state.multicart->pcb_type == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	state.multicart->gamedrv_name = pool_strdup_lib(state.multicart->data->pool, gamedrv);
	if (state.multicart->gamedrv_name == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* do we have to load resources? */
	if (load_flags & MULTICART_FLAGS_LOAD_RESOURCES)
	{
		err = load_all_resources(options, &state);
		if (err != MCERR_NONE)
			goto done;

		err = load_all_sockets(&state);
		if (err != MCERR_NONE)
			goto done;
	}

	err = MCERR_NONE;

done:
	if (pool != NULL)
		pool_free_lib(pool);
	if (state.zip != NULL)
		zip_file_close(state.zip);
	if (layout_text != NULL)
		free(layout_text);
	if (state.layout_xml != NULL)
		xml_file_free(state.layout_xml);

	if ((err != MCERR_NONE) && (state.multicart != NULL))
	{
		multicart_close(options, state.multicart);
		state.multicart = NULL;
	}
	*cart = state.multicart;
	return err;
}


/*-------------------------------------------------
    multicart_close - closes a multicart
-------------------------------------------------*/

void multicart_close(emu_options &options, multicart_t *cart)
{
	save_ram_resources(options, cart);
	pool_free_lib(cart->data->pool);
}

/*-------------------------------------------------
    DEVICE_START( multicartslot )
-------------------------------------------------*/

static DEVICE_START( multicartslot )
{
	const multicartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_START, use it */
	if (config->device_start != NULL)
	{
		(*config->device_start)(device);
	}
}


/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( cartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( multicartslot )
{
	device_t *device = &image.device();
	const multicartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_IMAGE_LOAD, use it */
	if (config->device_load != NULL)
		return (*config->device_load)(image);

	/* otherwise try the normal route */
	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
    DEVICE_IMAGE_UNLOAD( multicartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_UNLOAD( multicartslot )
{
	device_t *device = &image.device();
	const multicartslot_config *config = get_config(device);

	/* if this cartridge has a custom DEVICE_IMAGE_UNLOAD, use it */
	if (config->device_unload != NULL)
	{
		(*config->device_unload)(image);
		return;
	}
}
/*-------------------------------------------------
    DEVICE_GET_INFO( multicartslot )
-------------------------------------------------*/

DEVICE_GET_INFO( multicartslot )
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(multicartslot_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = sizeof(multicartslot_config); break;
		case DEVINFO_INT_IMAGE_TYPE:				info->i = IO_CARTSLOT; break;
		case DEVINFO_INT_IMAGE_READABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_RESET_ON_LOAD:		info->i = 1; break;
		case DEVINFO_INT_IMAGE_MUST_BE_LOADED:      info->i = 0; break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(multicartslot);					break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(multicartslot);		break;
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(multicartslot);		break;
		case DEVINFO_FCT_IMAGE_GET_DEVICES:			info->f = (genf *) DEVICE_IMAGE_GET_DEVICES_NAME(multicartslot);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "MultiCartslot"); break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "MultiCartslot"); break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:
			if ( device && downcast<const legacy_image_device_base *>(device)->inline_config() && get_config(device)->extensions )
			{
				strcpy(info->s, get_config(device)->extensions);
			}
			else
			{
				strcpy(info->s, "bin");
			}
			break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(MULTICARTSLOT, multicartslot);

//**************************************************************************
//  DEVICE CARTSLOT INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_cart_slot_interface - constructor
//-------------------------------------------------

device_cart_slot_interface::device_cart_slot_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_cart_slot_interface - destructor
//-------------------------------------------------

device_cart_slot_interface::~device_cart_slot_interface()
{
}

//**************************************************************************
//  LIVE LEGACY cart_slot DEVICE
//**************************************************************************

//-------------------------------------------------
//  legacy_cart_slot_device_base - constructor
//-------------------------------------------------

legacy_cart_slot_device_base::legacy_cart_slot_device_base(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, UINT32 clock, device_get_config_func get_config)
	: legacy_device_base(mconfig, type, tag, owner, clock, get_config),
	  device_cart_slot_interface(mconfig, *this)
{
}

/* end of multicart */
typedef UINT8	(*read8z_device_func)  (ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, UINT8 *value);

/* We set the number of slots to 8, although we may have up to 16. From a
   logical point of view we could have 256, but the operating system only checks
   the first 16 banks. */
#define NUMBER_OF_CARTRIDGE_SLOTS 8

#define VERBOSE 2
#define LOG logerror

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

	return (const ti99_multicart_config *) downcast<const legacy_device_base *>(device)->inline_config();
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

INLINE multicartslot_t *get_safe_cartslot_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MULTICARTSLOT);

	return (multicartslot_t *)downcast<legacy_device_base *>(device)->token();
}

static WRITE_LINE_DEVICE_HANDLER( cart_grom_ready )
{
	device_t *cartdev = device->owner()->owner();
	ti99_multicart_state *cartslots = get_safe_token(cartdev);
	cartslots->ready(state);
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

/*
    Activates a slot in the multi-cartridge extender.
*/
static void cartridge_slot_set(device_t *cartsys, UINT8 slotnumber)
{
	ti99_multicart_state *cartslots = get_safe_token(cartsys);
	if (VERBOSE>2)
		if (cartslots->active_slot != slotnumber) LOG("ti99: gromport: Setting cartslot to %d\n", slotnumber);
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

	if (VERBOSE>1) LOG("ti99: gromport: mounting cartridge in slot %d\n", slotnumber);

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

	pcb_def->read = (read8z_device_func)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CARTINFO_FCT_6000_R);
	pcb_def->write = (write8_device_func)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CARTINFO_FCT_6000_W);
	pcb_def->cruread = (read8z_device_func)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CARTINFO_FCT_CRU_R);
	pcb_def->cruwrite = (write8_device_func)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CARTINFO_FCT_CRU_W);

    pcb_def->assemble = (assmfct *)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CART_FCT_ASSM);
    pcb_def->disassemble = (assmfct *)downcast<const legacy_cart_slot_device_base *>(pcb)->get_legacy_fct(TI99CART_FCT_DISASSM);

	pcb_def->cartridge = &cartslots->cartridge[index];
	pcb_def->cartridge->pcb = pcb;
}

/*****************************************************************************
  Cartridge type: None
    This PCB device is just a pseudo device
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
	if (VERBOSE>1) LOG("ti99: gromport: Write access to cartridge ROM at address %04x ignored\n", offset);
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing ROM for paged cartridge");
		return IMAGE_INIT_FAIL;
	}
	if (cart->rom2_ptr==NULL)
	{
		if (VERBOSE>1) LOG("ti99: gromport: Missing second ROM for paged cartridge");
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
		if (VERBOSE>1) LOG("ti99: gromport: Write access to cartridge ROM at address %04x ignored", offset);
	}
	else
	{
		if (cartridge->ram_ptr==NULL)
		{
			if (VERBOSE>1) LOG("ti99: gromport: No cartridge RAM at address %04x", offset);
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing GROM for Mini Memory");
		// should not fail here because there may be variations of
		// cartridges which do not use all parts
//      return IMAGE_INIT_FAIL;
	}
	if (cart->rom_size==0)
	{
		if (VERBOSE>1) LOG("ti99: gromport: Missing ROM for Mini Memory");
//      return IMAGE_INIT_FAIL;
	}
	if (cart->ram_size==0)
	{
		if (VERBOSE>1) LOG("ti99: gromport: Missing RAM for Mini Memory");
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
	if (VERBOSE>1) LOG("ti99: gromport: Superspace: CRU accessed at %04x\n", offset);
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
	if (VERBOSE>1) LOG("ti99: gromport: Superspace: CRU accessed at %04x\n", offset);
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
		if (VERBOSE>1) LOG("ti99: gromport: No cartridge RAM at address %04x", offset);
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing RAM for SuperSpace");
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing ROM for paged cartridge");
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing ROM for pagedcru cartridge");
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
		if (VERBOSE>1) LOG("ti99: gromport: Missing loader GROM for GRAM Kracker system");
		return IMAGE_INIT_FAIL;
	}

	if (cart->ram_size < 81920)
	{
		if (VERBOSE>1) LOG("ti99: gromport: Missing or insufficient RAM for GRAM Kracker system");
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
	multicartslot_t *cart = get_safe_cartslot_token(device);

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
		multicartslot_t *cart = get_safe_cartslot_token(&image.device());

		/* try opening this as a multicart */
		// This line requires that cartslot_t be included in cartslot.h,
		// otherwise one cannot make use of multicart handling within such a
		// custom LOAD function.
		multicart_open_error me = multicart_open(image.device().machine().options(), image.filename(), image.device().machine().system().name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);
		if (VERBOSE>1) LOG("ti99: gromport: opened %s as cartridge\n", image.filename());
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
		multicartslot_t *cart = get_safe_cartslot_token(&image.device());

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

	cartslots->ready.resolve(ready, *device);

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
//  int slot;
	cartridge_t *cartridge;
	ti99_multicart_state *cartslots = get_safe_token(device);

	// Set the cart slot.
	// 1001 1wbb bbbb bbr0

	cartridge_slot_set(device, (UINT8)((offset>>2) & 0x00ff));
//  slot = cartslots->active_slot;

//  if (cartslots->fixed_slot==AUTO && cartslots->next_free_slot==1)
//      slot=0;

	// Handle the GRAM Kracker
	if ((cartslots->gk_slot != -1) && (input_port_read(device->machine(), "CARTSLOT")==CART_GK))
	{
		cartridge_gram_kracker_writeg(device, offset, data);
		return;
	}

	// We need to send the write request to all attached cartridges
	// so the slot is irrelevant here. (We don't have GRAM cartridges,
	// anyway, so it's just used for setting the adddress)
	for (int j=0; j < NUMBER_OF_CARTRIDGE_SLOTS; j++)
	{
		cartridge = &cartslots->cartridge[j];
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

	// Set the cart slot. Bit pattern:
	// 1001 1wbb bbbb bbr0

	cartridge_slot_set(device, (UINT8)((offset>>2) & 0x00ff));

	// Handle the GRAM Kracker
	// BTW, the GK does not have a readable address counter, but the console
	// GROMs will keep our address counter up to date. That is
	// exactly what happens in the real machine.
	// (The console GROMs are not accessed here but directly via the datamux
	// so we can just return without doing anything)
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

	// We need to send the read request to all cartriges so that
	// the internal address counters stay in sync. However, we only respect
	// the return of the selected cartridge.
	for (int j=0; j < NUMBER_OF_CARTRIDGE_SLOTS; j++)
	{
		// Select the cartridge which the multicart system is currently pointing at
		cartridge = &cartslots->cartridge[j];
		// Send the request to all mounted GROMs on this PCB
		if (cartridge->pcb!=NULL)
		{
			for (int i=0; i < 5; i++)
			{
				ti99_pcb_t *pcbdef = get_safe_pcb_token(cartridge->pcb);
				if (pcbdef->grom[i]!=NULL)
				{
					UINT8 newvalue = *value;
					ti99grom_rz(pcbdef->grom[i], offset, &newvalue);
					if (j==slot)
					{
						*value = newvalue;
					}
				}
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
			// Otherwise, the console GROM 1 will respond (at another location)
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
	cartridge_t *gramkrackerbox;
	ti99_multicart_state *cartslots = get_safe_token(cartsys);

	// This is only called when there is really a GK module
	gramkrackerbox = &cartslots->cartridge[cartslots->gk_slot];

	if ((offset & 0x0002)==0x0002)
	{
		// Set address
		if (gramkrackerbox->waddr_LSB)
		{
			/* Accept low byte (2nd write) */
			gramkrackerbox->grom_address = (gramkrackerbox->grom_address & 0xff00) | data;
			gramkrackerbox->waddr_LSB = FALSE;
		}
		else
		{
			/* Accept high byte (1st write) */
			gramkrackerbox->grom_address = ((data<<8) & 0xff00) | (gramkrackerbox->grom_address & 0x00ff);
			gramkrackerbox->waddr_LSB = TRUE;
		}
	}
	else
	{
		// According to manual:
		// Writing to GRAM 0: switch 2 set to GRAM 0 + Write protect switch (4) in 1 or 2 position
		// Writing to GRAM 1: switch 3 set to GRAM 1-2 + Loader off (5); write prot has no effect
		// Writing to GRAM 2: switch 1 set to GRAM 1-2 (write prot has no effect)
		// Writing to GRAM 3-7: switch set to GK_NORMAL, no cartridge inserted
		// GK_NORMAL switch has no effect on GRAM 0-2
		if (((gramkrackerbox->grom_address & 0xe000)==0x0000 && get_gk_switch(cartsys, 2)==GK_GRAM0 && get_gk_switch(cartsys, 4)!=GK_WP)
			|| ((gramkrackerbox->grom_address & 0xe000)==0x2000 && get_gk_switch(cartsys, 3)==GK_GRAM12 && get_gk_switch(cartsys, 5)==GK_LDOFF)
			|| ((gramkrackerbox->grom_address & 0xe000)==0x4000 && get_gk_switch(cartsys, 3)==GK_GRAM12)
			|| ((gramkrackerbox->grom_address & 0xe000)==0x6000 && get_gk_switch(cartsys, 1)==GK_NORMAL && cartslots->gk_guest_slot==-1)
			|| ((gramkrackerbox->grom_address & 0x8000)==0x8000 && get_gk_switch(cartsys, 1)==GK_NORMAL && cartslots->gk_guest_slot==-1))
		{
			(gramkrackerbox->ram_ptr)[gramkrackerbox->grom_address] = data;
		}
		// The GK GROM emulation does not wrap at 8K boundaries.
		gramkrackerbox->grom_address = (gramkrackerbox->grom_address+1) & 0xffff;

		/* Reset the write address flipflop. */
		gramkrackerbox->waddr_LSB = FALSE;
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
			*value = (gromkrackerbox->ram_ptr)[offset+0x10000 - 0x6000];
		}
		else
		{
			if (get_gk_switch(device, 4)==GK_BANK2)
			{
				*value = (gromkrackerbox->ram_ptr)[offset+0x12000 - 0x6000];
			}
			else
			{
				// Write protection on; auto-bank
				if (gromkrackerbox->ram_page==0)
				{
					*value = (gromkrackerbox->ram_ptr)[offset+0x10000 - 0x6000];
				}
				else
				{
					*value = (gromkrackerbox->ram_ptr)[offset+0x12000 - 0x6000];
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
		ti99_pcb_t *pcbdef = get_safe_pcb_token(gkguestcart->pcb);
			(*pcbdef->write)(gkguestcart->pcb, offset, data);
	}
	else
	{
		if (get_gk_switch(device, 1)==GK_OFF)
			return;

		if (get_gk_switch(device, 4)==GK_BANK1)
		{
			(gromkrackerbox->ram_ptr)[offset + 0x10000 - 0x6000] = data;
		}
		else
		{
			if (get_gk_switch(device, 4)==GK_BANK2)
			{
				(gromkrackerbox->ram_ptr)[offset + 0x12000 - 0x6000] = data;
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
#define TI99_CARTRIDGE_SLOT(p)  MCFG_MULTICARTSLOT_ADD(p) \
	MCFG_MULTICARTSLOT_EXTENSION_LIST("rpk") \
	MCFG_MULTICARTSLOT_PCBTYPE(0, "none", TI99_CARTRIDGE_PCB_NONE) \
	MCFG_MULTICARTSLOT_PCBTYPE(1, "standard", TI99_CARTRIDGE_PCB_STD) \
	MCFG_MULTICARTSLOT_PCBTYPE(2, "paged", TI99_CARTRIDGE_PCB_PAGED) \
	MCFG_MULTICARTSLOT_PCBTYPE(3, "minimem", TI99_CARTRIDGE_PCB_MINIMEM) \
	MCFG_MULTICARTSLOT_PCBTYPE(4, "super", TI99_CARTRIDGE_PCB_SUPER) \
	MCFG_MULTICARTSLOT_PCBTYPE(5, "mbx", TI99_CARTRIDGE_PCB_MBX) \
	MCFG_MULTICARTSLOT_PCBTYPE(6, "paged379i", TI99_CARTRIDGE_PCB_PAGED379I) \
	MCFG_MULTICARTSLOT_PCBTYPE(7, "pagedcru", TI99_CARTRIDGE_PCB_PAGEDCRU) \
	MCFG_MULTICARTSLOT_PCBTYPE(8, "gramkracker", TI99_CARTRIDGE_PCB_GRAMKRACKER) \
	MCFG_MULTICARTSLOT_START(ti99_cartridge) \
	MCFG_MULTICARTSLOT_LOAD(ti99_cartridge) \
	MCFG_MULTICARTSLOT_UNLOAD(ti99_cartridge)

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

DEFINE_LEGACY_DEVICE(TI99_MULTICART, ti99_multicart);
