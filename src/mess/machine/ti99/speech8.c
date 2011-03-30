/*
    TI-99/8 Speech synthesizer subsystem

    The TI-99/8 contains a speech synthesizer inside the console, so we cannot
    reuse the spchsyn implementation of the P-Box speech synthesizer.

    Note that this subsystem also contains the speech roms.
*/
#include "emu.h"
#include "speech8.h"
#include "sound/tms5220.h"
#include "sound/wave.h"

#define TMS5220_ADDRESS_MASK 0x3FFFFUL	/* 18-bit mask for tms5220 address */
#define speech8_region "speech8_region"

typedef struct _ti99_speech8_state
{
	device_t			*vsp;
	UINT8					*speechrom_data;		/* pointer to speech ROM data */
	int 					load_pointer;			/* which 4-bit nibble will be affected by load address */
	int 					ROM_bits_count;				/* current bit position in ROM */
	UINT32					speechROMaddr;				/* 18 bit pointer in ROM */
	UINT32					speechROMlen;			/* length of data pointed by speechrom_data, from 0 to 2^18 */

} ti99_speech8_state;

INLINE ti99_speech8_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TISPEECH8);

	return (ti99_speech8_state *)downcast<legacy_device_base *>(device)->token();
}

/*****************************************************************************/

/*
    Read 'count' bits serially from speech ROM
*/
static int ti99_spchroms_read(device_t *vspdev, int count)
{
	device_t *device = vspdev->owner();
	ti99_speech8_state *spsys = get_safe_token(device);

	int val;

	if (spsys->load_pointer)
	{	/* first read after load address is ignored */
		spsys->load_pointer = 0;
		count--;
	}

	if (spsys->speechROMaddr < spsys->speechROMlen)
	{
		if (count < spsys->ROM_bits_count)
		{
			spsys->ROM_bits_count -= count;
			val = (spsys->speechrom_data[spsys->speechROMaddr] >> spsys->ROM_bits_count) & (0xFF >> (8 - count));
		}
		else
		{
			val = ((int)spsys->speechrom_data[spsys->speechROMaddr]) << 8;

			spsys->speechROMaddr = (spsys->speechROMaddr + 1) & TMS5220_ADDRESS_MASK;

			if (spsys->speechROMaddr < spsys->speechROMlen)
				val |= spsys->speechrom_data[spsys->speechROMaddr];

			spsys->ROM_bits_count += 8 - count;

			val = (val >> spsys->ROM_bits_count) & (0xFF >> (8 - count));
		}
	}
	else
		val = 0;

	return val;
}

/*
    Write an address nibble to speech ROM
*/
static void ti99_spchroms_load_address(device_t *vspdev, int data)
{
	device_t *device = vspdev->owner();
	ti99_speech8_state *spsys = get_safe_token(device);
	// tms5220 data sheet says that if we load only one 4-bit nibble, it won't work.
	// This code does not care about this.
	spsys->speechROMaddr = ((spsys->speechROMaddr & ~(0xf << spsys->load_pointer))
		| (((unsigned long) (data & 0xf)) << spsys->load_pointer) ) & TMS5220_ADDRESS_MASK;
	spsys->load_pointer += 4;
	spsys->ROM_bits_count = 8;
}

/*
    Perform a read and branch command
*/
static void ti99_spchroms_read_and_branch(device_t *vspdev)
{
	device_t *device = vspdev->owner();
	ti99_speech8_state *spsys = get_safe_token(device);
	// tms5220 data sheet says that if more than one speech ROM (tms6100) is present,
	// there is a bus contention.  This code does not care about this. */
	if (spsys->speechROMaddr < spsys->speechROMlen-1)
		spsys->speechROMaddr = (spsys->speechROMaddr & 0x3c000UL)
			| (((((unsigned long) spsys->speechrom_data[spsys->speechROMaddr]) << 8)
			| spsys->speechrom_data[spsys->speechROMaddr+1]) & 0x3fffUL);
	else if (spsys->speechROMaddr == spsys->speechROMlen-1)
		spsys->speechROMaddr = (spsys->speechROMaddr & 0x3c000UL)
			| ((((unsigned long) spsys->speechrom_data[spsys->speechROMaddr]) << 8) & 0x3fffUL);
	else
		spsys->speechROMaddr = (spsys->speechROMaddr & 0x3c000UL);

	spsys->ROM_bits_count = 8;
}

