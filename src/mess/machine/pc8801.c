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

static int ROMmode,RAMmode,maptvram;
static int no4throm,no4throm2,port71_save;
static unsigned char *mainROM;
unsigned char *pc8801_mainRAM=NULL;
static int is_V2mode,is_Nbasic,is_8MHz;
int pc88sr_is_highspeed;
static int port32_save;
static int text_window;
static int extmem_mode=-1;
static unsigned char *extRAM=NULL;
static void *ext_bank_80[4],*ext_bank_88[256];
static UINT8 extmem_ctrl[2];

static int use_5FD;
static void pc8801_init_5fd(running_machine *machine);

static void pc88sr_init_fmsound(void);
static int enable_FM_IRQ;
static int FM_IRQ_save;
#define FM_IRQ_LEVEL 4

WRITE8_HANDLER( pc88_rtc_w )
{
	pc88_state *state = (pc88_state *)space->machine->driver_data;

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

static int interrupt_level_reg;
static int interrupt_mask_reg;
static int interrupt_trig_reg;

static void pc8801_update_interrupt(running_machine *machine)
{
	int level, i;

	level = -1;
	for (i = 0; i < 8; i++)
	{
		if ((interrupt_trig_reg & (1<<i)) != 0)
			level = i;
	}
	if (level >= 0 && level<interrupt_level_reg)
	{
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
	}
}

static IRQ_CALLBACK( pc8801_interrupt_callback )
{
	int level, i;

	level = 0;
	for (i = 0; i < 8; i++)
	{
		if ((interrupt_trig_reg & (1<<i))!=0)
			level = i;
	}
	interrupt_trig_reg &= ~(1<<level);
	return level*2;
}

WRITE8_HANDLER( pc8801_write_interrupt_level )
{
	interrupt_level_reg = data&0x0f;
	pc8801_update_interrupt(space->machine);
}

WRITE8_HANDLER( pc8801_write_interrupt_mask )
{
	interrupt_mask_reg = ((data&0x01)<<2) | (data&0x02)
		| ((data&0x04)>>2) | 0xf8;
}

static void pc8801_raise_interrupt(running_machine *machine, int level)
{
  interrupt_trig_reg |= interrupt_mask_reg & (1<<level);
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
	interrupt_level_reg = 0;
	interrupt_mask_reg = 0xf8;
	interrupt_trig_reg = 0x0;
	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), pc8801_interrupt_callback);
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
	pc88_state *state = (pc88_state *)space->machine->driver_data;
	const device_config *speaker = devtag_get_device(space->machine, "beep");

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
	pc88_state *state = (pc88_state *)space->machine->driver_data;
	UINT8 r;

	r = centronics_busy_r(state->centronics);
	r |= pc8801_is_24KHz ? 0x00 : 0x02;
	r |= use_5FD ? 0x00 : 0x08;
	r |= upd1990a_data_out_r(state->upd1990a) ? 0x10 : 0x00;
	if(video_screen_get_vblank(space->machine->primary_screen)) r|=0x20;

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
  int r;

  /* read DIP-SW */
  r=input_port_read(space->machine, "DSW2")<<1;
  /* change bit 6 according speed switch */
  if(pc88sr_is_highspeed) {
    r|=0x40;
  } else {
    r&=0xbf;
  }
  /* change bit 7 according BASIC mode */
  if(is_V2mode) {
    r&=0x7f;
  } else {
    r|=0x80;
  }
  return r;
}

static WRITE8_HANDLER( pc8801_writemem1 )
{
  pc8801_mainRAM[offset]=data;
}

static WRITE8_HANDLER( pc8801_writemem2 )
{
  pc8801_mainRAM[offset+0x6000]=data;
}

static READ8_HANDLER( pc8801_read_textwindow )
{
  return pc8801_mainRAM[(offset+text_window*0x100)&0xffff];
}

static WRITE8_HANDLER( pc8801_write_textwindow )
{
  pc8801_mainRAM[(offset+text_window*0x100)&0xffff]=data;
}

static void select_extmem(char **r,char **w,UINT8 *ret_ctrl)
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

  if(ext_bank_88[extmem_ctrl[1]]!=NULL) {
    if(extmem_ctrl[0]&0x01) SET_RADDR(ext_bank_88[extmem_ctrl[1]]);
    if(extmem_ctrl[0]&0x10) SET_WADDR(ext_bank_88[extmem_ctrl[1]]);
    SET_RET(0,(~extmem_ctrl[0])&0x11)
    if(ext_bank_88[extmem_ctrl[1]]==ext_bank_88[extmem_ctrl[1]&0x0f]) {
      /* PC-8801-N02 */
      SET_RET(1,extmem_ctrl[1]&0x0f);
    } else {
      /* PIO-8234H */
      SET_RET(1,extmem_ctrl[1]);
    }
  }

  for(i=0;i<4;i++) {
    if(ext_bank_80[i]!=NULL) {
      if(extmem_ctrl[0]&(0x01<<i)) {
	SET_RADDR(ext_bank_80[i]);
	SET_RET(0,(0x01<<i)|0xf0);
      }
      if(extmem_ctrl[0]&(0x10<<i)) SET_WADDR(ext_bank_80[i]);
    }
  }

