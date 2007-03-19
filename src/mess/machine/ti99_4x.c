/*
	Machine code for TI99/4, TI-99/4A, TI-99/8, and SNUG SGCPU (a.k.a. 99/4P).
	Raphael Nabet, 1999-2003.
	Some code was originally derived from Ed Swartz's V9T9.

	References:
	* The TI-99/4A Tech Pages.  Great site with complete technical description
	  of the TI99/4a.
		<http://www.nouspikel.com/ti99/titech.htm>
	* ftp.whtech.com has schematics and hardware documentations.  The documents
	  of most general interest are:
		<ftp://ftp.whtech.com//datasheets/Hardware manuals/99-4  console specification and schematic.pdf>
		<ftp://ftp.whtech.com//datasheets/Hardware manuals/99-4A Console and peripheral Expansion System Technical Data.pdf>
	  Schematics for various extension cards are available in the same directory.
	* V9T9 source code.  This older emulator was often fairly accurate.
	* Harald Glaab's site has software and documentation for the various SNUG
	  cards: 99/4P (nicknamed "SGCPU"), EVPC, BwG.
		<http://home.t-online.de/home/harald.glaab/snug/>
	* The TI-99/4 CPU ROM is the only source of information for the IR remote
	  handset controller protocol.  I have written a document that explains
	  this protocol.
	* TI-99/8 ROM source code (6 PC99 disk images):
		<ftp://ftp.whtech.com//source/998Source Disk1.DSK>
		...
		<ftp://ftp.whtech.com//source/998Source Disk6.DSK>
	* TI-99/8 schematics.  This is a valuable source of information, BUT the
	  schematics describe an early prototype (no Extended Basic, etc.).
		<ftp://ftp.whtech.com//datasheets & manuals/99-8 Computer/99-8 schematic.max>
	  I have written an index for the signals on these schematics.

Emulated:
	* All TI99 basic console hardware, except a few tricks in TMS9901 emulation.
	* Cartridges with ROM (either non-paged or paged), RAM (minimemory) and
	  GROM.  Supercart and MBX support are not finished.
	* Speech Synthesizer, with standard speech ROM (no speech ROM expansion).
	* Disk emulation (SSSD disk images with TI fdc, both SSSD and DSDD with
	  SNUG's BwG and HFDC fdcs).
	* Hard disk (HFDC and IDE).
	* Mechatronics mouse.

	Compatibility looks quite good.

Issues:
	* floppy disk timings are not emulated (general issue)

TODO:
	* support for other peripherals and DSRs as documentation permits
	* implement the EVPC palette chip
	* finish 99/4p support: ROM6, HSGPL (implemented, but not fully debugged)
	* save minimemory contents
*/

#include <math.h>
#include "driver.h"
#include "tms9901.h"
#include "video/tms9928a.h"
#include "video/v9938.h"
#include "audio/spchroms.h"
#include "devices/cassette.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "994x_ser.h"
#include "99_dsk.h"
#include "99_ide.h"
#include "99_hsgpl.h"
#include "99_usbsm.h"

#include "sound/tms5220.h"	/* for tms5220_set_variant() */
#include "sound/5220intf.h"
#include "sound/sn76496.h"

#include "devices/harddriv.h"	/* for device_init_mess_hd() */
#include "smc92x4.h"	/* for smc92x4_hd_load()/smc92x4_hd_unload() */


/* prototypes */
static READ16_HANDLER ( ti99_rspeech_r );
static WRITE16_HANDLER ( ti99_wspeech_w );

static void tms9901_set_int1(int state);
static void tms9901_interrupt_callback(int intreq, int ic);
static int ti99_R9901_0(int offset);
static int ti99_R9901_1(int offset);
static int ti99_R9901_2(int offset);
static int ti99_R9901_3(int offset);

static void ti99_handset_set_ack(int offset, int data);
static void ti99_handset_task(void);
static void mecmouse_poll(void);
static void ti99_KeyC(int offset, int data);
static void ti99_AlphaW(int offset, int data);

static int ti99_8_R9901_0(int offset);
static int ti99_8_R9901_1(int offset);
static void ti99_8_KeyC(int offset, int data);
static void ti99_8_WCRUS(int offset, int data);
static void ti99_8_PTGEN(int offset, int data);

static void ti99_CS_motor(int offset, int data);
static void ti99_audio_gate(int offset, int data);
static void ti99_CS_output(int offset, int data);

static void ti99_8_internal_dsr_init(void);

static void ti99_4p_internal_dsr_init(void);
static void ti99_TIxram_init(void);
static void ti99_sAMSxram_init(void);
static void ti99_4p_mapper_init(void);
static void ti99_myarcxram_init(void);
static void ti99_evpc_init(void);

/*
	pointer to extended RAM area
*/
static UINT16 *xRAM_ptr;

/*
	Configuration
*/
/* model of ti99 in emulation */
static enum
{
	model_99_4,
	model_99_4a,
	model_99_8,
	model_99_4p
} ti99_model;
/* memory extension type */
static xRAM_kind_t xRAM_kind;
/* TRUE if speech synthesizer present */
static char has_speech;
/* floppy disk controller type */
static fdc_kind_t fdc_kind;
/* TRUE if ide card present */
static char has_ide;
/* TRUE if evpc card present */
static char has_evpc;
/* TRUE if rs232 card present */
static char has_rs232;
/* TRUE if ti99/4 IR remote handset present */
static char has_handset;
/* TRUE if hsgpl card present */
static char has_hsgpl;
/* TRUE if mechatronics mouse present */
static char has_mecmouse;
/* TRUE if usb-sm card present */
static char has_usb_sm;


/* tms9901 setup */
static const tms9901reset_param tms9901reset_param_ti99_4x =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_R9901_0,
		ti99_R9901_1,
		ti99_R9901_2,
		ti99_R9901_3
	},

	{	/* write handlers */
		ti99_handset_set_ack,
		NULL,
		ti99_KeyC,
		ti99_KeyC,
		ti99_KeyC,
		ti99_AlphaW,
		ti99_CS_motor,
		ti99_CS_motor,
		ti99_audio_gate,
		ti99_CS_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3MHz */
	3000000.
};

static const tms9901reset_param tms9901reset_param_ti99_8 =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	{	/* read handlers */
		ti99_8_R9901_0,
		ti99_8_R9901_1,
		ti99_R9901_2,
		ti99_R9901_3
	},

	{	/* write handlers */
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_KeyC,
		ti99_8_WCRUS,
		ti99_8_PTGEN,
		ti99_CS_motor,
		NULL,
		ti99_audio_gate,
		ti99_CS_output,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* interrupt handler */
	tms9901_interrupt_callback,

	/* clock rate = 3.58MHz??? 2.68MHz??? */
	/*3579545.*/2684658.75
};

/* handset interface */
static int handset_buf;
static int handset_buflen;
static int handset_clock;
static int handset_ack;
enum
{
	max_handsets = 4
};

/* mechatronics  */
static int mecmouse_sel;
static int mecmouse_read_y;
static int mecmouse_x, mecmouse_y;
static int mecmouse_x_buf, mecmouse_y_buf;

/* keyboard interface */
static int KeyCol;
static int AlphaLockLine;

/*
	GROM support.

In short:

	TI99/4x hardware supports GROMs.  GROMs are slow ROM devices, which are
	interfaced via a 8-bit data bus, and include an internal address pointer
	which is incremented after each read.  This implies that accesses are
	faster when reading consecutive bytes, although the address pointer can be
	read and written at any time.

	They are generally used to store programs in GPL (Graphic Programming
	Language: a proprietary, interpreted language - the interpreter takes most
	of a TI99/4(a) CPU ROMs).  They can used to store large pieces of data,
	too.

	Both TI-99/4 and TI-99/4a include three GROMs, with some start-up code,
	system routines and TI-Basic.  TI99/4 includes an additional Equation
	Editor.  According to the preliminary schematics found on ftp.whtech.com,
	TI-99/8 includes the three standard GROMs and 16 GROMs for the UCSD
	p-system.  TI99/2 does not include GROMs at all, and was not designed to
	support any, although it should be relatively easy to create an expansion
	card with the GPL interpreter and a /4a cartridge port.

The simple way:

	Each GROM is logically 8kb long.

	Communication with GROM is done with 4 memory-mapped locations.  You can read or write
	a 16-bit address pointer, and read data from GROMs.  One register allows to write data, too,
	which would support some GRAMs, but AFAIK TI never built such GRAMs (although docs from TI
	do refer to this possibility).

	Since address are 16-bit long, you can have up to 8 GROMs.  So, a cartridge may
	include up to 5 GROMs.  (Actually, there is a way more GROMs can be used - see below...)

	The address pointer is incremented after each GROM operation, but it will always remain
	within the bounds of the currently selected GROM (e.g. after 0x3fff comes 0x2000).

Some details:

	Original TI-built GROM are 6kb long, but take 8kb in address space.  The extra 2kb can be
	read, and follow the following formula:
		GROM[0x1800+offset] = GROM[0x0800+offset] | GROM[0x1000+offset];
	(sounds like address decoding is incomplete - we are lucky we don't burn any silicon when
	doing so...)

	Needless to say, some hackers simulated 8kb GRAMs and GROMs with normal RAM/PROM chips and
	glue logic.

GPL ports:

	When accessing the GROMs registers, 8 address bits (cpu_addr & 0x03FC) may
	be used as a port number, which permits the use of up to 256 independant
	GROM ports, with 64kb of address space in each.  TI99/4(a) ROMs can take
	advantage of the first 16 ports: it will look for GPL programs in every
	GROM of the 16 first ports.  Additionally, while the other 240 ports cannot
	contain standard GROMs with GPL code, they may still contain custom GROMs
	with data.

	Note, however, that the TI99/4(a) console does not decode the page number, so console GROMs
	occupy the first 24kb of EVERY port, and cartridge GROMs occupy the next 40kb of EVERY port
	(with the exception of one demonstration from TI that implements several distinct ports).
	(Note that the TI99/8 console does have the required decoder.)  Fortunately, GROM drivers
	have a relatively high impedance, and therefore extension cards can use TTL drivers to impose
	another value on the data bus with no risk of burning the console GROMs.  This hack permits
	the addition of additionnal GROMs in other ports, with the only side effect that whenever
	the address pointer in port N is altered, the address pointer of console GROMs in port 0
	is altered, too.  Overriding the system GROMs with custom code is possible, too (some hackers
	have done so), but I would not recommended it, as connecting such a device to a TI99/8 might
	burn some drivers.

	The p-code card (-> UCSD Pascal system) contains 8 GROMs, all in port 16.  This port is not
	in the range recognized by the TI ROMs (0-15), and therefore it is used by the p-code DSR ROMs
	as custom data.  Additionally, some hackers used the extra ports to implement "GRAM" devices.
*/
/* GROM_port_t: descriptor for a port of 8 GROMs */
typedef struct GROM_port_t
{
	/* pointer to GROM data */
	UINT8 *data_ptr;
	/* current address pointer for the active GROM in port (16 bits) */
	unsigned int addr;
	/* GROM data buffer */
	UINT8 buf;
	/* internal flip-flops that are set after the first access to the GROM
	address so that next access is mapped to the LSB, and cleared after each
	data access */
	char raddr_LSB, waddr_LSB;
} GROM_port_t;

