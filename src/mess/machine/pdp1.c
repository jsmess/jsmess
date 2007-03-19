/*
	pdp1 machine code

	includes emulation for I/O devices (with the IOT opcode) and control panel functions

	TODO:
	* typewriter out should overwrite the typewriter buffer
	* improve emulation of the internals of tape reader?
	* improve puncher timing?
*/

#include <stdio.h>

#include "driver.h"

#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"


/* This flag makes the emulated pdp-1 trigger a sequence break request when a character has been
typed on the typewriter keyboard.  It is useful in order to test sequence break, but we need
to emulate a connection box in which we can connect each wire to any interrupt line.  Also,
we need to determine the exact relationship between the status register and the sequence break
system (both standard and type 20). */
#define USE_SBS 0

#define LOG_IOT 0
#define LOG_IOT_EXTRA 0

/* IOT completion may take much more than the 5us instruction time.  A possible programming
error would be to execute an IOT before the last IOT of the same kind is over: such a thing
would confuse the targeted IO device, which might ignore the latter IOT, execute either
or both IOT incompletely, or screw up completely.  I insist that such an error can be caused
by a pdp-1 programming error, even if there is no emulator error. */
#define LOG_IOT_OVERLAP 0


static int tape_read(UINT8 *reply);
static void reader_callback(int dummy);
static void puncher_callback(int nac);
static void tyo_callback(int nac);
static void dpy_callback(int dummy);
static void pdp1_machine_stop(running_machine *machine);


/* pointer to pdp-1 RAM */
int *pdp1_memory;


/*
	devices which are known to generate a completion pulse (source: maintainance manual 9-??,
	and 9-20, 9-21):
	emulated:
	* perforated tape reader
	* perforated tape punch
	* typewriter output
	* CRT display
	unemulated:
	* card punch (pac: 43)
	* line printer (lpr, lsp, but NOT lfb)

	This list should probably include additional optional devices (card reader comes to mind).
*/

/* IO status word */
static int io_status;

/* defines for io_status bits */
enum
{
	io_st_pen = 0400000,	/* light pen: light has hit the pen */
	io_st_ptr = 0200000,	/* perforated tape reader: reader buffer full */
	io_st_tyo = 0100000,	/* typewriter out: device ready */
	io_st_tyi = 0040000,	/* typewriter in: new character in buffer */
	io_st_ptp = 0020000		/* perforated tape punch: device ready */
};

/* tape reader registers */
typedef struct tape_reader_t
{
	mess_image *fd;	/* file descriptor of tape image */

	int motor_on;	/* 1-bit reader motor on */

	int rb;			/* 18-bit reader buffer */
	int rcl;		/* 1-bit reader clutch */
	int rc;			/* 2-bit reader counter */
	int rby;		/* 1-bit reader binary mode flip-flop */
	int rcp;		/* 1-bit reader "need a completion pulse" flip-flop */

	mame_timer *timer;	/* timer to simulate reader timing */
} tape_reader_t;

static tape_reader_t tape_reader;


/* tape puncher registers */
typedef struct tape_puncher_t
{
	mess_image *fd;	/* file descriptor of tape image */

	mame_timer *timer;	/* timer to generate completion pulses */
} tape_puncher_t;

static tape_puncher_t tape_puncher;


/* typewriter registers */
typedef struct typewriter_t
{
	mess_image *fd;	/* file descriptor of output image */

	int tb;			/* typewriter buffer */

	mame_timer *tyo_timer;/* timer to generate completion pulses */
} typewriter_t;

static typewriter_t typewriter;


/* crt display timer */
static mame_timer *dpy_timer;

/* light pen config */
static lightpen_t lightpen;


/* MIT parallel drum (mostly similar to type 23) */
typedef struct parallel_drum_t
{
	mess_image *fd;	/* file descriptor of drum image */

	int il;			/* initial location (12-bit) */
	int wc;			/* word counter (12-bit) */
	int wcl;		/* word core location counter (16-bit) */
	int rfb;		/* read field buffer (5-bit) */
	int wfb;		/* write field buffer (5-bit) */

	int dba;

	mame_timer *rotation_timer;	/* timer called each time dc is 0 */
	mame_timer *il_timer;		/* timer called each time dc is il */
} parallel_drum_t;

static parallel_drum_t parallel_drum;

#define PARALLEL_DRUM_WORD_TIME TIME_IN_USEC(8.5)
#define PARALLEL_DRUM_ROTATION_TIME TIME_IN_USEC(8.5*4096)


static OPBASE_HANDLER(setOPbasefunc)
{
	/* just to get rid of the warnings */
	return -1;
}


static void pdp1_machine_reset(running_machine *machine)
{
	int config;

	config = readinputport(pdp1_config);
	pdp1_reset_param.extend_support = (config >> pdp1_config_extend_bit) & pdp1_config_extend_mask;
	pdp1_reset_param.hw_mul_div = (config >> pdp1_config_hw_mul_div_bit) & pdp1_config_hw_mul_div_mask;
	pdp1_reset_param.type_20_sbs = (config >> pdp1_config_type_20_sbs_bit) & pdp1_config_type_20_sbs_mask;

	tape_reader.timer = timer_alloc(reader_callback);
	tape_puncher.timer = timer_alloc(puncher_callback);
	typewriter.tyo_timer = timer_alloc(tyo_callback);
	dpy_timer = timer_alloc(dpy_callback);

	/* reset device state */
	tape_reader.rcl = tape_reader.rc = 0;
	io_status = io_st_tyo | io_st_ptp;
	lightpen.active = lightpen.down = 0;
	lightpen.x = lightpen.y = 0;
	lightpen.radius = 10;	/* ??? */
	pdp1_update_lightpen_state(&lightpen);
}


