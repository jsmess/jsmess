/***************************************************************************

  Casio PV-1000 cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( pooyan )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "pooyan.bin", 0x0000, 0x2000, CRC(bd6edc48) SHA1(606b7d603da23e70012b361d7ffb53ea026f88f5) )
SOFTWARE_END

SOFTWARE_START( amidar )
	ROM_REGION( 0x2000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "amidar.bin", 0x0000, 0x2000, CRC(c82628cc) SHA1(00322d4025fef19728194442f9e06a4cccd01933) )
SOFTWARE_END

SOFTWARE_START( exitemah )
	ROM_REGION( 0x4000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "exitemah.bin", 0x0000, 0x4000, CRC(0d2f829c) SHA1(6cc9e5d234bc1b483108ae4dc79d644802a64ef0) )
SOFTWARE_END


SOFTWARE_LIST_START( pv1000_cart )
	SOFTWARE( pooyan,   0,         1982, "Casio", "Pooyan", 0, 0 )		/* GPA-101 */
	/* Super Cobra GPA-102 */
	/* Tutankham GPA-103 */
	SOFTWARE( amidar,   0,         1982, "Casio", "Amidar", 0, 0 )		/* GPA-104 */
	/* Dig Dug GPA-105 */
	/* Warp & Warp GPA-106 */
	/* Turpin GPA-107 */
	/* GPA-108 */
	/* Pachinko-UFO GPA-109 */
	/* Fighting Bug GPA-110 */
	/* Space Panic GPA-111 */
	/* Naughty Boy GPA-112 */
	/* GPA-113 */
	/* Dirty Chameleon GPA-114 */
	SOFTWARE( exitemah, 0,         1982, "Casio", "Excite Mahjong", 0, 0 )	/* GPA-115 */
SOFTWARE_LIST_END


SOFTWARE_LIST( pv1000_cart, "Casio PV-1000 cartridges" )
