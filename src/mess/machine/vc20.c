/***************************************************************************

	commodore vic20 home computer
	Peter Trauner
	(peter.trauner@jk.uni-linz.ac.at)

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/

#include <ctype.h>
#include <stdio.h>

#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"

#include "machine/6522via.h"
#include "video/vic6560.h"
#include "includes/vc1541.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/cbmdrive.h"

#include "includes/vc20.h"

#include "devices/cassette.h"
#include "devices/cartslot.h"

#include "deprecat.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

static UINT8 keyboard[8] =
{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static int via1_portb, via1_porta, via0_ca2;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

static int has_vc1541 = 0;
static int ieee = 0; /* ieee cartridge (interface and rom)*/
static UINT8 *vc20_rom_2000;
static UINT8 *vc20_rom_4000;
static UINT8 *vc20_rom_6000;
static UINT8 *vc20_rom_a000;

static emu_timer *datasette_timer;

UINT8 *vc20_memory_9400;


/** via 0 addr 0x9110
 ca1 restore key (low)
 ca2 cassette motor on
 pa0 serial clock in
 pa1 serial data in
 pa2 also joy 0 in
 pa3 also joy 1 in
 pa4 also joy 2 in
 pa5 also joybutton/lightpen in
 pa6 cassette switch in
 pa7 inverted serial atn out
 pa2 till pa6, port b, cb1, cb2 userport
 irq connected to m6502 nmi
*/
static void vc20_via0_irq (running_machine *machine, int level)
{
	cpu_set_input_line(machine->cpu[0], INPUT_LINE_NMI, level);
}

static READ8_HANDLER( vc20_via0_read_ca1 )
{
	return !( input_port_read(space->machine, "SPECIAL") & 0x02 );
}

static READ8_HANDLER( vc20_via0_read_ca2 )
{
	running_machine *machine = space->machine;
	DBG_LOG (1, "tape", ("motor read %d\n", via0_ca2));
	return via0_ca2;
}

