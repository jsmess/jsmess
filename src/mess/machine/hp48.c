#include "driver.h"

#include "includes/hp48.h"

/*
  0
  1bc
  1fc6
  1fe3 configure memory
   2248
   2256
  201a power control register loaded (currently 1!)
  202e
   26fd copies 0x36 nibbles from 701c to 700a !
    c192 ram address into d0
	11fc
   2494
   1929
    13437
 */







UINT8 *hp48_ram, *hp48_card1, *hp48_card2;
static int out;

#define HDW 0
#define RAM 1
#define CARD2 2
#define CARD1 3
#define NCE3
#define ROM 4
static struct {
	int state;
	struct {
		int realsize, 
			adr,/* a11..a0 ignored */
			size;/* a19..a12 comparator select, 1 means address line must match adr */
	} mem[5];
} hp48s= {
	0,
	{
		{ 32 }, // no size configure, only a4..a0 ignored
		{ 128*1024 }, //s 32kb, sx 128kb
		{ 32*1024 }, // card2 32 kb or 128kb
		{ 32*1024 }, // card1 32 kb or 128kb
		{ 256*1024, 0 } // not configurable, always visible
	}
};

// to do support weired comparator settings
static void hp48_config(void)
{
	int begin, end;

	// lowest priority first
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0, 0xfffff, 0, 0, MRA8_ROM);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0, 0xfffff, 0, 0, MWA8_NOP);
	if (hp48s.mem[CARD1].adr!=-1)
	{
		begin=hp48s.mem[CARD1].adr&hp48s.mem[CARD1].size&~0xfff;
		end=begin|(hp48s.mem[CARD1].size^0xff000)|0xfff;
		if (end!=begin)
		{
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MRA8_BANK1);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MWA8_BANK1);
			memory_set_bankptr(1, hp48_card1);
		}
	}
	if (hp48s.mem[CARD2].adr!=-1) {
		begin=hp48s.mem[CARD2].adr&hp48s.mem[CARD2].size&~0xfff;
		end=begin|(hp48s.mem[CARD2].size^0xff000)|0xfff;
		if (end!=begin) {
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MRA8_BANK2);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MWA8_BANK2);
			memory_set_bankptr(2, hp48_card2);
		}
	}
	if (hp48s.mem[RAM].adr!=-1) {
		begin=hp48s.mem[RAM].adr&hp48s.mem[RAM].size&~0xfff;
		end=begin|(hp48s.mem[RAM].size^0xff000)|0xfff;
		if (end!=begin) {
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MRA8_BANK3);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, MWA8_BANK3);
			memory_set_bankptr(3, hp48_ram);
		}
	}
	if (hp48s.mem[HDW].adr!=-1)
	{
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, hp48s.mem[HDW].adr&~0x3f, 
								 hp48s.mem[HDW].adr|0x3f, 0, 0, hp48_read);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, hp48s.mem[HDW].adr&~0x3f,
								  hp48s.mem[HDW].adr|0x3f, 0, 0, hp48_write);
	}
	memory_set_context(0);
}

/* priority on the bus
   hdw, ram, ce2, ce1, nce3, rom */
void hp48_mem_reset(void)
{
	int i;
	hp48s.state=0;

	for (i=0; i+1< sizeof(hp48s.mem)/sizeof(hp48s.mem[0]); i++) {
		hp48s.mem[i].adr=-1;
	}
	hp48_config();
}

void hp48_mem_config(int v)
{
	if (hp48s.mem[HDW].adr==-1) {
		hp48s.mem[HDW].adr=v;
		hp48_config();
	} else if (hp48s.mem[RAM].adr==-1) {
		if (hp48s.state==0) {
			hp48s.mem[RAM].size=v;
			hp48s.state++;
		} else {
			hp48s.mem[RAM].adr=v;
			hp48s.state=0;
			hp48_config();
		}
	} else if (hp48s.mem[CARD1].adr==-1) {
		if (hp48s.state==0) {
			hp48s.mem[CARD1].size=v;
			hp48s.state++;
		} else {
			hp48s.mem[CARD1].adr=v;
			hp48s.state=0;
			hp48_config();
		}
	} else if (hp48s.mem[CARD1].adr==-1) {
		if (hp48s.state==0) {
			hp48s.mem[CARD2].size=v;
			hp48s.state++;
		} else {
			hp48s.mem[CARD2].adr=v;
			hp48s.state=0;
			hp48_config();
		}
	}
}

