/***************************************************************************

    GROM emulation (aka TMC0430)

    This is a preliminary implementation since it does not yet contain
    internal timing; instead, it directly accesses the processor to
    simulate wait states
    When this will have become available, the file will be moved to the
    general circuit collection (emu/machine)

          +----+--+----+
      AD7 |1    G    16| Vss
      AD6 |2    R    15| GR
      AD5 |3    O    14| Vdd
      AD4 |4    M    13| GRC
      AD3 |5         12| M
      AD2 |6         11| MO
      AD1 |7         10| GS*
      AD0 |8          9| Vcc
          +------------+

    GR  = GROM Ready. Should be connected to processor's READY/HOLD*.
    GRC = GROM clock. Typically in the range of 400-500 kHz.
    M   = Direction. 1=read, 0=write
    MO  = Mode. 1=address counter access, 0=data access
    GS* = GROM select. 0=select, 1=deselect


    GROMs are slow ROM devices, which are
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

GROM ports:

    When accessing the GROMs registers, 8 address bits (cpu_addr & 0x03FC) may
    be used as a port number, which permits the use of up to 256 independent
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

    Every GROM has an internal ID which represents the high-order three
    address bits. The address counter can be set to any value from 0
    to 0xffff; the GROM will only react if selected and if the current
    address counter's high-order bits match the ID of the chip.
    Example: When the ID is 6, the GROM will react when the address
    counter contains a value from 0xc000 to 0xdfff.

    Writable GROMs are called GRAMs. Although the TI-99 systems reserve a
    port in the memory space, no one has ever seen a GRAM circuit in the wild.
    However, third-party products like HSGPL or GRAM Kracker simulate GRAMs
    using conventional RAM with some addressing circuitry, usually in a custom
    chip.

    CHECK: Reading the address increases the counter only once. The first access
    returns the MSB, the second (and all following accesses) return the LSB.

    Michael Zapf, August 2010
    January 2012: rewritten as class

***************************************************************************/

#include "emu.h"
#include "grom.h"

#define LOG logerror
#define VERBOSE 1

/*
    Constructor.
*/
ti99_grom_device::ti99_grom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, GROM, "TI-99 GROM device", tag, owner, clock)
{
}

/*
    Reading from the chip. Represents an access with M=1, GS*=0. The MO bit is
    defined by the offset (0 or 1). This is the enhanced read function with
    Z state.
*/
READ8Z_MEMBER( ti99_grom_device::readz )
{
	if (offset & 2)
	{
		// GROMs generally answer the address read request
		// (important if GROM simulators do not serve the request but rely on
		// the console GROMs) so we don't check the ident

		/* When reading, reset the hi/lo flag byte for writing. */
		/* TODO: Verify this with a real machine. */
		m_waddr_LSB = false;

		/* Address reading is done in two steps; first, the high byte */
		/* is transferred, then the low byte. */
		if (m_raddr_LSB)
		{
			/* second pass */
			*value = m_address & 0x00ff;
			m_raddr_LSB = false;
		}
		else
		{
			/* first pass */
			*value = (m_address & 0xff00)>>8;
			m_raddr_LSB = true;
		}
	}
	else
	{
		if (((m_address >> 13)&0x07)==m_ident)
		{
			// GROMs are buffered. Data is retrieved from a buffer,
			// while the buffer is replaced with the next cell content.
			*value = m_buffer;

			// Get next value, put it in buffer. Note that the GROM
			// wraps at 8K boundaries.
			UINT16 addr = m_address-(m_ident<<13);

			if (!m_extended && ((m_address&0x1fff)>=0x1800))
				m_buffer = m_memptr[addr-0x1000] | m_memptr[addr-0x0800];
			else
				m_buffer = m_memptr[addr];
		}
		// Note that all GROMs update their address counter.
		if (m_rollover)
			m_address = ((m_address + 1) & 0x1FFF) | (m_address & 0xE000);
		else
			m_address = (m_address + 1)&0xFFFF;

		// Reset the read and write address flipflops.
		m_raddr_LSB = m_waddr_LSB = false;
	}
}

/*
    Writing to the chip. Represents an access with M=0, GS*=0. The MO bit is
    defined by the offset (0 or 1).
*/
WRITE8_MEMBER( ti99_grom_device::write )
{
	if (offset & 2)
	{
		/* write GROM address */
		/* see comments above */
		m_raddr_LSB = false;

		/* Implements the internal flipflop. */
		/* The Editor/Assembler manuals says that the current address */
		/* plus one is returned. This effect is properly emulated */
		/* by using a read-ahead buffer. */
		if (m_waddr_LSB)
		{
			/* Accept low byte (2nd write) */
			m_address = (m_address & 0xFF00) | data;
			/* Setting the address causes a new prefetch */
			if (is_selected())
			{
				m_buffer = m_memptr[m_address-(m_ident<<13)];
			}
			m_waddr_LSB = false;
		}
		else
		{
			/* Accept high byte (1st write). Do not advance the address conter. */
			m_address = (data << 8) | (m_address & 0xFF);
			m_waddr_LSB = true;
			return;
		}
	}
	else
	{
		/* write GRAM data */
		if ((((m_address >> 13)&0x07)==m_ident) && m_writable)
		{
			UINT16 write_addr;
			// We need to rewind by 1 because the read address has already advanced.
			// However, do not change the address counter!
			if (m_rollover)
				write_addr = ((m_address + 0x1fff) & 0x1FFF) | (m_address & 0xE000);
			else
				write_addr = (m_address - 1) & 0xFFFF;

			// UINT16 addr = m_address-(m_ident<<13);
			if (m_extended || ((m_address&0x1fff)<0x1800))
				m_memptr[write_addr-(m_ident<<13)] = data;
		}
		m_raddr_LSB = m_waddr_LSB = false;
	}

	if (m_rollover)
		m_address = ((m_address + 1) & 0x1FFF) | (m_address & 0xE000);
	else
		m_address = (m_address + 1) & 0xFFFF;
}

/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

void ti99_grom_device::device_start(void)
{
	m_gromready.resolve(m_ready, *this);
	// Test
	// m_gromready(ASSERT_LINE);
}

void ti99_grom_device::device_config_complete(void)
{
	const ti99grom_config *conf = reinterpret_cast<const ti99grom_config *>(static_config());

	if ( conf != NULL )
	{
		*static_cast<ti99grom_config *>(this) = *conf;
	}
}

void ti99_grom_device::device_reset(void)
{
	m_address = 0;
	m_raddr_LSB = false;
	m_waddr_LSB = false;
	m_buffer = 0;

	m_memptr = owner()->subregion(m_regionname)->base();
	assert (m_memptr!=NULL);

	m_memptr += m_offset_reg;
}

const device_type GROM = &device_creator<ti99_grom_device>;
