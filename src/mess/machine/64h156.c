/**********************************************************************

    Commodore 64H165 Gate Array emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "64h156.h"
#include "formats/g64_dsk.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c64h156_t c64h156_t;
struct _c64h156_t
{
	/* motors */
	int stp;								/* stepper motor phase */
	int mtr;								/* spindle motor on */

	/* track */
	UINT8 track_buffer[G64_BUFFER_SIZE];	/* track data buffer */
	int track_len;							/* track length */
	int buffer_pos;							/* current byte position within track buffer */
	int bit_pos;							/* current bit position within track buffer byte */
	int bit_count;							/* current data byte bit counter */
	UINT16 data;							/* data shift register */
	UINT8 yb;								/* GCR data byte to write */

	/* signals */
	int ds;									/* density select */
	int soe;								/* s? output enable */
	int byte;								/* byte ready */
	int mode;								/* mode (0 = write, 1 = read) */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c64h156_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == C64H156);
	return (c64h156_t *)downcast<legacy_device_base *>(device)->token();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

READ8_DEVICE_HANDLER( c64h156_yb_r )
{
	return 0;
}

WRITE8_DEVICE_HANDLER( c64h156_yb_w )
{
}

READ_LINE_DEVICE_HANDLER( c64h156_sync_r )
{
	return 0;
}

READ_LINE_DEVICE_HANDLER( c64h156_byte_r )
{
	return 0;
}

void c64h156_stp_w(device_t *device, int stp)
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_stp1_w )
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_mtr_w )
{
}

void c64h156_ds_w(device_t *device, int data)
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_ted_w )
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_oe_w )
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_soe_w )
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_atni_w )
{
}

WRITE_LINE_DEVICE_HANDLER( c64h156_atna_w )
{
}

/*-------------------------------------------------
    DEVICE_START( c64h156 )
-------------------------------------------------*/

static DEVICE_START( c64h156 )
{
//  c64h156_t *c64h156 = get_safe_token(device);

	/* allocate data timer */
//  c64h156->bit_timer = device->machine().scheduler().timer_alloc(FUNC(bit_tick), (void *)device);

	/* register for state saving */
//  device->save_item(NAME(c64h156->));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c64h156 )
-------------------------------------------------*/

DEVICE_GET_INFO( c64h156 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c64h156_t);								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c64h156);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							/* Nothing */												break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 64H156");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 1541");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(C64H156, c64h156);
