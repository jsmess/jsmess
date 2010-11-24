/***************************************************************************
    commodore c65 home computer
    peter.trauner@jk.uni-linz.ac.at
    documention
     www.funet.fi
 ***************************************************************************/

#include "emu.h"

#include "includes/cbm.h"
#include "includes/c65.h"
#include "includes/c64.h"
#include "cpu/m6502/m4510.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "devices/messram.h"
#include "video/vic4567.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)





/*UINT8 *c65_basic; */
/*UINT8 *c65_kernal; */
/*UINT8 *c65_dos; */
/*UINT8 *c65_monitor; */
/*UINT8 *c65_graphics; */


static void c65_nmi( running_machine *machine )
{
	c65_state *state = machine->driver_data<c65_state>();
	running_device *cia_1 = machine->device("cia_1");
	int cia1irq = mos6526_irq_r(cia_1);

	if (state->nmilevel != (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq)	/* KEY_RESTORE */
	{
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);

		state->nmilevel = (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq;
	}
}


/***********************************************

    CIA Interfaces

***********************************************/

/*
 *  CIA 0 - Port A keyboard line select
 *  CIA 0 - Port B keyboard line read
 *
 *  flag cassette read input, serial request in
 *  irq to irq connected
 *
 *  see machine/cbm.c
 */

static READ8_DEVICE_HANDLER( c65_cia0_port_a_r )
{
	UINT8 cia0portb = mos6526_pb_r(device->machine->device("cia_0"), 0);

	return cbm_common_cia0_port_a_r(device, cia0portb);
}

static READ8_DEVICE_HANDLER( c65_cia0_port_b_r )
{
	c65_state *state = device->machine->driver_data<c65_state>();
	UINT8 value = 0xff;
	UINT8 cia0porta = mos6526_pa_r(device->machine->device("cia_0"), 0);

	value &= cbm_common_cia0_port_b_r(device, cia0porta);

	if (!(state->_6511_port & 0x02))
		value &= state->keyline;

	return value;
}

static WRITE8_DEVICE_HANDLER( c65_cia0_port_b_w )
{
//  was there lightpen support in c65 video chip?
//  running_device *vic3 = device->machine->device("vic3");
//  vic3_lightpen_write(vic3, data & 0x10);
}

static void c65_irq( running_machine *machine, int level )
{
	c65_state *state = machine->driver_data<c65_state>();
	if (level != state->old_level)
	{
		DBG_LOG(machine, 3, "mos6510", ("irq %s\n", level ? "start" : "end"));
		cputag_set_input_line(machine, "maincpu", M6510_IRQ_LINE, level);
		state->old_level = level;
	}
}

/* is this correct for c65 as well as c64? */
static void c65_cia0_interrupt( running_device *device, int level )
{
	c65_state *state = device->machine->driver_data<c65_state>();
	c65_irq (device->machine, level || state->vicirq);
}

/* is this correct for c65 as well as c64? */
void c65_vic_interrupt( running_machine *machine, int level )
{
	c65_state *state = machine->driver_data<c65_state>();
	running_device *cia_0 = machine->device("cia_0");
#if 1
	if (level != state->vicirq)
	{
		c65_irq (machine, level || mos6526_irq_r(cia_0));
		state->vicirq = level;
	}
#endif
}

const mos6526_interface c65_ntsc_cia0 =
{
	60,
	DEVCB_LINE(c65_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia0_port_a_r),
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia0_port_b_r),
	DEVCB_HANDLER(c65_cia0_port_b_w)
};

const mos6526_interface c65_pal_cia0 =
{
	50,
	DEVCB_LINE(c65_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia0_port_a_r),
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia0_port_b_r),
	DEVCB_HANDLER(c65_cia0_port_b_w)
};

