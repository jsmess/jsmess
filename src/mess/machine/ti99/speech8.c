/****************************************************************************

    TI-99/8 Speech synthesizer subsystem

    The TI-99/8 contains a speech synthesizer inside the console, so we cannot
    reuse the spchsyn implementation of the P-Box speech synthesizer.
    Accordingly, this is not a ti_expansion_card_device.

    Michael Zapf
    February 2012: Rewritten as class

*****************************************************************************/

#include "speech8.h"
#include "sound/wave.h"

#define TMS5220_ADDRESS_MASK 0x3FFFFUL	/* 18-bit mask for tms5220 address */

#define VERBOSE 1
#define LOG logerror

#define SPEECHSYN_TAG "speechsyn"
#define SPEECHROM_TAG "speechrom"

/****************************************************************************/

ti998_spsyn_device::ti998_spsyn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, TI99_SPEECH8, "TI-99/8 Speech synthesizer (onboard)", tag, owner, clock)
{
	m_shortname = "ti99_speech8";
}

/*
    Memory read
*/
READ8Z_MEMBER( ti998_spsyn_device::readz )
{
	if ((offset & m_select_mask)==m_select_value)
	{
		device_adjust_icount(machine().device("maincpu"),-(18+3));		/* this is just a minimum, it can be more */
		*value = tms5220_status_r(m_vsp, offset) & 0xff;
	}
}

/*
    Memory write
*/
WRITE8_MEMBER( ti998_spsyn_device::write )
{
	if ((offset & m_select_mask)==(m_select_value | 0x0400))
	{
		device_adjust_icount(machine().device("maincpu"),-(54+3));		/* this is just an approx. minimum, it can be much more */

		/* RN: the stupid design of the tms5220 core means that ready is cleared */
		/* when there are 15 bytes in FIFO.  It should be 16.  Of course, if */
		/* it were the case, we would need to store the value on the bus, */
		/* which would be more complex. */
		if (!tms5220_readyq_r(m_vsp))
		{
			attotime time_to_ready = attotime::from_double(tms5220_time_to_ready(m_vsp));
			int cycles_to_ready = machine().device<cpu_device>("maincpu")->attotime_to_cycles(time_to_ready);
			if (VERBOSE>8) LOG("time to ready: %f -> %d\n", time_to_ready.as_double(), (int) cycles_to_ready);

			device_adjust_icount(machine().device("maincpu"),-cycles_to_ready);
			machine().scheduler().timer_set(attotime::zero, FUNC_NULL);
		}
		tms5220_data_w(m_vsp, offset, data);
	}
}

/****************************************************************************
    Callbacks from TMS5220

    TODO: Convert to methods as soon as TMS5220 has been converted
*****************************************************************************/
/*
    Read 'count' bits serially from speech ROM
*/
static int ti998_spchroms_read(device_t *vspdev, int count)
{
	ti998_spsyn_device *adapter = static_cast<ti998_spsyn_device*>(vspdev->owner());

	int val;

	if (adapter->m_load_pointer)
	{	/* first read after load address is ignored */
		adapter->m_load_pointer = 0;
		count--;
	}

	if (adapter->m_sprom_address < adapter->m_sprom_length)
	{
		if (count < adapter->m_rombits_count)
		{
			adapter->m_rombits_count -= count;
			val = (adapter->m_speechrom[adapter->m_sprom_address] >> adapter->m_rombits_count) & (0xFF >> (8 - count));
		}
		else
		{
			val = ((int)adapter->m_speechrom[adapter->m_sprom_address]) << 8;

			adapter->m_sprom_address = (adapter->m_sprom_address + 1) & TMS5220_ADDRESS_MASK;

			if (adapter->m_sprom_address < adapter->m_sprom_length)
				val |= adapter->m_speechrom[adapter->m_sprom_address];

			adapter->m_rombits_count += 8 - count;

			val = (val >> adapter->m_rombits_count) & (0xFF >> (8 - count));
		}
	}
	else
		val = 0;

	return val;
}

