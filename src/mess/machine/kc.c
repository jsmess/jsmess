#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "includes/kc.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "image.h"

#define KC_DEBUG

static int kc85_pio_data[2];

static void kc85_4_update_0x0c000(void);
static void kc85_4_update_0x0e000(void);
static void kc85_4_update_0x08000(void);
//static void kc85_4_update_0x04000(void);

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK ENABLE (1=blink, 0=no blink)
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */


/* load image */
static int kc_load(mess_image *image, unsigned char **ptr)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image_length(image);

	if (datasize!=0)
	{
		/* malloc memory for this data */
		data = malloc(datasize);

		if (data!=NULL)
		{
			/* read whole file */
			image_fread(image, data, datasize);

			*ptr = data;

			logerror("File loaded!\r\n");

			/* ok! */
			return 1;
		}
	}

	return 0;
}

struct kcc_header
{
	unsigned char	name[10];
	unsigned char	reserved[6];
	unsigned char	anzahl;
	unsigned char	load_address_l;
	unsigned char	load_address_h;
	unsigned char	length_l;
	unsigned char	length_h;
	unsigned short	execution_address;
	unsigned char	pad[128-2-2-2-1-16];
};

/* appears to work a bit.. */
/* load file, then type: MENU and it should now be displayed. */
/* now type name that has appeared! */

/* load snapshot */
QUICKLOAD_LOAD(kc)
{
	unsigned char *data;
	struct kcc_header *header;
	int addr;
	int datasize;
	int i;

	if (!kc_load(image, &data))
		return INIT_FAIL;

	header = (struct kcc_header *) data;
	datasize = (header->length_l & 0x0ff) | ((header->length_h & 0x0ff)<<8);
	datasize = datasize-128;
	addr = (header->load_address_l & 0x0ff) | ((header->load_address_h & 0x0ff)<<8);

	for (i=0; i<datasize; i++)
		mess_ram[(addr+i) & 0x0ffff] = data[i+128];
	return INIT_PASS;
}


/******************/
/* DISK EMULATION */
/******************/
/* used by KC85/2, KC85/3 and KC85/4 */
/* floppy disc interface has:
- Z80 at 4Mhz
- Z80 CTC
- 1k ram
- NEC765 floppy disc controller
*/

/* bit 7: DMA Request (DRQ from FDC) */
/* bit 6: Interrupt (INT from FDC) */
/* bit 5: Drive Ready */
/* bit 4: Index pulse from disc */
static unsigned char kc85_disc_hw_input_gate;

DEVICE_LOAD( kc85_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		basicdsk_set_geometry(image, 80, 2, 9, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

#if 0
static void kc85_disc_hw_ctc_interrupt(int state)
{
	cpunum_set_input_line(1, 0, state);
}
#endif

 READ8_HANDLER(kc85_disk_hw_ctc_r)
{
	return z80ctc_1_r(offset);
}

WRITE8_HANDLER(kc85_disk_hw_ctc_w)
{
	z80ctc_1_w(offset,data);
}

WRITE8_HANDLER(kc85_disc_interface_ram_w)
{
	int addr;


	/* bits 1,0 of i/o address define 256 byte block to access.
	bits 15-8 define the byte offset in the 256 byte block selected */
	addr = ((offset & 0x03)<<8) | ((offset>>8) & 0x0ff);

	logerror("interface ram w: %04x %02x\n",addr,data);

	program_write_byte(addr|0x0f000,data);
}

 READ8_HANDLER(kc85_disc_interface_ram_r)
{
	int addr;


	addr = ((offset & 0x03)<<8) | ((offset>>8) & 0x0ff);

	logerror("interface ram r: %04x\n",addr);

	return program_read_byte(addr|0x0f000);
}

/* 4-bit latch used to reset disc interface etc */
WRITE8_HANDLER(kc85_disc_interface_latch_w)
{
	logerror("kc85 disc interface latch w\n");
}

 READ8_HANDLER(kc85_disc_hw_input_gate_r)
{
	return kc85_disc_hw_input_gate;
}

WRITE8_HANDLER(kc85_disc_hw_terminal_count_w)
{
	logerror("kc85 disc hw tc w: %02x\n",data);
	nec765_set_tc_state(data & 0x01);
}


/* callback for /INT output from FDC */
static void kc85_fdc_interrupt(int state)
{
	kc85_disc_hw_input_gate &=~(1<<6);
	if (state)
		kc85_disc_hw_input_gate |=(1<<6);
}

/* callback for /DRQ output from FDC */
static void kc85_fdc_dma_drq(int state, int read_)
{
	kc85_disc_hw_input_gate &=~(1<<7);
	if (state)
		kc85_disc_hw_input_gate |=(1<<7);
}

static struct nec765_interface kc_fdc_interface=
{
	kc85_fdc_interrupt,
	kc85_fdc_dma_drq
};

static void kc85_disk_reset_timer_callback(int dummy)
{
	cpunum_set_reg(1, REG_PC, 0x0f000);
	cpunum_set_reg(0, REG_PC, 0x0f000);
}

static void kc_disc_interface_init(void)
{
	timer_set(TIME_NOW, 0, kc85_disk_reset_timer_callback);

	nec765_init(&kc_fdc_interface,NEC765A);

	/* reset ctc */
	z80ctc_reset(1); 

	/* hold cpu at reset */
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}

/*****************/
/* MODULE SYSTEM */
/*****************/

/* The KC85/4 and KC85/3 are "modular systems". These computers can be expanded with modules.
*/

/*
	Module ID		Module Name			Module Description


					D001				Basis Device
					D002				Bus Driver device
	a7				D004				Floppy Disk Interface Device


	ef				M001				Digital IN/OUT
	ee				M003				V24
					M005				User (empty)
					M007				Adapter (empty)
	e7				M010				ADU1
	f6				M011				64k RAM
	fb				M012				Texor
	f4				M022				Expander RAM (16k)
	f7				M025				User PROM (8k)
	fb				M026				Forth
	fb				M027				Development
	e3				M029				DAU1
*/


static unsigned char *kc85_module_rom;

struct kc85_module
{
	/* id of module */
	int id;
	/* name */
	const char *module_name;
	/* description */
	const char *module_description;
	/* enable module */
	void (*enable)(int state);
};
/*
static struct kc85_module kc85_v24_module=
{
	0x0ee,
	"M003",
	"V24"
};
*/
static struct kc85_module kc85_disk_interface_device=
{
	0x0a7,
	"D004",
	"Disk Interface"
};

static struct kc85_module	*modules[256>>2];
/*

	port xx80

	- xx is module id.


	Only addressess divisible by 4 are checked.
	If module does not exist, 0x0ff is returned.

	When xx80 is read, if a module exists a id will be returned.
	Id's for known modules are listed above.
  */

/* bus drivers 4 */

 READ8_HANDLER(kc85_module_r)
{
	int port_upper;
	int module_index;
#ifdef KC_DEBUG
	logerror("kc85 module r: %04x\n",offset);
#endif

	port_upper = (offset>>8) & 0x0ff;

	/* module is accessed at every 4th address. 0x00,0x04,0x08,0x0c etc */
	if ((port_upper & 0x03)!=0)
	{
		return 0x0ff;
	}

	module_index = (port_upper>>2) & 0x03f;

	if (modules[module_index])
	{
		return modules[module_index]->id;
	}

	return 0x0ff;
}

WRITE8_HANDLER(kc85_module_w)
{
	logerror("kc85 module w: %04x %02x\n",offset,data);
}


static void kc85_module_system_init(void)
{
	int i;

	kc85_module_rom = NULL;
	for (i=0; i<64; i++)
	{
		modules[i] = NULL;
	}

	modules[30] = &kc85_disk_interface_device;
}



/**********************/
/* CASSETTE EMULATION */
/**********************/
/* used by KC85/4 and KC85/3 */
/*
	The cassette motor is controlled by bit 6 of PIO port A.
	The cassette read data is connected to /ASTB input of the PIO.
	A edge from the cassette therefore will trigger a interrupt
	from the PIO.
	The duration between two edges can be timed and the data-bit
	identified.

	I have used a timer to feed data into /ASTB. The timer is only
	active when the cassette motor is on.
*/


#define KC_CASSETTE_TIMER_FREQUENCY TIME_IN_HZ(4800)

/* this timer is used to update the cassette */
static mame_timer *kc_cassette_timer;
/* this is the current state of the cassette motor */
static int kc_cassette_motor_state;
/* ardy output from pio */
static unsigned char kc_ardy;

static void kc_cassette_timer_callback(int dummy)
{
	int bit;

	/* the cassette data is linked to /astb input of
	the pio. */

	bit = 0;

	/* get data from cassette */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038)
		bit = 1;

	/* update astb with bit */
	z80pio_astb_w(0,bit & kc_ardy);
}

static void	kc_cassette_init(void)
{
	kc_cassette_timer = timer_alloc(kc_cassette_timer_callback);
}

static void	kc_cassette_set_motor(int motor_state)
{
	/* state changed? */
	if (((kc_cassette_motor_state^motor_state)&0x01)!=0)
	{
		/* set new motor state in cassette device */
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),
			motor_state ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
			CASSETTE_MASK_MOTOR);

		if (motor_state)
		{
			/* start timer */
			timer_adjust(kc_cassette_timer, 0, 0, KC_CASSETTE_TIMER_FREQUENCY);
		}
		else
		{
			/* stop timer */
			timer_reset(kc_cassette_timer, TIME_NEVER);
		}
	}

	/* store new state */
	kc_cassette_motor_state = motor_state;
}