/*
 * CIA 1 - Port A
 * bit 7 serial bus data input
 * bit 6 serial bus clock input
 * bit 5 serial bus data output
 * bit 4 serial bus clock output
 * bit 3 serial bus atn output
 * bit 2 rs232 data output
 * bits 1-0 vic-chip system memory bank select
 *
 * CIA 1 - Port B
 * bit 7 user rs232 data set ready
 * bit 6 user rs232 clear to send
 * bit 5 user
 * bit 4 user rs232 carrier detect
 * bit 3 user rs232 ring indicator
 * bit 2 user rs232 data terminal ready
 * bit 1 user rs232 request to send
 * bit 0 user rs232 received data
 *
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
static READ8_DEVICE_HANDLER( c65_cia1_port_a_r )
{
	UINT8 value = 0xff;
	running_device *serbus = device->machine->device("serial_bus");

	if (!cbm_iec_clk_r(serbus))
		value &= ~0x40;

	if (!cbm_iec_data_r(serbus))
		value &= ~0x80;

	return value;
}

static WRITE8_DEVICE_HANDLER( c65_cia1_port_a_w )
{
	static const int helper[4] = {0xc000, 0x8000, 0x4000, 0x0000};
	running_device *serbus = device->machine->device("serial_bus");

	cbm_iec_atn_w(serbus, device, !BIT(data, 3));
	cbm_iec_clk_w(serbus, device, !BIT(data, 4));
	cbm_iec_data_w(serbus, device, !BIT(data, 5));

	c64_vicaddr = c64_memory + helper[data & 0x03];
}

static WRITE_LINE_DEVICE_HANDLER( c65_cia1_interrupt )
{
	c65_nmi(device->machine);
}

const mos6526_interface c65_ntsc_cia1 =
{
	60,
	DEVCB_LINE(c65_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia1_port_a_r),
	DEVCB_HANDLER(c65_cia1_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL
};

const mos6526_interface c65_pal_cia1 =
{
	50,
	DEVCB_LINE(c65_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(c65_cia1_port_a_r),
	DEVCB_HANDLER(c65_cia1_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/***********************************************

    Memory Handlers

***********************************************/

/* processor has only 1 mega address space !? */
/* and system 8 megabyte */
/* dma controller and bankswitch hardware ?*/
static READ8_HANDLER( c65_read_mem )
{
	UINT8 result;
	if (offset <= 0x0ffff)
		result = c64_memory[offset];
	else
		result = space->read_byte(offset);
	return result;
}

static WRITE8_HANDLER( c65_write_mem )
{
	if (offset <= 0x0ffff)
		c64_memory[offset] = data;
	else
		space->write_byte(offset, data);
}