static WRITE8_HANDLER( vc20_via0_write_ca2 )
{
	via0_ca2 = data ? 0 : 1;

	if(via0_ca2)
	{
		cassette_change_state(device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(datasette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
	}
	else
	{
		cassette_change_state(device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" ),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		timer_reset(datasette_timer, attotime_never);
	}
}

static  READ8_HANDLER( vc20_via0_read_porta )
{
	UINT8 value = 0xff;

	value &= ~(input_port_read(space->machine, "JOY") & 0x3c);

	/* to short to be recognized normally */
	/* should be reduced to about 1 or 2 microseconds */
	/*  if ((input_port_read(space->machine, "CTRLSEL") & 0x20 ) && (input_port_read(space->machine, "JOY") & 0x40) )  // i.e. LIGHTPEN_BUTTON
		value &= ~0x20; */
	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~0x01;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~0x02;

	if ((cassette_get_state(device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" )) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		value &= ~0x40;
	else
		value |=  0x40;

	return value;
}

static WRITE8_HANDLER( vc20_via0_write_porta )
{
	running_machine *machine = space->machine;
	cbm_serial_atn_write (serial_atn = !(data & 0x80));
	DBG_LOG (1, "serial out", ("atn %s\n", serial_atn ? "high" : "low"));
}

/* via 1 addr 0x9120
 * port a input from keyboard (low key pressed in line of matrix
 * port b select lines of keyboard matrix (low)
 * pb7 also joystick right in! (low)
 * pb3 also cassette write
 * ca1 cassette read
 * ca2 inverted serial clk out
 * cb1 serial srq in
 * cb2 inverted serial data out
 * irq connected to m6502 irq
 */
static void vc20_via1_irq (running_machine *machine, int level)
{
	cpu_set_input_line(machine->cpu[0], M6502_IRQ_LINE, level);
}

static READ8_HANDLER( vc20_via1_read_porta )
{
	int value = 0xff;

	if (!(via1_portb & 0x01))
		value &= keyboard[0];

	if (!(via1_portb & 0x02))
		value &= keyboard[1];

	if (!(via1_portb & 0x04))
		value &= keyboard[2];

	if (!(via1_portb & 0x08))
		value &= keyboard[3];

	if (!(via1_portb & 0x10))
		value &= keyboard[4];

	if (!(via1_portb & 0x20))
		value &= keyboard[5];

	if (!(via1_portb & 0x40))
		value &= keyboard[6];

	if (!(via1_portb & 0x80))
		value &= keyboard[7];

	return value;
}

static  READ8_HANDLER( vc20_via1_read_ca1 )
{
	UINT8 data = (cassette_input(device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" )) > +0.0) ? 1 : 0;
	return data;
}

static WRITE8_HANDLER( vc20_via1_write_ca2 )
{
	cbm_serial_clock_write (serial_clock = !data);
}

static  READ8_HANDLER( vc20_via1_read_portb )
{
	UINT8 value = 0xff;

	if (!(via1_porta & 0x80)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x80)) t &= ~0x80;
		if (!(keyboard[6] & 0x80)) t &= ~0x40;
		if (!(keyboard[5] & 0x80)) t &= ~0x20;
		if (!(keyboard[4] & 0x80)) t &= ~0x10;
		if (!(keyboard[3] & 0x80)) t &= ~0x08;
		if (!(keyboard[2] & 0x80)) t &= ~0x04;
		if (!(keyboard[1] & 0x80)) t &= ~0x02;
		if (!(keyboard[0] & 0x80)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x40)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x40)) t &= ~0x80;
		if (!(keyboard[6] & 0x40)) t &= ~0x40;
		if (!(keyboard[5] & 0x40)) t &= ~0x20;
		if (!(keyboard[4] & 0x40)) t &= ~0x10;
		if (!(keyboard[3] & 0x40)) t &= ~0x08;
		if (!(keyboard[2] & 0x40)) t &= ~0x04;
		if (!(keyboard[1] & 0x40)) t &= ~0x02;
		if (!(keyboard[0] & 0x40)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x20)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x20)) t &= ~0x80;
		if (!(keyboard[6] & 0x20)) t &= ~0x40;
		if (!(keyboard[5] & 0x20)) t &= ~0x20;
		if (!(keyboard[4] & 0x20)) t &= ~0x10;
		if (!(keyboard[3] & 0x20)) t &= ~0x08;
		if (!(keyboard[2] & 0x20)) t &= ~0x04;
		if (!(keyboard[1] & 0x20)) t &= ~0x02;
		if (!(keyboard[0] & 0x20)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x10)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x10)) t &= ~0x80;
		if (!(keyboard[6] & 0x10)) t &= ~0x40;
		if (!(keyboard[5] & 0x10)) t &= ~0x20;
		if (!(keyboard[4] & 0x10)) t &= ~0x10;
		if (!(keyboard[3] & 0x10)) t &= ~0x08;
		if (!(keyboard[2] & 0x10)) t &= ~0x04;
		if (!(keyboard[1] & 0x10)) t &= ~0x02;
		if (!(keyboard[0] & 0x10)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x08)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x08)) t &= ~0x80;
		if (!(keyboard[6] & 0x08)) t &= ~0x40;
		if (!(keyboard[5] & 0x08)) t &= ~0x20;
		if (!(keyboard[4] & 0x08)) t &= ~0x10;
		if (!(keyboard[3] & 0x08)) t &= ~0x08;
		if (!(keyboard[2] & 0x08)) t &= ~0x04;
		if (!(keyboard[1] & 0x08)) t &= ~0x02;
		if (!(keyboard[0] & 0x08)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x04)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x04)) t &= ~0x80;
		if (!(keyboard[6] & 0x04)) t &= ~0x40;
		if (!(keyboard[5] & 0x04)) t &= ~0x20;
		if (!(keyboard[4] & 0x04)) t &= ~0x10;
		if (!(keyboard[3] & 0x04)) t &= ~0x08;
		if (!(keyboard[2] & 0x04)) t &= ~0x04;
		if (!(keyboard[1] & 0x04)) t &= ~0x02;
		if (!(keyboard[0] & 0x04)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x02)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x02)) t &= ~0x80;
		if (!(keyboard[6] & 0x02)) t &= ~0x40;
		if (!(keyboard[5] & 0x02)) t &= ~0x20;
		if (!(keyboard[4] & 0x02)) t &= ~0x10;
		if (!(keyboard[3] & 0x02)) t &= ~0x08;
		if (!(keyboard[2] & 0x02)) t &= ~0x04;
		if (!(keyboard[1] & 0x02)) t &= ~0x02;
		if (!(keyboard[0] & 0x02)) t &= ~0x01;
		value &= t;
    }

	if (!(via1_porta & 0x01)) 
	{
		UINT8 t = 0xff;
		if (!(keyboard[7] & 0x01)) t &= ~0x80;
		if (!(keyboard[6] & 0x01)) t &= ~0x40;
		if (!(keyboard[5] & 0x01)) t &= ~0x20;
		if (!(keyboard[4] & 0x01)) t &= ~0x10;
		if (!(keyboard[3] & 0x01)) t &= ~0x08;
		if (!(keyboard[2] & 0x01)) t &= ~0x04;
		if (!(keyboard[1] & 0x01)) t &= ~0x02;
		if (!(keyboard[0] & 0x01)) t &= ~0x01;
		value &= t;
    }

	value &= ~(input_port_read(space->machine, "JOY") & 0x80);

	return value;
}

