/*
	Experimental tm990/189 ("University Module") driver.

	The tm990/189 is a simple board built around a tms9980 at 2.0 MHz.
	The board features:
	* a calculator-like alphanumeric keyboard, a 10-digit 8-segment display,
	  a sound buzzer and 4 status LEDs
	* a 4kb ROM socket (0x3000-0x3fff), and a 2kb ROM socket (0x0800-0x0fff)
	* 1kb of RAM expandable to 2kb (0x0000-0x03ff and 0x0400-0x07ff)
	* a tms9901 controlling a custom parallel I/O port (available for
	  expansion)
	* an optional on-board serial interface (either TTY or RS232): TI ROMs
	  support a terminal connected to this port
	* an optional tape interface
	* an optional bus extension port for adding additional custom devices (TI
	  sold a video controller expansion built around a tms9918, which was
	  supported by University Basic)

	One tms9901 is set up so that it handles tms9980 interrupts.  The other
	tms9901, the tms9902, and extension cards can trigger interrupts on the
	interrupt-handling tms9901.

	TI sold two ROM sets for this machine: a monitor and assembler ("UNIBUG",
	packaged as one 4kb EPROM) and a Basic interpreter ("University BASIC",
	packaged as a 4kb and a 2kb EPROM).  Users could burn and install custom
	ROM sets, too.

	This board was sold to universities to learn either assembly language or
	BASIC programming.

	A few hobbyists may have bought one of these, too.  This board can actually
	be used as a development kit for the tms9980, but it was not supported as
	such (there was no EPROM programmer or mass storage system for the
	tm990/189, though you could definitively design your own and attach them to
	the extension port).

	Raphael Nabet 2003
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/tms9901.h"
#include "machine/tms9902.h"
#include "video/tms9928a.h"
#include "devices/cassette.h"
#include "sound/speaker.h"

static int load_state;
static int ic_state;

static int digitsel;
static int segment;

static void *displayena_timer;
#define displayena_duration TIME_IN_MSEC(4.5)	/* Can anyone confirm this? 74LS123 connected to C=0.1uF and R=100kOhm */
static UINT8 segment_state[10];
static UINT8 old_segment_state[10];
static UINT8 LED_state;

static void *joy1x_timer;
static void *joy1y_timer;
static void *joy2x_timer;
static void *joy2y_timer;

enum
{
	tm990_189_palette_size = 3
};
unsigned char tm990_189_palette[tm990_189_palette_size*3] =
{
	0x00,0x00,0x00,	/* black */
	0xFF,0x00,0x00	/* red for LEDs */
};

static int LED_display_window_left;
static int LED_display_window_top;
static int LED_display_window_width;
static int LED_display_window_height;

static void hold_load(void);

static void usr9901_interrupt_callback(int intreq, int ic);
static void sys9901_interrupt_callback(int intreq, int ic);

static void usr9901_led_w(int offset, int data);

static int sys9901_r0(int offset);
static void sys9901_digitsel_w(int offset, int data);
static void sys9901_segment_w(int offset, int data);
static void sys9901_dsplytrgr_w(int offset, int data);
static void sys9901_shiftlight_w(int offset, int data);
static void sys9901_spkrdrive_w(int offset, int data);
static void sys9901_tapewdata_w(int offset, int data);


/* user tms9901 setup */
static const tms9901reset_param usr9901reset_param =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INT3 | TMS9901_INT4 | TMS9901_INT5 | TMS9901_INT6,	/* only input pins whose state is always known */

	{	/* read handlers */
		NULL
		/*usr9901_r0,
		usr9901_r1,
		usr9901_r2,
		usr9901_r3*/
	},

	{	/* write handlers */
		usr9901_led_w,
		usr9901_led_w,
		usr9901_led_w,
		usr9901_led_w/*,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w,
		usr9901_w*/
	},

	/* interrupt handler */
	usr9901_interrupt_callback,

	/* clock rate = 2MHz */
	2000000.
};

/* system tms9901 setup */
static const tms9901reset_param sys9901reset_param =
{
	0,	/* only input pins whose state is always known */

	{	/* read handlers */
		sys9901_r0,
		NULL,
		NULL,
		NULL
	},

	{	/* write handlers */
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_digitsel_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_segment_w,
		sys9901_dsplytrgr_w,
		sys9901_shiftlight_w,
		sys9901_spkrdrive_w,
		sys9901_tapewdata_w
	},

	/* interrupt handler */
	sys9901_interrupt_callback,

	/* clock rate = 2MHz */
	2000000.
};

