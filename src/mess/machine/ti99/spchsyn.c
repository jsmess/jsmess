/*
    TI-99 Speech synthesizer

    We emulate the Speech Synthesizer plugged onto a P-Box adapter. The original
    Speech Synthesizer device was provided as a box to be plugged into the
    right side of the console. In order to be used with Geneve and SGCPU, the
    speech synthesizer must be moved into the Peripheral Box.

    The Speech Synthesizer used for the TI was the TMS5200, aka TMC0285, a
    predecessor of the TMS5220 which was used in other commercial products.

    Note that this adapter also contains the speech roms.
*/
#include "emu.h"
#include "peribox.h"
#include "spchsyn.h"
#include "sound/tms5220.h"
#include "sound/wave.h"

#define TMS5220_ADDRESS_MASK 0x3FFFFUL	/* 18-bit mask for tms5220 address */
#define speech_region "speech_region"

typedef ti99_pebcard_config ti99_speech_config;

typedef struct _ti99_speech_state
{
	device_t				*vsp;
	UINT8					*speechrom_data;		/* pointer to speech ROM data */
	int 					load_pointer;			/* which 4-bit nibble will be affected by load address */
	int 					ROM_bits_count;				/* current bit position in ROM */
	UINT32					speechROMaddr;				/* 18 bit pointer in ROM */
	UINT32					speechROMlen;			/* length of data pointed by speechrom_data, from 0 to 2^18 */
	ti99_peb_connect		lines;

	int						select_mask;
	int						select_value;
} ti99_speech_state;

INLINE ti99_speech_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TISPEECH);

	return (ti99_speech_state *)downcast<legacy_device_base *>(device)->token();
}

/*****************************************************************************/

