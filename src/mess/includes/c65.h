/*****************************************************************************
 *
 * includes/c65.h
 *
 ****************************************************************************/

#ifndef C65_H_
#define C65_H_

#include "machine/6526cia.h"

typedef struct
{
	int version;
	UINT8 data[4];
} dma_t;

typedef struct
{
	int state;

	UINT8 reg[0x0f];

	UINT8 buffer[0x200];
	int cpu_pos;
	int fdc_pos;

	UINT16 status;

	attotime time;
	int head,track,sector;
} fdc_t;

typedef struct
{
	UINT8 reg;
} expansion_ram_t;

class c65_state : public driver_device
{
public:
	c65_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *chargen;
	UINT8 *interface;
	int charset_select;
	int c64mode;
	UINT8 _6511_port;
	UINT8 keyline;
	UINT8 vicirq;
	int old_level;
	int old_value;
	int old_data;
	int nmilevel;
	dma_t dma;
	int dump_dma;
	fdc_t fdc;
	expansion_ram_t expansion_ram;
	int io_on;
	int io_dc00_on;
};


/*----------- defined in machine/c65.c -----------*/

/*extern UINT8 *c65_memory; */
/*extern UINT8 *c65_basic; */
/*extern UINT8 *c65_kernal; */
/*extern UINT8 *c65_dos; */
/*extern UINT8 *c65_monitor; */
/*extern UINT8 *c65_graphics; */

void c65_bankswitch (running_machine *machine);
void c65_colorram_write (int offset, int value);

int c65_dma_read(running_machine *machine, int offset);
int c65_dma_read_color(running_machine *machine, int offset);
void c65_vic_interrupt(running_machine *machine, int level);
void c65_bankswitch_interface(running_machine *machine, int value);

DRIVER_INIT( c65 );
DRIVER_INIT( c65pal );
MACHINE_START( c65 );
INTERRUPT_GEN( c65_frame_interrupt );

extern const mos6526_interface c65_ntsc_cia0, c65_pal_cia0;
extern const mos6526_interface c65_ntsc_cia1, c65_pal_cia1;

#endif /* C65_H_ */