static void rts_callback(int which, int RTS);
static void xmit_callback(int which, int data);

static const tms9902reset_param tms9902_params =
{
	2000000.,				/* clock rate (2MHz) */
	NULL,/*int_callback,*/	/* called when interrupt pin state changes */
	rts_callback,			/* called when Request To Send pin state changes */
	NULL,/*brk_callback,*/	/* called when BReaK state changes */
	xmit_callback			/* called when a character is transmitted */
};

static mess_image *rs232_fp;
static UINT8 rs232_rts;
static mame_timer *rs232_input_timer;

static MACHINE_RESET( tm990_189 )
{
	displayena_timer = timer_alloc(NULL);

	hold_load();

	tms9901_init(0, &usr9901reset_param);
	tms9901_init(1, &sys9901reset_param);
	tms9901_reset(0);
	tms9901_reset(1);

	tms9902_init(0, &tms9902_params);
}

static const TMS9928a_interface tms9918_interface =
{
	TMS99x8,
	0x4000,
	0, 0,
	NULL
};

static MACHINE_START( tm990_189_v )
{
	TMS9928A_configure(&tms9918_interface);
	return 0;
}

static MACHINE_RESET( tm990_189_v )
{
	displayena_timer = timer_alloc(NULL);

	joy1x_timer = timer_alloc(NULL);
	joy1y_timer = timer_alloc(NULL);
	joy2x_timer = timer_alloc(NULL);
	joy2y_timer = timer_alloc(NULL);

	hold_load();

	tms9901_init(0, &usr9901reset_param);
	tms9901_init(1, &sys9901reset_param);
	tms9901_reset(0);
	tms9901_reset(1);

	tms9902_init(0, &tms9902_params);

	TMS9928A_reset();
}


/*
	tm990_189 video emulation.

	Has an integrated 10 digit 8-segment display.
	Supports EIA and TTY terminals, and an optional 9918 controller.
*/

static PALETTE_INIT( tm990_189 )
{
	palette_set_colors(machine, 0, tm990_189_palette, tm990_189_palette_size);

	/*memcpy(colortable, & tm990_189_colortable, sizeof(tm990_189_colortable));*/
}

static VIDEO_EOF( tm990_189 )
{
	int i;

	for (i=0; i<10; i++)
		segment_state[i] = 0;
}

INLINE void draw_digit(void)
{
	segment_state[digitsel] |= ~segment;
}

static void update_common(mame_bitmap *bitmap,
							int display_x, int display_y,
							int LED14_x, int LED14_y,
							int LED56_x, int LED56_y,
							int LED7_x, int LED7_y,
							pen_t fore_color, pen_t back_color)
{
	int i, j;
	static const struct{ int x, y; int w, h; } bounds[8] =
	{
		{ 1, 1, 5, 1 },
		{ 6, 2, 1, 5 },
		{ 6, 8, 1, 5 },
		{ 1,13, 5, 1 },
		{ 0, 8, 1, 5 },
		{ 0, 2, 1, 5 },
		{ 1, 7, 5, 1 },
		{ 8,12, 2, 2 },
	};

	for (i=0; i<10; i++)
	{
		old_segment_state[i] |= segment_state[i];

		for (j=0; j<8; j++)
			plot_box(bitmap, display_x + i*16 + bounds[j].x, display_y + bounds[j].y,
						bounds[j].w, bounds[j].h,
						(old_segment_state[i] & (1 << j)) ? fore_color : back_color);

		old_segment_state[i] = segment_state[i];
	}

	for (i=0; i<4; i++)
	{
		plot_box(bitmap, LED14_x + 48+4 - i*16, LED14_y + 4, 8, 8,
					(LED_state & (1 << i)) ? fore_color : back_color);
	}

	plot_box(bitmap, LED7_x+4, LED7_y+4, 8, 8,
				(LED_state & 0x10) ? fore_color : back_color);

	plot_box(bitmap, LED56_x+16+4, LED56_y+4, 8, 8,
				(LED_state & 0x20) ? fore_color : back_color);

	plot_box(bitmap, LED56_x+4, LED56_y+4, 8, 8,
				(LED_state & 0x40) ? fore_color : back_color);
}

