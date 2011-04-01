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

#include "emu.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/tms9901.h"
#include "machine/tms9902.h"
#include "video/tms9928a.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/serial.h"


class tm990189_state : public driver_device
{
public:
	tm990189_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int m_load_state;
	int m_ic_state;
	int m_digitsel;
	int m_segment;
	emu_timer *m_displayena_timer;
	UINT8 m_segment_state[10];
	UINT8 m_old_segment_state[10];
	UINT8 m_LED_state;
	emu_timer *m_joy1x_timer;
	emu_timer *m_joy1y_timer;
	emu_timer *m_joy2x_timer;
	emu_timer *m_joy2y_timer;
	int m_LED_display_window_left;
	int m_LED_display_window_top;
	int m_LED_display_window_width;
	int m_LED_display_window_height;
	device_image_interface *m_rs232_fp;
	UINT8 m_rs232_rts;
	emu_timer *m_rs232_input_timer;
	UINT8 m_bogus_read_save;
};




#define displayena_duration attotime::from_usec(4500)	/* Can anyone confirm this? 74LS123 connected to C=0.1uF and R=100kOhm */


enum
{
	tm990_189_palette_size = 3
};
static const unsigned char tm990_189_palette[tm990_189_palette_size*3] =
{
	0x00,0x00,0x00,	/* black */
	0xFF,0x00,0x00	/* red for LEDs */
};


static void hold_load(running_machine &machine);



static MACHINE_RESET( tm990_189 )
{
	hold_load(machine);
}

static MACHINE_START( tm990_189 )
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	state->m_displayena_timer = machine.scheduler().timer_alloc(FUNC(NULL));
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
	tm990189_state *state = machine.driver_data<tm990189_state>();
	TMS9928A_configure(&tms9918_interface);

	state->m_displayena_timer = machine.scheduler().timer_alloc(FUNC(NULL));

	state->m_joy1x_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_joy1y_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_joy2x_timer = machine.scheduler().timer_alloc(FUNC(NULL));
	state->m_joy2y_timer = machine.scheduler().timer_alloc(FUNC(NULL));
}

static MACHINE_RESET( tm990_189_v )
{
	hold_load(machine);

	TMS9928A_reset();
}


/*
    tm990_189 video emulation.

    Has an integrated 10 digit 8-segment display.
    Supports EIA and TTY terminals, and an optional 9918 controller.
*/

static PALETTE_INIT( tm990_189 )
{
	int i;

	for ( i = 0; i < tm990_189_palette_size; i++ ) {
		palette_set_color_rgb(machine, i, tm990_189_palette[i*3], tm990_189_palette[i*3+1], tm990_189_palette[i*3+2]);
	}
}

static SCREEN_EOF( tm990_189 )
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	int i;

	for (i=0; i<10; i++)
		state->m_segment_state[i] = 0;
}

INLINE void draw_digit(tm990189_state *state)
{
	state->m_segment_state[state->m_digitsel] |= ~state->m_segment;
}

static void update_common(running_machine &machine, bitmap_t *bitmap,
							int display_x, int display_y,
							int LED14_x, int LED14_y,
							int LED56_x, int LED56_y,
							int LED7_x, int LED7_y,
							pen_t fore_color, pen_t back_color)
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
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
		state->m_old_segment_state[i] |= state->m_segment_state[i];

		for (j=0; j<8; j++)
			plot_box(bitmap, display_x + i*16 + bounds[j].x, display_y + bounds[j].y,
						bounds[j].w, bounds[j].h,
						(state->m_old_segment_state[i] & (1 << j)) ? fore_color : back_color);

		state->m_old_segment_state[i] = state->m_segment_state[i];
	}

	for (i=0; i<4; i++)
	{
		plot_box(bitmap, LED14_x + 48+4 - i*16, LED14_y + 4, 8, 8,
					(state->m_LED_state & (1 << i)) ? fore_color : back_color);
	}

	plot_box(bitmap, LED7_x+4, LED7_y+4, 8, 8,
				(state->m_LED_state & 0x10) ? fore_color : back_color);

	plot_box(bitmap, LED56_x+16+4, LED56_y+4, 8, 8,
				(state->m_LED_state & 0x20) ? fore_color : back_color);

	plot_box(bitmap, LED56_x+4, LED56_y+4, 8, 8,
				(state->m_LED_state & 0x40) ? fore_color : back_color);
}