/*
  pin 2 = gnd
  pin 3 = read
  pin 1 = k1		?? modulating signal
  pin 4 = k0		?? signal??
  pin 5 = motor on


	Tape signals:
		K0, K1		??
		MOTON		motor control
		ASTB		read?

		T1-T4 give 4 bit a/d tone sound?
		K1, K0 are mixed with tone.

	Cassette read goes into ASTB of PIO.
	From this, KC must be able to detect the length
	of the pulses and can read the data.


	Tape write: clock comes from CTC?
	truck signal resets, 5v signal for set.
	output gives k0 and k1.




*/




/**********************/
/* KEYBOARD EMULATION */
/**********************/
/* used by KC85/4 and KC85/3 */

/* The basic transmit proceedure is working, keys are received */
/* Todo: Key-repeat, and allowing the same key to be pressed twice! */

//#define KC_KEYBOARD_DEBUG

/*

  E-mail from Torsten Paul about the keyboard:

	Torsten.Paul@gmx.de

"> I hope that you will be able to help me so I can add keyboard
> emulation to it.
>
Oh yes the keyboard caused me a lot of trouble too. I still don't
understand it completely.
A first dirty but working keyboard support is quite easy because
of the good and modular system rom. Most programs use the supplied
routines to read keyboard input so patching the right memory address
with the keycode that is originally supplied by the interrupt
routines for keyboard input will work. So if you first want to
have a simple start to get other things to work you can try this.

    * write keycode (ascii) to address IX+0dh (0x1fd)
    * set bit 0 at address IX+08h (0x1f8) - simply writing 01h
      worked for me but some other bits are used too
      this bit is reset if the key is read by the system

> From the schematics I see that the keyboard is linked to the CTC and PIO,
> but I don't understand it fully.
> Please can you tell me more about how the keyboard is scanned?
>
The full emulation is quite tricky and I still have some programs that
behave odd. (A good hint for a correct keyboard emulation is Digger ;-).

Ok, now the technical things:

The keyboard of the KC is driven by a remote control circuit that is
originally designed for infrared remote control. This one was named
U807 and I learned there should be a similar chip called SAB 3021
available but I never found the specs on the web. The SAB 3021 was
produced by Valvo which doesn't exist anymore (bought by Phillips
if I remember correctly). If you have more luck finding the specs
I'm still interested.
There also was a complementary chip for the recieving side but that
was not used in the KC unfortunately. They choosed to measure the
pulses sent by the U807 via PIO and CTC.

Anyway here is what I know about the protocol:

The U807 sends impulses with equal length. The information is
given by the time between two impulses. Short time means bit is 1
long time mean bit is 0. The impulses are modulated by a 62.5 kHz
Signal but that's not interesting for the emulator.
The timing comes from a base clock of 4 MHz that is divided
multiple times:

4 MHz / 64 - 62.5 kHz -> used for filtered amplification
62.5 kHz / 64 - 976 Hz -> 1.024 ms
976 / 7 - 140 Hz -> 7.2 ms
976 / 5 - 195 Hz -> 5.1 ms

short - 5.12 ms - 1 bit
long - 7.168 ms - 0 bit

       +-+     +-+       +-+     +-+
       | |     | |       | |     | |
    +--+ +-----+ +-------+ +-----+ +--....

         |     | |---0---| |--1--|
            ^
            |
        Startbit = shift key

The keyboard can have 64 keys with an additional key that is used by
the KC as shift key. This additional key is directly connected to the
chip and changes the startbit. All other keys are arranged in a matrix.

The chip sends full words of 7 bits (including the startbit) at least
twice for each keypress with a spacing of 14 * 1.024 ms. If the key
is still pressed the double word is repeated after 19 * 1.024 ms.

The impulses trigger the pio interrupt line for channel B that
triggers the time measurement by the CTC channel 3." */