/* descriptor for console GROMs */
GROM_port_t console_GROMs;

/* true if hsgpl is enabled (i.e. has_hsgpl is true and hsgpl cru bit crdena is
set) */
static char hsgpl_crdena;

/*
	Cartridge support
*/
/* pointer on two 8kb cartridge pages */
static UINT16 *cartridge_pages[2] = {NULL, NULL};
static UINT8 *cartridge_pages_8[2] = {NULL, NULL};
/* flag: TRUE if the cartridge is minimemory (4kb of battery-backed SRAM in 0x5000-0x5fff) */
static char cartridge_minimemory = FALSE;
/* flag: TRUE if the cartridge is paged */
static char cartridge_paged = FALSE;
/* flag: TRUE if the cartridge is MBX-style */
static char cartridge_mbx = FALSE;
/* flag on the data for the current page (cartridge_pages[0] if cartridge is not paged) */
static UINT16 *current_page_ptr;
static UINT8 *current_page_ptr_8;
/* keep track of cart file types - required for cleanup... */
typedef enum slot_type_t { SLOT_EMPTY = -1, SLOT_GROM = 0, SLOT_CROM = 1, SLOT_DROM = 2, SLOT_MINIMEM = 3, SLOT_MBX = 4 } slot_type_t;
static slot_type_t slot_type[3] = { SLOT_EMPTY, SLOT_EMPTY, SLOT_EMPTY};

/* true if 99/4p rom6 is enabled */
static int ti99_4p_internal_rom6_enable;
/* pointer to the ROM6 data */
static UINT16 *ti99_4p_internal_ROM6;


/* ti99/8 hardware */
static UINT8 ti99_8_CRUS;

static UINT32 ti99_8_mapper_regs[16];
static UINT8 ti99_8_mapper_status;
enum
{
	ms_WPE = 0x80,	/* Write-Protect Error */
	ms_XCE = 0x40,	/* eXeCute Error */
	ms_RPE = 0x20	/* Read-Protect Error */
};

static UINT8 *xRAM_ptr_8;
static UINT8 *ROM0_ptr_8;
static UINT8 *ROM1_ptr_8;
static UINT8 *ROM2_ptr_8;
static UINT8 *ROM3_ptr_8;
static UINT8 *sRAM_ptr_8;

/* tms9900_ICount: used to implement memory waitstates (hack) */
/* tms9995_ICount: used to implement memory waitstates (hack) */
/* NPW 23-Feb-2004 - externs no longer needed because we now use activecpu_adjust_icount() */



/*===========================================================================*/
/*
	General purpose code:
	initialization, cart loading, etc.
*/

DRIVER_INIT( ti99_4 )
{
	int i, j;


	ti99_model = model_99_4;
	has_evpc = FALSE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram);
	console_GROMs.data_ptr = memory_region(region_grom);

	/* Generate missing chunk of each console GROMs */
	for (i=0; i<2; i++)
		for (j=0; j<0x800; j++)
		{
			console_GROMs.data_ptr[0x2000*i+0x1800+j] = console_GROMs.data_ptr[0x2000*i+0x0800+j]
															| console_GROMs.data_ptr[0x2000*i+0x1000+j];
		}
}

DRIVER_INIT( ti99_4a )
{
	ti99_model = model_99_4a;
	has_evpc = FALSE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram);
	console_GROMs.data_ptr = memory_region(region_grom);
}

DRIVER_INIT( ti99_4ev )
{
	ti99_model = model_99_4a;
	has_evpc = TRUE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram);
	cartridge_pages[0] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart);
	cartridge_pages[1] = (UINT16 *) (memory_region(REGION_CPU1)+offset_cart + 0x2000);
	console_GROMs.data_ptr = memory_region(region_grom);
}

DRIVER_INIT( ti99_8 )
{
	ti99_model = model_99_8;
	has_evpc = FALSE;

	/* set up memory pointers */
	xRAM_ptr_8 = memory_region(REGION_CPU1) + offset_xram_8;
	ROM0_ptr_8 = memory_region(REGION_CPU1) + offset_rom0_8;
	ROM1_ptr_8 = ROM0_ptr_8 + 0x2000;
	ROM2_ptr_8 = ROM1_ptr_8 + 0x2000;
	ROM3_ptr_8 = ROM2_ptr_8 + 0x2000;
	sRAM_ptr_8 = memory_region(REGION_CPU1) + offset_sram_8;

	cartridge_pages_8[0] = memory_region(REGION_CPU1)+offset_cart_8;
	cartridge_pages_8[1] = memory_region(REGION_CPU1)+offset_cart_8 + 0x2000;
	console_GROMs.data_ptr = memory_region(region_grom);
}

DRIVER_INIT( ti99_4p )
{
	ti99_model = model_99_4p;
	has_evpc = TRUE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(REGION_CPU1) + offset_xram_4p);
	/*console_GROMs.data_ptr = memory_region(region_grom);*/
}

DEVICE_INIT( ti99_cart )
{
	int id = image_index_in_device(image);
	cartridge_pages[id] = (UINT16 *) (memory_region(REGION_CPU1) + offset_cart + (id * 0x2000));
	return INIT_PASS;
}

/*
	Load ROM.  All files are in raw binary format.
	1st ROM: GROM (up to 40kb)
	2nd ROM: CPU ROM (8kb)
	3rd ROM: CPU ROM, 2nd page (8kb)

	We don't need to support 99/4p, as it has no cartridge port.
*/
DEVICE_LOAD( ti99_cart )
{
	/* Trick - we identify file types according to their extension */
	/* Note that if we do not recognize the extension, we revert to the slot location <-> type
	scheme.  I do this because the extension concept is quite unfamiliar to mac people
	(I am dead serious). */
	/* Original idea by Norberto Bensa */
	const char *name = image_filename(image);
	const char *ch, *ch2;
	int id = image_index_in_device(image);
	int i;
	slot_type_t type = (slot_type_t) id;

	/* There is a circuitry in TI99/4(a) that resets the console when a
	cartridge is inserted or removed.  We emulate this instead of resetting the
	emulator (which is the default in MESS). */
	/*cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
	tms9901_reset(0);
	if (! has_evpc)
		TMS9928A_reset();
	if (has_evpc)
		v9938_reset();*/

	ch = strrchr(name, '.');
	ch2 = (ch-1 >= name) ? ch-1 : "";

	if (ch)
	{
		if ((! mame_stricmp(ch2, "g.bin")) || (! mame_stricmp(ch, ".grom")) || (! mame_stricmp(ch, ".g")))
		{
			/* grom */
			type = SLOT_GROM;
		}
		else if ((! mame_stricmp(ch2, "c.bin")) || (! mame_stricmp(ch, ".crom")) || (! mame_stricmp(ch, ".c")))
		{
			/* rom first page */
			type = SLOT_CROM;
		}
		else if ((! mame_stricmp(ch2, "d.bin")) || (! mame_stricmp(ch, ".drom")) || (! mame_stricmp(ch, ".d")))
		{
			/* rom second page */
			type = SLOT_DROM;
		}
		else if ((! mame_stricmp(ch2, "m.bin")) || (! mame_stricmp(ch, ".mrom")) || (! mame_stricmp(ch, ".m")))
		{
			/* rom minimemory  */
			type = SLOT_MINIMEM;
		}
		else if ((! mame_stricmp(ch2, "b.bin")) || (! mame_stricmp(ch, ".brom")) || (! mame_stricmp(ch, ".b")))
		{
			/* rom MBX  */
			type = SLOT_MBX;
		}
	}

	slot_type[id] = type;

	switch (type)
	{
	case SLOT_EMPTY:
		break;

	case SLOT_GROM:
		image_fread(image, memory_region(region_grom) + 0x6000, 0xA000);
		break;

	case SLOT_MINIMEM:
		cartridge_minimemory = TRUE;
	case SLOT_MBX:
		if (type == SLOT_MBX)
			cartridge_mbx = TRUE;
	case SLOT_CROM:
		if (ti99_model == model_99_8)
		{
			image_fread(image, cartridge_pages_8[0], 0x2000);
			current_page_ptr_8 = cartridge_pages_8[0];
		}
		else
		{
			image_fread(image, cartridge_pages[0], 0x2000);
			for (i = 0; i < 0x1000; i++)
				cartridge_pages[0][i] = BIG_ENDIANIZE_INT16(cartridge_pages[0][i]);
			current_page_ptr = cartridge_pages[0];
		}
		break;

	case SLOT_DROM:
		cartridge_paged = TRUE;
		if (ti99_model == model_99_8)
		{
			image_fread(image, cartridge_pages_8[1], 0x2000);
			current_page_ptr_8 = cartridge_pages_8[0];
		}
		else
		{
			image_fread(image, cartridge_pages[1], 0x2000);
			for (i = 0; i < 0x1000; i++)
				cartridge_pages[1][i] = BIG_ENDIANIZE_INT16(cartridge_pages[1][i]);
			current_page_ptr = cartridge_pages[0];
		}
		break;
	}

	return INIT_PASS;
}

DEVICE_UNLOAD( ti99_cart )
{
	int id = image_index_in_device(image);

	/* There is a circuitry in TI99/4(a) that resets the console when a
	cartridge is inserted or removed.  We emulate this instead of resetting the
	emulator (which is the default in MESS). */
	/*cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
	tms9901_reset(0);
	if (! has_evpc)
		TMS9928A_reset();
	if (has_evpc)
		v9938_reset();*/

	switch (slot_type[id])
	{
	case SLOT_EMPTY:
		break;

	case SLOT_GROM:
		memset(memory_region(region_grom) + 0x6000, 0, 0xA000);
		break;

	case SLOT_MINIMEM:
		cartridge_minimemory = FALSE;
		/* we should insert some code to save the minimem contents... */
	case SLOT_MBX:
		if (slot_type[id] == SLOT_MBX)
			cartridge_mbx = FALSE;
			/* maybe we should insert some code to save the memory contents... */
	case SLOT_CROM:
		if (ti99_model == model_99_8)
		{
			memset(cartridge_pages_8[0], 0, 0x2000);
		}
		else
		{
			memset(cartridge_pages[0], 0, 0x2000);
		}
		break;

	case SLOT_DROM:
		cartridge_paged = FALSE;
		if (ti99_model == model_99_8)
		{
			current_page_ptr_8 = cartridge_pages_8[0];
		}
		else
		{
			current_page_ptr = cartridge_pages[0];
		}
		break;
	}

	slot_type[id] = SLOT_EMPTY;
}

DEVICE_LOAD( ti99_hd )
{
	int id = image_index_in_device(image);
	return smc92x4_hd_load(image, id);
}

DEVICE_UNLOAD( ti99_hd )
{
	int id = image_index_in_device(image);
	smc92x4_hd_unload(image, id);
}

