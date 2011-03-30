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

***************************************************************************/

#include "emu.h"
#include "grom.h"
#include "datamux.h"

/* ti99_grom_state: GROM state */
struct _ti99grom_state
{
	/* Address pointer. */
	// This value is always expected to be in the range 0x0000 - 0xffff, even
	// when this GROM is not addressed.
	UINT16 address;

	/* GROM data buffer. */
	UINT8 buffer;

	/* Internal flip-flop. Used when retrieving the address counter. */
	UINT8 raddr_LSB;

	/* Internal flip-flops. Used when writing the address counter.*/
	UINT8 waddr_LSB;

	/* ID of this GROM. */
	UINT8 ident;

	/* Pointer to the memory region contained in this GROM. */
	UINT8 *memptr;

	/* GROM size. May be 0x1800 or 0x2000. */
	// If the GROM has only 6 KiB, the remaining 2 KiB are filled with a
	// specific byte pattern which is created by a logical OR of lower
	// regions
	int extended;

	/* Determines whether this is a GRAM. */
	int writable;

	/* Determines whether there is a rollover at the end of the address space. */
	int rollover;

	/* Ready callback. This line is usually connected to the READY pin of the CPU. */
	devcb_resolved_write_line gromready;
};
typedef struct _ti99grom_state ti99grom_state;