/*

	The CTC timer 3 count is initialised at 0x08f and counts down.

	The pulses are connected into PIO /BSTB and generate a interrupt
	on a positive edge. A pulse will not get through if BRDY from PIO is
	not true!

	Then the interrupt occurs, the CTC count is read. The time between
	pulses is therefore measured by the CTC.

	The time is checked and depending on the value the KC detects
	a 1 or 0 bit sent from the keyboard.

	Summary From code below:

	0x065<=count<0x078	-> 0 bit				"short pulse"
	0x042<=count<0x065	-> 1 bit				"long pulse"
	count<0x014			-> ignore
	count>=0x078		-> ignore
	0x014<=count<0x042	-> signal end of code	"very long pulse"

	codes are sent bit 0, bit 1, bit 2...bit 7. bit 0 is the state
	of the shift key.

	Torsten's e-mail indicates "short pulse" for 1 bit, and "long
	pulse" for 0 bit, but I found this to be incorrect. However,
	the timings are correct.


	Keyboard reading procedure extracted from KC85/4 system rom:

0345  f5        push    af
0346  db8f      in      a,(#8f)			; get CTC timer 3 count
0348  f5        push    af
0349  3ea7      ld      a,#a7			; channel 3, enable int, select counter mode, control word
										; write, software reset, time constant follows
034b  d38f      out     (#8f),a
034d  3e8f      ld      a,#8f			; time constant
034f  d38f      out     (#8f),a
0351  f1        pop     af

0352  fb        ei
0353  b7        or      a				; count is over
0354  284d      jr      z,#03a3         ;

	;; check count is in range
0356  fe14      cp      #14
0358  3849      jr      c,#03a3         ;
035a  fe78      cp      #78
035c  3045      jr      nc,#03a3        ;

	;; at this point, time must be between #14 and #77 to be valid

	;; if >=#65, then carry=0, and a 0 bit has been detected

035e  fe65      cp      #65
0360  303d      jr      nc,#039f        ; (61)

	;; if <#65, then a 1 bit has been detected. carry is set with the addition below.
	;; a carry will be generated if the count is >#42

	;; therefore for a 1 bit to be generated, then #42<=time<#65
	;; must be true.

0362  c6be      add     a,#be
0364  3839      jr      c,#039f         ; (57)

	;; this code appears to take the transmitted scan-code
	;; and converts it into a useable code by the os???
0366  e5        push    hl
0367  d5        push    de
	;; convert hardware scan-code into os code
0368  dd7e0c    ld      a,(ix+#0c)
036b  1f        rra
036c  ee01      xor     #01
036e  dd6e0e    ld      l,(ix+#0e)
0371  dd660f    ld      h,(ix+#0f)
0374  1600      ld      d,#00
0376  5f        ld      e,a
0377  19        add     hl,de
0378  7e        ld      a,(hl)
0379  d1        pop     de
037a  e1        pop     hl

	;; shift lock pressed?
037b  ddcb087e  bit     7,(ix+#08)
037f  200a      jr      nz,#038b
	;; no.

	;; alpha char?
0381  fe40      cp      #40
0383  3806      jr      c,#038b
0385  fe80      cp      #80
0387  3002      jr      nc,#038b
	;; yes, it is a alpha char
	;; force to lower case
0389  ee20      xor     #20

038b  ddbe0d    cp      (ix+#0d)		;; same as stored?
038e  201d      jr      nz,#03ad

	 ;; yes - must be held for a certain time before it can repeat?
0390  f5        push    af
0391  3ae0b7    ld      a,(#b7e0)
0394  ddbe0a    cp      (ix+#0a)
0397  3811      jr      c,#03aa
0399  f1        pop     af

039a  dd340a    inc     (ix+#0a)		;; incremenent repeat count?
039d  1804      jr      #03a3

	;; update scan-code received so far

039f  ddcb0c1e  rr      (ix+#0c)		; shift in 0 or 1 bit depending on what has been selected

03a3  db89      in      a,(#89)			; used to clear brdy
03a5  d389      out     (#89),a
03a7  f1        pop     af
03a8  ed4d      reti

03aa  f1        pop     af
03ab  1808      jr      #03b5

;; clear count
03ad  dd360a00  ld      (ix+#0a),#00

;; shift lock?
03b1  fe16      cp      #16
03b3  2809      jr      z,#03be

;; store char
03b5  dd770d    ld      (ix+#0d),a
03b8  ddcb08c6  set     0,(ix+#08)
03bc  18e5      jr      #03a3

;; toggle shift lock on/off
03be  dd7e08    ld      a,(ix+#08)
03c1  ee80      xor     #80
03c3  dd7708    ld      (ix+#08),a
;; shift/lock was last key pressed
03c6  3e16      ld      a,#16
03c8  18eb      jr      #03b5

03ca  b7        or      a
03cb  ddcb0846  bit     0,(ix+#08)
03cf  c8        ret     z
03d0  dd7e0d    ld      a,(ix+#0d)
03d3  37        scf
03d4  c9        ret
03d5  cdcae3    call    #e3ca
03d8  d0        ret     nc
03d9  ddcb0886  res     0,(ix+#08)
03dd  c9        ret
03de  cdcae3    call    #e3ca
03e1  d0        ret     nc
03e2  fe03      cp      #03
03e4  37        scf
03e5  c8        ret     z
03e6  a7        and     a
03e7  c9        ret


  Keyboard reading procedure extracted from KC85/3 rom:


019a  f5        push    af
019b  db8f      in      a,(#8f)
019d  f5        push    af
019e  3ea7      ld      a,#a7
01a0  d38f      out     (#8f),a
01a2  3e8f      ld      a,#8f
01a4  d38f      out     (#8f),a
01a6  f1        pop     af
01a7  ddcb085e  bit     3,(ix+#08)
01ab  ddcb089e  res     3,(ix+#08)
01af  2055      jr      nz,#0206        ; (85)

01b1  fe65      cp      #65
01b3  3054      jr      nc,#0209


	;; count>=#65 = 0 bit
	;; #44<=count<#65 = 1 bit
	;; count<#44 = end of code

01b5  fe44      cp      #44
01b7  3053      jr      nc,#020c
01b9  e5        push    hl
01ba  d5        push    de
01bb  ddcb0c3e  srl     (ix+#0c)
01bf  dd7e08    ld      a,(ix+#08)
01c2  e680      and     #80
01c4  07        rlca
01c5  ddae0c    xor     (ix+#0c)
01c8  2600      ld      h,#00
01ca  dd5e0e    ld      e,(ix+#0e)
01cd  dd560f    ld      d,(ix+#0f)
01d0  6f        ld      l,a
01d1  19        add     hl,de
01d2  7e        ld      a,(hl)
01d3  d1        pop     de
01d4  e1        pop     hl
01d5  ddbe0d    cp      (ix+#0d)
01d8  2811      jr      z,#01eb         ; (17)
01da  dd770d    ld      (ix+#0d),a
01dd  ddcb08a6  res     4,(ix+#08)
01e1  ddcb08c6  set     0,(ix+#08)
01e5  dd360a00  ld      (ix+#0a),#00
01e9  181b      jr      #0206           ; (27)
01eb  dd340a    inc     (ix+#0a)
01ee  ddcb0866  bit     4,(ix+#08)
01f2  200c      jr      nz,#0200        ; (12)
01f4  ddcb0a66  bit     4,(ix+#0a)
01f8  280c      jr      z,#0206         ; (12)
01fa  ddcb08e6  set     4,(ix+#08)
01fe  18e1      jr      #01e1           ; (-31)
0200  ddcb0a4e  bit     1,(ix+#0a)
0204  20db      jr      nz,#01e1        ; (-37)
0206  f1        pop     af
0207  ed4d      reti
0209  b7        or      a
020a  1801      jr      #020d           ; (1)
020c  37        scf
020d  ddcb0c1e  rr      (ix+#0c)
0211  18f3      jr      #0206           ; (-13)


*/