/*
    Read 'count' bits serially from speech ROM
*/
static int ti99_spchroms_read(device_t *vspdev, int count)
{
	device_t *device = vspdev->owner();
	ti99_speech_state *adapter = get_safe_token(device);

	int val;

	if (adapter->load_pointer)
	{	/* first read after load address is ignored */
		adapter->load_pointer = 0;
		count--;
	}

	if (adapter->speechROMaddr < adapter->speechROMlen)
	{
		if (count < adapter->ROM_bits_count)
		{
			adapter->ROM_bits_count -= count;
			val = (adapter->speechrom_data[adapter->speechROMaddr] >> adapter->ROM_bits_count) & (0xFF >> (8 - count));
		}
		else
		{
			val = ((int)adapter->speechrom_data[adapter->speechROMaddr]) << 8;

			adapter->speechROMaddr = (adapter->speechROMaddr + 1) & TMS5220_ADDRESS_MASK;

			if (adapter->speechROMaddr < adapter->speechROMlen)
				val |= adapter->speechrom_data[adapter->speechROMaddr];

			adapter->ROM_bits_count += 8 - count;

			val = (val >> adapter->ROM_bits_count) & (0xFF >> (8 - count));
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
	ti99_speech_state *adapter = get_safe_token(device);
	// tms5220 data sheet says that if we load only one 4-bit nibble, it won't work.
	// This code does not care about this.
	adapter->speechROMaddr = ((adapter->speechROMaddr & ~(0xf << adapter->load_pointer))
		| (((unsigned long) (data & 0xf)) << adapter->load_pointer) ) & TMS5220_ADDRESS_MASK;
	adapter->load_pointer += 4;
	adapter->ROM_bits_count = 8;
}

/*
    Perform a read and branch command
*/
static void ti99_spchroms_read_and_branch(device_t *vspdev)
{
	device_t *device = vspdev->owner();
	ti99_speech_state *adapter = get_safe_token(device);
	// tms5220 data sheet says that if more than one speech ROM (tms6100) is present,
	// there is a bus contention.  This code does not care about this. */
	if (adapter->speechROMaddr < adapter->speechROMlen-1)
		adapter->speechROMaddr = (adapter->speechROMaddr & 0x3c000UL)
			| (((((unsigned long) adapter->speechrom_data[adapter->speechROMaddr]) << 8)
			| adapter->speechrom_data[adapter->speechROMaddr+1]) & 0x3fffUL);
	else if (adapter->speechROMaddr == adapter->speechROMlen-1)
		adapter->speechROMaddr = (adapter->speechROMaddr & 0x3c000UL)
			| ((((unsigned long) adapter->speechrom_data[adapter->speechROMaddr]) << 8) & 0x3fffUL);
	else
		adapter->speechROMaddr = (adapter->speechROMaddr & 0x3c000UL);

	adapter->ROM_bits_count = 8;
}

/*****************************************************************************/

/*
    Memory read
*/
static READ8Z_DEVICE_HANDLER( speech_rz )
{
	ti99_speech_state *adapter = get_safe_token(device);

	if ((offset & adapter->select_mask)==adapter->select_value)
	{
		device_adjust_icount(device->machine().device("maincpu"),-(18+3));		/* this is just a minimum, it can be more */
		*value = tms5220_status_r(adapter->vsp, offset) & 0xff;
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( speech_w )
{
	ti99_speech_state *adapter = get_safe_token(device);

	if ((offset & adapter->select_mask)==(adapter->select_value | 0x0400))
	{
		device_adjust_icount(device->machine().device("maincpu"),-(54+3));		/* this is just an approx. minimum, it can be much more */

		/* RN: the stupid design of the tms5220 core means that ready is cleared */
		/* when there are 15 bytes in FIFO.  It should be 16.  Of course, if */
		/* it were the case, we would need to store the value on the bus, */
		/* which would be more complex. */
		if (!tms5220_readyq_r(adapter->vsp))
		{
			attotime time_to_ready = attotime::from_double(tms5220_time_to_ready(adapter->vsp));
			int cycles_to_ready = device->machine().device<cpu_device>("maincpu")->attotime_to_cycles(time_to_ready);
			logerror("time to ready: %f -> %d\n", time_to_ready.as_double(), (int) cycles_to_ready);

			device_adjust_icount(device->machine().device("maincpu"),-cycles_to_ready);
			device->machine().scheduler().timer_set(attotime::zero, FUNC(NULL));
		}
		tms5220_data_w(adapter->vsp, offset, data);
	}
}

/**************************************************************************/

static const ti99_peb_card speech_adapter_card =
{
	speech_rz, speech_w,			// memory access read/write
	NULL, NULL,						// CRU access (none here)
	NULL, NULL,						// SENILA/B access (none here)
	NULL, NULL						// 16 bit access (none here)
};

static DEVICE_START( ti99_speech )
{
	ti99_speech_state *adapter = get_safe_token(device);
	/* Resolve the callbacks to the PEB */
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();
	devcb_resolve_write_line(&adapter->lines.ready, &topeb->ready, device);
	adapter->vsp = device->subdevice("speechsyn");
}

static DEVICE_STOP( ti99_speech )
{
}

static DEVICE_RESET( ti99_speech )
{
	ti99_speech_state *adapter = get_safe_token(device);
	/* Register the adapter */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "SPEECH"))
	{
		int success = mount_card(peb, device, &speech_adapter_card, get_pebcard_config(device)->slot);
		if (!success)
		{
			logerror("speech: Could not mount the speech synthesizer\n");
			return;
		}

		astring *region = new astring();
		astring_assemble_3(region, device->tag(), ":", speech_region);

		adapter->speechrom_data = device->machine().region(astring_c(region))->base();
		adapter->speechROMlen = device->machine().region(astring_c(region))->bytes();
		adapter->speechROMaddr = 0;
		adapter->load_pointer = 0;
		adapter->ROM_bits_count = 0;

		adapter->select_mask = 0x7fc01;
		adapter->select_value = 0x79000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			adapter->select_mask = 0x1ffc01;
			adapter->select_value = 0x179000;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( speech_ready )
{
	ti99_speech_state *adapter = get_safe_token(device->owner());
	logerror("speech: READY called by VSP\n");
	devcb_call_write_line( &adapter->lines.ready, state );
}

/*
    TMS5220 speech synthesizer
    Note that in the real hardware, the predecessor TMC0285 was used.
*/
static const tms5220_interface ti99_4x_tms5200interface =
{
	DEVCB_NULL,					/* no IRQ callback */
	DEVCB_LINE(speech_ready),
	ti99_spchroms_read,				/* speech ROM read handler */
	ti99_spchroms_load_address,		/* speech ROM load address handler */
	ti99_spchroms_read_and_branch	/* speech ROM read and branch handler */
};

MACHINE_CONFIG_FRAGMENT( ti99_speech )
	MCFG_SOUND_ADD("speechsyn", TMC0285, 680000L)
	MCFG_SOUND_CONFIG(ti99_4x_tms5200interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( ti99_speech )
	ROM_REGION(0x8000, speech_region, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99_speech##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_ROM_REGION | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI-99 Speech Synthesizer"
#define DEVTEMPLATE_SHORTNAME           "ti99spsyn"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TISPEECH, ti99_speech );