static const TMS9928a_interface tms9918_interface =
{
	TMS99x8,
	0x4000,
	0, 0,
	tms9901_set_int2
};

MACHINE_START( ti99_4_60hz )
{
	TMS9928A_configure(&tms9918_interface);
	return 0;
}

static const TMS9928a_interface tms9929_interface =
{
	TMS9929,
	0x4000,
	0, 0,
	tms9901_set_int2
};

MACHINE_START( ti99_4_50hz )
{
	TMS9928A_configure(&tms9929_interface);
	return 0;
}

static const TMS9928a_interface tms9918a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	tms9901_set_int2
};

MACHINE_START( ti99_4a_60hz )
{
	TMS9928A_configure(&tms9918a_interface);
	return 0;
}

static const TMS9928a_interface tms9929a_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	tms9901_set_int2
};

MACHINE_START( ti99_4a_50hz )
{
	TMS9928A_configure(&tms9929a_interface);
	return 0;
}

/*
	ti99_init_machine(); called before ti99_load_rom...
*/
MACHINE_RESET( ti99 )
{
	/*console_GROMs.data_ptr = memory_region(region_grom);*/
	console_GROMs.addr = 0;

	if (ti99_model == model_99_8)
	{
		/* ... */
		ti99_8_CRUS = 1;		/* ??? maybe there is a pull-up */
	}
	else if (ti99_model == model_99_4p)
	{
		/* set up system ROM and scratch pad pointers */
		memory_set_bankptr(1, memory_region(REGION_CPU1) + offset_rom0_4p);	/* system ROM */
		memory_set_bankptr(2, memory_region(REGION_CPU1) + offset_sram_4p);	/* scratch pad */
		memory_set_bankptr(11, memory_region(REGION_CPU1) + offset_dram_4p);	/* extra RAM for debugger */
	}
	else
	{
		/* set up scratch pad pointer */
		memory_set_bankptr(1, memory_region(REGION_CPU1) + offset_sram);
	}

	if (ti99_model != model_99_4p)
	{	/* reset cartridge mapper */
		if (ti99_model == model_99_8)
			current_page_ptr_8 = cartridge_pages_8[0];
		else
			current_page_ptr = cartridge_pages[0];
	}

	/* init tms9901 */
	if (ti99_model == model_99_8)
		tms9901_init(0, & tms9901reset_param_ti99_8);
	else
		tms9901_init(0, & tms9901reset_param_ti99_4x);

	if (! has_evpc)
		TMS9928A_reset();
	if (has_evpc)
		v9938_reset();

	/* clear keyboard interface state (probably overkill, but can't harm) */
	KeyCol = 0;
	AlphaLockLine = 0;

	/* reset handset */
	handset_buf = 0;
	handset_buflen = 0;
	handset_clock = 0;
	handset_ack = 0;
	tms9901_set_single_int(0, 12, 0);

	/* read config */
	if (ti99_model == model_99_8)
		xRAM_kind = xRAM_kind_99_8;			/* hack */
	else if (ti99_model == model_99_4p)
		xRAM_kind = xRAM_kind_99_4p_1Mb;	/* hack */
	else
		xRAM_kind = (readinputport(input_port_config) >> config_xRAM_bit) & config_xRAM_mask;
	if (ti99_model == model_99_8)
		has_speech = TRUE;
	else
		has_speech = (readinputport(input_port_config) >> config_speech_bit) & config_speech_mask;
	fdc_kind = (readinputport(input_port_config) >> config_fdc_bit) & config_fdc_mask;
	has_ide = (readinputport(input_port_config) >> config_ide_bit) & config_ide_mask;
	has_rs232 = (readinputport(input_port_config) >> config_rs232_bit) & config_rs232_mask;
	has_handset = (ti99_model == model_99_4) && ((readinputport(input_port_config) >> config_handsets_bit) & config_handsets_mask);
	has_hsgpl = (ti99_model == model_99_4p) || ((readinputport(input_port_config) >> config_hsgpl_bit) & config_hsgpl_mask);
	has_usb_sm = (readinputport(input_port_config) >> config_usbsm_bit) & config_usbsm_mask;

	/* set up optional expansion hardware */
	ti99_peb_init(ti99_model == model_99_4p, tms9901_set_int1, NULL);

	if (ti99_model == model_99_8)
		ti99_8_internal_dsr_init();

	if (ti99_model == model_99_4p)
		ti99_4p_internal_dsr_init();

	if (has_speech)
	{
		spchroms_interface speech_intf = { region_speech_rom };

		spchroms_config(& speech_intf);

		if (ti99_model != model_99_8)
		{
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x93ff, 0, 0, ti99_rspeech_r);
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x9400, 0x97ff, 0, 0, ti99_wspeech_w);

			sndti_set_info_int(SOUND_TMS5220, 0, SNDINFO_INT_TMS5220_VARIANT, variant_tms0285);
		}
	}
	else
	{
		if (ti99_model != model_99_8)
		{
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x93ff, 0, 0, ti99_nop_8_r);
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x9400, 0x97ff, 0, 0, ti99_nop_8_w);
		}
	}

	switch (xRAM_kind)
	{
	case xRAM_kind_none:
	default:
		/* reset mem handler to none */
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_nop_8_r);
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_nop_8_w);
		memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_nop_8_r);
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_nop_8_w);
		break;
	case xRAM_kind_TI:
		ti99_TIxram_init();
		break;
	case xRAM_kind_super_AMS:
		ti99_sAMSxram_init();
		break;
	case xRAM_kind_99_4p_1Mb:
		ti99_4p_mapper_init();
		break;
	case xRAM_kind_foundation_128k:
	case xRAM_kind_foundation_512k:
	case xRAM_kind_myarc_128k:
	case xRAM_kind_myarc_512k:
		ti99_myarcxram_init();
		break;
	case xRAM_kind_99_8:
		/* nothing needs to be done */
		break;
	}

	switch (fdc_kind)
	{
	case fdc_kind_TI:
		ti99_fdc_init();
		break;
#if HAS_99CCFDC
	case fdc_kind_CC:
		ti99_ccfdc_init();
		break;
#endif
	case fdc_kind_BwG:
		ti99_bwg_init();
		break;
	case fdc_kind_hfdc:
		ti99_hfdc_init();
		break;
	case fdc_kind_none:
		break;
	}

	if (has_ide)
	{
		ti99_ide_init(ti99_model == model_99_8);
		ti99_ide_load_memcard();
	}

	if (has_rs232)
		ti99_rs232_init();

	if (has_hsgpl)
	{
		ti99_hsgpl_init();
		ti99_hsgpl_load_memcard();
	}
	else
		hsgpl_crdena = 0;

	if (has_usb_sm)
		ti99_usbsm_init(ti99_model == model_99_8);

	if (has_evpc)
		ti99_evpc_init();

	/* initialize mechatronics mouse */
	mecmouse_sel = 0;
	mecmouse_read_y = 0;
	mecmouse_x = 0;
	mecmouse_y = 0;
}

void machine_stop_ti99(void)
{
	if (has_ide)
		ti99_ide_save_memcard();

	if (has_hsgpl)
		ti99_hsgpl_save_memcard();

	if (has_rs232)
		ti99_rs232_cleanup();

	tms9901_cleanup(0);
}


/*
	video initialization.
*/
VIDEO_START( ti99_4ev )
{
	return v9938_init(Machine, MODEL_V9938, 0x20000, tms9901_set_int2);	/* v38 with 128 kb of video RAM */
}

/*
	VBL interrupt  (mmm...  actually, it happens when the beam enters the lower
	border, so it is not a genuine VBI, but who cares ?)
*/
void ti99_vblank_interrupt(void)
{
	TMS9928A_interrupt();
	if (has_handset)
		ti99_handset_task();
	has_mecmouse = (readinputport(input_port_config) >> config_mecmouse_bit) & config_mecmouse_mask;
	if (has_mecmouse)
		mecmouse_poll();
}

void ti99_4ev_hblank_interrupt(void)
{
	static int line_count;
	v9938_interrupt();
	if (++line_count == 262)
	{
		line_count = 0;
		has_mecmouse = (readinputport(input_port_config) >> config_mecmouse_bit) & config_mecmouse_mask;
		if (has_mecmouse)
			mecmouse_poll();
	}
}

/*
	Backdoor function that is called by hsgpl, so that the machine code knows
	whether hsgpl or console GROMs are active
*/
void set_hsgpl_crdena(int data)
{
	hsgpl_crdena = (data != 0);
}

/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark MEMORY HANDLERS
#endif
/*
	Memory handlers.

	TODO:
	* save minimem RAM when quitting
	* actually implement GRAM support and GPL port support
*/

/*
	Same as MRA16_NOP, but with an additionnal delay caused by the 16->8 bit
	bus multiplexer.
*/
READ16_HANDLER ( ti99_nop_8_r )
{
	activecpu_adjust_icount(-4);

	return (0);
}

WRITE16_HANDLER ( ti99_nop_8_w )
{
	activecpu_adjust_icount(-4);
}

/*
	TI-99 cartridge port: attached to the 8-bit bus.
	
	Cartridges are usually made of ROM, through a few cartridges include some
	RAM as well.  Bank switching is common: I know three banking schemes,
	namely TI, MBX and Supercart.  The TI scheme writes a dummy value in
	cartridge space, and the write address gives the bank number.  The MBX
	scheme has a cartridge bank register at 0x6fff.  I think the Supercart
	scheme has a cartridge bank register in CRU space, but I don't what the
	register address is, and it is therefore not emulated.

	HSGPL maps here, too, in order to emulate cartridge ROM/RAM.
*/
READ16_HANDLER ( ti99_cart_r )
{
	activecpu_adjust_icount(-4);

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		return ti99_hsgpl_rom6_r(offset, mem_mask);

	if (cartridge_mbx && (offset >= 0x0600) && (offset <= 0x07fe))
		return (cartridge_pages[0])[offset];

	return current_page_ptr[offset];
}

WRITE16_HANDLER ( ti99_cart_w )
{
	activecpu_adjust_icount(-4);

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		ti99_hsgpl_rom6_w(offset, data, mem_mask);
	else if (cartridge_minimemory && offset >= 0x800)
		/* handle minimem RAM */
		COMBINE_DATA(current_page_ptr+offset);
	else if (cartridge_mbx)
	{	/* handle MBX cart */
		/* RAM in 0x6c00-0x6ffd (presumably non-paged) */
		/* mapper at 0x6ffe */
		if ((offset >= 0x0600) && (offset <= 0x07fe))
			COMBINE_DATA(cartridge_pages[0]+offset);
		else if ((offset == 0x07ff) && ACCESSING_MSB16)
			current_page_ptr = cartridge_pages[cartridge_paged ? ((data >> 8) & 1) : 0];
	}
	else if (cartridge_paged)
		/* handle pager */
		current_page_ptr = cartridge_pages[offset & 1];
}

