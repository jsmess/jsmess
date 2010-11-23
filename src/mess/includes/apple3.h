/*****************************************************************************
 *
 * includes/apple3.h
 *
 * Apple ///
 *
 ****************************************************************************/

#ifndef APPLE3_H_
#define APPLE3_H_

#include "machine/applefdc.h"
#include "machine/6522via.h"

#define VAR_VM0			0x0001
#define VAR_VM1			0x0002
#define VAR_VM2			0x0004
#define VAR_VM3			0x0008
#define VAR_EXTA0		0x0010
#define VAR_EXTA1		0x0020
#define VAR_EXTPOWER	0x0040
#define VAR_EXTSIDE		0x0080


class apple3_state : public driver_device
{
public:
	apple3_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT32 flags;
	UINT8 via_0_a;
	UINT8 via_0_b;
	UINT8 via_1_a;
	UINT8 via_1_b;
	int via_1_irq;
	int enable_mask;
	offs_t zpa;
	int profile_lastaddr;
	UINT8 profile_gotstrobe;
	UINT8 profile_readdata;
	UINT8 profile_busycount;
	UINT8 profile_busy;
	UINT8 profile_online;
	UINT8 profile_writedata;
	UINT8 last_n;
	UINT8 *char_mem;
	UINT32 *hgr_map;
};


/*----------- defined in machine/apple3.c -----------*/

extern const applefdc_interface apple3_fdc_interface;
extern const via6522_interface apple3_via_0_intf;
extern const via6522_interface apple3_via_1_intf;

MACHINE_RESET( apple3 );
DRIVER_INIT( apple3 );
INTERRUPT_GEN( apple3_interrupt );

READ8_HANDLER( apple3_00xx_r );
WRITE8_HANDLER( apple3_00xx_w );

READ8_HANDLER( apple3_indexed_read );
WRITE8_HANDLER( apple3_indexed_write );


/*----------- defined in video/apple3.c -----------*/

VIDEO_START( apple3 );
VIDEO_UPDATE( apple3 );
void apple3_write_charmem(running_machine *machine);


#endif /* APPLE3_H_ */