INLINE ti99grom_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == GROM);

	return (ti99grom_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const ti99grom_config *get_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == GROM);

	return (const ti99grom_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/*
    Indicates whether this chip will react on the next read/write data
    access. We do not have a tri-state handling on the read handlers, so this
    serves to avoid the read access.
*/
static int is_selected(device_t *chip)
{
	ti99grom_state *grom = get_safe_token(chip);
	return (((grom->address >> 13)&0x07)==grom->ident);
}

/*
    Reading from the chip. Represents an access with M=1, GS*=0. The MO bit is
    defined by the offset (0 or 1). This is the enhanced read function with
    Z state.
*/
READ8Z_DEVICE_HANDLER( ti99grom_rz )
{
	ti99grom_state *grom = get_safe_token(device);

	if (offset & 2)
	{
		if (((grom->address >> 13)&0x07)!=grom->ident)
			return;

		/* When reading, reset the hi/lo flag byte for writing. */
		/* TODO: Verify this with a real machine. */
		grom->waddr_LSB = FALSE;

		/* Address reading is done in two steps; first, the high byte */
		/* is transferred, then the low byte. */
		if (grom->raddr_LSB)
		{
			/* second pass */
			*value = grom->address & 0x00ff;
			grom->raddr_LSB = FALSE;
		}
		else
		{
			/* first pass */
			*value = (grom->address & 0xff00)>>8;
			grom->raddr_LSB = TRUE;
		}
	}
	else
	{
		if (((grom->address >> 13)&0x07)==grom->ident)
		{
			/* GROMs are buffered. Data is retrieved from a buffer, */
			/* while the buffer is replaced with the next cell content. */
			*value = grom->buffer;

			/* Get next value, put it in buffer. Note that the GROM */
			/* wraps at 8K boundaries. */
			UINT16 addr = grom->address-(grom->ident<<13);

			if (!grom->extended && ((grom->address&0x1fff)>=0x1800))
				grom->buffer = grom->memptr[addr-0x1000] | grom->memptr[addr-0x0800];
			else
				grom->buffer = grom->memptr[addr];
		}
		/* Note that all GROMs update their address counter. */
		if (grom->rollover)
			grom->address = ((grom->address + 1) & 0x1FFF) | (grom->address & 0xE000);
		else
			grom->address = (grom->address + 1)&0xFFFF;

		/* Reset the read and write address flipflops. */
		grom->raddr_LSB = grom->waddr_LSB = FALSE;
	}
}

/*
    Reading from the chip. Represents an access with M=1, GS*=0. The MO bit is
    defined by the offset (0 or 1).
*/
#ifdef UNUSED_FUNCTION
READ8_DEVICE_HANDLER( ti99grom_r )
{
	UINT8 reply = 0;
	ti99grom_rz(device, offset, &reply);
	return reply;
}
#endif


/*
    Writing to the chip. Represents an access with M=0, GS*=0. The MO bit is
    defined by the offset (0 or 1).
*/
WRITE8_DEVICE_HANDLER( ti99grom_w )
{
	ti99grom_state *grom = get_safe_token(device);
	if (offset & 2)
	{
		/* write GROM address */
		/* see comments above */
		grom->raddr_LSB = FALSE;

		/* Implements the internal flipflop. */
		/* The Editor/Assembler manuals says that the current address */
		/* plus one is returned. This effect is properly emulated */
		/* by using a read-ahead buffer. */
		if (grom->waddr_LSB)
		{
			/* Accept low byte (2nd write) */
			grom->address = (grom->address & 0xFF00) | data;
			/* Setting the address causes a new prefetch */
			if (is_selected(device))
			{
				grom->buffer = grom->memptr[grom->address-(grom->ident<<13)];
			}
			grom->waddr_LSB = FALSE;
		}
		else
		{
			/* Accept high byte (1st write). Do not advance the address conter. */
			grom->address = (data << 8) | (grom->address & 0xFF);
			grom->waddr_LSB = TRUE;
			return;
		}
	}
	else
	{
		/* write GRAM data */
		if ((((grom->address >> 13)&0x07)==grom->ident) && grom->writable)
		{
			UINT16 write_addr;
			// We need to rewind by 1 because the read address has already advanced.
			// However, do not change the address counter!
			if (grom->rollover)
				write_addr = ((grom->address + 0x1fff) & 0x1FFF) | (grom->address & 0xE000);
			else
				write_addr = (grom->address - 1) & 0xFFFF;

			// UINT16 addr = grom->address-(grom->ident<<13);
			if (grom->extended || ((grom->address&0x1fff)<0x1800))
				grom->memptr[write_addr-(grom->ident<<13)] = data;
		}
		grom->raddr_LSB = grom->waddr_LSB = FALSE;
	}

	if (grom->rollover)
		grom->address = ((grom->address + 1) & 0x1FFF) | (grom->address & 0xE000);
	else
		grom->address = (grom->address + 1) & 0xFFFF;
}

/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

static DEVICE_START( ti99grom )
{
	ti99grom_state *grom = get_safe_token(device);
	const ti99grom_config* gromconf = (const ti99grom_config*)get_config(device);

	grom->writable = gromconf->writeable;
	grom->ident = gromconf->ident;

	grom->extended = (gromconf->size==0x1800)? TRUE : FALSE;

	grom->rollover = gromconf->rollover;

	devcb_write_line ready = DEVCB_LINE(gromconf->ready);
	devcb_resolve_write_line(&grom->gromready, &ready, device);
	// Test
	// devcb_call_write_line(&grom->gromready, ASSERT_LINE);
}

static DEVICE_STOP( ti99grom )
{
}

static DEVICE_RESET( ti99grom )
{
	const ti99grom_config* gromconf = (const ti99grom_config*)get_config(device);
	ti99grom_state *grom = get_safe_token(device);
	grom->address = 0;
	grom->raddr_LSB = FALSE;
	grom->waddr_LSB = FALSE;
	grom->buffer = 0;

	/* Try to get the pointer to the memory contents from the parent */
	if (gromconf->region==NULL)
		grom->memptr = (*gromconf->get_memory)(device->owner());
	else
		grom->memptr = device->machine().region(gromconf->region)->base();

	//  TODO: Check whether this may be 0 for console GROMs.
	//  assert (grom->memptr!=NULL);

	grom->memptr += gromconf->offset;

	// TODO: If the GROM is a subdevice of another device, and a region
	// is specified, we need to first determine the device tag and concatenate
	// the GROM region behind this device tag; otherwise the region will not be
	// found. May be done with an astring.
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti99grom##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG
#define DEVTEMPLATE_NAME                "tmc0430"
#define DEVTEMPLATE_FAMILY              "TTL"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( GROM, ti99grom );