/*
	99/4p cartridge space: mapped either to internal ROM6 or to HSGPL.
*/
READ16_HANDLER ( ti99_4p_cart_r )
{
	if (ti99_4p_internal_rom6_enable)
		return ti99_4p_internal_ROM6[offset];

	activecpu_adjust_icount(-4);

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		return ti99_hsgpl_rom6_r(offset, mem_mask);

	return 0;
}

WRITE16_HANDLER ( ti99_4p_cart_w )
{
	if (ti99_4p_internal_rom6_enable)
	{
		ti99_4p_internal_ROM6 = (UINT16 *) (memory_region(REGION_CPU1) + (offset & 1) ? offset_rom6b_4p : offset_rom6_4p);
		return;
	}

	activecpu_adjust_icount(-4);

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		ti99_hsgpl_rom6_w(offset, data, mem_mask);
}

/*---------------------------------------------------------------------------*/
/*
	memory-mapped register handlers.

Theory:

	These are registers to communicate with several peripherals.
	These registers are all 8-bit-wide, and are located at even adresses between
	0x8400 and 0x9FFE.
	The registers are identified by (addr & 1C00), and, for VDP and GPL access,
	by (addr & 2).  These registers are either read-only or write-only.  (Or,
	more accurately, (addr & 4000) disables register read, whereas
	!(addr & 4000) disables register write.)

	Memory mapped registers list:
	- write sound chip. (8400-87FE)
	- read VDP memory. (8800-8BFC), (addr&2) == 0
	- read VDP status. (8802-8BFE), (addr&2) == 2
	- write VDP memory. (8C00-8FFC), (addr&2) == 0
	- write VDP address and VDP register. (8C02-8FFE), (addr&2) == 2
	- read speech synthesis chip. (9000-93FE)
	- write speech synthesis chip. (9400-97FE)
	- read GPL memory. (9800-9BFC), (addr&2) == 0 (1)
	- read GPL adress. (9802-9BFE), (addr&2) == 2 (1)
	- write GPL memory. (9C00-9FFC), (addr&2) == 0 (1)
	- write GPL adress. (9C02-9FFE), (addr&2) == 2 (1)

(1) on some hardware, (addr & 0x3FC) provides a GPL page number.
*/

/*
	TMS9919 sound chip write
*/
WRITE16_HANDLER ( ti99_wsnd_w )
{
	activecpu_adjust_icount(-4);

	SN76496_0_w(offset, (data >> 8) & 0xff);
}

/*
	TMS9918(A)/9928(A)/9929(A) VDP read
*/
READ16_HANDLER ( ti99_rvdp_r )
{
	activecpu_adjust_icount(-4);

	if (offset & 1)
	{	/* read VDP status */
		return ((int) TMS9928A_register_r(0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) TMS9928A_vram_r(0)) << 8;
	}
}

/*
	TMS9918(A)/9928(A)/9929(A) vdp write
*/
WRITE16_HANDLER ( ti99_wvdp_w )
{
	activecpu_adjust_icount(-4);

	if (offset & 1)
	{	/* write VDP address */
		TMS9928A_register_w(0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(0, (data >> 8) & 0xff);
	}
}

/*
	V38 VDP read
*/
READ16_HANDLER ( ti99_rv38_r )
{
	activecpu_adjust_icount(-4);

	if (offset & 1)
	{	/* read VDP status */
		return ((int) v9938_status_r(0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) v9938_vram_r(0)) << 8;
	}
}

/*
	V38 vdp write
*/
WRITE16_HANDLER ( ti99_wv38_w )
{
	activecpu_adjust_icount(-4);

	switch (offset & 3)
	{
	case 0:
		/* write VDP data */
		v9938_vram_w(0, (data >> 8) & 0xff);
		break;
	case 1:
		/* write VDP address */
		v9938_command_w(0, (data >> 8) & 0xff);
		break;
	case 2:
		/* write VDP palette */
		v9938_palette_w(0, (data >> 8) & 0xff);
		break;
	case 3:
		/* write VDP register */
		v9938_register_w(0, (data >> 8) & 0xff);
		break;
	}
}

/*
	TMS0285 speech chip read
*/
static READ16_HANDLER ( ti99_rspeech_r )
{
	activecpu_adjust_icount(-(18+3));		/* this is just a minimum, it can be more */

	return ((int) tms5220_status_r(offset)) << 8;
}

#if 0

static void speech_kludge_callback(int dummy)
{
	if (! tms5220_ready_r())
	{
		/* Weirdly enough, we are always seeing some problems even though
		everything is working fine. */
		double time_to_ready = tms5220_time_to_ready();
		logerror("ti99/4a speech says aaargh!\n");
		logerror("(time to ready: %f -> %d)\n", time_to_ready, (int) ceil(3000000*time_to_ready));
	}
}

#endif

/*
	TMS0285 speech chip write
*/
static WRITE16_HANDLER ( ti99_wspeech_w )
{
	activecpu_adjust_icount(-(54+3));		/* this is just an approx. minimum, it can be much more */

#if 1
	/* the stupid design of the tms5220 core means that ready is cleared when
	there are 15 bytes in FIFO.  It should be 16.  Of course, if it were the
	case, we would need to store the value on the bus, which would be more
	complex. */
	if (! tms5220_ready_r())
	{
		double time_to_ready = tms5220_time_to_ready();
		int cycles_to_ready = ceil(TIME_TO_CYCLES(0, time_to_ready));

		logerror("time to ready: %f -> %d\n", time_to_ready, (int) cycles_to_ready);

		activecpu_adjust_icount(-cycles_to_ready);
		timer_set(TIME_NOW, 0, /*speech_kludge_callback*/NULL);
	}
#endif

	tms5220_data_w(offset, (data >> 8) & 0xff);
}

/*
	GPL read
*/
READ16_HANDLER ( ti99_rgpl_r )
{
	int reply;


	activecpu_adjust_icount(-4 /*20+3*/);		/* from 4 to 23? */

	if (offset & 1)
	{	/* read GPL address */
		/* the console GROMs are always affected */
		console_GROMs.waddr_LSB = FALSE;	/* right??? */

		if (console_GROMs.raddr_LSB)
		{
			reply = (console_GROMs.addr << 8) & 0xff00;
			console_GROMs.raddr_LSB = FALSE;
		}
		else
		{
			reply = console_GROMs.addr & 0xff00;
			console_GROMs.raddr_LSB = TRUE;
		}
	}
	else
	{	/* read GPL data */
		/* the console GROMs are always affected */
		reply = ((int) console_GROMs.buf) << 8;	/* retreive buffer */

		/* read ahead */
		console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
		console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;
	}

	if (hsgpl_crdena)
		/* hsgpl buffers are stronger than console GROM buffers */
		reply = ti99_hsgpl_gpl_r(offset, mem_mask);

	return reply;
}

/*
	GPL write
*/
WRITE16_HANDLER ( ti99_wgpl_w )
{
	activecpu_adjust_icount(-4/*20+3*/);		/* from 4 to 23? */

	if (offset & 1)
	{	/* write GPL address */
		/* the console GROMs are always affected */
		console_GROMs.raddr_LSB = FALSE;

		if (console_GROMs.waddr_LSB)
		{
			console_GROMs.addr = (console_GROMs.addr & 0xFF00) | ((data >> 8) & 0xFF);

			/* read ahead */
			console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
			console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);

			console_GROMs.waddr_LSB = FALSE;
		}
		else
		{
			console_GROMs.addr = (data & 0xFF00) | (console_GROMs.addr & 0xFF);

			console_GROMs.waddr_LSB = TRUE;
		}

	}
	else
	{	/* write GPL data */
		/* the console GROMs are always affected */
		/* BTW, console GROMs are never GRAMs, therefore there is no need to
		actually write anything */
		/* read ahead */
		console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
		console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;

	}

	if (hsgpl_crdena)
		ti99_hsgpl_gpl_w(offset, data, mem_mask);
}

/*
	GPL read
*/
READ16_HANDLER ( ti99_4p_rgpl_r )
{
	activecpu_adjust_icount(-4);		/* HSGPL is located on 8-bit bus? */

	return /*hsgpl_crdena ?*/ ti99_hsgpl_gpl_r(offset, mem_mask) /*: 0*/;
}

/*
	GPL write
*/
WRITE16_HANDLER ( ti99_4p_wgpl_w )
{
	activecpu_adjust_icount(-4);		/* HSGPL is located on 8-bit bus? */

	/*if (hsgpl_crdena)*/
		ti99_hsgpl_gpl_w(offset, data, mem_mask);
}


#if 0
#pragma mark -
#pragma mark 99/8 MEMORY HANDLERS
#endif

 READ8_HANDLER ( ti99_8_r )
{
	int page = offset >> 12;
	UINT32 mapper_reg;
	int reply = 0;

	if (ti99_8_CRUS)
	{
		/* memory mapped ports enabled */
		if ((offset >= 0x0000) && (offset < 0x2000))
			/* ROM? */
			return ROM0_ptr_8[offset & 0x1fff];
		else if ((offset >= 0x8000) && (offset < 0xa000))
		{
			/* ti99 scratch pad and memory-mapped registers */
			/* 0x8000-0x9fff */
			switch ((offset - 0x8000) >> 10)
			{
			case 0:
				/* RAM */
				reply = sRAM_ptr_8[offset & 0x1fff];
				break;

			case 1:
				/* sound write + RAM */
				if (offset >= 0x8410)
					reply = sRAM_ptr_8[offset & 0x1fff];
				break;

			case 2:
				/* VDP read + mapper status */
				if (offset < 0x8810)
				{
					if (! (offset & 1))
					{
						if (offset & 2)
							/* read VDP status */
							reply = TMS9928A_register_r(0);
						else
							/* read VDP RAM */
							reply = TMS9928A_vram_r(0);
					}
				}
				else
				{
					reply = ti99_8_mapper_status;

					ti99_8_mapper_status = 0;
				}
				break;

			case 4:
				/* speech read */
				if (! (offset & 1))
				{
					activecpu_adjust_icount(-16*4);		/* this is just a minimum, it can be more */
					reply = tms5220_status_r(0);
				}
				break;

			case 6:
				/* GPL read */
				if (! (offset & 1))
					reply = ti99_rgpl_r(offset >> 1, 0) >> 8;
				break;

			default:
				logerror("unmapped read offs=%d\n", (int) offset);
				break;
			}
			return reply;
		}
	}

	mapper_reg = ti99_8_mapper_regs[page];
	offset = (mapper_reg + (offset & 0x0fff)) & 0x00ffffff;

	/* test read protect */
	/*if (mapper_reg & 0x20000000)
		;*/

	/* test execute protect */
	/*if (mapper_reg & 0x40000000)
		;*/

	if (offset < 0x010000)
		/* Read RAM */
		reply = xRAM_ptr_8[offset];

	if (offset >= 0xff0000)
	{
		switch ((offset >> 13) & 7)
		{
		default:
			/* should never happen */
		case 0:
			/* unassigned??? */
		case 1:
			/* ??? */
		case 4:
			/* internal DSR ROM??? (normally enabled with a write to CRU >2700) */
		case 7:
			/* ??? */
			logerror("unmapped read page=%d offs=%d\n", (int) page, (int) offset);
			break;

		case 2:
			/* DSR space */
			reply = ti99_8_peb_r(offset & 0x1fff);
			break;

		case 3:
			/* cartridge space */
			offset &= 0x1fff;
#if 0
			if (hsgpl_crdena)
				/* hsgpl is enabled */
				reply = ti99_hsgpl_rom6_r(offset, mem_mask);
			else
#endif

			if (cartridge_mbx && (offset >= 0x0c00) && (offset <= 0x0ffd))
				reply = (cartridge_pages_8[0])[offset];
			else
				reply = current_page_ptr_8[offset];
			break;

		case 5:
			/* >2000 ROM (ROM1) */
			reply = ROM1_ptr_8[offset & 0x1fff];
			break;

		case 6:
			/* >6000 ROM */
			reply = ROM3_ptr_8[offset & 0x1fff];
			break;
		}
	}

	return reply;
}

