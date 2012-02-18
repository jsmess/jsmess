/***************************************************************************

    TI-99/4(A) databus multiplexer circuit

    The DMUX is used to convert the 16-bit databus of the TMS9900 into
    an 8-bit databus. The processor writes a 16 bit word which is split
    by this circuit into two bytes that are sent serially over the 8-bit bus.
    In the opposite direction, one 16-bit read request from the CPU is
    translated into two 8-bit read requests (odd address / even address) from
    this datamux. Its 8-bit latch (LS373) holds the first byte, while the
    datamux puts the CPU on hold, gets the second byte, and routes that second
    byte to the D0-D7 lines, while the latch now puts the first byte on D8-D15.
    Since we get two memory accesses each time, there are twice as many
    wait states than for a direct 16-bit access (order LSB, MSB).

    In addition, since the TMS 9900 also supports byte operations, every write
    operation is automatically preceded by a read operation, so this adds even
    more delays.

    Within the TI-99/4(A) console, only the internal ROM and the small internal
    RAM ("scratch pad RAM") are directly connected to the 16-bit bus. All other
    devices (video, audio, speech, GROM, and the complete P-Box system are
    connected to the datamux.

    The TMS9995 which is used in the Geneve has an internal multiplex, and
    the byte order is reversed: MSB, LSB

    ROM = 4K * 16 bit (8 KiB) system ROM (kind of BIOS, plus the GPL interpreter)
    RAM = 128 * 16 bit (256 byte) system RAM ("scratch pad")

    Many users (me too) used to solder a 16K * 16 bit (32 KiB) SRAM circuit into
    the console, before the datamux, decoded to 0x2000-0x3fff and 0xa000-0xffff.
    (This expansion was also called 0-waitstate, since it could be accessed
    with the full databus width, and the datamux did not create waitstates.)

    +---+                                                   +-------+
    |   |===##========##== D0-D7 ==========##===============|TMS9918| Video
    |   |   ||        ||                   ||               +-------+
    | T |   +-----+  +-----+      LS245  +----+
    | M |   | ROM |  | RAM |             +----+
    | S |   +-----+  +-----+               || |                     :
    |   |---||-||-----||-||----------------||-|---------------------:
    | 9 |   ||        ||    A0 - A14       || |                A0   : Sound
    | 9 |---||--------||-------------------||-|----------+    -A15  : GROM
    | 0 |   ||        ||      LS373  +-+   || | +----A15-+----------: Cartridges
    | 0 |   ||        ||   ##========|<|===## | |                   : Speech
    |   |   ||        ||   ||  +-+   +-+   || | |                   : Expansion
    |   |===## D8-D15 ##===##==|>|=====|===##=|=|=========== D0-D7 =: cards
    +---+                      +-+     |      | |                   :
      ^                     LS244|     |      | |
      |                          |     +--+---+-++
      |                          +--------| DMUX |
      +--- READY -------------------------+------+

         Databus width
        :------------- 16 bit ---------------|---------- 8 bit -----:

    A0=MSB; A15=LSB
    D0=MSB; D15=LSB

    We integrate the 16 bit memory expansion in this datamux component
    (pretending that the memory expansion was soldered on top of the datamux)

    January 2012: Rewritten as class

***************************************************************************/

#include "emu.h"
#include "datamux.h"

/*
    Constructor
*/
ti99_datamux_device::ti99_datamux_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: device_t(mconfig, DATAMUX, "Databus multiplexer", tag, owner, clock)
{
}

#define LOG logerror

/***************************************************************************
    DEVICE ACCESSOR FUNCTIONS
***************************************************************************/

/*
    Read access. We are using two loops because the delay between both
    accesses must not occur within the loop. So we have one access on the bus,
    a delay, and then the second access (each one with possibly many attached
    devices)
*/
READ16_MEMBER( ti99_datamux_device::read )
{
	UINT8 hbyte = 0;
	UINT16 addr = (offset << 1);

	// Looks ugly, but this is close to the real thing. If the 16bit
	// memory expansion is installed in the console, and the access hits its
	// space, just respond to the memory access and don't bother the
	// datamux in any way. In particular, do not make the datamux insert wait
	// states.
	if (m_use32k)
	{
		UINT16 base = 0;
		if ((addr & 0xe000)==0x2000) base = 0x1000;
		if (((addr & 0xe000)==0xa000) || ((addr & 0xc000)==0xc000)) base = 0x4000;

		if (base != 0)
		{
			UINT16 reply = m_ram16b[offset-base];
			// Store the prefetch
			m_lowbyte = reply & 0x00ff;
			m_highbyte = (reply & 0xff00)>>8;
			return reply;
		}
	}

	attached_device *dev = m_devices.first();

	// Reading the odd address first (addr+1)
	while (dev != NULL)
	{
		if (((addr+1) & dev->m_config->address_mask)==dev->m_config->select)
		{
			// Cast to the bus8z_device (see ti99defs.h)
			bus8z_device *devz = static_cast<bus8z_device *>(dev->m_device);
			devz->readz(*m_space, addr+1, &m_latch);
			m_lowbyte = m_latch;
		}
		// hope we don't have two devices answering...
		// consider something like a logical OR and maybe some artificial smoke
		dev = dev->m_next;
	}

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);
	dev = m_devices.first();

	// Reading the even address now (addr)
	while (dev != NULL)
	{
		if (dev->m_config->write_select != 0xffff) // write-only
		{
			if ((addr & dev->m_config->address_mask)==dev->m_config->select)
			{
				bus8z_device *devz = static_cast<bus8z_device *>(dev->m_device);
				devz->readz(*m_space, addr, &hbyte);
				m_highbyte = hbyte;
			}
		}
		dev = dev->m_next;
	}

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);

	// use the latch and the currently read byte and put it on the 16bit bus
	return (hbyte<<8) | m_latch ;
}

