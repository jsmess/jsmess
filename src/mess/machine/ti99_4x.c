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
	* save minimemory contents 

	Compatibility looks quite good.

Issues:
	* floppy disk timings are not emulated (general issue)

TODO:
	* support for other peripherals and DSRs as documentation permits
	* implement the EVPC palette chip
	* finish 99/4p support: ROM6, HSGPL (implemented, but not fully debugged)
*/

#include <math.h>
#include "driver.h"
#include "tms9901.h"
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
#include "ti99pcod.h"

#include "sound/tms5220.h"	/* for tms5220_set_variant() */
#include "sound/5220intf.h"
#include "sound/sn76496.h"

#include "devices/harddriv.h"	/* for device_init_mess_hd() */
#include "smc92x4.h"	/* for smc92x4_hd_load()/smc92x4_hd_unload() */

/* prototypes */
static READ16_HANDLER ( ti99_rspeech_r );
static WRITE16_HANDLER ( ti99_wspeech_w );

static void tms9901_set_int1(running_machine *machine, int state);

static void ti99_handset_task(running_machine *machine);
static void mecmouse_poll(running_machine *machine);

static void ti99_8_internal_dsr_reset(running_machine *machine);

static void ti99_4p_internal_dsr_reset(running_machine *machine);
static void ti99_TIxram_init(running_machine *machine);
static void ti99_sAMSxram_init(running_machine *machine);
static void ti99_4p_mapper_init(running_machine *machine);
static void ti99_myarcxram_init(running_machine *machine);
static void ti99_evpc_reset(running_machine *machine);

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

/* TRUE if p-code card present */
static char has_pcode;

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

	The handling of cartridge GROMs has moved to ti99cart.c.	
*/

/* descriptor for console GROMs */
static GROM_port_t console_GROMs;

/* true if hsgpl is enabled (i.e. has_hsgpl is true and hsgpl cru bit crdena is
set) */
static char hsgpl_crdena;

/* true if 99/4p rom6 is enabled */
static int ti99_4p_internal_rom6_enable;
/* pointer to the ROM6 data */
static UINT16 *ti99_4p_internal_ROM6;

/* Pointer to the cartridge system for the 99/8 */
static const device_config *multicart8;

/* ti99/8 hardware */
static UINT8 ti99_8_enable_rom_and_ports;

static UINT32 ti99_8_mapper_regs[16];
static UINT8 ti99_8_mapper_status;

static UINT8 *xRAM_ptr_8;
static UINT8 *ROM0_ptr_8;
static UINT8 *ROM1_ptr_8;
static UINT8 *ROM2_ptr_8;
static UINT8 *ROM3_ptr_8;
static UINT8 *sRAM_ptr_8;

/* tms9900_ICount: used to implement memory waitstates (hack) */
/* tms9995_ICount: used to implement memory waitstates (hack) */
/* NPW 23-Feb-2004 - externs no longer needed because we now use cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),) */

/* Link to the cartridge system. */
static const device_config *cartslots;

/*===========================================================================*/
/*
	General purpose code:
	initialization, cart loading, etc.
*/

static void create_grom_high2k(int first, int last)
{
        /* Generate missing chunk of each console GROMs. Although a GROM
           occupies 8 KiB of address space, it only has 6 KiB capacity, and
           the last 2 KiB mirror previous content as shown above. */
        int base;
        for (base=(first&0xe000); base<((last+1)&0xe000); base+=0x2000)
        {
                int j;
                for (j=0; j<0x800; j++)
                {
                        console_GROMs.data_ptr[base+0x1800+j] =
                                  console_GROMs.data_ptr[base+0x0800+j]
                                | console_GROMs.data_ptr[base+0x1000+j];
                }
        }
}

int is_99_8()
{
	return ti99_model == model_99_8;
}

DRIVER_INIT( ti99_4 )
{
	ti99_model = model_99_4;
	has_evpc = FALSE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(machine, "maincpu") + offset_xram);
	console_GROMs.data_ptr = memory_region(machine, region_grom);
	create_grom_high2k(0x0000, 0x5fff);
}

DRIVER_INIT( ti99_4a )
{
	ti99_model = model_99_4a;
	has_evpc = FALSE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(machine, "maincpu") + offset_xram);
	console_GROMs.data_ptr = memory_region(machine, region_grom);
	create_grom_high2k(0x0000, 0x5fff);
}

DRIVER_INIT( ti99_4ev )
{
	UINT8 *mem = memory_region(machine, "maincpu");
	ti99_model = model_99_4a;
	has_evpc = TRUE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (mem + offset_xram);
	console_GROMs.data_ptr = memory_region(machine, region_grom);
	create_grom_high2k(0x0000, 0x5fff);
}

DRIVER_INIT( ti99_8 )
{
	UINT8 *mem = memory_region(machine, "maincpu");
	ti99_model = model_99_8;
	has_evpc = FALSE;
	
	/* set up memory pointers */
	xRAM_ptr_8 = mem + offset_xram_8;
	ROM0_ptr_8 = mem + offset_rom0_8;
	ROM1_ptr_8 = ROM0_ptr_8 + 0x2000;
	ROM2_ptr_8 = ROM1_ptr_8 + 0x2000;
	ROM3_ptr_8 = ROM2_ptr_8 + 0x2000;
	sRAM_ptr_8 = mem + offset_sram_8;

	/* Pull this one up here. */
	ti99_8_enable_rom_and_ports = 1;
	
	console_GROMs.data_ptr = memory_region(machine, region_grom);
	create_grom_high2k(0x0000, 0x5fff);

	multicart8 = devtag_get_device(machine, "ti99_multicart");
	assert (multicart8 != NULL);
}

DRIVER_INIT( ti99_4p )
{
	ti99_model = model_99_4p;
	has_evpc = TRUE;

	/* set up memory pointers */
	xRAM_ptr = (UINT16 *) (memory_region(machine, "maincpu") + offset_xram_4p);
	/*console_GROMs.data_ptr = memory_region(machine, region_grom);*/
	create_grom_high2k(0x0000, 0x5fff);
}

static const TMS9928a_interface tms9918_interface =
{
	TMS99x8,
	0x4000,
	15, 15,
	tms9901_set_int2
};

MACHINE_START( ti99_4_60hz )
{
    ti99_common_init(machine, &tms9918_interface);
}

static const TMS9928a_interface tms9929_interface =
{
	TMS9929,
	0x4000,
	13, 13,
	tms9901_set_int2
};

MACHINE_START( ti99_4_50hz )
{
    ti99_common_init(machine, &tms9929_interface);
}

static const TMS9928a_interface tms9918a_interface =
{
	TMS99x8A,
	0x4000,
	15, 15,
	tms9901_set_int2
};

MACHINE_START( ti99_4a_60hz )
{
    ti99_common_init(machine, &tms9918a_interface);
}

static const TMS9928a_interface tms9929a_interface =
{
	TMS9929A,
	0x4000,
	13, 13,
	tms9901_set_int2
};

MACHINE_START( ti99_4a_50hz )
{
    ti99_common_init(machine, &tms9929a_interface);
}

MACHINE_START( ti99_4ev_60hz)
{
    /* has an own VDP, so skip initing the VDP */
    ti99_common_init(machine, 0);
}