#undef SET_RADDR
#undef SET_WADDR
#undef SET_RET
}

void pc8801_update_bank(running_machine *machine)
{
	const address_space *program = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	char *ext_r,*ext_w;

	select_extmem(&ext_r,&ext_w,NULL);
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
		if(RAMmode)
		{
			/* RAM */
			memory_install_write_bank(program, 0x0000, 0x5fff, 0, 0, "bank1");
			memory_install_write_bank(program, 0x6000, 0x7fff, 0, 0, "bank2");
			memory_set_bankptr(machine, "bank1", pc8801_mainRAM + 0x0000);
			memory_set_bankptr(machine, "bank2", pc8801_mainRAM + 0x6000);
		}
		else
		{
			/* ROM */
			/* write through to main RAM */
			memory_install_write8_handler(program, 0x0000, 0x5fff, 0, 0, pc8801_writemem1);
			memory_install_write8_handler(program, 0x6000, 0x7fff, 0, 0, pc8801_writemem2);
			if(ROMmode)
			{
				/* N-BASIC */
				memory_set_bankptr(machine, "bank1", mainROM + 0x0000);
				memory_set_bankptr(machine, "bank2", mainROM + 0x6000);
			}
			else
			{
				/* N88-BASIC */
				memory_set_bankptr(machine, "bank1", mainROM + 0x8000);
				if(no4throm==1) {
					/* 4th ROM 1 */
					memory_set_bankptr(machine, "bank2", mainROM + 0x10000 + 0x2000 * no4throm2);
				} else {
					memory_set_bankptr(machine, "bank2", mainROM + 0xe000);
				}
			}
		}
	}

	/* 0x8000 to 0xffff */
	if (RAMmode || ROMmode)  {
		memory_install_readwrite_bank(program, 0x8000, 0x83ff, 0, 0, "bank3");
	} else {
		memory_install_readwrite8_handler(program, 0x8000, 0x83ff, 0, 0, pc8801_read_textwindow, pc8801_write_textwindow);
	}

	memory_set_bankptr(machine, "bank4", pc8801_mainRAM + 0x8400);

	if (pc8801_is_vram_select(machine))
	{
		/* VRAM */
		/* already maped */
	}
	else
	{
		memory_install_readwrite_bank(program, 0xc000, 0xefff, 0, 0, "bank5");
		memory_install_readwrite_bank(program, 0xf000, 0xffff, 0, 0, "bank6");

		memory_set_bankptr(machine, "bank5", pc8801_mainRAM + 0xc000);
		if(maptvram)
			memory_set_bankptr(machine, "bank6", pc88sr_textRAM);
		else
			memory_set_bankptr(machine, "bank6", pc8801_mainRAM + 0xf000);
	}
}

READ8_HANDLER( pc88_extmem_r )
{
	UINT8 ret[2];
	select_extmem(NULL,NULL,ret);
	return ret[offset];
}

WRITE8_HANDLER( pc88_extmem_w )
{
  extmem_ctrl[offset]=data;
  pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc88sr_outport_31 )
{
  /* bit 5 not implemented */
  RAMmode=((data&0x02)!=0);
  ROMmode=((data&0x04)!=0);
  pc8801_update_bank(space->machine);
  pc88sr_disp_31(space, offset,data);
}

READ8_HANDLER( pc88sr_inport_32 )
{
  return(port32_save);
}

WRITE8_HANDLER( pc88sr_outport_32 )
{
  /* bit 2, 3 not implemented */
  port32_save=data;
  maptvram=((data&0x10)==0);
  no4throm2=(data&3);
  enable_FM_IRQ=((data & 0x80) == 0x00);
  if(FM_IRQ_save && enable_FM_IRQ) pc8801_raise_interrupt(space->machine, FM_IRQ_LEVEL);
  pc88sr_disp_32(space, offset,data);
  pc8801_update_bank(space->machine);
}

READ8_HANDLER( pc8801_inport_70 )
{
  return text_window;
}

WRITE8_HANDLER( pc8801_outport_70 )
{
  text_window=data;
  pc8801_update_bank(space->machine);
}

WRITE8_HANDLER( pc8801_outport_78 )
{
  text_window=((text_window+1)&0xff);
  pc8801_update_bank(space->machine);
}

