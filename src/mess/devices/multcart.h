/*********************************************************************

	multcart.h

	Multi-cartridge handling code

*********************************************************************/

#ifndef __MULTCART_H__
#define __MULTCART_H__

#include "osdcore.h"


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

typedef struct _multicart multicart;
struct _multicart
{
	const multicart_resource *	resources;
	const multicart_socket *	sockets;
	const char *				pcb_type;
	multicart_private *			data;
};


enum _multicart_open_error
{
	MCERR_NONE,
	MCERR_NOT_MULTICART,
	MCERR_CORRUPT,
	MCERR_OUT_OF_MEMORY
};
typedef enum _multicart_open_error multicart_open_error;


/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* opens a multicart */
multicart_open_error multicart_open(const char *filename, multicart_load_flags load_flags, multicart **cart);

/* closes a multicart */
void multicart_close(multicart *cart);

#endif /* __MULTCART_H__ */
