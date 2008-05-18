/*

	X68000 custom SASI Hard Disk controller

 0xe96001 (R/W) - SASI data I/O
 0xe96003 (W)   - SEL signal high (0)
 0xe96003 (R)   - SASI status
                  - bit 4 = MSG - if 1, content of data line is a message
                  - bit 3 = Command / Data - if 1, content of data line is a command or status, otherwise it is data.
                  - bit 2 = I/O - if 0, Host -> Controller (Output), otherwise Controller -> Host (Input).
                  - bit 1 = BSY - if 1, HD is busy.
                  - bit 0 = REQ - if 1, host is demanding data transfer to the host.
 0xe96005 (W/O) - data is arbitrary (?)
 0xe96007 (W/O) - SEL signal low (1)

*/

#include "x68k_hdc.h"

DEVICE_START( x68k_hdc )
{
	sasi_ctrl_t* sasi = device->token;

	assert(device->machine != NULL);

	sasi->status = 0x00;
	sasi->phase = SASI_PHASE_BUSFREE;
}

WRITE16_DEVICE_HANDLER( x68k_hdc_w )
{
	// SASI HDC - HDDs are not a required system component, so this is something to be done later
	logerror("SASI: write to HDC, offset %04x, data %04x\n",offset,data);
}

READ16_DEVICE_HANDLER( x68k_hdc_r )
{
	logerror("SASI: [%08x] read from HDC, offset %04x\n",activecpu_get_pc(),offset);
	switch(offset)
	{
	case 0x00:
		return 0x00;
	case 0x01:
		return 0x00;
	case 0x02:
		return 0x00;
	case 0x03:
		return 0x00;
	default:
		return 0xff;
	}
}

DEVICE_SET_INFO( x68k_hdc )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO(x68k_hdc)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_OTHER;				break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(sasi_ctrl_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(x68k_hdc); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(x68k_hdc);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/*info->reset = DEVICE_RESET_NAME(x68k_hdc);*/	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "X68000 SASI Hard Disk Controller";	break;
		case DEVINFO_STR_FAMILY:						info->s = "SASI Hard Disk Controller";			break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}

