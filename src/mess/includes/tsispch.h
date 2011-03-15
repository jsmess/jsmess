/***************************************************************************

        tsispch.h

****************************************************************************/

#pragma once

#ifndef _TSISPCH_H_
#define _TSISPCH_H_

#include "machine/terminal.h"

class tsispch_state : public driver_device
{
public:
	tsispch_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_dsp(*this, "dsp"),
		  m_terminal(*this, TERMINAL_TAG)
		{ }
		
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_dsp;
	required_device<device_t> m_terminal;

	UINT8 infifo[32];			// input fifo
	UINT8 infifo_tail_ptr;		// " tail
	UINT8 infifo_head_ptr;		// " head
	UINT8 statusLED;			// status led

	virtual void machine_reset();
	DECLARE_WRITE16_MEMBER(led_w);
	DECLARE_READ16_MEMBER(dsp_data_r);
	DECLARE_WRITE16_MEMBER(dsp_data_w);
	DECLARE_READ16_MEMBER(dsp_status_r);
	DECLARE_WRITE16_MEMBER(dsp_status_w);
};


#endif	// _TSISPCH_H_
