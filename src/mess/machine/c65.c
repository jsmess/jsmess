/***************************************************************************
	commodore c65 home computer
	peter.trauner@jk.uni-linz.ac.at
    documention
     www.funet.fi
 ***************************************************************************/

#include "driver.h"

#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "video/vic4567.h"

#include "includes/cbmserb.h"
#include "includes/cbmdrive.h"

#include "includes/c65.h"
#include "includes/c64.h"

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

static int c65_charset_select=0;

static int c64mode=0;

/*UINT8 *c65_basic; */
/*UINT8 *c65_kernal; */
UINT8 *c65_chargen;
/*UINT8 *c65_dos; */
/*UINT8 *c65_monitor; */
UINT8 *c65_interface;
/*UINT8 *c65_graphics; */

/* processor has only 1 mega address space !? */
/* and system 8 megabyte */
/* dma controller and bankswitch hardware ?*/
static READ8_HANDLER(c65_read_mem)
{
	UINT8 result;
	if (offset <= 0x0FFFF)
		result = c64_memory[offset];
	else
		result = memory_read_byte(space, offset);
	return result;
}

static WRITE8_HANDLER(c65_write_mem)
{
	if (offset <= 0x0FFFF)
		c64_memory[offset] = data;
	else
		memory_write_byte(space, offset, data);
}

static struct {
	int version;
	UINT8 data[4];
} dma;
/* dma chip at 0xd700
  used:
   writing banknumber to offset 2
   writing hibyte to offset 1
   writing lobyte to offset 0
    cpu holded, dma transfer (data at address) executed, cpu activated

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
static void c65_dma_port_w(running_machine *machine, int offset, int value)
{
	static int dump=0;
	PAIR pair, src, dst, len;
	UINT8 cmd, fill;
	int i;
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);

	switch (offset&3) {
	case 2:
	case 1:
		dma.data[offset&3]=value;
		break;
	case 0:
		pair.b.h3=0;
		pair.b.h2=dma.data[2];
		pair.b.h=dma.data[1];
		pair.b.l=dma.data[0]=value;
		cmd=c65_read_mem(space, pair.d++);
		len.w.h=0;
		len.b.l=c65_read_mem(space, pair.d++);
		len.b.h=c65_read_mem(space, pair.d++);
		src.b.h3=0;
		fill=src.b.l=c65_read_mem(space, pair.d++);
		src.b.h=c65_read_mem(space, pair.d++);
		src.b.h2=c65_read_mem(space, pair.d++);
		dst.b.h3=0;
		dst.b.l=c65_read_mem(space, pair.d++);
		dst.b.h=c65_read_mem(space, pair.d++);
		dst.b.h2=c65_read_mem(space, pair.d++);

		switch (cmd) {
		case 0:
			if (src.d==0x3ffff) dump=1;
			if (dump)
				DBG_LOG(1,"dma copy job",
						("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
						 len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
						 c65_read_mem(space, pair.d+1) ) );
			if ( (dma.version==1)
				 &&( (src.d&0x400000)||(dst.d&0x400000)) ) {
				if ( !(src.d&0x400000) ) {
					dst.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(space, dst.d--,c65_read_mem(space, src.d++));
				} else if ( !(dst.d&0x400000) ) {
					src.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(space, dst.d++,c65_read_mem(space, src.d--));
				} else {
					src.d&=~0x400000;
					dst.d&=~0x400000;
					for (i=0; i<len.w.l; i++)
						c65_write_mem(space, --dst.d,c65_read_mem(space, --src.d));
				}
			} else {
				for (i=0; i<len.w.l; i++)
					c65_write_mem(space, dst.d++,c65_read_mem(space, src.d++));
			}
			break;
		case 3:
			DBG_LOG(3,"dma fill job",
					("len:%.4x value:%.2x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, fill, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d+1)));
				for (i=0; i<len.w.l; i++)
					c65_write_mem(space, dst.d++,fill);
				break;
		case 0x30:
			DBG_LOG(1,"dma copy down",
					("len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d+1) ) );
			for (i=0; i<len.w.l; i++)
				c65_write_mem(space, dst.d--,c65_read_mem(space, src.d--));
			break;
		default:
			DBG_LOG(1,"dma job",
					("cmd:%.2x len:%.4x src:%.6x dst:%.6x sub:%.2x modrm:%.2x\n",
					 cmd,len.w.l, src.d, dst.d, c65_read_mem(space, pair.d),
					 c65_read_mem(space, pair.d+1)));
		}
		break;
	default:
		DBG_LOG (1, "dma chip write", ("%.3x %.2x\n", offset,value));
		break;
	}
}

static int c65_dma_port_r(running_machine *machine, int offset)
{
	/* offset 3 bit 7 in progress ? */
	DBG_LOG (2, "dma chip read", ("%.3x\n", offset));
    return 0x7f;
}

