/*

	Atmel Serial DataFlash

	(c) 2001-2007 Tim Schuerewegen

	AT45DB041 -  528 KByte
	AT45DB081 - 1056 KByte
	AT45DB161 - 2112 KByte

*/

#ifndef _AT45DBXX_H_
#define _AT45DBXX_H_

#include "driver.h"


/***************************************************************************
    MACROS
***************************************************************************/

#define AT45DB041		DEVICE_GET_INFO_NAME(at45db041)

#define MDRV_AT45DB041_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, AT45DB041, 0) \

#define AT45DB081		DEVICE_GET_INFO_NAME(at45db081)

#define MDRV_AT45DB081_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, AT45DB081, 0) \

#define AT45DB161		DEVICE_GET_INFO_NAME(at45db161)

#define MDRV_AT45DB161_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, AT45DB161, 0) \


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(at45db041);
DEVICE_GET_INFO(at45db081);
DEVICE_GET_INFO(at45db161);

// pins
void at45dbxx_pin_cs(const device_config *device, int data);
void at45dbxx_pin_sck(const device_config *device,  int data);
void at45dbxx_pin_si(const device_config *device,  int data);
int  at45dbxx_pin_so(const device_config *device);

// load/save
void at45dbxx_load(const device_config *device, mame_file *file);
void at45dbxx_save(const device_config *device, mame_file *file);

// non-volatile ram handler
/*
NVRAM_HANDLER( at45dbxx );
*/

#endif