static void pdp1_machine_stop(running_machine *machine)
{
	/* the core will take care of freeing the timers, BUT we must set the variables
	to NULL if we don't want to risk confusing the tape image init function */
	tape_reader.timer = tape_puncher.timer = typewriter.tyo_timer = dpy_timer = NULL;
}


/*
	driver init function

	Set up the pdp1_memory pointer, and generate font data.
*/
MACHINE_START( pdp1 )
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[pdp1_fontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
		
		/* non-spacing middle dot */
		0x00,
		0x00,
		0x00,
		0x20,
		0x00,
		0x00,
		0x00,
		0x00,
		/* non-spacing overstrike */
		0x00,
		0x00,
		0x00,
		0x00,
		0xfc,
		0x00,
		0x00,
		0x00,
		/* implies */
		0x00,
		0x00,
		0x70,
		0x08,
		0x08,
		0x08,
		0x70,
		0x00,
		/* or */
		0x88,
		0x88,
		0x88,
		0x50,
		0x50,
		0x50,
		0x20,
		0x00,
		/* and */
		0x20,
		0x50,
		0x50,
		0x50,
		0x88,
		0x88,
		0x88,
		0x00,
		/* up arrow */
		0x20,
		0x70,
		0xa8,
		0x20,
		0x20,
		0x20,
		0x20,
		0x00,
		/* right arrow */
		0x00,
		0x20,
		0x10,
		0xf8,
		0x10,
		0x20,
		0x00,
		0x00,
		/* multiply */
		0x00,
		0x88,
		0x50,
		0x20,
		0x50,
		0x88,
		0x00,
		0x00,
	};

	/* set up memory regions */
	pdp1_memory = auto_malloc(0x40000);

	/* set up our font */
	dst = memory_region(REGION_GFX1);
	memcpy(dst, fontdata6x8, pdp1_fontdata_size);

	memory_set_opbase_handler(0, setOPbasefunc);

	add_reset_callback(machine, pdp1_machine_reset);
	add_exit_callback(machine, pdp1_machine_stop);
	return 0;
}


READ18_HANDLER(pdp1_read_mem)
{
	return pdp1_memory ? pdp1_memory[offset] : 0;
}


WRITE18_HANDLER(pdp1_write_mem)
{
	if (pdp1_memory)
		pdp1_memory[offset] = data;
}


/*
	perforated tape handling
*/

void pdp1_get_open_mode(const struct IODevice *dev, int id,
	unsigned int *readable, unsigned int *writeable, unsigned int *creatable)
{
	/* unit 0 is read-only, unit 1 is write-only */
	if (id)
	{
		*readable = 0;
		*writeable = 1;
		*creatable = 1;
	}
	else
	{
		*readable = 1;
		*writeable = 1;
		*creatable = 1;
	}
}



DEVICE_INIT( pdp1_tape )
{
	return INIT_PASS;
}



/*
	Open a perforated tape image

	unit 0 is reader (read-only), unit 1 is puncher (write-only)
*/
DEVICE_LOAD( pdp1_tape )
{
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		tape_reader.fd = image;

		/* start motor */
		tape_reader.motor_on = 1;

		/* restart reader IO when necessary */
		/* note that this function may be called before pdp1_init_machine, therefore
		before tape_reader.timer is allocated.  It does not matter, as the clutch is never
		down at power-up, but we must not call timer_enable with a NULL parameter! */
		if (tape_reader.timer)
		{
			if (tape_reader.motor_on && tape_reader.rcl)
			{
				/* delay is approximately 1/400s */
				timer_adjust(tape_reader.timer, TIME_IN_MSEC(2.5), 0, 0.);
			}
			else
			{
				timer_enable(tape_reader.timer, 0);
			}
		}
		break;

	case 1:
		/* punch unit */
		tape_puncher.fd = image;
		break;
	}

	return INIT_PASS;
}

DEVICE_UNLOAD( pdp1_tape )
{
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		tape_reader.fd = NULL;

		/* stop motor */
		tape_reader.motor_on = 0;

		if (tape_reader.timer)
			timer_enable(tape_reader.timer, 0);
		break;

	case 1:
		/* punch unit */
		tape_puncher.fd = NULL;
		break;
	}
}

/*
	Read a byte from perforated tape
*/
static int tape_read(UINT8 *reply)
{
	if (tape_reader.fd && (image_fread(tape_reader.fd, reply, 1) == 1))
		return 0;	/* unit OK */
	else
		return 1;	/* unit not ready */
}

/*
	Write a byte to perforated tape
*/
static void tape_write(UINT8 data)
{
	if (tape_puncher.fd)
		image_fwrite(tape_puncher.fd, & data, 1);
}

