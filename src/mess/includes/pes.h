/***************************************************************************

        pes.h

****************************************************************************/

#pragma once

#ifndef _PES_H_
#define _PES_H_

class pes_state : public driver_device
{
public:
	pes_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_tmslatch;			// latched data for speech chip
	UINT8 m_wsstate;			// /WS
	UINT8 m_rsstate;			// /RS
	UINT8 m_serial_cts;			// CTS

	virtual void machine_reset();
	DECLARE_WRITE8_MEMBER(rsws_w);
};


#endif	// _PES_H_
