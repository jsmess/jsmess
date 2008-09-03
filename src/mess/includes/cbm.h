/*****************************************************************************
 *
 * includes/cbm.h
 *
 ****************************************************************************/

#ifndef CBM_H_
#define CBM_H_

#include "devices/snapquik.h"


/* global header file for c16, c64, c65, c128, vc20 */

/***********************************************

	CBM Debug Logging Macro

***********************************************/

/* call the XXX_LOG with XXX_LOG("info",("%fmt\n",args));			*
 * where "info" can also be 0 to append .."%fmt",args to a line.	*/

#define LOG(LEVEL,N,M,A)  \
        { \
	  if(LEVEL>=N) { \
	    if( M ) \
              logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()), (char*)M );\
	    logerror A; \
	  } \
        }

/* debugging level here for all on or off */
#if 1
# ifdef VERBOSE_DBG
#  undef VERBOSE_DBG
# endif
# if 1
#  define VERBOSE_DBG 0
# else
#  define VERBOSE_DBG 1
# endif
#endif

#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)


/***********************************************

	CBM Quickloads

***********************************************/

QUICKLOAD_LOAD( cbm_pet1 );
QUICKLOAD_LOAD( cbm_pet );
QUICKLOAD_LOAD( cbm_c16 );
QUICKLOAD_LOAD( cbm_c64 );
QUICKLOAD_LOAD( cbm_vc20 );
QUICKLOAD_LOAD( cbmb );
QUICKLOAD_LOAD( cbm500 );
QUICKLOAD_LOAD( cbm_c65 );

#define CBM_QUICKLOAD_DELAY_SECONDS 3


/***********************************************

	CBM Cartridges

***********************************************/


#define CBM_ROM_ADDR_UNKNOWN 0
#define CBM_ROM_ADDR_LO -1
#define CBM_ROM_ADDR_HI -2

typedef struct {
	int addr, size /*, bank*/;	// mi serve prob questo per implementare il corretto caricamento delle cart tipo >0!
	UINT8 *chip;
} CBM_ROM;



/* prg file format
 * sfx file format
 * sda file format
 * 0 lsb 16bit address
 * 2 chip data */

/* p00 file format (p00 .. p63, s00 .. s63, ..)
 * 0x0000 C64File
 * 0x0007 0
 * 0x0008 Name in commodore encoding?
 * 0x0018 0 0
 * 0x001a lsb 16bit address
 * 0x001c data */


/***********************************************

	CBM Datasette Tapes

***********************************************/

void datasette_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CBM_H_ */
