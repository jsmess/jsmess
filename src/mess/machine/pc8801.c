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

#include "mscommon.h"
#include "driver.h"
#include "timer.h"
#include "includes/pc8801.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
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
void pc8801_init_5fd(void);

static void pc88sr_init_fmsound(void);
static int enable_FM_IRQ;
static int FM_IRQ_save;
#define FM_IRQ_LEVEL 4

/*
  calender IC (PD1990)
  */

static UINT8 calender_reg[5];
static int calender_save;
static int calender_hold;

WRITE8_HANDLER(pc8801_calender)
{
  calender_save=data;
  /* printer (not yet) */
  /* UOP3 (bit 7 -- not yet) */
}

static void calender_strobe(void)
{
	mame_system_time systime;

	switch(calender_save&0x07)
	{
		case 0:
			calender_hold = 1;
			break;
		case 1:
			calender_hold = 0;
			break;
		case 2:
			/* time set (not yet) */
			calender_hold = 1;
			break;
		case 3:
			/* get the current date/time from the core */
			mame_get_current_datetime(Machine, &systime);

			calender_reg[4] = (systime.local_time.month + 1) * 16 + systime.local_time.weekday;
			calender_reg[3] = dec_2_bcd(systime.local_time.mday);
			calender_reg[2] = dec_2_bcd(systime.local_time.hour);
			calender_reg[1] = dec_2_bcd(systime.local_time.minute);
			calender_reg[0] = dec_2_bcd(systime.local_time.second);
			calender_hold = 1;
			break;
	}
}

static void calender_shift(void)
{
  if(!calender_hold) {
    calender_reg[0] = (calender_reg[0] >> 1) | ((calender_reg[1] << 7) & 0x80);
    calender_reg[1] = (calender_reg[1] >> 1) | ((calender_reg[2] << 7) & 0x80);
    calender_reg[2] = (calender_reg[2] >> 1) | ((calender_reg[3] << 7) & 0x80);
    calender_reg[3] = (calender_reg[3] >> 1) | ((calender_reg[4] << 7) & 0x80);
    calender_reg[4] = (calender_reg[4] >> 1) | ((calender_save << 4) & 0x80);
  }
}

static int calender_data(void)
{
  return (calender_reg[0]&0x01)!=0x00;
}

/* interrupt staff */

static int interrupt_level_reg;
static int interrupt_mask_reg;
static int interrupt_trig_reg;

static void pc8801_update_interrupt(void)
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
		cpunum_set_input_line (0, 0, HOLD_LINE);
	}
}

static int pc8801_interupt_callback (int cpu)
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

WRITE8_HANDLER(pc8801_write_interrupt_level)
{
	interrupt_level_reg = data&0x0f;
	pc8801_update_interrupt();
}

WRITE8_HANDLER(pc8801_write_interrupt_mask)
{
	interrupt_mask_reg = ((data&0x01)<<2) | (data&0x02)
		| ((data&0x04)>>2) | 0xf8;
}

static void pc8801_raise_interrupt(int level)
{
  interrupt_trig_reg |= interrupt_mask_reg & (1<<level);
  pc8801_update_interrupt();
}

INTERRUPT_GEN( pc8801_interrupt )
{
	pc8801_raise_interrupt(1);
}

static void pc8801_timer_interrupt(int dummy)
{
  pc8801_raise_interrupt(2);
}

static void pc8801_init_interrupt(void)
{
	interrupt_level_reg=0;
	interrupt_mask_reg=0xf8;
	interrupt_trig_reg=0x0;
	cpunum_set_irq_callback(0,pc8801_interupt_callback);
	timer_pulse(TIME_IN_HZ(600),0,pc8801_timer_interrupt);
}

WRITE8_HANDLER(pc88sr_outport_30)
{
  /* bit 1-5 not implemented yet */
  pc88sr_disp_30(offset,data);
}