/*
    Write an address nibble to speech ROM
*/
static void ti998_spchroms_load_address(device_t *vspdev, int data)
{
	ti998_spsyn_device *adapter = static_cast<ti998_spsyn_device*>(vspdev->owner());

	// tms5220 data sheet says that if we load only one 4-bit nibble, it won't work.
	// This code does not care about this.
	adapter->m_sprom_address = ((adapter->m_sprom_address & ~(0xf << adapter->m_load_pointer))
		| (((unsigned long) (data & 0xf)) << adapter->m_load_pointer) ) & TMS5220_ADDRESS_MASK;
	adapter->m_load_pointer += 4;
	adapter->m_rombits_count = 8;
}

/*
    Perform a read and branch command
*/
static void ti998_spchroms_read_and_branch(device_t *vspdev)
{
	ti998_spsyn_device *adapter = static_cast<ti998_spsyn_device*>(vspdev->owner());
	// tms5220 data sheet says that if more than one speech ROM (tms6100) is present,
	// there is a bus contention.  This code does not care about this. */
	if (adapter->m_sprom_address < adapter->m_sprom_length-1)
		adapter->m_sprom_address = (adapter->m_sprom_address & 0x3c000UL)
			| (((((unsigned long) adapter->m_speechrom[adapter->m_sprom_address]) << 8)
			| adapter->m_speechrom[adapter->m_sprom_address+1]) & 0x3fffUL);
	else if (adapter->m_sprom_address == adapter->m_sprom_length-1)
		adapter->m_sprom_address = (adapter->m_sprom_address & 0x3c000UL)
			| ((((unsigned long) adapter->m_speechrom[adapter->m_sprom_address]) << 8) & 0x3fffUL);
	else
		adapter->m_sprom_address = (adapter->m_sprom_address & 0x3c000UL);

	adapter->m_rombits_count = 8;
}

/*****************************************************************************/
/*
    Callback interface instance
*/
static const tms5220_interface ti99_8_tms5200interface =
{
	DEVCB_NULL,						// no IRQ callback
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, ti998_spsyn_device, speech8_ready),
	ti998_spchroms_read,				// speech ROM read handler
	ti998_spchroms_load_address,		// speech ROM load address handler
	ti998_spchroms_read_and_branch	// speech ROM read and branch handler
};

/**************************************************************************/

WRITE_LINE_MEMBER( ti998_spsyn_device::speech8_ready )
{
	//ti99_speech8_state *spsys = get_safe_token(device->owner());
	if (VERBOSE>4) LOG("speech8: READY called by VSP; no forwarding implemented yet\n");
}

void ti998_spsyn_device::device_start()
{
}

void ti998_spsyn_device::device_config_complete()
{
	m_vsp = subdevice(SPEECHSYN_TAG);
}

void ti998_spsyn_device::device_reset()
{
	m_speechrom = subregion(SPEECHROM_TAG)->base();
	m_sprom_length = subregion(SPEECHROM_TAG)->bytes();
	m_sprom_address = 0;
	m_load_pointer = 0;
	m_rombits_count = 0;

	m_select_mask = 0xfc01;
	m_select_value = 0x9000;
	if (VERBOSE>4) LOG("speech8: reset\n");
}

// Unlike the TI-99/4A, the 99/8 uses the TMS5220
MACHINE_CONFIG_FRAGMENT( ti998_speech )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEECHSYN_TAG, TMS5220, 680000L)
	MCFG_SOUND_CONFIG(ti99_8_tms5200interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( ti998_speech )
	ROM_REGION(0x8000, SPEECHROM_TAG, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, BAD_DUMP CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

machine_config_constructor ti998_spsyn_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ti998_speech );
}

const rom_entry *ti998_spsyn_device::device_rom_region() const
{
	return ROM_NAME( ti998_speech );
}
const device_type TI99_SPEECH8 = &device_creator<ti998_spsyn_device>;

