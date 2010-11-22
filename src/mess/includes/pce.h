/*****************************************************************************
 *
 * includes/pce.h
 *
 * NEC PC Engine/TurboGrafx16
 *
 ****************************************************************************/

#ifndef PCE_H_
#define PCE_H_

#include "sound/msm5205.h"

#define C6280_TAG			"c6280"

#define	MAIN_CLOCK		21477270
#define PCE_CD_CLOCK	9216000

#define PCE_HEADER_SIZE		512

#define TG_16_JOY_SIG		0x00
#define PCE_JOY_SIG			0x40
#define NO_CD_SIG			0x80
#define CD_SIG				0x00
/* these might be used to indicate something, but they always seem to return 1 */
#define CONST_SIG			0x30

/* the largest possible cartridge image (street fighter 2 - 2.5MB) */
#define PCE_ROM_MAXSIZE		0x280000


/*----------- defined in machine/pce.c -----------*/

extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern UINT8 *pce_cd_ram;
DEVICE_IMAGE_LOAD(pce_cart);
NVRAM_HANDLER( pce );
WRITE8_HANDLER ( pce_joystick_w );
 READ8_HANDLER ( pce_joystick_r );

extern const msm5205_interface pce_cd_msm5205_interface;
WRITE8_HANDLER( pce_cd_bram_w );
WRITE8_HANDLER( pce_cd_intf_w );
READ8_HANDLER( pce_cd_intf_r );
WRITE8_HANDLER( pce_cd_acard_w );
READ8_HANDLER( pce_cd_acard_r );
WRITE8_HANDLER( pce_cd_acard_wram_w );
READ8_HANDLER( pce_cd_acard_wram_r );

DRIVER_INIT( pce );
DRIVER_INIT( tg16 );
DRIVER_INIT( sgx );
MACHINE_START( pce );
MACHINE_RESET( pce );


#endif /* PCE_H_ */