WRITE8_HANDLER(pc88sr_outport_40)
     /* bit 3,4,6 not implemented */
     /* bit 7 incorrect behavior */
{
  static int port_save;

  if((port_save&0x02) == 0x00 && (data&0x02) != 0x00) calender_strobe();
  if((port_save&0x04) == 0x00 && (data&0x04) != 0x00) calender_shift();
  port_save=data;

  if((input_port_17_r(0)&0x40)==0x00) {
    data&=0x7f;
  }
  switch(data&0xa0) {
  case 0x00:
    beep_set_state(0, 0);
    break;
  case 0x20:
    beep_set_frequency(0, 2400);
    beep_set_state(0, 1);
    break;
  case 0x80:
  case 0xa0:
    beep_set_frequency(0, 0);
    beep_set_state(0, 1);
    break;
  }
}

 READ8_HANDLER(pc88sr_inport_40)
     /* bit0, 2 not implemented */
{
  int r;

  r = pc8801_is_24KHz ? 0x00 : 0x02;
  r |= use_5FD ? 0x00 : 0x08;
  r |= calender_data() ? 0x10 : 0x00;
  if(cpu_getvblank()) r|=0x20;
  return r|0xc0;
}

 READ8_HANDLER(pc88sr_inport_30)
     /* DIP-SW1
	bit 0: BASIC selection (0 = N-BASIC, 1 = N88-BASIC)
	bit 1: terminal mode (0 = terminal mode, 1 = BASIC mode)
	bit 2: startup text width (0 = 80chars/line, 1 = 40chars/line)
	bit 3: startup text height (0 = 25lines/screen, 1 = 20lines/screen)
	bit 4: S parameter (0 = enable, 1 = disable)
	bit 5: operation when recevied DEL code (0 = handle DEL code,
						 1 = ignore DEL code)
	bit 6: universal input port 1 (currently always 1)
	bit 7: universal input port 2 (currently always 1)
      */
{
  int r;

  /* read DIP-SW */
  r=input_port_17_r(0)<<1;
  /* change bit 0 according BASIC mode */
  if(is_Nbasic) {
    r&=0xfe;
  } else {
    r|=0x01;
  }
  /* force set bit 6, 7 */
  r|=0xc0;
  return r;
}

 READ8_HANDLER(pc88sr_inport_31)
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
  r=input_port_18_r(0)<<1;
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

static WRITE8_HANDLER ( pc8801_writemem1 )
{
  pc8801_mainRAM[offset]=data;
}

static WRITE8_HANDLER ( pc8801_writemem2 )
{
  pc8801_mainRAM[offset+0x6000]=data;
}

static  READ8_HANDLER ( pc8801_read_textwindow )
{
  return pc8801_mainRAM[(offset+text_window*0x100)&0xffff];
}

static WRITE8_HANDLER ( pc8801_write_textwindow )
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
	*r=(x); \
      } \
    } \
  } while(0);

#define SET_WADDR(x) \
  do { \
    if(w!=NULL) { \
      if(*w!=NULL) { \
	logerror("write multiple bank of extension memory.\n"); \
      } else { \
	*w=(x); \
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

void pc8801_update_bank(void)
{
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
		memory_set_bankptr(1, ext_r + 0x0000);
		memory_set_bankptr(2, ext_r + 0x6000);
		if(ext_w==NULL)
		{
			/* read only mode */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x5fff, 0, 0, MWA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_NOP);
		}
		else
		{
			/* r/w mode */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x5fff, 0, 0, MWA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_BANK2);
			if(ext_w!=ext_r) logerror("differnt between read and write bank of extension memory.\n");
		}
	}
	else
	{
		/* 0x0000 to 0x7fff */
		if(RAMmode)
		{
			/* RAM */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x5fff, 0, 0, MWA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_BANK2);
			memory_set_bankptr(1, pc8801_mainRAM + 0x0000);
			memory_set_bankptr(2, pc8801_mainRAM + 0x6000);
		}
		else
		{
			/* ROM */
			/* write through to main RAM */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x5fff, 0, 0, pc8801_writemem1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, pc8801_writemem2);
			if(ROMmode)
			{
				/* N-BASIC */
				memory_set_bankptr(1, mainROM + 0x0000);
				memory_set_bankptr(2, mainROM + 0x6000);
			}
			else
			{
				/* N88-BASIC */
				memory_set_bankptr(1, mainROM + 0x8000);
				if(no4throm==1) {
					/* 4th ROM 1 */
					memory_set_bankptr(2, mainROM + 0x10000 + 0x2000 * no4throm2);
				} else {
					memory_set_bankptr(2, mainROM + 0xe000);
				}
			}
		}
	}

	/* 0x8000 to 0xffff */
	memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x8000, 0x83ff, 0, 0, (RAMmode || ROMmode) ? MRA8_BANK3 : pc8801_read_textwindow);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x83ff, 0, 0, (RAMmode || ROMmode) ? MWA8_BANK3 : pc8801_write_textwindow);

	memory_set_bankptr(4, pc8801_mainRAM + 0x8400);

	if(is_pc8801_vram_select())
	{
		/* VRAM */
		/* already maped */
	}
	else
	{
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, MRA8_BANK5);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xefff, 0, 0, MWA8_BANK5);
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, 0, 0, MRA8_BANK6);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, 0, 0, MWA8_BANK6);

		memory_set_bankptr(5, pc8801_mainRAM + 0xc000);
		if(maptvram)
			memory_set_bankptr(6, pc88sr_textRAM);
		else
			memory_set_bankptr(6, pc8801_mainRAM + 0xf000);
	}
}

 READ8_HANDLER(pc8801_read_extmem)
{
	UINT8 ret[2];
	select_extmem(NULL,NULL,ret);
	return ret[offset];
}

