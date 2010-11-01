/***************************************************************************

    TI-99/8 Address decoder and mapper

    This component implements the address decoder and mapper logic from the
    TI-99/8 console.

    The TI-99/8 defines a "logical address map" with 64 KiB (according to the
    16 address bits) and a "physical address map" with 16 MiB (according to the
    24 address bits of the mapper). Note that the mapper only uses 16 outgoing
    address lines and multiplexes the address bytes.

    Note: The TI-99/8's internal codename was "Armadillo"

    Initial setting of mapper (as defined in the power-up routine, TI-99/4A mode)

    0   00ff0000 -> Unmapped; logical address 0000...0fff = ROM0
    1   00ff0000 -> Unmapped; logical address 1000...1fff = ROM0
    2   00000800 -> DRAM; 2000 = 000800, 2fff = 0017ff
    3   00001800 -> DRAM; 3000 = 001800, 3fff = 0027ff
    4   00ff4000 -> DSR space (internal / ioport)
    5   00ff5000 -> DSR space (internal / ioport)
    6   00ff6000 -> Cartridge space (6000..6fff)
    7   00ff7000 -> Cartridge space (7000..7fff)
    8   00ff0000 -> Unmapped; device ports (VDP) and SRAM
    9   00ff0000 -> Unmapped; device ports (Speech, GROM)
    A   00002800 -> DRAM; a000 = 002800, afff = 0037ff
    B   00003800 -> DRAM; b000 = 003800, bfff = 0047ff
    C   00004800 -> DRAM; c000 = 004800, cfff = 0057ff
    D   00005800 -> DRAM; d000 = 005800, dfff = 0067ff
    E   00006800 -> DRAM; e000 = 006800, efff = 0077ff
    F   00007800 -> DRAM; f000 = 007800, ffff = 0087ff

    Informations taken from
    [1] ARMADILLO PRODUCT SPECIFICATIONS
    [2] TI-99/8 Graphics Programming Language interpreter

    Michael Zapf, October 2010

***************************************************************************/

#include "emu.h"
#include "peribox.h"
#include "mapper8.h"

#define MAXLOGDEV 20
#define MAXPHYSDEV 10

#define SRAM_SIZE 2048

#define DRAM_SIZE 65536

#define MAPPER_CRU_BASE 0x2700

#define MAP8_SRAM (void*)1
#define MAP8_ROM0 (void*)2
#define MAP8_ROM1 (void*)3
#define MAP8_ROM1A (void*)4
#define MAP8_DRAM (void*)5
#define MAP8_INTS (void*)6

typedef struct _log_addressed_device
{
	/* The device. */
	running_device	*device;

	/* Mode. */
	int				mode;

	/* Stop. If true, stop the address decoder here and do not look for parallel devices. */
	int				stop;

	/* Address bits involved */
	UINT16			address_mask;

	/* Value of the address bits which are involved for selecting for read. */
	UINT16			select;

	/* Value of the address bits which are involved for selecting for write. */
	UINT16			write_select;

	/* Read access. */
	mapper_read_function read_byte;

	/* Write access. */
	mapper_write_function write_byte;

} log_addressed_device;

typedef struct _phys_addressed_device
{
	/* The device. */
	running_device	*device;

	/* Stop. If true, stop the address decoder here and do not look for parallel devices. */
	int				stop;

	/* Address bits involved */
	UINT32			address_mask;

	/* Value of the address bits which are involved for selecting for read. */
	UINT32			select;

	/* Read access. */
	mapper_read_function read_byte;

	/* Write access. */
	mapper_write_function write_byte;

} phys_addressed_device;

typedef struct _mapper8_state
{
	/* All devices that are attached to the 16-bit address bus. */
	log_addressed_device logcomp[MAXLOGDEV];

	/* All devices that are attached to the 24-bit mapped address bus. */
	phys_addressed_device physcomp[MAXPHYSDEV];

	/* The I/O port. */
	running_device	*ioport;

	/* Select bit for the internal DSR. */
	int	dsr_selected;

	/* 99/4A compatibility mode. Name is taken from the spec. If 1, 99/4A compatibility is active. */
	int	CRUS;

	/* P-Code mode. Name is taken from the spec. If 0, P-Code libraries are available. */
	// May be read as "Pascal and Text-to-speech GROM libraries ENable"
	// Note: this is negative logic. GROMs are present only for PTGEN=0
	int PTGEN;

	/* Address mapper registers. Each offset is selected by the first 4 bits
       of the logical address. */
	UINT32	pas_offset[16];

	/* SRAM area of the system. Directly connected to the address decoder. */
	UINT8	*sram;

	/* DRAM area of the system. Connected to the mapped address bus. */
	UINT8	*dram;

	/* ROM area of the system. Directly connected to the address decoder. */
	UINT8	*rom;

	/* Points to the next free slot in the list. */
	int logindex, physindex;

} mapper8_state;