static VIDEO_UPDATE( tm990_189 )
{
	update_common(bitmap, 580, 150, 110, 508, 387, 456, 507, 478, Machine->pens[1], Machine->pens[0]);
	return 0;
}

static VIDEO_UPDATE( tm990_189_v )
{
	video_update_tms9928a(machine, screen, bitmap, cliprect);

	plot_box(bitmap, LED_display_window_left, LED_display_window_top, LED_display_window_width, LED_display_window_height, Machine->pens[1]);
	update_common(bitmap,
					LED_display_window_left, LED_display_window_top,
					LED_display_window_left, LED_display_window_top+16,
					LED_display_window_left+80, LED_display_window_top+16,
					LED_display_window_left+128, LED_display_window_top+16,
					Machine->pens[6], Machine->pens[1]);
	return 0;
}

/*
	Interrupt handlers
*/

static void field_interrupts(void)
{
	if (load_state)
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 2);
	else
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, ic_state);
}

/*
	hold and debounce load line (emulation is inaccurate)
*/

static void clear_load(int dummy)
{
	load_state = FALSE;
	field_interrupts();
}

static void hold_load(void)
{
	load_state = TRUE;
	field_interrupts();
	timer_set(TIME_IN_MSEC(100), 0, clear_load);
}

/*
	tms9901 code
*/

static void usr9901_interrupt_callback(int intreq, int ic)
{
	ic_state = ic & 7;

	if (!load_state)
		field_interrupts();
}

static void usr9901_led_w(int offset, int data)
{
	if (data)
		LED_state |= (1 << offset);
	else
		LED_state &= ~(1 << offset);
}

static void sys9901_interrupt_callback(int intreq, int ic)
{
	(void)ic;

	tms9901_set_single_int(0, 5, intreq);
}

static int sys9901_r0(int offset)
{
	int reply = 0x00;

	/* keyboard read */
	if (digitsel < 9)
		reply |= readinputport(digitsel) << 1;

	/* tape input */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0)
		reply |= 0x40;

	return reply;
}

static void sys9901_digitsel_w(int offset, int data)
{
	if (data)
		digitsel |= 1 << offset;
	else
		digitsel &= ~ (1 << offset);
}

static void sys9901_segment_w(int offset, int data)
{
	if (data)
		segment |= 1 << (offset-4);
	else
	{
		segment &= ~ (1 << (offset-4));
		if ((timer_timeleft(displayena_timer) >= 0.) && (digitsel < 10))
			draw_digit();
	}
}

static void sys9901_dsplytrgr_w(int offset, int data)
{
	if ((!data) && (digitsel < 10))
	{
		timer_reset(displayena_timer, displayena_duration);
		draw_digit();
	}
}

static void sys9901_shiftlight_w(int offset, int data)
{
	if (data)
		LED_state |= 0x10;
	else
		LED_state &= ~0x10;
}

static void sys9901_spkrdrive_w(int offset, int data)
{
	speaker_level_w(0, data);
}

static void sys9901_tapewdata_w(int offset, int data)
{
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), data ? +1.0 : -1.0);
}

/*
	serial interface
*/

static void rs232_input_callback(int dummy)
{
	UINT8 buf;

	if (/*rs232_rts &&*/ /*(mame_ftell(rs232_fp) < mame_fsize(rs232_fp))*/1)
	{
		if (image_fread(rs232_fp, &buf, 1) == 1)
			tms9902_push_data(0, buf);
	}
}

/*
	Initialize rs232 unit and open image
*/
static int device_load_tm990_189_rs232(mess_image *image)
{
	int id = image_index_in_device(image);

	if (id != 0)
		return INIT_FAIL;

	rs232_fp = image;

	tms9902_set_dsr(id, 1);
	rs232_input_timer = timer_alloc(rs232_input_callback);
	timer_adjust(rs232_input_timer, 0., 0, TIME_IN_SEC(.01));

	return INIT_PASS;
}

/*
	close a rs232 image
*/
static void device_unload_tm990_189_rs232(mess_image *image)
{
	int id = image_index_in_device(image);

	if (id != 0)
		return;

	rs232_fp = NULL;

	tms9902_set_dsr(id, 0);

	timer_reset(rs232_input_timer, TIME_NEVER);	/* FIXME - timers should only be allocated once */
}

static void rts_callback(int which, int RTS)
{
	rs232_rts = RTS;
	tms9902_set_cts(which, RTS);
}

