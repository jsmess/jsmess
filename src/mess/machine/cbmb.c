/***************************************************************************
	commodore b series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "deprecat.h"

#include "includes/cbm.h"
#include "machine/tpi6525.h"
#include "video/vic6567.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/cbmdrive.h"

#include "includes/cbmb.h"

#include "devices/cartslot.h"

/* 2008-05 FP: Were these added as a reminder to add configs of 
drivers 8 & 9 as in pet.c ? */
#define IEEE8ON 0
#define IEEE9ON 0

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	{ \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time()), (char*) M ); \
			logerror A; \
		} \
	}

static TIMER_CALLBACK(cbmb_frame_interrupt);

/* keyboard lines */
static int cbmb_keyline_a, cbmb_keyline_b, cbmb_keyline_c;

static int p500 = 0;
static int cbm700;
static int cbm_ntsc;
UINT8 *cbmb_basic;
UINT8 *cbmb_kernal;
static UINT8 *cbmb_chargen;
UINT8 *cbmb_videoram;
UINT8 *cbmb_colorram;
UINT8 *cbmb_memory;

/* tpi at 0xfde00
 in interrupt mode
 irq to m6509 irq
 pa7 ieee nrfd
 pa6 ieee ndac
 pa5 ieee eoi
 pa4 ieee dav
 pa3 ieee atn
 pa2 ieee ren
 pa1 ieee io chips te
 pa0 ieee io chip dc
 pb1 ieee seq
 pb0 ieee ifc
 cb ?
 ca chargen rom address line 12
 i4 acia irq
 i3 ?
 i2 cia6526 irq
 i1 self pb0
 i0 tod (50 or 60 hertz frequency)
 */
static int cbmb_tpi0_port_a_r(void)
{
	int data = 0;
	if (cbm_ieee_nrfd_r()) data |= 0x80;
	if (cbm_ieee_ndac_r()) data |= 0x40;
	if (cbm_ieee_eoi_r()) data |= 0x20;
	if (cbm_ieee_dav_r()) data |= 0x10;
	if (cbm_ieee_atn_r()) data |= 0x08;
/*	if (cbm_ieee_ren_r()) data |= 0x04; */
	return data;
}

static void cbmb_tpi0_port_a_w(int data)
{
	cbm_ieee_nrfd_w(0, data & 0x80);
	cbm_ieee_ndac_w(0, data & 0x40);
	cbm_ieee_eoi_w(0, data & 0x20);
	cbm_ieee_dav_w(0, data & 0x10);
	cbm_ieee_atn_w(0, data & 0x08);
/*	cbm_ieee_ren_w(0, data & 0x04); */
	logerror("cbm ieee %d %d\n", data & 0x02, data & 0x01);
}

/* tpi at 0xfdf00
  cbm 500
   port c7 video address lines?
   port c6 ?
  cbm 600
   port c7 low
   port c6 ntsc 1, 0 pal
  cbm 700
   port c7 high
   port c6 ?
  port c5 .. c0 keyboard line select
  port a7..a0 b7..b0 keyboard input */
static void cbmb_keyboard_line_select_a(int line)
{
	cbmb_keyline_a=line;
}

static void cbmb_keyboard_line_select_b(int line)
{
	cbmb_keyline_b=line;
}

static void cbmb_keyboard_line_select_c(int line)
{
	cbmb_keyline_c=line;
}

static int cbmb_keyboard_line_a(void)
{
	int data = 0;
	
	if (!(cbmb_keyline_c & 0x01)) 
		data |= input_port_read(Machine, "ROW0");

	if (!(cbmb_keyline_c & 0x02)) 
		data |= input_port_read(Machine, "ROW2");

	if (!(cbmb_keyline_c & 0x04)) 
		data |= input_port_read(Machine, "ROW4");

	if (!(cbmb_keyline_c & 0x08)) 
		data |= input_port_read(Machine, "ROW6");

	if (!(cbmb_keyline_c & 0x10)) 
		data |= input_port_read(Machine, "ROW8");

	if (!(cbmb_keyline_c & 0x20)) 
		data |= input_port_read(Machine, "ROW10");

	return data ^0xff;
}