INLINE mapper8_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(downcast<legacy_device_base *>(device)->token() != NULL);
	return (mapper8_state *)downcast<legacy_device_base *>(device)->token();
}

static void mapper_mount_logical_device(running_device *mapperdev, running_device *busdevice, int mode, int stop, UINT16 address_mask, UINT16 address_bits, UINT16 write_sel, mapper_read_function read, mapper_write_function write)
{
	mapper8_state *mapper = get_safe_token(mapperdev);
	int index = mapper->logindex++;
	if (index < MAXLOGDEV)
	{
		if (mapper->logcomp[index].device == NULL)
		{
			mapper->logcomp[index].device = busdevice;
			mapper->logcomp[index].mode = mode;
			mapper->logcomp[index].stop = stop;

			mapper->logcomp[index].address_mask = address_mask;
			mapper->logcomp[index].select = address_bits;
			mapper->logcomp[index].write_select = write_sel;
			mapper->logcomp[index].read_byte = read;
			mapper->logcomp[index].write_byte = write;
			// printf("registering the device at logical position %d, device %lx\n", index, busdevice);
		}
	}
	else
	{
		logerror("ti99_8_mapper: Tried to mount too many logical devices to the mapper. (Actually a bug.)\n");
	}
}

static void mapper_mount_physical_device(running_device *mapperdev, running_device *busdevice, int stop, UINT32 address_mask, UINT32 address_bits, mapper_read_function read, mapper_write_function write)
{
	mapper8_state *mapper = get_safe_token(mapperdev);
	int index = mapper->physindex++;
	if (index < MAXPHYSDEV)
	{
		if (mapper->physcomp[index].device == NULL)
		{
			mapper->physcomp[index].device = busdevice;
			mapper->physcomp[index].stop = stop;
			mapper->physcomp[index].address_mask = address_mask;
			mapper->physcomp[index].select = address_bits;
			mapper->physcomp[index].read_byte = read;
			mapper->physcomp[index].write_byte = write;
			// printf("registering the device at physical position %d, device %lx\n", index, busdevice);
		}
	}
	else
	{
		logerror("ti99_8_mapper: Tried to mount too many phys devices to the mapper. (Actually a bug.)\n");
	}
}

/***************************************************************************
    DEVICE ACCESSOR FUNCTIONS
***************************************************************************/

static int check_logically_addressed_r( device_t *device, offs_t offset, UINT8 *value)
{
	mapper8_state *mapper = get_safe_token(device);
	int found = FALSE;
	// printf("ti99_8: read access %04x ... ", offset);
	// Look for components responding to the logical address
	for (int i=0; i < MAXLOGDEV; i++)
	{
		log_addressed_device *adev = &mapper->logcomp[i];
		if (adev->device != NULL)
		{
			// Check for the mode
			if (adev->mode == mapper->CRUS || (mapper->CRUS==0 && adev->mode==1 && mapper->PTGEN==FALSE))
			{
				if ((offset & adev->address_mask)==adev->select)
				{
					if (adev->device == MAP8_SRAM)
					{
						*value = mapper->sram[offset & ~adev->address_mask];
						// printf("sram");
						found = TRUE;
					}
					else
					{
						if (adev->device == MAP8_ROM0)
						{
							// Starts at 0000
							*value = mapper->rom[offset & ~adev->address_mask];
							// printf("rom0");
							found = TRUE;
						}
						else
						{
							// device
							if (adev->read_byte != NULL)
							{
								// printf("device ");
								adev->read_byte(adev->device, offset, value);
								found = TRUE;
							}
						}
					}
				}
			}
		}
		if (found && adev->stop) break;
	}
	return found;
}

