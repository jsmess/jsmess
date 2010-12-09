#pragma once

#ifndef __TEK405X__
#define __TEK405X__

#define MC6800_TAG			"u61"
#define MC6820_Y_TAG		"u561"
#define MC6820_X_TAG		"u565"
#define MC6820_TAPE_TAG		"u361"
#define MC6820_KB_TAG		"u461"
#define MC6820_GPIB_TAG		"u265"
#define MC6820_COM_TAG		"u5"
#define MC6850_TAG			"u25"
#define IEEE488_TAG			"ieee488"
#define SCREEN_TAG			"screen"

#define AM2901A_TAG			"am2901a"

class tek4051_state : public driver_device
{
public:
	tek4051_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MC6800_TAG),
		  m_ram(*this, "messram")
	 { }

	required_device<cpu_device> m_maincpu;
	required_device<running_device> m_ram;

	virtual void machine_start();

	virtual void video_start();

	DECLARE_WRITE8_MEMBER( lbs_w );
};

class tek4052_state : public driver_device
{
public:
	tek4052_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, AM2901A_TAG),
		  m_ram(*this, "messram")
	 { }

	required_device<cpu_device> m_maincpu;
	required_device<running_device> m_ram;

	virtual void machine_start();

	virtual void video_start();
};

#endif