READ8_HANDLER( pc88sr_inport_71 )
{
  return(port71_save);
}

WRITE8_HANDLER( pc88sr_outport_71 )
/* bit 1-7 not implemented (no ROMs) */
{
  port71_save=data;
  switch(data) {
  case 0xff: no4throm=0; break;
  case 0xfe: no4throm=1; break;
  case 0xfd: no4throm=2; break;
  case 0xfb: no4throm=3; break;
  case 0xf7: no4throm=4; break;
  case 0xef: no4throm=5; break;
  case 0xdf: no4throm=6; break;
  case 0xbf: no4throm=7; break;
  case 0x7f: no4throm=8; break;
  default:
    logerror ("pc8801 : write illegal data 0x%.2x to port 0x71, select main rom.\n",data);
    no4throm=0;
    break;
  }
  pc8801_update_bank(space->machine);
}

static void pc8801_init_bank(running_machine *machine, int hireso)
{
	int i,j;
	int num80,num88,numIO;
	unsigned char *e;

	RAMmode=0;
	ROMmode=0;
	maptvram=0;
	no4throm=0;
	no4throm2=0;
	port71_save=0xff;
	port32_save=0x80;
	mainROM = memory_region(machine, "maincpu");
	pc8801_mainRAM = auto_alloc_array(machine, UINT8, 0x10000);
	memset(pc8801_mainRAM, 0, 0x10000);

	extmem_ctrl[0]=extmem_ctrl[1]=0;
	pc8801_update_bank(machine);
	pc8801_video_init(machine, hireso);

  if(extmem_mode!=input_port_read(machine, "MEM")) {
    extmem_mode=input_port_read(machine, "MEM");
    if(extRAM!=NULL) {
      free(extRAM);
      extRAM=NULL;
    }
    for(i=0;i<4;i++) ext_bank_80[i]=NULL;
    for(i=0;i<256;i++) ext_bank_88[i]=NULL;
    num80=num88=numIO=0;
    switch(extmem_mode) {
    case 0x00: /* none */
      break;
    case 0x01: /* 32KB(PC-8012-02 x 1) */
      num80=1;
      break;
    case 0x02: /* 64KB(PC-8012-02 x 2) */
      num80=2;
      break;
    case 0x03: /* 128KB(PC-8012-02 x 4) */
      num80=4;
      break;
    case 0x04: /* 128KB(PC-8801-02N x 1) */
      num88=1;
      break;
    case 0x05: /* 256KB(PC-8801-02N x 2) */
      num88=2;
      break;
    case 0x06: /* 512KB(PC-8801-02N x 4) */
      num88=4;
      break;
    case 0x07: /* 1M(PIO-8234H-1M x 1) */
      numIO=1;
      break;
    case 0x08: /* 2M(PIO-8234H-2M x 1) */
      numIO=2;
      break;
    case 0x09: /* 4M(PIO-8234H-2M x 2) */
      numIO=4;
      break;
    case 0x0a: /* 8M(PIO-8234H-2M x 4) */
      numIO=8;
      break;
    case 0x0b: /* 1.1M(PIO-8234H-1M x 1 + PC-8801-02N x 1) */
      num88=1;
      numIO=1;
      break;
    case 0x0c: /* 2.1M(PIO-8234H-2M x 1 + PC-8801-02N x 1) */
      num88=1;
      numIO=2;
      break;
    case 0x0d: /* 4.1M(PIO-8234H-2M x 2 + PC-8801-02N x 1) */
      num88=1;
      numIO=4;
      break;
    default:
      logerror("pc8801 : illegal extension memory mode.\n");
      return;
    }
    if(num80!=0 || num88!=0 || numIO!=0) {
      extRAM=(UINT8*)malloc(num80*0x8000+num88*0x20000+numIO*0x100000);
      e=extRAM;
      for(i=0;i<num80;i++) {
	ext_bank_80[i]=e;
	e+=0x8000;
      }
      for(i=0;i<num88*4;i++) {
	for(j=i;j<256;j+=16) {
	  ext_bank_88[j]=e;
	}
	e+=0x8000;
      }
      if(num88==0) {
	for(i=0;i<numIO*32;i++) {
	  ext_bank_88[(i&0x07)|((i&0x18)<<1)|((i&0x20)>>2)|(i&0xc0)]=e;
	  e+=0x8000;
	}
      } else {
	for(i=0;i<numIO*32;i++) {
	  ext_bank_88[(i&0x07)|((i&0x78)<<1)|0x08]=e;
	  e+=0x8000;
	}
      }
    }
  }
}