WRITE8_HANDLER ( ti99_8_w )
{
	int page = offset >> 12;
	UINT32 mapper_reg;

	if (ti99_8_CRUS)
	{
		/* memory mapped ports enabled */
		if ((offset >= 0x0000) && (offset < 0x2000))
			/* ROM? */
			return;
		else if ((offset >= 0x8000) && (offset < 0xa000))
		{
			/* ti99 scratch pad and memory-mapped registers */
			/* 0x8000-0x9fff */
			switch ((offset - 0x8000) >> 10)
			{
			case 0:
				/* RAM */
				sRAM_ptr_8[offset & 0x1fff] = data;
				break;

			case 1:
				/* sound write + RAM */
				if (offset < 0x8410)
					SN76496_0_w(offset, data);
				else
					sRAM_ptr_8[offset & 0x1fff] = data;
				break;

			case 2:
				/* VDP read + mapper control */
				if (offset >= 0x8810)
				{
					int file = (data >> 1) & 3;
					int i;

					if (data & 1)
					{	/* load */
						for (i=0; i<16; i++)
						{
							ti99_8_mapper_regs[i] = (sRAM_ptr_8[(file << 6) + (i << 2)] << 24)
													| (sRAM_ptr_8[(file << 6) + (i << 2) + 1] << 16)
													| (sRAM_ptr_8[(file << 6) + (i << 2) + 2] << 8)
													| sRAM_ptr_8[(file << 6) + (i << 2) + 3];
						}
					}
					else
					{	/* save */
						for (i=0; i<16; i++)
						{
							sRAM_ptr_8[(file << 6) + (i << 2)] = ti99_8_mapper_regs[i] >> 24;
							sRAM_ptr_8[(file << 6) + (i << 2) + 1] = ti99_8_mapper_regs[i] >> 16;
							sRAM_ptr_8[(file << 6) + (i << 2) + 2] = ti99_8_mapper_regs[i] >> 8;
							sRAM_ptr_8[(file << 6) + (i << 2) + 3] = ti99_8_mapper_regs[i];
						}
					}
				}
				break;

			case 3:
				/* VDP write */
				if (! (offset & 1))
				{
					if (offset & 2)
						/* read VDP status */
						TMS9928A_register_w(0, data);
					else
						/* read VDP RAM */
						TMS9928A_vram_w(0, data);
				}
				break;

			case 5:
				/* speech write */
				if (! (offset & 1))
				{
					activecpu_adjust_icount(-48*4);		/* this is just an approx. minimum, it can be much more */

					/* the stupid design of the tms5220 core means that ready is cleared when
					there are 15 bytes in FIFO.  It should be 16.  Of course, if it were the
					case, we would need to store the value on the bus, which would be more
					complex. */
					if (! tms5220_ready_r())
					{
						double time_to_ready = tms5220_time_to_ready();
						double d = ceil(TIME_TO_CYCLES(0, time_to_ready));
						int cycles_to_ready = ((int) (d + 3)) & ~3;

						logerror("time to ready: %f -> %d\n", time_to_ready, (int) cycles_to_ready);

						activecpu_adjust_icount(-cycles_to_ready);
						timer_set(TIME_NOW, 0, /*speech_kludge_callback*/NULL);
					}

					tms5220_data_w(offset, data);
				}
				break;

			case 7:
				/* GPL write */
				if (! (offset & 1))
					ti99_wgpl_w(offset >> 1, data << 8, 0);
				break;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				break;
			}
			return;
		}
	}

	mapper_reg = ti99_8_mapper_regs[page];
	offset = (mapper_reg + (offset & 0x0fff)) & 0x00ffffff;

	/* test write protect */
	/*if (mapper_reg & 0x80000000)
		;*/

	if (offset < 0x010000)
	{	/* Write RAM */
		xRAM_ptr_8[offset] = data;
	}
	else if (offset >= 0xff0000)
	{
		switch ((offset >> 13) & 7)
		{
		default:
			/* should never happen */
		case 0:
			/* unassigned??? */
		case 1:
			/* ??? */
		case 4:
			/* internal DSR ROM??? (normally enabled with a write to CRU >2700) */
		case 7:
			/* ??? */
			break;

		case 2:
			/* DSR space */
			ti99_8_peb_w(offset & 0x1fff, data);
			break;

		case 3:
			/* cartridge space */
			offset &= 0x1fff;
#if 0
			if (hsgpl_crdena)
				/* hsgpl is enabled */
				ti99_hsgpl_rom6_w(offset, data);
			else
#endif
			if (cartridge_minimemory && (offset >= 0x1000))
				/* handle minimem RAM */
				current_page_ptr_8[offset] = data;
			else if (cartridge_mbx)
			{	/* handle MBX cart */
				/* RAM in 0x6c00-0x6ffd (presumably non-paged) */
				/* mapper at 0x6ffe */
				if ((offset >= 0x0c00) && (offset <= 0x0ffd))
					(cartridge_pages_8[0])[offset] = data;
				else if (offset == 0x0ffe)
					current_page_ptr_8 = cartridge_pages_8[cartridge_paged ? (data & 1) : 0];
			}
			else if (cartridge_paged)
				/* handle pager */
				current_page_ptr_8 = cartridge_pages_8[(offset & 2) >> 1];
			break;

		case 5:
			/* >2000 ROM (ROM1) */
			break;

		case 6:
			/* >6000 ROM */
			break;
		}
	}
	else
		logerror("unmapped write page=%d offs=%d\n", (int) page, (int) offset);
}

/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark TI-99/4 HANDSETS
#endif
/*
	Handset support (TI99/4 only)

	The ti99/4 was intended to support some so-called "IR remote handsets".
	This feature was canceled at the last minute (reportedly ten minutes before
	the introductory press conference in June 1979), but the first thousands of
	99/4 units had the required port, and the support code was seemingly not
	deleted from ROMs until the introduction of the ti99/4a in 1981.  You could
	connect up to 4 20-key keypads, and up to 4 joysticks with a maximum
	resolution of 15 levels on each axis.

	The keyboard DSR was able to couple two 20-key keypads together to emulate
	a full 40-key keyboard.  Keyboard modes 0, 1 and 2 would select either the
	console keyboard with its two wired remote controllers (i.e. joysticks), or
	remote handsets 1 and 2 with their associated IR remote controllers (i.e.
	joysticks), according to which was currently active.
*/

/*
	ti99_handset_poll_bus()

	Poll the current state of the 4-bit data bus that goes from the I/R
	receiver to the tms9901.
*/
static int ti99_handset_poll_bus(void)
{
	return (has_handset) ? (handset_buf & 0xf) : 0;
}

/*
	ti99_handset_ack_callback()

	Handle data acknowledge sent by the ti-99/4 handset ISR (through tms9901
	line P0).  This function is called by a delayed timer 30us after the state
	of P0 is changed, because, in one occasion, the ISR asserts the line before
	it reads the data, so we need to delay the acknowledge process.
*/
static void ti99_handset_ack_callback(int dummy)
{
	handset_clock = ! handset_clock;
	handset_buf >>= 4;
	handset_buflen--;
	tms9901_set_single_int(0, 12, 0);

	if (handset_buflen == 1)
	{
		/* Unless I am missing something, the third and last nybble of the
		message is not acknowledged by the DSR in any way, and the first nybble
		of next message is not requested for either, so we need to decide on
		our own when we can post a new event.  Currently, we wait for 1000us
		after the DSR acknowledges the second nybble. */
		timer_set(TIME_IN_USEC(1000), 0, ti99_handset_ack_callback);
	}

	if (handset_buflen == 0)
		/* See if we need to post a new event */
		ti99_handset_task();
}

/*
	ti99_handset_set_ack()

	Handler for tms9901 P0 pin (handset data acknowledge)
*/
static void ti99_handset_set_ack(int offset, int data)
{
	if (has_handset && handset_buflen && (data != handset_ack))
	{
		handset_ack = data;
		if (data == handset_clock)
			/* I don't know what the real delay is, but 30us apears to be enough */
			timer_set(TIME_IN_USEC(30), 0, ti99_handset_ack_callback);
	}
}

/*
	ti99_handset_post_message()

	Post a 12-bit message: trigger an interrupt on the tms9901, and store the
	message in the I/R receiver buffer so that the handset ISR will read this
	message.

	message: 12-bit message to post (only the 12 LSBits are meaningful)
*/
static void ti99_handset_post_message(int message)
{
	/* post message and assert interrupt */
	handset_clock = 1;
	handset_buf = ~ message;
	handset_buflen = 3;
	tms9901_set_single_int(0, 12, 1);
}

/*
	ti99_handset_poll_keyboard()

	Poll the current state of one given handset keypad.

	num: number of the keypad to poll (0-3)

	Returns TRUE if the handset state has changed and a message was posted.
*/
static int ti99_handset_poll_keyboard(int num)
{
	static UINT8 previous_key[max_handsets];

	UINT32 key_buf;
	UINT8 current_key;
	int i;


	/* read current key state */
	key_buf = ( readinputport(input_port_IR_keypads+num)
					| (readinputport(input_port_IR_keypads+num+1) << 16) ) >> (4*num);

	/* If a key was previously pressed, this key was not shift, and this key is
	still down, then don't change the current key press. */
	if (previous_key[num] && (previous_key[num] != 0x24)
			&& (key_buf & (1 << (previous_key[num] & 0x1f))))
	{
		/* check the shift modifier state */
		if (((previous_key[num] & 0x20) != 0) == ((key_buf & 0x0008) != 0))
			/* the shift modifier state has not changed */
			return FALSE;
		else
		{
			/* the shift modifier state has changed: we need to update the
			keyboard state */
			if (key_buf & 0x0008)
			{	/* shift has been pressed down */
				previous_key[num] = current_key = previous_key[num] | 0x20;
			}
			else
			{	/* shift has been pressed down */
				previous_key[num] = current_key = previous_key[num] & ~0x20;
			}
			/* post message */
			ti99_handset_post_message((((unsigned) current_key) << 4) | (num << 1));

			return TRUE;
		}
				
	}

	current_key = 0;	/* default value if no key is down */
	for (i=0; i<20; i++)
	{
		if (key_buf & (1 << i))
		{
			current_key = i + 1;
			if (key_buf & 0x0008)
				current_key |= 0x20;	/* set shift flag */

			if (current_key != 0x24)
				/* If this is the shift key, any other key we may find will
				have higher priority; otherwise, we may exit the loop and keep
				the key we have just found. */
				break;
		}
	}

	if (current_key != previous_key[num])
	{
		previous_key[num] = current_key;

		/* post message */
		ti99_handset_post_message((((unsigned) current_key) << 4) | (num << 1));

		return TRUE;
	}
		
	return FALSE;
}

