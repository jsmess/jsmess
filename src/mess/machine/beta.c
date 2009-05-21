/*********************************************************************

	beta.h

	Implementation of Beta disk drive support for Spectrum and clones
	
	04/05/2008 Created by Miodrag Milanovic

*********************************************************************/
#include "driver.h"
#include "devices/basicdsk.h"
#include "machine/wd17xx.h"
#include "machine/beta.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _beta_disk_state beta_disk_state;
struct _beta_disk_state
{
	UINT8 betadisk_status;
	UINT8 betadisk_active;

	const device_config *wd179x;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/
INLINE beta_disk_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (beta_disk_state *)device->token;
}


int betadisk_is_active(const device_config *device)
{
	beta_disk_state *beta = get_safe_token(device);

	return beta->betadisk_active;
}

void betadisk_enable(const device_config *device)
{
	beta_disk_state *beta = get_safe_token(device);
	
	beta->betadisk_active = 1;
}

void betadisk_disable(const device_config *device)
{
	beta_disk_state *beta = get_safe_token(device);
	
	beta->betadisk_active = 0;
}

void betadisk_clear_status(const device_config *device)
{
	beta_disk_state *beta = get_safe_token(device);
	
	beta->betadisk_status = 0;
}

static WD17XX_CALLBACK( betadisk_wd179x_callback )
{
	const device_config *wd_device = devtag_get_device(device->machine, BETA_DISK_TAG);
	beta_disk_state *beta = get_safe_token(wd_device);
	if (wd_device->started) {	
		switch (state)
		{
			case WD17XX_DRQ_SET:
			{
				beta->betadisk_status |= (1<<6);
			}
			break;
	
			case WD17XX_DRQ_CLR:
			{
				beta->betadisk_status &=~(1<<6);
			}
			break;
	
			case WD17XX_IRQ_SET:
			{
				beta->betadisk_status |= (1<<7);
			}
			break;
	
			case WD17XX_IRQ_CLR:
			{
				beta->betadisk_status &=~(1<<7);
			}
			break;
		}
	}
}

const wd17xx_interface beta_wd17xx_interface = { betadisk_wd179x_callback, NULL };

READ8_DEVICE_HANDLER(betadisk_status_r)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		return wd17xx_status_r(beta->wd179x, offset); 
	} else {
		return 0xff;
	}
}

READ8_DEVICE_HANDLER(betadisk_track_r)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		return wd17xx_track_r(beta->wd179x, offset); 
	} else {
		return 0xff;
	}
}

READ8_DEVICE_HANDLER(betadisk_sector_r)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		return wd17xx_sector_r(beta->wd179x, offset); 
	} else {
		return 0xff;
	}
}

READ8_DEVICE_HANDLER(betadisk_data_r)
{
	
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		return wd17xx_data_r(beta->wd179x, offset); 
	} else {
		return 0xff;
	}
}

READ8_DEVICE_HANDLER(betadisk_state_r)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		return beta->betadisk_status;
	} else {
		return 0xff;
	}
}

WRITE8_DEVICE_HANDLER(betadisk_param_w)
{ 
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
  		wd17xx_set_drive(beta->wd179x, data & 3);  
  		wd17xx_set_side (beta->wd179x,(data & 0x10) ? 0 : 1 );
  		wd17xx_set_density(beta->wd179x, data & 0x20 ? DEN_MFM_HI : DEN_FM_LO );
  		if ((data & 0x04) == 0) // reset
  		{
  			wd17xx_reset(beta->wd179x);	
  		}    		
  		beta->betadisk_status = (data & 0x3f) | beta->betadisk_status;
  	}
} 	

WRITE8_DEVICE_HANDLER(betadisk_command_w)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		wd17xx_command_w(beta->wd179x, offset, data);
	}	
}

WRITE8_DEVICE_HANDLER(betadisk_track_w)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		wd17xx_track_w(beta->wd179x, offset, data);
	}	
}

WRITE8_DEVICE_HANDLER(betadisk_sector_w)
{
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		wd17xx_sector_w(beta->wd179x, offset, data);
	}	
}

