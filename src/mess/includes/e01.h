#pragma once

#ifndef __E01__
#define __E01__

#include "emu.h"

#define R65C102_TAG		"r65c102"
#define R6522_TAG		"ic21"
#define WD2793_TAG		"ic20"
#define MC6854_TAG		"mc6854"
#define CENTRONICS_TAG	"centronics"

class e01_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, e01_state(machine)); }

	e01_state(running_machine &machine) { }

	/* devices */
	running_device *fdc;
};

#endif
