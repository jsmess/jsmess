/***************************************************************************

    machine/pc8801.c

    Implementation of the PC8801

    On the PC8801, there are two CPUs.  The main CPU and the FDC CPU.  Two
    8255 PPIs are used for cross CPU communication.  These ports are connected
    as below:

        Main CPU 8255 port A <--> FDC CPU 8255 port B
        Main CPU 8255 port B <--> FDC CPU 8255 port A
        Main CPU 8255 port C (bits 7-4) <--> FDC CPU 8255 port C (bits 0-3)
        Main CPU 8255 port C (bits 0-3) <--> FDC CPU 8255 port C (bits 7-4)

***************************************************************************/

#include "emu.h"
#include "includes/pc8801.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/upd1990a.h"
#include "machine/upd765.h"
#include "sound/beep.h"


static void pc8801_init_5fd(running_machine *machine);

static void pc88sr_init_fmsound(running_machine *machine);
#define FM_IRQ_LEVEL 4

WRITE8_HANDLER( pc88_rtc_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	/* real time clock */
	upd1990a_c0_w(state->upd1990a, BIT(data, 0));
	upd1990a_c1_w(state->upd1990a, BIT(data, 1));
	upd1990a_c2_w(state->upd1990a, BIT(data, 2));
	upd1990a_data_in_w(state->upd1990a, BIT(data, 3));

	/* printer */
	centronics_data_w(state->centronics, 0, data);

	/* UOP3 (bit 7 -- not yet) */
}

/* interrupt staff */


static void pc8801_update_interrupt(running_machine *machine)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	int level, i;

	level = -1;
	for (i = 0; i < 8; i++)
	{
		if ((state->interrupt_trig_reg & (1<<i)) != 0)
			level = i;
	}
	if (level >= 0 && level<state->interrupt_level_reg)
	{
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	}
}

static IRQ_CALLBACK( pc8801_interrupt_callback )
{
	pc88_state *state = device->machine->driver_data<pc88_state>();
	int level, i;

	level = 0;
	for (i = 0; i < 8; i++)
	{
		if ((state->interrupt_trig_reg & (1<<i))!=0)
			level = i;
	}
	state->interrupt_trig_reg &= ~(1<<level);
	return level*2;
}

WRITE8_HANDLER( pc8801_write_interrupt_level )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	state->interrupt_level_reg = data&0x0f;
	pc8801_update_interrupt(space->machine);
}

WRITE8_HANDLER( pc8801_write_interrupt_mask )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	state->interrupt_mask_reg = ((data&0x01)<<2) | (data&0x02)
		| ((data&0x04)>>2) | 0xf8;
}

static void pc8801_raise_interrupt(running_machine *machine, int level)
{
	pc88_state *state = machine->driver_data<pc88_state>();
  state->interrupt_trig_reg |= state->interrupt_mask_reg & (1<<level);
  pc8801_update_interrupt(machine);
}

INTERRUPT_GEN( pc8801_interrupt )
{
	pc8801_raise_interrupt(device->machine, 1);
}

static TIMER_CALLBACK( pc8801_timer_interrupt )
{
	pc8801_raise_interrupt(machine, 2);
}

static void pc8801_init_interrupt(running_machine *machine)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	state->interrupt_level_reg = 0;
	state->interrupt_mask_reg = 0xf8;
	state->interrupt_trig_reg = 0x0;
	cpu_set_irq_callback(machine->device("maincpu"), pc8801_interrupt_callback);
}

WRITE8_HANDLER( pc88sr_outport_30 )
{
	/* bit 1-5 not implemented yet */
	pc88sr_disp_30(space, offset,data);
}

WRITE8_HANDLER( pc88sr_outport_40 )
{
	/* bit 3,4,6 not implemented */
	/* bit 7 incorrect behavior */
	pc88_state *state = space->machine->driver_data<pc88_state>();
	running_device *speaker = space->machine->device("beep");

	/* printer */
	centronics_strobe_w(state->centronics, BIT(data, 0));

	/* real time clock */
	upd1990a_stb_w(state->upd1990a, BIT(data, 1));
	upd1990a_clk_w(state->upd1990a, BIT(data, 2));

	if((input_port_read(space->machine, "DSW1") & 0x40) == 0x00) data&=0x7f;

	switch(data&0xa0)
	{
		case 0x00:
			beep_set_state(speaker, 0);
			break;
		case 0x20:
			beep_set_frequency(speaker, 2400);
			beep_set_state(speaker, 1);
			break;
		case 0x80:
		case 0xa0:
			beep_set_frequency(speaker, 0);
			beep_set_state(speaker, 1);
			break;
	}
}