/* number of keycodes that can be stored in queue */
#define KC_KEYCODE_QUEUE_LENGTH 256

/* no transmit is in progress, keyboard is idle ready to transmit a key */
#define KC_KEYBOARD_TRANSMIT_IDLE	0x0001
/* keyboard is transmitting a key-code */
#define KC_KEYBOARD_TRANSMIT_ACTIVE	0x0002

#define KC_KEYBOARD_NUM_LINES	9

typedef struct kc_keyboard
{
	/* list of stored keys */
	unsigned char keycodes[KC_KEYCODE_QUEUE_LENGTH];
	/* index of start of list */
	int head;
	/* index of end of list */
	int tail;

	/* transmitting state */
	int transmit_state;

	/* number of pulses remaining to be transmitted */
	int	transmit_pulse_count_remaining;
	/* count of pulses transmitted so far */
	int transmit_pulse_count;

	/* pulses to transmit */
	unsigned char transmit_buffer[32];
} kc_keyboard;

/* previous state of keyboard - currently used to detect if a key has been pressed/released  since last scan */
static int kc_previous_keyboard[KC_KEYBOARD_NUM_LINES-1];
/* brdy output from pio */
static unsigned char kc_brdy;

/*
	The kc keyboard is seperate from the kc base unit.

	The keyboard emulation uses 2 timers:

	update_timer is used to add scan-codes to the keycode list.
	transmit_timer is used to transmit the scan-code to the kc.
*/

static kc_keyboard keyboard_data;
static void kc_keyboard_attempt_transmit(void);
static void kc_keyboard_update(int dummy);

/* this is called at a regular interval */
static void kc_keyboard_transmit_timer_callback(int dummy)
{
	if (keyboard_data.transmit_pulse_count_remaining!=0)
	{
		int pulse_state;
		/* byte containing pulse state */
		int pulse_byte_count = keyboard_data.transmit_pulse_count>>3;
		/* bit within byte containing pulse state */
		int pulse_bit_count = 7-(keyboard_data.transmit_pulse_count & 0x07);

		/* get current pulse state */
		pulse_state = (keyboard_data.transmit_buffer[pulse_byte_count]>>pulse_bit_count) & 0x01;

#ifdef KC_KEYBOARD_DEBUG
		logerror("kc keyboard sending pulse: %02x\n",pulse_state);
#endif

		/* set pulse */
		z80pio_bstb_w(0,pulse_state & kc_brdy);

		/* update counts */
		keyboard_data.transmit_pulse_count_remaining--;
		keyboard_data.transmit_pulse_count++;

	}
	else
	{
		keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_IDLE;
	}
}

/* add a pulse */
static void kc_keyboard_add_pulse_to_transmit_buffer(int pulse_state)
{
	int pulse_byte_count = keyboard_data.transmit_pulse_count_remaining>>3;
	int pulse_bit_count = 7-(keyboard_data.transmit_pulse_count_remaining & 0x07);

	if (pulse_state)
	{
		keyboard_data.transmit_buffer[pulse_byte_count] |= (1<<pulse_bit_count);
	}
	else
	{
		keyboard_data.transmit_buffer[pulse_byte_count] &= ~(1<<pulse_bit_count);
	}

	keyboard_data.transmit_pulse_count_remaining++;
}