/*
	ti99_handset_poll_joystick()

	Poll the current state of one given handset joystick.

	num: number of the joystick to poll (0-3)

	Returns TRUE if the handset state has changed and a message was posted.
*/
static int ti99_handset_poll_joystick(int num)
{
	static UINT8 previous_joy[max_handsets];
	UINT8 current_joy;
	int current_joy_x, current_joy_y;
	int message;

	/* read joystick position */
	current_joy_x = readinputport(input_port_IR_joysticks+2*num);
	current_joy_y = readinputport(input_port_IR_joysticks+2*num+1);
	/* compare with last saved position */
	current_joy = current_joy_x | (current_joy_y << 4);
	if (current_joy != previous_joy[num])
	{
		/* save position */
		previous_joy[num] = current_joy;

		/* transform position to signed quantity */
		current_joy_x -= 7;
		current_joy_y -= 7;

		message = 0;

		/* set sign */
		/* note that we set the sign if the joystick position is 0 to work
		around a bug in the ti99/4 ROMs */
		if (current_joy_x <= 0)
		{
			message |= 0x040;
			current_joy_x = - current_joy_x;
		}

		if (current_joy_y <= 0)
		{
			message |= 0x400;
			current_joy_y = - current_joy_y;
		}

		/* convert absolute values to Gray code and insert in message */
		if (current_joy_x & 4)
			current_joy_x ^= 3;
		if (current_joy_x & 2)
			current_joy_x ^= 1;
		message |= current_joy_x << 3;

		if (current_joy_y & 4)
			current_joy_y ^= 3;
		if (current_joy_y & 2)
			current_joy_y ^= 1;
		message |= current_joy_y << 7;

		/* set joystick address */
		message |= (num << 1) | 0x1;

		/* post message */
		ti99_handset_post_message(message);

		return TRUE;
	}

	return FALSE;
}

/*
	ti99_handset_task()

	Manage handsets, posting an event if the state of any handset has changed.
*/
static void ti99_handset_task(void)
{
	int i;

	if (handset_buflen == 0)
	{	/* poll every handset */
		for (i=0; i<max_handsets; i++)
			if (ti99_handset_poll_joystick(i))
				return;
		for (i=0; i<max_handsets; i++)
			if (ti99_handset_poll_keyboard(i))
				return;
	}
	else if (handset_buflen == 3)
	{	/* update messages after they have been posted */
		if (handset_buf & 1)
		{	/* keyboard */
			ti99_handset_poll_keyboard((~ (handset_buf >> 1)) & 0x3);
		}
		else
		{	/* joystick */
			ti99_handset_poll_joystick((~ (handset_buf >> 1)) & 0x3);
		}
	}
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark MECHATRONICS MOUSE
#endif

static void mecmouse_select(int sel)
{
	if (mecmouse_sel != (sel != 0))
	{
		mecmouse_sel = (sel != 0);
		if (mecmouse_sel)
		{
			if (! mecmouse_read_y)
			{
				/* sample mouse data for both axes */
				if (mecmouse_x < -4)
					mecmouse_x_buf = -4;
				else if (mecmouse_x > 3)
					mecmouse_x_buf = 3;
				else
					mecmouse_x_buf = mecmouse_x;
				mecmouse_x -= mecmouse_x_buf;
				mecmouse_x_buf = (mecmouse_x_buf-1) & 7;

				if (mecmouse_y < -4)
					mecmouse_y_buf = -4;
				else if (mecmouse_y > 3)
					mecmouse_y_buf = 3;
				else
					mecmouse_y_buf = mecmouse_y;
				mecmouse_y -= mecmouse_y_buf;
				mecmouse_y_buf = (mecmouse_y_buf-1) & 7;
			}
		}
		else
			mecmouse_read_y = ! mecmouse_read_y;
	}
}

static void mecmouse_poll(void)
{
	static int last_mx = 0, last_my = 0;
	int new_mx, new_my;
	int delta_x, delta_y;

	new_mx = readinputport(input_port_mousex);
	new_my = readinputport(input_port_mousey);

	/* compute x delta */
	delta_x = new_mx - last_mx;

	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = 0x100-delta_x;
	if  (delta_x < -0x80)
		delta_x = -0x100-delta_x;

	last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = 0x100-delta_y;
	if  (delta_y < -0x80)
		delta_y = -0x100-delta_y;

	last_my = new_my;

	/* update state */
	mecmouse_x += delta_x;
	mecmouse_y += delta_y;
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark TMS9901 INTERFACE
#endif
/*
	TI99/4x-specific tms9901 I/O handlers

	See mess/machine/tms9901.c for generic tms9901 CRU handlers.

	BTW, although TMS9900 is generally big-endian, it is little endian as far as CRU is
	concerned. (i.e. bit 0 is the least significant)

KNOWN PROBLEMS:
	* a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is:
	  on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
	  timer mode even though the program did not explicitely ask... and THIS is impossible
	  to emulate efficiently (we would have to check every memory operation).
*/
/*
	TMS9901 interrupt handling on a TI99/4(a).

	TI99/4(a) uses the following interrupts:
	INT1: external interrupt (used by RS232 controller, for instance)
	INT2: VDP interrupt
	TMS9901 timer interrupt (overrides INT3)
	INT12: handset interrupt (only on a TI-99/4 with the handset prototypes)

	Three (occasionally four) interrupts are used by the system (INT1, INT2,
	timer, and INT12 on a TI-99/4 with remote handset prototypes), out of 15/16
	possible interrupts.  Keyboard pins can be used as interrupt pins, too, but
	this is not emulated (it's a trick, anyway, and I don't know any program
	which uses it).

	When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
	a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
	the interrupt pin and the revelant bit in the interrupt mask are set.

	TIMER interrupts are kind of an exception, since they are not associated with an external
	interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
	the pending interrupt (or am I wrong once again ?).

nota:
	All interrupt routines notify (by software) the TMS9901 of interrupt
	recognition (with a "SBO n").  However, unless I am missing something, this
	has absolutely no consequence on the TMS9901 (except for the TIMER
	interrupt routine), and interrupt routines would work fine without this
	SBO instruction.  This is quite weird.  Maybe the interrupt recognition
	notification is needed on TMS9985, or any other weird variant of TMS9900
	(how about the TI-99 development system connected to a TI990/10?).
*/

/*
	set the state of int1 (called by the peb core)
*/
static void tms9901_set_int1(int state)
{
	tms9901_set_single_int(0, 1, state);
}

/*
	set the state of int2 (called by the tms9928 core)
*/
void tms9901_set_int2(int state)
{
	tms9901_set_single_int(0, 2, state);
}

/*
	Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static void tms9901_interrupt_callback(int intreq, int ic)
{
	if (intreq)
	{
		/* On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		 * but hard-wired to force level 1 interrupts */
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 1);	/* interrupt it, baby */
	}
	else
	{
		cpunum_set_input_line(0, 0, CLEAR_LINE);
	}
}

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bit 3-7: keyboard status bits 0 to 4
*/
static int ti99_R9901_0(int offset)
{
	int answer;


	if ((ti99_model == model_99_4) && (KeyCol == 7))
		answer = (ti99_handset_poll_bus() << 3) | 0x80;
	else if (has_mecmouse && (KeyCol == ((ti99_model == model_99_4) ? 6 : 7)))
	{
		int buttons = readinputport(input_port_mouse_buttons) & 3;

		answer = (mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 4;

		if (! (buttons & 1))
			/* action button */
			answer |= 0x08;
		if (! (buttons & 2))
			/* home button */
			answer |= 0x80;
	}
	else
		answer = ((readinputport(input_port_keyboard + (KeyCol >> 1)) >> ((KeyCol & 1) * 8)) << 3) & 0xF8;

	if ((ti99_model == model_99_4a) || (ti99_model == model_99_4p))
	{
		if (! AlphaLockLine)
			answer &= ~ (readinputport(input_port_caps_lock) << 3);
	}

	return answer;
}

/*
	Read pins INT8*-INT15* of TI99's 9901.

	signification:
	 bit 0-2: keyboard status bits 5 to 7
	 bit 3: tape input mirror
	 (bit 4: IR remote handset interrupt)
	 bit 5-7: weird, not emulated
*/
static int ti99_R9901_1(int offset)
{
	int answer;


	if (/*(ti99_model == model_99_4) &&*/ (KeyCol == 7))
		answer = 0x07;
	else
		answer = ((readinputport(input_port_keyboard + (KeyCol >> 1)) >> ((KeyCol & 1) * 8)) >> 5) & 0x07;

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	/*if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0)
		answer |= 8;*/

	return answer;
}

/*
	Read pins P0-P7 of TI99's 9901.

	 bit 1: handset data clock pin
*/
static int ti99_R9901_2(int offset)
{
	return (has_handset && handset_clock) ? 2 : 0;
}

/*
	Read pins P8-P15 of TI99's 9901.

	 bit 26: IR handset interrupt
	 bit 27: tape input
*/
static int ti99_R9901_3(int offset)
{
	int answer;

	if (has_handset && (handset_buflen == 3))
		answer = 0;
	else
		answer = 4;	/* on systems without handset, the pin is pulled up to avoid spurious interrupts */

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0)
		answer |= 8;

	return answer;
}


/*
	WRITE key column select (P2-P4)
*/
static void ti99_KeyC(int offset, int data)
{
	if (data)
		KeyCol |= 1 << (offset-2);
	else
		KeyCol &= ~ (1 << (offset-2));

	if (has_mecmouse)
		mecmouse_select(KeyCol == ((ti99_model == model_99_4) ? 6 : 7));
}

/*
	WRITE alpha lock line - TI99/4a only (P5)
*/
static void ti99_AlphaW(int offset, int data)
{
	if ((ti99_model == model_99_4a) || (ti99_model == model_99_4p))
		AlphaLockLine = data;
}

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bits 3-4: unused?
	 bit 5: ???
	 bit 6-7: keyboard status bits 0 through 1
