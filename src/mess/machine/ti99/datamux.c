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
***************************************************************************/

#include "emu.h"
#include "peribox.h"
#include "datamux.h"

#define MAXDEV 10

typedef struct _attached_device
{
	/* The device. */
	device_t	*device;

	/* Address bits involved */
	UINT16			address_mask;

	/* Value of the address bits which are involved for selecting for read. */
	UINT16			select;

	/* Value of the address bits which are involved for selecting for write. */
	UINT16			write_select;

	/* Read access. */
	databus_read_function read_byte;

	/* Write access. */
	databus_write_function write_byte;

	/* Setting which determined if this device is mounted or not. */
	const char	*setting;

	/* Bits which must be set in the setting. */
	UINT8		set;

	/* Bits which must not be set in the setting. */
	UINT8		unset;
} attached_device;

typedef struct _datamux_state
{
	/* All devices that are attached to the 8-bit bus. */
	attached_device component[MAXDEV];

	/* Latch which stores the first byte */
	UINT8 latch;

	// Intermediate storage which holds the low byte of the previous read cycle.
	// In reality, the TMS9900 adds the low byte to the 16 bit memory word
	// from the previous read-before-write cycle, but this is not yet handled
	// by the TMS9900 emulation. TODO: rewrite TMS9900
	UINT8 lowbyte;
	UINT8 highbyte;

	/* Memory expansion (internal, 16 bit). */
	UINT16 *ram16b;

	/* Use the memory expansion? */
	int use32k;

	/* Management function */
	int devindex;
} datamux_state;


INLINE datamux_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DMUX);

	return (datamux_state *)downcast<legacy_device_base *>(device)->token();
}

static void dmux_mount_device(device_t *dmuxdev, device_t *busdevice, UINT16 address_mask, UINT16 address_bits, UINT16 write_sel, databus_read_function read, databus_write_function write)
{
	datamux_state *dmux = get_safe_token(dmuxdev);
	int index = dmux->devindex++;
	if (index < MAXDEV)
	{
		if (dmux->component[index].device == NULL)
		{
			dmux->component[index].device = busdevice;
			dmux->component[index].address_mask = address_mask;
			dmux->component[index].select = address_bits;
			dmux->component[index].write_select = write_sel;
			dmux->component[index].read_byte = read;
			dmux->component[index].write_byte = write;
			// printf("registering the device at position %d\n", index);
		}
	}
	else
	{
		logerror("ti99_dmux: Tried to mount too many devices to the DMUX. (Just a bug.)\n");
	}
}

/***************************************************************************
    DEVICE ACCESSOR FUNCTIONS
***************************************************************************/

/*
    Read access. We are using two loops because the delay between both
    accesses must not occur within the loop. So we have one access on the bus,
    a delay, and then the second access (each one with possibly many attached
    devices)
*/
READ16_DEVICE_HANDLER( ti99_dmux_r )
{
	UINT8 hbyte = 0;
	datamux_state *dmux = get_safe_token(device);
	UINT16 addr = (offset << 1);

	// Looks ugly, but this is close to the real thing. If the 16bit
	// memory expansion is installed in the console, and the access hits its
	// space, just respond to the memory access and don't bother the
	// datamux in any way. In particular, do not make the datamux insert wait
	// states.
	if (dmux->use32k)
	{
		UINT16 base = 0;
		if ((addr & 0xe000)==0x2000) base = 0x1000;
		if (((addr & 0xe000)==0xa000) || ((addr & 0xc000)==0xc000)) base = 0x4000;

		if (base != 0)
		{
			UINT16 reply = dmux->ram16b[offset-base];
			// Store the prefetch
			dmux->lowbyte = reply & 0x00ff;
			dmux->highbyte = (reply & 0xff00)>>8;
			return reply;
		}
	}

	for (int i=0; i < MAXDEV; i++)
	{
		attached_device *adev = &dmux->component[i];
		if (dmux->component[i].device != NULL)
		{
			if (adev->read_byte != NULL)
			{
				if (((addr+1) & adev->address_mask)==adev->select)
				{
					// put on the latch
					adev->read_byte(adev->device, addr+1, &dmux->latch);
					dmux->lowbyte = dmux->latch;
				}
			}
			// hope we don't have two devices answering...
			// consider something like a logical OR and maybe some artificial smoke
		}
	}

	// Takes three cycles
	device_adjust_icount(device->machine().device("maincpu"),-3);

	for (int i=0; i < MAXDEV; i++)
	{
		attached_device *adev = &dmux->component[i];
		if (dmux->component[i].device != NULL)
		{
			if (adev->read_byte != NULL)
			{
				if ((addr & adev->address_mask)==adev->select)
				{
					adev->read_byte(adev->device, addr, &hbyte);
					dmux->highbyte = hbyte;
				}
			}
		}
	}

	// Takes three cycles
	device_adjust_icount(device->machine().device("maincpu"),-3);

	// use the latch and the currently read byte and put it on the 16bit bus
	return (hbyte<<8) | dmux->latch ;
}