/* initialise keyboard queue */
static void kc_keyboard_init(void)
{
	int i;

	/* head and tail of list is at beginning */
	keyboard_data.head = (keyboard_data.tail = 0);

	/* 50hz is just a arbitrary value - used to put scan-codes into the queue for transmitting */
	timer_pulse(TIME_IN_HZ(50), 0, kc_keyboard_update);

	/* timer to transmit pulses to kc base unit */
	timer_pulse(TIME_IN_MSEC(1.024), 0, kc_keyboard_transmit_timer_callback);

	/* kc keyboard is not transmitting */
	keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_IDLE;

	/* setup transmit parameters */
	keyboard_data.transmit_pulse_count_remaining = 0;
	keyboard_data.transmit_pulse_count = 0;

	/* set initial state */
	z80pio_bstb_w(0,0);


	for (i=0; i<KC_KEYBOARD_NUM_LINES; i++)
	{
		/* read input port */
		kc_previous_keyboard[i] = readinputport(i);
	}
}


/* add a key to the queue */
static void kc_keyboard_queue_scancode(int scan_code)
{
	/* add a key */
	keyboard_data.keycodes[keyboard_data.tail] = scan_code;
	/* update next insert position */
	keyboard_data.tail = (keyboard_data.tail + 1) % KC_KEYCODE_QUEUE_LENGTH;
}

/* fill transmit buffer with pulse for 0 or 1 bit */
static void kc_keyboard_add_bit(int bit)
{
	if (bit)
	{
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
	}
	else
	{
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
	}

	/* "end of bit" pulse -> end of time for bit */
	kc_keyboard_add_pulse_to_transmit_buffer(1);
}


static void kc_keyboard_begin_transmit(int scan_code)
{
	int i;
	int scan;

	keyboard_data.transmit_pulse_count_remaining = 0;
	keyboard_data.transmit_pulse_count = 0;

	/* initial pulse -> start of code */
	kc_keyboard_add_pulse_to_transmit_buffer(1);

	scan = scan_code;

	/* state of shift key */
	kc_keyboard_add_bit(((readinputport(8) & 0x01)^0x01));

	for (i=0; i<6; i++)
	{
		/* each bit in turn */
		kc_keyboard_add_bit(scan & 0x01);
		scan = scan>>1;
	}

	/* signal end of scan-code */
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);

	kc_keyboard_add_pulse_to_transmit_buffer(1);

	/* back to original state */
	kc_keyboard_add_pulse_to_transmit_buffer(0);

}

/* attempt to transmit a new keycode to the base unit */
static void kc_keyboard_attempt_transmit(void)
{
	/* is the keyboard transmit is idle */
	if (keyboard_data.transmit_state == KC_KEYBOARD_TRANSMIT_IDLE)
	{
		/* keycode available? */
		if (keyboard_data.head!=keyboard_data.tail)
		{
			int code;

			keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_ACTIVE;

			/* get code */
			code = keyboard_data.keycodes[keyboard_data.head];
			/* update start of list */
			keyboard_data.head = (keyboard_data.head + 1) % KC_KEYCODE_QUEUE_LENGTH;

			/* setup transmit buffer with scan-code */
			kc_keyboard_begin_transmit(code);
		}
	}
}


/* update keyboard */
static void	kc_keyboard_update(int dummy)
{
	int i;

	/* scan all lines (excluding shift) */
	for (i=0; i<KC_KEYBOARD_NUM_LINES-1; i++)
	{
		int b;
		int keyboard_line_data;
		int changed_keys;
		int mask = 0x001;

		/* read input port */
		keyboard_line_data = readinputport(i);
		/* identify keys that have changed */
		changed_keys = keyboard_line_data ^ kc_previous_keyboard[i];
		/* store input port for next time */
		kc_previous_keyboard[i] = keyboard_line_data;

		/* scan through each bit */
		for (b=0; b<8; b++)
		{
			/* key changed? */
			if (changed_keys & mask)
			{
				/* yes */

				/* new state is pressed? */
				if ((keyboard_line_data & mask)!=0)
				{
					/* pressed */
					int code;

					/* generate fake code */
					code = (i<<3) | b;
#ifdef KC_KEYBOARD_DEBUG
					logerror("code: %02x\n",code);
#endif
					kc_keyboard_queue_scancode(code);
				}
			}

			mask = mask<<1;
		}
	}

	kc_keyboard_attempt_transmit();
}

/*********************************************************************/

/* port 0x084/0x085:

bit 7: RAF3
bit 6: RAF2
bit 5: RAF1
bit 4: RAF0
bit 3: FPIX. high resolution
bit 2: BLA1 .access screen
bit 1: BLA0 .pixel/color
bit 0: BILD .display screen 0 or 1
*/

/* port 0x086/0x087:

bit 7: ROCC
bit 6: ROF1
bit 5: ROF0
bit 4-2 are not connected
bit 1: WRITE PROTECT RAM 4
bit 0: ACCESS RAM 4
*/


/* KC85/4 specific */

static int kc85_84_data;
static int kc85_86_data;



/* port 0x086:

bit 7: CAOS ROM C
bit 6:
bit 5:
bit 4:
bit 3:
bit 2:
bit 1: write protect ram 4
bit 0: ram 4
*/

static void kc85_4_update_0x08000(void)
{
	unsigned char *ram_page;

    if (kc85_pio_data[1] & (1<<5))
    {
		int ram8_block;
		unsigned char *mem_ptr;

		/* ram8 block chosen */
		ram8_block = ((kc85_84_data)>>4) & 0x01;

		mem_ptr = mess_ram+0x08000+(ram8_block<<14);

		memory_set_bankptr(3, mem_ptr);
		memory_set_bankptr(4, mem_ptr+0x02800);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MRA8_BANK3);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MRA8_BANK4);

		/* write protect RAM8 ? */
		if ((kc85_pio_data[1] & (1<<6))==0)
		{
			/* ram8 is enabled and write protected */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MWA8_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MWA8_NOP);
		}
		else
		{
#ifdef KC_DEBUG
			logerror("RAM8 write enabled\n");
#endif
			/* ram8 is enabled and write enabled */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MWA8_BANK9);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MWA8_BANK10);
			memory_set_bankptr(9, mem_ptr);
			memory_set_bankptr(10, mem_ptr+0x02800);
		}
    }
    else
    {
#ifdef KC_DEBUG
		logerror("no memory at ram8\n");
#endif
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MRA8_NOP);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MWA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MWA8_NOP);
    }

	/* if IRM is enabled override block 3/9 settings */
	if (kc85_pio_data[0] & (1<<2))
	{
		/* IRM enabled - has priority over RAM8 enabled */
		ram_page = kc85_4_get_video_ram_base((kc85_84_data & 0x04), (kc85_84_data & 0x02));

		memory_set_bankptr(3, ram_page);
		memory_set_bankptr(9, ram_page);
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MRA8_BANK3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xa7ff, 0, 0, MWA8_BANK9);

		ram_page = kc85_4_get_video_ram_base(0, 0);

		memory_set_bankptr( 4, ram_page + 0x2800);
		memory_set_bankptr(10, ram_page + 0x2800);
		memory_install_read8_handler(0,  ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MRA8_BANK4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xbfff, 0, 0, MWA8_BANK10);
	}
}

