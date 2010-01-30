/*****************************************************************************
 *
 * includes/cbm.h
 *
 ****************************************************************************/

#ifndef CBM_H_
#define CBM_H_

#include "devices/snapquik.h"
#include "devices/cassette.h"


/* global header file for c16, c64, c65, c128, vc20 */

/*----------- defined in machine/cbm.c -----------*/

/* keyboard lines */
extern UINT8 c64_keyline[10];
void cbm_common_interrupt( running_device *device );

UINT8 cbm_common_cia0_port_a_r( running_device *device, UINT8 output_b );
UINT8 cbm_common_cia0_port_b_r( running_device *device, UINT8 output_a );


/***********************************************

    CBM Quickloads

***********************************************/

QUICKLOAD_LOAD( cbm_pet1 );
QUICKLOAD_LOAD( cbm_pet );
QUICKLOAD_LOAD( cbm_c16 );
QUICKLOAD_LOAD( cbm_c64 );
QUICKLOAD_LOAD( cbm_vc20 );
QUICKLOAD_LOAD( cbmb );
QUICKLOAD_LOAD( p500 );
QUICKLOAD_LOAD( cbm_c65 );

#define CBM_QUICKLOAD_DELAY_SECONDS 3


/***********************************************

    CBM Cartridges

***********************************************/


#define CBM_ROM_ADDR_UNKNOWN 0
#define CBM_ROM_ADDR_LO -1
#define CBM_ROM_ADDR_HI -2

typedef struct {
	int addr, size, index, start;
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

extern const cassette_config cbm_cassette_config;


#endif /* CBM_H_ */
