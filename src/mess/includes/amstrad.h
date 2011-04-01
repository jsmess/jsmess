/*****************************************************************************
 *
 * includes/amstrad.h
 *
 ****************************************************************************/

#ifndef AMSTRAD_H_
#define AMSTRAD_H_

#include "machine/upd765.h"
#include "video/mc6845.h"
#include "imagedev/snapquik.h"

/****************************
 * Gate Array data (CPC) -
 ****************************/
typedef struct
{
	bitmap_t	*bitmap;		/* The bitmap we work on */
	UINT8	pen_selected;		/* Pen selection */
	UINT8	mrer;				/* Mode and ROM Enable Register */
	UINT8	upper_bank;

	/* input signals from CRTC */
	int		vsync;
	int		hsync;
	int		de;
	int		ma;
	int		ra;

	/* used for timing */
	int		hsync_after_vsync_counter;
	int		hsync_counter;				/* The gate array counts CRTC HSYNC pulses using an internal 6-bit counter. */

	/* used for drawing the screen */
	attotime	last_draw_time;
	int		y;
	UINT16	*draw_p;					/* Position in the bitmap where we are currently drawing */
	UINT16	colour;
	UINT16	address;
	UINT8	*mode_lookup;
	UINT8	data;
	UINT8	ticks;
	UINT8	ticks_increment;
	UINT16	line_ticks;
	UINT8	colour_ticks;
	UINT8	max_colour_ticks;
} gate_array_t;

/****************************
 * ASIC data (CPC plus)
 ****************************/
typedef struct
{
	UINT8	*ram;				/* pointer to RAM used for the CPC+ ASIC memory-mapped registers */
	UINT8	enabled;			/* Are CPC plus features enabled/unlocked */
	UINT8	pri;				/* Programmable raster interrupt */
	UINT8	seqptr;				/* Current position in the ASIC unlocking sequence */
	UINT8	rmr2;				/* ROM mapping register 2 */
	UINT16	split_ma_base;		/* Used to handle split screen support */
	UINT16	split_ma_started;	/* Used to handle split screen support */
	UINT16	vpos;				/* Current logical scanline */
	UINT16	h_start;			/* Position where DE became active */
	UINT16	h_end;				/* Position where DE became inactive */
	UINT8	addr_6845;			/* We need these to store a shadow copy of R1 of the mc6845 */
	UINT8	horiz_disp;
	UINT8	hscroll;
	UINT8   de_start;			/* flag to check if DE is been enabled this frame yet */

	/* DMA */
	UINT8	dma_status;
	UINT8	dma_clear;			/* Set if DMA interrupts are to be cleared automatically */
	UINT8	dma_prescaler[3];	/* DMA channel prescaler */
	UINT16	dma_repeat[3];		/* Location of the DMA channel's last repeat */
	UINT16	dma_addr[3];		/* DMA channel address */
	UINT16	dma_loopcount[3];	/* Count loops taken on this channel */
	UINT16	dma_pause[3];		/* DMA pause count */
} asic_t;


class amstrad_state : public driver_device
{
public:
	amstrad_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_system_type;
	UINT8 m_aleste_mode;
	int m_plus_irq_cause;
	gate_array_t m_gate_array;
	asic_t m_asic;
	int m_GateArray_RamConfiguration;
	unsigned char *m_AmstradCPC_RamBanks[4];
	unsigned char *m_Aleste_RamBanks[4];
	int m_aleste_active_page[4];
	unsigned char *m_Amstrad_ROM_Table[256];
	UINT8 m_ppi_port_inputs[3];
	UINT8 m_ppi_port_outputs[3];
	int m_aleste_rtc_function;
	int m_aleste_fdc_int;
	int m_prev_reg;
	UINT16 m_GateArray_render_colours[17];
	UINT8 m_mode0_lookup[256];
	UINT8 m_mode1_lookup[256];
	UINT8 m_mode2_lookup[256];
	unsigned char *m_multiface_ram;
	unsigned long m_multiface_flags;
	int m_prev_data;
	int m_printer_bit8_selected;
	unsigned char m_Psg_FunctionSelected;
	int m_previous_ppi_portc_w;
};


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

MACHINE_START( amstrad );
MACHINE_RESET( amstrad );
MACHINE_START( kccomp );
MACHINE_RESET( kccomp );
MACHINE_START( plus );
MACHINE_RESET( plus );
MACHINE_START( gx4000 );
MACHINE_RESET( gx4000 );
MACHINE_START( aleste );
MACHINE_RESET( aleste );

SNAPSHOT_LOAD( amstrad );

DEVICE_IMAGE_LOAD(amstrad_plus_cartridge);

extern const mc6845_interface amstrad_mc6845_intf;
extern const mc6845_interface amstrad_plus_mc6845_intf;

VIDEO_START( amstrad );
SCREEN_UPDATE( amstrad );
SCREEN_EOF( amstrad );

PALETTE_INIT( amstrad_cpc );			/* For CPC464, CPC664, and CPC6128 */
PALETTE_INIT( amstrad_cpc_green );		/* For CPC464, CPC664, and CPC6128 */
PALETTE_INIT( kccomp );					/* For KC Compact */
PALETTE_INIT( amstrad_plus );			/* For CPC464+ and CPC6128+ */
PALETTE_INIT( aleste );					/* For aleste */

#endif /* AMSTRAD_H_ */