static int check_logically_addressed_w( device_t *device, offs_t offset, UINT8 data)
{
	mapper8_state *mapper = get_safe_token(device);
	int found = FALSE;

	// printf("ti99_8: write access %04x ... ", offset);
	for (int i=0; i < MAXLOGDEV; i++)
	{
		log_addressed_device *adev = &mapper->logcomp[i];
		if (adev->device != NULL)
		{
			// Check for the mode
			if (adev->mode == mapper->CRUS ||  (mapper->CRUS==0 && adev->mode==1 && mapper->PTGEN==FALSE))
			{
				if ((offset & adev->address_mask)==(adev->select | adev->write_select))
				{
					if (adev->device == MAP8_SRAM)
					{
						mapper->sram[offset & ~adev->address_mask] = data;
						// printf("sram");
						found = TRUE;
					}
					else
					{
						if (adev->device == MAP8_ROM0)
						{
							// printf("rom0 (ignored)");
							found = TRUE;
						}
						else
						{
							if (adev->write_byte != NULL)
							{
								// printf("device ");
								adev->write_byte(adev->device, offset, data);
								found = TRUE;
							}
						}
					}
				}
			}
		}
		if (found && adev->stop) break;
	}
	return found;
}

/*
    Read access.
*/
READ8_DEVICE_HANDLER( ti99_mapper8_r )
{
	UINT8 value = 0;
	mapper8_state *mapper = get_safe_token(device);
	int found = FALSE;

	if (mapper->sram==NULL)  // not initialized yet
		return 0;

	found = check_logically_addressed_r(device, offset, &value);

	if (!found)
	{
		// printf(" not logical ...");
		// In that case, the address decoder could not find a suitable device.
		// This means the logical address is transformed by the mapper.
		UINT32	pas_address = mapper->pas_offset[(offset & 0xf000)>>12] + (offset & 0xfff);
		// printf("%08x ", pas_address);

		// So now let's do the same as above with physical addresses
		for (int i=0; i < MAXPHYSDEV; i++)
		{
			phys_addressed_device *adev = &mapper->physcomp[i];
			if (adev->device != NULL)
			{
				if ((pas_address & adev->address_mask)==adev->select)
				{
					if (adev->device == MAP8_DRAM)
					{
						value = mapper->dram[pas_address & ~adev->address_mask];
						found = TRUE;
						// printf("dram");
					}
					else
					{
						if (adev->device == MAP8_ROM1)
						{
							// Starts at 2000 in the image, 8K
							value = mapper->rom[0x2000 + (pas_address & 0x1fff)];
							found = TRUE;
							// printf("rom1");
						}
						else
						{
							if (adev->device == MAP8_ROM1A)
							{
								// Starts at 6000 in the image, 8K
								value = mapper->rom[0x6000 + (pas_address & 0x1fff)];
								found = TRUE;
								// printf("rom1a");
							}
							else
							{
								if (adev->device == MAP8_INTS)
								{
									// Interrupt sense
									// printf("ti99_8: ILSENSE not implemented.\n");
									// printf("sense");
									found = TRUE;
								}
								else
								{
									// only one option left: cartridge port (aka grom port)
									if (adev->read_byte != NULL)
									{
										// printf("pdevice ");
										adev->read_byte(adev->device, pas_address, &value);
										found = TRUE;
									}
								}
							}
						}
					}
				}
			}
			if (found && adev->stop) break;
		}

		if (!found)
		{
			// Still not found? Then we route the physical address to the I/O port
			// which has the peribox attached
			// printf("not mapped ... ioport");
			if (mapper->ioport!=NULL)
				ti99_peb_data_rz(mapper->ioport, pas_address, &value);
			// printf("ti99_8: io read %04x = %02x\n", offset, value);
		}
	}
	// printf(" %02x\n", value);

	return value;
}