static int cbmb_keyboard_line_b(void)
{
	int data = 0;
	
	if (!(cbmb_keyline_c & 0x01)) 
		data |= input_port_read(Machine, "ROW1");

	if (!(cbmb_keyline_c & 0x02)) 
		data |= input_port_read(Machine, "ROW3");

	if (!(cbmb_keyline_c & 0x04)) 
		data |= input_port_read(Machine, "ROW5");

	if (!(cbmb_keyline_c & 0x08)) 
		data |= input_port_read(Machine, "ROW7");

	if (!(cbmb_keyline_c & 0x10)) 
		data |= input_port_read(Machine, "ROW9") | ((input_port_read(Machine, "SPECIAL") & 0x04) ? 1 : 0 );

	if (!(cbmb_keyline_c & 0x20)) 
		data |= input_port_read(Machine, "ROW11");

	return data ^0xff;
}

static int cbmb_keyboard_line_c(void)
{
	int data = 0;

	if ((input_port_read(Machine, "ROW0") & ~cbmb_keyline_a) || 
				(input_port_read(Machine, "ROW1") & ~cbmb_keyline_b)) 
		 data |= 0x01;

	if ((input_port_read(Machine, "ROW2") & ~cbmb_keyline_a) || 
				(input_port_read(Machine, "ROW3") & ~cbmb_keyline_b)) 
		 data |= 0x02;

	if ((input_port_read(Machine, "ROW4") & ~cbmb_keyline_a) || 
				(input_port_read(Machine, "ROW5") & ~cbmb_keyline_b)) 
		 data |= 0x04;

	if ((input_port_read(Machine, "ROW6") & ~cbmb_keyline_a) || 
				(input_port_read(Machine, "ROW7") & ~cbmb_keyline_b)) 
		 data |= 0x08;

	if ((input_port_read(Machine, "ROW8") & ~cbmb_keyline_a) || 
				((input_port_read(Machine, "ROW9") | ((input_port_read(Machine, "SPECIAL") & 0x04) ? 1 : 0)) & ~cbmb_keyline_b)) 
		 data |= 0x10;

	if ((input_port_read(Machine, "ROW10") & ~cbmb_keyline_a) || 
				(input_port_read(Machine, "ROW11") & ~cbmb_keyline_b)) 
		 data |= 0x20;

	if (!p500) 
	{
		if (!cbm_ntsc) 
			data |= 0x40;

		if (!cbm700) 
			data |= 0x80;
	}
	return data ^0xff;
}

static void cbmb_irq (running_machine *machine, int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6509", ("irq %s\n", level ? "start" : "end"));
		cpunum_set_input_line(machine, 0, M6502_IRQ_LINE, level);
		old_level = level;
	}
}

/*
  port a ieee in/out
  pa7 trigger gameport 2
  pa6 trigger gameport 1
  pa1 ?
  pa0 ?
  pb7 .. 4 gameport 2
  pb3 .. 0 gameport 1
 */
static UINT8 cbmb_cia_port_a_r(void)
{
	return cbm_ieee_data_r();
}

static void cbmb_cia_port_a_w(UINT8 data)
{
	cbm_ieee_data_w(0, data);
}

static void cbmb_tpi6525_0_irq2_level( running_machine *machine, int level )
{
	tpi6525_0_irq2_level(machine, level);
}

static const cia6526_interface cbmb_cia =
{
	CIA6526,
	cbmb_tpi6525_0_irq2_level,
	0.0, 60,

	{
		{ cbmb_cia_port_a_r, cbmb_cia_port_a_w },
		{ 0, 0 }
	}
};

WRITE8_HANDLER ( cbmb_colorram_w )
{
	cbmb_colorram[offset] = data | 0xf0;
}