/* dma chip at 0xd700
  used:
   writing banknumber to offset 2
   writing hibyte to offset 1
   writing lobyte to offset 0
    cpu holded, dma transfer(data at address) executed, cpu activated

  command data:
   0 command (0 copy, 3 fill)
   1,2 length
   3,4,5 source
   6,7,8 dest
   9 subcommand
   10 mod

   version 1:
   seldom copy (overlapping) from 0x402002 to 0x402008
   (making place for new line in basic area)
   for whats this bit 0x400000, or is this really the address?
   maybe means add counter to address for access,
   so allowing up or down copies, and reordering copies

   version 2:
   cmd 0x30 used for this
*/
static void c65_dma_port_w( running_machine *machine, int offset, int value )
{
	c65_state *state = machine->driver_data<c65_state>();
	PAIR pair, src, dst, len;
	UINT8 cmd, fill;
	int i;
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	switch (offset & 3)
	{
	case 2:
	case 1:
		state->dma.data[offset & 3] = value;
		break;
	case 0:
		pair.b.h3 = 0;
		pair.b.h2 = state->dma.data[2];
		pair.b.h = state->dma.data[1];
		pair.b.l = state->dma.data[0]=value;
		cmd = c65_read_mem(space, pair.d++);
		len.w.h = 0;
		len.b.l = c65_read_mem(space, pair.d++);
		len.b.h = c65_read_mem(space, pair.d++);
		src.b.h3 = 0;
		fill = src.b.l = c65_read_mem(space, pair.d++);
		src.b.h = c65_read_mem(space, pair.d++);
		src.b.h2 = c65_read_mem(space, pair.d++);
		dst.b.h3 = 0;
		dst.b.l = c65_read_mem(space, pair.d++);
		dst.b.h = c65_read_mem(space, pair.d++);
		dst.b.h2 = c65_read_mem(space, pair.d++);

		switch (cmd)
		{
		case 0:
			if (src.d == 0x3ffff) state->dump_dma = 1;
			if (state->dump_dma)
				DBG_LOG(space->machine, 1,"dma copy job",
						("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
						 len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
						 c65_read_mem(space, pair.d + 1) ) );
			if ((state->dma.version == 1)
				 && ( (src.d&0x400000) || (dst.d & 0x400000)))
			{
				if (!(src.d & 0x400000))
				{
					dst.d &= ~0x400000;
					for (i = 0; i < len.w.l; i++)
						c65_write_mem(space, dst.d--, c65_read_mem(space, src.d++));
				}
				else if (!(dst.d & 0x400000))
				{
					src.d &= ~0x400000;
					for (i = 0; i < len.w.l; i++)
						c65_write_mem(space, dst.d++, c65_read_mem(space, src.d--));
				}
				else
				{
					src.d &= ~0x400000;
					dst.d &= ~0x400000;
					for (i = 0; i < len.w.l; i++)
						c65_write_mem(space, --dst.d, c65_read_mem(space, --src.d));
				}
			}
			else
			{
				for (i = 0; i < len.w.l; i++)
					c65_write_mem(space, dst.d++, c65_read_mem(space, src.d++));
			}
			break;
		case 3:
			DBG_LOG(space->machine, 3,"dma fill job",
					("len:%.4x value:%.2x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, fill, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d + 1)));
				for (i = 0; i < len.w.l; i++)
					c65_write_mem(space, dst.d++, fill);
				break;
		case 0x30:
			DBG_LOG(space->machine, 1,"dma copy down",
					("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d + 1) ) );
			for (i = 0; i < len.w.l; i++)
				c65_write_mem(space, dst.d--,c65_read_mem(space, src.d--));
			break;
		default:
			DBG_LOG(space->machine, 1,"dma job",
					("cmd:%.2x len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 cmd,len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d + 1)));
		}
		break;
	default:
		DBG_LOG(space->machine, 1, "dma chip write", ("%.3x %.2x\n", offset, value));
		break;
	}
}

static int c65_dma_port_r( running_machine *machine, int offset )
{
	/* offset 3 bit 7 in progress ? */
	DBG_LOG(machine, 2, "dma chip read", ("%.3x\n", offset));
    return 0x7f;
}

static void c65_6511_port_w( running_machine *machine, int offset, int value )
{
	c65_state *state = machine->driver_data<c65_state>();
	if (offset == 7)
	{
		state->_6511_port = value;
	}
	DBG_LOG(machine, 2, "r6511 write", ("%.2x %.2x\n", offset, value));
}

static int c65_6511_port_r( running_machine *machine, int offset )
{
	int data = 0xff;

	if (offset == 7)
	{
		if (input_port_read(machine, "SPECIAL") & 0x20)
			data &= ~1;
	}
	DBG_LOG(machine, 2, "r6511 read", ("%.2x\n", offset));

	return data;
}

/* one docu states custom 4191 disk controller
 (for 2 1MB MFM disk drives, 1 internal, the other extern (optional) 1565
 with integrated 512 byte buffer

 0->0 reset ?

 0->1, 0->0, wait until 2 positiv, 1->0 ???

 0->0, 0 not 0 means no drive ???, other system entries


 reg 0 write/read
  0,1 written
  bit 1 set
  bit 2 set
  bit 3 set
  bit 4 set


 reg 0 read
  bit 0
  bit 1
  bit 2
  0..2 ->$1d4

 reg 1 write
  $01 written
  $18 written
  $46 written
  $80 written
  $a1 written
  $01 written, dec
  $10 written

 reg 2 read/write?(lsr)
  bit 2
  bit 4
  bit 5 busy waiting until zero, then reading reg 7
  bit 6 operation not activ flag!? or set overflow pin used
  bit 7 busy flag?

 reg 3 read/write?(rcr)
  bit 1
  bit 3
  bit 7 busy flag?

 reg 4
  track??
  0 written
  read -> $1d2
  cmp #$50
  bcs


 reg 5
  sector ??
  1 written
  read -> $1d3
  cmp #$b bcc


 reg 6
  head ??
  0 written
  read -> $1d1
  cmp #2 bcc

 reg 7 read
  #4e written
  12 times 0, a1 a1 a1 fe  written

 reg 8 read
  #ff written
  16 times #ff written

 reg 9
  #60 written

might use the set overflow input

$21a6c 9a6c format
$21c97 9c97 write operation
$21ca0 9ca0 get byte?
$21cab 9cab read reg 7
$21caf 9caf write reg 7
$21cb3
*/

#define FDC_LOST 4
#define FDC_CRC 8
#define FDC_RNF 0x10
#define FDC_BUSY 0x80
#define FDC_IRQ 0x200

#define FDC_CMD_MOTOR_SPIN_UP 0x10

#if 0
static void c65_fdc_state(void)
{
	c65_state *state = machine->driver_data<c65_state>();
	switch (state->fdc.state)
	{
	case FDC_CMD_MOTOR_SPIN_UP:
		if (timer_get_time(machine) - state->fdc.time)
		{
			state->fdc.state = 0;
			state->fdc.status &= ~FDC_BUSY;
		}
		break;
	}
}
#endif

static void c65_fdc_w( running_machine *machine, int offset, int data )
{
	c65_state *state = machine->driver_data<c65_state>();
	DBG_LOG(machine, 1, "fdc write", ("%.5x %.2x %.2x\n", cpu_get_pc(machine->device("maincpu")), offset, data));
	switch (offset & 0xf)
	{
	case 0:
		state->fdc.reg[0] = data;
		break;
	case 1:
		state->fdc.reg[1] = data;
		switch (data & 0xf9)
		{
		case 0x20: // wait for motor spin up
			state->fdc.status &= ~(FDC_IRQ|FDC_LOST|FDC_CRC|FDC_RNF);
			state->fdc.status |= FDC_BUSY;
			state->fdc.time = timer_get_time(machine);
			state->fdc.state = FDC_CMD_MOTOR_SPIN_UP;
			break;
		case 0: // cancel
			state->fdc.status &= ~(FDC_BUSY);
			state->fdc.state = 0;
			break;
		case 0x80: // buffered write
		case 0x40: // buffered read
		case 0x81: // unbuffered write
		case 0x41: // unbuffered read
		case 0x30:case 0x31: // step
			break;
		}
		break;
	case 2: case 3: // read only
		break;
	case 4:
		state->fdc.reg[offset & 0xf] = data;
		state->fdc.track = data;
		break;
	case 5:
		state->fdc.reg[offset & 0xf] = data;
		state->fdc.sector = data;
		break;
	case 6:
		state->fdc.reg[offset & 0xf] = data;
		state->fdc.head = data;
		break;
	case 7:
		state->fdc.buffer[state->fdc.cpu_pos++] = data;
		break;
	default:
		state->fdc.reg[offset & 0xf] = data;
		break;
	}
}

static int c65_fdc_r( running_machine *machine, int offset )
{
	c65_state *state = machine->driver_data<c65_state>();
	UINT8 data = 0;
	switch (offset & 0xf)
	{
	case 0:
		data = state->fdc.reg[0];
		break;
	case 1:
		data = state->fdc.reg[1];
		break;
	case 2:
		data = state->fdc.status;
		break;
	case 3:
		data = state->fdc.status >> 8;
		break;
	case 4:
		data = state->fdc.track;
		break;
	case 5:
		data = state->fdc.sector;
		break;
	case 6:
		data = state->fdc.head;
		break;
	case 7:
		data = state->fdc.buffer[state->fdc.cpu_pos++];
		break;
	default:
		data = state->fdc.reg[offset & 0xf];
		break;
	}
	DBG_LOG(machine, 1, "fdc read", ("%.5x %.2x %.2x\n", cpu_get_pc(machine->device("maincpu")), offset, data));
	return data;
}

/* version 1 ramcheck
   write 0:0
   read write read write 80000,90000,f0000
   write 0:8
   read write read write 80000,90000,f0000

   version 2 ramcheck???
   read 0:
   write 0:0
   read 0:
   first read and second read bit 0x80 set --> nothing
   write 0:0
   read 0
   write 0:ff
*/

static READ8_HANDLER( c65_ram_expansion_r )
{
	c65_state *state = space->machine->driver_data<c65_state>();
	UINT8 data = 0xff;
	if (messram_get_size(space->machine->device("messram")) > (128 * 1024))
		data = state->expansion_ram.reg;
	return data;
}

static WRITE8_HANDLER( c65_ram_expansion_w )
{
	c65_state *state = space->machine->driver_data<c65_state>();
	offs_t expansion_ram_begin;
	offs_t expansion_ram_end;

	if (messram_get_size(space->machine->device("messram")) > (128 * 1024))
	{
		state->expansion_ram.reg = data;

		expansion_ram_begin = 0x80000;
		expansion_ram_end = 0x80000 + (messram_get_size(space->machine->device("messram")) - 128*1024) - 1;

		if (data == 0x00) {
			memory_install_readwrite_bank(space, expansion_ram_begin, expansion_ram_end,0,0,"bank16");
			memory_set_bankptr(space->machine, "bank16", messram_get_ptr(space->machine->device("messram")) + 128*1024);
		} else {
			memory_nop_readwrite(space, expansion_ram_begin, expansion_ram_end,0,0);
		}
	}
}

static WRITE8_HANDLER( c65_write_io )
{
	running_device *sid_0 = space->machine->device("sid_r");
	running_device *sid_1 = space->machine->device("sid_l");
	running_device *vic3 = space->machine->device("vic3");

	switch (offset & 0xf00)
	{
	case 0x000:
		if (offset < 0x80)
			vic3_port_w(vic3, offset & 0x7f, data);
		else if (offset < 0xa0)
			c65_fdc_w(space->machine, offset&0x1f,data);
		else
		{
			c65_ram_expansion_w(space, offset&0x1f, data);
			/*ram expansion crtl optional */
		}
		break;
	case 0x100:
	case 0x200:
	case 0x300:
		vic3_palette_w(vic3, offset - 0x100, data);
		break;
	case 0x400:
		if (offset<0x420) /* maybe 0x20 */
			sid6581_w(sid_0, offset & 0x3f, data);
		else if (offset<0x440)
			sid6581_w(sid_1, offset & 0x3f, data);
		else
			DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x500:
		DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x600:
		c65_6511_port_w(space->machine, offset&0xff,data);
		break;
	case 0x700:
		c65_dma_port_w(space->machine, offset&0xff, data);
		break;
	}
}

static WRITE8_HANDLER( c65_write_io_dc00 )
{
	running_device *cia_0 = space->machine->device("cia_0");
	running_device *cia_1 = space->machine->device("cia_1");

	switch (offset & 0xf00)
	{
	case 0x000:
		mos6526_w(cia_0, offset, data);
		break;
	case 0x100:
		mos6526_w(cia_1, offset, data);
		break;
	case 0x200:
	case 0x300:
		DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset+0xc00, data));
		break;
	}
}

static READ8_HANDLER( c65_read_io )
{
	running_device *sid_0 = space->machine->device("sid_r");
	running_device *sid_1 = space->machine->device("sid_l");
	running_device *vic3 = space->machine->device("vic3");

	switch (offset & 0xf00)
	{
	case 0x000:
		if (offset < 0x80)
			return vic3_port_r(vic3, offset & 0x7f);
		if (offset < 0xa0)
			return c65_fdc_r(space->machine, offset&0x1f);
		else
		{
			return c65_ram_expansion_r(space, offset&0x1f);
			/*return; ram expansion crtl optional */
		}
		break;
	case 0x100:
	case 0x200:
	case 0x300:
	/* read only !? */
		DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset));
		break;
	case 0x400:
		if (offset < 0x420)
			return sid6581_r(sid_0, offset & 0x3f);
		if (offset < 0x440)
			return sid6581_r(sid_1, offset & 0x3f);
		DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset));
		break;
	case 0x500:
		DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset));
		break;
	case 0x600:
		return c65_6511_port_r(space->machine, offset&0xff);
	case 0x700:
		return c65_dma_port_r(space->machine, offset&0xff);
	}
	return 0xff;
}

