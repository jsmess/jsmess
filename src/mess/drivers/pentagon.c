#include "driver.h"
#include "includes/spectrum.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/tzx_cas.h"

extern MACHINE_START( scorpion );

/****************************************************************************************************/
/* pentagon */

static  READ8_HANDLER(pentagon_port_r)
{
	return 0x0ff;
}


static WRITE8_HANDLER(pentagon_port_w)
{
}



/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (pentagon_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( pentagon_port_r, pentagon_port_w )
ADDRESS_MAP_END

static MACHINE_RESET( pentagon )
{
	/* Bank 5 is always in 0x4000 - 0x7fff */
	memory_set_bankptr(2, mess_ram + (5<<14));

	/* Bank 2 is always in 0x8000 - 0xbfff */
	memory_set_bankptr(3, mess_ram + (2<<14));
}

static MACHINE_DRIVER_START( pentagon )
	MDRV_IMPORT_FROM( spectrum_128 )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(pentagon_io, 0)
	MDRV_MACHINE_START( scorpion )
	MDRV_MACHINE_RESET( pentagon )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START(pentagon)
	ROM_REGION(0x020000, REGION_CPU1, 0)
	ROM_CART_LOAD(0, "rom", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

SYSTEM_CONFIG_EXTERN(specpls3)

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( ????, pentagon, spectrum, 0,		pentagon,		spectrum,	0,		specpls3,	"???",		"Pentagon", GAME_NOT_WORKING)
