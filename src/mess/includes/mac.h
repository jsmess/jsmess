/*****************************************************************************
 *
 * includes/mac.h
 *
 * Macintosh driver declarations
 *
 ****************************************************************************/

#ifndef MAC_H_
#define MAC_H_

#include "machine/8530scc.h"
#include "machine/6522via.h"

/* tells which model is being emulated (set by macxxx_init) */
typedef enum
{
	MODEL_MAC_128K512K,
	MODEL_MAC_512KE,
	MODEL_MAC_PLUS,
	MODEL_MAC_SE,
	MODEL_MAC_SE_FDHD,
	MODEL_MAC_CLASSIC,
	MODEL_MAC_PORTABLE,
	MODEL_MAC_PB100,

	MODEL_MAC_II,		// Mac II class machines
	MODEL_MAC_II_FDHD,
	MODEL_MAC_IIX,
	MODEL_MAC_IICX,
	MODEL_MAC_IICI,
	MODEL_MAC_IISI,
	MODEL_MAC_IIFX,
	MODEL_MAC_SE30,

	MODEL_MAC_LC,		// LC class machines, generally all with the same memory map and the V8 gate array
	MODEL_MAC_LC_II,
	MODEL_MAC_LC_III,
	MODEL_MAC_CLASSIC_II,
	MODEL_MAC_COLOR_CLASSIC
} mac_model_t;

// video parameters
#define MAC_H_VIS	(512)
#define MAC_V_VIS	(342)
#define MAC_H_TOTAL	(704)		// (512+192)
#define MAC_V_TOTAL	(370)		// (342+28)

/*----------- defined in drivers/mac.c -----------*/
extern UINT32 *se30_vram;

/*----------- defined in machine/mac.c -----------*/

extern mac_model_t mac_model;
extern const via6522_interface mac_via6522_intf;
extern const via6522_interface mac_via6522_2_intf;
extern const via6522_interface mac_via6522_adb_intf;

MACHINE_START( macscsi );
MACHINE_RESET( mac );

DRIVER_INIT(mac128k512k);
DRIVER_INIT(mac512ke);
DRIVER_INIT(macplus);
DRIVER_INIT(macse);
DRIVER_INIT(macclassic);
DRIVER_INIT(maclc);
DRIVER_INIT(maciifdhd);
DRIVER_INIT(maciix);
DRIVER_INIT(maciicx);
DRIVER_INIT(maciici);
DRIVER_INIT(maciisi);
DRIVER_INIT(macse30);
DRIVER_INIT(macclassic2);
DRIVER_INIT(maclc2);

READ16_HANDLER ( mac_via_r );
WRITE16_HANDLER ( mac_via_w );
READ16_HANDLER ( mac_via2_r );
WRITE16_HANDLER ( mac_via2_w );
READ16_HANDLER ( mac_autovector_r );
WRITE16_HANDLER ( mac_autovector_w );
READ16_HANDLER ( mac_iwm_r );
WRITE16_HANDLER ( mac_iwm_w );
READ16_HANDLER ( mac_scc_r );
WRITE16_HANDLER ( mac_scc_w );
WRITE16_HANDLER ( mac_scc_2_w );
READ16_HANDLER ( macplus_scsi_r );
WRITE16_HANDLER ( macplus_scsi_w );
WRITE16_HANDLER ( macii_scsi_w );
READ32_HANDLER (macii_scsi_drq_r);
NVRAM_HANDLER( mac );
void mac_scc_ack(const device_config *device);
void mac_scc_mouse_irq( running_machine *machine, int x, int y );
void mac_fdc_set_enable_lines(const device_config *device, int enable_mask);


/*----------- defined in video/mac.c -----------*/

VIDEO_START( mac );
VIDEO_UPDATE( mac );
VIDEO_UPDATE( macse30 );
PALETTE_INIT( mac );

void mac_set_screen_buffer( int buffer );


/*----------- defined in audio/mac.c -----------*/

#define SOUND_MAC_SOUND DEVICE_GET_INFO_NAME(mac_sound)

DEVICE_GET_INFO( mac_sound );

void mac_enable_sound( const device_config *device, int on );
void mac_set_sound_buffer( const device_config *device, int buffer );
void mac_set_volume( const device_config *device, int volume );

void mac_sh_updatebuffer(const device_config *device);


#endif /* MAC_H_ */