/*
	common code for tape read commands (RPA, RPB, and read-in mode)
*/
static void begin_tape_read(int binary, int nac)
{
	tape_reader.rb = 0;
	tape_reader.rcl = 1;
	tape_reader.rc = (binary) ? 1 : 3;
	tape_reader.rby = (binary) ? 1 : 0;
	tape_reader.rcp = nac;

	if (LOG_IOT_OVERLAP)
	{
		if (timer_enable(tape_reader.timer, 0))
			logerror("Error: overlapped perforated tape reads (Read-in mode, RPA/RPB instruction)\n");
	}
	/* set up delay if tape is advancing */
	if (tape_reader.motor_on && tape_reader.rcl)
	{
		/* delay is approximately 1/400s */
		timer_adjust(tape_reader.timer, TIME_IN_MSEC(2.5), 0, 0.);
	}
	else
	{
		timer_enable(tape_reader.timer, 0);
	}
}

/*
	timer callback to simulate reader IO
*/
static void reader_callback(int dummy)
{
	int not_ready;
	UINT8 data;


	(void) dummy;

	if (tape_reader.rc)
	{
		not_ready = tape_read(& data);
		if (not_ready)
		{
			tape_reader.motor_on = 0;	/* let us stop the motor */
		}
		else
		{
			if ((! tape_reader.rby) || (data & 0200))
			{
				tape_reader.rb |= (tape_reader.rby) ? (data & 077) : data;

				if (tape_reader.rc != 3)
				{
					tape_reader.rb <<= 6;
				}

				tape_reader.rc = (tape_reader.rc+1) & 3;

				if (tape_reader.rc == 0)
				{	/* IO complete */
					tape_reader.rcl = 0;
					if (tape_reader.rcp)
					{
						cpunum_set_reg(0, PDP1_IO, tape_reader.rb);	/* transfer reader buffer to IO */
						pdp1_pulse_iot_done();
					}
					else
						io_status |= io_st_ptr;
				}
			}
		}
	}

	if (tape_reader.motor_on && tape_reader.rcl)
		/* delay is approximately 1/400s */
		timer_adjust(tape_reader.timer, TIME_IN_MSEC(2.5), 0, 0.);
	else
		timer_enable(tape_reader.timer, 0);
}

/*
	timer callback to generate punch completion pulse
*/
static void puncher_callback(int nac)
{
	io_status |= io_st_ptp;
	if (nac)
	{
		pdp1_pulse_iot_done();
	}
}

/*
	Initiate read of a 18-bit word in binary format from tape (used in read-in mode)
*/
void pdp1_tape_read_binary(void)
{
	begin_tape_read(1, 1);
}

/*
	perforated tape reader iot callbacks
*/

/*
	rpa iot callback

	op2: iot command operation (equivalent to mb & 077)
	nac: nead a completion pulse
	mb: contents of the MB register
	io: pointer on the IO register
	ac: contents of the AC register
*/
/*
 * Read Perforated Tape, Alphanumeric
 * rpa Address 0001
 *
 * This instruction reads one line of tape (all eight Channels) and transfers the resulting 8-bit code to
 * the Reader Buffer. If bits 5 and 6 of the rpa function are both zero (720001), the contents of the
 * Reader Buffer must be transferred to the IO Register by executing the rrb instruction. When the Reader
 * Buffer has information ready to be transferred to the IO Register, Status Register Bit 1 is set to
 * one. If bits 5 and 6 are different (730001 or 724001) the 8-bit code read from tape is automatically
 * transferred to the IO Register via the Reader Buffer and appears as follows:
 *
 * IO Bits        10 11 12 13 14 15 16 17
 * TAPE CHANNELS  8  7  6  5  4  3  2  1
 */
