/*****************************************************************************
 *
 * video/dl1416.h
 *
 * DL1416
 *
 * 4-Digit 16-Segment Alphanumeric Intelligent Display
 * with Memory/Decoder/Driver
 *
 * See video/dl1416.c for more info
 *
 * **************************************************************************/

#ifndef DL1416_H_
#define DL1416_H_

#include "driver.h"

typedef enum
{
	DL1416B,
	DL1416T
} dl1416_type_t;


/* Interface */
typedef struct _dl1416_interface dl1416_interface;
struct _dl1416_interface
{
	dl1416_type_t type;
	void (*digit_changed)(int digit, int data);
};

/* Configuration */
void dl1416_config(int which, const dl1416_interface *intf);
void dl1416_reset(int which);

/* Inputs */
void dl1416_set_input_w(int which, int data); /* Write enable */
void dl1416_set_input_ce(int which, int data); /* Chip enable */
void dl1416_set_input_cu(int which, int data); /* Cursor enable */
void dl1416_write(int which, offs_t offset, UINT8 data); /* Data */

/* Standard handlers */
WRITE8_HANDLER( dl1416_0_w );
WRITE8_HANDLER( dl1416_1_w );
WRITE8_HANDLER( dl1416_2_w );
WRITE8_HANDLER( dl1416_3_w );
WRITE8_HANDLER( dl1416_4_w );

#endif /*DL1416_H_*/
