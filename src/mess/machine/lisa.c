/*
	experimental LISA driver

	This driver runs most ROM test code successfully, and it loads the boot
	file, but it locks when booting from Lisa OS (probably because of floppy
	write bug).  MacWorks boots fine, though (I think it looks when we
	attempt to write to floppy, too).

	TODO :
	* fix floppy write bug
	* *** Boot and run LisaTest !!! ***
	* finish MMU (does not switch to bank 0 on 68k trap) --- fixed in June 2003???
	* support hard disk to boot office system
	* finish keyboard/mouse support
	* finish clock support
	* write SCC support
	* finalize sound support (involves adding new features to the 6522 VIA core)
	* fix warm-reset (I think I need to use a callback when 68k RESET
	  instruction is called)
	* write support for additionnal hardware (hard disk, etc...)
	* emulate LISA1 (?)
	* optimize MMU emulation !

	DONE (just a reminder to uplift my spirit) :
	* the lion's share of MMU (spring 2000)
	* video hardware (except contrast control) (spring 2000)
	* LISA2/Mac XL floppy hardware (november 2000)

	Credits :
	* the lisaemu project (<http://www.sundernet.com/>) has gathered much
	   hardware information (books, schematics...) without which this driver
	   could never have been written
	* The driver raised the interest of several MESS regulars (Paul Lunga,
	  Dennis Munsie...) who supported my feeble efforts

	Raphael Nabet, 2000-2003
*/

#include "driver.h"
#include "machine/6522via.h"
#include "machine/applefdc.h"
#include "devices/sonydriv.h"
#include "machine/lisa.h"
#include "cpu/m68000/m68k.h"
#include "sound/speaker.h"


/*
	pointers with RAM & ROM location
*/

/* up to 2MB of 68k RAM (normally 1MB or 512kb), generally 16kb of ROM */
UINT8 *lisa_ram_ptr;
UINT8 *lisa_rom_ptr;

/* offsets in REGION_CPU1 */
#define RAM_OFFSET 0x004000
#define ROM_OFFSET 0x000000

/* 1kb of RAM for 6504 floppy disk controller (shared with 68000), and 4kb of
ROM (8kb on some boards, but then only one 4kb bank is selected, according to
the drive type (TWIGGY or 3.5'')) */
UINT8 *lisa_fdc_ram;
UINT8 *lisa_fdc_rom;

/* special ROM (includes S/N) */
UINT8 *videoROM_ptr;


/*
	MMU regs
*/

static int setup;	/* MMU setup mode : allows to edit the MMU regs */

static int seg;		/* current SEG0-1 state (-> MMU register bank) */

/* lisa MMU segment regs */
typedef struct real_mmu_entry
{
	UINT16 sorg;
	UINT16 slim;
} real_mmu_entry;

/* MMU regs translated into a more efficient format */
typedef struct mmu_entry
{
	offs_t sorg;	/* (real_sorg & 0x0fff) << 9 */
	enum { RAM_stack_r, RAM_r, RAM_stack_rw, RAM_rw, IO, invalid, special_IO } type;	/* <-> (real_slim & 0x0f00) */
	int slim;	/* (~ ((real_slim & 0x00ff) << 9)) & 0x01ffff */
} mmu_entry;

static real_mmu_entry real_mmu_regs[4][128];	/* 4 banks of 128 regs */
static mmu_entry mmu_regs[4][128];


/*
	parity logic - only hard errors are emulated for now, since
	a) the ROMs power-up test only tests hard errors
	b) most memory boards do not use soft errors (i.e. they only generate 1
	  parity bit to detect errors, instead of generating several ECC bits to
	  fix errors)
*/

static int diag2;			/* -> writes wrong parity data into RAM */
static int test_parity;		/* detect parity hard errors */
static UINT16 mem_err_addr_latch;	/* address when parity error occured */
static int parity_error_pending;	/* parity error interrupt pending */

static int bad_parity_count;	/* total number of bytes in RAM which have wrong parity */
static UINT8 *bad_parity_table;	/* array : 1 bit set for each byte in RAM with wrong parity */


/*
	video
*/
static int VTMSK;				/* VBI enable */
static int VTIR;				/* VBI pending */
static UINT16 video_address_latch;	/* register : MSBs of screen bitmap address (LSBs are 0s) */
static UINT16 *videoram_ptr;		/* screen bitmap base address (derived from video_address_latch) */


/*
	2 vias : one is used for communication with COPS ; the other may be used to interface
	a hard disk
*/

static  READ8_HANDLER(COPS_via_in_b);
static WRITE8_HANDLER(COPS_via_out_a);
static WRITE8_HANDLER(COPS_via_out_b);
static WRITE8_HANDLER(COPS_via_out_ca2);
static WRITE8_HANDLER(COPS_via_out_cb2);
static void COPS_via_irq_func(int val);
static  READ8_HANDLER(parallel_via_in_b);

static int KBIR;	/* COPS VIA interrupt pending */

static struct via6522_interface lisa_via6522_intf[2] =
{
	{	/* COPS via */
		NULL, COPS_via_in_b,
		NULL, NULL,
		NULL, NULL,
		COPS_via_out_a, COPS_via_out_b,
		COPS_via_out_ca2, COPS_via_out_cb2,
		NULL, NULL,
		COPS_via_irq_func,
	},
	{	/* parallel interface via - incomplete */
		NULL, parallel_via_in_b,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
	}
};

/*
	floppy disk interface
*/

static int FDIR;
static int DISK_DIAG;

static int MT1;

static int PWM_floppy_motor_speed;

/*
	lisa model identification
*/
enum
{
	/*lisa1,*/		/* twiggy floppy drive */
	lisa2,		/* 3.5'' Sony floppy drive */
	lisa210,	/* modified I/O board, and internal 10Meg drive */
	mac_xl		/* same as above with modified video */
} lisa_model;

struct
{
	unsigned int has_fast_timers : 1;	/* I/O board VIAs are clocked at 1.25 MHz (?) instead of .5 MHz (?) (Lisa 2/10, Mac XL) */
										/* Note that the beep routine in boot ROMs implies that
										VIA clock is 1.25 times faster with fast timers than with
										slow timers.  I read the schematics again and again, and
										I simply don't understand : in one case the VIA is
										connected to the 68k E clock, which is CPUCK/10, and in
										another case, to a generated PH2 clock which is CPUCK/4,
										with additionnal logic to keep it in phase with the 68k
										memory cycle.  After hearing the beep when MacWorks XL
										boots, I bet the correct values are .625 MHz and .5 MHz.
										Maybe the schematics are wrong, and PH2 is CPUCK/8.
										Maybe the board uses a 6522 variant with different
										timings. */
	enum
	{
		twiggy,			/* twiggy drives (Lisa 1) */
		sony_lisa2,		/* 3.5'' drive with LisaLite adapter (Lisa 2) */
		sony_lisa210	/* 3.5'' drive with modified fdc hardware (Lisa 2/10, Mac XL) */
	} floppy_hardware;
	unsigned int has_double_sided_floppy : 1;	/* true on lisa 1 and *hacked* lisa 2/10 / Mac XL */
	unsigned int has_mac_xl_video : 1;	/* modified video for MacXL */
} lisa_features;

/*
	protos
*/

static READ16_HANDLER ( lisa_IO_r );
static WRITE16_HANDLER ( lisa_IO_w );


/*
	Interrupt handling
*/