static READ8_HANDLER( c65_read_io_dc00 )
{
	running_device *cia_0 = space->machine->device("cia_0");
	running_device *cia_1 = space->machine->device("cia_1");

	switch (offset & 0x300)
	{
	case 0x000:
		return mos6526_r(cia_0, offset);
	case 0x100:
		return mos6526_r(cia_1, offset);
	case 0x200:
	case 0x300:
		DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset+0xc00));
		break;
	}
	return 0xff;
}


/*
d02f:
 init a5 96 written (seems to be switch to c65 or vic3 mode)
 go64 0 written
*/

/* bit 1 external sync enable (genlock)
   bit 2 palette enable
   bit 6 vic3 c65 character set */
void c65_bankswitch_interface( running_machine *machine, int value )
{
	c65_state *state = machine->driver_data<c65_state>();
	DBG_LOG(machine, 2, "c65 bankswitch", ("%.2x\n",value));

	if (state->io_on)
	{
		if (value & 1)
		{
			memory_set_bankptr(machine, "bank8", c64_colorram + 0x400);
			memory_set_bankptr(machine, "bank9", c64_colorram + 0x400);
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, "bank8");
			memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, "bank9");
		}
		else
		{
			memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, c65_read_io_dc00);
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0,c65_write_io_dc00);
		}
	}

	state->io_dc00_on = !(value & 1);
