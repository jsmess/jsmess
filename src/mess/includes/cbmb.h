/*****************************************************************************
 *
 * includes/cbmb.h
 *
 * Commodore B Series Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 *
 ****************************************************************************/

#ifndef CBMB_H_
#define CBMB_H_

#include "video/mc6845.h"
#include "machine/6526cia.h"
#include "devices/cartslot.h"

class cbmb_state : public driver_device
{
public:
	cbmb_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard lines */
	int cbmb_keyline_a;
	int cbmb_keyline_b;
	int cbmb_keyline_c;

	int p500;
	int cbm700;
	int cbm_ntsc;
	UINT8 *videoram;
};

/*----------- defined in machine/cbmb.c -----------*/

extern UINT8 *cbmb_basic;
extern UINT8 *cbmb_kernal;
//extern UINT8 *cbmb_chargen;
//extern UINT8 *cbmb_memory;
extern UINT8 *cbmb_videoram;
extern UINT8 *cbmb_colorram;

extern const mos6526_interface cbmb_cia;

WRITE8_HANDLER ( cbmb_colorram_w );

READ8_DEVICE_HANDLER( cbmb_tpi0_port_a_r );
WRITE8_DEVICE_HANDLER( cbmb_tpi0_port_a_w );
READ8_DEVICE_HANDLER( cbmb_tpi0_port_b_r );
WRITE8_DEVICE_HANDLER( cbmb_tpi0_port_b_w );

WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_a );
WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_b );
WRITE8_DEVICE_HANDLER( cbmb_keyboard_line_select_c );
READ8_DEVICE_HANDLER( cbmb_keyboard_line_a );
READ8_DEVICE_HANDLER( cbmb_keyboard_line_b );
READ8_DEVICE_HANDLER( cbmb_keyboard_line_c );
void cbmb_irq(running_device *device, int level);

int cbmb_dma_read(running_machine *machine, int offset);
int cbmb_dma_read_color(running_machine *machine, int offset);

WRITE8_DEVICE_HANDLER( cbmb_change_font );

DRIVER_INIT( p500 );
DRIVER_INIT( cbm600 );
DRIVER_INIT( cbm600pal );
DRIVER_INIT( cbm600hu );
DRIVER_INIT( cbm700 );
MACHINE_RESET( cbmb );

MACHINE_CONFIG_EXTERN( cbmb_cartslot );


/*----------- defined in video/cbmb.c -----------*/

VIDEO_START( cbmb_crtc );
VIDEO_UPDATE( cbmb_crtc );
MC6845_UPDATE_ROW( cbm600_update_row );
MC6845_UPDATE_ROW( cbm700_update_row );
WRITE_LINE_DEVICE_HANDLER( cbmb_display_enable_changed );

void cbm600_vh_init(running_machine *machine);
void cbm700_vh_init(running_machine *machine);
VIDEO_START( cbm700 );

void cbmb_vh_set_font(int font);


#endif /* CBMB_H_ */