static void lisa_field_interrupts(void)
{
	if (parity_error_pending)
		return;	/* don't touch anything... */

	/*if (RSIR)
		// serial interrupt
		cpunum_set_input_line_and_vector(0, M68K_IRQ_6, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else if (int0)
		// external interrupt
		cpunum_set_input_line_and_vector(0, M68K_IRQ_5, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else if (int1)
		// external interrupt
		cpunum_set_input_line_and_vector(0, M68K_IRQ_4, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else if (int2)
		// external interrupt
		cpunum_set_input_line_and_vector(0, M68K_IRQ_3, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else*/ if (KBIR)
		/* COPS VIA interrupt */
		cpunum_set_input_line_and_vector(0, M68K_IRQ_2, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else if (FDIR || VTIR)
		/* floppy disk or VBl */
		cpunum_set_input_line_and_vector(0, M68K_IRQ_1, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	else
		/* clear all interrupts */
		cpunum_set_input_line_and_vector(0, M68K_IRQ_1, CLEAR_LINE, M68K_INT_ACK_AUTOVECTOR);
}

static void set_parity_error_pending(int value)
{
#if 0
	/* does not work well due to bugs in 68k cores */
	parity_error_pending = value;
	if (parity_error_pending)
	{
		cpunum_set_input_line_and_vector(0, M68K_IRQ_7, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
	}
	else
	{
		cpunum_set_input_line(0, M68K_IRQ_7, CLEAR_LINE);
	}
#else
	/* work-around... */
	if ((! parity_error_pending) && value)
	{
		parity_error_pending = TRUE;
		cpunum_set_input_line_and_vector(0, M68K_IRQ_7, PULSE_LINE, M68K_INT_ACK_AUTOVECTOR);
	}
	else if (parity_error_pending && (! value))
	{
		parity_error_pending = FALSE;
		lisa_field_interrupts();
	}
#endif
}

INLINE void set_VTIR(int value)
{
	if (VTIR != value)
	{
		VTIR = value;
		lisa_field_interrupts();
	}
}



/*
	keyboard interface
*/

static int COPS_Ready;		/* COPS is about to read a command from VIA port A */

static int COPS_command;	/* keeps the data sent by the VIA to the COPS */

static int fifo_data[8];	/* 8-byte FIFO used by COPS to buffer data sent to VIA */
static int fifo_size;			/* current data in FIFO */
static int fifo_head;
static int fifo_tail;
static int mouse_data_offset;	/* current offset for mouse data in FIFO (!) */

static int COPS_force_unplug;		/* force unplug keyboard/mouse line for COPS (comes from VIA) */

static void *mouse_timer = NULL;	/* timer called for mouse setup */

static int hold_COPS_data;	/* mirrors the state of CA2 - COPS does not send data until it is low */

static int NMIcode;

/* clock registers */
static struct
{
	long alarm;		/* alarm (20-bit binary) */
	int years;		/* years (4-bit binary ) */
	int days1;		/* days (BCD : 1-366) */
	int days2;
	int days3;
	int hours1;		/* hours (BCD : 0-23) */
	int hours2;
	int minutes1;	/* minutes (BCD : 0-59) */
	int minutes2;
	int seconds1;	/* seconds (BCD : 0-59) */
	int seconds2;
	int tenths;		/* tenths of second (BCD : 0-9) */

	int clock_write_ptr;	/* clock byte to be written next (-1 if clock write disabled) */
	enum
	{
		clock_timer_disable = 0,
		timer_disable = 1,
		timer_interrupt = 2,	/* timer underflow generates interrupt */
		timer_power_on = 3		/* timer underflow turns system on if it is off and gens interrupt */
	} clock_mode;			/* clock mode */
} clock_regs;



INLINE void COPS_send_data_if_possible(void)
{
	if ((! hold_COPS_data) && fifo_size && (! COPS_Ready))
	{
		/*logerror("Pushing one byte of data to VIA\n");*/

		via_set_input_a(0, fifo_data[fifo_head]);	/* output data */
		if (fifo_head == mouse_data_offset)
			mouse_data_offset = -1;	/* we just phased out the mouse data in buffer */
		fifo_head = (fifo_head+1) & 0x7;
		fifo_size--;
		via_set_input_ca1(0, 1);		/* pulse ca1 so that VIA reads it */
		via_set_input_ca1(0, 0);		/* BTW, I have no idea how a real COPS does it ! */
	}
}

/* send data (queue it into the FIFO if needed) */
static void COPS_queue_data(UINT8 *data, int len)
{
#if 0
	if (fifo_size + len <= 8)
#else
	/* trash old data */
	while (fifo_size > 8 - len)
	{
		if (fifo_head == mouse_data_offset)
			mouse_data_offset = -1;	/* we just phased out the mouse data in buffer */
		fifo_head = (fifo_head+1) & 0x7;
		fifo_size--;
	}
#endif

	{
		/*logerror("Adding %d bytes of data to FIFO\n", len);*/

		while (len--)
		{
			fifo_data[fifo_tail] = * (data++);
			fifo_tail = (fifo_tail+1) & 0x7;
			fifo_size++;
		}

		/*logerror("COPS_queue_data : trying to send data to VIA\n");*/
		COPS_send_data_if_possible();
	}
}

/*
	scan_keyboard()

	scan the keyboard, and add key transition codes to buffer as needed
*/
/* shamelessly stolen from machine/mac.c :-) */

/* keyboard matrix to detect transition */
static int key_matrix[8];

static void scan_keyboard( void )
{
	int i, j;
	int keybuf;
	UINT8 keycode;

	if (! COPS_force_unplug)
		for (i=0; i<8; i++)
		{
			keybuf = readinputport(i+2);

			if (keybuf != key_matrix[i])
			{	/* if state has changed, find first bit which has changed */
				/*logerror("keyboard state changed, %d %X\n", i, keybuf);*/

				for (j=0; j<16; j++)
				{
					if (((keybuf ^ key_matrix[i]) >> j) & 1)
					{
						/* update key_matrix */
						key_matrix[i] = (key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));

						/* create key code */
						keycode = (i << 4) | j;
						if (keybuf & (1 << j))
						{	/* key down */
							keycode |= 0x80;
						}
#if 0
						if (keycode == NMIcode)
						{	/* generate NMI interrupt */
							cpunum_set_input_line(0, M68K_IRQ_7, PULSE_LINE);
							cpunum_set_input_line_vector(0, M68K_IRQ_7, M68K_INT_ACK_AUTOVECTOR);
						}
#endif
						COPS_queue_data(& keycode, 1);
					}
				}
			}
		}
}

/* handle mouse moves */
/* shamelessly stolen from machine/mac.c :-) */
static void handle_mouse(int unused)
{
	static int last_mx = 0, last_my = 0;

	int diff_x = 0, diff_y = 0;
	int new_mx, new_my;

	/*if (COPS_force_unplug)
		return;*/	/* ???? */

	new_mx = readinputport(0);
	new_my = readinputport(1);

	/* see if it moved in the x coord */
	if (new_mx != last_mx)
	{
		diff_x = new_mx - last_mx;

		/* check for wrap */
		if (diff_x > 0x80)
			diff_x = 0x100-diff_x;
		if  (diff_x < -0x80)
			diff_x = -0x100-diff_x;

		last_mx = new_mx;
	}
	/* see if it moved in the y coord */
	if (new_my != last_my)
	{
		diff_y = new_my - last_my;

		/* check for wrap */
		if (diff_y > 0x80)
			diff_y = 0x100-diff_y;
		if  (diff_y < -0x80)
			diff_y = -0x100-diff_y;

		last_my = new_my;
	}

	/* update any remaining count and then return */
	if (diff_x || diff_y)
	{
		if (mouse_data_offset != -1)
		{
			fifo_data[mouse_data_offset] += diff_x;
			fifo_data[(mouse_data_offset+1) & 0x7] += diff_y;
		}
		else
		{
#if 0
			if (fifo_size <= 5)
#else
			/* trash old data */
			while (fifo_size > 5)
			{
				fifo_head = (fifo_head+1) & 0x7;
				fifo_size--;
			}
#endif

			{
				/*logerror("Adding 3 bytes of mouse data to FIFO\n");*/

				fifo_data[fifo_tail] = 0;
				mouse_data_offset = fifo_tail = (fifo_tail+1) & 0x7;
				fifo_data[fifo_tail] = diff_x;
				fifo_tail = (fifo_tail+1) & 0x7;
				fifo_data[fifo_tail] = diff_y;
				fifo_tail = (fifo_tail+1) & 0x7;
				fifo_size += 3;

				/*logerror("handle_mouse : trying to send data to VIA\n");*/
				COPS_send_data_if_possible();
			}
			/* else, mouse data is lost forever (correct ??) */
		}
	}
}

/* read command from the VIA port A */
static void read_COPS_command(int unused)
{
	int command;

	COPS_Ready = FALSE;

	/*logerror("read_COPS_command : trying to send data to VIA\n");*/
	COPS_send_data_if_possible();

	/* some pull-ups allow the COPS to read 1s when the VIA port is not set as output */
	command = (COPS_command | (~ via_read(0, VIA_DDRA))) & 0xff;

	if (command & 0x80)
		return;	/* NOP */

	if (command & 0xF0)
	{	/* commands with 4-bit immediate operand */
		int immediate = command & 0xf;

		switch ((command & 0xF0) >> 4)
		{
		case 0x1:	/* write clock data */
			if (clock_regs.clock_write_ptr != -1)
			{
				switch (clock_regs.clock_write_ptr)
				{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
					/* alarm */
					clock_regs.alarm &= ~ (0xf << (4 * (4 - clock_regs.clock_write_ptr)));
					clock_regs.alarm |= immediate << (4 * (4 - clock_regs.clock_write_ptr));
					break;
				case 5:
					/* year */
					clock_regs.years = immediate;
					break;
				case 6:
					/* day */
					clock_regs.days1 = immediate;
					break;
				case 7:
					/* day */
					clock_regs.days2 = immediate;
					break;
				case 8:
					/* day */
					clock_regs.days3 = immediate;
					break;
				case 9:
					/* hours */
					clock_regs.hours1 = immediate;
					break;
				case 10:
					/* hours */
					clock_regs.hours2 = immediate;
					break;
				case 11:
					/* minutes */
					clock_regs.minutes1 = immediate;
					break;
				case 12:
					/* minutes */
					clock_regs.minutes1 = immediate;
					break;
				case 13:
					/* seconds */
					clock_regs.seconds1 = immediate;
					break;
				case 14:
					/* seconds */
					clock_regs.seconds2 = immediate;
					break;
				case 15:
					/* tenth */
					clock_regs.tenths = immediate;
					break;
				}
				clock_regs.clock_write_ptr++;
				if (clock_regs.clock_write_ptr == 16)
					clock_regs.clock_write_ptr = -1;
			}

			break;

		case 0x2:	/* set clock mode */
			if (immediate & 0x8)
			{	/* start setting the clock */
				clock_regs.clock_write_ptr = 0;
			}
			else
			{	/* clock write disabled */
				clock_regs.clock_write_ptr = -1;
			}

			if (! (immediate & 0x4))
			{	/* enter sleep mode */
				/* ... */
			}
			else
			{	/* wake up */
				/* should never happen */
			}

			clock_regs.clock_mode = immediate & 0x3;
			break;

#if 0
		/* LED commands - not implemented in production LISAs */
		case 0x3:	/* write 4 keyboard LEDs */
			keyboard_leds = (keyboard_leds & 0x0f) | (immediate << 4);
			break;

		case 0x4:	/* write next 4 keyboard LEDs */
			keyboard_leds = (keyboard_leds & 0xf0) | immediate;
			break;
#endif

		case 0x5:	/* set high nibble of NMI character to nnnn */
			NMIcode = (NMIcode & 0x0f) | (immediate << 4);
			break;

		case 0x6:	/* set low nibble of NMI character to nnnn */
			NMIcode = (NMIcode & 0xf0) | immediate;
			break;

		case 0x7:	/* send mouse command */
			if (immediate & 0x8)
				timer_adjust(mouse_timer, 0, 0, TIME_IN_MSEC((immediate & 0x7)*4)); /* enable mouse */
			else
				timer_reset(mouse_timer, TIME_NEVER);
			break;
		}
	}
	else
	{	/* operand-less commands */
		switch (command)
		{
		case 0x0:	/*Turn I/O port on (???) */

			break;

		case 0x1:	/*Turn I/O port off (???) */

			break;

		case 0x2:	/* Read clock data */
			{
				/* format and send reply */

				UINT8 reply[7];

				reply[0] = 0x80;
				reply[1] = 0xE0 | clock_regs.years;
				reply[2] = (clock_regs.days1 << 4) | clock_regs.days2;
				reply[3] = (clock_regs.days3 << 4) | clock_regs.hours1;
				reply[4] = (clock_regs.hours2 << 4) | clock_regs.minutes1;
				reply[5] = (clock_regs.minutes2 << 4) | clock_regs.seconds1;
				reply[6] = (clock_regs.seconds2 << 4) | clock_regs.tenths;

				COPS_queue_data(reply, 7);
			}
			break;
		}
	}
}

/* this timer callback raises the COPS Ready line, which tells the COPS is about to read a command */
static void set_COPS_ready(int unused)
{
	COPS_Ready = TRUE;

	/* impulsion width : +/- 20us */
	timer_set(TIME_IN_USEC(20), 0, read_COPS_command);
}

static void reset_COPS(void)
{
	int i;

	fifo_size = 0;
	fifo_head = 0;
	fifo_tail = 0;
	mouse_data_offset = -1;

	for (i=0; i<8; i++)
		key_matrix[i] = 0;

	timer_reset(mouse_timer, TIME_NEVER);
}

static void unplug_keyboard(void)
{
	UINT8 cmd[2] =
	{
		0x80,	/* RESET code */
		0xFD	/* keyboard unplugged */
	};

	COPS_queue_data(cmd, 2);
}


static void plug_keyboard(void)
{
	/*
		possible keyboard IDs according to Lisa Hardware Manual and boot ROM source code

		2 MSBs : "mfg code" (-> 0x80 for Keytronics, 0x00 for "APD")
		6 LSBs :
			0x0x : "old US keyboard"
			0x3f : US keyboard
			0x3d : Canadian keyboard
			0x2f : UK
			0x2e : German
			0x2d : French
			0x27 : Swiss-French
			0x26 : Swiss-German
			unknown : spanish, US dvorak, italian & swedish
	*/

	UINT8 cmd[2] =
	{
		0x80,	/* RESET code */
		0x3f	/* keyboard ID - US for now */
	};

	COPS_queue_data(cmd, 2);
}


/* called at power-up */
static void init_COPS(void)
{
	COPS_Ready = FALSE;

	/* read command every ms (don't know the real value) */
	timer_pulse(TIME_IN_MSEC(1), 0, set_COPS_ready);

	reset_COPS();
}



/* VIA1 accessors (COPS, sound, and 2 hard disk lines) */

/*
	PA0-7 (I/O) : VIA <-> COPS data bus
	CA1 (I) : COPS sending valid data
	CA2 (O) : VIA -> COPS handshake
*/
static WRITE8_HANDLER(COPS_via_out_a)
{
	COPS_command = data;
}

static WRITE8_HANDLER(COPS_via_out_ca2)
{
	hold_COPS_data = data;

	/*logerror("COPS CA2 line state : %d\n", val);*/

	/*logerror("COPS_via_out_ca2 : trying to send data to VIA\n");*/
	COPS_send_data_if_possible();
}

/*
	PB7 (O) : CR* ("Controller Reset", used by hard disk interface) ???
	PB6 (I) : CRDY ("COPS ready") : set low by the COPS for 20us when it is reading a command
		from the data bus (command latched at low-to-high transition)
	PB5 (I/O) : PR* ; as output : "parity error latch reset" (only when CR* and RESET* are
		inactive) ; as input : low when CR* or RESET are low.
	PB4 (I) : FDIR (floppy disk interrupt request - the fdc shared RAM should not be accessed
		unless this bit is 1)
	PB1-3 (O) : sound volume
	PB0 (O) : forces disconnection of keyboard and mouse (allows to retrive keyboard ID, etc.)

	CB1 : not used
	CB2 (O) : sound output
*/
static  READ8_HANDLER(COPS_via_in_b)
{
	int val = 0;

	if (! COPS_Ready)
		val |= 0x40;

	if (FDIR)
		val |= 0x10;

	return val;
}

static WRITE8_HANDLER(COPS_via_out_b)
{
	/* pull-up */
	data |= (~ via_read(0, VIA_DDRA)) & 0x01;

	if (data & 0x01)
	{
		if (COPS_force_unplug)
		{
			COPS_force_unplug = FALSE;
			plug_keyboard();
		}
	}
	else
	{
		if (! COPS_force_unplug)
		{
			COPS_force_unplug = TRUE;
			unplug_keyboard();
			//reset_COPS();
		}
	}
}

static WRITE8_HANDLER(COPS_via_out_cb2)
{
	speaker_level_w(0, data);
}

static void COPS_via_irq_func(int val)
{
	if (KBIR != val)
	{
		KBIR = val;
		lisa_field_interrupts();
	}
}

/* VIA2 accessors (hard disk, and a few floppy disk lines) */

/*
	PA0-7 (I/O) : VIA <-> hard disk data bus (cf PB3)
	CA1 (I) : hard disk BSY line
	CA2 (O) : hard disk PSTRB* line
*/

/*
	PB7 (O) : WCNT line : set contrast latch on low-to-high transition ?
	PB6 (I) : floppy disk DISK DIAG line
	PB5 (I) : hard disk data DD0-7 current parity (does not depend on PB2)
	PB4 (O) : hard disk CMD* line
	PB3 (O) : hard disk DR/W* line ; controls the direction of the drivers on the data bus
	PB2 (O) : when low, disables hard disk interface drivers
	PB1 (I) : hard disk BSY line
	PB0 (I) : hard disk OCD (Open Cable Detect) line : 0 when hard disk attached
	CB1 : not used
	CB2 (I) : current parity latch value
*/
static  READ8_HANDLER(parallel_via_in_b)
{
	int val = 0;

	if (DISK_DIAG)
		val |= 0x40;

	/* tell there is no hard disk : */
	val |= 0x1;

	/* keep busy high to work around a bug??? */
	//val |= 0x2;

	return val;
}






/*
	LISA video emulation
*/

VIDEO_START( lisa )
{
	return 0;
}

/*
	Video update
*/
VIDEO_UPDATE( lisa )
{
	UINT16 *v;
	int x, y;
	/* resolution is 720*364 on lisa, vs 608*431 on mac XL */
	int resx = (lisa_features.has_mac_xl_video) ? 608 : 720;	/* width */
	int resy = (lisa_features.has_mac_xl_video) ? 431 : 364;	/* height */

	UINT8 line_buffer[720];

	v = videoram_ptr;

	for (y = 0; y < resy; y++)
	{
		for (x = 0; x < resx; x++)
			line_buffer[x] = (v[(x+y*resx)>>4] & (0x8000 >> ((x+y*resx) & 0xf))) ? 1 : 0;
		draw_scanline8(bitmap, 0, y, resx, line_buffer, Machine->pens, -1);
	}
	return 0;
}


static OPBASE_HANDLER (lisa_OPbaseoverride)
{
	/* upper 7 bits -> segment # */
	int segment = (address >> 17) & 0x7f;

	int the_seg = seg;

	address &= 0xffffff;
	/*logerror("logical address%lX\n", address);*/


	if (setup)
	{
		if (address & 0x004000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (address & 0x008000)
			{	/* MMU register : BUS error ??? */
				logerror("illegal opbase address%lX\n", (long) address);
			}
			else
			{	/* system ROMs */
				opcode_mask = 0xffffff;
				opcode_base = opcode_arg_base = lisa_rom_ptr - (address & 0xffc000);
				opcode_memory_min = (address & 0xffc000);
				opcode_memory_max = (address & 0xffc000) + 0x003fff;
				/*logerror("ROM (setup mode)\n");*/
			}

			return -1;
		}

	}

	if (cpunum_get_reg(0, M68K_SR) & 0x2000)
		/* supervisor mode -> force register file 0 */
		the_seg = 0;

	{
		int seg_offset = address & 0x01ffff;

		/* add revelant origin -> address */
		offs_t mapped_address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_r:
		case RAM_rw:
			if (seg_offset > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */
				logerror("illegal opbase address%lX\n", (long) address);
			}
			opcode_mask = 0xffffff;
			opcode_base = opcode_arg_base = lisa_ram_ptr + mapped_address - address;
			opcode_memory_min = (address & 0xffc000);
			opcode_memory_max = (address & 0xffc000) + 0x003fff;
			/*logerror("RAM\n");*/
			break;

		case RAM_stack_r:
		case RAM_stack_rw:	/* stack : bus error ??? */
		case IO:			/* I/O : bus error ??? */
		case invalid:		/* unmapped segment */
			/* bus error */
			logerror("illegal opbase address%lX\n", (long) address);
			break;

		case special_IO:
			opcode_mask = 0xffffff;
			opcode_base = opcode_arg_base = lisa_rom_ptr + (mapped_address & 0x003fff) - address;
			opcode_memory_min = (address & 0xffc000);
			opcode_memory_max = (address & 0xffc000) + 0x003fff;
			/*logerror("ROM\n");*/
			break;
		}
	}


	/*logerror("resulting offset%lX\n", answer);*/

	return -1;
}

static OPBASE_HANDLER (lisa_fdc_OPbaseoverride)
{
	/* 8kb of address space -> wraparound */
	return (address & 0x1fff);
}


/* should save PRAM to file */
/* TODO : save time difference with host clock, set default date, etc */
NVRAM_HANDLER(lisa)
{
	if (read_or_write)
	{
		mame_fwrite(file, lisa_fdc_ram, 1024);
	}
	else
	{
		if (file)
			mame_fread(file, lisa_fdc_ram, 1024);
		else
			memset(lisa_fdc_ram, 0, 1024);

		{
			/* Now we copy the host clock into the Lisa clock */
			mame_system_time systime;
			mame_get_base_datetime(Machine, &systime);

			clock_regs.alarm = 0xfffffL;
			/* The clock count starts on 1st January 1980 */
			clock_regs.years = (systime.local_time.year - 1980) & 0xf;
			clock_regs.days1 = (systime.local_time.day + 1) / 100;
			clock_regs.days2 = ((systime.local_time.day + 1) / 10) % 10;
			clock_regs.days3 = (systime.local_time.day + 1) % 10;
			clock_regs.hours1 = systime.local_time.hour / 10;
			clock_regs.hours2 = systime.local_time.hour % 10;
			clock_regs.minutes1 = systime.local_time.minute / 10;
			clock_regs.minutes2 = systime.local_time.minute % 10;
			clock_regs.seconds1 = systime.local_time.second / 10;
			clock_regs.seconds2 = systime.local_time.second % 10;
			clock_regs.tenths = 0;
		}
		clock_regs.clock_mode = timer_disable;
		clock_regs.clock_write_ptr = -1;
	}


#if 0
	UINT32 temp32;
	SINT8 temp8;
	temp32 = (clock_regs.alarm << 12) | (clock_regs.years << 8) | (clock_regs.days1 << 4)
	        | clock_regs.days2;

	temp32 = (clock_regs.days3 << 28) | (clock_regs.hours1 << 24) | (clock_regs.hours2 << 20)
	        | (clock_regs.minutes1 << 16) | (clock_regs.minutes2 << 12)
	        | (clock_regs.seconds1 << 8) | (clock_regs.seconds2 << 4) | clock_regs.tenths;

	temp8 = clock_mode;			/* clock mode */

	temp8 = clock_regs.clock_write_ptr;	/* clock byte to be written next (-1 if clock write disabled) */
#endif
}

/*void init_lisa1(void)
{
	lisa_model = lisa1;
	lisa_features.has_fast_timers = 0;
	lisa_features.floppy_hardware = twiggy;
	lisa_features.has_double_sided_floppy = 1;
	lisa_features.has_mac_xl_video = 0;
}*/

DRIVER_INIT( lisa2 )
{
	lisa_model = lisa2;
	lisa_features.has_fast_timers = 0;
	lisa_features.floppy_hardware = sony_lisa2;
	lisa_features.has_double_sided_floppy = 0;
	lisa_features.has_mac_xl_video = 0;
}

DRIVER_INIT( lisa210 )
{
	lisa_model = lisa210;
	lisa_features.has_fast_timers = 1;
	lisa_features.floppy_hardware = sony_lisa210;
	lisa_features.has_double_sided_floppy = 0;
	lisa_features.has_mac_xl_video = 0;
}

DRIVER_INIT( mac_xl )
{
	lisa_model = mac_xl;
	lisa_features.has_fast_timers = 1;
	lisa_features.floppy_hardware = sony_lisa210;
	lisa_features.has_double_sided_floppy = 0;
	lisa_features.has_mac_xl_video = 1;
}

static void lisa2_set_iwm_enable_lines(int enable_mask)
{
	/* E1 & E2 is connected to the Sony SEL line (?) */
	/*logerror("new sel line state %d\n", (enable_mask) ? 0 : 1);*/
	sony_set_sel_line((enable_mask) ? 0 : 1);
}

static void lisa210_set_iwm_enable_lines(int enable_mask)
{
	/* E2 is connected to the Sony enable line (?) */
	sony_set_enable_lines(enable_mask >> 1);
}

MACHINE_RESET( lisa )
{
	mouse_timer = timer_alloc(handle_mouse);

	lisa_ram_ptr = memory_region(REGION_CPU1) + RAM_OFFSET;
	lisa_rom_ptr = memory_region(REGION_CPU1) + ROM_OFFSET;

	videoROM_ptr = memory_region(REGION_GFX1);

	memory_set_opbase_handler(0, lisa_OPbaseoverride);
	memory_set_opbase_handler(1, lisa_fdc_OPbaseoverride);

	cpunum_set_info_fct(0, CPUINFO_PTR_M68K_RESET_CALLBACK, (genf *)/*lisa_reset_instr_callback*/NULL);

	/* init MMU */

	setup = TRUE;

	seg = 0;

	/* init parity */

	diag2 = FALSE;
	test_parity = FALSE;
	parity_error_pending = FALSE;

	bad_parity_count = 0;
	bad_parity_table = memory_region(REGION_USER1);
	memset(bad_parity_table, 0, memory_region_length(REGION_USER1));	/* Clear */

	/* init video */

	VTMSK = FALSE;
	set_VTIR(FALSE);

	video_address_latch = 0;
	videoram_ptr = (UINT16 *) lisa_ram_ptr;

	/* reset COPS keyboard/mouse controller */
	init_COPS();

	/* configure vias */
	via_config(0, & lisa_via6522_intf[0]);
	via_set_clock(0, lisa_features.has_fast_timers ? 1250000 : 500000);	/* one of these values must be wrong : see note after description of the has_fast_timers field */
	via_config(1, & lisa_via6522_intf[1]);
	via_set_clock(1, lisa_features.has_fast_timers ? 1250000 : 500000);	/* one of these values must be wrong : see note after description of the has_fast_timers field */

	via_reset();
	COPS_via_out_ca2(0, 0);	/* VIA core forgets to do so */

	/* initialize floppy */
	{
		struct applefdc_interface intf =
		{
			APPLEFDC_IWM,
			sony_set_lines,
			sony_set_enable_lines,

			sony_read_data,
			sony_write_data,
			sony_read_status
		};

		if (lisa_features.floppy_hardware == sony_lisa2)
		{
			intf.set_enable_lines = lisa2_set_iwm_enable_lines;
		}
		if (lisa_features.floppy_hardware == sony_lisa210)
		{
			intf.set_enable_lines = lisa210_set_iwm_enable_lines;
		}

		applefdc_init(& intf);

		if (lisa_features.floppy_hardware == sony_lisa2)
			sony_set_enable_lines(1);	/* on lisa2, drive unit 1 is always selected (?) */
	}
}

void lisa_interrupt(void)
{
	static int frame_count = 0;

	if ((++frame_count) == 6)
	{	/* increment clock every 1/10s */
		frame_count = 0;

		if (clock_regs.clock_mode != clock_timer_disable)
		{
			if ((++clock_regs.tenths) == 10)
			{
				clock_regs.tenths = 0;

				if (clock_regs.clock_mode != timer_disable)
				{
					if (clock_regs.alarm == 0)
					{
						/* generate reset (should cause a VIA interrupt...) */
						UINT8 cmd[2] =
						{
							0x80,	/* RESET code */
							0xFC	/* timer time-out */
						};
						COPS_queue_data(cmd, 2);

						clock_regs.alarm = 0xfffffL;
					}
					else
					{
						clock_regs.alarm--;
					}
				}

				if ((++clock_regs.seconds2) == 10)
				{
					clock_regs.seconds2 = 0;

					if ((++clock_regs.seconds1) == 6)
					{
						clock_regs.seconds1 = 0;

						if ((++clock_regs.minutes2) == 10)
						{
							clock_regs.minutes2 = 0;

							if ((++clock_regs.minutes1) == 6)
							{
								clock_regs.minutes1 = 0;

								if ((++clock_regs.hours2) == 10)
								{
									clock_regs.hours2 = 0;

									clock_regs.hours1++;
								}

								if ((clock_regs.hours1*10 + clock_regs.hours2) == 24)
								{
									clock_regs.hours1 = clock_regs.hours2 = 0;

									if ((++clock_regs.days3) == 10)
									{
										clock_regs.days3 = 0;

										if ((++clock_regs.days2) == 10)
										{
											clock_regs.days2 = 0;

											clock_regs.days1++;
										}
									}

									if ((clock_regs.days1*100 + clock_regs.days2*10 + clock_regs.days3) ==
										((clock_regs.years % 4) ? 366 : 367))
									{
										clock_regs.days1 = clock_regs.days2 = clock_regs.days3 = 0;

										clock_regs.years = (clock_regs.years + 1) & 0xf;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	/* set VBI */
	if (VTMSK)
		set_VTIR(TRUE);

	/* do keyboard scan */
	scan_keyboard();
}

/*
	Lots of fun with the Lisa fdc hardware

	The iwm floppy select line is connected to the drive SEL line in Lisa2 (which is why Lisa 2
	cannot support 2 floppy drives)...
*/

INLINE void lisa_fdc_ttl_glue_access(offs_t offset)
{
	switch ((offset & 0x000E) >> 1)
	{
	case 0:
		/*stop = offset & 1;*/	/* stop/run motor pulse generation */
		break;
	case 2:
		/*MT0 = offset & 1;*/	/* ???? */
		break;
	case 3:
		/* enable/disable the motor on Lisa 1 */
		/* can disable the motor on Lisa 2/10, too (although it is not useful) */
		/* On lisa 2, commands the loading of the speed register on lisalite board */
		if (lisa_features.floppy_hardware == sony_lisa2)
		{
			int oldMT1 = MT1;
			MT1 = offset & 1;
			if (MT1 && ! oldMT1)
			{
				PWM_floppy_motor_speed = (PWM_floppy_motor_speed << 1) & 0xff;
				if (applefdc_get_lines() & APPLEFDC_PH0)
					PWM_floppy_motor_speed |= 1;
				sony_set_speed(((256-PWM_floppy_motor_speed) * 1.3) + 237);
			}
		}
		/*else
			MT1 = offset & 1;*/
		break;
	case 4:
		/*DIS = offset & 1;*/	/* forbids access from the 68000 to our RAM */
		break;
	case 5:
		/*HDS = offset & 1;*/		/* head select (-> disk side) on twiggy */
		/*if (lisa_features.floppy_hardware == twiggy)
			twiggy_set_head_line(offset & 1);
		else*/ if (lisa_features.floppy_hardware == sony_lisa210)
			sony_set_sel_line(offset & 1);
		break;
	case 6:
		DISK_DIAG = offset & 1;
		break;
	case 7:
		FDIR = offset & 1;	/* Interrupt request to 68k */
		lisa_field_interrupts();
		break;
	}
}

 READ8_HANDLER ( lisa_fdc_io_r )
{
	int answer=0;

	switch ((offset & 0x0030) >> 4)
	{
	case 0:	/* IWM */
		answer = /*iwm_lisa_r*/applefdc_r(offset);
		break;

	case 1:	/* TTL glue */
		lisa_fdc_ttl_glue_access(offset);
		answer = 0;	/* ??? */
		break;

	case 2:	/* pulses the PWM LOAD line (bug!) */
		answer = 0;	/* ??? */
		break;

	case 3:	/* not used */
		answer = 0;	/* ??? */
		break;
	}

	return answer;
}

WRITE8_HANDLER ( lisa_fdc_io_w )
{
	switch ((offset & 0x0030) >> 4)
	{
	case 0:	/* IWM */
		/*iwm_lisa_w*/applefdc_w(offset, data);
		break;

	case 1:	/* TTL glue */
		lisa_fdc_ttl_glue_access(offset);
		break;

	case 2:	/* writes the PWM register */
		/* the written value is used to generate the motor speed control signal */
		/*if (lisa_features.floppy_hardware == twiggy)
			twiggy_set_speed((256-data) * ??? + ???);
		else*/ if (lisa_features.floppy_hardware == sony_lisa210)
			sony_set_speed(((256-data) * 1.3) + 237);
		break;

	case 3:	/* not used */
		break;
	}
}

READ8_HANDLER ( lisa_fdc_r )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0800))
			if (! (offset & 0x0400))
				return lisa_fdc_ram[offset & 0x03ff];
			else
				return lisa_fdc_io_r(offset & 0x03ff);
		else
			return 0;	/* ??? */
	}
	else
		return lisa_fdc_rom[offset & 0x0fff];
}

READ8_HANDLER ( lisa210_fdc_r )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0400))
			if (! (offset & 0x0800))
				return lisa_fdc_ram[offset & 0x03ff];
			else
				return lisa_fdc_io_r(offset & 0x03ff);
		else
			return 0;	/* ??? */
	}
	else
		return lisa_fdc_rom[offset & 0x0fff];
}

WRITE8_HANDLER ( lisa_fdc_w )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0800))
		{
			if (! (offset & 0x0400))
				lisa_fdc_ram[offset & 0x03ff] = data;
			else
				lisa_fdc_io_w(offset & 0x03ff, data);
		}
	}
}