static int cbmb_dma_read(int offset)
{
	if (offset >= 0x1000)
		return cbmb_videoram[offset & 0x3ff];
	else
		return cbmb_chargen[offset & 0xfff];
}

static int cbmb_dma_read_color(int offset)
{
	return cbmb_colorram[offset & 0x3ff];
}

static void cbmb_change_font(running_machine *machine, int level)
{
	cbmb_vh_set_font(level);
}

static void cbmb_common_driver_init(running_machine *machine)
{
	cbmb_chargen=memory_region(machine, "main") + 0x100000;
	/*    memset(c64_memory, 0, 0xfd00); */

	cia_config(machine, 0, &cbmb_cia);

	tpi6525[0].a.read = cbmb_tpi0_port_a_r;
	tpi6525[0].a.output = cbmb_tpi0_port_a_w;
	tpi6525[0].ca.output = cbmb_change_font;
	tpi6525[0].interrupt.output = cbmb_irq;
	tpi6525[1].a.read = cbmb_keyboard_line_a;
	tpi6525[1].b.read = cbmb_keyboard_line_b;
	tpi6525[1].c.read = cbmb_keyboard_line_c;
	tpi6525[1].a.output = cbmb_keyboard_line_select_a;
	tpi6525[1].b.output = cbmb_keyboard_line_select_b;
	tpi6525[1].c.output = cbmb_keyboard_line_select_c;

	timer_pulse(ATTOTIME_IN_MSEC(10), NULL, 0, cbmb_frame_interrupt);

	p500 = 0;
	cbm700 = 0;
	cbm_ieee_open();
}

DRIVER_INIT( cbm600 )
{
	cbmb_common_driver_init(machine);
	cbm_ntsc = 1;
	cbm600_vh_init(machine);
}

DRIVER_INIT( cbm600pal )
{
	cbmb_common_driver_init(machine);
	cbm_ntsc = 0;
	cbm600_vh_init(machine);
}

DRIVER_INIT( cbm600hu )
{
	cbmb_common_driver_init(machine);
	cbm_ntsc = 0;
}

DRIVER_INIT( cbm700 )
{
	cbmb_common_driver_init(machine);
	cbm700 = 1;
	cbm_ntsc = 0;
	cbm700_vh_init(machine);
}

DRIVER_INIT( p500 )
{
	cbmb_common_driver_init(machine);
	p500 = 1;
	cbm_ntsc = 1;
	vic6567_init(0, 0, cbmb_dma_read, cbmb_dma_read_color, NULL);
}

MACHINE_RESET( cbmb )
{
	sndti_reset(SOUND_SID6581, 0);
	cia_reset();
	tpi6525_0_reset();
	tpi6525_1_reset();

	cbm_drive_0_config (IEEE8ON ? IEEE : 0, 8);
	cbm_drive_1_config (IEEE9ON ? IEEE : 0, 9);
}