/* update status of memory area 0x0000-0x03fff */
static void kc85_4_update_0x00000(void)
{
	/* access ram? */
	if (kc85_pio_data[0] & (1<<1))
	{
#ifdef KC_DEBUG
		logerror("ram0 enabled\n");
#endif

		/* yes; set address of bank */
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MRA8_BANK1);
		memory_set_bankptr(1, mess_ram);

		/* write protect ram? */
		if ((kc85_pio_data[0] & (1<<3))==0)
		{
			/* yes */
#ifdef KC_DEBUG
			logerror("ram0 write protected\n");
#endif

			/* ram is enabled and write protected */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_ROM);
		}
		else
		{
#ifdef KC_DEBUG
			logerror("ram0 write enabled\n");
#endif

			/* ram is enabled and write enabled; and set address of bank */
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_BANK7);
			memory_set_bankptr(7, mess_ram);
		}
	}
	else
	{
#ifdef KC_DEBUG
		logerror("no memory at ram0!\n");
#endif

//		memory_set_bankptr(1,memory_region(REGION_CPU1) + 0x013000);
		/* ram is disabled */
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MRA8_NOP);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MWA8_NOP);
	}
}

/* update status of memory area 0x4000-0x07fff */
static void kc85_4_update_0x04000(void)
{
	read8_handler rh;
	write8_handler wh;

	/* access ram? */
	if (kc85_86_data & (1<<0))
	{
		UINT8 *mem_ptr;

		mem_ptr = mess_ram + 0x04000;

		/* yes */
		rh = MRA8_BANK2;
		/* set address of bank */
		memory_set_bankptr(2, mem_ptr);

		/* write protect ram? */
		if ((kc85_86_data & (1<<1))==0)
		{
			/* yes */
#ifdef KC_DEBUG
			logerror("ram4 write protected\n");
#endif

			/* ram is enabled and write protected */
			wh = MWA8_NOP;
		}
		else
		{
#ifdef KC_DEBUG
			logerror("ram4 write enabled\n");
#endif
			/* ram is enabled and write enabled */
			wh = MWA8_BANK8;
			/* set address of bank */
			memory_set_bankptr(8, mem_ptr);
		}
	}
	else
	{
#ifdef KC_DEBUG
		logerror("no memory at ram4!\n");
#endif
		/* ram is disabled */
		rh = MRA8_NOP;
		wh = MWA8_NOP;
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x7fff, 0, 0, wh);
}


