/*
	Corvus Concept driver

	Raphael Nabet, 2003
*/

enum
{
	input_port_keyboard_concept = 0,
	dipswitch_port_concept = 6,
	display_orientation_concept = 7
};

MACHINE_RESET(concept);
VIDEO_START(concept);
VIDEO_UPDATE(concept);
INTERRUPT_GEN( concept_interrupt );
READ16_HANDLER(concept_io_r);
WRITE16_HANDLER(concept_io_w);
