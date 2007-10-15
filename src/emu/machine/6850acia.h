#ifndef ACIA6850_H
#define ACIA6850_H

#define MAX_ACIA 4

struct acia6850_interface
{
	int	tx_clock;
	int	rx_clock;

	UINT8 *rx_pin;
	UINT8 *tx_pin;

	void (*int_callback)(int state);
};

void acia6850_config(int which, const struct acia6850_interface *intf);

WRITE8_HANDLER( acia6850_0_ctrl_w );
WRITE8_HANDLER( acia6850_1_ctrl_w );
WRITE8_HANDLER( acia6850_2_ctrl_w );
WRITE8_HANDLER( acia6850_3_ctrl_w );

WRITE8_HANDLER( acia6850_0_data_w );
WRITE8_HANDLER( acia6850_1_data_w );
WRITE8_HANDLER( acia6850_2_data_w );
WRITE8_HANDLER( acia6850_3_data_w );

READ8_HANDLER( acia6850_0_stat_r );
READ8_HANDLER( acia6850_1_stat_r );
READ8_HANDLER( acia6850_2_stat_r );
READ8_HANDLER( acia6850_3_stat_r );

READ8_HANDLER( acia6850_0_data_r );
READ8_HANDLER( acia6850_1_data_r );
READ8_HANDLER( acia6850_2_data_r );
READ8_HANDLER( acia6850_3_data_r );

READ16_HANDLER( acia6850_0_stat_lsb_r );
READ16_HANDLER( acia6850_1_stat_lsb_r );
READ16_HANDLER( acia6850_2_stat_lsb_r );
READ16_HANDLER( acia6850_3_stat_lsb_r );

READ16_HANDLER( acia6850_0_stat_msb_r );
READ16_HANDLER( acia6850_1_stat_msb_r );
READ16_HANDLER( acia6850_2_stat_msb_r );
READ16_HANDLER( acia6850_3_stat_msb_r );

READ16_HANDLER( acia6850_0_data_lsb_r );
READ16_HANDLER( acia6850_1_data_lsb_r );
READ16_HANDLER( acia6850_2_data_lsb_r );
READ16_HANDLER( acia6850_3_data_lsb_r );

READ16_HANDLER( acia6850_0_data_msb_r );
READ16_HANDLER( acia6850_1_data_msb_r );
READ16_HANDLER( acia6850_2_data_msb_r );
READ16_HANDLER( acia6850_3_data_msb_r );

WRITE16_HANDLER( acia6850_0_ctrl_msb_w );
WRITE16_HANDLER( acia6850_1_ctrl_msb_w );
WRITE16_HANDLER( acia6850_2_ctrl_msb_w );
WRITE16_HANDLER( acia6850_3_ctrl_msb_w );

WRITE16_HANDLER( acia6850_0_ctrl_lsb_w );
WRITE16_HANDLER( acia6850_1_ctrl_lsb_w );
WRITE16_HANDLER( acia6850_2_ctrl_lsb_w );
WRITE16_HANDLER( acia6850_3_ctrl_lsb_w );

WRITE16_HANDLER( acia6850_0_data_msb_w );
WRITE16_HANDLER( acia6850_1_data_msb_w );
WRITE16_HANDLER( acia6850_2_data_msb_w );
WRITE16_HANDLER( acia6850_3_data_msb_w );

WRITE16_HANDLER( acia6850_0_data_lsb_w );
WRITE16_HANDLER( acia6850_1_data_lsb_w );
WRITE16_HANDLER( acia6850_2_data_lsb_w );
WRITE16_HANDLER( acia6850_3_data_lsb_w );
#endif
