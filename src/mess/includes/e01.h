#pragma once

#ifndef __E01__
#define __E01__

#include "emu.h"

#define R65C102_TAG		"r65c102"
#define R6522_TAG		"ic21"
#define WD2793_TAG		"ic20"
#define MC6854_TAG		"mc6854"
#define CENTRONICS_TAG	"centronics"

class e01_state : public driver_device
{
public:
	e01_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* interrupt state */
	int adlc_ie;
	int hdc_ie;

	int rtc_irq;
	int via_irq;
	int hdc_irq;
	int fdc_drq;
	int adlc_irq;

	/* devices */
	running_device *fdc;
};

#endif