/* update memory address 0x0c000-0x0e000 */
static void kc85_4_update_0x0c000(void)
{
	read8_handler rh;

	if (kc85_86_data & (1<<7))
	{
		/* CAOS rom takes priority */
#ifdef KC_DEBUG
		logerror("CAOS rom 0x0c000\n");
#endif
		memory_set_bankptr(5,memory_region(REGION_CPU1) + 0x012000);
		rh = MRA8_BANK5;
	}
	else if (kc85_pio_data[0] & (1<<7))
	{
#ifdef KC_DEBUG
		/* BASIC takes next priority */
        logerror("BASIC rom 0x0c000\n");
#endif

        memory_set_bankptr(5, memory_region(REGION_CPU1) + 0x010000);
		rh = MRA8_BANK5;
	}
	else
	{
		if (kc85_module_rom)
		{
			logerror("module rom at 0xc000\n");

			memory_set_bankptr(5, kc85_module_rom);
			rh = MRA8_BANK5;
		}
		else
		{

#ifdef KC_DEBUG
			logerror("No roms 0x0c000\n");
#endif
			rh = MRA8_NOP;
		}
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, rh);
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_4_update_0x0e000(void)
{
	read8_handler rh;

	if (kc85_pio_data[0] & (1<<0))
	{
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */
#ifdef KC_DEBUG
		logerror("CAOS rom 0x0e000\n");
#endif
		/* read will access the rom */
		memory_set_bankptr(6,memory_region(REGION_CPU1) + 0x013000);
		rh = MRA8_BANK6;
	}
	else
	{
#ifdef KC_DEBUG
		logerror("no rom 0x0e000\n");
#endif
		rh = MRA8_NOP;
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, rh);
}

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */

WRITE8_HANDLER ( kc85_4_pio_data_w )
{
	kc85_pio_data[offset] = data;
	z80pio_d_w(0, offset, data);

	switch (offset)
	{

		case 0:
		{
			kc85_4_update_0x0c000();
			kc85_4_update_0x0e000();
			kc85_4_update_0x08000();
			kc85_4_update_0x00000();

			kc_cassette_set_motor((data>>6) & 0x01);
		}
		break;

		case 1:
		{
			int speaker_level;

			kc85_4_update_0x08000();

			/* 16 speaker levels */
			speaker_level = (data>>1) & 0x0f;

			/* this might not be correct, the range might
			be logarithmic and not linear! */
			speaker_level_w(0, (speaker_level<<4));
		}
		break;
	}
}


WRITE8_HANDLER ( kc85_4_86_w )
{
#ifdef KC_DEBUG
	logerror("0x086 W: %02x\n",data);
#endif

	kc85_86_data = data;

	kc85_4_update_0x0c000();
	kc85_4_update_0x04000();
}

 READ8_HANDLER ( kc85_4_86_r )
{
	return kc85_86_data;
}


WRITE8_HANDLER ( kc85_4_84_w )
{
#ifdef KC_DEBUG
	logerror("0x084 W: %02x\n",data);
#endif
	kc85_84_data = data;

	kc85_4_video_ram_select_bank(data & 0x01);

	kc85_4_update_0x08000();
}

 READ8_HANDLER ( kc85_4_84_r )
{
	return kc85_84_data;
}
/*****************************************************************/

/* update memory region 0x0c000-0x0e000 */
static void kc85_3_update_0x0c000(void)
{
	read8_handler rh;

	if (kc85_pio_data[0] & (1<<7))
	{
#ifdef KC_DEBUG
		/* BASIC takes next priority */
		logerror("BASIC rom 0x0c000\n");
#endif
		memory_set_bankptr(4, memory_region(REGION_CPU1) + 0x010000);
		rh = MRA8_BANK4;
	}
	else
	{
#ifdef KC_DEBUG
		logerror("No roms 0x0c000\n");
#endif
		rh = MRA8_NOP;
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0, rh);
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_3_update_0x0e000(void)
{
	read8_handler rh;

	if (kc85_pio_data[0] & (1<<0))
	{
#ifdef KC_DEBUG
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */
		logerror("CAOS rom 0x0e000\n");
#endif
		memory_set_bankptr(5,memory_region(REGION_CPU1) + 0x012000);
        rh = MRA8_BANK5;
	}
	else
	{
#ifdef KC_DEBUG
		logerror("no rom 0x0e000\n");
#endif
		rh = MRA8_NOP;
	}
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, rh);
}

/* update status of memory area 0x0000-0x03fff */
/* MRA8_BANK1 is used for read operations and MWA8_BANK6 is used
for write operations */
static void kc85_3_update_0x00000(void)
{
	read8_handler rh;
	write8_handler wh;

	/* access ram? */
	if (kc85_pio_data[0] & (1<<1))
	{
#ifdef KC_DEBUG
		logerror("ram0 enabled\n");
#endif
		/* yes */
		rh = MRA8_BANK1;
		/* set address of bank */
		memory_set_bankptr(1, mess_ram);

		/* write protect ram? */
		if ((kc85_pio_data[0] & (1<<3))==0)
		{
			/* yes */
#ifdef KC_DEBUG
			logerror("ram0 write protected\n");
#endif

			/* ram is enabled and write protected */
			wh = MWA8_NOP;
		}
		else
		{
#ifdef KC_DEBUG
		logerror("ram0 write enabled\n");
#endif

			/* ram is enabled and write enabled */
			wh = MWA8_BANK6;
			/* set address of bank */
			memory_set_bankptr(6, mess_ram);
		}
	}
	else
	{
#ifdef KC_DEBUG
		logerror("no memory at ram0!\n");
#endif

		/* ram is disabled */
		rh = MRA8_NOP;
		wh = MWA8_NOP;
	}

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, wh);
}

/* update status of memory area 0x08000-0x0ffff */
/* MRA8_BANK3 is used for read, MWA8_BANK8 is used for write */
static void kc85_3_update_0x08000(void)
{
	read8_handler rh;
	write8_handler wh;
    unsigned char *ram_page;

    if (kc85_pio_data[0] & (1<<2))
    {
        /* IRM enabled */
#ifdef KC_DEBUG
        logerror("IRM enabled\n");
#endif
		ram_page = mess_ram+0x08000;

		memory_set_bankptr(3, ram_page);
		memory_set_bankptr(8, ram_page);

		rh = MRA8_BANK3;
		wh = MWA8_BANK8;
    }
    else if (kc85_pio_data[1] & (1<<5))
    {
		/* RAM8 ACCESS */
#ifdef KC_DEBUG
		logerror("RAM8 enabled\n");
#endif
		ram_page = mess_ram + 0x04000;

		memory_set_bankptr(3, ram_page);
		rh = MRA8_BANK3;

		/* write protect RAM8 ? */
		if ((kc85_pio_data[1] & (1<<6))==0)
		{
#ifdef KC_DEBUG
			logerror("RAM8 write protected\n");
#endif
			/* ram8 is enabled and write protected */
			wh = MWA8_NOP;
		}
		else
		{
#ifdef KC_DEBUG
			logerror("RAM8 write enabled\n");
#endif
			/* ram8 is enabled and write enabled */
			wh = MWA8_BANK8;
			memory_set_bankptr(8,ram_page);
		}
    }
    else
    {
#ifdef KC_DEBUG
		logerror("no memory at ram8!\n");
#endif
		rh = MRA8_NOP;
		wh = MWA8_NOP;
    }

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, rh);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0, wh);
}


/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK ENABLE
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */

WRITE8_HANDLER ( kc85_3_pio_data_w )
{
   kc85_pio_data[offset] = data;
   z80pio_d_w(0, offset, data);

   switch (offset)
   {

		case 0:
		{
			kc85_3_update_0x0c000();
			kc85_3_update_0x0e000();
			kc85_3_update_0x00000();

			kc_cassette_set_motor((data>>6) & 0x01);
		}
		break;

		case 1:
		{
			int speaker_level;

			kc85_3_update_0x08000();

			/* 16 speaker levels */
			speaker_level = (data>>1) & 0x0f;

			/* this might not be correct, the range might
			be logarithmic and not linear! */
			speaker_level_w(0, (speaker_level<<4));
		}
		break;
   }
}


/*****************************************************************/

/* used by KC85/4 and KC85/3 */
static int kc85_50hz_state;
static int kc85_15khz_state;
static int kc85_15khz_count;

 READ8_HANDLER ( kc85_unmapped_r )
{
	return 0x0ff;
}

#if 0
static OPBASE_HANDLER( kc85_3_opbaseoverride )
{
	memory_set_opbase_handler(0,0);

	kc85_3_update_0x00000();

	return (cpunum_get_reg(0, REG_PC) & 0x0ffff);
}


static OPBASE_HANDLER( kc85_4_opbaseoverride )
{
	memory_set_opbase_handler(0,0);
	
	kc85_4_update_0x00000();

	return (cpunum_get_reg(0, REG_PC) & 0x0ffff);
}
#endif