READ8_HANDLER( pc88sr_inport_40 )
{
	/* bit 2 not implemented */
	pc88_state *state = space->machine->driver_data<pc88_state>();
	UINT8 r;

	r = centronics_busy_r(state->centronics);
	r |= state->is_24KHz ? 0x00 : 0x02;
	r |= state->use_5FD ? 0x00 : 0x08;
	r |= upd1990a_data_out_r(state->upd1990a) ? 0x10 : 0x00;
	if(space->machine->primary_screen->vblank()) r|=0x20;

	return r|0xc0;
}

READ8_HANDLER( pc88sr_inport_31 )
     /* DIP-SW2
    bit 0: serial parity (0 = enable, 1 = disable)
    bit 1: parity type (0 = even parity, 1 = odd parity)
    bit 2: serial bit length (0 = 8bits/char, 1 = 7bits/char)
    bit 3: stop bit length (0 = 2bits, 1 = 1bits)
    bit 4: X parameter (0 = enable, 1 = disable)
    bit 5: duplex mode (0 = half, 1 = full-duplex)
    bit 6: speed switch (0 = normal speed, 1 = high speed)
    bit 7: N88-BASIC version (0 = V2.x, 1 = V1.x)
      */
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  int r;

  /* read DIP-SW */
  r=input_port_read(space->machine, "DSW2")<<1;
  /* change bit 6 according speed switch */
  if(state->pc88sr_is_highspeed) {
    r|=0x40;
  } else {
    r&=0xbf;
  }
  /* change bit 7 according BASIC mode */
  if(state->is_V2mode) {
    r&=0x7f;
  } else {
    r|=0x80;
  }
  return r;
}

static WRITE8_HANDLER( pc8801_writemem1 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->mainRAM[offset]=data;
}

static WRITE8_HANDLER( pc8801_writemem2 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->mainRAM[offset+0x6000]=data;
}

static READ8_HANDLER( pc8801_read_textwindow )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  return state->mainRAM[(offset+state->text_window*0x100)&0xffff];
}

static WRITE8_HANDLER( pc8801_write_textwindow )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->mainRAM[(offset+state->text_window*0x100)&0xffff]=data;
}

static void select_extmem(pc88_state *state, char **r,char **w,UINT8 *ret_ctrl)
{
  int i;

  if(r!=NULL) *r=NULL;	/* read map */
  if(w!=NULL) *w=NULL;	/* write map */
  if(ret_ctrl!=NULL) {
    ret_ctrl[0]=ret_ctrl[1]=0xff;	/* port 0xe2/0xe3 input value */
  }

#define SET_RADDR(x) \
  do { \
    if(r!=NULL) { \
      if(*r!=NULL) { \
	logerror("read multiple bank of extension memory.\n"); \
      } else { \
	*r=(char*)(x); \
      } \
    } \
  } while(0);

#define SET_WADDR(x) \
  do { \
    if(w!=NULL) { \
      if(*w!=NULL) { \
	logerror("write multiple bank of extension memory.\n"); \
      } else { \
	*w=(char*)(x); \
      } \
    } \
  } while(0);

#define SET_RET(n,x) \
  do { \
    if(ret_ctrl!=NULL) { \
      if(ret_ctrl[n]!=0xff) { \
	logerror("conflict input value of port %.2x.\n",0xe2+(n)); \
      } else { \
	ret_ctrl[n]=(x); \
      } \
    } \
  } while(0);

  if(state->ext_bank_88[state->extmem_ctrl[1]]!=NULL) {
    if(state->extmem_ctrl[0]&0x01) SET_RADDR(state->ext_bank_88[state->extmem_ctrl[1]]);
    if(state->extmem_ctrl[0]&0x10) SET_WADDR(state->ext_bank_88[state->extmem_ctrl[1]]);
    SET_RET(0,(~state->extmem_ctrl[0])&0x11)
    if(state->ext_bank_88[state->extmem_ctrl[1]]==state->ext_bank_88[state->extmem_ctrl[1]&0x0f]) {
      /* PC-8801-N02 */
      SET_RET(1,state->extmem_ctrl[1]&0x0f);
    } else {
      /* PIO-8234H */
      SET_RET(1,state->extmem_ctrl[1]);
    }
  }

  for(i=0;i<4;i++) {
    if(state->ext_bank_80[i]!=NULL) {
      if(state->extmem_ctrl[0]&(0x01<<i)) {
	SET_RADDR(state->ext_bank_80[i]);
	SET_RET(0,(0x01<<i)|0xf0);
      }
      if(state->extmem_ctrl[0]&(0x10<<i)) SET_WADDR(state->ext_bank_80[i]);
    }
  }

#undef SET_RADDR
#undef SET_WADDR
#undef SET_RET
}