static void c65_6511_port_w(running_machine *machine, int offset, int value)
{
	if (offset==7) 
	{
		c65_6511_port=value;
	}
	DBG_LOG (2, "r6511 write", ("%.2x %.2x\n", offset, value));
}

static int c65_6511_port_r(running_machine *machine, int offset)
{
	int data=0xff;

	if (offset==7) 
	{
		if (input_port_read(machine, "SPECIAL") & 0x20) 
			data &= ~1;
	}
	DBG_LOG (2, "r6511 read", ("%.2x\n", offset));

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
static struct {
	int state;

	UINT8 reg[0x0f];

	UINT8 buffer[0x200];
	int cpu_pos;
	int fdc_pos;

	UINT16 status;

	attotime time;
	int head,track,sector;
} c65_fdc= { 0 };

#define FDC_LOST 4
#define FDC_CRC 8
#define FDC_RNF 0x10
#define FDC_BUSY 0x80
#define FDC_IRQ 0x200

#define FDC_CMD_MOTOR_SPIN_UP 0x10

#if 0
static void c65_fdc_state(void)
{
	switch (c65_fdc.state) {
	case FDC_CMD_MOTOR_SPIN_UP:
		if (timer_get_time(machine)-c65_fdc.time) {
			c65_fdc.state=0;
			c65_fdc.status&=~FDC_BUSY;
		}
		break;
	}
}
#endif

static void c65_fdc_w(running_machine *machine, int offset, int data)
{
	DBG_LOG (1, "fdc write", ("%.5x %.2x %.2x\n", cpu_get_pc(machine->cpu[0]), offset, data));
	switch (offset&0xf) {
	case 0:
		c65_fdc.reg[0]=data;
		break;
	case 1:
		c65_fdc.reg[1]=data;
		switch (data&0xf9) {
		case 0x20: // wait for motor spin up
			c65_fdc.status&=~(FDC_IRQ|FDC_LOST|FDC_CRC|FDC_RNF);
			c65_fdc.status|=FDC_BUSY;
			c65_fdc.time=timer_get_time(machine);
			c65_fdc.state=FDC_CMD_MOTOR_SPIN_UP;
			break;
		case 0: // cancel
			c65_fdc.status&=~(FDC_BUSY);
			c65_fdc.state=0;
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
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.track=data;
		break;
	case 5:
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.sector=data;
		break;
	case 6:
		c65_fdc.reg[offset&0xf]=data;
		c65_fdc.head=data;
		break;
	case 7:
		c65_fdc.buffer[c65_fdc.cpu_pos++]=data;
		break;
	default:
		c65_fdc.reg[offset&0xf]=data;
		break;
	}
}

static int c65_fdc_r(running_machine *machine, int offset)
{
	UINT8 data=0;
	switch (offset&0xf) {
	case 0:
		data=c65_fdc.reg[0];
		break;
	case 1:
		data=c65_fdc.reg[1];
		break;
	case 2:
		data=c65_fdc.status;
		break;
	case 3:
		data=c65_fdc.status>>8;
		break;
	case 4:
		data=c65_fdc.track;
		break;
	case 5:
		data=c65_fdc.sector;
		break;
	case 6:
		data=c65_fdc.head;
		break;
	case 7:
		data=c65_fdc.buffer[c65_fdc.cpu_pos++];
		break;
	default:
		data=c65_fdc.reg[offset&0xf];
		break;
	}
	DBG_LOG (1, "fdc read", ("%.5x %.2x %.2x\n", cpu_get_pc(machine->cpu[0]), offset, data));
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
static struct {
	UINT8 reg;
} expansion_ram = {0};

static READ8_HANDLER(c65_ram_expansion_r)
{
	UINT8 data = 0xff;
	if (mess_ram_size > (128 * 1024))
		data = expansion_ram.reg;
	return data;
}

static WRITE8_HANDLER(c65_ram_expansion_w)
{
	offs_t expansion_ram_begin;
	offs_t expansion_ram_end;

	if (mess_ram_size > (128 * 1024))
	{
		expansion_ram.reg = data;

		expansion_ram_begin = 0x80000;
		expansion_ram_end = 0x80000 + (mess_ram_size - 128*1024) - 1;

		memory_install_read8_handler(space, expansion_ram_begin, expansion_ram_end,
			0, 0, (data == 0x00) ? SMH_BANK16 : SMH_NOP);
		memory_install_write8_handler(space, expansion_ram_begin, expansion_ram_end,
			0, 0, (data == 0x00) ? SMH_BANK16 : SMH_NOP);

		if (data == 0x00)
			memory_set_bankptr(space->machine, 16, mess_ram + 128*1024);
	}
}

static WRITE8_HANDLER ( c65_write_io )
{
	running_machine *machine = space->machine;
	switch(offset&0xf00) {
	case 0x000:
		if (offset < 0x80)
			vic3_port_w (space, offset & 0x7f, data);
		else if (offset < 0xa0) {
			c65_fdc_w(space->machine, offset&0x1f,data);
		} else {
			c65_ram_expansion_w(space, offset&0x1f, data);
			/*ram expansion crtl optional */
		}
		break;
	case 0x100:case 0x200: case 0x300:
		vic3_palette_w(space, offset-0x100,data);
		break;
	case 0x400:
		if (offset<0x420) /* maybe 0x20 */
			sid6581_0_port_w (space, offset & 0x3f, data);
		else if (offset<0x440)
			sid6581_1_port_w(space, offset&0x3f, data);
		else
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x500:
		DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
		break;
	case 0x600:
		c65_6511_port_w(space->machine, offset&0xff,data);
		break;
	case 0x700:
		c65_dma_port_w(space->machine, offset&0xff, data);
		break;
	}
}

static WRITE8_HANDLER ( c65_write_io_dc00 )
{
	running_machine *machine = space->machine;
	const device_config *cia_0 = device_list_find_by_tag(space->machine->config->devicelist, CIA6526R1, "cia_0");
	const device_config *cia_1 = device_list_find_by_tag(space->machine->config->devicelist, CIA6526R1, "cia_1");

	switch(offset&0xf00) {
	case 0x000:
		cia_w(cia_0, offset, data);
		break;
	case 0x100:
		cia_w(cia_1, offset, data);
		break;
	case 0x200:
	case 0x300:
		DBG_LOG (1, "io write", ("%.3x %.2x\n", offset+0xc00, data));
		break;
	}
}

static READ8_HANDLER ( c65_read_io )
{
	running_machine *machine = space->machine;
	switch(offset&0xf00) {
	case 0x000:
		if (offset < 0x80)
			return vic3_port_r (space, offset & 0x7f);
		if (offset < 0xa0) {
			return c65_fdc_r(space->machine, offset&0x1f);
		} else {
			return c65_ram_expansion_r(space, offset&0x1f);
			/*return; ram expansion crtl optional */
		}
		break;
	case 0x100:case 0x200: case 0x300:
	/* read only !? */
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x400:
		if (offset<0x420)
			return sid6581_0_port_r(space, offset & 0x3f);
		if (offset<0x440)
			return sid6581_1_port_r(space, offset&0x3f);
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x500:
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		break;
	case 0x600:
		return c65_6511_port_r(space->machine, offset&0xff);
	case 0x700:
		return c65_dma_port_r(space->machine, offset&0xff);
	}
	return 0xff;
}

static READ8_HANDLER ( c65_read_io_dc00 )
{
	running_machine *machine = space->machine;
	const device_config *cia_0 = device_list_find_by_tag(space->machine->config->devicelist, CIA6526R1, "cia_0");
	const device_config *cia_1 = device_list_find_by_tag(space->machine->config->devicelist, CIA6526R1, "cia_1");

	switch(offset&0x300) {
	case 0x000:
		return cia_r(cia_0, offset);
	case 0x100:
		return cia_r(cia_1, offset);
	case 0x200:
	case 0x300:
		DBG_LOG (1, "io read", ("%.3x\n", offset+0xc00));
		break;
	}
	return 0xff;
}


/*
d02f:
 init a5 96 written (seems to be switch to c65 or vic3 mode)
 go64 0 written
*/
static int c65_io_on=0, c65_io_dc00_on=0;

/* bit 1 external sync enable (genlock)
   bit 2 palette enable
   bit 6 vic3 c65 character set */
static void c65_bankswitch_interface(running_machine *machine, int value)
{
	static int old=0;
	read8_space_func rh;
	write8_space_func wh;

	DBG_LOG (2, "c65 bankswitch", ("%.2x\n",value));

	if (c65_io_on)
	{
		if (value&1)
		{
			memory_set_bankptr (machine, 8, c64_colorram + 0x400);
			memory_set_bankptr (machine, 9, c64_colorram + 0x400);
			rh = SMH_BANK8;
			wh = SMH_BANK9;
		}
		else
		{
			rh = c65_read_io_dc00;
			wh = c65_write_io_dc00;
		}
		memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, rh);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, wh);
	}

	c65_io_dc00_on=!(value&1);
#if 0
	/* cartridge roms !?*/
	if (value & 0x08)
		memory_set_bankptr (machine, 1, c64_roml);
	else
		memory_set_bankptr (machine, 1, c64_memory + 0x8000);

	if (value & 0x10)
		memory_set_bankptr (machine, 2, c64_basic);
	else
		memory_set_bankptr (machine, 2, c64_memory + 0xa000);
#endif
	if ((old^value) & 0x20) 
	{ 
	/* bankswitching faulty when doing actual page */
		if (value & 0x20) 
			memory_set_bankptr (machine, 3, c65_interface);
		else
			memory_set_bankptr (machine, 3, c64_memory + 0xc000);
	}
	c65_charset_select=value&0x40;
#if 0
	/* cartridge roms !?*/
	if (value & 0x80)
		memory_set_bankptr (machine, 8, c64_kernal);
	else
		memory_set_bankptr (machine, 6, c64_memory + 0xe000);
#endif
	old=value;
}

void c65_bankswitch (running_machine *machine)
{
	static int old = -1;
	int data, loram, hiram, charen;
	read8_space_func rh4, rh8;
	write8_space_func wh5, wh9;

	data = (UINT8) cpu_get_info_int(machine->cpu[0], CPUINFO_INT_M6510_PORT);
	if (data == old)
		return;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		memory_set_bankptr (machine, 1, c64_roml);
	}
	else
	{
		memory_set_bankptr (machine, 1, c64_memory + 0x8000);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		memory_set_bankptr (machine, 2, c64_romh);
	}
	else if (loram && hiram)
	{
		memory_set_bankptr (machine, 2, c64_basic);
	}
	else
	{
		memory_set_bankptr (machine, 2, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		c65_io_on = 1;
		rh4 = c65_read_io;
		wh5 = c65_write_io;
		memory_set_bankptr (machine, 6, c64_colorram);
		memory_set_bankptr (machine, 7, c64_colorram);

		if (c65_io_dc00_on)
		{
			rh8 = c65_read_io_dc00;
			wh9 = c65_write_io_dc00;
		}
		else
		{
			rh8 = SMH_BANK8;
			wh9 = SMH_BANK9;
			memory_set_bankptr (machine, 8, c64_colorram+0x400);
			memory_set_bankptr (machine, 9, c64_colorram+0x400);
		}
		memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, rh8);
		memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0dc00, 0x0dfff, 0, 0, wh9);
	}
	else
	{
		c65_io_on = 0;
		rh4 = SMH_BANK4;
		wh5 = SMH_BANK5;
		memory_set_bankptr(machine, 5, c64_memory+0xd000);
		memory_set_bankptr(machine, 7, c64_memory+0xd800);
		memory_set_bankptr(machine, 9, c64_memory+0xdc00);
		if (!charen && (loram || hiram))
		{
			memory_set_bankptr (machine, 4, c64_chargen);
			memory_set_bankptr (machine, 6, c64_chargen+0x800);
			memory_set_bankptr (machine, 8, c64_chargen+0xc00);
		}
		else
		{
			memory_set_bankptr (machine, 4, c64_memory + 0xd000);
			memory_set_bankptr (machine, 6, c64_memory + 0xd800);
			memory_set_bankptr (machine, 8, c64_memory + 0xdc00);
		}
	}
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, rh4);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM), 0x0d000, 0x0d7ff, 0, 0, wh5);

	if (!c64_game && c64_exrom)
	{
		memory_set_bankptr (machine, 10, c64_romh);
	}
	else
	{
		if (hiram)
		{
			memory_set_bankptr (machine, 10, c64_kernal);
		}
		else
		{
			memory_set_bankptr (machine, 10, c64_memory + 0xe000);
		}
	}
	old = data;
}