void ti99_common_init(running_machine *machine, const TMS9928a_interface *gfxparm) 
{
	if (gfxparm != 0)
		TMS9928A_configure(gfxparm);

        /* Initialize all. Actually, at this point, we don't know
           how the switches are set. Later we use the configuration switches to
           determine which one to use. */
	ti99_peb_init();
	ti99_floppy_controllers_init_all(machine);
	ti99_ide_init(machine);
	ti99_rs232_init(machine);
	ti99_hsgpl_init(machine);
	ti99_usbsm_init(machine);
	
	/* Find the cartslot device and cache it. This is a string search,
	   and we don't want to repeat it on each memory access. */
	cartslots = devtag_get_device(machine, "ti99_multicart");
	assert(cartslots != NULL);
}


/*
	ti99_init_machine(); called before ti99_load_rom...
*/
MACHINE_RESET( ti99 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/*console_GROMs.data_ptr = memory_region(machine, region_grom);*/
	console_GROMs.addr = 0;

	if (ti99_model == model_99_8)
	{
		/* ... */
		ti99_8_enable_rom_and_ports = 1;		/* ??? maybe there is a pull-up */
		/* MZ: already setting this in driver init, otherwise machine locks up */
	}
	else if (ti99_model == model_99_4p)
	{
		UINT8* mem = memory_region(machine, "maincpu");

		/* set up system ROM and scratch pad pointers */
		memory_set_bankptr(machine, 1, mem + offset_rom0_4p);	/* system ROM */
		memory_set_bankptr(machine, 2, mem + offset_sram_4p);	/* scratch pad */
		memory_set_bankptr(machine, 11, mem + offset_dram_4p);	/* extra RAM for debugger */
	}
	else
	{
		/* set up scratch pad pointer */
		memory_set_bankptr(machine, 1, memory_region(machine, "maincpu") + offset_sram);
	}

/*	if (ti99_model != model_99_4p)
	{
		if (ti99_model == model_99_8)
			current_page_ptr_8 = cartridge_pages_8[0];
		else
			current_page_ptr = cartridge_pages[0];
	} */

	if (!has_evpc) TMS9928A_reset();
        else v9938_reset(0);

	/* clear keyboard interface state (probably overkill, but can't harm) */
	KeyCol = 0;
	AlphaLockLine = 0;

	/* reset handset */
	handset_buf = 0;
	handset_buflen = 0;
	handset_clock = 0;
	handset_ack = 0;
	tms9901_set_single_int(devtag_get_device(machine, "tms9901"), 12, 0);

	/* read config */
	if (ti99_model == model_99_8)
		xRAM_kind = xRAM_kind_99_8;			/* hack */
	else if (ti99_model == model_99_4p)
		xRAM_kind = xRAM_kind_99_4p_1Mb;	/* hack */
	else
		xRAM_kind = (input_port_read(machine, "CFG") >> config_xRAM_bit) & config_xRAM_mask;
	if (ti99_model == model_99_8)
		has_speech = TRUE;
	else
		has_speech = (input_port_read(machine, "CFG") >> config_speech_bit) & config_speech_mask;
	fdc_kind = (input_port_read(machine, "CFG") >> config_fdc_bit) & config_fdc_mask;
	has_ide = (input_port_read(machine, "CFG") >> config_ide_bit) & config_ide_mask;
	has_rs232 = (input_port_read(machine, "CFG") >> config_rs232_bit) & config_rs232_mask;
	has_handset = (ti99_model == model_99_4) && ((input_port_read(machine, "CFG") >> config_handsets_bit) & config_handsets_mask);
	has_hsgpl = ((ti99_model == model_99_4p) || (input_port_read(machine, "CFG") >> config_hsgpl_bit) & config_hsgpl_mask);
	has_usb_sm = (input_port_read(machine, "CFG") >> config_usbsm_bit) & config_usbsm_mask;
	has_pcode = (input_port_read(machine, "CFG") >> config_pcode_bit) & config_pcode_mask;
	
	/* set up optional expansion hardware */
	ti99_peb_reset(ti99_model == model_99_4p, tms9901_set_int1, NULL);

	if (ti99_model == model_99_8)
		ti99_8_internal_dsr_reset(machine);

	if (ti99_model == model_99_4p)
		ti99_4p_internal_dsr_reset(machine);

        if (has_speech) 
	{
		static const spchroms_interface speech_intf = { region_speech_rom };

		spchroms_config(machine, &speech_intf);

		if (ti99_model != model_99_8)
		{
			memory_install_read16_handler(space, 0x9000, 0x93ff, 0, 0, ti99_rspeech_r);
			memory_install_write16_handler(space, 0x9400, 0x97ff, 0, 0, ti99_wspeech_w);
		}
	}
	else
	{
		/* which is never true for TI-99/8 */
		memory_install_read16_handler(space, 0x9000, 0x93ff, 0, 0, ti99_nop_8_r);
		memory_install_write16_handler(space, 0x9400, 0x97ff, 0, 0, ti99_nop_8_w);
	}
	/* Check whether we have locked the cartslot. */
	lock_cartridge_slot(cartslots, (input_port_read(machine, "CFG") >> config_cartslot_bit) & config_cartslot_mask);

	switch (xRAM_kind)
	{
	case xRAM_kind_none:
	default:
		memory_install_read16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_nop_8_r);
		memory_install_write16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_nop_8_w);
		memory_install_read16_handler(space, 0xa000, 0xffff, 0, 0, ti99_nop_8_r);
		memory_install_write16_handler(space, 0xa000, 0xffff, 0, 0, ti99_nop_8_w);
		break;
	case xRAM_kind_TI:
		ti99_TIxram_init(machine);
		break;
	case xRAM_kind_super_AMS:
		ti99_sAMSxram_init(machine);
		break;
	case xRAM_kind_99_4p_1Mb:
		ti99_4p_mapper_init(machine);
		break;
	case xRAM_kind_foundation_128k:
	case xRAM_kind_foundation_512k:
	case xRAM_kind_myarc_128k:
	case xRAM_kind_myarc_512k:
		ti99_myarcxram_init(machine);
		break;
	case xRAM_kind_99_8:
		break;
	}

	switch (fdc_kind)
	{
	case fdc_kind_TI:
		ti99_fdc_reset(machine);
		break;
#if HAS_99CCFDC
	case fdc_kind_CC:
		ti99_ccfdc_reset(machine);
		break;
#endif
	case fdc_kind_BwG:
		ti99_bwg_reset(machine);
		break;
	case fdc_kind_hfdc:
		ti99_hfdc_reset(machine);
		break;
	case fdc_kind_none:
		break;
	}

	if (has_ide) {
		ti99_ide_reset(machine, ti99_model == model_99_8);
		ti99_ide_load_memcard(machine);
	}

	if (has_rs232)
		ti99_rs232_reset(machine);

	if (has_hsgpl)	{
		ti99_hsgpl_reset(machine);
		ti99_hsgpl_load_memcard(machine);
	}
	else
		hsgpl_crdena = 0;

	if (has_usb_sm)
		ti99_usbsm_reset(machine, ti99_model == model_99_8);

	if (has_evpc)
		ti99_evpc_reset(machine);

	if (has_pcode)
		ti99_pcode_reset(machine);

	/* initialize mechatronics mouse */
	mecmouse_sel = 0;
	mecmouse_read_y = 0;
	mecmouse_x = 0;
	mecmouse_y = 0;
}

#ifdef UNUSED_FUNCTION
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
#endif