void pc8801_update_bank(running_machine *machine)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	address_space *program = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	char *ext_r,*ext_w;

	select_extmem(state, &ext_r,&ext_w,NULL);
	if (ext_r!=NULL || ext_w!=NULL)
	{
		if (ext_r==NULL)
		{
			logerror("set write only mode to extension memory (treat as R/W mode).\n");
			ext_r=ext_w;
		}

		/* extension memory */
		memory_set_bankptr(machine, "bank1", ext_r + 0x0000);
		memory_set_bankptr(machine, "bank2", ext_r + 0x6000);
		if(ext_w==NULL)
		{
			/* read only mode */
			memory_nop_write(program, 0x0000, 0x5fff, 0, 0);
			memory_nop_write(program, 0x6000, 0x7fff, 0, 0);
		}
		else
		{
			/* r/w mode */
			memory_install_write_bank(program, 0x0000, 0x5fff, 0, 0, "bank1");
			memory_install_write_bank(program, 0x6000, 0x7fff, 0, 0, "bank2");
			if(ext_w!=ext_r) logerror("differnt between read and write bank of extension memory.\n");
		}
	}
	else
	{
		/* 0x0000 to 0x7fff */
		if(state->RAMmode)
		{
			/* RAM */
			memory_install_write_bank(program, 0x0000, 0x5fff, 0, 0, "bank1");
			memory_install_write_bank(program, 0x6000, 0x7fff, 0, 0, "bank2");
			memory_set_bankptr(machine, "bank1", state->mainRAM + 0x0000);
			memory_set_bankptr(machine, "bank2", state->mainRAM + 0x6000);
		}
		else
		{
			/* ROM */
			/* write through to main RAM */
			memory_install_write8_handler(program, 0x0000, 0x5fff, 0, 0, pc8801_writemem1);
			memory_install_write8_handler(program, 0x6000, 0x7fff, 0, 0, pc8801_writemem2);
			if(state->ROMmode)
			{
				/* N-BASIC */
				memory_set_bankptr(machine, "bank1", state->mainROM + 0x0000);
				memory_set_bankptr(machine, "bank2", state->mainROM + 0x6000);
			}
			else
			{
				/* N88-BASIC */
				memory_set_bankptr(machine, "bank1", state->mainROM + 0x8000);
				if(state->no4throm==1) {
					/* 4th ROM 1 */
					memory_set_bankptr(machine, "bank2", state->mainROM + 0x10000 + 0x2000 * state->no4throm2);
				} else {
					memory_set_bankptr(machine, "bank2", state->mainROM + 0xe000);
				}
			}
		}
	}

	/* 0x8000 to 0xffff */
	if (state->RAMmode || state->ROMmode)  {
		memory_install_readwrite_bank(program, 0x8000, 0x83ff, 0, 0, "bank3");
	} else {
		memory_install_readwrite8_handler(program, 0x8000, 0x83ff, 0, 0, pc8801_read_textwindow, pc8801_write_textwindow);
	}

	memory_set_bankptr(machine, "bank4", state->mainRAM + 0x8400);

	if (pc8801_is_vram_select(machine))
	{
		/* VRAM */
		/* already maped */
	}
	else
	{
		memory_install_readwrite_bank(program, 0xc000, 0xefff, 0, 0, "bank5");
		memory_install_readwrite_bank(program, 0xf000, 0xffff, 0, 0, "bank6");

		memory_set_bankptr(machine, "bank5", state->mainRAM + 0xc000);
		if(state->maptvram)
			memory_set_bankptr(machine, "bank6", state->pc88sr_textRAM);
		else
			memory_set_bankptr(machine, "bank6", state->mainRAM + 0xf000);
	}
}

READ8_HANDLER( pc88_extmem_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
	UINT8 ret[2];
	select_extmem(state,NULL,NULL,ret);
	return ret[offset];
}

WRITE8_HANDLER( pc88_extmem_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->extmem_ctrl[offset]=data;
  pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc88sr_outport_31 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  /* bit 5 not implemented */
  state->RAMmode=((data&0x02)!=0);
  state->ROMmode=((data&0x04)!=0);
  pc8801_update_bank(space->machine);
  pc88sr_disp_31(space, offset,data);
}