static void fix_V1V2(void)
{
  if(is_Nbasic) is_V2mode=0;
  if(is_V2mode) pc88sr_is_highspeed=1;
  switch((is_Nbasic ? 1 : 0) |
	 (is_V2mode ? 2 : 0) |
	 (pc88sr_is_highspeed ? 4 : 0))
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
    fatalerror("Illegal basic mode=(%d,%d,%d)\n",is_Nbasic,is_V2mode,pc88sr_is_highspeed);
    break;
  }
  if(is_8MHz) {
    logerror(", 8MHz\n");
  } else {
    logerror(", 4MHz\n");
  }
}

static void pc88sr_ch_reset(running_machine *machine, int hireso)
{
	const device_config *speaker = devtag_get_device(machine, "beep");
	int a;

	a=input_port_read(machine, "CFG");
	is_Nbasic = ((a&0x01)==0x00);
	is_V2mode = ((a&0x02)==0x00);
	pc88sr_is_highspeed = ((a&0x04)!=0x00);
	is_8MHz = ((a&0x08)!=0x00);
	fix_V1V2();
	pc8801_init_bank(machine, hireso);
	pc8801_init_5fd(machine);
	pc8801_init_interrupt(machine);
	beep_set_state(speaker, 0);
	beep_set_frequency(speaker, 2400);
	pc88sr_init_fmsound();
}

/* 5 inch floppy drive */

static READ8_DEVICE_HANDLER( cpu_8255_c_r )
{
	pc88_state *state = (pc88_state *)device->machine->driver_data;

	return state->i8255_1_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( cpu_8255_c_w )
{
	pc88_state *state = (pc88_state *)device->machine->driver_data;

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
	pc88_state *state = (pc88_state *)device->machine->driver_data;

	return state->i8255_0_pc >> 4;
}

static WRITE8_DEVICE_HANDLER( fdc_8255_c_w )
{
	pc88_state *state = (pc88_state *)device->machine->driver_data;

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
	pc88_state *state = (pc88_state *)machine->driver_data;

	upd765_tc_w(state->upd765, 0);
}

READ8_HANDLER( pc8801fd_upd765_tc )
{
	pc88_state *state = (pc88_state *)space->machine->driver_data;

	upd765_tc_w(state->upd765, 1);
	timer_set(space->machine,  attotime_zero, NULL, 0, pc8801fd_upd765_tc_to_zero );
	return 0;
}

const upd765_interface pc8801_fdc_interface =
{
	DEVCB_CPU_INPUT_LINE("sub", INPUT_LINE_IRQ0),
	NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static void pc8801_init_5fd(running_machine *machine)
{
	use_5FD = (input_port_read(machine, "DSW2") & 0x80) != 0x00;

	if (!use_5FD)
		cputag_suspend(machine, "sub", SUSPEND_REASON_DISABLE, 1);
	else
		cputag_resume(machine, "sub", SUSPEND_REASON_DISABLE);

	cpu_set_input_line_vector(devtag_get_device(machine, "sub"), 0, 0);

	floppy_mon_w(floppy_get_device(machine, 0), CLEAR_LINE);
	floppy_mon_w(floppy_get_device(machine, 1), CLEAR_LINE);
	floppy_drive_set_ready_state(floppy_get_device(machine, 0), 1, 1);
	floppy_drive_set_ready_state(floppy_get_device(machine, 1), 1, 1);
}

/*
  FM sound
  */

static void pc88sr_init_fmsound(void)
{
  enable_FM_IRQ=0;
  FM_IRQ_save=0;
}

void pc88sr_sound_interupt(const device_config *device, int irq)
{
	FM_IRQ_save=irq;
	if(FM_IRQ_save && enable_FM_IRQ)
		pc8801_raise_interrupt(device->machine, FM_IRQ_LEVEL);
}

/*
  KANJI ROM
  */

READ8_HANDLER( pc88_kanji_r )
{
	pc88_state *state = (pc88_state *)space->machine->driver_data;

	UINT8 *kanji = memory_region(space->machine, "gfx1");
	UINT32 addr = (state->kanji << 1) | !offset;

	return kanji[addr];
}

WRITE8_HANDLER( pc88_kanji_w )
{
	pc88_state *state = (pc88_state *)space->machine->driver_data;

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
	pc88_state *state = (pc88_state *)space->machine->driver_data;

	UINT8 *kanji2 = memory_region(space->machine, "kanji2");
	UINT32 addr = (state->kanji2 << 1) | !offset;

	return kanji2[addr];
}

WRITE8_HANDLER( pc88_kanji2_w )
{
	pc88_state *state = (pc88_state *)space->machine->driver_data;

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
	pc88_state *state = (pc88_state *)machine->driver_data;

	/* find devices */
	state->upd765 = devtag_get_device(machine, UPD765_TAG);
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

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