void hp48_mem_unconfig(int v)
{
	int i;
	for (i=0; i+1< sizeof(hp48s.mem)/sizeof(hp48s.mem[0]); i++) {
		if (hp48s.mem[i].adr==v) {
			hp48s.mem[i].adr=-1;
			hp48s.state=0;
			hp48_config();
			break;
		}
	}
}

#define HDW_ID_ADR 0x19
#define RAM_ID_SIZE 3
#define RAM_ID_ADR 0xf4
#define CE1_ID_SIZE 5
#define CE1_ID_ADR 0xf6
#define CE2_ID_SIZE 7
#define CE2_ID_ADR 0xf8
#define NCE3_ID_SIZE 1
#define NCE3_ID_ADR 0xf2
int hp48_mem_id(void)
{
	if (hp48s.mem[HDW].adr==-1) {
		return (hp48s.mem[HDW].adr&~0x3f)|HDW_ID_ADR;
    } else if (hp48s.mem[RAM].adr==-1) {
		if (hp48s.state==0) return (hp48s.mem[RAM].size&~0xff)|RAM_ID_SIZE;
		else return (hp48s.mem[RAM].size&~0xff)|RAM_ID_ADR;
    } else if (hp48s.mem[CARD1].adr==-1) {
		if (hp48s.state==0) return (hp48s.mem[CARD1].size&~0xff)|RAM_ID_SIZE;
		else return (hp48s.mem[CARD1].size&~0xff)|RAM_ID_ADR;
    } else if (hp48s.mem[CARD2].adr==-1) {
		if (hp48s.state==0) return (hp48s.mem[CARD2].size&~0xff)|RAM_ID_SIZE;
		else return (hp48s.mem[CARD2].size&~0xff)|RAM_ID_ADR;
	}
	return 0;
}

#define TIMER1_VALUE hp48_hardware.data[0x37]
HP48_HARDWARE hp48_hardware={ 
	{0} 
};

void hp48_crc(int adr, int data)
{
	if ((hp48s.mem[HDW].adr==-1)
		||(adr<(hp48s.mem[HDW].adr&~0x3f))
		||(adr>(hp48s.mem[HDW].adr|0x3f)) )
		hp48_hardware.crc=(hp48_hardware.crc >> 4) 
			^ (((hp48_hardware.crc ^ data) & 0xf) * 0x1081);
	else 
		hp48_hardware.crc=(hp48_hardware.crc >> 4) 
			^ ((hp48_hardware.crc& 0xf) * 0x1081);
}