READ8_HANDLER( pc88sr_inport_32 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  return(state->port32_save);
}

WRITE8_HANDLER( pc88sr_outport_32 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  /* bit 2, 3 not implemented */
  state->port32_save=data;
  state->maptvram=((data&0x10)==0);
  state->no4throm2=(data&3);
  state->enable_FM_IRQ=((data & 0x80) == 0x00);
  if(state->FM_IRQ_save && state->enable_FM_IRQ) pc8801_raise_interrupt(space->machine, FM_IRQ_LEVEL);
  pc88sr_disp_32(space, offset,data);
  pc8801_update_bank(space->machine);
}

READ8_HANDLER( pc8801_inport_70 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  return state->text_window;
}

WRITE8_HANDLER( pc8801_outport_70 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->text_window=data;
  pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc8801_outport_78 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->text_window=((state->text_window+1)&0xff);
  pc8801_update_bank(space->machine);
}

READ8_HANDLER( pc88sr_inport_71 )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  return(state->port71_save);
}

WRITE8_HANDLER( pc88sr_outport_71 )
/* bit 1-7 not implemented (no ROMs) */
{
	pc88_state *state = space->machine->driver_data<pc88_state>();
  state->port71_save=data;
  switch(data) {
  case 0xff: state->no4throm=0; break;
  case 0xfe: state->no4throm=1; break;
  case 0xfd: state->no4throm=2; break;
  case 0xfb: state->no4throm=3; break;
  case 0xf7: state->no4throm=4; break;
  case 0xef: state->no4throm=5; break;
  case 0xdf: state->no4throm=6; break;
  case 0xbf: state->no4throm=7; break;
  case 0x7f: state->no4throm=8; break;
  default:
    logerror ("pc8801 : write illegal data 0x%.2x to port 0x71, select main rom.\n",data);
    state->no4throm=0;
    break;
  }
  pc8801_update_bank(space->machine);
}

// FIXME: shouldn't we use ram device in place of the below code?
static void pc8801_init_bank(running_machine *machine, int hireso)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	int i,j;
	int num80,num88,numIO;
	unsigned char *e;

	state->RAMmode=0;
	state->ROMmode=0;
	state->maptvram=0;
	state->no4throm=0;
	state->no4throm2=0;
	state->port71_save=0xff;
	state->port32_save=0x80;
	state->mainROM = memory_region(machine, "maincpu");
	state->mainRAM = auto_alloc_array_clear(machine, UINT8, 0x10000);

	state->extmem_ctrl[0]=state->extmem_ctrl[1]=0;
	pc8801_update_bank(machine);
	pc8801_video_init(machine, hireso);

	if (state->extmem_mode != input_port_read(machine, "MEM"))
	{
		state->extmem_mode = input_port_read(machine, "MEM");

		// reset all the bank-related quantities
		memset(state->extRAM, 0, state->extRAM_size);

		for (i = 0; i < 4; i++)
			state->ext_bank_80[i] = NULL;

		for (i = 0; i < 256; i++)
			state->ext_bank_88[i] = NULL;

		num80 = num88 = numIO = 0;

		// set up the required number of banks
		switch (state->extmem_mode)
		{
			case 0x00: /* none */
				break;
			case 0x01: /* 32KB(PC-8012-02 x 1) */
				num80 = 1;
				break;
			case 0x02: /* 64KB(PC-8012-02 x 2) */
				num80 = 2;
				break;
			case 0x03: /* 128KB(PC-8012-02 x 4) */
				num80 = 4;
				break;
			case 0x04: /* 128KB(PC-8801-02N x 1) */
				num88 = 1;
				break;
			case 0x05: /* 256KB(PC-8801-02N x 2) */
				num88 = 2;
				break;
			case 0x06: /* 512KB(PC-8801-02N x 4) */
				num88 = 4;
				break;
			case 0x07: /* 1M(PIO-8234H-1M x 1) */
				numIO = 1;
				break;
			case 0x08: /* 2M(PIO-8234H-2M x 1) */
				numIO = 2;
				break;
			case 0x09: /* 4M(PIO-8234H-2M x 2) */
				numIO = 4;
				break;
			case 0x0a: /* 8M(PIO-8234H-2M x 4) */
				numIO = 8;
				break;
			case 0x0b: /* 1.1M(PIO-8234H-1M x 1 + PC-8801-02N x 1) */
				num88 = 1;
				numIO = 1;
				break;
			case 0x0c: /* 2.1M(PIO-8234H-2M x 1 + PC-8801-02N x 1) */
				num88 = 1;
				numIO = 2;
				break;
			case 0x0d: /* 4.1M(PIO-8234H-2M x 2 + PC-8801-02N x 1) */
				num88 = 1;
				numIO = 4;
				break;
			default:
				logerror("pc8801 : illegal extension memory mode.\n");
				return;
		}

		// point the banks to the correct memory
		if (num80 != 0 || num88 != 0 || numIO != 0)
		{
			e = state->extRAM;

			for (i = 0; i < num80; i++)
			{
				state->ext_bank_80[i] = e;
				e += 0x8000;
			}

			for (i = 0; i < num88 * 4; i++)
			{
				for (j = i; j < 256; j += 16)
				{
					state->ext_bank_88[j] = e;
				}
				e += 0x8000;
			}

			if (num88 == 0)
			{
				for (i = 0; i < numIO * 32; i++)
				{
					state->ext_bank_88[(i & 0x07) | ((i & 0x18) << 1) | ((i & 0x20) >> 2) | (i & 0xc0)] = e;
					e += 0x8000;
				}
			}
			else
			{
				for (i = 0; i < numIO * 32; i++)
				{
					state->ext_bank_88[(i & 0x07) | ((i & 0x78) << 1) | 0x08] = e;
					e += 0x8000;
				}
			}
		}
	}
}

