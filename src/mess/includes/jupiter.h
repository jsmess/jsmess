#pragma once

#ifndef __JUPITER__
#define __JUPITER__

#define MCM6571AP_TAG	"vid125_6c"
#define S6820_TAG		"vid125_4a"
#define Z80_TAG			"cpu126_4c"
#define INS1771N1_TAG	"fdi027_4c"
#define MC6820P_TAG		"fdi027_4b"
#define MC6850P_TAG		"rsi068_6a"
#define MC6821P_TAG		"sdm058_4b"

class jupiter2_state : public driver_device
{
public:
	jupiter2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MCM6571AP_TAG)
	{ }

	required_device<cpu_device> m_maincpu;

	virtual void machine_start();
};

class jupiter3_state : public driver_device
{
public:
	jupiter3_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, Z80_TAG)
	{ }

	required_device<cpu_device> m_maincpu;

	virtual void machine_start();
};

#endif
