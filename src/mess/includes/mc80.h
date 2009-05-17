/*****************************************************************************
 *
 * includes/mc80.h
 *
 ****************************************************************************/

#ifndef MC80_H_
#define MC80_H_

#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"

/*----------- defined in machine/mc80.c -----------*/

/*****************************************************************************/
/*                            Implementation for MC80.2x                     */
/*****************************************************************************/

extern MACHINE_RESET(mc8020);
extern const z80ctc_interface mc8020_ctc_intf;
extern const z80pio_interface mc8020_z80pio_intf;

/*****************************************************************************/
/*                            Implementation for MC80.3x                     */
/*****************************************************************************/

extern WRITE8_HANDLER( zve_write_protect_w );
extern WRITE8_HANDLER( vis_w );
extern WRITE8_HANDLER( eprom_prog_w );
extern MACHINE_RESET(mc8030);
extern const z80pio_interface zve_z80pio_intf;
extern const z80pio_interface asp_z80pio_intf;
extern const z80ctc_interface zve_z80ctc_intf;
extern const z80ctc_interface asp_z80ctc_intf;
extern const z80sio_interface asp_z80sio_intf;


/*----------- defined in video/mc80.c -----------*/

/*****************************************************************************/
/*                            Implementation for MC80.2x                     */
/*****************************************************************************/

extern UINT8* mc8020_video_ram;

extern VIDEO_START( mc8020 );
extern VIDEO_UPDATE( mc8020 );

/*****************************************************************************/
/*                            Implementation for MC80.3x                     */
/*****************************************************************************/

extern UINT8 *mc8030_video_mem;

extern VIDEO_START( mc8030 );
extern VIDEO_UPDATE( mc8030 );

#endif /* MC80_H_ */