void c65_colorram_write (int offset, int value)
{
	c64_colorram[offset & 0x7ff] = value | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c65_dma_read(running_machine *machine, int offset)
{
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
			if (c65_charset_select)
				return c65_chargen[offset & 0xfff];
			else
				return c64_chargen[offset & 0xfff];
		}
		return c64_vicaddr[offset & 0x3fff];
	}
	return c64_vicaddr[offset & 0x3fff];
}

static int c65_dma_read_color(running_machine *machine, int offset)
{
	if (c64mode) return c64_colorram[offset&0x3ff]&0xf;
	return c64_colorram[offset & 0x7ff];
}

static void c65_common_driver_init (running_machine *machine)
{
	c64_memory = auto_malloc(0x10000);
	memset(c64_memory, 0, 0x10000);
	memory_set_bankptr(machine, 11, c64_memory + 0x00000);
	memory_set_bankptr(machine, 12, c64_memory + 0x08000);
	memory_set_bankptr(machine, 13, c64_memory + 0x0a000);
	memory_set_bankptr(machine, 14, c64_memory + 0x0c000);
	memory_set_bankptr(machine, 15, c64_memory + 0x0e000);

	/* C65 had no datasette port */
	c64_tape_on = 0;

	/*memset(c64_memory+0x40000, 0, 0x800000-0x40000); */

	vic4567_init(machine, c64_pal, c65_dma_read, c65_dma_read_color,
				  c64_vic_interrupt, c65_bankswitch_interface);
}

DRIVER_INIT( c65 )
{
	dma.version=2;
	c65_common_driver_init(machine);

}

DRIVER_INIT( c65pal )
{
	dma.version=1;
	c64_pal = 1;
	c65_common_driver_init(machine);
}

MACHINE_START( c65 )
{
	/* clear upper memory */
	memset(mess_ram + 128*1024, 0xff, mess_ram_size -  128*1024);

	serial_config(machine, &sim_drive_interface);
	cbm_serial_reset_write (machine, 0);
	cbm_drive_0_config (SERIAL, 10);
	cbm_drive_1_config (SERIAL, 11);
	c64_vicaddr = c64_memory;

	c64mode = 0;

	c65_bankswitch_interface(machine, 0xff);
	c65_bankswitch (machine);
}