static SCREEN_UPDATE( tm990_189 )
{
	update_common(screen->machine(), bitmap, 580, 150, 110, 508, 387, 456, 507, 478, 1, 0);
	return 0;
}

static VIDEO_START( tm990_189_v )
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	screen_device *screen = machine.first_screen();
	const rectangle &visarea = screen->visible_area();

	/* NPW 27-Feb-2006 - ewwww gross!!! maybe this can be fixed when
     * multimonitor support is added?*/
	state->m_LED_display_window_left = visarea.min_x;
	state->m_LED_display_window_top = visarea.max_y - 32;
	state->m_LED_display_window_width = visarea.max_x - visarea.min_x;
	state->m_LED_display_window_height = 32;
}

static SCREEN_UPDATE( tm990_189_v )
{
	tm990189_state *state = screen->machine().driver_data<tm990189_state>();
	SCREEN_UPDATE_CALL(tms9928a);

	plot_box(bitmap, state->m_LED_display_window_left, state->m_LED_display_window_top, state->m_LED_display_window_width, state->m_LED_display_window_height, screen->machine().pens[1]);
	update_common(screen->machine(), bitmap,
					state->m_LED_display_window_left, state->m_LED_display_window_top,
					state->m_LED_display_window_left, state->m_LED_display_window_top+16,
					state->m_LED_display_window_left+80, state->m_LED_display_window_top+16,
					state->m_LED_display_window_left+128, state->m_LED_display_window_top+16,
					6, 1);
	return 0;
}

/*
    Interrupt handlers
*/

static void field_interrupts(running_machine &machine)
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	if (state->m_load_state)
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 2);
	else
		cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, state->m_ic_state);
}

/*
    hold and debounce load line (emulation is inaccurate)
*/

static TIMER_CALLBACK(clear_load)
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	state->m_load_state = FALSE;
	field_interrupts(machine);
}

static void hold_load(running_machine &machine)
{
	tm990189_state *state = machine.driver_data<tm990189_state>();
	state->m_load_state = TRUE;
	field_interrupts(machine);
	machine.scheduler().timer_set(attotime::from_msec(100), FUNC(clear_load));
}

/*
    tms9901 code
*/

static TMS9901_INT_CALLBACK ( usr9901_interrupt_callback )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	state->m_ic_state = ic & 7;

	if (!state->m_load_state)
		field_interrupts(device->machine());
}

static WRITE8_DEVICE_HANDLER( usr9901_led_w )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	if (data)
		state->m_LED_state |= (1 << offset);
	else
		state->m_LED_state &= ~(1 << offset);
}

static TMS9901_INT_CALLBACK( sys9901_interrupt_callback )
{
	(void)ic;

	tms9901_set_single_int(device->machine().device("tms9901_0"), 5, intreq);
}

static READ8_DEVICE_HANDLER( sys9901_r0 )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	int reply = 0x00;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7", "LINE8" };

	/* keyboard read */
	if (state->m_digitsel < 9)
		reply |= input_port_read(device->machine(), keynames[state->m_digitsel]) << 1;

	/* tape input */
	if (cassette_input(device->machine().device("cassette")) > 0.0)
		reply |= 0x40;

	return reply;
}

static WRITE8_DEVICE_HANDLER( sys9901_digitsel_w )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	if (data)
		state->m_digitsel |= 1 << offset;
	else
		state->m_digitsel &= ~ (1 << offset);
}

static WRITE8_DEVICE_HANDLER( sys9901_segment_w )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	if (data)
		state->m_segment |= 1 << (offset-4);
	else
	{
		state->m_segment &= ~ (1 << (offset-4));
		if ((state->m_displayena_timer->remaining() > attotime::zero)  && (state->m_digitsel < 10))
			draw_digit(state);
	}
}

static WRITE8_DEVICE_HANDLER( sys9901_dsplytrgr_w )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	if ((!data) && (state->m_digitsel < 10))
	{
		state->m_displayena_timer->reset(displayena_duration);
		draw_digit(state);
	}
}

static WRITE8_DEVICE_HANDLER( sys9901_shiftlight_w )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	if (data)
		state->m_LED_state |= 0x10;
	else
		state->m_LED_state &= ~0x10;
}

static WRITE8_DEVICE_HANDLER( sys9901_spkrdrive_w )
{
	device_t *speaker = device->machine().device("speaker");
	speaker_level_w(speaker, data);
}

static WRITE8_DEVICE_HANDLER( sys9901_tapewdata_w )
{
	cassette_output(device->machine().device("cassette"), data ? +1.0 : -1.0);
}