#if 0
	/* cartridge roms !?*/
	if (value & 0x08)
		memory_set_bankptr(machine, "bank1", c64_roml);
	else
		memory_set_bankptr(machine, "bank1", c64_memory + 0x8000);

	if (value & 0x10)
		memory_set_bankptr(machine, "bank2", c64_basic);
	else
		memory_set_bankptr(machine, "bank2", c64_memory + 0xa000);
#endif
	if ((state->old_value^value) & 0x20)
	{
	/* bankswitching faulty when doing actual page */
		if (value & 0x20)
			memory_set_bankptr(machine, "bank3", state->interface);
		else
			memory_set_bankptr(machine, "bank3", c64_memory + 0xc000);
	}
	state->charset_select = value & 0x40;
#if 0
	/* cartridge roms !?*/
	if (value & 0x80)
		memory_set_bankptr(machine, "bank8", c64_kernal);
	else
		memory_set_bankptr(machine, "bank6", c64_memory + 0xe000);
#endif
	state->old_value = value;
}

void c65_bankswitch( running_machine *machine )
{
	c65_state *state = machine->driver_data<c65_state>();
	int data, loram, hiram, charen;

	data = m4510_get_port(machine->device<legacy_cpu_device>("maincpu"));
	if (data == state->old_data)
		return;

	DBG_LOG(machine, 1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom) || (loram && hiram && !c64_exrom))
		memory_set_bankptr(machine, "bank1", c64_roml);
	else
		memory_set_bankptr(machine, "bank1", c64_memory + 0x8000);

	if ((!c64_game && c64_exrom && hiram) || (!c64_exrom))
		memory_set_bankptr(machine, "bank2", c64_romh);
	else if (loram && hiram)
		memory_set_bankptr(machine, "bank2", c64_basic);
	else
		memory_set_bankptr(machine, "bank2", c64_memory + 0xa000);

	if ((!c64_game && c64_exrom) || (charen && (loram || hiram)))
	{
		state->io_on = 1;
		memory_set_bankptr(machine, "bank6", c64_colorram);
		memory_set_bankptr(machine, "bank7", c64_colorram);

		if (state->io_dc00_on)
		{
			memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, c65_read_io_dc00);
			memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, c65_write_io_dc00);
		}
		else
		{
			memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, "bank8");
			memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, "bank9");
			memory_set_bankptr(machine, "bank8", c64_colorram + 0x400);
			memory_set_bankptr(machine, "bank9", c64_colorram + 0x400);
		}
		memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, c65_read_io);
		memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, c65_write_io);
	}
	else
	{
		state->io_on = 0;
		memory_set_bankptr(machine, "bank5", c64_memory + 0xd000);
		memory_set_bankptr(machine, "bank7", c64_memory + 0xd800);
		memory_set_bankptr(machine, "bank9", c64_memory + 0xdc00);
		if (!charen && (loram || hiram))
		{
			memory_set_bankptr(machine, "bank4", c64_chargen);
			memory_set_bankptr(machine, "bank6", c64_chargen + 0x800);
			memory_set_bankptr(machine, "bank8", c64_chargen + 0xc00);
		}
		else
		{
			memory_set_bankptr(machine, "bank4", c64_memory + 0xd000);
			memory_set_bankptr(machine, "bank6", c64_memory + 0xd800);
			memory_set_bankptr(machine, "bank8", c64_memory + 0xdc00);
		}
		memory_install_read_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, "bank4");
		memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, "bank5");
	}

	if (!c64_game && c64_exrom)
	{
		memory_set_bankptr(machine, "bank10", c64_romh);
	}
	else
	{
		if (hiram)
		{
			memory_set_bankptr(machine, "bank10", c64_kernal);
		}
		else
		{
			memory_set_bankptr(machine, "bank10", c64_memory + 0xe000);
		}
	}
	state->old_data = data;
}

