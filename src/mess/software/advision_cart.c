/***************************************************************************

  Entex Industries Inc. Adventure Vision cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"

#define ADVISION_ROM_LOAD( set, name, offset, length, hash )	\
SOFTWARE_START( set ) \
	ROM_REGION( 0x1000, CARTRIDGE_REGION_ROM, ROMREGION_ERASEFF ) \
	ROM_LOAD(name, offset, length, hash) \
SOFTWARE_END

ADVISION_ROM_LOAD( defender,"defender.bin", 0, 0x1000, CRC(3e280096) SHA1(33e6bc08b3e8a951942e078102e90fbc8ba729a8) )
ADVISION_ROM_LOAD( turtles, "turtles.bin",  0, 0x1000, CRC(33a23eba) SHA1(88748cf4080646e0a2f115a7f6e6001916314816) )
ADVISION_ROM_LOAD( supcobra,"supcobra.bin", 0, 0x1000, CRC(b6045c9e) SHA1(fd059081d8d82f51dd1415cc3710f1fa5ebb1336) )
ADVISION_ROM_LOAD( spcforce,"spcforce.bin", 0, 0x1000, CRC(c70d4028) SHA1(86c1a6eb8a5070f4be32bf4e83c0d5951e831d27) )

SOFTWARE_LIST_START( advision_cart )
	SOFTWARE( defender, 0, 1982, "Entex Industries Inc.", "Defender",    0, 0 )    /* 6075 */
	SOFTWARE( turtles , 0, 1982, "Entex Industries Inc.", "Turtles",     0, 0 )    /* 6076 */
	SOFTWARE( supcobra, 0, 1982, "Entex Industries Inc.", "Super Cobra", 0, 0 )    /* 6077 */
	SOFTWARE( spcforce, 0, 1982, "Entex Industries Inc.", "Space Force", 0, 0 )    /* 6078 */
SOFTWARE_LIST_END

SOFTWARE_LIST( advision_cart, "Entex Industries Adventure Vision cartridges" )