/*****************************************************************************
 *
 * includes/cybiko.h
 *
 * Cybiko Wireless Inter-tainment System
 *
 * (c) 2001-2007 Tim Schuerewegen
 *
 * Cybiko Classic (V1)
 * Cybiko Classic (V2)
 * Cybiko Xtreme
 *
 ****************************************************************************/

#ifndef CYBIKO_H_
#define CYBIKO_H_


typedef struct
{
	int sck; // serial clock
	int txd; // transmit data
	int rxd; // receive data
} CYBIKO_RS232_PINS;

typedef struct
{
	CYBIKO_RS232_PINS pin;
	UINT8 rx_bits, rx_byte, tx_byte, tx_bits;
} CYBIKO_RS232;

class cybiko_state : public driver_device
{
public:
	cybiko_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	CYBIKO_RS232 m_rs232;
};


/*----------- defined in machine/cybiko.c -----------*/

// driver init
DRIVER_INIT( cybikov1 );
DRIVER_INIT( cybikov2 );
DRIVER_INIT( cybikoxt );

// non-volatile ram handler
#if 0
NVRAM_HANDLER( cybikov1 );
NVRAM_HANDLER( cybikov2 );
NVRAM_HANDLER( cybikoxt );
#endif

// machine start
MACHINE_START( cybikov1 );
MACHINE_START( cybikov2 );
MACHINE_START( cybikoxt );

// machine reset
MACHINE_RESET( cybikov1 );
MACHINE_RESET( cybikov2 );
MACHINE_RESET( cybikoxt );

// lcd read/write
READ16_HANDLER( cybiko_lcd_r );
WRITE16_HANDLER( cybiko_lcd_w );

// key read
READ16_HANDLER( cybiko_key_r );

// unknown
WRITE16_HANDLER( cybiko_unk1_w );
READ16_HANDLER( cybiko_unk2_r );

// onchip registers read/write
READ8_HANDLER( cybikov1_io_reg_r );
READ8_HANDLER( cybikov2_io_reg_r );
READ8_HANDLER( cybikoxt_io_reg_r );
WRITE8_HANDLER( cybikov1_io_reg_w );
WRITE8_HANDLER( cybikov2_io_reg_w );
WRITE8_HANDLER( cybikoxt_io_reg_w );


#endif /* CYBIKO_H_ */