static void xmit_callback(int which, int data)
{
	UINT8 buf = data;

	if (rs232_fp)
		image_fwrite(rs232_fp, &buf, 1);
}

/*
	External instruction decoding
*/

static WRITE8_HANDLER(ext_instr_decode)
{
	int ext_op_ID;

	ext_op_ID = ((offset+0x800) >> 11) & 0x3;
	if (data)
		ext_op_ID |= 4;
	switch (ext_op_ID)
	{
	case 2: /* IDLE: handle elsewhere */
		break;

	case 3: /* RSET: not used in default board */
		break;

	case 5: /* CKON: set DECKCONTROL */
		LED_state |= 0x20;
		{
			mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);
			cassette_change_state(img, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		}
		break;

	case 6: /* CKOF: clear DECKCONTROL */
		LED_state &= ~0x20;
		{
			mess_image *img = image_from_devtype_and_index(IO_CASSETTE, 0);
			cassette_change_state(img, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		}
		break;

	case 7: /* LREX: trigger LOAD */
		hold_load();
		break;
	}
}

static void idle_callback(int state)
{
	if (state)
		LED_state |= 0x40;
	else
		LED_state &= ~0x40;
}

/*
	Video Board handling
*/

static  READ8_HANDLER(video_vdp_r)
{
	int reply = 0;
	static UINT8 bogus_read_save;

	/* When the tms9980 reads @>2000 or @>2001, it actually does a word access:
	it reads @>2000 first, then @>2001.  According to schematics, both access
	are decoded to the VDP: read accesses are therefore bogus, all the more so
	since the two reads are too close (1us) for the VDP to be able to reload
	the read buffer: the read address pointer is probably incremented by 2, but
	only the first byte is valid.  There is a work around for this problem: all
	you need is reloading the address pointer before each read.  However,
	software always uses the second byte, which is very weird, particularly
	for the status port.  Presumably, since the read buffer has not been
	reloaded, the second byte read from the memory read port is equal to the
	first; however, this explanation is not very convincing for the status
	port.  Still, I do not have any better explanation, so I will stick with
	it. */

	if (offset & 2)
		reply = TMS9928A_register_r(0);
	else
		reply = TMS9928A_vram_r(0);

	if (!(offset & 1))
		bogus_read_save = reply;
	else
		reply = bogus_read_save;

	return reply;
}

static WRITE8_HANDLER(video_vdp_w)
{
	if (offset & 1)
	{
		if (offset & 2)
			TMS9928A_register_w(0, data);
		else
			TMS9928A_vram_w(0, data);
	}
}

static  READ8_HANDLER(video_joy_r)
{
	int reply = readinputport(9);


	if (timer_timeleft(joy1x_timer) < 0.)
		reply |= 0x01;

	if (timer_timeleft(joy1y_timer) < 0.)
		reply |= 0x02;

	if (timer_timeleft(joy2x_timer) < 0.)
		reply |= 0x08;

	if (timer_timeleft(joy2y_timer) < 0.)
		reply |= 0x10;

	return reply;
}

static WRITE8_HANDLER(video_joy_w)
{
	timer_reset(joy1x_timer, TIME_IN_USEC(readinputport(10)*28+28));
	timer_reset(joy1y_timer, TIME_IN_USEC(readinputport(11)*28+28));
	timer_reset(joy2x_timer, TIME_IN_USEC(readinputport(12)*28+28));
	timer_reset(joy2y_timer, TIME_IN_USEC(readinputport(13)*28+28));
}


/*
	Memory map:

	0x0000-0x03ff: 1kb RAM
	0x0400-0x07ff: 1kb onboard expansion RAM
	0x0800-0x0bff or 0x0800-0x0fff: 1kb or 2kb onboard expansion ROM
	0x1000-0x2fff: reserved for offboard expansion
		Only known card: Color Video Board
			0x1000-0x17ff (R): Video board ROM 1
			0x1800-0x1fff (R): Video board ROM 2
			0x2000-0x27ff (R): tms9918 read ports (bogus)
				(address & 2) == 0: data port (bogus)
				(address & 2) == 1: status register (bogus)
			0x2800-0x2fff (R): joystick read port
				d2: joy 2 switch
				d3: joy 2 Y (length of pulse after retrigger is proportional to axis position)
				d4: joy 2 X (length of pulse after retrigger is proportional to axis position)
				d2: joy 1 switch
				d3: joy 1 Y (length of pulse after retrigger is proportional to axis position)
				d4: joy 1 X (length of pulse after retrigger is proportional to axis position)
			0x1801-0x1fff ((address & 1) == 1) (W): joystick write port (retrigger)
			0x2801-0x2fff ((address & 1) == 1) (W): tms9918 write ports
				(address & 2) == 0: data port
				(address & 2) == 1: control register
	0x3000-0x3fff: 4kb onboard ROM
*/

static ADDRESS_MAP_START(tm990_189_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x07ff) AM_RAM									/* RAM */
	AM_RANGE(0x0800, 0x0fff) AM_ROM									/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */
	AM_RANGE(0x1000, 0x2fff) AM_NOP									/* reserved for expansion (RAM and/or tms9918 video controller) */
	AM_RANGE(0x3000, 0x3fff) AM_ROM									/* main ROM - unibug or university basic */

ADDRESS_MAP_END

static ADDRESS_MAP_START(tm990_189_v_memmap, ADDRESS_SPACE_PROGRAM, 8)

	AM_RANGE(0x0000, 0x07ff) AM_RAM									/* RAM */
	AM_RANGE(0x0800, 0x0fff) AM_ROM									/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */

	AM_RANGE(0x1000, 0x17ff) AM_READWRITE(MRA8_ROM, MWA8_NOP)		/* video board ROM 1 */
	AM_RANGE(0x1800, 0x1fff) AM_READWRITE(MRA8_ROM, video_joy_w)	/* video board ROM 2 and joystick write port*/
	AM_RANGE(0x2000, 0x27ff) AM_READWRITE(video_vdp_r, MWA8_NOP)	/* video board tms9918 read ports (bogus) */
	AM_RANGE(0x2800, 0x2fff) AM_READWRITE(video_joy_r, video_vdp_w)	/* video board joystick read port and tms9918 write ports */

	AM_RANGE(0x3000, 0x3fff) AM_ROM									/* main ROM - unibug or university basic */

ADDRESS_MAP_END

/*
	CRU map

	The board features one tms9901 for keyboard and sound I/O, another tms9901
	to drive the parallel port and a few LEDs and handle tms9980 interrupts,
	and one optional tms9902 for serial I/O.

	* bits >000->1ff (R12=>000->3fe): first TMS9901: parallel port, a few LEDs,
		interrupts
		- CRU bits 1-5: UINT1* through UINT5* (jumper connectable to parallel
			port or COMINT from tms9902)
		- CRU bit 6: KBINT (interrupt request from second tms9901)
		- CRU bits 7-15: mirrors P15-P7
		- CRU bits 16-31: P0-P15 (parallel port)
			(bits 16, 17, 18 and 19 control LEDs numbered 1, 2, 3 and 4, too)
	* bits >200->3ff (R12=>400->7fe): second TMS9901: display, keyboard and
		sound
		- CRU bits 1-5: KB1*-KB5* (inputs from current keyboard row)
		- CRU bit 6: RDATA (tape in)
		- CRU bits 7-15: mirrors P15-P7
		- CRU bits 16-19: DIGITSELA-DIGITSELD (select current display digit and
			keyboard row)
		- CRU bits 20-27: SEGMENTA*-SEGMENTG* and SEGMENTP* (drives digit
			segments)
		- CRU bit 28: DSPLYTRGR* (will generate a pulse to drive the digit
			segments)
		- bit 29: SHIFTLIGHT (controls shift light)
		- bit 30: SPKRDRIVE (controls buzzer)
		- bit 31: WDATA (tape out)
	* bits >400->5ff (R12=>800->bfe): optional TMS9902
	* bits >600->7ff (R12=>c00->ffe): reserved for expansion
	* write 0 to bits >1000->17ff: IDLE: flash IDLE LED
	* write 0 to bits >1800->1fff: RSET: not connected, but it is decoded and
		hackers can connect any device they like to this pin
	* write 1 to bits >0800->0fff: CKON: light FWD LED and activates
		DECKCONTROL* signal (part of tape interface)
	* write 1 to bits >1000->17ff: CKOF: light off FWD LED and deactivates
		DECKCONTROL* signal (part of tape interface)
	* write 1 to bits >1800->1fff: LREX: trigger load interrupt

	Keyboard map: see input ports

	Display segment designation:
		   a
		  ___
		 |   |
		f|   |b
		 |___|
		 | g |
		e|   |c
		 |___|  .p
		   d
*/

static ADDRESS_MAP_START(tm990_189_writecru, ADDRESS_SPACE_IO, 8)


	AM_RANGE(0x000, 0x1ff) AM_WRITE(tms9901_0_cru_w)	/* user I/O tms9901 */
	AM_RANGE(0x200, 0x3ff) AM_WRITE(tms9901_1_cru_w)	/* system I/O tms9901 */
	AM_RANGE(0x400, 0x5ff) AM_WRITE(tms9902_0_cru_w)	/* optional tms9902 */

	AM_RANGE(0x0800,0x1fff)AM_WRITE(ext_instr_decode)	/* external instruction decoding (IDLE, RSET, CKON, CKOF, LREX) */

ADDRESS_MAP_END

static ADDRESS_MAP_START(tm990_189_readcru, ADDRESS_SPACE_IO, 8)

	AM_RANGE(0x00, 0x3f) AM_READ(tms9901_0_cru_r)		/* user I/O tms9901 */
	AM_RANGE(0x40, 0x6f) AM_READ(tms9901_1_cru_r)		/* system I/O tms9901 */
	AM_RANGE(0x80, 0xcf) AM_READ(tms9902_0_cru_r)		/* optional tms9902 */

ADDRESS_MAP_END



static tms9980areset_param reset_params =
{
	idle_callback
};

static MACHINE_DRIVER_START(tm990_189)

	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MDRV_CPU_ADD(TMS9980, 2000000)
	MDRV_CPU_CONFIG(reset_params)
	MDRV_CPU_PROGRAM_MAP(tm990_189_memmap, 0)
	MDRV_CPU_IO_MAP(tm990_189_readcru, tm990_189_writecru)
	/*MDRV_CPU_VBLANK_INT(tm990_189_interrupt, 1)*/
	/*MDRV_CPU_PERIODIC_INT(NULL, 0)*/

	MDRV_MACHINE_RESET( tm990_189 )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware - we emulate a 8-segment LED display */
	MDRV_SCREEN_REFRESH_RATE(75)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(/*DEFAULT_REAL_60HZ_VBLANK_DURATION*/0))
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(750, 532)
	MDRV_SCREEN_VISIBLE_AREA(0, 750-1, 0, 532-1)

	/*MDRV_GFXDECODE(tm990_189_gfxdecodeinfo)*/
	MDRV_PALETTE_LENGTH(tm990_189_palette_size)
	MDRV_COLORTABLE_LENGTH(/*tm990_189_colortable_size*/0)

	MDRV_PALETTE_INIT(tm990_189)
	/*MDRV_VIDEO_START(tm990_189)*/
	MDRV_VIDEO_EOF(tm990_189)
	MDRV_VIDEO_UPDATE(tm990_189)

	/* sound hardware */
	/* one two-level buzzer */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)MACHINE_DRIVER_END