*/
static int ti99_8_R9901_0(int offset)
{
	int answer;


	if (has_mecmouse && (KeyCol == 15))
	{
		int buttons = readinputport(input_port_mouse_buttons) & 3;

		answer = ((mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 7) & 0x80;

		if (! (buttons & 1))
			/* action button */
			answer |= 0x40;
	}
	else
		answer = (readinputport(input_port_keyboard + KeyCol) << 6) & 0xC0;

	return answer;
}

/*
	Read pins INT8*-INT15* of TI99's 9901.

	signification:
	 bit 0-2: keyboard status bits 2 to 4
	 bit 3: tape input mirror
	 (bit 4: IR remote handset interrupt)
	 bit 5-7: weird, not emulated
*/
static int ti99_8_R9901_1(int offset)
{
	int answer;


	if (has_mecmouse && (KeyCol == 15))
	{
		int buttons = readinputport(input_port_mouse_buttons) & 3;

		answer = ((mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 1) & 0x03;

		if (! (buttons & 2))
			/* home button */
			answer |= 0x04;
	}
	else
		answer = (readinputport(input_port_keyboard + KeyCol) >> 2) & 0x07;

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	/*if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0)
		answer |= 8;*/

	return answer;
}

/*
	WRITE key column select (P0-P3)
*/
static void ti99_8_KeyC(int offset, int data)
{
	if (data)
		KeyCol |= 1 << offset;
	else
		KeyCol &= ~ (1 << offset);

	if (has_mecmouse)
		mecmouse_select(KeyCol == 15);
}

static void ti99_8_WCRUS(int offset, int data)
{
	ti99_8_CRUS = data;
}

static void ti99_8_PTGEN(int offset, int data)
{
	/* ... */
}

/*
	command CS1/CS2 tape unit motor (P6-P7)
*/
static void ti99_CS_motor(int offset, int data)
{
	mess_image *img = image_from_devtype_and_index(IO_CASSETTE, offset-6);
	cassette_change_state(img, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
	audio gate (P8)

	Set to 1 before using tape: this enables the mixing of tape input sound
	with computer sound.

	We do not really need to emulate this as the tape recorder generates sound
	on its own.
*/
static void ti99_audio_gate(int offset, int data)
{
}

/*
	tape output (P9)
	I think polarity is correct, but don't take my word for it.
*/
static void ti99_CS_output(int offset, int data)
{
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), data ? +1 : -1);
	if (ti99_model != model_99_8)	/* 99/8 only has one tape port!!! */
		cassette_output(image_from_devtype_and_index(IO_CASSETTE, 1), data ? +1 : -1);
}



/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark 99/8 INTERNAL DSR
#endif
/*
	TI 99/8 internal DSR support.

	Includes a few specific signals, and an extra ROM.
*/

/* prototypes */
static void ti99_8_internal_dsr_cru_w(int offset, int data);
static  READ8_HANDLER(ti99_8_internal_dsr_r);


static const ti99_peb_card_handlers_t ti99_8_internal_dsr_handlers =
{
	NULL,
	ti99_8_internal_dsr_cru_w,
	ti99_8_internal_dsr_r,
	NULL
};

/* pointer to the internal DSR ROM data */
static UINT8 *ti99_8_internal_DSR;


/* set up handlers, and set initial state */
static void ti99_8_internal_dsr_init(void)
{
	ti99_8_internal_DSR = memory_region(REGION_CPU1) + offset_rom0_8 + 0x4000;

	ti99_peb_set_card_handlers(0x2700, & ti99_8_internal_dsr_handlers);
}

/* write CRU bit:
	bit0: enable/disable internal DSR ROM,
	bit1: hard reset */
static void ti99_8_internal_dsr_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 1:
		/* hard reset -- not emulated */
		break;
	}
}

/* read internal DSR ROM */
static  READ8_HANDLER(ti99_8_internal_dsr_r)
{
	return ti99_8_internal_DSR[offset];
}



/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark SGCPU INTERNAL DSR
#endif
/*
	SNUG SGCPU (a.k.a. 99/4p) internal DSR support.

	Includes a few specific signals, and an extra ROM.
*/

/* prototypes */
static void ti99_4p_internal_dsr_cru_w(int offset, int data);
static READ16_HANDLER(ti99_4p_internal_dsr_r);


static const ti99_peb_16bit_card_handlers_t ti99_4p_internal_dsr_handlers =
{
	NULL,
	ti99_4p_internal_dsr_cru_w,
	ti99_4p_internal_dsr_r,
	NULL
};

/* pointer to the internal DSR ROM data */
static UINT16 *ti99_4p_internal_DSR;


/* set up handlers, and set initial state */
static void ti99_4p_internal_dsr_init(void)
{
	ti99_4p_internal_DSR = (UINT16 *) (memory_region(REGION_CPU1) + offset_rom4_4p);
	ti99_4p_internal_ROM6 = (UINT16 *) (memory_region(REGION_CPU1) + offset_rom6_4p);

	ti99_peb_set_16bit_card_handlers(0x0f00, & ti99_4p_internal_dsr_handlers);

	ti99_4p_internal_rom6_enable = 0;
	ti99_4p_peb_set_senila(0);
	ti99_4p_peb_set_senilb(0);
}

/* write CRU bit:
	bit0: enable/disable internal DSR ROM,
	bit1: enable/disable internal cartridge ROM
	bit2: set/clear senila
	bit3: set/clear senilb*/
static void ti99_4p_internal_dsr_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 1:
		ti99_4p_internal_rom6_enable = data;
		break;
	case 2:
		ti99_4p_peb_set_senila(data);
		break;
	case 3:
		ti99_4p_peb_set_senilb(data);
		break;
	case 4:
		/* 0: 16-bit (fast) memory timings */
		/* 1: 8-bit memory timings */
		break;
	case 5:
		/* if 0: "KBENA" mode (enable keyboard?) */
		/* if 1: "KBINH" mode (inhibit keyboard?) */
		break;
	}
}

/* read internal DSR ROM */
static READ16_HANDLER(ti99_4p_internal_dsr_r)
{
	return ti99_4p_internal_DSR[offset];
}



#if 0
#pragma mark -
#pragma mark MEMORY EXPANSION CARDS
#endif
/*===========================================================================*/
/*
	TI memory extension support.

	Simple 8-bit DRAM: 32kb in two chunks of 8kb and 24kb repectively.
	Since the RAM is on the 8-bit bus, there is an additionnal delay.
*/

static READ16_HANDLER ( ti99_TIxramlow_r );
static WRITE16_HANDLER ( ti99_TIxramlow_w );
static READ16_HANDLER ( ti99_TIxramhigh_r );
static WRITE16_HANDLER ( ti99_TIxramhigh_w );

static void ti99_TIxram_init(void)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_TIxramlow_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_TIxramlow_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_TIxramhigh_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_TIxramhigh_w);
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_TIxramlow_r )
{
	activecpu_adjust_icount(-4);

	return xRAM_ptr[offset];
}

static WRITE16_HANDLER ( ti99_TIxramlow_w )
{
	activecpu_adjust_icount(-4);

	COMBINE_DATA(xRAM_ptr + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_TIxramhigh_r )
{
	activecpu_adjust_icount(-4);

	return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_TIxramhigh_w )
{
	activecpu_adjust_icount(-4);

	COMBINE_DATA(xRAM_ptr + offset+0x1000);
}


/*===========================================================================*/
/*
	Super AMS memory extension support.

	Up to 1Mb of SRAM.  Straightforward mapper, works with 4kb chunks.  The mapper was designed
	to be extendable with other RAM areas.
*/

/* prototypes */
static void sAMS_cru_w(int offset, int data);
static  READ8_HANDLER(sAMS_mapper_r);
static WRITE8_HANDLER(sAMS_mapper_w);

static READ16_HANDLER ( ti99_sAMSxramlow_r );
static WRITE16_HANDLER ( ti99_sAMSxramlow_w );
static READ16_HANDLER ( ti99_sAMSxramhigh_r );
static WRITE16_HANDLER ( ti99_sAMSxramhigh_w );


static const ti99_peb_card_handlers_t sAMS_expansion_handlers =
{
	NULL,
	sAMS_cru_w,
	sAMS_mapper_r,
	sAMS_mapper_w
};


static int sAMS_mapper_on;
static int sAMSlookup[16];


/* set up super AMS handlers, and set initial state */
static void ti99_sAMSxram_init(void)
{
	int i;


	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_sAMSxramlow_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_sAMSxramlow_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_sAMSxramhigh_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_sAMSxramhigh_w);

	ti99_peb_set_card_handlers(0x1e00, & sAMS_expansion_handlers);

	sAMS_mapper_on = 0;

	for (i=0; i<16; i++)
		sAMSlookup[i] = i << 11;
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void sAMS_cru_w(int offset, int data)
{
	if (offset == 1)
		sAMS_mapper_on = data;
}

/* read a mapper register */
static  READ8_HANDLER(sAMS_mapper_r)
{
	return (sAMSlookup[(offset >> 1) & 0xf] >> 11);
}

/* write a mapper register */
static WRITE8_HANDLER(sAMS_mapper_w)
{
	sAMSlookup[(offset >> 1) & 0xf] = ((int) data) << 11;
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_sAMSxramlow_r )
{
	activecpu_adjust_icount(-4);

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]];
	else
		return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_sAMSxramlow_w )
{
	activecpu_adjust_icount(-4);

	if (sAMS_mapper_on)
		COMBINE_DATA(xRAM_ptr + (offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]);
	else
		COMBINE_DATA(xRAM_ptr + offset+0x1000);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_sAMSxramhigh_r )
{
	activecpu_adjust_icount(-4);

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x5000+offset)>>11]];
	else
		return xRAM_ptr[offset+0x5000];
}

static WRITE16_HANDLER ( ti99_sAMSxramhigh_w )
{
	activecpu_adjust_icount(-4);

	if (sAMS_mapper_on)
		COMBINE_DATA(xRAM_ptr + (offset&0x7ff)+sAMSlookup[(0x5000+offset)>>11]);
	else
		COMBINE_DATA(xRAM_ptr + offset+0x5000);
}


/*===========================================================================*/
/*
	SNUG SGCPU (a.k.a. 99/4p) Super AMS clone support.
	Compatible with Super AMS, but uses a 16-bit bus.

	Up to 1Mb of SRAM.  Straightforward mapper, works with 4kb chunks.
*/

/* prototypes */
static void ti99_4p_mapper_cru_w(int offset, int data);
static READ16_HANDLER(ti99_4p_mapper_r);
static WRITE16_HANDLER(ti99_4p_mapper_w);


static const ti99_peb_16bit_card_handlers_t ti99_4p_mapper_handlers =
{
	NULL,
	ti99_4p_mapper_cru_w,
	ti99_4p_mapper_r,
	ti99_4p_mapper_w
};


static int ti99_4p_mapper_on;
static int ti99_4p_mapper_lookup[16];