/*
    Write access.
*/
WRITE16_DEVICE_HANDLER( ti99_dmux_w )
{
	datamux_state *dmux = get_safe_token(device);
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

//  printf("write addr=%04x memmask=%04x value=%04x\n", addr, mem_mask, data);
	if (mem_mask == 0xff00)
		data = data | dmux->lowbyte;

	if (mem_mask == 0x00ff)
		data = data | (dmux->highbyte << 8);

	// Handle the internal 32K expansion
	if (dmux->use32k)
	{
		if (addr>=0x2000 && addr<0x4000)
		{
			dmux->ram16b[offset-0x1000] = data; // index 0000 - 0fff
			return;
		}
		if (addr>=0xa000)
		{
			dmux->ram16b[offset-0x4000] = data; // index 1000 - 4fff
			return;
		}
	}

	for (int i=0; i < MAXDEV; i++)
	{
		attached_device *adev = &dmux->component[i];
		if (dmux->component[i].device != NULL)
		{
			if (adev->write_byte != NULL)
			{
				if (((addr+1) & adev->address_mask)==(adev->select|adev->write_select))
				{
					// write the byte from the lower 8 lines
					adev->write_byte(adev->device, addr+1, data & 0xff);
				}
			}
		}
	}

	// Takes three cycles
	device_adjust_icount(device->machine().device("maincpu"),-3);

	for (int i=0; i < MAXDEV; i++)
	{
		attached_device *adev = &dmux->component[i];
		if (dmux->component[i].device != NULL)
		{
			if (adev->write_byte != NULL)
			{
				if ((addr & adev->address_mask)==(adev->select|adev->write_select))
				{
					// write the byte from the upper 8 lines
					adev->write_byte(adev->device, addr, (data>>8) & 0xff);
				}
			}
		}
	}

	// Takes three cycles
	device_adjust_icount(device->machine().device("maincpu"),-3);
}

/* CRU space is not involved in the dmux. */

/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

static DEVICE_START( datamux )
{
	datamux_state *dmux = get_safe_token(device);
	dmux->ram16b = NULL;
}

static DEVICE_STOP( datamux )
{
	datamux_state *dmux = get_safe_token(device);
	if (dmux->ram16b) free(dmux->ram16b);
}

static DEVICE_RESET( datamux )
{
	datamux_state *dmux = get_safe_token(device);
	dmux->use32k = (input_port_read(device->machine(), "RAM")==RAM_TI32_INT)? TRUE : FALSE;

	dmux->devindex = 0;
	int i = 0;
	int done=FALSE;

	for (int index = 0; index < MAXDEV; index++)
	{
		dmux->component[index].device = NULL;
	}

	if (dmux->ram16b==NULL) dmux->ram16b = (UINT16*)malloc(32768);
	bus_device *cons = (bus_device*)device->baseconfig().static_config();
	while (!done)
	{
		if (cons[i].name != NULL)
		{
			device_t *dev = device->machine().device(cons[i].name);
			if (dev!=NULL)
			{
				UINT32 set = 0;
				if (cons[i].setting!=NULL)
					set = input_port_read(device->machine(), cons[i].setting);

				if (((set & cons[i].set)==cons[i].set)&&((set & cons[i].unset)==0))
				{
					dmux_mount_device(device, dev, cons[i].address_mask, cons[i].select_pattern, cons[i].write_select, cons[i].read, cons[i].write);
					logerror("ti99_datamux: Device %s mounted at index %d.\n", cons[i].name, i);
				}
				else
				{
					logerror("ti99_datamux: Device %s not mounted due to configuration setting %s.\n", cons[i].name, cons[i].setting);
				}
			}
			else
				logerror("ti99_datamux: Device %s not found.\n", cons[i].name);
		}
		else done=TRUE;
		i++;
	}
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##datamux##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET
#define DEVTEMPLATE_NAME                "TI-99 Databus 16/8 Multiplexer"
#define DEVTEMPLATE_FAMILY              "Internal devices"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( DMUX, datamux );