/*
    Write access.
*/
WRITE8_DEVICE_HANDLER( ti99_mapper8_w )
{
	mapper8_state *mapper = get_safe_token(device);
	int found = FALSE;
	if (mapper->sram==NULL)
		return;
//  // printf("ti99_8: write %04x = %02x\n", offset, data);

	// Look for components responding to the logical address
	found = check_logically_addressed_w(device, offset, data);

	if (!found)
	{
		// printf(" not logical ...");
		// In that case, the address decoder could not find a suitable device.
		// This means the logical address is transformed by the mapper.
		UINT32 pas_address = mapper->pas_offset[(offset & 0xf000)>>12] + (offset & 0xfff);

		// So now let's do the same as above with physical addresses
		for (int i=0; i < MAXPHYSDEV; i++)
		{
			phys_addressed_device *adev = &mapper->physcomp[i];
			if (adev->device != NULL)
			{
				if ((pas_address & adev->address_mask)==adev->select)
				{
					if (adev->device == MAP8_DRAM)
					{
						mapper->dram[pas_address & ~adev->address_mask] = data;
						// printf("dram base %08x", pas_address);
						found = TRUE;
					}
					else
					{
						if (adev->device == MAP8_ROM1 || adev->device == MAP8_ROM1A)
						{
							// printf("rom1 (ignored)");
							found = TRUE;
						}
						else
						{
							if (adev->device == MAP8_INTS)
							{
								// Interrupt sense
								// printf("ilsense (ignored)");
								found = TRUE;
							}
							else
							{
								// only one option left: cartridge port (aka grom port)
								if (adev->write_byte != NULL)
								{
									// printf("pdevice ");
									adev->write_byte(adev->device, pas_address, data);
									found = TRUE;
								}
							}
						}
					}
				}
			}
			if (found && adev->stop) break;
		}

		if (!found)
		{
			// Still not found? Then we route the physical address to the I/O port
			// which has the peribox attached
			// printf("not mapped ... ioport");
			if (mapper->ioport!=NULL)
				ti99_peb_data_w(mapper->ioport, pas_address, data);
			// printf("ti99_8: io write %04x = %02x\n", offset, data);
		}
	}
	// printf(" %02x\n", data);
}

/*
    Reconfigure mapper. Writing to this address copies the values in the
    SRAM into the mapper and vice versa.
    Format:
    0000 bbbl; bbb=bank, l=load
*/
WRITE8_DEVICE_HANDLER( ti99_mapreg_w )
{
	mapper8_state *mapper = get_safe_token(device);

	if ((data & 0xf0)==0)
	{
		int bankindx = (data & 0x0e)>>1;
		if (data & 1)
		{
			// printf("mapper8: load from sram bank %02x", bankindx);
			// Load from SRAM
			for (int i=0; i < 16; i++)
			{
				int ptr = (bankindx << 6);
				mapper->pas_offset[i] =
					(mapper->sram[i*4 + ptr] << 24)
					+ (mapper->sram[i*4 + ptr+1] << 16)
					+ (mapper->sram[i*4 + ptr+2] << 8)
					+ (mapper->sram[i*4 + ptr+3]);
//              // printf(" values (%04x) %02x %02x %02x %02x, ", i*4+ptr, mapper->sram[i*4 + ptr], mapper->sram[i*4 + ptr+1], mapper->sram[i*4 + ptr+2], mapper->sram[i*4 + ptr+3]);
			}
		}
		else
		{
			// Store in SRAM
			// printf("mapper8: copy to sram bank %02x", bankindx);
			for (int i=0; i < 16; i++)
			{
				int ptr = (bankindx << 6);
				mapper->sram[i*4 + ptr]    =  (mapper->pas_offset[i] >> 24)& 0xff;
				mapper->sram[i*4 + ptr +1] =  (mapper->pas_offset[i] >> 16)& 0xff;
				mapper->sram[i*4 + ptr +2] =  (mapper->pas_offset[i] >> 8)& 0xff;
				mapper->sram[i*4 + ptr +3] =  (mapper->pas_offset[i])& 0xff;
			}
		}
	}
}

/************************************************************************/

/*
    Internal DSR
*/
READ8Z_DEVICE_HANDLER( ti998dsr_rz )
{
	mapper8_state *mapper = get_safe_token(device);
	if (mapper->dsr_selected)
	{
		//  Starts at 0x4000 in the image
		*value = mapper->rom[0x4000 + (offset & 0x1fff)];
	}
	else
	{
		if (mapper->ioport!=NULL)
			ti99_peb_data_rz(mapper->ioport, 0x4000 + (offset & 0x1fff), value);
//      printf("ti99_8: io1 read %04x = %02x\n", offset, *value);
	}
}