/*
    Write access.
*/
WRITE16_MEMBER( ti99_datamux_device::write )
{
	UINT16 addr = (offset << 1);

	// The handling of byte writing is done in MESS using mem_mask.
	// If the mem_mask is ff00, the CPU is about to write the high byte. This
	// is not the true story, since the real CPU never writes half a word.
	// In reality, the TMS9900 performs a read-before-write, stores the word
	// internally, and replaces the high or low byte when it is about to write
	// a byte to an even or odd address, respectively. Then it writes the
	// complete new word.
	// Example: Memory locations 0x6000 contains the word 0x1234 (BE). If we
	// write a 0x56 to 0x6000, the CPU first reads 0x6000, stores the 0x1234,
	// replaces the first byte with 0x56, and then writes 0x5634 to 0x6000.

	// Until the TMS9900 emulation is corrected in this way, we are using
	// the DMUX in place of the internal buffer.

//  LOG("write addr=%04x memmask=%04x value=%04x\n", addr, mem_mask, data);
	if (mem_mask == 0xff00)
		data = data | m_lowbyte;

	if (mem_mask == 0x00ff)
		data = data | (m_highbyte << 8);

	// Handle the internal 32K expansion
	if (m_use32k)
	{
		if (addr>=0x2000 && addr<0x4000)
		{
			m_ram16b[offset-0x1000] = data; // index 0000 - 0fff
			return;
		}
		if (addr>=0xa000)
		{
			m_ram16b[offset-0x4000] = data; // index 1000 - 4fff
			return;
		}
	}

	attached_device *dev = m_devices.first();
	while (dev != NULL)
	{
		if (((addr+1) & dev->m_config->address_mask)==(dev->m_config->select | dev->m_config->write_select))
		{
			bus8z_device *devz = static_cast<bus8z_device *>(dev->m_device);
			devz->write(*m_space, addr+1, data & 0xff);
		}
		dev = dev->m_next;
	}

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);
	dev = m_devices.first();

	while (dev != NULL)
	{
		if ((addr & dev->m_config->address_mask)==(dev->m_config->select | dev->m_config->write_select))
		{
			// write the byte from the upper 8 lines
			bus8z_device *devz = static_cast<bus8z_device *>(dev->m_device);
			devz->write(*m_space, addr, (data>>8) & 0xff);
		}
		dev = dev->m_next;
	}

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);
}

/* CRU space is not involved in the dmux. */

/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

void ti99_datamux_device::device_start(void)
{
	m_ram16b = NULL;
}

void ti99_datamux_device::device_stop(void)
{
	if (m_ram16b) free(m_ram16b);
}

void ti99_datamux_device::device_reset(void)
{
	const dmux_device_list_entry *list = reinterpret_cast<const dmux_device_list_entry *>(static_config());

	m_cpu = machine().device("maincpu");
	m_space = m_cpu->memory().space(AS_PROGRAM);

	m_devices.reset(); // clear the list
	m_use32k = (input_port_read(machine(), "RAM")==1)? TRUE : FALSE;

	// better use a region?
	if (m_ram16b==NULL) m_ram16b = (UINT16*)malloc(32768);

	// Now building the list of active devices at this databus multiplex.
	// We allow for turning off devices according to configuration switch settings.
	// In particular, the HSGPL card cannot function unless the console GROMs are
	// removed.
	if ( list != NULL )
	{
		bool done = false;
		for (int i=0; !done; i++)
		{
			if (list[i].name == NULL)
			{
				done = true;
			}
			else
			{
				UINT32 set = 0;
				bool active_device = true;
				if (list[i].setting!=NULL)
				{
					set = input_port_read(machine(), list[i].setting);
					active_device = ((set & list[i].set)==list[i].set) && ((set & list[i].unset)==0);
				}
				if (active_device)
				{
					device_t *dev = machine().device(list[i].name);
					if (dev != NULL)
					{
						attached_device *ad = new attached_device(dev, list[i]);
						m_devices.append(*ad);
						LOG("ti99_datamux: Device %s mounted at index %d.\n", list[i].name, i);
					}
					else
					{
						LOG("ti99_datamux: Device %s not found.\n", list[i].name);
					}
				}
				else
				{
					LOG("ti99_datamux: Device %s not mounted due to configuration setting %s.\n", list[i].name, list[i].setting);
				}
			}
		}
	}
	LOG("Datamux device count = %d\n", m_devices.count());
}

const device_type DATAMUX = &device_creator<ti99_datamux_device>;