WRITE8_HANDLER( hp48_write )
{
	switch (offset) {
		//lcd
	case 0: case 1: case 2: case 3:
	case 0xb: case 0xc:
		hp48_hardware.data[offset]=data;
		break;		
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24:
	case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: //wo
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
		hp48_hardware.data[offset]=data;
		break;		
		// uart
	case 0xd: case 0x10: case 0x11: case 0x12: 
	case 0x13: //w0
	case 0x14: case 0x15: //r0
	case 0x16: case 0x17: //w0
		hp48_hardware.data[offset]=data;
		break;
		// infrared serial
	case 0x1a: case 0x1c: case 0x1d:
		hp48_hardware.data[offset]=data;
		break;
		// timers
	case 0x2e: case 0x2f:
	case 0x37:
	case 0x38: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf)|data;break;
	case 0x39: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf0)|(data<<4);break;
	case 0x3a: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf00)|(data<<8);break;
	case 0x3b: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf000)|(data<<12);break;
	case 0x3c: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf0000)|(data<<16);break;
	case 0x3d: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf00000)|(data<<20);break;
	case 0x3e: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf000000)|(data<<24);break;
	case 0x3f: hp48_hardware.timer2=(hp48_hardware.timer2&~0xf0000000)|(data<<28);break;
		hp48_hardware.data[offset]=data;
		break;
 		// cards
	case 0xe:case 0xf:
		hp48_hardware.data[offset]=data;
		break;
	case 4: // crc low
		hp48_hardware.data[offset]=data;
		hp48_hardware.crc=(hp48_hardware.crc&~0xf)|data;
		break;
	case 5: // crc high
		hp48_hardware.data[offset]=data;
		hp48_hardware.crc=(hp48_hardware.crc&~0xf0)|(data<<4);
		break;
	case 6: // crc high
		hp48_hardware.data[offset]=data;
		hp48_hardware.crc=(hp48_hardware.crc&~0xf00)|(data<<8);
		break;
	case 7:
		hp48_hardware.data[offset]=data;
		hp48_hardware.crc=(hp48_hardware.crc&~0xf000)|(data<<12);
		break;
	case 8: //power state
		hp48_hardware.data[offset]=data;
		break;
	case 9: // power control
		// 0 rst input
		// 1 grst input
		// 2 enable very low batterie interrupt
		// 3 enable low batterie interrupt
		hp48_hardware.data[offset]=data;
		break;
	case 0xa: //ro chip mode
		hp48_hardware.data[offset]=data;
		break;
	case 0x18: case 0x19: //lsb of service request
		hp48_hardware.data[offset]=data;
		break;		
	case 0x1b: // base nibble offset
		hp48_hardware.data[offset]=data;
		break;
	case 0x1e: // scratch pad, used by the interrupt handler
		hp48_hardware.data[offset]=data;
		break;
	case 0x1f: // IRAM@ 7 for s/sx, 8 for g/gx
		hp48_hardware.data[offset]=data;
		break;
	default: //temporary for better debugging, write of ro registers
		hp48_hardware.data[offset]=data;
	}
}

 READ8_HANDLER( hp48_read )
{
	int data=0;
	switch (offset) {
		// lcd 
	case 0: case 1: case 2: case 3:
	case 9:
		data=1; //reset
		break;
	case 0xb: case 0xc:
		data=hp48_hardware.data[offset];
		break;
	case 0x28: // ro line count / rasterline?
		break;
	case 0x29: // ro line count / rasterline?
		break;
		// cards
	case 0xe:case 0xf:
		data=hp48_hardware.data[offset];
		break;
	case 4: data=hp48_hardware.crc&0xf;break;
	case 5: data=(hp48_hardware.crc>>4)&0xf;break;
	case 6: data=(hp48_hardware.crc>>8)&0xf;break;
	case 7: data=(hp48_hardware.crc>>12)&0xf;break;
	case 0x38: data=hp48_hardware.timer2&0xf;break;
	case 0x39: data=(hp48_hardware.timer2>>4)&0xf;break;
	case 0x3a: data=(hp48_hardware.timer2>>8)&0xf;break;
	case 0x3b: data=(hp48_hardware.timer2>>12)&0xf;break;
	case 0x3c: data=(hp48_hardware.timer2>>16)&0xf;break;
	case 0x3d: data=(hp48_hardware.timer2>>20)&0xf;break;
	case 0x3e: data=(hp48_hardware.timer2>>24)&0xf;break;
	case 0x3f: data=(hp48_hardware.timer2>>28)&0xf;break;
	default: //temporary for better debugging, read of wo registers
		data=hp48_hardware.data[offset];
	}
	return data;
}

static void hp48_timer(int param)
{
	static int delay=256;
	if (--delay==0) {
		delay=256;
		TIMER1_VALUE=(TIMER1_VALUE-1)&0xf;
		if (TIMER1_VALUE==0) {
		}
	}
	hp48_hardware.timer2--;
	if (hp48_hardware.timer2==0) {
	}
}

void hp48_out(int v)
{
	out=v;
}

