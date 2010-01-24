/***************************************************************************

  Super A'Can cartridges

***************************************************************************/

#include "emu.h"
#include "softlist.h"
#include "devices/cartslot.h"


/* F002 - Sango Fighter */
SOFTWARE_START( sangofgt )
	ROM_REGION( 0x300000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "sangofgt.bin", 0, 0x300000, CRC(a4de6dde) SHA1(f4bed63775130a75eb9c50b32e0cf50d1a7b8f50) )
SOFTWARE_END

/* F005 - Super Taiwanese Baseball League */
SOFTWARE_START( staiwbbl )
	ROM_REGION( 0x200000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "16005.0", 0, 0x200000, CRC(ccf6829b) SHA1(17a413803d8749fbe9643ca56d703afd64569b9f) )
SOFTWARE_END


/* F007 - Super Light Saga - Dragon Force */
SOFTWARE_START( slghtsag )
	ROM_REGION( 0x300000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "16007.0", 0x000000, 0x200000, CRC(56c1c3fb) SHA1(249e2ad6d8d40ecd31eda5a1bd5e5d0f47174a27) )
	ROM_LOAD( "08007.1", 0x200000, 0x100000, CRC(fc79f05f) SHA1(7ce2e23ea3fd25764935708be4d47bf1a9843938) )
SOFTWARE_END


/* F011 - Boom Zoo */
SOFTWARE_START( boomzoo )
	ROM_REGION( 0x80000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "boomzoo.bin", 0, 0x80000, CRC(6099bb44) SHA1(0b5fbe2117bb77a827453c5489b3af691e5c7ade) )
SOFTWARE_END


SOFTWARE_LIST_START( supracan_cart )
	/* F001 - Fomosa Duel */
	SOFTWARE( sangofgt, 0, 1995, "Panda Entertainment Technology", "Sango Fighter", 0, 0 )	/* F002 */
	/* F003 - 1995 - Funtech - The Son of Evil */
	/* F004 - Speedy Dragon/Sonic Dragon */
	SOFTWARE( staiwbbl, 0, 1995, "C&E Soft", "Super Taiwanese Baseball League", 0, 0 )	/* F005 */
	/* F006 - 1995 - Funtech - Journey to the Laugh */
	SOFTWARE( slghtsag, 0, 1996, "Kingformation", "Super Light Saga - Dragon Force", 0, 0 )	/* F007 */
	/* F008 - 1995 - Funtech - Monopoly: Adventure in Africa */
	/* F009 - Gambling Lord */
	/* F010 - Magical Pool */
	SOFTWARE( boomzoo,   0, 1996, "Funtech", "Boom Zoo", 0, 0 )	/* F011 */
	/* F012 - Rebel Star */
SOFTWARE_LIST_END


SOFTWARE_LIST( supracan_cart, "Super A'Can cartridges" )