/*
    serial interface
*/

static TIMER_CALLBACK(rs232_input_callback)
{
	//tm990189_state *state = machine.driver_data<tm990189_state>();
	UINT8 buf;
	device_image_interface *image = (device_image_interface *)(ptr);
	if (/*state->m_rs232_rts &&*/ /*(mame_ftell(state->m_rs232_fp) < mame_fsize(state->m_rs232_fp))*/1)
	{
		if (image->fread(&buf, 1) == 1)
			tms9902_push_data(machine.device("tms9902"), buf);
	}
}

/*
    Initialize rs232 unit and open image
*/
static DEVICE_IMAGE_LOAD( tm990_189_rs232 )
{
	tm990189_state *state = image.device().machine().driver_data<tm990189_state>();
	tms9902_set_dsr(image.device().machine().device("tms9902"), 1);
	state->m_rs232_input_timer = image.device().machine().scheduler().timer_alloc(FUNC(rs232_input_callback), (void*)image);
	state->m_rs232_input_timer->adjust(attotime::zero, 0, attotime::from_msec(10));

	return IMAGE_INIT_PASS;
}

/*
    close a rs232 image
*/
static DEVICE_IMAGE_UNLOAD( tm990_189_rs232 )
{
	tm990189_state *state = image.device().machine().driver_data<tm990189_state>();
	tms9902_set_dsr(image.device().machine().device("tms9902"), 0);

	state->m_rs232_input_timer->reset();	/* FIXME - timers should only be allocated once */
}