static TIMER_CALLBACK(cbmb_frame_interrupt)
{
	static int level = 0;
#if 0
	int controller1 = input_port_read(Machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(Machine, "CTRLSEL") & 0x70;
#endif

	tpi6525_0_irq0_level(machine, level);
	level = !level;
	if (level) return ;

#if 0
	value = 0xff;
	switch(controller1)
	{
		case 0x00:
			value &= ~input_port_read(machine, "JOY1_1B");			/* Joy1 Directions + Button 1 */
			break;
		
		case 0x01:
			if (input_port_read(machine, "OTHER") & 0x40)			/* Paddle2 Button */
				value &= ~0x08;
			if (input_port_read(machine, "OTHER") & 0x80)			/* Paddle1 Button */
				value &= ~0x04;
			break;

		case 0x02:
			if (input_port_read(machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;
			
		case 0x03:
			value &= ~(input_port_read(machine, "JOY1_2B") & 0x1f);	/* Joy1 Directions + Button 1 */
			break;
		
		case 0x04:
/* was there any input on the lightpen? where is it mapped? */
//			if (input_port_read(machine, "OTHER") & 0x04)			/* Lightpen Signal */
//				value &= ?? ;
			break;

		case 0x07:
			break;

		default:
			logerror("Invalid Controller 1 Setting %d\n", controller1);
			break;
	}

	c64_keyline[8] = value;


	value = 0xff;
	switch(controller2)
	{
		case 0x00:
			value &= ~input_port_read(machine, "JOY2_1B");			/* Joy2 Directions + Button 1 */
			break;
		
		case 0x10:
			if (input_port_read(machine, "OTHER") & 0x10)			/* Paddle4 Button */
				value &= ~0x08;
			if (input_port_read(machine, "OTHER") & 0x20)			/* Paddle3 Button */
				value &= ~0x04;
			break;

		case 0x20:
			if (input_port_read(machine, "OTHER") & 0x02)			/* Mouse Button Left */
				value &= ~0x10;
			if (input_port_read(machine, "OTHER") & 0x01)			/* Mouse Button Right */
				value &= ~0x01;
			break;
		
		case 0x30:
			value &= ~(input_port_read(machine, "JOY2_2B") & 0x1f);	/* Joy2 Directions + Button 1 */
			break;

		case 0x40:
/* was there any input on the lightpen? where is it mapped? */
//			if (input_port_read(machine, "OTHER") & 0x04)			/* Lightpen Signal */
//				value &= ?? ;
			break;

		case 0x70:
			break;

		default:
			logerror("Invalid Controller 2 Setting %d\n", controller2);
			break;
	}

	c64_keyline[9] = value;
#endif

	vic2_frame_interrupt (machine, 0);

	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x04 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

	CBM Business Computers Cartridges

***********************************************/


static CBM_ROM cbmb_cbm_cart[0x20]= { {0} };

static DEVICE_IMAGE_LOAD(cbmb_cart)
{
	int size = image_length(image), test;
	const char *filetype;
	int address = 0;

	filetype = image_filetype(image);

 	if (!mame_stricmp (filetype, "crt"))
	{
	/* We temporarily remove .crt loading. Previous versions directly used 
	the same routines used to load C64 .crt file, but I seriously doubt the
	formats are compatible. While waiting for confirmation about .crt dumps
	for CBMB machines, we simply do not load .crt files */
	}
	else 
	{
		/* Assign loading address according to extension */
		if (!mame_stricmp (filetype, "10"))
			address = 0x1000;

		else if (!mame_stricmp (filetype, "20"))
			address = 0x2000;

		else if (!mame_stricmp (filetype, "40"))
			address = 0x4000;

		else if (!mame_stricmp (filetype, "60"))
			address = 0x6000;

		logerror("Loading cart %s at %.4x size:%.4x\n", image_filename(image), address, size);

		/* Does cart contain any data? */
		cbmb_cbm_cart[0].chip = (UINT8*) image_malloc(image, size);
		if (!cbmb_cbm_cart[0].chip)
			return INIT_FAIL;

		/* Store data, address & size */
		cbmb_cbm_cart[0].addr = address;
		cbmb_cbm_cart[0].size = size;
		test = image_fread(image, cbmb_cbm_cart[0].chip, cbmb_cbm_cart[0].size);

		if (test != cbmb_cbm_cart[0].size)
			return INIT_FAIL;
	}
				
	/* Finally load the cart */
//	This could be needed with .crt support
//	for (i = 0; (i < sizeof(pet_cbm_cart) / sizeof(pet_cbm_cart[0])) && (pet_cbm_cart[i].size != 0); i++) 
//		memcpy(cbmb_memory + cbmb_cbm_cart[i].addr + 0xf0000, cbmb_cbm_cart[i].chip, cbmb_cbm_cart[i].size);
	memcpy(cbmb_memory + cbmb_cbm_cart[0].addr + 0xf0000, cbmb_cbm_cart[0].chip, cbmb_cbm_cart[0].size);

	return INIT_PASS;
}

void cbmb_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt,10,20,40,60"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(cbmb_cart); break;

		default:									cartslot_device_getinfo(devclass, state, info); break;
	}
}
