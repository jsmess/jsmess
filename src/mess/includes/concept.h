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

/* keyboard interface */
enum
{
	KeyQueueSize = 32,
	MaxKeyMessageLen = 1
};

typedef struct
{
	read8_space_func reg_read;
	write8_space_func reg_write;
	read8_space_func rom_read;
	write8_space_func rom_write;
} expansion_slot_t;


class concept_state : public driver_device
{
public:
	concept_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 *videoram;
	UINT8 pending_interrupts;
	char clock_enable;
	char clock_address;
	UINT8 KeyQueue[KeyQueueSize];
	int KeyQueueHead;
	int KeyQueueLen;
	UINT32 KeyStateSave[3];
	UINT8 fdc_local_status;
	UINT8 fdc_local_command;
	expansion_slot_t expansion_slots[4];
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