int hp48_in(void)
{
	int data=0;
#if 1
	if (out&0x100) {
		if (KEY_B) data|=0x10;
		if (KEY_C) data|=8;
		if (KEY_D) data|=4;
		if (KEY_E) data|=2;
		if (KEY_F) data|=1;
	}
	if (out&0x80) {
		if (KEY_H) data|=0x10;
		if (KEY_I) data|=8;
		if (KEY_J) data|=4;
		if (KEY_K) data|=2;
		if (KEY_L) data|=1;
	}
	if (out&0x40) {
		if (KEY_N) data|=0x10;
		if (KEY_O) data|=8;
		if (KEY_P) data|=4;
		if (KEY_Q) data|=2;
		if (KEY_R) data|=1;
	}
	if (out&0x20) {
		if (KEY_T) data|=0x10;
		if (KEY_U) data|=8;
		if (KEY_V) data|=4;
		if (KEY_W) data|=2;
		if (KEY_X) data|=1;
	}
	if (out&0x10) {
		if (KEY_ON) data|=0x20;
		if (KEY_ENTER) data|=0x10;
		if (KEY_Y) data|=8;
		if (KEY_Z) data|=4;
		if (KEY_DEL) data|=2;
		if (KEY_LEFT) data|=1;
	}
	if (out&8) {
		if (KEY_ALPHA) data|=0x20;
		if (KEY_S) data|=0x10;
		if (KEY_7) data|=8;
		if (KEY_8) data|=4;
		if (KEY_9) data|=2;
		if (KEY_DIVIDE) data|=1;
	}
	if (out&4) {
		if (KEY_ORANGE) data|=0x20;
		if (KEY_G) data|=0x10;
		if (KEY_4) data|=8;
		if (KEY_5) data|=4;
		if (KEY_6) data|=2;
		if (KEY_MULTIPLY) data|=1;
	}
	if (out&2) {
		if (KEY_BLUE) data|=0x20;
		if (KEY_A) data|=0x10;
		if (KEY_1) data|=8;
		if (KEY_2) data|=4;
		if (KEY_3) data|=2;
		if (KEY_MINUS) data|=1;
	}
	if (out&1) {
		if (KEY_M) data|=0x10;
		if (KEY_0) data|=8;
		if (KEY_POINT) data|=4;
		if (KEY_SPC) data|=2;
		if (KEY_PLUS) data|=1;
	}
#else
	if (out&0x100) data|=input_port_0_r(0);
	if (out&0x080) data|=input_port_1_r(0);
	if (out&0x040) data|=input_port_2_r(0);
	if (out&0x020) data|=input_port_3_r(0);
	if (out&0x010) data|=input_port_4_r(0);
	if (out&0x008) data|=input_port_5_r(0);
	if (out&0x004) data|=input_port_6_r(0);
	if (out&0x002) data|=input_port_7_r(0);
	if (out&0x001) data|=input_port_8_r(0);
#endif
	return data;
}

DRIVER_INIT( hp48s )
{
	int i,j,t;
	UINT8 *mem=memory_region(REGION_CPU1);
	for (i=0x3ffff,j=0x7fffe; i>=0;i--,j-=2) {
		t=mem[i];
		mem[j+1]=t>>4;
		mem[j]=t&0xf;
	}
	memset(mem+0x80000, 0, 0x80000); //unused area must be 0!
	memset(mem+0x140000, 0, 0x40000); //unused area must be 0!
	memset(mem+0x180000, 0, 0x40000); //unused area must be 0!

	hp48_ram=memory_region(REGION_CPU1)+0x100000;
	hp48_card1=memory_region(REGION_CPU1)+0x140000;
	hp48_card2=memory_region(REGION_CPU1)+0x180000;

	timer_pulse(1.0/8192, 0, hp48_timer);
}

DRIVER_INIT( hp48g )
{
	int i,j,t;
	UINT8 *mem=memory_region(REGION_CPU1);
	UINT8 *gfx=memory_region(REGION_GFX1);

	for (i=0x7ffff,j=0xffffe; i>=0;i--,j-=2) {
		t=mem[i];
		mem[j]=t>>4;
		mem[j+1]=t&0xf;
	}
	for (i=0; i<256; i++) gfx[i]=i;

	hp48_ram=memory_region(REGION_CPU1)+0x100000;
	hp48_card1=memory_region(REGION_CPU1)+0x140000;
	hp48_card2=memory_region(REGION_CPU1)+0x180000;

	timer_pulse(1.0/8192, 0, hp48_timer);
}

MACHINE_RESET( hp48 )
{
	hp48_mem_reset();
}