/*
	video initialization.
*/
VIDEO_START( ti99_4ev )
{
	VIDEO_START_CALL(generic_bitmapped);
	v9938_init(machine, 0, machine->primary_screen, tmpbitmap, MODEL_V9938, 0x20000, tms9901_set_int2);	/* v38 with 128 kb of video RAM */
}

/*
	VBL interrupt  (mmm...  actually, it happens when the beam enters the lower
	border, so it is not a genuine VBI, but who cares ?)
*/
INTERRUPT_GEN( ti99_vblank_interrupt )
{
	TMS9928A_interrupt(device->machine);
	if (has_handset)
		ti99_handset_task(device->machine);
	has_mecmouse = (input_port_read(device->machine, "CFG") >> config_mecmouse_bit) & config_mecmouse_mask;
	if (has_mecmouse)
		mecmouse_poll(device->machine);
}

INTERRUPT_GEN( ti99_4ev_hblank_interrupt )
{
	static int line_count;
	v9938_interrupt(device->machine, 0);
	if (++line_count == 262)
	{
		line_count = 0;
		has_mecmouse = (input_port_read(device->machine, "CFG") >> config_mecmouse_bit) & config_mecmouse_mask;
		if (has_mecmouse)
			mecmouse_poll(device->machine);
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
	* actually implement GRAM support and GPL port support
*/

/*
	Same as MRA16_NOP, but with an additionnal delay caused by the 16->8 bit
	bus multiplexer.
*/
READ16_HANDLER ( ti99_nop_8_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	return (0);
}

WRITE16_HANDLER ( ti99_nop_8_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));
}

/*
	99/4p cartridge space: mapped either to internal ROM6 or to HSGPL.
*/
READ16_HANDLER ( ti99_4p_cart_r )
{
	if (ti99_4p_internal_rom6_enable)
		return ti99_4p_internal_ROM6[offset];

	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		return ti99_hsgpl_rom6_r(space, offset, mem_mask);

	return 0;
}

WRITE16_HANDLER ( ti99_4p_cart_w )
{
	if (ti99_4p_internal_rom6_enable)
	{
		ti99_4p_internal_ROM6 = (UINT16 *) (memory_region(space->machine, "maincpu") + (FPTR)((offset & 1) ? offset_rom6b_4p : offset_rom6_4p));
		return;
	}

	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (hsgpl_crdena)
		/* hsgpl is enabled */
		ti99_hsgpl_rom6_w(space, offset, data, mem_mask);
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
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	sn76496_w(devtag_get_device(space->machine, "sn76496"), offset, (data >> 8) & 0xff);
}

/*
	TMS9918(A)/9928(A)/9929(A) VDP read
*/
READ16_HANDLER ( ti99_rvdp_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (offset & 1)
	{	/* read VDP status */
		return ((int) TMS9928A_register_r(space, 0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) TMS9928A_vram_r(space, 0)) << 8;
	}
}

/*
	TMS9918(A)/9928(A)/9929(A) vdp write
*/
WRITE16_HANDLER ( ti99_wvdp_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (offset & 1)
	{	/* write VDP address */
		TMS9928A_register_w(space, 0, (data >> 8) & 0xff);
	}
	else
	{	/* write VDP data */
		TMS9928A_vram_w(space, 0, (data >> 8) & 0xff);
	}
}

/*
	V38 VDP read
*/
READ16_HANDLER ( ti99_rv38_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (offset & 1)
	{	/* read VDP status */
		return ((int) v9938_0_status_r(space, 0)) << 8;
	}
	else
	{	/* read VDP RAM */
		return ((int) v9938_0_vram_r(space, 0)) << 8;
	}
}

/*
	V38 vdp write
*/
WRITE16_HANDLER ( ti99_wv38_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	switch (offset & 3)
	{
	case 0:
		/* write VDP data */
		v9938_0_vram_w(space, 0, (data >> 8) & 0xff);
		break;
	case 1:
		/* write VDP address */
		v9938_0_command_w(space, 0, (data >> 8) & 0xff);
		break;
	case 2:
		/* write VDP palette */
		v9938_0_palette_w(space, 0, (data >> 8) & 0xff);
		break;
	case 3:
		/* write VDP register */
		v9938_0_register_w(space, 0, (data >> 8) & 0xff);
		break;
	}
}

/*
	TMS0285 speech chip read
*/
static READ16_HANDLER ( ti99_rspeech_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-(18+3));		/* this is just a minimum, it can be more */

	return ((int) tms5220_status_r(devtag_get_device(space->machine, "tms5220"), offset)) << 8;
}

#if 0

static void speech_kludge_callback(int dummy)
{
	if (! tms5220_ready_r())
	{
		/* Weirdly enough, we are always seeing some problems even though
		everything is working fine. */
		attotime time_to_ready = double_to_attotime(tms5220_time_to_ready());
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
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-(54+3));		/* this is just an approx. minimum, it can be much more */

#if 1
	/* the stupid design of the tms5220 core means that ready is cleared when
	there are 15 bytes in FIFO.  It should be 16.  Of course, if it were the
	case, we would need to store the value on the bus, which would be more
	complex. */
	if (! tms5220_ready_r(devtag_get_device(space->machine, "tms5220")))
	{
		attotime time_to_ready = double_to_attotime(tms5220_time_to_ready(devtag_get_device(space->machine, "tms5220")));
		int cycles_to_ready = cputag_attotime_to_clocks(space->machine, "maincpu", time_to_ready);

		logerror("time to ready: %f -> %d\n", attotime_to_double(time_to_ready), (int) cycles_to_ready);

		cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-cycles_to_ready);
		timer_set(space->machine, attotime_zero, NULL, 0, /*speech_kludge_callback*/NULL);
	}
#endif

	tms5220_data_w(devtag_get_device(space->machine, "tms5220"), offset, (data >> 8) & 0xff);
}

static UINT8 GROM_dataread(void)
{
	UINT8 reply;
	if (console_GROMs.addr >= 0x6000)
	{
		/* Pass the (one and only) program counter for GROMs. The 
		   buffer is set and used in the cartridge chip. */
		reply = cartridge_grom_read(cartslots, console_GROMs.addr-0x6000);
	}
	else 
	{
		/* GROMs are buffered. Data is retrieved from a buffer, 
		while the buffer is replaced with the next cell 
		content. */
		reply = console_GROMs.buf;
		
		/* Get next value, put it in buffer. Note that the
		GROM wraps at 8K boundaries. */
		console_GROMs.buf = console_GROMs.data_ptr[console_GROMs.addr];
	}

//	printf("GROM address = %04x, reply = %02x\n", console_GROMs.addr, reply>>8);
	/* The program counter wraps at each GROM chip size (8K), 
	   so 0x5fff + 1 = 0x4000. */
	console_GROMs.addr = ((console_GROMs.addr + 1) & 0x1FFF) | (console_GROMs.addr & 0xE000);
	
	/* Reset the read and write address flipflops. */
	console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;

	return reply;
}

