/***************************************************************************

        pes.h

****************************************************************************/

#pragma once

#ifndef _PES_H_
#define _PES_H_

#include "machine/terminal.h"

class pes_state : public driver_device
{
public:
	pes_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_terminal(*this, TERMINAL_TAG),
		  m_speech(*this, "tms5220")
		{ }
		
	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_speech;

	UINT8 m_wsstate;			// /WS
	UINT8 m_rsstate;			// /RS
	UINT8 m_port3_state;		// Port3 state as last written
	UINT8 m_infifo[32];			// input fifo
	UINT8 m_infifo_tail_ptr;		// " tail
	UINT8 m_infifo_head_ptr;		// " head

	virtual void machine_reset();
	DECLARE_WRITE8_MEMBER(rsws_w);
	DECLARE_WRITE8_MEMBER(port1_w);
	DECLARE_WRITE8_MEMBER(port3_w);
	DECLARE_READ8_MEMBER(port1_r);
	DECLARE_READ8_MEMBER(port3_r);
};


#endif	// _PES_H_
