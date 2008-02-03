/***************************************************************************

	configms.c - MESS specific config handling

****************************************************************************/

#include "mame.h"
#include "deprecat.h"
#include "configms.h"
#include "config.h"
#include "xmlfile.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    device_dirs_load - loads MESS device directory
	configuration items
-------------------------------------------------*/

static void device_dirs_load(int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	const struct IODevice *dev;
	int i;
	mess_image *image;
	const char *dev_instance;
	const char *working_directory;

	if ((config_type == CONFIG_TYPE_GAME) && (parentnode != NULL))
	{
		for (node = xml_get_sibling(parentnode->child, "device"); node; node = xml_get_sibling(node->next, "device"))
		{
			image = NULL;
			dev_instance = xml_get_attribute_string(node, "instance", NULL);

			if (dev_instance != NULL)
			{
				for (dev = Machine->devices; !image && (dev->type < IO_COUNT); dev++)
				{
					for (i = 0; !image && (i < dev->count); i++)
					{
						if (!strcmp(dev_instance, device_instancename(&dev->devclass, i)))
							image = image_from_device_and_index(dev, i);
					}
				}

				if (image != NULL)
				{
					working_directory = xml_get_attribute_string(node, "directory", NULL);
					if (working_directory != NULL)
						image_set_working_directory(image, working_directory);
				}
			}
		}
	}
}



/*-------------------------------------------------
    device_dirs_save - saves out MESS device
	directories to the configuration file
-------------------------------------------------*/

static void device_dirs_save(int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	const struct IODevice *dev;
	int i;
	mess_image *image;
	const char *dev_instance;

	/* only care about game-specific data */
	if (config_type == CONFIG_TYPE_GAME)
	{
		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			for (i = 0; i < dev->count; i++)
			{
				image = image_from_device_and_index(dev, i);
				dev_instance = device_instancename(&dev->devclass, i);

				node = xml_add_child(parentnode, "device", NULL);
				if (node)
				{
					xml_set_attribute(node, "instance", dev_instance);
					xml_set_attribute(node, "directory", image_working_directory(image));
				}
			}
		}
	}
}



/*-------------------------------------------------
    mess_config_init - sets up MESS specific
	options on config files
-------------------------------------------------*/

void mess_config_init(running_machine *machine)
{
	config_register("device_directories", device_dirs_load, device_dirs_save);
}
