/*****************************************************************************
 *
 * includes/sym1.h
 *
 * Synertek Systems Corp. SYM-1
 *
 * Early driver by PeT <mess@utanet.at>, May 2000
 * Rewritten by Dirk Best, October 2007
 *
 ****************************************************************************/

#ifndef SYM1_H_
#define SYM1_H_

#include "machine/6532riot.h"
#include "machine/6522via.h"
#include "machine/74145.h"

/* SYM-1 main (and only) oscillator Y1 */
#define SYM1_CLOCK  XTAL_1MHz


class sym1_state : public driver_device
{
public:
	sym1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram_1k;
	UINT8 *m_ram_2k;
	UINT8 *m_ram_3k;
	UINT8 *m_riot_ram;
	UINT8 *m_monitor;
	UINT8 m_riot_port_a;
	UINT8 m_riot_port_b;
	emu_timer *m_led_update;
};


/*----------- defined in drivers/sym1.c -----------*/

/* Pointer to the monitor ROM, which includes the reset vectors for the CPU */


/*----------- defined in machine/sym1.c -----------*/

extern const riot6532_interface sym1_r6532_interface;
extern const ttl74145_interface sym1_ttl74145_intf;
extern const via6522_interface sym1_via0;
extern const via6522_interface sym1_via1;
extern const via6522_interface sym1_via2;

DRIVER_INIT( sym1 );
MACHINE_RESET( sym1 );


#endif /* SYM1_H_ */