void iot_rpa(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("Warning, RPA instruction not fully emulated: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	begin_tape_read(0, nac);
}

/*
	rpb iot callback
*/
/*
 * Read Perforated Tape, Binary rpb
 * Address 0002
 *
 * The instruction reads three lines of tape (six Channels per line) and assembles
 * the resulting 18-bit word in the Reader Buffer.  For a line to be recognized by
 * this instruction Channel 8 must be punched (lines with Channel 8 not punched will
 * be skipped over).  Channel 7 is ignored.  The instruction sub 5137, for example,
 * appears on tape and is assembled by rpb as follows:
 *
 * Channel 8 7 6 5 4 | 3 2 1
 * Line 1  X   X     |   X
 * Line 2  X   X   X |     X
 * Line 3  X     X X | X X X
 * Reader Buffer 100 010 101 001 011 111
 *
 * (Vertical dashed line indicates sprocket holes and the symbols "X" indicate holes
 * punched in tape). 
 *
 * If bits 5 and 6 of the rpb instruction are both zero (720002), the contents of
 * the Reader Buffer must be transferred to the IO Register by executing
 * a rrb instruction.  When the Reader Buffer has information ready to be transferred
 * to the IO Register, Status Register Bit 1 is set to one.  If bits 5 and 6 are
 * different (730002 or 724002) the 18-bit word read from tape is automatically
 * transferred to the IO Register via the Reader Buffer. 
 */
void iot_rpb(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("Warning, RPB instruction not fully emulated: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	begin_tape_read(1, nac);
}

/*
	rrb iot callback
*/
void iot_rrb(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("RRB instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	*io = tape_reader.rb;
	io_status &= ~io_st_ptr;
}

/*
	perforated tape punch iot callbacks
*/

/*
	ppa iot callback
*/
/*
 * Punch Perforated Tape, Alphanumeric
 * ppa Address 0005
 *
 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
 * Bit 17 conditions Hole 1. Bit 16 conditions Hole 2, etc. Bit 10 conditions Hole 8
 */
void iot_ppa(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("PPA instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	tape_write(*io & 0377);
	io_status &= ~io_st_ptp;
	/* delay is approximately 1/63.3 second */
	if (LOG_IOT_OVERLAP)
	{
		if (timer_enable(tape_puncher.timer, 0))
			logerror("Error: overlapped PPA/PPB instructions: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));
	}

	timer_adjust(tape_puncher.timer, TIME_IN_MSEC(15.8), nac, 0.);
}

/*
	ppb iot callback
*/
/*
 * Punch Perforated Tape, Binary
 * ppb Addres 0006 
 *
 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
 * Bit 5 conditions Hole 1. Bit 4 conditions Hole 2, etc. Bit 0 conditions Hole 6.
 * Hole 7 is left blank. Hole 8 is always punched in this mode. 
 */
void iot_ppb(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("PPB instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	tape_write((*io >> 12) | 0200);
	io_status &= ~io_st_ptp;
	/* delay is approximately 1/63.3 second */
	if (LOG_IOT_OVERLAP)
	{
		if (timer_enable(tape_puncher.timer, 0))
			logerror("Error: overlapped PPA/PPB instructions: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));
	}
	timer_adjust(tape_puncher.timer, TIME_IN_MSEC(15.8), nac, 0.);
}


/*
	Typewriter handling

	The alphanumeric on-line typewriter is a standard device on pdp-1: it
	can both handle keyboard input and print output text.
*/

/*
	Open a file for typewriter output
*/
DEVICE_LOAD(pdp1_typewriter)
{
	/* open file */
	typewriter.fd = image;

	io_status |= io_st_tyo;

	return INIT_PASS;
}

DEVICE_UNLOAD(pdp1_typewriter)
{
	typewriter.fd = NULL;
}

/*
	Write a character to typewriter
*/
static void typewriter_out(UINT8 data)
{
	if (LOG_IOT_EXTRA)
		logerror("typewriter output %o\n", data);

	pdp1_typewriter_drawchar(data);
	if (typewriter.fd)
#if 1
		image_fwrite(typewriter.fd, & data, 1);
#else
	{
		static const char ascii_table[2][64] =
		{	/* n-s = non-spacing */
			{	/* lower case */
				' ',				'1',				'2',				'3',
				'4',				'5',				'6',				'7',
				'8',				'9',				'*',				'*',
				'*',				'*',				'*',				'*',
				'0',				'/',				's',				't',
				'u',				'v',				'w',				'x',
				'y',				'z',				'*',				',',
				'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
				'\200',/*n-s middle dot*/'j',			'k',				'l',
				'm',				'n',				'o',				'p',
				'q',				'r',				'*',				'*',
				'-',				')',				'\201',/*n-s overstrike*/'(',
				'*',				'a',				'b',				'c',
				'd',				'e',				'f',				'g',
				'h',				'i',				'*',/*Lower Case*/	'.',
				'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
			},
			{	/* upper case */
				' ',				'"',				'\'',				'~',
				'\202',/*implies*/	'\203',/*or*/		'\204',/*and*/		'<',
				'>',				'\205',/*up arrow*/	'*',				'*',
				'*',				'*',				'*',				'*',
				'\206',/*right arrow*/'?',				'S',				'T',
				'U',				'V',				'W',				'X',
				'Y',				'Z',				'*',				'=',
				'*',/*black*/		'*',/*red*/			'\t',/*Tab*/		'*',
				'_',/*n-s???*/		'J',				'K',				'L',
				'M',				'N',				'O',				'P',
				'Q',				'R',				'*',				'*',
				'+',				']',				'|',/*n-s???*/		'[',
				'*',				'A',				'B',				'C',
				'D',				'E',				'F',				'G',
				'H',				'I',				'*',/*Lower Case*/	'\207',/*multiply*/
				'*',/*Upper Case*/	'\b',/*Backspace*/	'*',				'*'/*Carriage Return*/
			}
		};
		static int case_shift;

		data &= 0x3f;

		switch (data)
		{
		case 034:
			/* Black: ignore */
			//color = color_typewriter_black;
			{
				static char black[5] = { '\033', '[', '3', '0', 'm' };
				image_fwrite(typewriter.fd, black, sizeof(black));
			}
			break;

		case 035:
			/* Red: ignore */
			//color = color_typewriter_red;
			{
				static char red[5] = { '\033', '[', '3', '1', 'm' };
				image_fwrite(typewriter.fd, red, sizeof(red));
			}
			break;

		case 072:
			/* Lower case */
			case_shift = 0;
			break;

		case 074:
			/* Upper case */
			case_shift = 1;
			break;

		case 077:
			/* Carriage Return */
			{
				static char line_end[2] = { '\r', '\n' };
				image_fwrite(typewriter.fd, line_end, sizeof(line_end));
			}
			break;

		default:
			/* Any printable character... */

			if ((data != 040) && (data != 056))	/* 040 and 056 are non-spacing characters: don't try to print right now */
				/* print character (lookup ASCII equivalent in table) */
				image_fwrite(typewriter.fd, & ascii_table[case_shift][data], 1);

			break;
		}
	}
#endif
}

/*
	timer callback to generate typewriter completion pulse
*/
static void tyo_callback(int nac)
{
	io_status |= io_st_tyo;
	if (nac)
	{
		pdp1_pulse_iot_done();
	}
}

/*
	tyo iot callback
*/
void iot_tyo(int op2, int nac, int mb, int *io, int ac)
{
	int ch, delay;

	if (LOG_IOT_EXTRA)
		logerror("Warning, TYO instruction not fully emulated: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	ch = (*io) & 077;

	typewriter_out(ch);
	io_status &= ~io_st_tyo;

	/* compute completion delay (source: maintainance manual 9-12, 9-13 and 9-14) */
	switch (ch)
	{
	case 072:	/* lower-case */
	case 074:	/* upper-case */
	case 034:	/* black */
	case 035:	/* red */
		delay = 175;	/* approximately 175ms (?) */
		break;

	case 077:	/* carriage return */
		delay = 205;	/* approximately 205ms (?) */
		break;

	default:
		delay = 105;	/* approximately 105ms */
		break;
	}
	if (LOG_IOT_OVERLAP)
	{
		if (timer_enable(typewriter.tyo_timer, 0))
			logerror("Error: overlapped TYO instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));
	}

	timer_adjust(typewriter.tyo_timer, TIME_IN_MSEC(delay), nac, 0.);
}

/*
	tyi iot callback
*/
/* When a typewriter key is struck, the code for the struck key is placed in the
 * typewriter buffer, Program Flag 1 is set, and the type-in status bit is set to
 * one. A program designed to accept typed-in data would periodically check
 * Program Flag 1, and if found to be set, an In-Out Transfer Instruction with
 * address 4 could be executed for the information to be transferred to the
 * In-Out Register. This In-Out Transfer should not use the optional in-out wait.
 * The information contained in the typewriter buffer is then transferred to the
 * right six bits of the In-Out Register. The tyi instruction automatically
 * clears the In-Out Register before transferring the information and also clears
 * the type-in status bit.
 */
void iot_tyi(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("Warning, TYI instruction not fully emulated: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	*io = typewriter.tb;
	if (! (io_status & io_st_tyi))
		*io |= 0100;
	else
	{
		io_status &= ~io_st_tyi;
		if (USE_SBS)
			cpunum_set_input_line_and_vector(0, 0, CLEAR_LINE, 0);	/* interrupt it, baby */
	}
}


/*
 * PRECISION CRT DISPLAY (TYPE 30)
 *
 * This sixteen-inch cathode ray tube display is intended to be used as an on-line output device for the
 * PDP-1. It is useful for high speed presentation of graphs, diagrams, drawings, and alphanumerical
 * information. The unit is solid state, self-powered and completely buffered. It has magnetic focus and
 * deflection.
 *
 * Display characteristics are as follows:
 *
 *     Random point plotting
 *     Accuracy of points +/-3 per cent of raster size
 *     Raster size 9.25 by 9.25 inches
 *     1024 by 1024 addressable locations
 *     Fixed origin at center of CRT
 *     Ones complement binary arithmetic
 *     Plots 20,000 points per second
 *
 * Resolution is such that 512 points along each axis are discernable on the face of the tube.
 *
 * One instruction is added to the PDP-1 with the installation of this display:
 *
 * Display One Point On CRT
 * dpy Address 0007
 *
 * This instruction clears the light pen status bit and displays one point using bits 0 through 9 of the
 * AC to represent the (signed) X coordinate of the point and bits 0 through 9 of the IO as the (signed)
 * Y coordinate.
 *
 * Many variations of the Type 30 Display are available. Some of these include hardware for line and
 * curve generation.
 */


/*
	timer callback to generate crt completion pulse
*/
static void dpy_callback(int dummy)
{
	(void) dummy;
	pdp1_pulse_iot_done();
}


/*
	dpy iot callback

	Light on one pixel on CRT
*/
void iot_dpy(int op2, int nac, int mb, int *io, int ac)
{
	int x;
	int y;


	x = ((ac+0400000) & 0777777) >> 8;
	y = (((*io)+0400000) & 0777777) >> 8;
	pdp1_plot(x, y);

	/* light pen 32 support */
	io_status &= ~io_st_pen;

	if (lightpen.down && ((x-lightpen.x)*(x-lightpen.x)+(y-lightpen.y)*(y-lightpen.y) < lightpen.radius*lightpen.radius))
	{
		io_status |= io_st_pen;

		cpunum_set_reg(0, PDP1_PF3, 1);
	}

	if (nac)
	{
		/* 50us delay */
		if (LOG_IOT_OVERLAP)
		{
			/* note that overlap detection is incomplete: it will only work if both DPY
			instructions require a completion pulse */
			if (timer_enable(dpy_timer, 0))
				logerror("Error: overlapped DPY instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));
		}
		timer_adjust(dpy_timer, TIME_IN_USEC(50), 0, 0.);
	}
}



/*
	MIT parallel drum (variant of type 23)
*/

static void parallel_drum_set_il(int il)
{
	double il_phase;

	parallel_drum.il = il;

	il_phase = (il * PARALLEL_DRUM_WORD_TIME) - timer_timeelapsed(parallel_drum.rotation_timer);
	if (il_phase < 0.)
		il_phase += PARALLEL_DRUM_ROTATION_TIME;
	timer_adjust(parallel_drum.il_timer, il_phase, 0, PARALLEL_DRUM_ROTATION_TIME);
}

static void il_timer_callback(int dummy)
{
	(void) dummy;

	if (parallel_drum.dba)
	{
		/* set break request and status bit 5 */
		/* ... */
	}
}

static void parallel_drum_init(void)
{
	parallel_drum.rotation_timer = timer_alloc(NULL);
	timer_adjust(parallel_drum.rotation_timer, PARALLEL_DRUM_ROTATION_TIME, 0, PARALLEL_DRUM_ROTATION_TIME);

	parallel_drum.il_timer = timer_alloc(il_timer_callback);
	parallel_drum_set_il(0);
}

/*
	Open a file for drum
*/
DEVICE_LOAD(pdp1_drum)
{
	/* open file */
	parallel_drum.fd = image;

	return INIT_PASS;
}

DEVICE_UNLOAD(pdp1_drum)
{
	parallel_drum.fd = NULL;
}

void iot_dia(int op2, int nac, int mb, int *io, int ac)
{
	parallel_drum.wfb = ((*io) & 0370000) >> 12;
	parallel_drum_set_il((*io) & 0007777);

	parallel_drum.dba = 0;	/* right? */
}

void iot_dba(int op2, int nac, int mb, int *io, int ac)
{
	parallel_drum.wfb = ((*io) & 0370000) >> 12;
	parallel_drum_set_il((*io) & 0007777);

	parallel_drum.dba = 1;
}

/*
	Read a word from drum
*/
static UINT32 drum_read(int field, int position)
{
	int offset = (field*4096+position)*3;
	UINT8 buf[3];

	if (parallel_drum.fd && (!image_fseek(parallel_drum.fd, offset, SEEK_SET)) && (image_fread(parallel_drum.fd, buf, 3) == 3))
		return ((buf[0] << 16) | (buf[1] << 8) | buf[2]) & 0777777;

	return 0;
}

/*
	Write a word to drum
*/
static void drum_write(int field, int position, UINT32 data)
{
	int offset = (field*4096+position)*3;
	UINT8 buf[3];

	if (parallel_drum.fd)
	{
		buf[0] = data >> 16;
		buf[1] = data >> 8;
		buf[2] = data;

		image_fseek(parallel_drum.fd, offset, SEEK_SET);
		image_fwrite(parallel_drum.fd, buf, 3);
	}
}

void iot_dcc(int op2, int nac, int mb, int *io, int ac)
{
	double delay;
	int dc;

	parallel_drum.rfb = ((*io) & 0370000) >> 12;
	parallel_drum.wc = - ((*io) & 0007777);

	parallel_drum.wcl = ac & 0177777/*0007777???*/;

	parallel_drum.dba = 0;	/* right? */
	/* clear status bit 5... */

	/* do transfer */
	delay = timer_timeleft(parallel_drum.il_timer);
	dc = parallel_drum.il;
	do
	{
		if ((parallel_drum.wfb >= 1) && (parallel_drum.wfb <= 22))
		{
			drum_write(parallel_drum.wfb-1, dc, READ_PDP_18BIT(parallel_drum.wcl));
		}

		if ((parallel_drum.rfb >= 1) && (parallel_drum.rfb <= 22))
		{
			WRITE_PDP_18BIT(parallel_drum.wcl, drum_read(parallel_drum.rfb-1, dc));
		}

		parallel_drum.wc = (parallel_drum.wc+1) & 07777;
		parallel_drum.wcl = (parallel_drum.wcl+1) & 0177777/*0007777???*/;
		dc = (dc+1) & 07777;
		if (parallel_drum.wc)
			delay += PARALLEL_DRUM_WORD_TIME;
	} while (parallel_drum.wc);
	activecpu_adjust_icount(-TIME_TO_CYCLES(0, delay));
	/* if no error, skip */
	cpunum_set_reg(0, PDP1_PC, cpunum_get_reg(0, PDP1_PC)+1);
}

void iot_dra(int op2, int nac, int mb, int *io, int ac)
{
	(*io) = (int) (timer_timeelapsed(parallel_drum.rotation_timer)/PARALLEL_DRUM_WORD_TIME) & 0007777;

	/* set parity error and timing error... */
}



/*
	iot 11 callback

	Read state of Spacewar! controllers

	Not documented, except a few comments in the Spacewar! source code:
		it should leave the control word in the io as follows.
		high order 4 bits, rotate ccw, rotate cw, (both mean hyperspace)
		fire rocket, and fire torpedo. low order 4 bits, same for
		other ship. routine is entered by jsp cwg.
*/
void iot_011(int op2, int nac, int mb, int *io, int ac)
{
	int key_state = readinputport(pdp1_spacewar_controllers);
	int reply;


	reply = 0;

	if (key_state & ROTATE_RIGHT_PLAYER2)
		reply |= 0400000;
	if (key_state & ROTATE_LEFT_PLAYER2)
		reply |= 0200000;
	if (key_state & THRUST_PLAYER2)
		reply |= 0100000;
	if (key_state & FIRE_PLAYER2)
		reply |= 0040000;
	if (key_state & HSPACE_PLAYER2)
		reply |= 0600000;

	if (key_state & ROTATE_RIGHT_PLAYER1)
		reply |= 010;
	if (key_state & ROTATE_LEFT_PLAYER1)
		reply |= 004;
	if (key_state & THRUST_PLAYER1)
		reply |= 002;
	if (key_state & FIRE_PLAYER1)
		reply |= 001;
	if (key_state & HSPACE_PLAYER1)
		reply |= 014;

	*io = reply;
}


/*
	cks iot callback

	check IO status
*/
void iot_cks(int op2, int nac, int mb, int *io, int ac)
{
	if (LOG_IOT_EXTRA)
		logerror("CKS instruction: mb=0%06o, pc=0%06o\n", (unsigned) mb, (unsigned) cpunum_get_reg(0, PDP1_PC));

	*io = io_status;
}


/*
	callback which is called when Start Clear is pulsed

	IO devices should reset
*/
void pdp1_io_sc_callback(void)
{
	tape_reader.rcl = tape_reader.rc = 0;
	if (tape_reader.timer)
		timer_enable(tape_reader.timer, 0);

	if (tape_puncher.timer)
		timer_enable(tape_puncher.timer, 0);

	if (typewriter.tyo_timer)
		timer_enable(typewriter.tyo_timer, 0);

	if (dpy_timer)
		timer_enable(dpy_timer, 0);

	io_status = io_st_tyo | io_st_ptp;
}


/*
	typewriter keyboard handler
*/
static void pdp1_keyboard(void)
{
	int i;
	int j;

	int typewriter_keys[4];
	static int old_typewriter_keys[4];

	int typewriter_transitions;


	for (i=0; i<4; i++)
		typewriter_keys[i] = readinputport(pdp1_typewriter + i);

	for (i=0; i<4; i++)
	{
		typewriter_transitions = typewriter_keys[i] & (~ old_typewriter_keys[i]);
		if (typewriter_transitions)
		{
			for (j=0; (((typewriter_transitions >> j) & 1) == 0) /*&& (j<16)*/; j++)
				;
			typewriter.tb = (i << 4) + j;
			io_status |= io_st_tyi;
			#if USE_SBS
				cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 0);	/* interrupt it, baby */
			#endif
			cpunum_set_reg(0, PDP1_PF1, 1);
			pdp1_typewriter_drawchar(typewriter.tb);	/* we want to echo input */
			break;
		}
	}

	for (i=0; i<4; i++)
		old_typewriter_keys[i] = typewriter_keys[i];
}

static void pdp1_lightpen(void)
{
	int x_delta, y_delta;
	int current_state;
	static int previous_state;

	lightpen.active = (readinputport(pdp1_config) >> pdp1_config_lightpen_bit) & pdp1_config_lightpen_mask;

	current_state = readinputport(pdp1_lightpen_state);

	/* update pen down state */
	lightpen.down = lightpen.active && (current_state & pdp1_lightpen_down);

	/* update size of pen tip hole */
	if ((current_state & ~previous_state) & pdp1_lightpen_smaller)
	{
		lightpen.radius --;
		if (lightpen.radius < 0)
			lightpen.radius = 0;
	}
	if ((current_state & ~previous_state) & pdp1_lightpen_larger)
	{
		lightpen.radius ++;
		if (lightpen.radius > 32)
			lightpen.radius = 32;
	}

	previous_state = current_state;

	/* update pen position */
	x_delta = readinputport(pdp1_lightpen_x);
	y_delta = readinputport(pdp1_lightpen_y);

	if (x_delta >= 0x80)
		x_delta -= 0x100;
	if (y_delta >= 0x80)
		y_delta -= 256;

	lightpen.x += x_delta;
	lightpen.y += y_delta;

	if (lightpen.x < 0)
		lightpen.x = 0;
	if (lightpen.x > 1023)
		lightpen.x = 1023;
	if (lightpen.y < 0)
		lightpen.y = 0;
	if (lightpen.y > 1023)
		lightpen.y = 1023;

	pdp1_update_lightpen_state(&lightpen);
}

/*
	Not a real interrupt - just handle keyboard input
*/
INTERRUPT_GEN( pdp1_interrupt )
{
	int control_keys;
	int tw_keys;
	int ta_keys;

	static int old_control_keys;
	static int old_tw_keys;
	static int old_ta_keys;

	int control_transitions;
	int tw_transitions;
	int ta_transitions;


	cpunum_set_reg(0, PDP1_SS, readinputport(pdp1_sense_switches));

	/* read new state of control keys */
	control_keys = readinputport(pdp1_control_switches);

	if (control_keys & pdp1_control)
	{
		/* compute transitions */
		control_transitions = control_keys & (~ old_control_keys);

		if (control_transitions & pdp1_extend)
		{
			cpunum_set_reg(0, PDP1_EXTEND_SW, ! cpunum_get_reg(0, PDP1_EXTEND_SW));
		}
		if (control_transitions & pdp1_start_nobrk)
		{
			pdp1_pulse_start_clear();	/* pulse Start Clear line */
			cpunum_set_reg(0, PDP1_EXD, cpunum_get_reg(0, PDP1_EXTEND_SW));
			cpunum_set_reg(0, PDP1_SBM, 0);
			cpunum_set_reg(0, PDP1_OV, 0);
			cpunum_set_reg(0, PDP1_PC, cpunum_get_reg(0, PDP1_TA));
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_start_brk)
		{
			pdp1_pulse_start_clear();	/* pulse Start Clear line */
			cpunum_set_reg(0, PDP1_EXD, cpunum_get_reg(0, PDP1_EXTEND_SW));
			cpunum_set_reg(0, PDP1_SBM, 1);
			cpunum_set_reg(0, PDP1_OV, 0);
			cpunum_set_reg(0, PDP1_PC, cpunum_get_reg(0, PDP1_TA));
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_stop)
		{
			cpunum_set_reg(0, PDP1_RUN, 0);
			cpunum_set_reg(0, PDP1_RIM, 0);	/* bug : we stop after reading an even-numbered word
											(i.e. data), whereas a real pdp-1 stops after reading
											an odd-numbered word (i.e. dio instruciton) */
		}
		if (control_transitions & pdp1_continue)
		{
			cpunum_set_reg(0, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_examine)
		{
			pdp1_pulse_start_clear();	/* pulse Start Clear line */
			cpunum_set_reg(0, PDP1_PC, cpunum_get_reg(0, PDP1_TA));
			cpunum_set_reg(0, PDP1_MA, cpunum_get_reg(0, PDP1_PC));
			cpunum_set_reg(0, PDP1_IR, LAC);	/* this instruction is actually executed */

			cpunum_set_reg(0, PDP1_MB, READ_PDP_18BIT(cpunum_get_reg(0, PDP1_MA)));
			cpunum_set_reg(0, PDP1_AC, cpunum_get_reg(0, PDP1_MB));
		}
		if (control_transitions & pdp1_deposit)
		{
			pdp1_pulse_start_clear();	/* pulse Start Clear line */
			cpunum_set_reg(0, PDP1_PC, cpunum_get_reg(0, PDP1_TA));
			cpunum_set_reg(0, PDP1_MA, cpunum_get_reg(0, PDP1_PC));
			cpunum_set_reg(0, PDP1_AC, cpunum_get_reg(0, PDP1_TW));
			cpunum_set_reg(0, PDP1_IR, DAC);	/* this instruction is actually executed */

			cpunum_set_reg(0, PDP1_MB, cpunum_get_reg(0, PDP1_AC));
			WRITE_PDP_18BIT(cpunum_get_reg(0, PDP1_MA), cpunum_get_reg(0, PDP1_MB));
		}
		if (control_transitions & pdp1_read_in)
		{	/* set cpu to read instructions from perforated tape */
			pdp1_pulse_start_clear();	/* pulse Start Clear line */
			cpunum_set_reg(0, PDP1_PC, (cpunum_get_reg(0, PDP1_TA) & 0170000)
										|  (cpunum_get_reg(0, PDP1_PC) & 0007777));	/* transfer ETA to EPC */
			/*cpunum_set_reg(0, PDP1_MA, cpunum_get_reg(0, PDP1_PC));*/
			cpunum_set_reg(0, PDP1_EXD, cpunum_get_reg(0, PDP1_EXTEND_SW));
			cpunum_set_reg(0, PDP1_OV, 0);		/* right??? */
			cpunum_set_reg(0, PDP1_RUN, 0);
			cpunum_set_reg(0, PDP1_RIM, 1);
		}
		if (control_transitions & pdp1_reader)
		{
		}
		if (control_transitions & pdp1_tape_feed)
		{
		}
		if (control_transitions & pdp1_single_step)
		{
			cpunum_set_reg(0, PDP1_SNGL_STEP, ! cpunum_get_reg(0, PDP1_SNGL_STEP));
		}
		if (control_transitions & pdp1_single_inst)
		{
			cpunum_set_reg(0, PDP1_SNGL_INST, ! cpunum_get_reg(0, PDP1_SNGL_INST));
		}

		/* remember new state of control keys */
		old_control_keys = control_keys;


		/* handle test word keys */
		tw_keys = (readinputport(pdp1_tw_switches_MSB) << 16) | readinputport(pdp1_tw_switches_LSB);

		/* compute transitions */
		tw_transitions = tw_keys & (~ old_tw_keys);

		if (tw_transitions)
			cpunum_set_reg(0, PDP1_TW, cpunum_get_reg(0, PDP1_TW) ^ tw_transitions);

		/* remember new state of test word keys */
		old_tw_keys = tw_keys;


		/* handle address keys */
		ta_keys = readinputport(pdp1_ta_switches);

		/* compute transitions */
		ta_transitions = ta_keys & (~ old_ta_keys);

		if (ta_transitions)
			cpunum_set_reg(0, PDP1_TA, cpunum_get_reg(0, PDP1_TA) ^ ta_transitions);

		/* remember new state of test word keys */
		old_ta_keys = ta_keys;

	}
	else
	{
		old_control_keys = 0;
		old_tw_keys = 0;
		old_ta_keys = 0;

		pdp1_keyboard();
	}

	pdp1_lightpen();
}