WRITE8_HANDLER(pc8801_write_extmem)
{
  extmem_ctrl[offset]=data;
  pc8801_update_bank();
}

WRITE8_HANDLER(pc88sr_outport_31)
{
  /* bit 5 not implemented */
  RAMmode=((data&0x02)!=0);
  ROMmode=((data&0x04)!=0);
  pc8801_update_bank();
  pc88sr_disp_31(offset,data);
}

 READ8_HANDLER(pc88sr_inport_32)
{
  return(port32_save);
}

WRITE8_HANDLER(pc88sr_outport_32)
{
  /* bit 2, 3 not implemented */
  port32_save=data;
  maptvram=((data&0x10)==0);
  no4throm2=(data&3);
  enable_FM_IRQ=((data & 0x80) == 0x00);
  if(FM_IRQ_save && enable_FM_IRQ) pc8801_raise_interrupt(FM_IRQ_LEVEL);
  pc88sr_disp_32(offset,data);
  pc8801_update_bank();
}

 READ8_HANDLER(pc8801_inport_70)
{
  return text_window;
}

WRITE8_HANDLER(pc8801_outport_70)
{
  text_window=data;
  pc8801_update_bank();
}

WRITE8_HANDLER(pc8801_outport_78)
{
  text_window=((text_window+1)&0xff);
  pc8801_update_bank();
}

 READ8_HANDLER(pc88sr_inport_71)
{
  return(port71_save);
}

WRITE8_HANDLER(pc88sr_outport_71)
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
  pc8801_update_bank();
}

