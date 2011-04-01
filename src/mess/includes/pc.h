/*****************************************************************************
 *
 * includes/pc.h
 *
 ****************************************************************************/

#ifndef PC_H_
#define PC_H_

#include "machine/ins8250.h"
#include "machine/i8255a.h"
#include "machine/8237dma.h"

class pc_state : public driver_device
{
public:
	pc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	device_t *m_maincpu;
	device_t *m_pic8259;
	device_t *m_dma8237;
	device_t *m_pit8253;
	/* U73 is an LS74 - dual flip flop */
	/* Q2 is set by OUT1 from the 8253 and goes to DRQ1 on the 8237 */
	UINT8	m_u73_q2;
	UINT8	m_out1;
	int m_dma_channel;
	UINT8 m_dma_offset[2][4];
	UINT8 m_pc_spkrdata;
	UINT8 m_pc_input;

	int						m_ppi_portc_switch_high;
	int						m_ppi_speaker;
	int						m_ppi_keyboard_clear;
	UINT8					m_ppi_keyb_clock;
	UINT8					m_ppi_portb;
	UINT8					m_ppi_clock_signal;
	UINT8					m_ppi_data_signal;
	UINT8					m_ppi_shift_register;
	UINT8					m_ppi_shift_enable;
};

/*----------- defined in machine/pc.c -----------*/

extern const i8237_interface ibm5150_dma8237_config;
extern const struct pit8253_config ibm5150_pit8253_config;
extern const struct pit8253_config pcjr_pit8253_config;
extern const struct pic8259_interface ibm5150_pic8259_config;
extern const struct pic8259_interface pcjr_pic8259_config;
extern const ins8250_interface ibm5150_com_interface[4];
extern const i8255a_interface ibm5150_ppi8255_interface;
extern const i8255a_interface ibm5160_ppi8255_interface;
extern const i8255a_interface pc_ppi8255_interface;
extern const i8255a_interface pcjr_ppi8255_interface;

UINT8 pc_speaker_get_spk(running_machine &machine);
void pc_speaker_set_spkrdata(running_machine &machine, UINT8 data);
void pc_speaker_set_input(running_machine &machine, UINT8 data);

void mess_init_pc_common( running_machine &machine, UINT32 flags, void (*set_keyb_int_func)(running_machine &, int), void (*set_hdc_int_func)(running_machine &,int,int));

WRITE8_HANDLER( pc_nmi_enable_w );
READ8_HANDLER( pcjr_nmi_enable_r );

READ8_HANDLER( pc_page_r );
WRITE8_HANDLER( pc_page_w );

WRITE8_HANDLER( ibm5150_kb_set_clock_signal );
WRITE8_HANDLER( ibm5150_kb_set_data_signal );

DRIVER_INIT( ibm5150 );
DRIVER_INIT( pccga );
DRIVER_INIT( pcmda );
DRIVER_INIT( europc );
DRIVER_INIT( bondwell );
DRIVER_INIT( pc200 );
DRIVER_INIT( ppc512 );
DRIVER_INIT( pc1512 );
DRIVER_INIT( pc1640 );
DRIVER_INIT( pc_vga );
DRIVER_INIT( t1000hx );
DRIVER_INIT( pcjr );

MACHINE_START( pc );
MACHINE_RESET( pc );
MACHINE_START( pcjr );
MACHINE_RESET( pcjr );

DEVICE_IMAGE_LOAD( pcjr_cartridge );

INTERRUPT_GEN( pc_frame_interrupt );
INTERRUPT_GEN( pc_vga_frame_interrupt );
INTERRUPT_GEN( pcjr_frame_interrupt );


READ8_HANDLER( pc_rtc_r );
WRITE8_HANDLER( pc_rtc_w );

READ16_HANDLER( pc16le_rtc_r );
WRITE16_HANDLER( pc16le_rtc_w );

void pc_rtc_init(running_machine &machine);

READ8_HANDLER ( pc_EXP_r );
WRITE8_HANDLER ( pc_EXP_w );

#endif /* PC_H_ */