void c65_colorram_write( int offset, int value )
{
	c64_colorram[offset & 0x7ff] = value | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
int c65_dma_read( running_machine *machine, int offset )
{
	c65_state *state = machine->driver_data<c65_state>();
	if (!c64_game && c64_exrom)
	{
		if (offset < 0x3000)
			return c64_memory[offset];
		return c64_romh[offset & 0x1fff];
	}
	if ((c64_vicaddr == c64_memory) || (c64_vicaddr == c64_memory + 0x8000))
	{
		if (offset < 0x1000)
			return c64_vicaddr[offset & 0x3fff];
		if (offset < 0x2000) {
			if (state->charset_select)
				return state->chargen[offset & 0xfff];
			else
				return c64_chargen[offset & 0xfff];
		}
		return c64_vicaddr[offset & 0x3fff];
	}
	return c64_vicaddr[offset & 0x3fff];
}

int c65_dma_read_color( running_machine *machine, int offset )
{
	c65_state *state = machine->driver_data<c65_state>();
	if (state->c64mode)
		return c64_colorram[offset & 0x3ff] & 0xf;
	return c64_colorram[offset & 0x7ff];
}

static void c65_common_driver_init( running_machine *machine )
{
	c65_state *state = machine->driver_data<c65_state>();
	c64_memory = auto_alloc_array_clear(machine, UINT8, 0x10000);
	memory_set_bankptr(machine, "bank11", c64_memory + 0x00000);
	memory_set_bankptr(machine, "bank12", c64_memory + 0x08000);
	memory_set_bankptr(machine, "bank13", c64_memory + 0x0a000);
	memory_set_bankptr(machine, "bank14", c64_memory + 0x0c000);
	memory_set_bankptr(machine, "bank15", c64_memory + 0x0e000);

	cbm_common_init();
	state->keyline = 0xff;

	c64_pal = 0;
	state->charset_select = 0;
	state->_6511_port = 0xff;
	state->vicirq = 0;
	state->old_data = -1;

	/* C65 had no datasette port */
	c64_tape_on = 0;
	c64_game = 1;
	c64_exrom = 1;

	/*memset(c64_memory + 0x40000, 0, 0x800000 - 0x40000); */
}

DRIVER_INIT( c65 )
{
	c65_state *state = machine->driver_data<c65_state>();
	state->dma.version = 2;
	c65_common_driver_init(machine);
}

DRIVER_INIT( c65pal )
{
	c65_state *state = machine->driver_data<c65_state>();
	state->dma.version = 1;
	c65_common_driver_init(machine);
	c64_pal = 1;
}

MACHINE_START( c65 )
{
	c65_state *state = machine->driver_data<c65_state>();
	/* clear upper memory */
	memset(messram_get_ptr(machine->device("messram")) + 128*1024, 0xff, messram_get_size(machine->device("messram")) -  128*1024);

//removed   cbm_drive_0_config (SERIAL, 10);
//removed   cbm_drive_1_config (SERIAL, 11);
	c64_vicaddr = c64_memory;

	state->c64mode = 0;

	c65_bankswitch_interface(machine, 0xff);
	c65_bankswitch (machine);
}


INTERRUPT_GEN( c65_frame_interrupt )
{
	c65_state *state = device->machine->driver_data<c65_state>();
	int value;

	c65_nmi(device->machine);

	/* common keys input ports */
	cbm_common_interrupt(device);

	/* c65 specific: function keys input ports */
	value = 0xff;

	value &= ~input_port_read(device->machine, "FUNCT");
	state->keyline = value;
}