/*
	GPL read
*/
READ16_HANDLER ( ti99_grom_r )
{
	UINT16 reply;

	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4 /*20+3*/);		/* from 4 to 23? */
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	/* This implementation features a multislot cartridge system which
	   is based on multiple GROM base addresses. The standard base 
	   is >9800 (slot 0). When we access the port >9804, we switch to 
	   slot 1, >9808 is slot 2, ... >983C is slot 15. Although theoretically
	   we could address up to 256 banks, the TI operating system does not
	   check more than 16 banks.
	   Cartridges may also contain ROMs which need to be banked 
	   simultaneously. We use the cartridge slot number to swap the ROM at
	   >6000, or RAM if available. I don't know whether this is the way
	   Texas Instruments envisaged it in the OS, but it is the only
	   plausible way to allow for GROM+ROM cartridges.
	   Note that some cartridges may have programming flaws which only 
	   appear when the cartridge is used in a higher-numbered slot.
	   Parsec is one example which only runs in slot 0 as the 
	   programmers hard-coded an access to GROM port 0. Note: This *may*
	   be worked around if we make sure that switching only occurs
	   when the address is set, not when data is read or written. But
	   it's not clear whether this has unwanted side effects.
	*/
	
	/* Activates a slot in the multi-cartridge extender. */
	cartridge_slot_set(cartslots, (offset & 0x01fe)/2);
	
	if (offset & 1)
	{	/* Read GROM address
		   We only have one GROM program counter for the console GROMs
		   and all cartridge GROMs in all banks(!). Hardware 
		   implementations must take this into account as well. Thus,
		   we use a structure here in the console, rather than separate
		   counters in each GROM chip. */
		
		/* When reading, reset the hi/lo flag byte for writing. 
		   TODO: Verify this with a real machine. */
		console_GROMs.waddr_LSB = FALSE;

		/* Address reading is done in two steps; first, the high byte
		   is transferred, then the low byte. */
		if (console_GROMs.raddr_LSB)
		{
			/* second pass */
			reply = (console_GROMs.addr << 8) & 0xff00;
			console_GROMs.raddr_LSB = FALSE;
		}
		else
		{
			/* first pass */
			reply = console_GROMs.addr & 0xff00;
			console_GROMs.raddr_LSB = TRUE;
		}
	}
	else
	{	
		/* Read GROM data */
		reply = ((UINT16)GROM_dataread()) << 8;		
	}

	if (hsgpl_crdena)
		/* hsgpl buffers are stronger than console GROM buffers */
		reply = ti99_hsgpl_gpl_r(space, offset, mem_mask);

	return reply;
}

/*
	GPL write
*/
WRITE16_HANDLER ( ti99_grom_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4/*20+3*/);		/* from 4 to 23? */
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	/* Activates a slot in the multi-cartridge extender. */
	cartridge_slot_set(cartslots, (offset & 0x01fe)/2);

	// 1001 1wbb bbbb bbr0
	
	if (offset & 1)
	{	/* write GROM address */
		/* see comments above */
		console_GROMs.raddr_LSB = FALSE;
		
		/* Implements the internal flipflop.
		   The Editor/Assembler manuals says that the current address
		   plus one is returned. This effect is properly emulated
		   as we are using a read-ahead buffer.
		*/
		if (console_GROMs.waddr_LSB)
		{
			/* Accept low byte (2nd write) */
			console_GROMs.addr = (console_GROMs.addr & 0xFF00) | ((data >> 8) & 0xFF);
			
			/* Setting the address causes a new read, putting the 
			   value into the buffer. We don't need the value here. */
			GROM_dataread();
			console_GROMs.waddr_LSB = FALSE;
		}
		else
		{
			/* Accept high byte (1st write) */
			console_GROMs.addr = (data & 0xFF00) | (console_GROMs.addr & 0xFF);
			console_GROMs.waddr_LSB = TRUE;
		}

	}
	else
	{
		/* write GROM data */
		/* the console GROMs are always affected */
		/* BTW, console GROMs are never GRAMs, therefore there is no
		   need to actually write anything, so we just read ahead 
		   TODO: There could be cartridges with GRAM, so this must be
		   fixed.
		*/
		GROM_dataread();
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;
	}

	if (hsgpl_crdena)
		ti99_hsgpl_gpl_w(space, offset, data, mem_mask);
}

/*
	GROM read for TI-99/8
	For comments, see the handler for TI-99/4A
*/
static READ8_HANDLER ( ti99_grom_r8 )
{
	UINT8 reply;

	cpu_adjust_icount(space->machine->cpu[0],-4);
	
	/* Activates a slot in the multi-cartridge extender. */
	// 1001 1wbb bbbb bbr0
	cartridge_slot_set(cartslots, (offset & 0x03fc)/4);
	
	if (offset & 2)
	{	
		console_GROMs.waddr_LSB = FALSE;

		if (console_GROMs.raddr_LSB)
		{
			reply = console_GROMs.addr & 0x00ff;
			console_GROMs.raddr_LSB = FALSE;
		}
		else
		{
			reply = (console_GROMs.addr>>8) & 0x00ff;
			console_GROMs.raddr_LSB = TRUE;
		}
	}
	else
		reply = GROM_dataread();		

	return reply;
}

/*
	GROM write for TI-99/8
	For comments, see the handler for TI-99/4A
*/
static WRITE8_HANDLER ( ti99_grom_w8 )
{
	cpu_adjust_icount(space->machine->cpu[0],-4/*20+3*/);		/* from 4 to 23? */

	/* Activates a slot in the multi-cartridge extender. */
	cartridge_slot_set(cartslots, (offset & 0x03fc)/4);
	
	if (offset & 2)
	{
		console_GROMs.raddr_LSB = FALSE;
		if (console_GROMs.waddr_LSB)
		{
			console_GROMs.addr = (console_GROMs.addr & 0xFF00) | (data & 0xFF);
			
			GROM_dataread();
			console_GROMs.waddr_LSB = FALSE;
		}
		else
		{
			console_GROMs.addr = ((data<<8) & 0xFF00) | (console_GROMs.addr & 0xFF);
			console_GROMs.waddr_LSB = TRUE;
		}

	}
	else
	{
		GROM_dataread();
		console_GROMs.raddr_LSB = console_GROMs.waddr_LSB = FALSE;
	}
}


/*
	GROM read for TI-99/4p
*/
READ16_HANDLER ( ti99_4p_grom_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);		/* HSGPL is located on 8-bit bus? */
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	return /*hsgpl_crdena ?*/ ti99_hsgpl_gpl_r(space, offset, mem_mask) /*: 0*/;
}

/*
	GROM write
*/
WRITE16_HANDLER ( ti99_4p_grom_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);		/* HSGPL is located on 8-bit bus? */
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	/*if (hsgpl_crdena)*/
		ti99_hsgpl_gpl_w(space, offset, data, mem_mask);
}


#if 0
#pragma mark -
#pragma mark 99/8 MEMORY HANDLERS
#endif