static void fix_V1V2(pc88_state *state)
{
  if(state->is_Nbasic) state->is_V2mode=0;
  if(state->is_V2mode) state->pc88sr_is_highspeed=1;
  switch((state->is_Nbasic ? 1 : 0) |
	 (state->is_V2mode ? 2 : 0) |
	 (state->pc88sr_is_highspeed ? 4 : 0))
  {
  case 0:
    logerror("N88-BASIC(V1-S)");
    break;
  case 1:
    logerror("N-BASIC(S)");
    break;
  case 4:
    logerror("N88-BASIC(V1-H)");
    break;
  case 5:
    logerror("N-BASIC(H)");
    break;
  case 6:
    logerror("N88-BASIC(V2)");
    break;
  default:
    fatalerror("Illegal basic mode=(%d,%d,%d)",state->is_Nbasic,state->is_V2mode,state->pc88sr_is_highspeed);
    break;
  }
  if(state->is_8MHz) {
    logerror(", 8MHz\n");
  } else {
    logerror(", 4MHz\n");
  }
}

static void pc88sr_ch_reset(running_machine *machine, int hireso)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	running_device *speaker = machine->device("beep");
	int a;

	// old code was allocating/freeing a smaller region depending on the "MEM" config,
	// but I guess this was the reason of bug #1952. So we now allocate the maximum
	// possible amount of extended RAM, and then we only use the amount we need.
	// eventually, we should convert the driver to use the ram device, imho.
	state->extRAM_size = 4 * 0x8000 + 4 * 0x20000 + 8 * 0x100000;
	state->extRAM = auto_alloc_array_clear(machine, UINT8, state->extRAM_size);

	a = input_port_read(machine, "CFG");
	state->is_Nbasic = ((a&0x01)==0x00);
	state->is_V2mode = ((a&0x02)==0x00);
	state->pc88sr_is_highspeed = ((a&0x04)!=0x00);
	state->is_8MHz = ((a&0x08)!=0x00);
	fix_V1V2(state);
	pc8801_init_bank(machine, hireso);
	pc8801_init_5fd(machine);
	pc8801_init_interrupt(machine);
	beep_set_state(speaker, 0);
	beep_set_frequency(speaker, 2400);
	pc88sr_init_fmsound(machine);
}

/* 5 inch floppy drive */

