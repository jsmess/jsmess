/*****************************************************************************
 *
 * includes/amstrad.h
 *
 ****************************************************************************/

#ifndef AMSTRAD_H_
#define AMSTRAD_H_

#include "machine/upd765.h"
#include "video/mc6845.h"
#include "devices/snapquik.h"

/*----------- defined in machine/amstrad.c -----------*/

READ8_HANDLER ( amstrad_cpc_io_r );
WRITE8_HANDLER ( amstrad_cpc_io_w );

WRITE8_HANDLER( amstrad_plus_asic_4000_w );
WRITE8_HANDLER( amstrad_plus_asic_6000_w );
READ8_HANDLER( amstrad_plus_asic_4000_r );
READ8_HANDLER( amstrad_plus_asic_6000_r );

READ8_DEVICE_HANDLER( amstrad_ppi_porta_r );
READ8_DEVICE_HANDLER( amstrad_ppi_portb_r );
WRITE8_DEVICE_HANDLER( amstrad_ppi_porta_w );
WRITE8_DEVICE_HANDLER( amstrad_ppi_portc_w );

READ8_HANDLER ( amstrad_psg_porta_read );

WRITE_LINE_DEVICE_HANDLER( aleste_interrupt );

MACHINE_RESET( amstrad );
MACHINE_RESET( kccomp );
MACHINE_START( plus );
MACHINE_RESET( plus );
MACHINE_RESET( gx4000 );
MACHINE_RESET( aleste );

SNAPSHOT_LOAD( amstrad );

DEVICE_IMAGE_LOAD(amstrad_plus_cartridge);

extern const mc6845_interface mc6845_amstrad_intf;
extern const mc6845_interface mc6845_amstrad_plus_intf;

VIDEO_START( amstrad );
VIDEO_UPDATE( amstrad );
VIDEO_EOF( amstrad );

PALETTE_INIT( amstrad_cpc );			/* For CPC464, CPC664, and CPC6128 */
PALETTE_INIT( amstrad_cpc_green );		/* For CPC464, CPC664, and CPC6128 */
PALETTE_INIT( kccomp );					/* For KC Compact */
PALETTE_INIT( amstrad_plus );			/* For CPC464+ and CPC6128+ */
PALETTE_INIT( aleste );					/* For aleste */

#endif /* AMSTRAD_H_ */