READ8_HANDLER( ti99_8_r )
{
	/* Set the page. Page size is 4 KiB; page number are the high 4 bits. */
	int page = offset >> 12;
	UINT32 mapper_reg;
	UINT8 reply = 0;

	if (ti99_8_enable_rom_and_ports)
	{
		/* This is only important for address access 0000-1fff and 
		   8000-9fff. Outside, the mapper mode is used. */
		if ((offset >= 0x0000) && (offset < 0x2000))
		{
			/* ROM */
			reply = ROM0_ptr_8[offset & 0x1fff];
			return reply;
		}
		else if ((offset >= 0x8000) && (offset < 0xa000))
		{
			/* ti99 scratch pad and memory-mapped registers */
			/* 0x8000-0x9fff */
			switch ((offset - 0x8000) >> 10)
			{
			case 0:
				/* RAM: >8000 - >83ff */
				reply = sRAM_ptr_8[offset & 0x1fff];
				break;

			case 1:
				/* >8400 - >840f: sound (cannot read) 
				   >8410 - >87ff: SRAM */
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
							/* read VDP status >8802 */
							reply = TMS9928A_register_r(space, 0);
						else
							/* read VDP RAM >8800 */
							reply = TMS9928A_vram_r(space, 0);
					}
				}
				else
				{
					/* Read mapper >8810 */
					reply = ti99_8_mapper_status;
					ti99_8_mapper_status = 0;
				}
				break;

			case 4:
				/* speech read: >9000 */
				if (! (offset & 1))
				{
					cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-16*4);		/* this is just a minimum, it can be more */
					reply = tms5220_status_r(devtag_get_device(space->machine, "tms5220"), 0);
				}
				break;

			case 6:
				/* GROM read: >9800 */
				if (! (offset & 1))
					reply = ti99_grom_r8(space, offset);
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
			/* ff0000 - ff1fff: unassigned? */
		case 1:
			/* ff2000 - ff3fff: ? */
		case 4:
			/* ff8000 - ff9fff: internal DSR ROM? (normally enabled with a write to CRU >2700) */
		case 7:
			/* ffe000 - ffffff: ? */
			logerror("unmapped read page=%d offs=%d\n", (int) page, (int) offset);
			break;

		case 2:
			/* ff4000 - ff5fff: DSR space */
			reply = ti99_8_peb_r(space, offset & 0x1fff);
			break;

		case 3:
			/* ff6000 - ff7fff: cartridge ROM space */
			offset &= 0x1fff;

			/* We do not support hsgpl here because it is a 16 bit 
			   system. Just check the cartridges.  */
			reply = ti99_multicart8_r(multicart8, offset); 
			break;

		case 5:
			/* ffa000 - ffbfff: >2000 ROM (ROM1) */
			reply = ROM1_ptr_8[offset & 0x1fff];
			break;

		case 6:
			/* ffc000 - ffdfff: >6000 ROM */
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

	if (ti99_8_enable_rom_and_ports)
	{
		/* memory mapped ports enabled */
		if ((offset >= 0x0000) && (offset < 0x2000))
		{
			/* ROM? */
			return;
		}
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
					sn76496_w(devtag_get_device(space->machine, "sn76496"), offset, data);
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
						TMS9928A_register_w(space, 0, data);
					else
						/* read VDP RAM */
						TMS9928A_vram_w(space, 0, data);
				}
				break;

			case 5:
				/* speech write */
				if (! (offset & 1))
				{
					cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-48*4);		/* this is just an approx. minimum, it can be much more */

					/* the stupid design of the tms5220 core means that ready is cleared when
					there are 15 bytes in FIFO.  It should be 16.  Of course, if it were the
					case, we would need to store the value on the bus, which would be more
					complex. */
					if (! tms5220_ready_r(devtag_get_device(space->machine, "tms5220")))
					{
						attotime time_to_ready = double_to_attotime(tms5220_time_to_ready(devtag_get_device(space->machine, "tms5220")));
						double d = ceil(cputag_attotime_to_clocks(space->machine, "maincpu", time_to_ready));
						int cycles_to_ready = ((int) (d + 3)) & ~3;

						logerror("time to ready: %f -> %d\n", attotime_to_double(time_to_ready)
							, (int) cycles_to_ready);

						cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-cycles_to_ready);
						timer_set(space->machine, attotime_zero, NULL, 0, /*speech_kludge_callback*/NULL);
					}

					tms5220_data_w(devtag_get_device(space->machine, "tms5220"), offset, data);
				}
				break;

			case 7:
				/* GPL write */
				if (! (offset & 1))
					ti99_grom_w8(space, offset, data);
				break;

			default:
				logerror("unmapped write offs=%d data=%d\n", (int) offset, (int) data);
				break;
			}
			return;
		}
	}

	mapper_reg = ti99_8_mapper_regs[page];
	
	/* 4 KiB page size */ 
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
			ti99_8_peb_w(space, offset & 0x1fff, data);
			break;

		case 3:
			/* cartridge space */
			/* We do not support HSGPL for the 99/8 at this time. */
			ti99_multicart8_w(multicart8, offset & 0x1fff, data); 
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
	{
		logerror("unmapped write page=%d offs=%d\n", (int) page, (int) offset);
	}
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
static TIMER_CALLBACK(ti99_handset_ack_callback)
{
	handset_clock = ! handset_clock;
	handset_buf >>= 4;
	handset_buflen--;
	tms9901_set_single_int(devtag_get_device(machine, "tms9901"), 12, 0);

	if (handset_buflen == 1)
	{
		/* Unless I am missing something, the third and last nybble of the
		message is not acknowledged by the DSR in any way, and the first nybble
		of next message is not requested for either, so we need to decide on
		our own when we can post a new event.  Currently, we wait for 1000us
		after the DSR acknowledges the second nybble. */
		timer_set(machine, ATTOTIME_IN_USEC(1000), NULL, 0, ti99_handset_ack_callback);
	}

	if (handset_buflen == 0)
		/* See if we need to post a new event */
		ti99_handset_task(machine);
}

/*
	ti99_handset_set_ack()

	Handler for tms9901 P0 pin (handset data acknowledge)
*/
static WRITE8_DEVICE_HANDLER( ti99_handset_set_ack )
{
	if (has_handset && handset_buflen && (data != handset_ack))
	{
		handset_ack = data;
		if (data == handset_clock)
			/* I don't know what the real delay is, but 30us apears to be enough */
			timer_set(device->machine, ATTOTIME_IN_USEC(30), NULL, 0, ti99_handset_ack_callback);
	}
}

/*
	ti99_handset_post_message()

	Post a 12-bit message: trigger an interrupt on the tms9901, and store the
	message in the I/R receiver buffer so that the handset ISR will read this
	message.

	message: 12-bit message to post (only the 12 LSBits are meaningful)
*/
static void ti99_handset_post_message(running_machine *machine, int message)
{
	/* post message and assert interrupt */
	handset_clock = 1;
	handset_buf = ~ message;
	handset_buflen = 3;
	tms9901_set_single_int(devtag_get_device(machine, "tms9901"), 12, 1);
}

/*
	ti99_handset_poll_keyboard()

	Poll the current state of one given handset keypad.

	num: number of the keypad to poll (0-3)

	Returns TRUE if the handset state has changed and a message was posted.
*/
static int ti99_handset_poll_keyboard(running_machine *machine, int num)
{
	static UINT8 previous_key[max_handsets];

	UINT32 key_buf;
	UINT8 current_key;
	int i;
	static const char *const keynames[] = { "KP0", "KP1", "KP2", "KP3", "KP4" };

	/* read current key state */
	key_buf = ( input_port_read(machine, keynames[num]) | (input_port_read(machine, keynames[num + 1]) << 16) ) >> (4*num);

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
			ti99_handset_post_message(machine, (((unsigned) current_key) << 4) | (num << 1));

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
		ti99_handset_post_message(machine, (((unsigned) current_key) << 4) | (num << 1));

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
static int ti99_handset_poll_joystick(running_machine *machine, int num)
{
	static UINT8 previous_joy[max_handsets];
	UINT8 current_joy;
	int current_joy_x, current_joy_y;
	int message;
	static const char *const joynames[2][4] =
			{
				{ "JOY0", "JOY2", "JOY4", "JOY6" },		// X axis
				{ "JOY1", "JOY3", "JOY5", "JOY7" }		// Y axis
			};

	/* read joystick position */
	current_joy_x = input_port_read(machine, joynames[0][num]);
	current_joy_y = input_port_read(machine, joynames[1][num]);
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
		ti99_handset_post_message(machine, message);

		return TRUE;
	}

	return FALSE;
}

