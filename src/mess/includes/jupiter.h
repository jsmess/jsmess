/*****************************************************************************
 *
 * includes/jupiter.h
 *
 ****************************************************************************/

#ifndef JUPITER_H_
#define JUPITER_H_

typedef struct
{
	UINT8 hdr_type;
	UINT8 hdr_name[10];
	UINT16 hdr_len;
	UINT16 hdr_addr;
	UINT8 hdr_vars[8];
	UINT8 hdr_3c4c;
	UINT8 hdr_3c4d;
	UINT16 dat_len;
} jupiter_tape_t;


class jupiter_state : public driver_device
{
public:
	jupiter_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	emu_timer *set_irq_timer;
	emu_timer *clear_irq_timer;
	UINT8 *charram;
	UINT8 *expram;
	jupiter_tape_t tape;
};


/*----------- defined in machine/jupiter.c -----------*/


SNAPSHOT_LOAD(jupiter);


/*----------- defined in video/jupiter.c -----------*/

#endif /* JUPITER_H_ */