static READ8_DEVICE_HANDLER( cpu_8255_c_r )
{
	pc88_state *state = device->machine->driver_data<pc88_state>();

	return state->i8255_1_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( cpu_8255_c_w )
{
	pc88_state *state = device->machine->driver_data<pc88_state>();

	state->i8255_0_pc = data;
}

I8255A_INTERFACE( pc8801_8255_config_0 )
{
	DEVCB_DEVICE_HANDLER(FDC_I8255A_TAG, i8255a_pb_r),	// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_HANDLER(cpu_8255_c_r),		// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(cpu_8255_c_w)			// Port C write
};

static READ8_DEVICE_HANDLER( fdc_8255_c_r )
{
	pc88_state *state = device->machine->driver_data<pc88_state>();

	return state->i8255_0_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( fdc_8255_c_w )
{
	pc88_state *state = device->machine->driver_data<pc88_state>();

	state->i8255_1_pc = data;
}

I8255A_INTERFACE( pc8801_8255_config_1 )
{
	DEVCB_DEVICE_HANDLER(CPU_I8255A_TAG, i8255a_pb_r),	// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_HANDLER(fdc_8255_c_r),		// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_NULL,							// Port B write
	DEVCB_HANDLER(fdc_8255_c_w)			// Port C write
};

static TIMER_CALLBACK( pc8801fd_upd765_tc_to_zero )
{
	pc88_state *state = machine->driver_data<pc88_state>();

	upd765_tc_w(state->upd765, 0);
}

READ8_HANDLER( pc8801fd_upd765_tc )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	upd765_tc_w(state->upd765, 1);
	timer_set(space->machine,  attotime_zero, NULL, 0, pc8801fd_upd765_tc_to_zero );
	return 0;
}

const upd765_interface pc8801_fdc_interface =
{
	DEVCB_CPU_INPUT_LINE("sub", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static void pc8801_init_5fd(running_machine *machine)
{
	pc88_state *state = machine->driver_data<pc88_state>();
	state->use_5FD = (input_port_read(machine, "DSW2") & 0x80) != 0x00;

	if (!state->use_5FD)
		machine->device<cpu_device>("sub")->suspend(SUSPEND_REASON_DISABLE, 1);
	else
		machine->device<cpu_device>("sub")->resume(SUSPEND_REASON_DISABLE);

	cpu_set_input_line_vector(machine->device("sub"), 0, 0);

	floppy_mon_w(floppy_get_device(machine, 0), CLEAR_LINE);
	floppy_mon_w(floppy_get_device(machine, 1), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(machine, 0), 1, 1);
	floppy_drive_set_ready_state(floppy_get_device(machine, 1), 1, 1);
}

/*
  FM sound
  */

static void pc88sr_init_fmsound(running_machine *machine)
{
	pc88_state *state = machine->driver_data<pc88_state>();
  state->enable_FM_IRQ=0;
  state->FM_IRQ_save=0;
}

void pc88sr_sound_interupt(running_device *device, int irq)
{
	pc88_state *state = device->machine->driver_data<pc88_state>();
	state->FM_IRQ_save=irq;
	if(state->FM_IRQ_save && state->enable_FM_IRQ)
		pc8801_raise_interrupt(device->machine, FM_IRQ_LEVEL);
}

/*
  KANJI ROM
  */

READ8_HANDLER( pc88_kanji_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	UINT8 *kanji = memory_region(space->machine, "gfx1");
	UINT32 addr = (state->kanji << 1) | !offset;

	return kanji[addr];
}

WRITE8_HANDLER( pc88_kanji_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	switch (offset)
	{
	case 0:
		state->kanji = (state->kanji & 0xff00) | data;
		break;

	case 1:
		state->kanji = (data << 8) | (state->kanji & 0xff);
		break;
	}
}

READ8_HANDLER( pc88_kanji2_r )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	UINT8 *kanji2 = memory_region(space->machine, "kanji2");
	UINT32 addr = (state->kanji2 << 1) | !offset;

	return kanji2[addr];
}

WRITE8_HANDLER( pc88_kanji2_w )
{
	pc88_state *state = space->machine->driver_data<pc88_state>();

	switch (offset)
	{
	case 0:
		state->kanji2 = (state->kanji2 & 0xff00) | data;
		break;

	case 1:
		state->kanji2 = (data << 8) | (state->kanji2 & 0xff);
		break;
	}
}

/* Machine Initialization */

MACHINE_START( pc88srl )
{
	pc88_state *state = machine->driver_data<pc88_state>();

	state->extmem_mode = -1;

	/* find devices */
	state->upd765 = machine->device(UPD765_TAG);
	state->upd1990a = machine->device(UPD1990A_TAG);
	state->cassette = machine->device(CASSETTE_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);
	upd1990a_oe_w(state->upd1990a, 1);

	timer_pulse(machine, ATTOTIME_IN_HZ(600), NULL, 0, pc8801_timer_interrupt);
}

MACHINE_RESET( pc88srl )
{
	pc88sr_ch_reset(machine, 0);
}

MACHINE_START( pc88srh )
{
	MACHINE_START_CALL(pc88srl);
}

MACHINE_RESET( pc88srh )
{
	pc88sr_ch_reset(machine, 1);
}
