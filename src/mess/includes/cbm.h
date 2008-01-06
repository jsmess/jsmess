/*****************************************************************************
 *
 * includes/cbm.h
 *
 ****************************************************************************/

#ifndef CBM_H_
#define CBM_H_

#include "devices/snapquik.h"

#ifdef __cplusplus
extern "C" {
#endif


/* must be defined until some driver init problems are solved */
#define NEW_GAMEDRIVER


/*----------- defined in machine/cbm.c -----------*/

/* global header file for
 * vc20
 * c16
 * c64
 * c128
 * c65*/

void cbmcartslot_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info);

/**************************************************************************
 * Logging
 * call the XXX_LOG with XXX_LOG("info",("%fmt\n",args));
 * where "info" can also be 0 to append .."%fmt",args to a line.
 **************************************************************************/
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
#else
# define PET_TEST_CODE
#endif

#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)

QUICKLOAD_LOAD( cbm_pet1 );
QUICKLOAD_LOAD( cbm_pet );
QUICKLOAD_LOAD( cbm_c16 );
QUICKLOAD_LOAD( cbm_c64 );
QUICKLOAD_LOAD( cbm_vc20 );
QUICKLOAD_LOAD( cbmb );
QUICKLOAD_LOAD( cbm500 );
QUICKLOAD_LOAD( cbm_c65 );

#define CBM_QUICKLOAD_DELAY 3.0

typedef struct {
#define CBM_ROM_ADDR_UNKNOWN 0
#define CBM_ROM_ADDR_LO -1
#define CBM_ROM_ADDR_HI -2
	int addr, size;
	UINT8 *chip;
} CBM_ROM;


extern INT8 cbm_c64_game;
extern INT8 cbm_c64_exrom;
extern CBM_ROM cbm_rom[0x20];

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


#ifdef __cplusplus
}
#endif

#endif /* CBM_H_ */
