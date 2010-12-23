/*****************************************************************************
 *
 * includes/osbexec.h
 *
 ****************************************************************************/

#ifndef OSBEXEC_H_
#define OSBEXEC_H_

class osbexec_state : public driver_device
{
public:
	osbexec_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
};


#endif /* OSBEXEC_H_ */
