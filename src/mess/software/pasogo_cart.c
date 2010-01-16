/***************************************************************************

  Koei PasoGo cartridges

***************************************************************************/

#include "driver.h"
#include "softlist.h"
#include "devices/cartslot.h"


SOFTWARE_START( igmks1 )
	ROM_REGION( 0x00080000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "yrm0442m-184s", 0x00000000, 0x00080000, CRC(32f9c38a) SHA1(1be82afcdf5e2d1a0e873fda4161e663d7a53a85) )
SOFTWARE_END

SOFTWARE_START( tnt1 )
	ROM_REGION( 0x00080000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "ks-1009.ic4", 0x00000000, 0x00080000, CRC(3e70fca6) SHA1(c46cdc9e01f2f5c66b2523e1d30355e51c839f27) )
SOFTWARE_END

SOFTWARE_START( dgoban )
	ROM_REGION( 0x00100000, CARTRIDGE_REGION_ROM, 0 )
	ROM_LOAD( "ks-1010.ic4", 0x00000000, 0x00100000, CRC(b6a3f97c) SHA1(2de63b05ec93a4ba3ea55ba131c0706927a5bf39) )
SOFTWARE_END


SOFTWARE_LIST_START( pasogo_cart )
	SOFTWARE( igmks1,   0,         1996, "Koei", "Igo Meikyokushuu - Dai Ikkan", 0, 0 )		/* KS-1004. Contains 4M SOP40 ROM only. */
	SOFTWARE( tnt1,   0,         1996, "Koei", "Tsuyoku Naru Tesuji - Dai Ikkan", 0, 0 )	/* KS-1009. Contains 4M SOP40 ROM only. */
	SOFTWARE( dgoban,    0,         1996, "Koei", "Denshi Goban", 0, 0 )				/* KS-1010. Contians 8M SOP44 ROM, 62256 SOP32 RAM, Microchip 1081N (reset IC?watchdog?battery-power-switcher?) and 3V battery. */
SOFTWARE_LIST_END


SOFTWARE_LIST( pasogo_cart, "Koei PasoGo cartridges" )
