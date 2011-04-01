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
#include "imagedev/cartslot.h"

class cbmb_state : public driver_device
{
public:
	cbmb_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard lines */
	int m_cbmb_keyline_a;
	int m_cbmb_keyline_b;
	int m_cbmb_keyline_c;

	int m_p500;
	int m_cbm700;
	int m_cbm_ntsc;
	UINT8 *m_videoram;
	UINT8 *m_basic;
	UINT8 *m_kernal;
	UINT8 *m_colorram;
	int m_keyline_a;
	int m_keyline_b;
	int m_keyline_c;
	UINT8 *m_chargen;
	int m_old_level;
	int m_irq_level;
	int m_font;
};

/*----------- defined in machine/cbmb.c -----------*/

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
void cbmb_irq(device_t *device, int level);

int cbmb_dma_read(running_machine &machine, int offset);
int cbmb_dma_read_color(running_machine &machine, int offset);

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
SCREEN_UPDATE( cbmb_crtc );
MC6845_UPDATE_ROW( cbm600_update_row );
MC6845_UPDATE_ROW( cbm700_update_row );
WRITE_LINE_DEVICE_HANDLER( cbmb_display_enable_changed );

void cbm600_vh_init(running_machine &machine);
void cbm700_vh_init(running_machine &machine);
VIDEO_START( cbm700 );

void cbmb_vh_set_font(running_machine &machine, int font);


#endif /* CBMB_H_ */
