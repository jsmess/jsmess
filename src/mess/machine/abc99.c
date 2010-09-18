#include "emu.h"
#include "abc99.h"
#include "cpu/mcs48/mcs48.h"

/*-------------------------------------------------
    ADDRESS_MAP( abc99_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( abc99_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( abc99_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( abc99_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

/*-------------------------------------------------
    MACHINE_DRIVER( abc99 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( abc99 )
	MDRV_CPU_ADD("i8035", I8035, XTAL_4_608MHz)
	MDRV_CPU_PROGRAM_MAP(abc99_map)
	MDRV_CPU_IO_MAP(abc99_io_map)
MACHINE_CONFIG_END

/*-------------------------------------------------
    ROM( abc99 )
-------------------------------------------------*/

ROM_START( abc99 )
	ROM_REGION( 0x1800, "i8035", 0 )
	ROM_LOAD( "10681909.bin", 0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) )
	ROM_LOAD( "10726864.bin", 0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( abc99 )
-------------------------------------------------*/

static DEVICE_START( abc99 )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abc99 )
-------------------------------------------------*/

DEVICE_GET_INFO( abc99 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
			
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = rom_abc99;				break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(abc99); break;
			
		/* --- the following bits of info are returned as pointers to functions --- */
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