WRITE8_HANDLER ( lisa210_fdc_w )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0400))
		{
			if (! (offset & 0x0800))
				lisa_fdc_ram[offset & 0x03ff] = data;
			else
				lisa_fdc_io_w(offset & 0x03ff, data);
		}
	}
}

READ16_HANDLER ( lisa_r )
{
	int answer=0;

	/* segment register set */
	int the_seg = seg;

	/* upper 7 bits -> segment # */
	int segment = (offset >> 16) & 0x7f;


	/*logerror("read, logical address%lX\n", offset);*/

	if (setup)
	{	/* special setup mode */
		if (offset & 0x002000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (offset & 0x004000)
			{	/* read MMU register */
				/*logerror("read from segment registers (%X:%X) ", the_seg, segment);*/
				if (offset & 0x000004)
				{	/* sorg register */
					answer = real_mmu_regs[the_seg][segment].sorg;
					/*logerror("sorg, data = %X\n", answer);*/
				}
				else
				{	/* slim register */
					answer = real_mmu_regs[the_seg][segment].slim;
					/*logerror("slim, data = %X\n", answer);*/
				}
			}
			else
			{	/* system ROMs */
				answer = ((UINT16*)lisa_rom_ptr)[offset & 0x001fff];

				/*logerror("dst address in ROM (setup mode)\n");*/
			}

			return answer;
		}
	}

	if (cpunum_get_reg(0, M68K_SR) & 0x2000)
		/* supervisor mode -> force register file 0 */
		the_seg = 0;

	{
		/* offset in segment */
		int seg_offset = (offset & 0x00ffff) << 1;

		/* add revelant origin -> address */
		offs_t address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		/*logerror("read, logical address%lX\n", offset);
		logerror("physical address%lX\n", address);*/

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_stack_r:
		case RAM_stack_rw:
			if (address <= mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			answer = *(UINT16 *)(lisa_ram_ptr + address);

			if (bad_parity_count && test_parity
					&& (bad_parity_table[address >> 3] & (0x3 << (address & 0x7))))
			{
				mem_err_addr_latch = address >> 5;
				set_parity_error_pending(TRUE);
			}
			break;

		case RAM_r:
		case RAM_rw:
			if (address > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			answer = *(UINT16 *)(lisa_ram_ptr + address);

			if (bad_parity_count && test_parity
					&& (bad_parity_table[address >> 3] & (0x3 << (address & 0x7))))
			{
				mem_err_addr_latch = address >> 5;
				set_parity_error_pending(TRUE);
			}
			break;

		case IO:
			answer = lisa_IO_r((address & 0x00ffff) >> 1, mem_mask);
			break;

		case invalid:		/* unmapped segment */
			/* bus error */

			answer = 0;
			break;

		case special_IO:
			if (! (address & 0x008000))
				answer = *(UINT16 *)(lisa_rom_ptr + (address & 0x003fff));
			else
			{	/* read serial number from ROM */
				/* this has to be be the least efficient way to read a ROM :-) */
				/* this emulation is not guaranteed accurate */

				/* problem : due to collisions with video, timings of the LISA CPU
				are slightly different from timings of a bare 68k */
				/* so we use a kludge... */
#if 0
				/* theory - probably partially wrong, anyway */
				int time_in_frame = cpu_getcurrentcycles();
				int videoROM_address;

				videoROM_address = (time_in_frame / 4) & 0x7f;

				if ((time_in_frame >= 70000) || (time_in_frame <= 74000))	/* these values are approximative */
				{	/* if VSyncing, read ROM 2nd half ? */
					videoROM_address |= 0x80;
				}
#else
				/* kludge */
				/* this code assumes the program always tries to read consecutive bytes */
#if 0
				int time_in_frame = cpu_getcurrentcycles();
				static int videoROM_address = 0;

				videoROM_address = (videoROM_address + 1) & 0x7f;
				/* the BOOT ROM only reads 56 bits, so there must be some wrap-around for
				videoROM_address <= 56 */
				/* pixel clock 20MHz, memory access rate 1.25MHz, horizontal clock 22.7kHz
				according to Apple, which must stand for 1.25MHz/55 = 22727kHz, vertical
				clock approximately 60Hz, which means there are about 380 lines, including VBlank */
				if (videoROM_address == 55)
					videoROM_address = 0;

				if ((time_in_frame >= 70000) && (time_in_frame <= 74000))	/* these values are approximative */
				{	/* if VSyncing, read ROM 2nd half ? */
					videoROM_address |= 0x80;
				}
#else
				int time_in_frame = cpu_getscanline();
				static int videoROM_address = 0;

				videoROM_address = (videoROM_address + 1) & 0x7f;
				/* the BOOT ROM only reads 56 bits, so there must be some wrap-around for
				videoROM_address <= 56 */
				/* pixel clock 20MHz, memory access rate 1.25MHz, horizontal clock 22.7kHz
				according to Apple, which must stand for 1.25MHz/55 = 22727kHz, vertical
				clock approximately 60Hz, which means there are about 380 lines, including VBlank */
				/* The values are different on the Mac XL, and I don't know the correct values
				for sure. */
				if (videoROM_address == ((lisa_features.has_mac_xl_video) ? 48/* ??? */ : 55))
					videoROM_address = 0;

				/* Something appears to be wrong with the timings, since we expect to read the
				2nd half when v-syncing, i.e. for lines beyond the 431th or 364th one (provided
				there are no additionnal margins).
				This is caused by the fact that 68k timings are wrong (memory accesses are
				interlaced with the video hardware, which is not emulated). */
				if (lisa_features.has_mac_xl_video)
				{
					if ((time_in_frame >= 374) && (time_in_frame <= 392))	/* these values have not been tested */
					{	/* if VSyncing, read ROM 2nd half ? */
						videoROM_address |= 0x80;
					}
				}
				else
				{
					if ((time_in_frame >= 319) && (time_in_frame <= 338))	/* these values are approximative */
					{	/* if VSyncing, read ROM 2nd half ? */
						videoROM_address |= 0x80;
					}
				}
#endif

#endif

				answer = videoROM_ptr[videoROM_address] << 8;

				/*logerror("%X %X\n", videoROM_address, answer);*/
			}
			break;
		}
	}

	/*logerror("result %X\n", answer);*/

	return answer;
}

WRITE16_HANDLER ( lisa_w )
{
	/* segment register set */
	int the_seg = seg;

	/* upper 7 bits -> segment # */
	int segment = (offset >> 16) & 0x7f;


	if (setup)
	{
		if (offset & 0x002000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (offset & 0x004000)
			{	/* write to MMU register */
				/*logerror("write to segment registers (%X:%X) ", the_seg, segment);*/
				if (offset & 0x000004)
				{	/* sorg register */
					/*logerror("sorg, data = %X\n", data);*/
					real_mmu_regs[the_seg][segment].sorg = data;
					mmu_regs[the_seg][segment].sorg = (data & 0x0fff) << 9;
				}
				else
				{	/* slim register */
					/*logerror("slim, data = %X\n", data);*/
					real_mmu_regs[the_seg][segment].slim = data;
					mmu_regs[the_seg][segment].slim = (~ (data << 9)) & 0x01ffff;
					switch ((data & 0x0f00) >> 8)
					{
					case 0x4:
						/*logerror("type : RAM stack r\n");*/
						mmu_regs[the_seg][segment].type = RAM_stack_r;
						break;
					case 0x5:
						/*logerror("type : RAM r\n");*/
						mmu_regs[the_seg][segment].type = RAM_r;
						break;
					case 0x6:
						/*logerror("type : RAM stack rw\n");*/
						mmu_regs[the_seg][segment].type = RAM_stack_rw;
						break;
					case 0x7:
						/*logerror("type : RAM rw\n");*/
						mmu_regs[the_seg][segment].type = RAM_rw;
						break;
					case 0x8:
					case 0x9:	/* not documented, but used by ROMs (?) */
						/*logerror("type : I/O\n");*/
						mmu_regs[the_seg][segment].type = IO;
						break;
					case 0xC:
						/*logerror("type : invalid\n");*/
						mmu_regs[the_seg][segment].type = invalid;
						break;
					case 0xF:
						/*logerror("type : special I/O\n");*/
						mmu_regs[the_seg][segment].type = special_IO;
						break;
					default:	/* "unpredictable results" */
						/*logerror("type : unknown\n");*/
						mmu_regs[the_seg][segment].type = invalid;
						break;
					}
				}
			}
			else
			{	/* system ROMs : read-only ??? */
				/* bus error ??? */
			}
			return;
		}
	}

	if (cpunum_get_reg(0, M68K_SR) & 0x2000)
		/* supervisor mode -> force register file 0 */
		the_seg = 0;

	{
		/* offset in segment */
		int seg_offset = (offset & 0x00ffff) << 1;

		/* add revelant origin -> address */
		offs_t address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_stack_rw:
			if (address <= mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			COMBINE_DATA((UINT16 *) (lisa_ram_ptr + address));
			if (diag2)
			{
				if ((ACCESSING_LSB)
					&& ! (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] |= 0x1 << (address & 0x7);
					bad_parity_count++;
				}
				if ((ACCESSING_MSB)
					&& ! (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] |= 0x2 << (address & 0x7);
					bad_parity_count++;
				}
			}
			else if (bad_parity_table[address >> 3] & (0x3 << (address & 0x7)))
			{
				if ((ACCESSING_LSB)
					&& (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x1 << (address & 0x7));
					bad_parity_count--;
				}
				if ((ACCESSING_MSB)
					&& (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x2 << (address & 0x7));
					bad_parity_count--;
				}
			}
			break;

		case RAM_rw:
			if (address > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			COMBINE_DATA((UINT16 *) (lisa_ram_ptr + address));
			if (diag2)
			{
				if ((ACCESSING_LSB)
					&& ! (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] |= 0x1 << (address & 0x7);
					bad_parity_count++;
				}
				if ((ACCESSING_MSB)
					&& ! (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] |= 0x2 << (address & 0x7);
					bad_parity_count++;
				}
			}
			else if (bad_parity_table[address >> 3] & (0x3 << (address & 0x7)))
			{
				if ((ACCESSING_LSB)
					&& (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x1 << (address & 0x7));
					bad_parity_count--;
				}
				if ((ACCESSING_MSB)
					&& (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x2 << (address & 0x7));
					bad_parity_count--;
				}
			}
			break;

		case IO:
			lisa_IO_w((address & 0x00ffff) >> 1, data, mem_mask);
			break;

		case RAM_stack_r:	/* read-only */
		case RAM_r:			/* read-only */
		case special_IO:	/* system ROMs : read-only ??? */
		case invalid:		/* unmapped segment */
			/* bus error */

			break;
		}
	}
}


/**************************************************************************************\
* I/O Slot Memory                                                                      *
*                                                                                      *
* 000000 - 001FFF Slot 0 Low  Decode                                                   *
* 002000 - 003FFF Slot 0 High Decode                                                   *
* 004000 - 005FFF Slot 1 Low  Decode                                                   *
* 006000 - 007FFF Slot 1 High Decode                                                   *
* 008000 - 009FFF Slot 2 Low  Decode                                                   *
* 00A000 - 00BFFF Slot 2 High Decode                                                   *
* 00C000 - 00CFFF Floppy Disk Controller shared RAM                                    *
*   00c001-00c7ff floppy disk control                                                  *
* 00D000 - 00DFFF I/O Board Devices                                                    *
*   00d000-00d3ff serial ports control                                                 *
*   00d800-00dbff paralel port                                                         *
*   00dc00-00dfff keyboard/mouse cops via                                              *
* 00E000 - 00FFFF CPU Board Devices                                                    *
*   00e000-00e01e cpu board control                                                    *
*   00e01f-00e7ff unused                                                               *
*   00e8xx-video address latch                                                         *
*   00f0xx memory error address latch                                                  *
*   00f8xx status register                                                             *
*                                                                                      *
\**************************************************************************************/

INLINE void cpu_board_control_access(offs_t offset)
{
	switch ((offset & 0x03ff) << 1)
	{
	case 0x0002:	/* Set DIAG1 Latch */
	case 0x0000:	/* Reset DIAG1 Latch */
		break;
	case 0x0006:	/* Set Diag2 Latch */
		diag2 = TRUE;
		break;
	case 0x0004:	/* ReSet Diag2 Latch */
		diag2 = FALSE;
		break;
	case 0x000A:	/* SEG1 Context Selection bit SET */
		/*logerror("seg bit 0 set\n");*/
		seg |= 1;
		break;
	case 0x0008:	/* SEG1 Context Selection bit RESET */
		/*logerror("seg bit 0 clear\n");*/
		seg &= ~1;
		break;
	case 0x000E:	/* SEG2 Context Selection bit SET */
		/*logerror("seg bit 1 set\n");*/
		seg |= 2;
		break;
	case 0x000C:	/* SEG2 Context Selection bit RESET */
		/*logerror("seg bit 1 clear\n");*/
		seg &= ~2;
		break;
	case 0x0010:	/* SETUP register SET */
		setup = TRUE;
		break;
	case 0x0012:	/* SETUP register RESET */
		setup = FALSE;
		break;
	case 0x001A:	/* Enable Vertical Retrace Interrupt */
		VTMSK = TRUE;
		break;
	case 0x0018:	/* Disable Vertical Retrace Interrupt */
		VTMSK = FALSE;
		set_VTIR(FALSE);
		break;
	case 0x0016:	/* Enable Soft Error Detect. */
	case 0x0014:	/* Disable Soft Error Detect. */
		break;
	case 0x001E:	/* Enable Hard Error Detect */
		test_parity = TRUE;
		break;
	case 0x001C:	/* Disable Hard Error Detect */
		test_parity = FALSE;
		set_parity_error_pending(FALSE);
		break;
	}
}

static READ16_HANDLER ( lisa_IO_r )
{
	int answer=0;

	switch ((offset & 0x7000) >> 12)
	{
	case 0x0:
		/* Slot 0 Low */
		break;

	case 0x1:
		/* Slot 0 High */
		break;

	case 0x2:
		/* Slot 1 Low */
		break;

	case 0x3:
		/* Slot 1 High */
		break;

	case 0x4:
		/* Slot 2 Low */
		break;

	case 0x5:
		/* Slot 2 High */
		break;

	case 0x6:
		if (! (offset & 0x800))
		{
			if (! (offset & 0x400))
			{
				/*if (ACCESSING_LSB)*/	/* Geez, who cares ? */
					answer = lisa_fdc_ram[offset & 0x03ff] & 0xff;	/* right ??? */
			}
		}
		else
		{
			/* I/O Board Devices */
			switch ((offset & 0x0600) >> 9)
			{
			case 0:	/* serial ports control */
				/*SCCBCTL	        .EQU    $FCD241	        ;SCC channel B control
				ACTL	        .EQU    2	        ;offset to SCC channel A control
				SCCDATA	        .EQU    4	        ;offset to SCC data regs*/
				break;

			case 2:	/* parallel port */
				/* 1 VIA located at 0xD901 */
				if (ACCESSING_LSB)
					return via_read(1, (offset >> 2) & 0xf);
				break;

			case 3:	/* keyboard/mouse cops via */
				/* 1 VIA located at 0xDD81 */
				if (ACCESSING_LSB)
					return via_read(0, offset & 0xf);
				break;
			}
		}
		break;

	case 0x7:
		/* CPU Board Devices */
		switch ((offset & 0x0C00) >> 10)
		{
		case 0x0:	/* cpu board control */
			cpu_board_control_access(offset & 0x03ff);
			break;

		case 0x1:	/* Video Address Latch */
			answer = video_address_latch;
			break;

		case 0x2:	/* Memory Error Address Latch */
			answer = mem_err_addr_latch;
			break;

		case 0x3:	/* Status Register */
			answer = 0;
			if (! parity_error_pending)
				answer |= 0x02;
			if (! VTIR)
				answer |= 0x04;
			/* huh... we need to emulate some other bits */
			break;
		}
		break;
	}

	return answer;
}

static WRITE16_HANDLER ( lisa_IO_w )
{
	switch ((offset & 0x7000) >> 12)
	{
	case 0x0:
		/* Slot 0 Low */
		break;

	case 0x1:
		/* Slot 0 High */
		break;

	case 0x2:
		/* Slot 1 Low */
		break;

	case 0x3:
		/* Slot 1 High */
		break;

	case 0x4:
		/* Slot 2 Low */
		break;

	case 0x5:
		/* Slot 2 High */
		break;

	case 0x6:
		if (! (offset & 0x0800))
		{
			/* Floppy Disk Controller shared RAM */
			if (! (offset & 0x0400))
			{
				if (ACCESSING_LSB)
					lisa_fdc_ram[offset & 0x03ff] = data & 0xff;
			}
		}
		else
		{
			/* I/O Board Devices */
			switch ((offset & 0x0600) >> 9)
			{
			case 0:	/* serial ports control */
				break;

			case 2:	/* paralel port */
				if (ACCESSING_LSB)
					via_write(1, (offset >> 2) & 0xf, data & 0xff);
				break;

			case 3:	/* keyboard/mouse cops via */
				if (ACCESSING_LSB)
					via_write(0, offset & 0xf, data & 0xff);
				break;
			}
		}
		break;

	case 0x7:
		/* CPU Board Devices */
		switch ((offset & 0x0C00) >> 10)
		{
		case 0x0:	/* cpu board control */
			cpu_board_control_access(offset & 0x03ff);
			break;

		case 0x1:	/* Video Address Latch */
			/*logerror("video address latch write offs=%X, data=%X\n", offset, data);*/
			COMBINE_DATA(& video_address_latch);
			videoram_ptr = ((UINT16 *)lisa_ram_ptr) + ((video_address_latch << 6) & 0xfc000);
			/*logerror("video address latch %X -> base address %X\n", video_address_latch,
							(video_address_latch << 7) & 0x1f8000);*/
			break;
		}
		break;
	}
}