WRITE8_DEVICE_HANDLER(betadisk_data_w)
{	
	beta_disk_state *beta = get_safe_token(device);
	
	if (beta->betadisk_active==1) {
		wd17xx_data_w(beta->wd179x, offset, data);
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

static MACHINE_DRIVER_START( beta_disk )
	MDRV_WD179X_ADD("wd179x", beta_wd17xx_interface )
MACHINE_DRIVER_END

ROM_START( beta_disk )
	ROM_REGION( 0x4000, "beta", ROMREGION_LOADBYNAME )
	//ROM_LOAD( "trd501.rom",		0x0000, 0x4000, CRC(3e3cdd4c) )
	ROM_LOAD( "trd503.rom", 	0x0000, 0x4000, CRC(10751aba) SHA1(21695e3f2a8f796386ce66eea8a246b0ac44810c))
/*	ROM_LOAD( "trd504.rom",  	0x0000, 0x4000, CRC(ba310874) )
	ROM_LOAD( "trd504em.rom",  	0x0000, 0x4000, CRC(0d3f8b43) )
	ROM_LOAD( "trd504f.rom",  	0x0000, 0x4000, CRC(ab3100d8) )
	ROM_LOAD( "trd504s.rom",  	0x0000, 0x4000, CRC(522ebbd6) )
	ROM_LOAD( "trd504t.rom",  	0x0000, 0x4000, CRC(e212d1e0) )
	ROM_LOAD( "trd504tm.rom",  	0x0000, 0x4000, CRC(2334b8c6) )
	ROM_LOAD( "trd505.rom",  	0x0000, 0x4000, CRC(fdff3810) )
	ROM_LOAD( "trd505d.rom",	0x0000, 0x4000, CRC(31e4be08) )
	ROM_LOAD( "trd505h.rom",  	0x0000, 0x4000, CRC(9ba15549) )
	ROM_LOAD( "trd512.rom",		0x0000, 0x4000, CRC(b615d6c4) )
	ROM_LOAD( "trd512f.rom",	0x0000, 0x4000, CRC(edb74f8c) )
	ROM_LOAD( "trd513f.rom",	0x0000, 0x4000, CRC(6b1c17f3) )
	ROM_LOAD( "trd513fm.rom",	0x0000, 0x4000, CRC(bad0c0a0) )
	ROM_LOAD( "trd56661.rom",	0x0000, 0x4000, CRC(8528c789) )
	ROM_LOAD( "trd604.rom",		0x0000, 0x4000, CRC(d8882a8c) )
	ROM_LOAD( "trd605e.rom",	0x0000, 0x4000, CRC(56d3c2db) )
	ROM_LOAD( "trd605r.rom",	0x0000, 0x4000, CRC(f8816a47) )
	ROM_LOAD( "trd607m.rom",	0x0000, 0x4000, CRC(5a062f03) )
	ROM_LOAD( "trd608.rom",		0x0000, 0x4000, CRC(5c998d53) )
	ROM_LOAD( "trd609.rom",		0x0000, 0x4000, CRC(91028924) )
	ROM_LOAD( "trd610e.rom",	0x0000, 0x4000, CRC(95395ca4) ) */
ROM_END	


/*-------------------------------------------------
    DEVICE_START( beta_disk )
-------------------------------------------------*/

static DEVICE_START( beta_disk )
{
	beta_disk_state *beta = get_safe_token(device);
	astring *tempstring = astring_alloc();

	/* validate arguments */
	assert(device->tag != NULL);

	/* find our WD179x */
	astring_printf(tempstring, "%s:%s", device->tag, "wd179x");	
	beta->wd179x = devtag_get_device(device->machine, astring_c(tempstring));

	astring_free(tempstring);	
}

/*-------------------------------------------------
    DEVICE_RESET( beta_disk )
-------------------------------------------------*/

static DEVICE_RESET( beta_disk )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( beta_disk )
-------------------------------------------------*/

DEVICE_GET_INFO( beta_disk )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(beta_disk_state);							break;		
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(beta_disk);						break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(beta_disk);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(beta_disk);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(beta_disk);					break;
		
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Beta Disk Interface");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Beta Disk Interface");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
