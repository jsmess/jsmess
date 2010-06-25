#include "emu.h"
#include "abc99.h"

/* ROMs */

ROM_START( abc99 )
	ROM_REGION( 0x1800, "maincpu", 0 )
	ROM_LOAD( "10681909.bin", 0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) )
	ROM_LOAD( "10726864.bin", 0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3) )
ROM_END

/* Device Interface */

static DEVICE_START( abc99 )
{
}

DEVICE_GET_INFO( abc99 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(abc99);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Luxor ABC-99");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Luxor ABC");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

DEFINE_LEGACY_DEVICE(ABC99, abc99);