/*
	ti99_handset_task()

	Manage handsets, posting an event if the state of any handset has changed.
*/
static void ti99_handset_task(running_machine *machine)
{
	int i;

	if (handset_buflen == 0)
	{	/* poll every handset */
		for (i=0; i<max_handsets; i++)
			if (ti99_handset_poll_joystick(machine, i))
				return;
		for (i=0; i<max_handsets; i++)
			if (ti99_handset_poll_keyboard(machine, i))
				return;
	}
	else if (handset_buflen == 3)
	{	/* update messages after they have been posted */
		if (handset_buf & 1)
		{	/* keyboard */
			ti99_handset_poll_keyboard(machine, (~ (handset_buf >> 1)) & 0x3);
		}
		else
		{	/* joystick */
			ti99_handset_poll_joystick(machine, (~ (handset_buf >> 1)) & 0x3);
		}
	}
}


/*===========================================================================*/
#if 0
#pragma mark -
#pragma mark MECHATRONICS MOUSE
#endif

static void mecmouse_select(int selnow, int stick1, int stick2)
{
	/* The Mechatronic mouse is connected to the joystick port and occupies
	   both joystick select lines and the switch lines. From these five 
	   lines, left/right/down are used for the motion (i.e. 3 motion steps 
	   for positive and four for negative motion and one for rest), 
	   the fire line is used for the secondary mouse button, and the up 
	   line is used for the primary button.
	   The mouse motion is delivered by the same lines for both directions;
	   this requires swapping the axes. According to the source code of
	   the accompanying mouse driver, the readout of the current axis is
	   done by selecting joystick 1, then joystick 2. The axis swapping is
	   achieved by selecting stick 1 again. When selecting stick 2, the
	   second axis is seen on the input lines.
	   Interrupting this sequence will lead to swapped axes. This is
	   prevented by resetting the toggle when the mouse is deselected 
	   (neither 1 nor 2 are selected).

	   The joystick lines are selected as follows:
	   TI-99/4:  Stick 1: P4=1, P3=0, P2=1 (5)
	             Stick 2: P4=1, P3=1, P2=0 (6)
		
	   TI-99/4A: Stick 1: P4=1, P3=1, P2=0 (6)
	             Stick 2: P4=1, P3=1, P2=1 (7)

	   TI-99/8:  Stick 1: P3=1, P2=1, P1=1, P0=0 (14)
	             Stick 2: P3=1, P2=1, P1=1, P0=1 (15)
		     
	   Geneve: n/a, has own mouse handling via v9938
	   
	   As we can only deliver at max 3 steps positive and 4 steps negative,
	   we split the delta so that subsequent queries add up to the actual
	   delta. That is, one delta of +10 yields a 3+3+3+1. 
	   
	   mecmouse_x holds the current delta to be counted down for x
	   (y accordingly)

	   mecmouse_x_buf is the current step count reported to CRU
	   
	   Michael Zapf, 2008-01-22
	*/
	
	if (selnow==stick2) {
		if (mecmouse_sel==stick1) {
			if (! mecmouse_read_y)
			{
				/* Sample x motion. */
				if (mecmouse_x < -4)
					mecmouse_x_buf = -4;
				else if (mecmouse_x > 3)
					mecmouse_x_buf = 3;
				else
					mecmouse_x_buf = mecmouse_x;
				mecmouse_x -= mecmouse_x_buf;
				mecmouse_x_buf = (mecmouse_x_buf-1) & 7;
			}
			else 
			{
				/* Sample y motion. */
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
		mecmouse_sel = selnow;
	}
	else if (selnow==stick1) 
	{
		if (mecmouse_sel==stick2)
		{
			/* Swap the axes. */
			mecmouse_read_y = ! mecmouse_read_y;
		}
		mecmouse_sel = selnow;
	}
	else 
	{
		/* Reset the axis toggle when the mouse is deselected */
		mecmouse_read_y = 0;
	}
}

static void mecmouse_poll(running_machine *machine)
{
	static int last_mx = 0, last_my = 0;
	int new_mx, new_my;
	int delta_x, delta_y;

	new_mx = input_port_read(machine, "MOUSEX");
	new_my = input_port_read(machine, "MOUSEY");

	/* compute x delta */
	delta_x = new_mx - last_mx;
	
	/* check for wrap */
	if (delta_x > 0x80)
		delta_x = -0x100+delta_x;
	if  (delta_x < -0x80)
		delta_x = 0x100+delta_x;

	/* Prevent unplausible values at startup. */
	if (delta_x > 100 || delta_x<-100) delta_x = 0;

	last_mx = new_mx;

	/* compute y delta */
	delta_y = new_my - last_my;

	/* check for wrap */
	if (delta_y > 0x80)
		delta_y = -0x100+delta_y;
	if  (delta_y < -0x80)
		delta_y = 0x100+delta_y;

	if (delta_y > 100 || delta_y<-100) delta_y = 0;

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
static void tms9901_set_int1(running_machine *machine, int state)
{
	tms9901_set_single_int(devtag_get_device(machine, "tms9901"), 1, state);
}

/*
	set the state of int2 (called by the tms9928 core)
*/
void tms9901_set_int2(running_machine *machine, int state)
{
	tms9901_set_single_int(devtag_get_device(machine, "tms9901"), 2, state);
}

/*
	Called by the 9901 core whenever the state of INTREQ and IC0-3 changes
*/
static TMS9901_INT_CALLBACK( tms9901_interrupt_callback )
{
	if (intreq)
	{
		/* On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
		 * but hard-wired to force level 1 interrupts */
		cputag_set_input_line_and_vector(device->machine, "maincpu", 0, ASSERT_LINE, 1);	/* interrupt it, baby */
	}
	else
	{
		cputag_set_input_line(device->machine, "maincpu", 0, CLEAR_LINE);
	}
}

/*
	Read pins INT3*-INT7* of TI99's 9901.

	signification:
	 (bit 1: INT1 status)
	 (bit 2: INT2 status)
	 bit 3-7: keyboard status bits 0 to 4
*/
static READ8_DEVICE_HANDLER( ti99_R9901_0 )
{
	int answer;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

	if ((ti99_model == model_99_4) && (KeyCol == 7))
		answer = (ti99_handset_poll_bus() << 3) | 0x80;
	else if (has_mecmouse && (KeyCol == ((ti99_model == model_99_4) ? 6 : 7)))
	{
		int buttons = input_port_read(device->machine, "MOUSE0") & 3;

		answer = (mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 4;

		if (! (buttons & 1))
			/* action button */
			answer |= 0x08;
		if (! (buttons & 2))
			/* home button */
			answer |= 0x80;
	}
	else
	{
		answer = ((input_port_read(device->machine, keynames[KeyCol >> 1]) >> ((KeyCol & 1) * 8)) << 3) & 0xF8;
	}
	
	if ((ti99_model == model_99_4a) || (ti99_model == model_99_4p))
	{
		if (! AlphaLockLine)
			answer &= ~ (input_port_read(device->machine, "ALPHA") << 3);
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
static READ8_DEVICE_HANDLER( ti99_R9901_1 )
{
	int answer;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

	if (/*(ti99_model == model_99_4) &&*/ (KeyCol == 7))
		answer = 0x07;
	else
	{
		answer = ((input_port_read(device->machine, keynames[KeyCol >> 1]) >> ((KeyCol & 1) * 8)) >> 5) & 0x07;
	}
	
	/* we don't take CS2 into account, as CS2 is a write-only unit */
	/*if (cassette_input(devtag_get_device(device->machine, "cassette1")) > 0)
		answer |= 8;*/

	return answer;
}

/*
	Read pins P0-P7 of TI99's 9901.

	 bit 1: handset data clock pin
*/
static READ8_DEVICE_HANDLER( ti99_R9901_2 )
{
	return (has_handset && handset_clock) ? 2 : 0;
}

/*
	Read pins P8-P15 of TI99's 9901.

	 bit 26: IR handset interrupt
	 bit 27: tape input
*/
static READ8_DEVICE_HANDLER( ti99_R9901_3 )
{
	int answer;

	if (has_handset && (handset_buflen == 3))
		answer = 0;
	else
		answer = 4;	/* on systems without handset, the pin is pulled up to avoid spurious interrupts */

	/* we don't take CS2 into account, as CS2 is a write-only unit */
	if ( ti99_model != model_99_8 )
	{
		if (cassette_input(devtag_get_device(device->machine, "cassette1")) > 0)
			answer |= 8;
	}
	else
	{
		if (cassette_input(devtag_get_device(device->machine, "cassette")) > 0)
			answer |= 8;
	}

	return answer;
}


/*
	WRITE key column select (P2-P4)
*/
static WRITE8_DEVICE_HANDLER( ti99_KeyC )
{
	if (data)
		KeyCol |= 1 << (offset-2);
	else
		KeyCol &= ~ (1 << (offset-2));
	
        if (has_mecmouse)
                mecmouse_select(KeyCol, (ti99_model == model_99_4) ? 5 : 6,
					(ti99_model == model_99_4) ? 6 : 7); 
}

/*
	WRITE alpha lock line - TI99/4a only (P5)
*/
static WRITE8_DEVICE_HANDLER( ti99_AlphaW )
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
static READ8_DEVICE_HANDLER( ti99_8_R9901_0 )
{
	int answer;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7",
										"KEY8", "KEY9", "KEY10", "KEY11", "KEY12", "KEY13", "KEY14", "KEY15" };

	if (has_mecmouse && (KeyCol == 15))
	{
		int buttons = input_port_read(device->machine, "MOUSE0") & 3;

		answer = ((mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 7) & 0x80;

		if (! (buttons & 1))
			/* action button */
			answer |= 0x40;
	}
	else
	{
		answer = (input_port_read(device->machine, keynames[KeyCol]) << 6) & 0xC0;
	}
	
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
static READ8_DEVICE_HANDLER( ti99_8_R9901_1 )
{
	int answer;
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7",
										"KEY8", "KEY9", "KEY10", "KEY11", "KEY12", "KEY13", "KEY14", "KEY15" };

	if (has_mecmouse && (KeyCol == 15))
	{
		int buttons = input_port_read(device->machine, "MOUSE0") & 3;

		answer = ((mecmouse_read_y ? mecmouse_y_buf : mecmouse_x_buf) << 1) & 0x03;

		if (! (buttons & 2))
			/* home button */
			answer |= 0x04;
	}
	else
	{
		answer = (input_port_read(device->machine, keynames[KeyCol]) >> 2) & 0x07;
	}
	
	/* we don't take CS2 into account, as CS2 is a write-only unit */
	/*if (cassette_input(devtag_get_device(machine, "cassette")) > 0)
		answer |= 8;*/

	return answer;
}

/*
	WRITE key column select (P0-P3)
*/
static WRITE8_DEVICE_HANDLER( ti99_8_KeyC )
{
	if (data)
		KeyCol |= 1 << offset;
	else
		KeyCol &= ~ (1 << offset);

	if (has_mecmouse)
		mecmouse_select(KeyCol, 14, 15);
}

static WRITE8_DEVICE_HANDLER( ti99_8_WCRUS )
{
	ti99_8_enable_rom_and_ports = data;
}

static WRITE8_DEVICE_HANDLER( ti99_8_PTGEN )
{
	/* ... */
}

/*
	command CS1/CS2 tape unit motor (P6-P7)
*/
static WRITE8_DEVICE_HANDLER( ti99_CS_motor )
{
	const device_config *img;

	if ( ti99_model != model_99_8 )
	{
		img = devtag_get_device(device->machine, (offset-6 ) ? "cassette2" :  "cassette1" );
	}
	else
	{
		img = devtag_get_device(device->machine, "cassette");
	}
	cassette_change_state(img, data ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

/*
	audio gate (P8)

	Set to 1 before using tape: this enables the mixing of tape input sound
	with computer sound.

	We do not really need to emulate this as the tape recorder generates sound
	on its own.
*/
static WRITE8_DEVICE_HANDLER( ti99_audio_gate )
{
}

/*
	tape output (P9)
	I think polarity is correct, but don't take my word for it.
*/
static WRITE8_DEVICE_HANDLER( ti99_CS_output )
{
	if (ti99_model != model_99_8)	/* 99/8 only has one tape port!!! */
	{
		cassette_output(devtag_get_device(device->machine, "cassette1"), data ? +1 : -1);
		cassette_output(devtag_get_device(device->machine, "cassette2"), data ? +1 : -1);
	}
	else
	{
		cassette_output(devtag_get_device(device->machine, "cassette"), data ? +1 : -1);
	}
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
static void ti99_8_internal_dsr_cru_w(running_machine *machine, int offset, int data);
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
static void ti99_8_internal_dsr_reset(running_machine *machine)
{
	ti99_8_internal_DSR = memory_region(machine, "maincpu") + offset_rom0_8 + 0x4000;

	ti99_peb_set_card_handlers(0x2700, & ti99_8_internal_dsr_handlers);
}

/* write CRU bit:
	bit0: enable/disable internal DSR ROM,
	bit1: hard reset */
static void ti99_8_internal_dsr_cru_w(running_machine *machine, int offset, int data)
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
static void ti99_4p_internal_dsr_cru_w(running_machine *machine, int offset, int data);
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
static void ti99_4p_internal_dsr_reset(running_machine *machine)
{
	UINT8* mem = memory_region(machine, "maincpu");

	ti99_4p_internal_DSR = (UINT16 *) (mem + offset_rom4_4p);
	ti99_4p_internal_ROM6 = (UINT16 *) (mem + offset_rom6_4p);

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
static void ti99_4p_internal_dsr_cru_w(running_machine *machine, int offset, int data)
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

static void ti99_TIxram_init(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_read16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_TIxramlow_r);
	memory_install_write16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_TIxramlow_w);
	memory_install_read16_handler(space, 0xa000, 0xffff, 0, 0, ti99_TIxramhigh_r);
	memory_install_write16_handler(space, 0xa000, 0xffff, 0, 0, ti99_TIxramhigh_w);
}

/* low 8 kb: 0x2000-0x3fff */
static READ16_HANDLER ( ti99_TIxramlow_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	return xRAM_ptr[offset];
}

static WRITE16_HANDLER ( ti99_TIxramlow_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	COMBINE_DATA(xRAM_ptr + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_TIxramhigh_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_TIxramhigh_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	COMBINE_DATA(xRAM_ptr + offset+0x1000);
}


/*===========================================================================*/
/*
	Super AMS memory extension support.

	Up to 1Mb of SRAM.  Straightforward mapper, works with 4kb chunks.  The mapper was designed
	to be extendable with other RAM areas.
*/

/* prototypes */
static void sAMS_cru_w(running_machine *machine, int offset, int data);
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
static void ti99_sAMSxram_init(running_machine *machine)
{
	int i;

	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_read16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_sAMSxramlow_r);
	memory_install_write16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_sAMSxramlow_w);
	memory_install_read16_handler(space, 0xa000, 0xffff, 0, 0, ti99_sAMSxramhigh_r);
	memory_install_write16_handler(space, 0xa000, 0xffff, 0, 0, ti99_sAMSxramhigh_w);

	ti99_peb_set_card_handlers(0x1e00, & sAMS_expansion_handlers);

	sAMS_mapper_on = 0;

	for (i=0; i<16; i++)
		sAMSlookup[i] = i << 11;
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void sAMS_cru_w(running_machine *machine, int offset, int data)
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
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]];
	else
		return xRAM_ptr[offset+0x1000];
}

static WRITE16_HANDLER ( ti99_sAMSxramlow_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (sAMS_mapper_on)
		COMBINE_DATA(xRAM_ptr + (offset&0x7ff)+sAMSlookup[(0x1000+offset)>>11]);
	else
		COMBINE_DATA(xRAM_ptr + offset+0x1000);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_sAMSxramhigh_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

	if (sAMS_mapper_on)
		return xRAM_ptr[(offset&0x7ff)+sAMSlookup[(0x5000+offset)>>11]];
	else
		return xRAM_ptr[offset+0x5000];
}

static WRITE16_HANDLER ( ti99_sAMSxramhigh_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);
//	cpu_spinuntil_time(space->machine->cpu[0], ATTOTIME_IN_USEC(6));

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
static void ti99_4p_mapper_cru_w(running_machine *machine, int offset, int data);
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
static void ti99_4p_mapper_init(running_machine *machine)
{
	int i;

	/* Not required at run-time */
	/*memory_install_read16_handler(space, 0x2000, 0x2fff, SMH_BANK(3));
	memory_install_write16_handler(space, 0x2000, 0x2fff, SMH_BANK(3));
	memory_install_read16_handler(space, 0x3000, 0x3fff, SMH_BANK(4));
	memory_install_write16_handler(space, 0x3000, 0x3fff, SMH_BANK(4));
	memory_install_read16_handler(space, 0xa000, 0xafff, SMH_BANK(5));
	memory_install_write16_handler(space, 0xa000, 0xafff, SMH_BANK(5));
	memory_install_read16_handler(space, 0xb000, 0xbfff, SMH_BANK(6));
	memory_install_write16_handler(space, 0xb000, 0xbfff, SMH_BANK(6));
	memory_install_read16_handler(space, 0xc000, 0xcfff, SMH_BANK(7));
	memory_install_write16_handler(space, 0xc000, 0xcfff, SMH_BANK(7));
	memory_install_read16_handler(space, 0xd000, 0xdfff, SMH_BANK(8));
	memory_install_write16_handler(space, 0xd000, 0xdfff, SMH_BANK(8));
	memory_install_read16_handler(space, 0xe000, 0xefff, SMH_BANK(9));
	memory_install_write16_handler(space, 0xe000, 0xefff, SMH_BANK(9));
	memory_install_read16_handler(space, 0xf000, 0xffff, SMH_BANK(10));
	memory_install_write16_handler(space, 0xf000, 0xffff, SMH_BANK(10));*/

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
			memory_set_bankptr(machine,3+(i-2), xRAM_ptr + (i<<11));
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			memory_set_bankptr(machine,5+(i-10), xRAM_ptr + (i<<11));
			break;
		}
	}
}

/* write CRU bit:
	bit0: enable/disable mapper registers in DSR space,
	bit1: enable/disable address mapping */
static void ti99_4p_mapper_cru_w(running_machine *machine, int offset, int data)
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
					memory_set_bankptr(machine,3+(i-2), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
					break;

				case 10:
				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
					memory_set_bankptr(machine,5+(i-10), xRAM_ptr + (ti99_4p_mapper_on ? (ti99_4p_mapper_lookup[i]) : (i<<11)));
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
			memory_set_bankptr(space->machine,3+(page-2), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;

		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			memory_set_bankptr(space->machine,5+(page-10), xRAM_ptr+ti99_4p_mapper_lookup[page]);
			break;
		}
	}
}


/*===========================================================================*/
/*
	myarc-ish memory extension support (foundation, and myarc clones).

	Up to 512kb of RAM.  Straightforward mapper, works with 32kb chunks.
*/

static int myarc_cru_r(running_machine *machine, int offset);
static void myarc_cru_w(running_machine *machine, int offset, int data);

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
static void ti99_myarcxram_init(running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	
	memory_install_read16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_myarcxramlow_r);
	memory_install_write16_handler(space, 0x2000, 0x3fff, 0, 0, ti99_myarcxramlow_w);
	memory_install_read16_handler(space, 0xa000, 0xffff, 0, 0, ti99_myarcxramhigh_r);
	memory_install_write16_handler(space, 0xa000, 0xffff, 0, 0, ti99_myarcxramhigh_w);

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
static int myarc_cru_r(running_machine *machine, int offset)
{
	/*if (offset == 0)*/	/* right??? */
	{
		return (myarc_cur_page_offset >> 14);
	}
}

/* write CRU bit:
	bit 1-2 (128kb) or 1-4 (512kb): write map offset */
static void myarc_cru_w(running_machine *machine, int offset, int data)
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
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);

	return xRAM_ptr[myarc_cur_page_offset + offset];
}

