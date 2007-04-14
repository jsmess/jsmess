//============================================================
//
//	rc_cfg.c - rc handling in XML .cfg files
//
//	Tasks:
//		1.  Create code to allow MESS to read INI info in CFG
//		2.  Modify existing code to not trample on CFG settings
//		3.  Make MESSGUI CFG aware
//
//============================================================

#include <windows.h>

#include "rc_cfg.h"
#include "windows/config.h"

static struct rc_struct *win_rc;

static void config_rc_load(int config_type, struct xml_data_node *parentnode)
{
}



static void config_rc_write(struct xml_data_node *node, const struct rc_option *option)
{
	int i;
	const char *s;
	char buf[32];

	for(i = 0; option[i].type; i++)
	{
		s = NULL;
		switch (option[i].type)
		{
			case rc_seperator:
				break;

			case rc_link:
				config_rc_write(node, option[i].dest);
				break;

			case rc_string:
				s = *(char **)option[i].dest;
				break;

			case rc_bool:
			case rc_int:
				snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%d", *(int *)option[i].dest);
				s = buf;
				break;

			case rc_float:
				snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%f", *(float *)option[i].dest);
				s = buf;
				break;
		}
		if (s && *s)
			xml_add_child(node, option[i].name, s);
	}
}



static void config_rc_save(int config_type, struct xml_data_node *parentnode)
{
	extern struct rc_struct *rc;

	switch(config_type)
	{
		case CONFIG_TYPE_GAME:
		case CONFIG_TYPE_DEFAULT:
			config_rc_write(parentnode, rc_get_options(rc));
			break;
	}
}



void win_config_rc_register(void)
{
	config_register("windows", config_rc_load, config_rc_save);
}