DEVICE_GET_INFO( tm990_189_rs232 )
{
	switch ( state )
	{
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( tm990_189_rs232 );    break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( tm990_189_rs232 );    break;
		case DEVINFO_STR_NAME:		                strcpy(info->s, "TM990/189 RS232 port");	                    break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                         break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                        	break;
		default:									DEVICE_GET_INFO_CALL(serial);	break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(TM990_189_RS232, tm990_189_rs232);
DEFINE_LEGACY_IMAGE_DEVICE(TM990_189_RS232, tm990_189_rs232);

#define MCFG_TM990_189_RS232_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, TM990_189_RS232, 0)


static TMS9902_RST_CALLBACK( rts_callback )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	state->m_rs232_rts = RTS;
	tms9902_set_cts(device, RTS);
}

static TMS9902_XMIT_CALLBACK( xmit_callback )
{
	tm990189_state *state = device->machine().driver_data<tm990189_state>();
	UINT8 buf = data;

	if (state->m_rs232_fp)
		state->m_rs232_fp->fwrite(&buf, 1);
}

/*
    External instruction decoding
*/

static WRITE8_HANDLER(ext_instr_decode)
{
	tm990189_state *state = space->machine().driver_data<tm990189_state>();
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
		state->m_LED_state |= 0x20;
		{
			device_t *img = space->machine().device("cassette");
			cassette_change_state(img, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		}
		break;

	case 6: /* CKOF: clear DECKCONTROL */
		state->m_LED_state &= ~0x20;
		{
			device_t *img = space->machine().device("cassette");
			cassette_change_state(img, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		}
		break;

	case 7: /* LREX: trigger LOAD */
		hold_load(space->machine());
		break;
	}
}

static void idle_callback(device_t *device, int state)
{
	tm990189_state *drvstate = device->machine().driver_data<tm990189_state>();
	if (state)
		drvstate->m_LED_state |= 0x40;
	else
		drvstate->m_LED_state &= ~0x40;
}

/*
    Video Board handling
*/

static  READ8_HANDLER(video_vdp_r)
{
	tm990189_state *state = space->machine().driver_data<tm990189_state>();
	int reply = 0;

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
		reply = TMS9928A_register_r(space, 0);
	else
		reply = TMS9928A_vram_r(space, 0);

	if (!(offset & 1))
		state->m_bogus_read_save = reply;
	else
		reply = state->m_bogus_read_save;

	return reply;
}

static WRITE8_HANDLER(video_vdp_w)
{
	if (offset & 1)
	{
		if (offset & 2)
			TMS9928A_register_w(space, 0, data);
		else
			TMS9928A_vram_w(space, 0, data);
	}
}

static READ8_HANDLER(video_joy_r)
{
	tm990189_state *state = space->machine().driver_data<tm990189_state>();
	int reply = input_port_read(space->machine(), "BUTTONS");


	if (state->m_joy1x_timer->remaining() < attotime::zero)
		reply |= 0x01;

	if (state->m_joy1y_timer->remaining() < attotime::zero)
		reply |= 0x02;

	if (state->m_joy2x_timer->remaining() < attotime::zero)
		reply |= 0x08;

	if (state->m_joy2y_timer->remaining() < attotime::zero)
		reply |= 0x10;

	return reply;
}

static WRITE8_HANDLER(video_joy_w)
{
	tm990189_state *state = space->machine().driver_data<tm990189_state>();
	state->m_joy1x_timer->reset(attotime::from_usec(input_port_read(space->machine(), "JOY1_X")*28+28));
	state->m_joy1y_timer->reset(attotime::from_usec(input_port_read(space->machine(), "JOY1_Y")*28+28));
	state->m_joy2x_timer->reset(attotime::from_usec(input_port_read(space->machine(), "JOY2_X")*28+28));
	state->m_joy2y_timer->reset(attotime::from_usec(input_port_read(space->machine(), "JOY2_Y")*28+28));
}

/* user tms9901 setup */
static const tms9901_interface usr9901reset_param =
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
static const tms9901_interface sys9901reset_param =
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

static const tms9902_interface tms9902_params =
{
	2000000.,				/* clock rate (2MHz) */
	NULL,/*int_callback,*/	/* called when interrupt pin state changes */
	rts_callback,			/* called when Request To Send pin state changes */
	NULL,/*brk_callback,*/	/* called when BReaK state changes */
	xmit_callback			/* called when a character is transmitted */
};

static ADDRESS_MAP_START(tm990_189_memmap, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_RAM									/* RAM */
	AM_RANGE(0x0800, 0x0fff) AM_ROM									/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */
	AM_RANGE(0x1000, 0x2fff) AM_NOP									/* reserved for expansion (RAM and/or tms9918 video controller) */
	AM_RANGE(0x3000, 0x3fff) AM_ROM									/* main ROM - unibug or university basic */
ADDRESS_MAP_END

static ADDRESS_MAP_START(tm990_189_v_memmap, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_RAM									/* RAM */
	AM_RANGE(0x0800, 0x0fff) AM_ROM									/* extra ROM - application programs with unibug, remaining 2kb of program for university basic */

	AM_RANGE(0x1000, 0x17ff) AM_ROM AM_WRITENOP		/* video board ROM 1 */
	AM_RANGE(0x1800, 0x1fff) AM_ROM AM_WRITE(video_joy_w)	/* video board ROM 2 and joystick write port*/
	AM_RANGE(0x2000, 0x27ff) AM_READ(video_vdp_r) AM_WRITENOP	/* video board tms9918 read ports (bogus) */
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

static ADDRESS_MAP_START(tm990_189_cru_map, AS_IO, 8)
	AM_RANGE(0x00, 0x3f) AM_DEVREAD("tms9901_0", tms9901_cru_r)		/* user I/O tms9901 */
	AM_RANGE(0x40, 0x6f) AM_DEVREAD("tms9901_1", tms9901_cru_r)		/* system I/O tms9901 */
	AM_RANGE(0x80, 0xcf) AM_DEVREAD("tms9902", tms9902_cru_r)		/* optional tms9902 */

	AM_RANGE(0x000, 0x1ff) AM_DEVWRITE("tms9901_0", tms9901_cru_w)	/* user I/O tms9901 */
	AM_RANGE(0x200, 0x3ff) AM_DEVWRITE("tms9901_1", tms9901_cru_w)	/* system I/O tms9901 */
	AM_RANGE(0x400, 0x5ff) AM_DEVWRITE("tms9902", tms9902_cru_w)	/* optional tms9902 */

	AM_RANGE(0x0800,0x1fff)AM_WRITE(ext_instr_decode)	/* external instruction decoding (IDLE, RSET, CKON, CKOF, LREX) */
ADDRESS_MAP_END


static const tms9980areset_param reset_params =
{
	idle_callback
};

static MACHINE_CONFIG_START( tm990_189, tm990189_state )
	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MCFG_CPU_ADD("maincpu", TMS9980, 2000000)
	MCFG_CPU_CONFIG(reset_params)
	MCFG_CPU_PROGRAM_MAP(tm990_189_memmap)
	MCFG_CPU_IO_MAP(tm990_189_cru_map)

	MCFG_MACHINE_START( tm990_189 )
	MCFG_MACHINE_RESET( tm990_189 )

	/* video hardware - we emulate a 8-segment LED display */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(75)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(750, 532)
	MCFG_SCREEN_VISIBLE_AREA(0, 750-1, 0, 532-1)
	MCFG_SCREEN_UPDATE(tm990_189)
	MCFG_SCREEN_EOF(tm990_189)

	/*MCFG_GFXDECODE(tm990_189)*/
	MCFG_PALETTE_LENGTH(tm990_189_palette_size)

	MCFG_PALETTE_INIT(tm990_189)
	/*MCFG_VIDEO_START(tm990_189)*/

	/* sound hardware */
	/* one two-level buzzer */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )

	/* tms9901 */
	MCFG_TMS9901_ADD("tms9901_0", usr9901reset_param)
	MCFG_TMS9901_ADD("tms9901_1", sys9901reset_param)
	/* tms9902 */
	MCFG_TMS9902_ADD("tms9902", tms9902_params)

	MCFG_TM990_189_RS232_ADD("rs232")
MACHINE_CONFIG_END

#define LEFT_BORDER		15
#define RIGHT_BORDER		15
#define TOP_BORDER_60HZ		27
#define BOTTOM_BORDER_60HZ	24

static MACHINE_CONFIG_START( tm990_189_v, tm990189_state )
	/* basic machine hardware */
	/* TMS9980 CPU @ 2.0 MHz */
	MCFG_CPU_ADD("maincpu", TMS9980, 2000000)
	MCFG_CPU_CONFIG(reset_params)
	MCFG_CPU_PROGRAM_MAP(tm990_189_v_memmap)
	MCFG_CPU_IO_MAP(tm990_189_cru_map)

	MCFG_MACHINE_START( tm990_189_v )
	MCFG_MACHINE_RESET( tm990_189_v )

	/* video hardware */
	MCFG_FRAGMENT_ADD(tms9928a)
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(LEFT_BORDER+32*8+RIGHT_BORDER, TOP_BORDER_60HZ+24*8+BOTTOM_BORDER_60HZ + 32)
	MCFG_SCREEN_VISIBLE_AREA(LEFT_BORDER-12, LEFT_BORDER+32*8+12-1, TOP_BORDER_60HZ-9, TOP_BORDER_60HZ+24*8+9-1 + 32)
	MCFG_SCREEN_UPDATE(tm990_189_v)
	MCFG_SCREEN_EOF(tm990_189)

	MCFG_VIDEO_START(tm990_189_v)

	/* sound hardware */
	/* one two-level buzzer */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )

	/* tms9901 */
	MCFG_TMS9901_ADD("tms9901_0", usr9901reset_param)
	MCFG_TMS9901_ADD("tms9901_1", sys9901reset_param)
	/* tms9902 */
	MCFG_TMS9902_ADD("tms9902", tms9902_params)

	MCFG_TM990_189_RS232_ADD("rs232")
MACHINE_CONFIG_END


/*
  ROM loading
*/
ROM_START(990189)
	/*CPU memory space*/
	ROM_REGION(0x4000, "maincpu",0)

	/* extra ROM */
	ROM_LOAD("990-469.u32", 0x0800, 0x0800, CRC(08df7edb) SHA1(fa9751fd2e3e5d7ae03819fc9c7099e2ddd9fb53))

	/* boot ROM */
	ROM_LOAD("990-469.u33", 0x3000, 0x1000, CRC(e9b4ac1b) SHA1(96e88f4cb7a374033cdf3af0dc26ca5b1d55b9f9))
	/*ROM_LOAD("unibasic.bin", 0x3000, 0x1000, CRC(de4d9744))*/	/* older, partial dump of university BASIC */

ROM_END

ROM_START(990189v)
	/*CPU memory space*/
	ROM_REGION(0x4000, "maincpu",0)

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

static INPUT_PORTS_START(tm990_189)

	/* 45-key calculator-like alphanumeric keyboard... */
	PORT_START("LINE0")    /* row 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Sp *") PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')	PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ret '") PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)	PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("$ =") PORT_CODE(KEYCODE_STOP)	PORT_CHAR('$')	PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',')	PORT_CHAR('<')

	PORT_START("LINE1")    /* row 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ (") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('+')	PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- )") PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('-')	PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ /") PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('@')	PORT_CHAR('/')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("> %") PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('>')	PORT_CHAR('%')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 ^") PORT_CODE(KEYCODE_0)	PORT_CHAR('0')	PORT_CHAR('^')

	PORT_START("LINE2")    /* row 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 .") PORT_CODE(KEYCODE_1)	PORT_CHAR('1')	PORT_CHAR('.')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 ;") PORT_CODE(KEYCODE_2)	PORT_CHAR('2')	PORT_CHAR(';')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 :") PORT_CODE(KEYCODE_3)	PORT_CHAR('3')	PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 ?") PORT_CODE(KEYCODE_4)	PORT_CHAR('4')	PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 !") PORT_CODE(KEYCODE_5)	PORT_CHAR('5')	PORT_CHAR('!')

	PORT_START("LINE3")    /* row 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 _") PORT_CODE(KEYCODE_6)	PORT_CHAR('6')	PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \"") PORT_CODE(KEYCODE_7)	PORT_CHAR('7')	PORT_CHAR('\"')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 #") PORT_CODE(KEYCODE_8)	PORT_CHAR('8')	PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (ESC)") PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A (SOH)") PORT_CODE(KEYCODE_A)	PORT_CHAR('A')

	PORT_START("LINE4")    /* row 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B (STH)") PORT_CODE(KEYCODE_B)	PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C (ETX)") PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D (EOT)") PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E (ENQ)") PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F (ACK)") PORT_CODE(KEYCODE_F)	PORT_CHAR('F')

	PORT_START("LINE5")    /* row 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G (BEL)") PORT_CODE(KEYCODE_G)	PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H (BS)") PORT_CODE(KEYCODE_H)	PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I (HT)") PORT_CODE(KEYCODE_I)	PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J (LF)") PORT_CODE(KEYCODE_J)	PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K (VT)") PORT_CODE(KEYCODE_K)	PORT_CHAR('K')

	PORT_START("LINE6")    /* row 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L (FF)") PORT_CODE(KEYCODE_L)	PORT_CHAR('L')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M (DEL)") PORT_CODE(KEYCODE_M)	PORT_CHAR('M')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N (SO)") PORT_CODE(KEYCODE_N)	PORT_CHAR('N')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O (SI)") PORT_CODE(KEYCODE_O)	PORT_CHAR('O')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P (DLE)") PORT_CODE(KEYCODE_P)	PORT_CHAR('P')

	PORT_START("LINE7")    /* row 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q (DC1)") PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R (DC2)") PORT_CODE(KEYCODE_R)	PORT_CHAR('R')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S (DC3)") PORT_CODE(KEYCODE_S)	PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T (DC4)") PORT_CODE(KEYCODE_T)	PORT_CHAR('T')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U (NAK)") PORT_CODE(KEYCODE_U)	PORT_CHAR('U')

	PORT_START("LINE8")    /* row 8 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V <-D") PORT_CODE(KEYCODE_V)		PORT_CHAR('V')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W (ETB)") PORT_CODE(KEYCODE_W)	PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X (CAN)") PORT_CODE(KEYCODE_X)	PORT_CHAR('X')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y (EM)") PORT_CODE(KEYCODE_Y)	PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z ->D") PORT_CODE(KEYCODE_Z)		PORT_CHAR('Z')

	/* analog joysticks (video board only) */

	PORT_START("BUTTONS")	/* joystick 1 & 2 buttons */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_PLAYER(2)

	PORT_START("JOY1_X")	/* joystick 1, X axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1)

	PORT_START("JOY1_Y")	/* joystick 1, Y axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(1) PORT_REVERSE

	PORT_START("JOY2_X")	/* joystick 2, X axis */
	PORT_BIT( 0x3ff, 0x180,  IPT_AD_STICK_X) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x180 ) PORT_PLAYER(2)

	PORT_START("JOY2_Y")	/* joystick 2, Y axis */
	PORT_BIT( 0x3ff, 0x1aa,  IPT_AD_STICK_Y) PORT_SENSITIVITY(JOYSTICK_SENSITIVITY) PORT_KEYDELTA(JOYSTICK_DELTA) PORT_MINMAX(0xd2,0x282 ) PORT_PLAYER(2) PORT_REVERSE
INPUT_PORTS_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    COMPANY                 FULLNAME */
COMP( 1978,	990189,	  0,		0,		tm990_189,	tm990_189,	0,		"Texas Instruments",	"TM 990/189 University Board microcomputer with University Basic" , 0)
COMP( 1980,	990189v,  990189,	0,		tm990_189_v,tm990_189,	0,		"Texas Instruments",	"TM 990/189 University Board microcomputer with University Basic and Video Board Interface" , 0)
