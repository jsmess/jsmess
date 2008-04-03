/***************************************************************************

    OKI MSM6255 Dot Matrix LCD Controller implementation

    Copyright MESS team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __MSM6255_VIDEO__
#define __MSM6255_VIDEO__

typedef UINT8 (*msm6255_char_ram_read_func)(const device_config *device, UINT16 ma, UINT8 ra);
#define MSM6255_CHAR_RAM_READ(name) UINT8 name(const device_config *device, UINT16 ma, UINT8 ra)

#define MSM6255		DEVICE_GET_INFO_NAME(msm6255)

/* interface */
typedef struct _msm6255_interface msm6255_interface;
struct _msm6255_interface
{
	const char *screen_tag;		/* screen we are acting on */
	int pixel_clock;			/* the pixel clock of the chip */
	int character_clock;		/* the character clock of the chip */

	/* character memory read function */
	msm6255_char_ram_read_func		char_ram_r;
};

/* device interface */
DEVICE_GET_INFO( msm6255 );

/* character memory access */
READ8_DEVICE_HANDLER ( msm6255_register_r );
WRITE8_DEVICE_HANDLER ( msm6255_register_w );

/* updates the screen */
void msm6255_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
