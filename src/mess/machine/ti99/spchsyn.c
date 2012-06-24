/****************************************************************************

    TI-99 Speech synthesizer

    We emulate the Speech Synthesizer plugged onto a P-Box adapter. The original
    Speech Synthesizer device was provided as a box to be plugged into the
    right side of the console. In order to be used with Geneve and SGCPU, the
    speech synthesizer must be moved into the Peripheral Box.

    The Speech Synthesizer used for the TI was the TMS5200, aka TMC0285, a
    predecessor of the TMS5220 which was used in other commercial products.

    Note that this adapter also contains the speech roms.

    Michael Zapf

    February 2012: Rewritten as class

*****************************************************************************/

#include "spchsyn.h"
#include "sound/wave.h"

#define TMS5220_ADDRESS_MASK 0x3FFFFUL	/* 18-bit mask for tms5220 address */

#define VERBOSE 1
#define LOG logerror

#define SPEECHROM_TAG "speechrom"

#define REAL_TIMING 0

/****************************************************************************/

ti_speech_synthesizer_device::ti_speech_synthesizer_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: ti_expansion_card_device(mconfig, TI99_SPEECH, "TI-99 Speech synthesizer (on adapter card)", tag, owner, clock)
{
	m_shortname = "ti99_speech";
}

/*
    Comments on real timing in the TMS5200 emulation

    Real timing means that the synthesizer clears the /READY line (puts high)
    whenever a read or write access is in progress. This is done by setting
    /RS and /WS lines, according to the read or write operation. The /READY line
    is asserted again after some time. Real timing is used once the tms5220_wsq_w
    and tms5220_rsq_w are called.

    Within the TI systems, the /RS and /WS lines are controlled directly by the
    address bus. There is currently no way to insert wait states between
    the address setting and the data bus sampling, since this is an atomic
    operation in the emulator (read_byte). It would be necessary to somehow
    announce the pending read before it actually happens so that devices
    like this VSP may insert wait states before the read.

    The TMS5220 implementation assumes that wait states are respected and
    therefore delivers bad values when queried too early. It uses a latch that
    gets the new value after some time has expired.

    To fix this we have to modify the RS/WS methods in TMS5200 to immediately
    set the status (after updating the sound status, or the status will be
    outdated too early).

    Also note that the /RS and /WS lines must be cleared (put to high) when the
    read is done. Again, this is not possible with the current implementation.
    So we do this in the ready callback.

    On the bottom line we will stay with the not-REAL_TIMING for now and wait
    for the core to allow for split read accesses.
*/


/*
    Memory read
*/
#if REAL_TIMING
// ======  This is the version with real timing =======
READ8Z_MEMBER( ti_speech_synthesizer_device::readz )
{
	if ((offset & m_select_mask)==m_select_value)
	{
		tms5220_wsq_w(m_vsp, TRUE);
		tms5220_rsq_w(m_vsp, FALSE);
		*value = tms5220_status_r(m_vsp, offset) & 0xff;
		if (VERBOSE>4) LOG("spchsyn: read value = %02x\n", *value);
	}
}

/*
    Memory write
*/
WRITE8_MEMBER( ti_speech_synthesizer_device::write )
{
	if ((offset & m_select_mask)==(m_select_value | 0x0400))
	{
		tms5220_rsq_w(m_vsp, TRUE);
		tms5220_wsq_w(m_vsp, FALSE);
		if (VERBOSE>4) LOG("spchsyn: write value = %02x\n", data);
		tms5220_data_w(m_vsp, offset, data);
	}
}

#else
// ======  This is the version without real timing =======

READ8Z_MEMBER( ti_speech_synthesizer_device::readz )
{
	if ((offset & m_select_mask)==m_select_value)
	{
		device_adjust_icount(machine().device("maincpu"),-(18+3));		/* this is just a minimum, it can be more */
		*value = tms5220_status_r(m_vsp, offset) & 0xff;
		if (VERBOSE>4) LOG("spchsyn: read value = %02x\n", *value);
	}
}

/*
    Memory write
*/
WRITE8_MEMBER( ti_speech_synthesizer_device::write )
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
		if (VERBOSE>4) LOG("spchsyn: write value = %02x\n", data);
		tms5220_data_w(m_vsp, offset, data);
	}
}
#endif

/****************************************************************************
    Callbacks from TMS5220

    TODO: Convert to methods as soon as TMS5220 has been converted
*****************************************************************************/
/*
    Read 'count' bits serially from speech ROM
*/
static int ti99_spchroms_read(device_t *vspdev, int count)
{
	ti_speech_synthesizer_device *adapter = static_cast<ti_speech_synthesizer_device*>(vspdev->owner());

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
static void ti99_spchroms_load_address(device_t *vspdev, int data)
{
	ti_speech_synthesizer_device *adapter = static_cast<ti_speech_synthesizer_device*>(vspdev->owner());

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
static void ti99_spchroms_read_and_branch(device_t *vspdev)
{
	ti_speech_synthesizer_device *adapter = static_cast<ti_speech_synthesizer_device*>(vspdev->owner());
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

/****************************************************************************/

/*
    Callback interface instance
*/
static const tms5220_interface ti99_4x_tms5200interface =
{
	DEVCB_NULL,						// no IRQ callback
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, ti_speech_synthesizer_device, speech_ready),
	ti99_spchroms_read,				// speech ROM read handler
	ti99_spchroms_load_address,		// speech ROM load address handler
	ti99_spchroms_read_and_branch	// speech ROM read and branch handler
};

/****************************************************************************/

WRITE_LINE_MEMBER( ti_speech_synthesizer_device::speech_ready )
{
	// The TMS5200 implementation uses TRUE/FALSE, not ASSERT/CLEAR semantics
	// and we have to adapt a /READY to a READY line.
	// The real synthesizer board uses a transistor for that purpose.
	m_slot->set_ready((state==0)? ASSERT_LINE : CLEAR_LINE);
	if (VERBOSE>5) LOG("spchsyn: READY = %d\n", (state==0));

#if REAL_TIMING
	// Need to do that here (see explanations above)
	if (state==0)
	{
		tms5220_rsq_w(m_vsp, TRUE);
		tms5220_wsq_w(m_vsp, TRUE);
	}
#endif
}

void ti_speech_synthesizer_device::device_start()
{
}

void ti_speech_synthesizer_device::device_config_complete()
{
	m_vsp = subdevice("speechsyn");
}

void ti_speech_synthesizer_device::device_reset()
{
	m_speechrom = memregion(SPEECHROM_TAG)->base();
	m_sprom_length = memregion(SPEECHROM_TAG)->bytes();
	m_sprom_address = 0;
	m_load_pointer = 0;
	m_rombits_count = 0;

	if (m_genmod)
	{
		m_select_mask = 0x1ffc01;
		m_select_value = 0x179000;
	}
	else
	{
		m_select_mask = 0x7fc01;
		m_select_value = 0x79000;
	}
}

MACHINE_CONFIG_FRAGMENT( ti99_speech )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speechsyn", TMC0285, 640000L)
	MCFG_SOUND_CONFIG(ti99_4x_tms5200interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( ti99_speech )
	ROM_REGION(0x8000, SPEECHROM_TAG, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

machine_config_constructor ti_speech_synthesizer_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ti99_speech );
}

const rom_entry *ti_speech_synthesizer_device::device_rom_region() const
{
	return ROM_NAME( ti99_speech );
}
const device_type TI99_SPEECH = &device_creator<ti_speech_synthesizer_device>;
