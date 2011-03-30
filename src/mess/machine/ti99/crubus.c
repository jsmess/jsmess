/*
    TI-99 CRU Bus
*/

#include "emu.h"
#include "crubus.h"

#define MAXCRU 20

typedef struct _crubus_dev
{
	/* The device. */
	device_t		*device;

	/* Read access. */
	cru_read_function	cru_read;

	/* Write access. */
	cru_write_function	cru_write;

	/* Address bits involved. The CRU makes use of the address bus. */
	UINT16			address_mask;

	/* Value of the address bits which are involved for selecting for read. */
	UINT16			select;

} crubus_dev;

typedef struct _crubus_state
{
	crubus_dev			component[MAXCRU];
	/* Management function */
	int devindex;

} crubus_state;

INLINE crubus_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == CRUBUS);

	return (crubus_state *)downcast<legacy_device_base *>(device)->token();
}

static void cru_mount_device(device_t *device, device_t *crudev, UINT16 address_mask, UINT16 address_bits, cru_read_function cruread, cru_write_function cruwrite)
{
	crubus_state *crubus = get_safe_token(device);
	int index = crubus->devindex++;
//  logerror("register cru device %lx\n", (long unsigned int)crudev);
	if (index < MAXCRU)
	{
		if (crubus->component[index].device == NULL)
		{
			crubus->component[index].device = crudev;
			crubus->component[index].cru_read = cruread;
			crubus->component[index].cru_write = cruwrite;
			crubus->component[index].address_mask = address_mask;
			crubus->component[index].select = address_bits;
		}
	}
}

/***************************************************************************
    ACCESSOR FUNCTIONS
***************************************************************************/

READ8_DEVICE_HANDLER( ti99_crubus_r )
{
	crubus_state *crubus = get_safe_token(device);
	UINT8 reply = 0;

	for (int i=0; i < MAXCRU; i++)
	{
		crubus_dev *crudev = &crubus->component[i];
		if (crubus->component[i].device != NULL)
		{
			if (crudev->cru_read != NULL)
			{
				if (((offset<<4) & crudev->address_mask)==crudev->select)
				{
					// CRU read is implemented as an 8-bit read access.
					// We shift the address for the sake of readability
					// Each read access returns 8 consecutive CRU bits
					// Shift 4 = addr*8*2
					crudev->cru_read(crudev->device, offset<<4, &reply);
				}
			}
		}
	}
//  printf("cru read %04x = %02x\n", offset<<4, reply);

	return reply;
}

WRITE8_DEVICE_HANDLER( ti99_crubus_w )
{
	crubus_state *crubus = get_safe_token(device);

//  printf("cru write %04x = %02x\n", offset<<1, data);
	for (int i=0; i < MAXCRU; i++)
	{
		crubus_dev *crudev = &crubus->component[i];
		if (crubus->component[i].device != NULL)
		{
			if (crudev->cru_write != NULL)
			{
				if (((offset<<1) & crudev->address_mask)==crudev->select)
				{
					// CRU write is implemented as a 1-bit access
					crudev->cru_write(crudev->device, offset<<1, data);
				}
			}
		}
	}
}

/***************************************************************************
    DEVICE FUNCTIONS
***************************************************************************/

static DEVICE_START( crubus )
{
	int i = 0;
	int done=FALSE;
	cruconf_device *cons = (cruconf_device*)device->baseconfig().static_config();
	crubus_state *crubus = get_safe_token(device);

	logerror("start CRU bus\n");

	crubus->devindex = 0;

	while (!done)
	{
		if (cons[i].name != NULL)
		{
			device_t *dev = device->machine().device(cons[i].name);
			if (dev!=NULL)
				cru_mount_device(device, dev, cons[i].address_mask, cons[i].select_pattern, cons[i].read, cons[i].write);
			else
				logerror("ti99_crubus: Device %s not found.\n", cons[i].name);
		}
		else done=TRUE;
		i++;
	}
}

static DEVICE_STOP( crubus )
{
	logerror("stop CRU bus\n");
}

static DEVICE_RESET( crubus )
{
	logerror("reset CRU bus\n");
}

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##crubus##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET
#define DEVTEMPLATE_NAME                "TI99 CRU bus system"
#define DEVTEMPLATE_FAMILY              "Internal devices"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( CRUBUS, crubus );