static MACHINE_DRIVER_START(tm990_189_v)

	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MDRV_CPU_ADD(TMS9980, 2000000)
	MDRV_CPU_CONFIG(reset_params)
	MDRV_CPU_PROGRAM_MAP(tm990_189_v_memmap, 0)
	MDRV_CPU_IO_MAP(tm990_189_readcru, tm990_189_writecru)
	/*MDRV_CPU_VBLANK_INT(tm990_189_interrupt, 1)*/
	/*MDRV_CPU_PERIODIC_INT(NULL, 0)*/

	MDRV_MACHINE_START( tm990_189_v )
	MDRV_MACHINE_RESET( tm990_189_v )
	/*MDRV_NVRAM_HANDLER( NULL )*/

	/* video hardware */
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_IMPORT_FROM(tms9928a)
	MDRV_VIDEO_EOF(tm990_189)
	MDRV_VIDEO_UPDATE(tm990_189_v)

	/* NPW 27-Feb-2006 - ewwww gross!!! maybe this can be fixed when 
	 * multimonitor support is added?*/
	LED_display_window_left = machine->screen[0].defstate.visarea.min_x;
	LED_display_window_top = machine->screen[0].defstate.visarea.max_y;
	LED_display_window_width = machine->screen[0].defstate.visarea.max_x - machine->screen[0].defstate.visarea.min_x;
	LED_display_window_height = 32;
	machine->screen[0].defstate.visarea.max_y += 32;
	machine->screen[0].defstate.height += 32;

	/* sound hardware */
	/* one two-level buzzer */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(SPEAKER,	0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(990189)
	/*CPU memory space*/
	ROM_REGION(0x4000, REGION_CPU1,0)

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb) SHA1(fa9751fd2e3e5d7ae03819fc9c7099e2ddd9fb53))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b) SHA1(96e88f4cb7a374033cdf3af0dc26ca5b1d55b9f9))
	/*ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744))*/	/* older, partial dump of university BASIC */