static void kc85_reset_timer_callback(int dummy)
{
	cpunum_set_reg(0, REG_PC, 0x0f000);
}

 READ8_HANDLER ( kc85_pio_data_r )
{
	return z80pio_d_r(0,offset);
}

 READ8_HANDLER ( kc85_pio_control_r )
{
	return z80pio_c_r(0,offset);
}



WRITE8_HANDLER ( kc85_pio_control_w )
{
   z80pio_c_w(0, offset, data);
}


 READ8_HANDLER ( kc85_ctc_r )
{
	unsigned char data;

	data = z80ctc_0_r(offset);
//#ifdef KC_KEYBOARD_DEBUG
	//logerror("ctc data r:%02x\n",data);
//#endif
	return data;
}

WRITE8_HANDLER ( kc85_ctc_w )
{
	//logerror("ctc data w:%02x\n",data);

	z80ctc_0_w(offset,data);
}


static void kc85_pio_interrupt(int state)
{
	cpunum_set_input_line(0, 0, state);
}

static void kc85_ctc_interrupt(int state)
{
	cpunum_set_input_line(0, 1, state);
}

/* callback for ardy output from PIO */
/* used in KC85/4 & KC85/3 cassette interface */
static void	kc85_pio_ardy_callback(int state)
{
	kc_ardy = state & 0x01;
#ifdef KC_DEBUG
	if (state)
	{
		logerror("PIO A Ready\n");
	}
#endif

}

/* callback for brdy output from PIO */
/* used in KC85/4 & KC85/3 keyboard interface */
static void kc85_pio_brdy_callback(int state)
{
	kc_brdy = state & 0x01;
#ifdef KC_DEBUG
	if (state)
	{
		logerror("PIO B Ready\n");
	}
#endif
}

static z80pio_interface kc85_pio_intf =
{
	kc85_pio_interrupt,		/* callback when change interrupt status */
	kc85_pio_ardy_callback,	/* portA ready active callback */
	kc85_pio_brdy_callback	/* portB ready active callback */
};

/* used in cassette write -> K0 */
static WRITE8_HANDLER(kc85_zc0_callback)
{


}

/* used in cassette write -> K1 */
static WRITE8_HANDLER(kc85_zc1_callback)
{

}


static void kc85_15khz_timer_callback(int dummy)
{
	/* toggle state of square wave */
	kc85_15khz_state^=1;

	/* set clock input for channel 2 and 3 to ctc */
	z80ctc_0_trg0_w(0,kc85_15khz_state);
	z80ctc_0_trg1_w(0,kc85_15khz_state);

	kc85_15khz_count++;

	if (kc85_15khz_count>=312)
	{
		kc85_15khz_count = 0;

		/* toggle state of square wave */
		kc85_50hz_state^=1;

		/* set clock input for channel 2 and 3 to ctc */
		z80ctc_0_trg2_w(0,kc85_50hz_state);
		z80ctc_0_trg3_w(0,kc85_50hz_state);
	}
}

/* video blink */
static WRITE8_HANDLER(kc85_zc2_callback)
{
	/* is blink enabled? */
	if (kc85_pio_data[1] & (1<<7))
	{
		/* yes */
		/* toggle state of blink to video hardware */
		kc85_video_set_blink_state(data);
	}
}

static z80ctc_interface	kc85_ctc_intf =
{
	1379310.344828,
	0,
    kc85_ctc_interrupt,
	kc85_zc0_callback,
	kc85_zc1_callback,
    kc85_zc2_callback
};

static void	kc85_common_init(void)
{
	z80pio_init(0, &kc85_pio_intf);
	z80ctc_init(0, &kc85_ctc_intf);

	z80ctc_reset(0);
	z80pio_reset(0);

	kc_cassette_init();
	kc_keyboard_init();

	/* kc85 has a 50hz input to the ctc channel 2 and 3 */
	/* channel 2 this controls the video colour flash */
	/* kc85 has a 15khz (?) input to the ctc channel 0 and 1 */
	/* channel 0 and channel 1 are used for cassette write */
	kc85_50hz_state = 0;
	kc85_15khz_state = 0;
	kc85_15khz_count = 0;
	timer_pulse(TIME_IN_HZ(15625), 0, kc85_15khz_timer_callback);
	timer_set(TIME_NOW, 0, kc85_reset_timer_callback);
	kc85_module_system_init();
}

/*****************************************************************/

MACHINE_RESET( kc85_4 )
{
	kc85_84_data = 0x0828;
	kc85_86_data = 0x063;
	/* enable CAOS rom in range 0x0e000-0x0ffff */
	/* ram0 enable, irm enable */
	kc85_pio_data[0] = 0x0f;
	kc85_pio_data[1] = 0x0f1;

	kc85_4_update_0x04000();
	kc85_4_update_0x08000();
	kc85_4_update_0x0c000();
	kc85_4_update_0x0e000();

	kc85_common_init();

	kc85_4_update_0x00000();


#if 0
	/* this is temporary. Normally when a Z80 is reset, it will
	execute address 0. It appears the KC85 series pages the rom
	at address 0x0000-0x01000 which has a single jump in it,
	can't see yet where it disables it later!!!! so for now
	here will be a override */
	memory_set_opbase_handler(0, kc85_4_opbaseoverride);
#endif
}

MACHINE_RESET( kc85_4d )
{
	machine_reset_kc85_4(machine);
	kc_disc_interface_init();
}

MACHINE_RESET( kc85_3 )
{
	kc85_pio_data[0] = 0x0f;
	kc85_pio_data[1] = 0x0f1;

	memory_set_bankptr(2,mess_ram+0x0c000);
	memory_set_bankptr(7,mess_ram+0x0c000);

	kc85_3_update_0x08000();
	kc85_3_update_0x0c000();
	kc85_3_update_0x0e000();

	kc85_common_init();

	kc85_3_update_0x00000();

#if 0
	/* this is temporary. Normally when a Z80 is reset, it will
	execute address 0. It appears the KC85 series pages the rom
	at address 0x0000-0x01000 which has a single jump in it,
	can't see yet where it disables it later!!!! so for now
	here will be a override */
	memory_set_opbase_handler(0, kc85_3_opbaseoverride);
#endif
}
