#pragma once

#ifndef __E01__
#define __E01__

#include "emu.h"

#define R65C102_TAG		"r65c102"
#define R6522_TAG		"ic21"
#define WD2793_TAG		"ic20"
#define MC6854_TAG		"mc6854"
#define CENTRONICS_TAG	"centronics"

class e01_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, e01_state(machine)); }

	e01_state(running_machine &machine)
		: driver_data_t(machine) { }

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
