/*********************************************************************

	beta.h

	Implementation of Beta disk drive support for Spectrum and clones
	
	04/05/2008 Created by Miodrag Milanovic

*********************************************************************/
#include "driver.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"
#include "machine/beta.h"

static int betadisk_status;
static int betadisk_active;

int betadisk_is_active(void)
{
	return betadisk_active;
}

void betadisk_enable(void)
{
	betadisk_active = 1;
}

void betadisk_disable(void)
{
	betadisk_active = 0;
}

void betadisk_clear_status(void)
{
	betadisk_status = 0;
}

void betadisk_wd179x_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	switch (state)
	{
		case WD17XX_DRQ_SET:
		{
			betadisk_status |= (1<<6);
		}
		break;

		case WD17XX_DRQ_CLR:
		{
			betadisk_status &=~(1<<6);
		}
		break;

		case WD17XX_IRQ_SET:
		{
			betadisk_status |= (1<<7);
		}
		break;

		case WD17XX_IRQ_CLR:
		{
			betadisk_status &=~(1<<7);
		}
		break;
	}
}

READ8_HANDLER(betadisk_status_r)
{
	if (betadisk_active==1) {
		return wd17xx_status_r(space, offset); 
	} else {
		return 0xff;
	}
}

READ8_HANDLER(betadisk_track_r)
{
	if (betadisk_active==1) {
		return wd17xx_track_r(space, offset); 
	} else {
		return 0xff;
	}
}

READ8_HANDLER(betadisk_sector_r)
{
	if (betadisk_active==1) {
		return wd17xx_sector_r(space, offset); 
	} else {
		return 0xff;
	}
}

READ8_HANDLER(betadisk_data_r)
{
	if (betadisk_active==1) {
		return wd17xx_data_r(space, offset); 
	} else {
		return 0xff;
	}
}

READ8_HANDLER(betadisk_state_r)
{
	if (betadisk_active==1) {
		return betadisk_status; 
	} else {
		return 0xff;
	}
}

WRITE8_HANDLER(betadisk_param_w)
{ 
	if (betadisk_active==1) {
  		wd17xx_set_drive ( data & 3);  
  		wd17xx_set_side ((data & 0x10) ? 0 : 1 );
  		wd17xx_set_density(data & 0x20 ? DEN_MFM_HI : DEN_FM_LO );
  		if ((data & 0x04) == 0) // reset
  		{
  			wd17xx_reset(space->machine);	
  		}    		
  		betadisk_status = (data & 0x3f) | betadisk_status;
  	}
} 	

WRITE8_HANDLER(betadisk_command_w)
{
	if (betadisk_active==1) {
		wd17xx_command_w(space, offset, data);
	}	
}

WRITE8_HANDLER(betadisk_track_w)
{
	if (betadisk_active==1) {
		wd17xx_track_w(space, offset, data);
	}	
}

WRITE8_HANDLER(betadisk_sector_w)
{
	if (betadisk_active==1) {
		wd17xx_sector_w(space, offset, data);
	}	
}

WRITE8_HANDLER(betadisk_data_w)
{
	if (betadisk_active==1) {
		wd17xx_data_w(space, offset, data);
	}	
}

static DEVICE_IMAGE_LOAD( beta_floppy )
{
	UINT8 data[1];
	int heads;
	int cylinders;
	
	if (device_load_basicdsk_floppy (image) != INIT_PASS)
		return INIT_FAIL;

	image_fseek( image, 0x8e3 , SEEK_SET );
	/* Read and verify the header */
	if ( 1 != image_fread( image, data, 1 ) )
	{
		image_seterror( image, IMAGE_ERROR_UNSUPPORTED, "Unable to read header" );
		return 1;
	} 
	
	image_fseek( image, 0 , SEEK_SET );
  	/* guess geometry of disk */
  	heads =  data[0] & 0x08 ? 1 : 2;
  	cylinders = data[0] & 0x01 ? 40 : 80;
  
	basicdsk_set_geometry (image, cylinders, heads, 16, 256, 1, 0, FALSE);
	return INIT_PASS;
}

void beta_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(beta_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "trd"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}	
}