/*****************************************************************************/

/*
    Memory read
*/
READ8Z_DEVICE_HANDLER( ti998spch_rz )
{
	ti99_speech8_state *spsys = get_safe_token(device);

	if ((offset & 0xfc01)==0x9000)
	{
		device_adjust_icount(device->machine().device("maincpu"),-(18+3));		/* this is just a minimum, it can be more */
		*value = tms5220_status_r(spsys->vsp, offset) & 0xff;
	}
}

/*
    Memory write
*/
WRITE8_DEVICE_HANDLER( ti998spch_w )
{
	ti99_speech8_state *spsys = get_safe_token(device);

	if ((offset & 0xfc01)==0x9400)
	{
		device_adjust_icount(device->machine().device("maincpu"),-(54+3));		/* this is just an approx. minimum, it can be much more */

		/* RN: the stupid design of the tms5220 core means that ready is cleared */
		/* when there are 15 bytes in FIFO.  It should be 16.  Of course, if */
		/* it were the case, we would need to store the value on the bus, */
		/* which would be more complex. */
		if (!tms5220_readyq_r(spsys->vsp))
		{
			attotime time_to_ready = attotime::from_double(tms5220_time_to_ready(spsys->vsp));
			int cycles_to_ready = device->machine().device<cpu_device>("maincpu")->attotime_to_cycles(time_to_ready);
			logerror("time to ready: %f -> %d\n", time_to_ready.as_double(), (int) cycles_to_ready);

			device_adjust_icount(device->machine().device("maincpu"),-cycles_to_ready);
			device->machine().scheduler().timer_set(attotime::zero, FUNC(NULL));
		}
		tms5220_data_w(spsys->vsp, offset, data);
	}
}

/**************************************************************************/

static DEVICE_START( ti99_speech8 )
{
	ti99_speech8_state *spsys = get_safe_token(device);
	/* Resolve the callbacks to the PEB */
	spsys->vsp = device->subdevice("speechsyn");
}

static DEVICE_STOP( ti99_speech8 )
{
}

static DEVICE_RESET( ti99_speech8 )
{
	ti99_speech8_state *spsys = get_safe_token(device);

	astring *region = new astring();
	astring_assemble_3(region, device->tag(), ":", speech8_region);

	spsys->speechrom_data = device->machine().region(astring_c(region))->base();
	spsys->speechROMlen = device->machine().region(astring_c(region))->bytes();
	spsys->speechROMaddr = 0;
	spsys->load_pointer = 0;
	spsys->ROM_bits_count = 0;
}

static WRITE_LINE_DEVICE_HANDLER( speech8_ready )
{
	//ti99_speech8_state *spsys = get_safe_token(device->owner());
	logerror("speech8: READY called by VSP\n");
}

/*
    TMS5220 speech synthesizer
    Note that in the real hardware, the predecessor TMC0285 was used.
*/
static const tms5220_interface ti99_4x_tms5200interface =
{
	DEVCB_NULL,					/* no IRQ callback */
	DEVCB_LINE(speech8_ready),
	ti99_spchroms_read,				/* speech ROM read handler */
	ti99_spchroms_load_address,		/* speech ROM load address handler */
	ti99_spchroms_read_and_branch	/* speech ROM read and branch handler */
};

MACHINE_CONFIG_FRAGMENT( ti99_speech8 )
	MCFG_SOUND_ADD("speechsyn", TMS5220, 680000L)
	MCFG_SOUND_CONFIG(ti99_4x_tms5200interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( ti99_speech8 )
	ROM_REGION(0x8000, speech8_region, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, BAD_DUMP CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_speech8##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_ROM_REGION | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99/8 Speech Synthesizer"
#define DEVTEMPLATE_SHORTNAME           "ti998sp"
#define DEVTEMPLATE_FAMILY              "Internal subsystem"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TISPEECH8, ti99_speech8 );

