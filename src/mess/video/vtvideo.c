/**********************************************************************

    DEC VT Terminal video emulation
    [ DC012 and DC011 emulation ]

    01/05/2009 Initial implementation [Miodrag Milanovic]

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "vtvideo.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define	VERBOSE			0

#define	LOG(x)		do { if (VERBOSE) logerror x; } while (0)
	
/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vt_video_t vt_video_t;
struct _vt_video_t
{
	devcb_resolved_read8		in_ram_func;

	const device_config *screen;	/* screen */
	UINT8 *gfx;		/* content of char rom */
	
    int skip_lines;	
    int height;
    int width;
    int lba7;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vt_video_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (vt_video_t *)device->token;
}

INLINE const vt_video_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == dc012));
	return (const vt_video_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/
READ8_DEVICE_HANDLER( lba7_r )
{
	vt_video_t *vt = get_safe_token(device);
	return vt->lba7;
}


WRITE8_DEVICE_HANDLER( dc012_w )
{
//	vt_video_t *vt = get_safe_token(device);
}


WRITE8_DEVICE_HANDLER( dc011_w )
{
//	vt_video_t *vt = get_safe_token(device);
}

WRITE8_DEVICE_HANDLER( vt_video_brightness_w )
{
	palette_set_color_rgb(device->machine, 1, data, data, data);
}

void vt_video_display_char(const device_config *device,bitmap_t *bitmap, UINT8 code, 
	int x, int y,UINT8 scroll_region,UINT8 display_type) 
{					
	UINT8 line=0;
   	int i,b,bit;
 	int double_width = (display_type==2) ? 1 : 0;
 	vt_video_t *vt = get_safe_token(device);

	for (i = 0; i < 10; i++)
	{
		switch(display_type) {
			case 0 : // bottom half, double height
					 line = vt->gfx[(code & 0x7f)*16 + (i >> 1)+5]; break;
			case 1 : // top half, double height
				 	 line = vt->gfx[(code & 0x7f)*16 + (i >> 1)]; break;
			case 2 : // double width				
			case 3 : // normal
					 line = vt->gfx[(code & 0x7f)*16 + i]; break;
		}
		
		for (b = 0; b < 8; b++)
		{
			bit = ((line << b) & 0x80) ? 1 : 0;
			if (double_width) {
				*BITMAP_ADDR16(bitmap, y*10+i, x*20+b*2)   =  bit;
				*BITMAP_ADDR16(bitmap, y*10+i, x*20+b*2+1) =  bit;
			} else {
				*BITMAP_ADDR16(bitmap, y*10+i, x*10+b) =  bit;
			}
		}
		// char interleave is filled with last bit
		if (double_width) {
			*BITMAP_ADDR16(bitmap, y*10+i, x*20+16) =  bit;
			*BITMAP_ADDR16(bitmap, y*10+i, x*20+17) =  bit;
			*BITMAP_ADDR16(bitmap, y*10+i, x*20+18) =  bit;
			*BITMAP_ADDR16(bitmap, y*10+i, x*20+19) =  bit;
		} else {
			*BITMAP_ADDR16(bitmap, y*10+i, x*10+8) =  bit;
			*BITMAP_ADDR16(bitmap, y*10+i, x*10+9) =  bit;
		}
	}
}

void vt_video_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	vt_video_t *vt = get_safe_token(device);

	UINT16 addr = 0;
    int line = 0;
    int xpos = 0;
    int ypos = 0;
    UINT8 code;
    int x = 0;
    UINT8 scroll_region = 1; // binary 1
    UINT8 display_type = 3;  // binary 11
    UINT16 temp =0;
    
	if (devcb_call_read8(&vt->in_ram_func, 0) !=0x7f) return;
		
    while(line < (vt->height + vt->skip_lines)) {
	    code =  devcb_call_read8(&vt->in_ram_func, addr + xpos);
	    if (code == 0x7f) {
	    	// end of line, fill empty till end of line
	    	if (line >= vt->skip_lines) {
	    		for(x = xpos; x < ((display_type==2) ? (vt->width / 2) : vt->width); x++ )
				{
					vt_video_display_char(device,bitmap,code,x,ypos,scroll_region,display_type);
				}
	    	}    
	    	// move to new data	
	    	temp = devcb_call_read8(&vt->in_ram_func, addr+xpos+1)*256 + devcb_call_read8(&vt->in_ram_func, addr+xpos+2);
	    	addr = (temp) & 0x1fff;
	    	// if A12 is 1 then it is 0x2000 block, if 0 then 0x4000 (AVO)
	    	if (addr & 0x1000) addr &= 0xfff; else addr |= 0x2000;
	    	scroll_region = (temp >> 15) & 1;
	    	display_type  = (temp >> 13) & 3;
	    	if (line >= vt->skip_lines) {
	    		ypos++;
	    	}
	    	xpos=0;
	    	line++;    	    	
	    } else {
	    	// display regular char
	    	if (line >= vt->skip_lines) {
				vt_video_display_char(device,bitmap,code,xpos,ypos,scroll_region,display_type);
			}    				
			xpos++;
			if (xpos > vt->width) {
				line++;				
				xpos=0;
			}
	    }
	}
	
}

/*-------------------------------------------------
    DEVICE_START( vt_video )
-------------------------------------------------*/

static DEVICE_START( vt_video )
{
	vt_video_t *vt = get_safe_token(device);
	const vt_video_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&vt->in_ram_func, &intf->in_ram_func, device);

	/* get the screen device */
	vt->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(vt->screen != NULL);
	
  	vt->gfx = memory_region(device->machine, intf->char_rom_region_tag);
	assert(vt->gfx != NULL);
	
}

static TIMER_CALLBACK(lba7_change)
{
	const device_config *device = ptr;
	vt_video_t *vt = get_safe_token(device);

	vt->lba7 = (vt->lba7) ? 0 : 1;
}

/*-------------------------------------------------
    DEVICE_RESET( vt_video )
-------------------------------------------------*/

static DEVICE_RESET( vt_video )
{
	vt_video_t *vt = get_safe_token(device);
	palette_set_color_rgb(device->machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(device->machine, 1, 0xff, 0xff, 0xff); // white

	vt->skip_lines = 2; // for 60Hz
	vt->width = 80;
	vt->height = 24;
	vt->lba7 = 0;

	// LBA7 is scan line frequency update
	timer_pulse(device->machine, ATTOTIME_IN_NSEC(31778), (void *) device, 0, lba7_change);	
}

/*-------------------------------------------------
    DEVICE_GET_INFO( dc012 )
-------------------------------------------------*/

DEVICE_GET_INFO( vt100_video )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vt_video_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vt_video);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vt_video);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "VT100 Video");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "VTxxx Video");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}
