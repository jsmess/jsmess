/***************************************************************************

  nc.c

***************************************************************************/

#include "driver.h"
#include "includes/nc.h"
#include "machine/serial.h"
#include "machine/msm8251.h"
#include "image.h"

/*************************************************************************************************/
/* PCMCIA Ram Card management */

/* the data is stored as a simple memory dump, there is no header or other information */

/* stores size of actual file on filesystem */
static int nc_card_size;

/* save card data back */
static void	nc_card_save(const device_config *image)
{
	/* if there is no data to write, quit */
	if (!nc_card_ram || !nc_card_size)
		return;

	logerror("attempting card save\n");

	/* write data */
	image_fwrite(image, nc_card_ram, nc_card_size);

	logerror("write succeeded!\r\n");
}

/* this mask will prevent overwrites from end of data */
static int nc_card_calculate_mask(int size)
{
	int i;

	/* memory block is visible as 16k blocks */
	/* mask can only operate on power of two sizes */
	/* memory cards normally in power of two sizes */
	/* maximum of 64 16k blocks can be accessed using memory paging of nc computer */
	/* max card size is therefore 1mb */
	for (i=14; i<20; i++)
	{
		if (size<(1<<i))
			return 0x03f>>(19-i);
	}

	return 0x03f;
}


/* load card image */
static int nc_card_load(const device_config *image, unsigned char **ptr)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image_length(image);

	if (datasize!=0)
	{
		/* malloc memory for this data */
		data = malloc(datasize);

		if (data!=NULL)
		{
			nc_card_size = datasize;

			/* read whole file */
			image_fread(image, data, datasize);

			*ptr = data;

			logerror("File loaded!\r\n");

			nc_membank_card_ram_mask = nc_card_calculate_mask(datasize);

			logerror("Mask: %02x\n",nc_membank_card_ram_mask);

			/* ok! */
			return 1;
		}
	}

	return 0;
}

DEVICE_START( nc_pcmcia_card )
{
	/* card not present */
	nc_set_card_present_state(0);
	/* card ram NULL */
	nc_card_ram = NULL;
	nc_card_size = 0;
}

/* load pcmcia card */
DEVICE_IMAGE_LOAD( nc_pcmcia_card )
{
	/* filename specified */

	/* attempt to load file */
	if (nc_card_load(image, &nc_card_ram))
	{
		if (nc_card_ram!=NULL)
		{
			/* card present! */
			if (nc_membank_card_ram_mask!=0)
			{
				nc_set_card_present_state(1);
			}
			return INIT_PASS;
		}
	}

	/* nc100 can run without a image */
	return INIT_FAIL;
}

DEVICE_IMAGE_UNLOAD( nc_pcmcia_card )
{
	/* save card data if there is any */
	nc_card_save(image);

	/* free ram allocated to card */
	if (nc_card_ram!=NULL)
	{
		free(nc_card_ram);
		nc_card_ram = NULL;
	}
	nc_card_size = 0;

	/* set card not present state */
	nc_set_card_present_state(0);
}


/*************************************************************************************************/
/* Serial */

DEVICE_IMAGE_LOAD( nc_serial )
{
	const device_config *uart = devtag_get_device(image->machine, "uart");

	/* filename specified */
	if (device_load_serial_device(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		/* connect serial chip to serial device */
		msm8251_connect_to_serial_device(uart, image);

		/* and start transmit */
		serial_device_set_transmit_state(image,1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}