/* set up handlers, and set initial state */
static void ti99_4p_mapper_init(void)
{
	int i;

	/* Not required at run-time */
	/*memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, MRA16_BANK3);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, MWA16_BANK3);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, MRA16_BANK4);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, MWA16_BANK4);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, MRA16_BANK5);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, MWA16_BANK5);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, MRA16_BANK6);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, MWA16_BANK6);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, MRA16_BANK7);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, MWA16_BANK7);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, MRA16_BANK8);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, MWA16_BANK8);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, MRA16_BANK9);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, MWA16_BANK9);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, MRA16_BANK10);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, MWA16_BANK10);*/

	ti99_peb_set_16bit_card_handlers(0x1e00, & ti99_4p_mapper_handlers);

	ti99_4p_mapper_on = 0;

	for (i=0; i<16; i++)
	{
		ti99_4p_mapper_lookup[i] = i << 11;

		/* update bank base */
		switch (i)
		{
		case 2:
		case 3:
			memory_set_bankptr(3+(i-2), xRAM_ptr + (i<<11));
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			memory_set_bankptr(5+(i-10), xRAM_ptr + (i<<11));
			break;
		}
	}
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void ti99_4p_mapper_cru_w(int offset, int data)
{
	int i;

	if (offset == 1)
	{
		if (ti99_4p_mapper_on != data)
		{
			ti99_4p_mapper_on = data;

			for (i=0; i<16; i++)
			{
				/* update bank base */
				switch (i)
				{
				case 2:
				case 3:
					memory_set_bankptr(3+(i-2), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
					break;

				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
					memory_set_bankptr(5+(i-10), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
					break;
				}
			}
		}
	}

}

/* read a mapper register */
static READ16_HANDLER(ti99_4p_mapper_r)
{
	return (ti99_4p_mapper_lookup[offset & 0xf] >> 3);
}

/* write a mapper register */
static WRITE16_HANDLER(ti99_4p_mapper_w)
{
	int page = offset & 0xf;

	ti99_4p_mapper_lookup[page] = (data & 0xff00) << 3;

	if (ti99_4p_mapper_on)
	{
		/* update bank base */
		switch (page)
		{
		case 2:
		case 3:
			memory_set_bankptr(3+(page-2), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			memory_set_bankptr(5+(page-10), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;
		}
	}
}


/*===========================================================================*/
/*
	myarc-ish memory extension support (foundation, and myarc clones).

	Up to 512kb of RAM.  Straightforward mapper, works with 32kb chunks.
*/

static int myarc_cru_r(int offset);
static void myarc_cru_w(int offset, int data);

static READ16_HANDLER ( ti99_myarcxramlow_r );
static WRITE16_HANDLER ( ti99_myarcxramlow_w );
static READ16_HANDLER ( ti99_myarcxramhigh_r );
static WRITE16_HANDLER ( ti99_myarcxramhigh_w );


static const ti99_peb_card_handlers_t myarc_expansion_handlers =
{
	myarc_cru_r,
	myarc_cru_w,
	NULL,
	NULL
};


static int myarc_cur_page_offset;
static int myarc_page_offset_mask;


/* set up myarc handlers, and set initial state */
static void ti99_myarcxram_init(void)
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_myarcxramlow_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, ti99_myarcxramlow_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_myarcxramhigh_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xffff, 0, 0, ti99_myarcxramhigh_w);

	switch (xRAM_kind)
	{
	case xRAM_kind_foundation_128k:	/* 128kb foundation */
	case xRAM_kind_myarc_128k:		/* 128kb myarc clone */
		myarc_page_offset_mask = 0x0c000;
		break;
	case xRAM_kind_foundation_512k:	/* 512kb foundation */
	case xRAM_kind_myarc_512k:		/* 512kb myarc clone */
		myarc_page_offset_mask = 0x3c000;
		break;
	default:
		break;	/* let's just keep GCC's big mouth shut */
	}

	switch (xRAM_kind)
	{
	case xRAM_kind_foundation_128k:	/* 128kb foundation */
	case xRAM_kind_foundation_512k:	/* 512kb foundation */
		ti99_peb_set_card_handlers(0x1e00, & myarc_expansion_handlers);
		break;
	case xRAM_kind_myarc_128k:		/* 128kb myarc clone */
	case xRAM_kind_myarc_512k:		/* 512kb myarc clone */
		ti99_peb_set_card_handlers(0x1000, & myarc_expansion_handlers);
		ti99_peb_set_card_handlers(0x1900, & myarc_expansion_handlers);
		break;
	default:
		break;	/* let's just keep GCC's big mouth shut */
	}

	myarc_cur_page_offset = 0;
}

/* read CRU bit:
	bit 1-2 (128kb) or 1-4 (512kb): read current map offset */
static int myarc_cru_r(int offset)
{
	/*if (offset == 0)*/	/* right??? */
	{
		return (myarc_cur_page_offset >> 14);
	}
}

/* write CRU bit:
	bit 1-2 (128kb) or 1-4 (512kb): write map offset */
static void myarc_cru_w(int offset, int data)
{
	offset &= 0x7;	/* right??? */
	if (offset >= 1)
	{
		int mask = 1 << (offset-1+14);

		if (data)
			myarc_cur_page_offset |= mask;
		else
			myarc_cur_page_offset &= ~mask;

		myarc_cur_page_offset &= myarc_page_offset_mask;
	}
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_myarcxramlow_r )
{
	activecpu_adjust_icount(-4);

	return xRAM_ptr[myarc_cur_page_offset + offset];
}

static WRITE16_HANDLER ( ti99_myarcxramlow_w )
{
	activecpu_adjust_icount(-4);

	COMBINE_DATA(xRAM_ptr + myarc_cur_page_offset + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_myarcxramhigh_r )
{
	activecpu_adjust_icount(-4);

	return xRAM_ptr[myarc_cur_page_offset + offset+0x1000];
}

static WRITE16_HANDLER ( ti99_myarcxramhigh_w )
{
	activecpu_adjust_icount(-4);

	COMBINE_DATA(xRAM_ptr + myarc_cur_page_offset + offset+0x1000);
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark EVPC VIDEO CARD
#endif
/*
	SNUG's EVPC emulation
*/

/* prototypes */
static int evpc_cru_r(int offset);
static void evpc_cru_w(int offset, int data);
static  READ8_HANDLER(evpc_mem_r);
static WRITE8_HANDLER(evpc_mem_w);

/* pointer to the evpc DSR area */
static UINT8 *ti99_evpc_DSR;

/* pointer to the evpc novram area */
/*static UINT8 *ti99_evpc_novram;*/

static int RAMEN;

static int evpc_dsr_page;

static const ti99_peb_card_handlers_t evpc_handlers =
{
	evpc_cru_r,
	evpc_cru_w,
	evpc_mem_r,
	evpc_mem_w
};


/*
	Reset evpc card, set up handlers
*/
static void ti99_evpc_init(void)
{
	ti99_evpc_DSR = memory_region(region_dsr) + offset_evpc_dsr;

	RAMEN = 0;
	evpc_dsr_page = 0;

	ti99_peb_set_card_handlers(0x1400, & evpc_handlers);
}

/*
	Read evpc CRU interface
*/
static int evpc_cru_r(int offset)
{
	return 0;	/* dip-switch value */
}

/*
	Write evpc CRU interface
*/
static void evpc_cru_w(int offset, int data)
{
	switch (offset)
	{
	case 0:
		break;

	case 1:
		if (data)
			evpc_dsr_page |= 1;
		else
			evpc_dsr_page &= ~1;
		break;

	case 2:
		break;

	case 3:
		RAMEN = data;
		break;

	case 4:
		if (data)
			evpc_dsr_page |= 4;
		else
			evpc_dsr_page &= ~4;
		break;

	case 5:
		if (data)
			evpc_dsr_page |= 2;
		else
			evpc_dsr_page &= ~2;
		break;

	case 6:
		break;

	case 7:
		break;
	}
}


static struct
{
	UINT8 read_index, write_index, mask;
	bool read;
	int state;
	struct { UINT8 red, green, blue; } color[0x100];
	//int dirty;
} evpc_palette;

/*
	read a byte in evpc DSR space
*/
static  READ8_HANDLER(evpc_mem_r)
{
	UINT8 reply = 0;


	if (offset < 0x1f00)
	{
		reply = ti99_evpc_DSR[offset+evpc_dsr_page*0x2000];
	}
	else if (offset < 0x1ff0)
	{
		if (RAMEN)
		{	/* NOVRAM */
			reply = 0;
		}
		else
		{
			reply = ti99_evpc_DSR[offset+evpc_dsr_page*0x2000];
		}
	}
	else
	{	/* PALETTE */
		logerror("palette read, offset=%d\n", offset-0x1ff0);
		switch (offset - 0x1ff0)
		{
		case 0:
			/* Palette Read Address Register */
			logerror("EVPC palette address read\n");
			reply = evpc_palette.write_index;
			break;

		case 2:
			/* Palette Read Color Value */
			logerror("EVPC palette color read\n");
			if (evpc_palette.read)
			{
				switch (evpc_palette.state)
				{
				case 0:
					reply = evpc_palette.color[evpc_palette.read_index].red;
					break;
				case 1:
					reply = evpc_palette.color[evpc_palette.read_index].green;
					break;
				case 2:
					reply = evpc_palette.color[evpc_palette.read_index].blue;
					break;
				}
				evpc_palette.state++;
				if (evpc_palette.state == 3)
				{
					evpc_palette.state = 0;
					evpc_palette.read_index++;
				}
			}
			break;

		case 4:
			/* Palette Read Pixel Mask */
			logerror("EVPC palette mask read\n");
			reply = evpc_palette.mask;
			break;
		case 6:
			/* Palette Read Address Register for Color Value */
			logerror("EVPC palette status read\n");
			if (evpc_palette.read)
				reply = 0;
			else
				reply = 3;
			break;
		}
	}

	return reply;
}

/*
	write a byte in evpc DSR space
*/
static WRITE8_HANDLER(evpc_mem_w)
{
	if ((offset >= 0x1f00) && (offset < 0x1ff0) && RAMEN)
	{	/* NOVRAM */
	}
	else if (offset >= 0x1ff0)
	{	/* PALETTE */
		logerror("palette write, offset=%d\n, data=%d", offset-0x1ff0, data);
		switch (offset - 0x1ff0)
		{
		case 8:
			/* Palette Write Address Register */
			logerror("EVPC palette address write (for write access)\n");
			evpc_palette.write_index = data;
			evpc_palette.state = 0;
			evpc_palette.read = 0;
			break;

		case 10:
			/* Palette Write Color Value */
			logerror("EVPC palette color write\n");
			if (! evpc_palette.read)
			{
				switch (evpc_palette.state)
				{
				case 0:
					evpc_palette.color[evpc_palette.write_index].red = data;
					break;
				case 1:
					evpc_palette.color[evpc_palette.write_index].green = data;
					break;
				case 2:
					evpc_palette.color[evpc_palette.write_index].blue = data;
					break;
				}
				evpc_palette.state++;
				if (evpc_palette.state == 3)
				{
					evpc_palette.state = 0;
					evpc_palette.write_index++;
				}
				//evpc_palette.dirty = 1;
			}
			break;

		case 12:
			/* Palette Write Pixel Mask */
			logerror("EVPC palette mask write\n");
			evpc_palette.mask = data;
			break;

		case 14:
			/* Palette Write Address Register for Color Value */
			logerror("EVPC palette address write (for read access)\n");
			evpc_palette.read_index=data;
			evpc_palette.state=0;
			evpc_palette.read=1;
			break;
		}
	}
}