static WRITE8_HANDLER( vc20_via1_write_porta )
{
	via1_porta = data;
}


static WRITE8_HANDLER( vc20_via1_write_portb )
{
/*  logerror("via1_write_portb: $%02X\n", data); */
	cassette_output(device_list_find_by_tag( space->machine->config->devicelist, CASSETTE, "cassette" ), (data & 0x08) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
	via1_portb = data;
}

static READ8_HANDLER( vc20_via1_read_cb1 )
{
	running_machine *machine = space->machine;
	DBG_LOG (1, "serial in", ("request read\n"));
	return cbm_serial_request_read ();
}

static WRITE8_HANDLER( vc20_via1_write_cb2 )
{
	cbm_serial_data_write (serial_data = !data);
}

/* ieee 6522 number 1 (via4)
 port b
  0 dav out
  1 nrfd out
  2 ndac out (or port a pin 2!?)
  3 eoi in
  4 dav in
  5 nrfd in
  6 ndac in
  7 atn in
 */
static READ8_HANDLER( vc20_via4_read_portb )
{
	UINT8 data = 0;
	if (cbm_ieee_eoi_r(space->machine)) data |= 0x08;
	if (cbm_ieee_dav_r(space->machine)) data |= 0x10;
	if (cbm_ieee_nrfd_r(space->machine)) data |= 0x20;
	if (cbm_ieee_ndac_r(space->machine)) data |= 0x40;
	if (cbm_ieee_atn_r(space->machine)) data |= 0x80;
	return data;
}

static WRITE8_HANDLER( vc20_via4_write_portb )
{
	cbm_ieee_dav_w(0, data & 0x01);
	cbm_ieee_nrfd_w(0, data & 0x02);
	cbm_ieee_ndac_w(0, data & 0x04);
}

/* ieee 6522 number 2 (via5)
   port a data read
   port b data write
   cb1 srq in ?
   cb2 eoi out
   ca2 atn out
*/
static WRITE8_HANDLER( vc20_via5_write_porta )
{
	cbm_ieee_data_w(0, data);
}

static READ8_HANDLER( vc20_via5_read_portb )
{
	return cbm_ieee_data_r(space->machine);
}

static WRITE8_HANDLER( vc20_via5_write_ca2 )
{
	cbm_ieee_atn_w(0, data);
}

static READ8_HANDLER( vc20_via5_read_cb1 )
{
	return cbm_ieee_srq_r(space->machine);
}

static WRITE8_HANDLER( vc20_via5_write_cb2 )
{
	cbm_ieee_eoi_w(0, data);
}

static const struct via6522_interface via0 =
{
	vc20_via0_read_porta,
	0,								   /*via0_read_portb, */
	vc20_via0_read_ca1,
	0,								   /*via0_read_cb1, */
	vc20_via0_read_ca2,
	0,								   /*via0_read_cb2, */
	vc20_via0_write_porta,
	0,								   /*via0_write_portb, */
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	vc20_via0_write_ca2,
	0,								   /*via0_write_cb2, */
	vc20_via0_irq
}, via1 =
{
	vc20_via1_read_porta,
	vc20_via1_read_portb,
	vc20_via1_read_ca1,
	vc20_via1_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via1_write_porta,								   /*via1_write_porta, */
	vc20_via1_write_portb,
	0,                                 /*via1_write_ca1, */
	0,                                 /*via1_write_cb1, */
	vc20_via1_write_ca2,
	vc20_via1_write_cb2,
	vc20_via1_irq
},
/* via2,3 used by vc1541 and 2031 disk drives */
via4 =
{
	0, /*vc20_via4_read_porta, */
	vc20_via4_read_portb,
	0, /*vc20_via4_read_ca1, */
	0, /*vc20_via5_read_cb1, */
	0, /* via1_read_ca2 */
	0,								   /*via1_read_cb2, */
	0,								   /*via1_write_porta, */
	vc20_via4_write_portb,
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	0, /*vc20_via5_write_ca2, */
	0, /*vc20_via5_write_cb2, */
	vc20_via1_irq
}, via5 =
{
	0,/*vc20_via5_read_porta, */
	vc20_via5_read_portb,
	0, /*vc20_via5_read_ca1, */
	vc20_via5_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via5_write_porta,
	0,/*vc20_via5_write_portb, */
	0,                                 /*via0_write_ca1, */
	0,                                 /*via0_write_cb1, */
	vc20_via5_write_ca2,
	vc20_via5_write_cb2,
	vc20_via1_irq
};

WRITE8_HANDLER ( vc20_write_9400 )
{
	vc20_memory_9400[offset] = data | 0xf0;
}


int vic6560_dma_read_color (int offset)
{
	return vc20_memory_9400[offset & 0x3ff];
}

int vic6560_dma_read (int offset)
{
	const address_space *space = cputag_get_address_space(Machine, "main", ADDRESS_SPACE_PROGRAM);
	/* should read real system bus between 0x9000 and 0xa000 */
	return memory_read_byte(space, VIC6560ADDR2VC20ADDR (offset));
}

WRITE8_HANDLER( vc20_0400_w ) 
{
	if ( mess_ram_size >= 8 * 1024 ) 
	{
		mess_ram[ 0x0400 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_2000_w ) 
{
	if ( ( mess_ram_size >= 16 * 1024 ) && vc20_rom_2000 == NULL ) 
	{
		mess_ram[ 0x2000 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_4000_w ) 
{
	if ( ( mess_ram_size >= 24 * 1024 ) && vc20_rom_4000 == NULL ) 
	{
		mess_ram[ 0x4000 + offset ] = data;
	}
}

WRITE8_HANDLER( vc20_6000_w ) 
{
	if ( ( mess_ram_size >= 32 * 1024 ) && vc20_rom_6000 == NULL ) 
	{
		mess_ram[ 0x6000 + offset ] = data;
	}
}


static void vc20_memory_init(running_machine *machine)
{
	static int inited = 0;
	UINT8 *memory = memory_region (machine, "main");

	if (inited) return;
	/* power up values are random (more likely bit set)
	   measured in cost reduced german vc20
	   6116? 2kbx8 ram in main area
	   2114 1kbx4 ram at non used color ram area 0x9400 */

	memset(memory, 0, 0x400);
	memset(memory + 0x1000, 0, 0x1000);

	memory[0x288]  = 0xff;		// makes ae's graphics look correctly
	memory[0xd]    = 0xff;		// for moneywars
	memory[0x1046] = 0xff;		// for jelly monsters, cosmic cruncher;

#if 0
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < 0x400; i += 0x40)
	{
		memset (memory + i, i & 0x40 ? 0 : 0xff, 0x40);
		memset (memory + 0x9400 + i, 0xf0 | (i & 0x40 ? 0 : 0xf), 0x40);
	}
	// this would be the straight forward memory init for
	// non cost reduced vic20 (2114 rams)
	for (i = 0x1000; i < 0x2000; i += 0x40) 
		memset(memory + i, i & 0x40 ? 0 : 0xff, 0x40);
#endif

	/* clears ieee cartrige rom */
	/* memset (memory + 0xa000, 0xff, 0x2000); */

	inited = 1;
}


static TIMER_CALLBACK( vic20_tape_timer )
{
	const address_space *space = cputag_get_address_space(machine, "main", ADDRESS_SPACE_PROGRAM);
	UINT8 data = (cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" )) > +0.0) ? 1 : 0;
	via_1_ca1_w(space, 0, data);
}

static void vc20_common_driver_init (running_machine *machine)
{
	vc20_memory_init(machine);

	datasette_timer = timer_alloc(machine, vic20_tape_timer, NULL);

	if (has_vc1541)
		drive_config (type_1541, 0, 0, 1, 8);

	via_config (0, &via0);
	via_config (1, &via1);
}

DRIVER_INIT( vc20 )
{
	vc20_common_driver_init (machine);
	vic6561_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic20 )
{
	vc20_common_driver_init (machine);
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic1001 )
{
	DRIVER_INIT_CALL( vic20 );
}

DRIVER_INIT( vc20v )
{
	has_vc1541 = 1;
	vc20_common_driver_init (machine);
	vic6561_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic20v )
{
	has_vc1541 = 1;
	vc20_common_driver_init (machine);
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
}

DRIVER_INIT( vic20i )
{
	ieee = 1;
	vc20_common_driver_init (machine);
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
	via_config (4, &via4);
	via_config (5, &via5);
	cbm_ieee_open();
}

MACHINE_RESET( vic20 )
{
	const address_space *space = cputag_get_address_space(machine, "main", ADDRESS_SPACE_PROGRAM);

	if (has_vc1541)
	{
		serial_config(machine, &fake_drive_interface);
		drive_reset ();
	}
	else
	{
		serial_config(machine, &sim_drive_interface);
		cbm_serial_reset_write (0);

		if (ieee) 
		{
			cbm_drive_0_config (IEEE, 8);
			cbm_drive_1_config (IEEE, 9);
		} 
		else 
		{
			cbm_drive_0_config (SERIAL, 8);
			cbm_drive_1_config (SERIAL, 9);
		}
	}

	via_reset ();
	via_0_ca1_w(space, 0, vc20_via0_read_ca1(space, 0));

	/* Set up memory banks */
	memory_set_bankptr (machine,  1, ( ( mess_ram_size >=  8 * 1024 ) ? mess_ram : memory_region(machine, "main") ) + 0x0400 );
	memory_set_bankptr (machine,  2, vc20_rom_2000 ? vc20_rom_2000 : ( ( ( mess_ram_size >= 16 * 1024 ) ? mess_ram : memory_region(machine, "main") ) + 0x2000 ) );
	memory_set_bankptr (machine,  3, vc20_rom_4000 ? vc20_rom_4000 : ( ( ( mess_ram_size >= 24 * 1024 ) ? mess_ram : memory_region(machine, "main") ) + 0x4000 ) );
	memory_set_bankptr (machine,  4, vc20_rom_6000 ? vc20_rom_6000 : ( ( ( mess_ram_size >= 32 * 1024 ) ? mess_ram : memory_region(machine, "main") ) + 0x6000 ) );
	memory_set_bankptr (machine,  5, vc20_rom_a000 ? vc20_rom_a000 : ( memory_region(machine, "main") + 0xa000 ) );
}


static TIMER_CALLBACK( lightpen_tick )
{
	if ((input_port_read(machine, "CTRLSEL") & 0xf0) == 0x20)
	{
		/* enable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable lightpen crosshair */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}
}

INTERRUPT_GEN( vic20_frame_interrupt )
{
	const address_space *space = cputag_get_address_space(device->machine, "main", ADDRESS_SPACE_PROGRAM);

	via_0_ca1_w(space, 0, vc20_via0_read_ca1(space, 0));
	keyboard[0] = input_port_read(device->machine, "ROW0");
	keyboard[1] = input_port_read(device->machine, "ROW1");
	keyboard[2] = input_port_read(device->machine, "ROW2");
	keyboard[3] = input_port_read(device->machine, "ROW3");
	keyboard[4] = input_port_read(device->machine, "ROW4");
	keyboard[5] = input_port_read(device->machine, "ROW5");
	keyboard[6] = input_port_read(device->machine, "ROW6");
	keyboard[7] = input_port_read(device->machine, "ROW7");

	/* check if lightpen has been chosen as input: if so, enable crosshair */
	timer_set(device->machine, attotime_zero, NULL, 0, lightpen_tick);

	set_led_status (1, input_port_read(device->machine, "SPECIAL") & 0x01 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

	VIC20 Cartridges

***********************************************/

static DEVICE_START(vic20_cart)
{
	vc20_memory_init(device->machine);
	vc20_rom_2000 = NULL;
	vc20_rom_4000 = NULL;
	vc20_rom_6000 = NULL;
	vc20_rom_a000 = NULL;
	return DEVICE_START_OK;
}

static DEVICE_IMAGE_LOAD(vic20_cart)
{
	int size = image_length(image), test;
	const char *filetype;
	const char *tag;
	int address = 0;

	static const unsigned char magic[] = {0x41, 0x30, 0x20, 0xc3, 0xc2, 0xcd};	/* A0 CBM at 0xa004 (module offset 4) */
	unsigned char buffer[sizeof (magic)];

	image_fseek (image, 4, SEEK_SET);
	image_fread (image, buffer, sizeof (magic));
	image_fseek (image, 0, SEEK_SET);

	/* Check if our cart has the magic string, and set its loading address according to its size */
	if (!memcmp (buffer, magic, sizeof (magic)))
		address = (size == 0x4000) ? 0x4000 : 0xa000;

	/* Give a loading address to non .bin / non .rom carts as well */
	/* We support .a0, .20, .40 and .60 files */
	filetype = image_filetype(image);

	if (!mame_stricmp (filetype, "a0"))
		address = 0xa000;

	else if (!mame_stricmp (filetype, "20"))
		address = 0x2000;

	else if (!mame_stricmp (filetype, "40"))
		address = 0x4000;

	else if (!mame_stricmp (filetype, "60"))
		address = 0x6000;

	/* As a last try, give a reasonable loading address also to .bin/.rom without the magic string */		
	else if (!address)
	{
		logerror("Cart %s does not contain the magic string: it may be loaded at the wrong memory address!\n", image_filename(image));
		address = (size == 0x4000) ? 0x4000 : 0xa000;
	}

	/* Decide the memory region we will load the cart in, to avoid problems when multiple carts need to be loaded */
	tag = (image_index_in_device(image)) ? "user2" : "user1";

	logerror("Loading cart %s at %.4x size:%.4x\n", image_filename(image), address, size);
	
	/* Finally load the cart */
	test = image_fread(image, memory_region_alloc( image->machine, tag, ( size & 0x1FFF ) ? ( size + 0x2000 ) : size, 0 ), size);

	if (test != size)
		return INIT_FAIL;

	/* Perform banking for the cartridge, based on the prescribed address */
	switch( address ) 
	{
		case 0x2000:
			vc20_rom_2000 = memory_region(image->machine, tag);
			break;

		case 0x4000:
			vc20_rom_4000 = memory_region(image->machine, tag);
			if ( size > 0x2000 ) 
			{
				vc20_rom_6000 = memory_region(image->machine, tag) + 0x2000;
			}
			break;

		case 0x6000:
			vc20_rom_6000 = memory_region(image->machine, tag);
			break;

		case 0xa000:
			vc20_rom_a000 = memory_region(image->machine, tag);
			break;
	}

	return INIT_PASS;
}


void vic20_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = DEVICE_START_NAME(vic20_cart); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(vic20_cart); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "rom,bin,a0,20,40,60"); break;

		default:											cartslot_device_getinfo(devclass, state, info); break;
	}
}