static WRITE16_HANDLER ( ti99_myarcxramlow_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);

	COMBINE_DATA(xRAM_ptr + myarc_cur_page_offset + offset);
}

/* high 24 kb: 0xa000-0xffff */
static READ16_HANDLER ( ti99_myarcxramhigh_r )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);

	return xRAM_ptr[myarc_cur_page_offset + offset+0x1000];
}

static WRITE16_HANDLER ( ti99_myarcxramhigh_w )
{
	cpu_adjust_icount(cputag_get_cpu(space->machine, "maincpu"),-4);

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
static int evpc_cru_r(running_machine *machine, int offset);
static void evpc_cru_w(running_machine *machine, int offset, int data);
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
static void ti99_evpc_reset(running_machine *machine)
{
	ti99_evpc_DSR = memory_region(machine, region_dsr) + offset_evpc_dsr;

	RAMEN = 0;
	evpc_dsr_page = 0;

	ti99_peb_set_card_handlers(0x1400, & evpc_handlers);
}

/*
	Read evpc CRU interface
*/
static int evpc_cru_r(running_machine *machine, int offset)
{
	return 0;	/* dip-switch value */
}

/*
	Write evpc CRU interface
*/
static void evpc_cru_w(running_machine *machine, int offset, int data)
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
	int read;
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

/* tms9901 setup */
const tms9901_interface tms9901reset_param_ti99_4x =
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

const tms9901_interface tms9901reset_param_ti99_8 =
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
