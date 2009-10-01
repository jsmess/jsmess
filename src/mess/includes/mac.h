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


// video parameters
#define MAC_H_VIS	(512)
#define MAC_V_VIS	(342)
#define MAC_H_TOTAL	(704)		// (512+192)
#define MAC_V_TOTAL	(370)		// (342+28)


/*----------- defined in machine/mac.c -----------*/

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
NVRAM_HANDLER( mac );
void mac_scc_ack(const device_config *device);
void mac_scc_mouse_irq( running_machine *machine, int x, int y );
void mac_fdc_set_enable_lines(const device_config *device, int enable_mask);


/*----------- defined in video/mac.c -----------*/

VIDEO_START( mac );
VIDEO_UPDATE( mac );
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