/*
    CRU handling. We handle the internal device at CRU address 0x2700 via
    this mapper component.
*/
WRITE8_DEVICE_HANDLER( mapper8c_w )
{
	mapper8_state *mapper = get_safe_token(device);
	if ((offset & 0xff00)==MAPPER_CRU_BASE)
	{
		int addr = offset & 0x00ff;
		if (addr==0)
			mapper->dsr_selected = data;
		if (addr==2)
		{
			// TODO
			// printf("ti-99/8: Hardware reset via CRU\n");
		}
	}
}

WRITE8_DEVICE_HANDLER( mapper8_CRUS_w )
{
	mapper8_state *mapper = get_safe_token(device);
	mapper->CRUS = data;
	// printf("ti99_8: CRUS = %02x\n", data);
}

WRITE8_DEVICE_HANDLER( mapper8_PTGEN_w )
{
	mapper8_state *mapper = get_safe_token(device);
	mapper->PTGEN = data;
	// printf("ti99_8: PTGEN = %02x\n", data);
}

/***************************************************************************
    DEVICE LIFECYCLE FUNCTIONS
***************************************************************************/

static DEVICE_START( mapper8 )
{
//  // printf("Starting mapper\n");
	mapper8_state *mapper = get_safe_token(device);
	mapper->sram = (UINT8*)malloc(SRAM_SIZE);
	mapper->dram = (UINT8*)malloc(DRAM_SIZE);
	mapper->rom = memory_region(device->machine, "maincpu");

	int i = 0;
	int done=FALSE;

	for (int index = 0; index < MAXLOGDEV; index++)
	{
		mapper->logcomp[index].device = NULL;
	}

	for (int index = 0; index < MAXPHYSDEV; index++)
	{
		mapper->physcomp[index].device = NULL;
	}

	// Need to connect the callbacks
	mapper->ioport = device->siblingdevice("peribox");
	assert(mapper->ioport != NULL);

	mapper8_dev_config *cons = (mapper8_dev_config*)device->baseconfig().static_config();

	mapper->logindex = 0;
	mapper->physindex = 0;

	const char *list[6] = { SRAMNAME, ROM0NAME, ROM1NAME, ROM1ANAME, DRAMNAME, INTSNAME };
	while (!done)
	{
		void *dev = NULL;
		if (cons[i].name==NULL)
			done = TRUE;
		else
		{
			for (int j=1; (j < 7) && (dev == NULL); j++)
			{
				// printf("%s, %s\n", cons[i].name, list[j-1]);
				if (strcmp(cons[i].name, list[j-1])==0)
					dev = (void *)j;
			}
			if (dev==NULL)
				dev = device->machine->device(cons[i].name);

			if (dev!=NULL)
			{
				if (cons[i].mode != 3)
				{
					mapper_mount_logical_device(device, (running_device*)dev, cons[i].mode, cons[i].stop, (UINT16)cons[i].address_mask, (UINT16)cons[i].select_pattern, (UINT16)cons[i].write_select, cons[i].read, cons[i].write);
				}
				else
				{
					mapper_mount_physical_device(device, (running_device*)dev, cons[i].stop, cons[i].address_mask, cons[i].select_pattern, cons[i].read, cons[i].write);
				}
			}
			else
				logerror("mapper8: Device %s not found.\n", cons[i].name);
			i++;
		}
	}

	mapper->dsr_selected = FALSE;
	mapper->CRUS = TRUE;
	mapper->PTGEN = TRUE;
}

static DEVICE_STOP( mapper8 )
{
	// printf("Stopping mapper\n");
	mapper8_state *mapper = get_safe_token(device);
	free(mapper->sram);
	free(mapper->dram);
}

static DEVICE_RESET( mapper8 )
{
	// printf("Resetting mapper\n");
	mapper8_state *mapper = get_safe_token(device);

	mapper->dsr_selected = FALSE;
	mapper->CRUS = TRUE;
	mapper->PTGEN = TRUE;
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##mapper8##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET
#define DEVTEMPLATE_NAME                "TI-99/8 Address Decoder and Mapper"
#define DEVTEMPLATE_FAMILY              "Internal devices"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( MAPPER8, mapper8 );
