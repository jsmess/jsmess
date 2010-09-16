/*****************************************************************************
 *
 * includes/concept.h
 *
 * Corvus Concept driver
 *
 * Raphael Nabet, 2003
 *
 ****************************************************************************/

#ifndef CONCEPT_H_
#define CONCEPT_H_

#include "machine/6522via.h"
#include "machine/wd17xx.h"

class concept_state : public driver_device
{
public:
	concept_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *videoram;
};


/*----------- defined in machine/concept.c -----------*/

extern const via6522_interface concept_via6522_intf;
extern const wd17xx_interface concept_wd17xx_interface;

MACHINE_START(concept);
VIDEO_START(concept);
VIDEO_UPDATE(concept);
INTERRUPT_GEN( concept_interrupt );
READ16_HANDLER(concept_io_r);
WRITE16_HANDLER(concept_io_w);


#endif /* CONCEPT_H_ */