ROM_END

ROM_START(990189v)
	/*CPU memory space*/
	ROM_REGION(0x4000, REGION_CPU1,0)

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb) SHA1(fa9751fd2e3e5d7ae03819fc9c7099e2ddd9fb53))

	/* extension ROM */
	ROM_LOAD_OPTIONAL("demo1000.u13", 0x1000, 0x0800, CRC(c0e16685) SHA1(d0d314134c42fa4682aafbace67f539f67f6ba65))

	/* extension ROM */
	ROM_LOAD_OPTIONAL("demo1800.u11", 0x1800, 0x0800, CRC(8737dc4b) SHA1(b87da7aa4d3f909e70f885c4b36999cc1abf5764))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b) SHA1(96e88f4cb7a374033cdf3af0dc26ca5b1d55b9f9))
	/*ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744))*/	/* older, partial dump of university BASIC */

ROM_END

#define JOYSTICK_DELTA			10
#define JOYSTICK_SENSITIVITY	100

INPUT_PORTS_START(tm990_189)

	/* 45-key calculator-like alphanumeric keyboard... */

	PORT_START    /* row 0 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Sp *") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ret '") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("$ =") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)

	PORT_START    /* row 1 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ (") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- )") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ /") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("> %") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 ^") PORT_CODE(KEYCODE_0)
		
	PORT_START    /* row 2 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 .") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 ;") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 :") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 ?") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 !") PORT_CODE(KEYCODE_5)
		
	PORT_START    /* row 3 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 _") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \"") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 #") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (ESC)") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A (SOH)") PORT_CODE(KEYCODE_A)

	PORT_START    /* row 4 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B (STH)") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C (ETX)") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D (EOT)") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E (ENQ)") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F (ACK)") PORT_CODE(KEYCODE_F)

	PORT_START    /* row 5 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G (BEL)") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H (BS)") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I (HT)") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J (LF)") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K (VT)") PORT_CODE(KEYCODE_K)

	PORT_START    /* row 6 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L (FF)") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M (DEL)") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N (SO)") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O (SI)") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P (DLE)") PORT_CODE(KEYCODE_P)

	PORT_START    /* row 7 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q (DC1)") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R (DC2)") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S (DC3)") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T (DC4)") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U (NAK)") PORT_CODE(KEYCODE_U)

	PORT_START    /* row 8 */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V <-D") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W (ETB)") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X (CAN)") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y (EM)") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z ->D") PORT_CODE(KEYCODE_Z)

	/* analog joysticks (video board only) */

	PORT_START	/* joystick 1 & 2 buttons */
		PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
		PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)

	PORT_START	/* joystick 1, X axis */
		PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1)

	PORT_START	/* joystick 1, Y axis */
		PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1) PORT_REVERSE

	PORT_START	/* joystick 2, X axis */
		PORT_BIT( 0x3ff, 0x180,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x180 ) PORT_PLAYER(2)

	PORT_START	/* joystick 2, Y axis */
		PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(2) PORT_REVERSE

INPUT_PORTS_END

static void tm990_189_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void tm990_189_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_tm990_189_rs232; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_tm990_189_rs232; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), ""); break;
	}
}

SYSTEM_CONFIG_START(tm990_189)
	/* a tape interface and a rs232 interface... */
	CONFIG_DEVICE(tm990_189_cassette_getinfo)
	CONFIG_DEVICE(tm990_189_serial_getinfo)
SYSTEM_CONFIG_END

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY					FULLNAME */
COMP( 1978,	990189,	  0,		0,		tm990_189,	tm990_189,	0,		tm990_189,	"Texas Instruments",	"TM 990/189 University Board microcomputer with University Basic" , 0)
COMP( 1980,	990189v,  990189,	0,		tm990_189_v,tm990_189,	0,		tm990_189,	"Texas Instruments",	"TM 990/189 University Board microcomputer with University Basic and Video Board Interface" , 0)
