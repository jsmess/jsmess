#include "driver.h"
#include "cpu/tms0980/tms0980.h"

/* Layout */
#include "stopthie.lh"


static MACHINE_DRIVER_START( stopthie )
	MDRV_CPU_ADD( "maincpu", TMS0980, 5000000 )	/* Clock is wrong */

	MDRV_DEFAULT_LAYOUT(layout_stopthie)
MACHINE_DRIVER_END

ROM_START( stopthie )
	ROM_REGION( 0x1000, "maincpu", 0 )
	/* Taken from patent 4341385, might have made mistakes when creating this rom */
	ROM_LOAD16_WORD( "stopthie.bin", 0x0000, 0x1000, BAD_DUMP CRC(49ef83ad) SHA1(407151f707aa4a62b7e034a1bcb957c42ea36707) )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY            FULLNAME      FLAGS */
CONS( 1979, stopthie,   0,      0,      stopthie,   0,      0,      0,      "Parker Brothers", "Stop the Thief", GAME_NOT_WORKING )

