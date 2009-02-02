/*********************************************************************

	multcart.c

	Multi-cartridge handling code

*********************************************************************/

#include "multcart.h"
#include "pool.h"
#include "unzip.h"
#include "corestr.h"
#include "xmlfile.h"
#include "driver.h"
#include "compcfg.h"


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
	multicart *				multicart;
	zip_file *				zip;
	xml_data_node *			layout_xml;
	xml_data_node *			resources_node;
	xml_data_node *			pcb_node;
	multicart_resource *	resources;
	multicart_socket *		sockets;
};


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

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
	const char *file;
	const zip_file_header *header;
	zip_error ziperr;

	/* locate the 'file' attribute */
	file = xml_get_attribute_string(resource_node, "file", NULL);
	if (file == NULL)
		return MCERR_NOT_MULTICART;

	/* locate the file in the ZIP file */
	header = find_file(state->zip, file);
	if (header == NULL)
		return MCERR_NOT_MULTICART;
	resource->length = header->uncompressed_length;

	/* allocate bytes for this resource */
	resource->ptr = pool_malloc(state->multicart->data->pool, resource->length);
	if (resource->ptr == NULL)
		return MCERR_OUT_OF_MEMORY;

	/* and decompress it */
	ziperr = zip_file_decompress(state->zip, resource->ptr, resource->length);
	if (ziperr != ZIPERR_NONE)
		return MCERR_NOT_MULTICART;

	return MCERR_NONE;
}


/*-------------------------------------------------
    load_ram_resource
-------------------------------------------------*/

static multicart_open_error load_ram_resource(multicart_load_state *state, xml_data_node *resource_node,
	multicart_resource *resource)
{
	const char *length_string;

	/* locate the 'length' attribute */
	length_string = xml_get_attribute_string(resource_node, "length", NULL);
	if (length_string == NULL)
		return MCERR_NOT_MULTICART;

	/* ...and parse it */
	resource->length = ram_parse_string(length_string);
	if (resource->length <= 0)
		return MCERR_NOT_MULTICART;

	return MCERR_NONE;
}


/*-------------------------------------------------
    load_resource
-------------------------------------------------*/

static multicart_open_error load_resource(multicart_load_state *state, xml_data_node *resource_node,
	multicart_resource_type resource_type)
{
	const char *id;
	multicart_open_error err;
	multicart_resource *resource;
	multicart_resource **next_resource;

	/* get the 'id' attribute; error if not present */
	id = xml_get_attribute_string(resource_node, "id", NULL);
	if (id == NULL)
		return MCERR_NOT_MULTICART;

	/* allocate memory for the resource */
	resource = pool_malloc(state->multicart->data->pool, sizeof(*resource));
	if (resource == NULL)
		return MCERR_OUT_OF_MEMORY;
	memset(resource, 0, sizeof(*resource));
	resource->type = resource_type;

	/* copy id */
	resource->id = pool_strdup(state->multicart->data->pool, id);
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
			err = load_ram_resource(state, resource_node, resource);
			if (err != MCERR_NONE)
				return err;
			break;

		default:
			return MCERR_NOT_MULTICART; 
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

static multicart_open_error load_all_resources(multicart_load_state *state)
{
	multicart_open_error err;
	xml_data_node *resource_node;
	multicart_resource_type resource_type;

	for (resource_node = state->resources_node->child; resource_node != NULL; resource_node = resource_node->next)
	{
		resource_type = get_resource_type(resource_node->name);
		if (resource_type != MULTICART_RESOURCE_TYPE_INVALID)
		{
			err = load_resource(state, resource_node, resource_type);
			if (err != MCERR_NONE)
				return err;
		}
	}

	state->multicart->resources = state->resources;
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
		return MCERR_NOT_MULTICART;

	/* find the resource */
	for (resource = state->multicart->resources; resource != NULL; resource = resource->next)
	{
		if (!strcmp(uses, resource->id))
			break;
	}
	if (resource == NULL)
		return MCERR_NOT_MULTICART;

	/* create the socket */
	socket = pool_malloc(state->multicart->data->pool, sizeof(*socket));
	if (socket == NULL)
		return MCERR_OUT_OF_MEMORY;
	memset(socket, 0, sizeof(*socket));
	socket->resource = resource;
	socket->ptr = resource->ptr;

	/* copy id */
	socket->id = pool_strdup(state->multicart->data->pool, id);
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
		socket->ptr = pool_malloc(state->multicart->data->pool, resource->length);
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

multicart_open_error multicart_open(const char *filename, multicart_load_flags load_flags, multicart **cart)
{
	multicart_open_error err;
	zip_error ziperr;
	object_pool *pool;
	multicart_load_state state = {0, };
	const zip_file_header *header;
	const char *pcb_type;
	char *layout_text = NULL;

	/* allocate an object pool */
	pool = pool_alloc(NULL);
	if (pool == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* allocate the multicart */
	state.multicart = pool_malloc(pool, sizeof(*state.multicart));
	if (state.multicart == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}
	memset(state.multicart, 0, sizeof(*state.multicart));

	/* allocate the multicart's private data */
	state.multicart->data = pool_malloc(pool, sizeof(*state.multicart->data));
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
		err = MCERR_NOT_MULTICART;
		goto done;
	}

	/* reserve space for the layout text */
	layout_text = malloc(header->uncompressed_length + 1);
	if (layout_text == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* uncompress the layout text */
	ziperr = zip_file_decompress(state.zip, layout_text, header->uncompressed_length);
	if (ziperr != ZIPERR_NONE)
	{
		err = MCERR_NOT_MULTICART;
		goto done;
	}
	layout_text[header->uncompressed_length] = '\0';

	/* parse the layout text */
	state.layout_xml = xml_string_read(layout_text, NULL);
	if (state.layout_xml == NULL)
	{
		err = MCERR_NOT_MULTICART;
		goto done;
	}

	/* locate the PCB node */
	if (!find_pcb_and_resource_nodes(state.layout_xml, &state.pcb_node, &state.resources_node))
	{
		err = MCERR_NOT_MULTICART;
		goto done;
	}

	/* get the PCB resource_type */
	pcb_type = xml_get_attribute_string(state.pcb_node, "type", "");
	state.multicart->pcb_type = pool_strdup(state.multicart->data->pool, pcb_type);
	if (state.multicart->pcb_type == NULL)
	{
		err = MCERR_OUT_OF_MEMORY;
		goto done;
	}

	/* do we have to load resources? */
	if (load_flags & MULTICART_FLAGS_LOAD_RESOURCES)
	{
		err = load_all_resources(&state);
		if (err != MCERR_NONE)
			goto done;

		err = load_all_sockets(&state);
		if (err != MCERR_NONE)
			goto done;
	}

	err = MCERR_NONE;

done:
	if (pool != NULL)
		pool_free(pool);
	if (state.zip != NULL)
		zip_file_close(state.zip);
	if (layout_text != NULL)
		free(layout_text);
	if (state.layout_xml != NULL)
		xml_file_free(state.layout_xml);

	if ((err != MCERR_NONE) && (state.multicart != NULL))
	{
		multicart_close(state.multicart);
		state.multicart = NULL;
	}
	*cart = state.multicart;
	return err;
}


/*-------------------------------------------------
    multicart_close - closes a multicart
-------------------------------------------------*/

void multicart_close(multicart *cart)
{
	pool_free(cart->data->pool);
}