static void pc8801_init_bank(int hireso)
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
	mainROM = memory_region(REGION_CPU1);
	pc8801_mainRAM = (UINT8 *) auto_malloc (0x10000);
	memset(pc8801_mainRAM, 0, 0x10000);

	extmem_ctrl[0]=extmem_ctrl[1]=0;
	pc8801_update_bank();
	pc8801_video_init(hireso);

  if(extmem_mode!=input_port_19_r(0)) {
    extmem_mode=input_port_19_r(0);
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
      if((extRAM=malloc(num80*0x8000+num88*0x20000+numIO*0x100000))==NULL) {
	logerror ("pc8801: out of memory!\n");
	return;
      }
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
	 (pc88sr_is_highspeed ? 4 : 0)) {
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

static void pc88sr_ch_reset (int hireso)
{
  int a;

  a=input_port_16_r(0);
  is_Nbasic = ((a&0x01)==0x00);
  is_V2mode = ((a&0x02)==0x00);
  pc88sr_is_highspeed = ((a&0x04)!=0x00);
  is_8MHz = ((a&0x08)!=0x00);
  fix_V1V2();
  pc8801_init_bank(hireso);
  pc8801_init_5fd();
  pc8801_init_interrupt();
  beep_set_state(0, 0);
  beep_set_frequency(0, 2400);
  pc88sr_init_fmsound();
}

MACHINE_RESET( pc88srl )
{
  pc88sr_ch_reset(0);
}

MACHINE_RESET( pc88srh )
{
	pc88sr_ch_reset(1);
}

/* 5 inch floppy drive */

static UINT8 load_8255_A(int chip)
{
	return use_5FD ? ppi8255_get_portB(1-chip) : 0xff;
}

static UINT8 load_8255_B(int chip)
{
	return use_5FD ? ppi8255_get_portA(1-chip) : 0xff;
}

static UINT8 load_8255_C(int chip)
{
	UINT8 result = 0xFF;
	UINT8 port_c;

	if (use_5FD)
	{
		port_c = ppi8255_get_portC(1-chip);
		result = ((port_c >> 4) & 0x0F) | ((port_c << 4) & 0xF0);
	}

	/* NPW 11-Feb-2006 - This is a brutal no-good hack to work around bug #759
	 *
	 * The problem is that in MESS 0.96, a regression was introduced that
	 * prevented the PC8801 from booting up.  The source of this regression
	 * was the 8255 PPI rewrite.
	 *
	 * What seems to happen is CPU #0 does a write to 8255 #0 port C at PC=37CF
	 * and this causes a communication to be made to CPU #1.  Under the
	 * previous 8255 code, this write would not actually make it to the the
	 * slave CPU, CPU #1.  After examining the data sheet, all indications are
	 * that the new 8255 behavior is indeed correct.
	 *
	 * So what I'm doing is an ugly hack so that when the slave CPU #1 sees the
	 * value written to it, I actually change this to the previous value,
	 * cloaking the communication.  So the PC8801 now boots up, but hopefully
	 * a correct fix can be made.
	 */
	if ((chip == 1) && (result == 0xF8))
		result = 0xF0;

	return result;
}

static READ8_HANDLER( load_8255_chip0_A )	{ return load_8255_A(0); }
static READ8_HANDLER( load_8255_chip1_A )	{ return load_8255_A(1); }
static READ8_HANDLER( load_8255_chip0_B )	{ return load_8255_B(0); }
static READ8_HANDLER( load_8255_chip1_B )	{ return load_8255_B(1); }
static READ8_HANDLER( load_8255_chip0_C )	{ return load_8255_C(0); }
static READ8_HANDLER( load_8255_chip1_C )	{ return load_8255_C(1); }


const ppi8255_interface pc8801_8255_config =
{
	2,
	{ load_8255_chip0_A, load_8255_chip1_A },
	{ load_8255_chip0_B, load_8255_chip1_B },
	{ load_8255_chip0_C, load_8255_chip1_C },
	{ NULL, NULL },
	{ NULL, NULL },
	{ NULL, NULL },
};

 READ8_HANDLER(pc8801fd_nec765_tc)
{
  nec765_set_tc_state(1);
  nec765_set_tc_state(0);
  return 0;
}

/* callback for /INT output from FDC */
static void pc8801_fdc_interrupt(int state)
{
    cpunum_set_input_line (1, 0, state ? HOLD_LINE : CLEAR_LINE);
}

/* callback for /DRQ output from FDC */
static void pc8801_fdc_dma_drq(int state, int read_)
{
}

static struct nec765_interface pc8801_fdc_interface=
{
	pc8801_fdc_interrupt,
	pc8801_fdc_dma_drq
};

void pc8801_init_5fd(void)
{
	use_5FD = (input_port_18_r(0)&0x80)!=0x00;
	ppi8255_init(&pc8801_8255_config);
	if (!use_5FD)
		cpunum_suspend(1, SUSPEND_REASON_DISABLE, 1);
	else
		cpunum_resume(1, SUSPEND_REASON_DISABLE);
	nec765_init(&pc8801_fdc_interface,NEC765A);
	cpunum_set_input_line_vector(1,0,0);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1,0);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1,0);
}

/*
  FM sound
  */

static void pc88sr_init_fmsound(void)
{
  enable_FM_IRQ=0;
  FM_IRQ_save=0;
}

void pc88sr_sound_interupt(int irq)
{
  FM_IRQ_save=irq;
  if(FM_IRQ_save && enable_FM_IRQ) pc8801_raise_interrupt(FM_IRQ_LEVEL);
}

/*
  KANJI ROM
  */

static UINT8 kanji_high,kanji_low;

WRITE8_HANDLER(pc8801_write_kanji1)
{
  switch(offset) {
  case 0:
    kanji_low=data;
    break;
  case 1:
    kanji_high=data;
    break;
  }
}

 READ8_HANDLER(pc8801_read_kanji1)
{
  switch(offset) {
  case 0:
    return *(memory_region(REGION_GFX1)+kanji_high*0x200+kanji_low*0x2+1);
  case 1:
    return *(memory_region(REGION_GFX1)+kanji_high*0x200+kanji_low*0x2+0);
  default:
    return 0xff;
  }
}

static UINT8 kanji_high2,kanji_low2;

WRITE8_HANDLER(pc8801_write_kanji2)
{
  switch(offset) {
  case 0:
    kanji_low2=data;
    break;
  case 1:
    kanji_high2=data;
    break;
  }
}

 READ8_HANDLER(pc8801_read_kanji2)
{
  switch(offset) {
  case 0:
    return *(memory_region(REGION_GFX1)+kanji_high2*0x200+kanji_low2*0x2+1+0x20000);
  case 1:
    return *(memory_region(REGION_GFX1)+kanji_high2*0x200+kanji_low2*0x2+0+0x20000);
  default:
    return 0xff;
  }
}
