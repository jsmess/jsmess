/***************************************************************************

    configms.c - MESS specific config handling

****************************************************************************/

#include "emu.h"
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

static void device_dirs_load(running_machine *machine, int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	const device_config *dev;
	image_device_info info;
	const device_config *image;
	const char *dev_instance;
	const char *working_directory;

	if ((config_type == CONFIG_TYPE_GAME) && (parentnode != NULL))
	{
		for (node = xml_get_sibling(parentnode->child, "device"); node; node = xml_get_sibling(node->next, "device"))
		{
			image = NULL;
			dev_instance = xml_get_attribute_string(node, "instance", NULL);

			if ((dev_instance != NULL) && (dev_instance[0] != '\0'))
			{
				for (dev = image_device_first(machine->config); (image == NULL) && (dev != NULL); dev = image_device_next(dev))
				{
					info = image_device_getinfo(machine->config, dev);
					if (!strcmp(dev_instance, info.instance_name))
						image = dev;
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

static void device_dirs_save(running_machine *machine, int config_type, xml_data_node *parentnode)
{
	xml_data_node *node;
	image_device_info info;
	const device_config *image;
	const char *dev_instance;

	/* only care about game-specific data */
	if (config_type == CONFIG_TYPE_GAME)
	{
		for (image = image_device_first(machine->config); image != NULL; image = image_device_next(image))
		{
			info = image_device_getinfo(machine->config, image);
			dev_instance = info.instance_name;

			node = xml_add_child(parentnode, "device", NULL);
			if (node != NULL)
			{
				xml_set_attribute(node, "instance", dev_instance);
				xml_set_attribute(node, "directory", image_working_directory(image));
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
	config_register(machine, "device_directories", device_dirs_load, device_dirs_save);
}
