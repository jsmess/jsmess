/*********************************************************************

    uimenu.c

    Internal MAME menus for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "emu.h"
#include "osdnet.h"
#include "emuopts.h"
#include "ui.h"
#include "rendutil.h"
#include "cheat.h"
#include "uiimage.h"
#include "uiinput.h"
#include "uimenu.h"
#include "audit.h"
#include "crsshair.h"
#include <ctype.h>
#include "imagedev/cassette.h"
#include "imagedev/bitbngr.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define VISIBLE_GAMES_IN_LIST	15

#define MAX_PHYSICAL_DIPS		10
#define MAX_INPUT_PORTS			32
#define MAX_BITS_PER_PORT		32

/* DIP switch rendering parameters */
#define DIP_SWITCH_HEIGHT		0.05f
#define DIP_SWITCH_SPACING		0.01
#define SINGLE_TOGGLE_SWITCH_FIELD_WIDTH 0.025f
#define SINGLE_TOGGLE_SWITCH_WIDTH 0.020f
/* make the switch 80% of the width space and 1/2 of the switch height */
#define PERCENTAGE_OF_HALF_FIELD_USED 0.80f
#define SINGLE_TOGGLE_SWITCH_HEIGHT ((DIP_SWITCH_HEIGHT / 2) * PERCENTAGE_OF_HALF_FIELD_USED)


enum
{
	INPUT_TYPE_DIGITAL = 0,
	INPUT_TYPE_ANALOG = 1,
	INPUT_TYPE_ANALOG_DEC = INPUT_TYPE_ANALOG + SEQ_TYPE_DECREMENT,
	INPUT_TYPE_ANALOG_INC = INPUT_TYPE_ANALOG + SEQ_TYPE_INCREMENT,
	INPUT_TYPE_TOTAL = INPUT_TYPE_ANALOG + SEQ_TYPE_TOTAL
};

enum
{
	ANALOG_ITEM_KEYSPEED = 0,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY,
	ANALOG_ITEM_COUNT
};

enum
{
	MEMCARD_ITEM_SELECT = 1,
	MEMCARD_ITEM_LOAD,
	MEMCARD_ITEM_EJECT,
	MEMCARD_ITEM_CREATE
};

enum
{
	VIDEO_ITEM_ROTATE = 0x80000000,
	VIDEO_ITEM_BACKDROPS,
	VIDEO_ITEM_OVERLAYS,
	VIDEO_ITEM_BEZELS,
	VIDEO_ITEM_CPANELS,
	VIDEO_ITEM_MARQUEES,
	VIDEO_ITEM_ZOOM,
	VIDEO_ITEM_VIEW
};


enum
{
	CROSSHAIR_ITEM_VIS = 0,
	CROSSHAIR_ITEM_PIC,
	CROSSHAIR_ITEM_AUTO_TIME
};


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/



/* internal input menu item data */
struct input_item_data
{
	input_item_data *	next;				/* pointer to next item in the list */
	const void *		ref;				/* reference to type description for global inputs or field for game inputs */
	input_seq_type		seqtype;			/* sequence type */
	input_seq			seq;				/* copy of the live sequence */
	const input_seq *	defseq;				/* pointer to the default sequence */
	const char *		name;				/* pointer to the base name of the item */
	UINT16				sortorder;			/* sorting information */
	UINT8				type;				/* type of port */
};


/* internal analog menu item data */
struct analog_item_data
{
	input_field_config *field;
	int					type;
	int					min, max;
	int					cur;
	int 				defvalue;
};


/* DIP switch descriptor */
struct dip_descriptor
{
	dip_descriptor *	next;
	const char *		name;
	UINT32				mask;
	UINT32				state;
};


/* extended settings menu state */
struct settings_menu_state
{
	dip_descriptor *	diplist;
};


/* extended input menu state */
struct input_menu_state
{
	UINT16				last_sortorder;
	const void *		pollingref;
	input_item_data *	pollingitem;
	UINT8				record_next;
	input_seq			starting_seq;
};


/* extended select game menu state */
struct select_game_state
{
	UINT8				error;
	UINT8				rerandomize;
	char				search[40];
	int					matchlist[VISIBLE_GAMES_IN_LIST];
	const game_driver *	driverlist[1];
};


/* internal crosshair menu item data */
struct crosshair_item_data
{
	UINT8				type;
	UINT8				player;
	UINT8				min, max;
	UINT8				cur;
	UINT8				defvalue;
	char				last_name[CROSSHAIR_PIC_NAME_LENGTH + 1];
	char				next_name[CROSSHAIR_PIC_NAME_LENGTH + 1];
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

// temporary hack until this is C++-ified
static driver_enumerator *drivlist;

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* menu handlers */
static void menu_main_populate(running_machine &machine, ui_menu *menu, void *state);
static void menu_input_groups(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_input_groups_populate(running_machine &machine, ui_menu *menu, void *state);
static void menu_input_general(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_input_general_populate(running_machine &machine, ui_menu *menu, input_menu_state *menustate, int group);
static void menu_input_specific(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_input_specific_populate(running_machine &machine, ui_menu *menu, input_menu_state *menustate);
static void menu_input_common(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static int CLIB_DECL menu_input_compare_items(const void *i1, const void *i2);
static void menu_input_populate_and_sort(running_machine &machine, ui_menu *menu, input_item_data *itemlist, input_menu_state *menustate);
static void menu_settings_dip_switches(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_settings_driver_config(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_settings_common(running_machine &machine, ui_menu *menu, void *state, UINT32 type);
static void menu_settings_populate(running_machine &machine, ui_menu *menu, settings_menu_state *menustate, UINT32 type);
static void menu_analog(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_analog_populate(running_machine &machine, ui_menu *menu);
static void menu_bookkeeping(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_bookkeeping_populate(running_machine &machine, ui_menu *menu, attotime *curtime);
static void menu_game_info(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_cheat(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_cheat_populate(running_machine &machine, ui_menu *menu);
static void menu_memory_card(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_memory_card_populate(running_machine &machine, ui_menu *menu, int cardnum);
static void menu_sliders(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_sliders_populate(running_machine &machine, ui_menu *menu, int menuless_mode);
static void menu_sliders_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float x1, float y1, float x2, float y2);
static void menu_video_targets(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_video_targets_populate(running_machine &machine, ui_menu *menu);
static void menu_video_options(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_video_options_populate(running_machine &machine, ui_menu *menu, render_target *target);
static void menu_crosshair(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_crosshair_populate(running_machine &machine, ui_menu *menu);
static void menu_quit_game(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_select_game(running_machine &machine, ui_menu *menu, void *parameter, void *state);
static void menu_select_game_populate(running_machine &machine, ui_menu *menu, select_game_state *menustate);
static void menu_select_game_build_driver_list(ui_menu *menu, select_game_state *menustate);
static void menu_select_game_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float x, float y, float x2, float y2);

/* menu helpers */
static void menu_settings_custom_render_one(render_container *container, float x1, float y1, float x2, float y2, const dip_descriptor *dip, UINT32 selectedmask);
static void menu_settings_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float x, float y, float x2, float y2);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_field_default_seq - return a pointer
    to the default sequence for the given field
-------------------------------------------------*/

INLINE const input_seq &get_field_default_seq(input_field_config *field, input_seq_type seqtype)
{
	if (field->seq[seqtype].is_default())
		return input_type_seq(field->machine(), field->type, field->player, seqtype);
	else
		return field->seq[seqtype];
}


/*-------------------------------------------------
    toggle_none_default - toggle between "NONE"
    and the default item
-------------------------------------------------*/

INLINE void toggle_none_default(input_seq &selected_seq, input_seq &original_seq, const input_seq &selected_defseq)
{
	/* if we used to be "none", toggle to the default value */
	if (original_seq.length() == 0)
		selected_seq = selected_defseq;

	/* otherwise, toggle to "none" */
	else
		selected_seq.reset();
}


/*-------------------------------------------------
    ui_slider_ui_handler - pushes the slider
    menu on the stack and hands off to the
    standard menu handler
-------------------------------------------------*/

UINT32 ui_slider_ui_handler(running_machine &machine, render_container *container, UINT32 state)
{
	UINT32 result;

	/* if this is the first call, push the sliders menu */
	if (state)
		ui_menu::stack_push(ui_menu_alloc(machine, container, menu_sliders, (void *)1));

	/* handle standard menus */
	result = ui_menu::ui_handler(machine, container, state);

	/* if we are cancelled, pop the sliders menu */
	if (result == UI_HANDLER_CANCEL)
		ui_menu::stack_pop(machine);

	return (ui_menu::menu_stack != NULL && ui_menu::menu_stack->handler == menu_sliders && ui_menu::menu_stack->parameter != NULL) ? 0 : UI_HANDLER_CANCEL;
}


/*-------------------------------------------------
    ui_menu_force_game_select - force the game
    select menu to be visible and inescapable
-------------------------------------------------*/

void ui_menu_force_game_select(running_machine &machine, render_container *container)
{
	char *gamename = (char *)machine.options().system_name();

	/* reset the menu stack */
	ui_menu::stack_reset(machine);

	/* add the quit entry followed by the game select entry */
	ui_menu *quit = ui_menu_alloc(machine, container, menu_quit_game, NULL);
	quit->set_special_main_menu(true);
	ui_menu::stack_push(quit);
	ui_menu::stack_push(ui_menu_alloc(machine, container, menu_select_game, gamename));

	/* force the menus on */
	ui_show_menu();

	/* make sure MAME is paused */
	machine.pause();
}


/***************************************************************************
    MENU HANDLERS
***************************************************************************/

/*-------------------------------------------------
    menu_main - handle the main menu
-------------------------------------------------*/

void menu_main(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_main_populate(machine, menu, state);

	/* process the menu */
	menu_event = menu->process(0);
	if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT)
		ui_menu::stack_push(ui_menu_alloc(machine, menu->container, (ui_menu_handler_func)menu_event->itemref, NULL));
}


/*-------------------------------------------------
    ui_menu_keyboard_mode - menu that
-------------------------------------------------*/

static void ui_menu_keyboard_mode(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;
	int natural = ui_get_use_natural_keyboard(machine);

	/* if the menu isn't built, populate now */
	if (!menu->populated())
	{
		menu->item_append("Keyboard Mode:", natural ? "Natural" : "Emulated", natural ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, NULL);
	}

	/* process the menu */
	menu_event = menu->process(0);

	if (menu_event != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT) {
			ui_set_use_natural_keyboard(machine, natural ^ TRUE);
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
		}
	}
}


/*-------------------------------------------------
    ui_slot_get_current_index - returns
-------------------------------------------------*/
int ui_slot_get_current_index(running_machine &machine, device_slot_interface *slot)
{
	const char *current = machine.options().value(slot->device().tag());
	const slot_interface* intf = slot->get_slot_interfaces();
	int val = -1;
	for (int i = 0; intf[i].name != NULL; i++) {
		if (strcmp(current, intf[i].name) == 0) val = i;
	}
	return val;
}

/*-------------------------------------------------
    ui_slot_get_length - returns
-------------------------------------------------*/
int ui_slot_get_length(running_machine &machine, device_slot_interface *slot)
{
	const slot_interface* intf = slot->get_slot_interfaces();
	int val = 0;
	for (int i = 0; intf[i].name != NULL; i++) val++;
	return val;
}

/*-------------------------------------------------
    ui_slot_get_next - returns
-------------------------------------------------*/
const char *ui_slot_get_next(running_machine &machine, device_slot_interface *slot)
{
	int idx = ui_slot_get_current_index(machine, slot) + 1;
	if (idx==ui_slot_get_length(machine,slot)) return "";
	return slot->get_slot_interfaces()[idx].name;
}

/*-------------------------------------------------
    ui_slot_get_prev - returns
-------------------------------------------------*/
const char *ui_slot_get_prev(running_machine &machine, device_slot_interface *slot)
{
	int idx = ui_slot_get_current_index(machine, slot) - 1;
	if (idx==-1) return "";
	if (idx==-2) idx = ui_slot_get_length(machine,slot) -1;
	if (idx==-1) return "";
	return slot->get_slot_interfaces()[idx].name;
}

/*-------------------------------------------------
    ui_get_slot_device - returns
-------------------------------------------------*/
const char *ui_get_slot_device(running_machine &machine, device_slot_interface *slot)
{
	return machine.options().value(slot->device().tag());
}


/*-------------------------------------------------
    ui_set_use_natural_keyboard - specifies
    whether the natural keyboard is active
-------------------------------------------------*/

void ui_set_slot_device(running_machine &machine, device_slot_interface *slot, const char *val)
{
	astring error;
	machine.options().set_value(slot->device().tag(), val, OPTION_PRIORITY_CMDLINE, error);
	assert(!error);
}

/*-------------------------------------------------
    menu_slot_devices_populate - populates the main
    slot device menu
-------------------------------------------------*/

static void menu_slot_devices_populate(running_machine &machine, ui_menu *menu, void *state)
{
	device_slot_interface *slot = NULL;

	/* cycle through all devices for this system */
	for (bool gotone = machine.devicelist().first(slot); gotone; gotone = slot->next(slot))
	{
		/* record the menu item */
		const char *title = ui_get_slot_device(machine,slot);
		menu->item_append(slot->device().tag(),  strcmp(title,"")==0 ? "------" : title, MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)slot);
	}
	menu->item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);
	menu->item_append("Reset",  NULL, 0, NULL);
}

/*-------------------------------------------------
    ui_menu_slot_devices - menu that
-------------------------------------------------*/

static void ui_menu_slot_devices(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
	{
		menu_slot_devices_populate(machine, menu, state);
	}

	/* process the menu */
	menu_event = menu->process(0);

	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT) {
			device_slot_interface *slot = (device_slot_interface *)menu_event->itemref;
			const char *val = (menu_event->iptkey == IPT_UI_LEFT) ? ui_slot_get_prev(machine,slot) : ui_slot_get_next(machine,slot);
			ui_set_slot_device(machine,slot,val);
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
		}
	} else if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT) {
		machine.schedule_hard_reset();
	}
}


/*-------------------------------------------------
    menu_network_devices_populate - populates the main
    network device menu
-------------------------------------------------*/

static void menu_network_devices_populate(running_machine &machine, ui_menu *menu, void *state)
{
	device_network_interface *network = NULL;

	/* cycle through all devices for this system */
	for (bool gotone = machine.devicelist().first(network); gotone; gotone = network->next(network))
	{
		int curr = network->get_interface();
		const char *title = NULL;
		const netdev_entry_t *entry = netdev_first();
		while(entry) {
			if(entry->id==curr) {
				title = entry->description;
				break;
			}			
			entry = entry->m_next;
		}
		
		menu->item_append(network->device().tag(),  (title) ? title : "------", MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)network);		
	}
}

/*-------------------------------------------------
    ui_menu_network_devices - menu that
-------------------------------------------------*/

static void ui_menu_network_devices(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
	{
		menu_network_devices_populate(machine, menu, state);
	}

	/* process the menu */
	menu_event = menu->process(0);

	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT) {
			device_network_interface *network = (device_network_interface *)menu_event->itemref;
			int curr = network->get_interface();
			if (menu_event->iptkey == IPT_UI_LEFT) curr--; else curr++;
			if (curr==-2) curr = netdev_count() - 1;
			network->set_interface(curr);
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
		}
	}
}

/*-------------------------------------------------
    menu_main_populate - populate the main menu
-------------------------------------------------*/

static void menu_main_populate(running_machine &machine, ui_menu *menu, void *state)
{
	input_field_config *field;
	input_port_config *port;
	int has_configs = FALSE;
	int has_analog = FALSE;
	int has_dips = FALSE;
	astring menu_text;
	/* scan the input port array to see what options we need to enable */
	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
		for (field = port->fieldlist().first(); field != NULL; field = field->next())
		{
			if (field->type == IPT_DIPSWITCH)
				has_dips = TRUE;
			if (field->type == IPT_CONFIG)
				has_configs = TRUE;
			if (input_type_is_analog(field->type))
				has_analog = TRUE;
		}

	/* add input menu items */
	menu->item_append("Input (general)", NULL, 0, (void *)menu_input_groups);

	menu_text.printf("Input (this %s)",emulator_info::get_capstartgamenoun());
	menu->item_append(menu_text.cstr(), NULL, 0, (void *)menu_input_specific);

	/* add optional input-related menus */
	if (has_dips)
		menu->item_append("Dip Switches", NULL, 0, (void *)menu_settings_dip_switches);
	if (has_configs)
		menu->item_append("Driver Configuration", NULL, 0, (void *)menu_settings_driver_config);
	if (has_analog)
		menu->item_append("Analog Controls", NULL, 0, (void *)menu_analog);

	/* add bookkeeping menu */
	menu->item_append("Bookkeeping Info", NULL, 0, (void *)menu_bookkeeping);

	/* add game info menu */
	menu_text.printf("%s Information",emulator_info::get_capstartgamenoun());
	menu->item_append(menu_text.cstr(), NULL, 0, (void *)menu_game_info);

	device_image_interface *image = NULL;
	if (machine.devicelist().first(image))
	{
		/* add image info menu */
		menu->item_append("Image Information", NULL, 0, (void*)ui_image_menu_image_info);

		/* add file manager menu */
		menu->item_append("File Manager", NULL, 0, (void*)ui_image_menu_file_manager);

		/* add tape control menu */
		if (machine.devicelist().first(CASSETTE))
			menu->item_append("Tape Control", NULL, 0, (void*)ui_mess_menu_tape_control);

		/* add bitbanger control menu */
		if (machine.devicelist().first(BITBANGER))
			menu->item_append("Bitbanger Control", NULL, 0, (void*)ui_mess_menu_bitbanger_control);
	}

	device_slot_interface *slot = NULL;
	if (machine.devicelist().first(slot))
	{
		/* add image info menu */
		menu->item_append("Slot Devices", NULL, 0, (void*)ui_menu_slot_devices);
	}

	device_network_interface *network = NULL;
	if (machine.devicelist().first(network))
	{
		/* add image info menu */
		menu->item_append("Network Devices", NULL, 0, (void*)ui_menu_network_devices);
	}

	/* add keyboard mode menu */
	if (input_machine_has_keyboard(machine) && inputx_can_post(machine))
		menu->item_append("Keyboard Mode", NULL, 0, (void *)ui_menu_keyboard_mode);

	/* add sliders menu */
	menu->item_append("Slider Controls", NULL, 0, (void *)menu_sliders);

	/* add video options menu */
	menu->item_append("Video Options", NULL, 0, (machine.render().target_by_index(1) != NULL) ? (void *)menu_video_targets : (void *)menu_video_options);

	/* add crosshair options menu */
	if (crosshair_get_usage(machine))
		menu->item_append("Crosshair Options", NULL, 0, (void *)menu_crosshair);

	/* add cheat menu */
	if (machine.options().cheat() && machine.cheat().first() != NULL)
		menu->item_append("Cheat", NULL, 0, (void *)menu_cheat);

	/* add memory card menu */
	if (machine.config().m_memcard_handler != NULL)
		menu->item_append("Memory Card", NULL, 0, (void *)menu_memory_card);

	/* add reset and exit menus */
	menu_text.printf("Select New %s",emulator_info::get_capstartgamenoun());
	menu->item_append(menu_text.cstr(), NULL, 0, (void *)menu_select_game);
}


/*-------------------------------------------------
    menu_input_groups - handle the input groups
    menu
-------------------------------------------------*/

static void menu_input_groups(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_input_groups_populate(machine, menu, state);

	/* process the menu */
	menu_event = menu->process(0);
	if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT)
		ui_menu::stack_push(ui_menu_alloc(machine, menu->container, menu_input_general, menu_event->itemref));
}


/*-------------------------------------------------
    menu_input_groups_populate - populate the
    input groups menu
-------------------------------------------------*/

static void menu_input_groups_populate(running_machine &machine, ui_menu *menu, void *state)
{
	int player;

	/* build up the menu */
	menu->item_append("User Interface", NULL, 0, (void *)(IPG_UI + 1));
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		char buffer[40];
		sprintf(buffer, "Player %d Controls", player + 1);
		menu->item_append(buffer, NULL, 0, (void *)(FPTR)(IPG_PLAYER1 + player + 1));
	}
	menu->item_append("Other Controls", NULL, 0, (void *)(FPTR)(IPG_OTHER + 1));
}



/*-------------------------------------------------
    menu_input_general - handle the general
    input menu
-------------------------------------------------*/

static void menu_input_general(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	menu_input_common(machine, menu, parameter, state);
}


/*-------------------------------------------------
    menu_input_general_populate - populate the
    general input menu
-------------------------------------------------*/

static void menu_input_general_populate(running_machine &machine, ui_menu *menu, input_menu_state *menustate, int group)
{
	input_item_data *itemlist = NULL;
	int suborder[SEQ_TYPE_TOTAL];
	astring tempstring;
	int sortorder = 1;

	/* create a mini lookup table for sort order based on sequence type */
	suborder[SEQ_TYPE_STANDARD] = 0;
	suborder[SEQ_TYPE_DECREMENT] = 1;
	suborder[SEQ_TYPE_INCREMENT] = 2;

	/* iterate over the input ports and add menu items */
	for (input_type_entry *entry = input_type_list(machine).first(); entry != NULL; entry = entry->next())

		/* add if we match the group and we have a valid name */
		if (entry->group == group && entry->name != NULL && entry->name[0] != 0)
		{
			input_seq_type seqtype;

			/* loop over all sequence types */
			sortorder++;
			for (seqtype = SEQ_TYPE_STANDARD; seqtype < SEQ_TYPE_TOTAL; seqtype++)
			{
				/* build an entry for the standard sequence */
				input_item_data *item = (input_item_data *)menu->m_pool_alloc(sizeof(*item));
				memset(item, 0, sizeof(*item));
				item->ref = entry;
				item->seqtype = seqtype;
				item->seq = input_type_seq(machine, entry->type, entry->player, seqtype);
				item->defseq = &entry->defseq[seqtype];
				item->sortorder = sortorder * 4 + suborder[seqtype];
				item->type = input_type_is_analog(entry->type) ? (INPUT_TYPE_ANALOG + seqtype) : INPUT_TYPE_DIGITAL;
				item->name = entry->name;
				item->next = itemlist;
				itemlist = item;

				/* stop after one, unless we're analog */
				if (item->type == INPUT_TYPE_DIGITAL)
					break;
			}
		}

	/* sort and populate the menu in a standard fashion */
	menu_input_populate_and_sort(machine, menu, itemlist, menustate);
}


/*-------------------------------------------------
    menu_input_specific - handle the game-specific
    input menu
-------------------------------------------------*/

static void menu_input_specific(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	menu_input_common(machine, menu, NULL, state);
}


/*-------------------------------------------------
    menu_input_specific_populate - populate
    the input menu with game-specific items
-------------------------------------------------*/

static void menu_input_specific_populate(running_machine &machine, ui_menu *menu, input_menu_state *menustate)
{
	input_item_data *itemlist = NULL;
	input_field_config *field;
	input_port_config *port;
	int suborder[SEQ_TYPE_TOTAL];
	astring tempstring;

	/* create a mini lookup table for sort order based on sequence type */
	suborder[SEQ_TYPE_STANDARD] = 0;
	suborder[SEQ_TYPE_DECREMENT] = 1;
	suborder[SEQ_TYPE_INCREMENT] = 2;

	/* iterate over the input ports and add menu items */
	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
		for (field = port->fieldlist().first(); field != NULL; field = field->next())
		{
			const char *name = input_field_name(field);

			/* add if we match the group and we have a valid name */
			if (name != NULL && input_condition_true(machine, &field->condition, port->owner()) &&
				((field->type == IPT_OTHER && field->name != NULL) || input_type_group(machine, field->type, field->player) != IPG_INVALID))
			{
				input_seq_type seqtype;
				UINT16 sortorder;

				/* determine the sorting order */
				if (field->type >= IPT_START1 && field->type <= __ipt_analog_end)
					sortorder = (field->type << 2) | (field->player << 12);
				else
					sortorder = field->type | 0xf000;

				/* loop over all sequence types */
				for (seqtype = SEQ_TYPE_STANDARD; seqtype < SEQ_TYPE_TOTAL; seqtype++)
				{
					/* build an entry for the standard sequence */
					input_item_data *item = (input_item_data *)menu->m_pool_alloc(sizeof(*item));
					memset(item, 0, sizeof(*item));
					item->ref = field;
					item->seqtype = seqtype;
					item->seq = input_field_seq(field, seqtype);
					item->defseq = &get_field_default_seq(field, seqtype);
					item->sortorder = sortorder + suborder[seqtype];
					item->type = input_type_is_analog(field->type) ? (INPUT_TYPE_ANALOG + seqtype) : INPUT_TYPE_DIGITAL;
					item->name = name;
					item->next = itemlist;
					itemlist = item;

					/* stop after one, unless we're analog */
					if (item->type == INPUT_TYPE_DIGITAL)
						break;
				}
			}
		}

	/* sort and populate the menu in a standard fashion */
	menu_input_populate_and_sort(machine, menu, itemlist, menustate);
}


/*-------------------------------------------------
    menu_input - display a menu for inputs
-------------------------------------------------*/

static void menu_input_common(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	input_item_data *seqchangeditem = NULL;
	input_menu_state *menustate;
	const ui_menu_event *menu_event;
	int invalidate = FALSE;

	/* if no state, allocate now */
	if (state == NULL)
		state = menu->alloc_state(sizeof(*menustate), NULL);
	menustate = (input_menu_state *)state;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
	{
		if (parameter != NULL)
			menu_input_general_populate(machine, menu, menustate, (FPTR)parameter - 1);
		else
			menu_input_specific_populate(machine, menu, menustate);
	}

	/* process the menu */
	menu_event = menu->process((menustate->pollingitem != NULL) ? UI_MENU_PROCESS_NOKEYS : 0);

	/* if we are polling, handle as a special case */
	if (menustate->pollingitem != NULL)
	{
		input_item_data *item = menustate->pollingitem;
		input_seq newseq;

		/* if UI_CANCEL is pressed, abort */
		if (ui_input_pressed(machine, IPT_UI_CANCEL))
		{
			menustate->pollingitem = NULL;
			menustate->record_next = FALSE;
			toggle_none_default(item->seq, menustate->starting_seq, *item->defseq);
			seqchangeditem = item;
		}

		/* poll again; if finished, update the sequence */
		if (machine.input().seq_poll())
		{
			menustate->pollingitem = NULL;
			menustate->record_next = TRUE;
			item->seq = machine.input().seq_poll_final();
			seqchangeditem = item;
		}
	}

	/* otherwise, handle the events */
	else if (menu_event != NULL && menu_event->itemref != NULL)
	{
		input_item_data *item = (input_item_data *)menu_event->itemref;
		switch (menu_event->iptkey)
		{
			/* an item was selected: begin polling */
			case IPT_UI_SELECT:
				menustate->pollingitem = item;
				menustate->last_sortorder = item->sortorder;
				menustate->starting_seq = item->seq;
				machine.input().seq_poll_start((item->type == INPUT_TYPE_ANALOG) ? ITEM_CLASS_ABSOLUTE : ITEM_CLASS_SWITCH, menustate->record_next ? &item->seq : NULL);
				invalidate = TRUE;
				break;

			/* if the clear key was pressed, reset the selected item */
			case IPT_UI_CLEAR:
				toggle_none_default(item->seq, item->seq, *item->defseq);
				menustate->record_next = FALSE;
				seqchangeditem = item;
				break;
		}

		/* if the selection changed, reset the "record next" flag */
		if (item->sortorder != menustate->last_sortorder)
			menustate->record_next = FALSE;
		menustate->last_sortorder = item->sortorder;
	}

	/* if the sequence changed, update it */
	if (seqchangeditem != NULL)
	{
		/* update a general input */
		if (parameter != NULL)
		{
			const input_type_entry *entry = (const input_type_entry *)seqchangeditem->ref;
			input_type_set_seq(machine, entry->type, entry->player, seqchangeditem->seqtype, &seqchangeditem->seq);
		}

		/* update a game-specific input */
		else
		{
			input_field_user_settings settings;

			input_field_get_user_settings((input_field_config *)seqchangeditem->ref, &settings);
			settings.seq[seqchangeditem->seqtype] = seqchangeditem->seq;
			input_field_set_user_settings((input_field_config *)seqchangeditem->ref, &settings);
		}

		/* invalidate the menu to force an update */
		invalidate = TRUE;
	}

	/* if the menu is invalidated, clear it now */
	if (invalidate)
	{
		menustate->pollingref = NULL;
		if (menustate->pollingitem != NULL)
			menustate->pollingref = menustate->pollingitem->ref;
		menu->reset(UI_MENU_RESET_REMEMBER_POSITION);
	}
}


/*-------------------------------------------------
    menu_input_compare_items - compare two
    items for quicksort
-------------------------------------------------*/

static int menu_input_compare_items(const void *i1, const void *i2)
{
	const input_item_data * const *data1 = (const input_item_data * const *)i1;
	const input_item_data * const *data2 = (const input_item_data * const *)i2;
	if ((*data1)->sortorder < (*data2)->sortorder)
		return -1;
	if ((*data1)->sortorder > (*data2)->sortorder)
		return 1;
	return 0;
}


/*-------------------------------------------------
    menu_input_populate_and_sort - take a list
    of input_item_data objects and build up the
    menu from them
-------------------------------------------------*/

static void menu_input_populate_and_sort(running_machine &machine, ui_menu *menu, input_item_data *itemlist, input_menu_state *menustate)
{
	const char *nameformat[INPUT_TYPE_TOTAL] = { 0 };
	input_item_data **itemarray, *item;
	int numitems = 0, curitem;
	astring subtext;
	astring text;

	/* create a mini lookup table for name format based on type */
	nameformat[INPUT_TYPE_DIGITAL] = "%s";
	nameformat[INPUT_TYPE_ANALOG] = "%s Analog";
	nameformat[INPUT_TYPE_ANALOG_INC] = "%s Analog Inc";
	nameformat[INPUT_TYPE_ANALOG_DEC] = "%s Analog Dec";

	/* first count the number of items */
	for (item = itemlist; item != NULL; item = item->next)
		numitems++;

	/* now allocate an array of items and fill it up */
	itemarray = (input_item_data **)menu->m_pool_alloc(sizeof(*itemarray) * numitems);
	for (item = itemlist, curitem = 0; item != NULL; item = item->next)
		itemarray[curitem++] = item;

	/* sort it */
	qsort(itemarray, numitems, sizeof(*itemarray), menu_input_compare_items);

	/* build the menu */
	for (curitem = 0; curitem < numitems; curitem++)
	{
		UINT32 flags = 0;

		/* generate the name of the item itself, based off the base name and the type */
		item = itemarray[curitem];
		assert(nameformat[item->type] != NULL);
		text.printf(nameformat[item->type], item->name);

		/* if we're polling this item, use some spaces with left/right arrows */
		if (menustate->pollingref == item->ref)
		{
			subtext.cpy("   ");
			flags |= MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW;
		}

		/* otherwise, generate the sequence name and invert it if different from the default */
		else
		{
			machine.input().seq_name(subtext, item->seq);
			flags |= (item->seq != *item->defseq) ? MENU_FLAG_INVERT : 0;
		}

		/* add the item */
		menu->item_append(text, subtext, flags, item);
	}
}


/*-------------------------------------------------
    menu_settings_dip_switches - handle the DIP
    switches menu
-------------------------------------------------*/

static void menu_settings_dip_switches(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	menu_settings_common(machine, menu, state, IPT_DIPSWITCH);
}


/*-------------------------------------------------
    menu_settings_driver_config - handle the
    driver config menu
-------------------------------------------------*/

static void menu_settings_driver_config(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	menu_settings_common(machine, menu, state, IPT_CONFIG);
}


/*-------------------------------------------------
    menu_settings_common - handle one of the
    switches menus
-------------------------------------------------*/

static void menu_settings_common(running_machine &machine, ui_menu *menu, void *state, UINT32 type)
{
	settings_menu_state *menustate;
	const ui_menu_event *menu_event;

	/* if no state, allocate now */
	if (state == NULL)
		state = menu->alloc_state(sizeof(*menustate), NULL);
	menustate = (settings_menu_state *)state;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_settings_populate(machine, menu, menustate, type);

	/* process the menu */
	menu_event = menu->process(0);

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		input_field_config *field = (input_field_config *)menu_event->itemref;
		input_field_user_settings settings;
		int changed = FALSE;

		switch (menu_event->iptkey)
		{
			/* if selected, reset to default value */
			case IPT_UI_SELECT:
				input_field_get_user_settings(field, &settings);
				settings.value = field->defvalue;
				input_field_set_user_settings(field, &settings);
				changed = TRUE;
				break;

			/* left goes to previous setting */
			case IPT_UI_LEFT:
				input_field_select_previous_setting(field);
				changed = TRUE;
				break;

			/* right goes to next setting */
			case IPT_UI_RIGHT:
				input_field_select_next_setting(field);
				changed = TRUE;
				break;
		}

		/* if anything changed, rebuild the menu, trying to stay on the same field */
		if (changed)
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
	}
}


/*-------------------------------------------------
    menu_settings_populate - populate one of the
    switches menus
-------------------------------------------------*/

static void menu_settings_populate(running_machine &machine, ui_menu *menu, settings_menu_state *menustate, UINT32 type)
{
	input_field_config *field;
	input_port_config *port;
	dip_descriptor **diplist_tailptr;
	int dipcount = 0;

	/* reset the dip switch tracking */
	menustate->diplist = NULL;
	diplist_tailptr = &menustate->diplist;

	/* loop over input ports and set up the current values */
	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
		for (field = port->fieldlist().first(); field != NULL; field = field->next())
			if (field->type == type && input_condition_true(machine, &field->condition, port->owner()))
			{
				UINT32 flags = 0;

				/* set the left/right flags appropriately */
				if (input_field_has_previous_setting(field))
					flags |= MENU_FLAG_LEFT_ARROW;
				if (input_field_has_next_setting(field))
					flags |= MENU_FLAG_RIGHT_ARROW;

				/* add the menu item */
				menu->item_append(input_field_name(field), input_field_setting_name(field), flags, (void *)field);

				/* for DIP switches, build up the model */
				if (type == IPT_DIPSWITCH && field->diploclist().count() != 0)
				{
					const input_field_diplocation *diploc;
					input_field_user_settings settings;
					UINT32 accummask = field->mask;

					/* get current settings */
					input_field_get_user_settings(field, &settings);

					/* iterate over each bit in the field */
					for (diploc = field->diploclist().first(); diploc != NULL; diploc = diploc->next())
					{
						UINT32 mask = accummask & ~(accummask - 1);
						dip_descriptor *dip;

						/* find the matching switch name */
						for (dip = menustate->diplist; dip != NULL; dip = dip->next)
							if (strcmp(dip->name, diploc->swname) == 0)
								break;

						/* allocate new if none */
						if (dip == NULL)
						{
							dip = (dip_descriptor *)menu->m_pool_alloc(sizeof(*dip));
							dip->next = NULL;
							dip->name = diploc->swname;
							dip->mask = dip->state = 0;
							*diplist_tailptr = dip;
							diplist_tailptr = &dip->next;
							if (mame_stricmp(dip->name, "FAKE") != 0)
								dipcount++;
						}

						/* apply the bits */
						dip->mask |= 1 << (diploc->swnum - 1);
						if (((settings.value & mask) != 0 && !diploc->invert) || ((settings.value & mask) == 0 && diploc->invert))
							dip->state |= 1 << (diploc->swnum - 1);

						/* clear the relevant bit in the accumulated mask */
						accummask &= ~mask;
					}
				}
			}

	/* configure the extra menu */
	if (type == IPT_DIPSWITCH && menustate->diplist != NULL)
		menu->set_custom_render(menu_settings_custom_render, 0.0f, dipcount * (DIP_SWITCH_HEIGHT + DIP_SWITCH_SPACING) + DIP_SWITCH_SPACING);
}


/*-------------------------------------------------
    menu_settings_custom_render - perform our special
    rendering
-------------------------------------------------*/

static void menu_settings_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float x1, float y1, float x2, float y2)
{
	input_field_config *field = (input_field_config *)selectedref;
	settings_menu_state *menustate = (settings_menu_state *)state;
	dip_descriptor *dip;

	/* add borders */
	y1 = y2 + UI_BOX_TB_BORDER;
	y2 = y1 + bottom;

	/* draw extra menu area */
	ui_draw_outlined_box(&machine.render().ui_container(), x1, y1, x2, y2, UI_BACKGROUND_COLOR);
	y1 += (float)DIP_SWITCH_SPACING;

	/* iterate over DIP switches */
	for (dip = menustate->diplist; dip != NULL; dip = dip->next)
	{
		if (mame_stricmp(dip->name, "FAKE") != 0)
		{
			const input_field_diplocation *diploc;
			UINT32 selectedmask = 0;

			/* determine the mask of selected bits */
			if (field != NULL)
				for (diploc = field->diploclist().first(); diploc != NULL; diploc = diploc->next())
					if (strcmp(dip->name, diploc->swname) == 0)
						selectedmask |= 1 << (diploc->swnum - 1);

			/* draw one switch */
			menu_settings_custom_render_one(&machine.render().ui_container(), x1, y1, x2, y1 + DIP_SWITCH_HEIGHT, dip, selectedmask);
			y1 += (float)(DIP_SWITCH_SPACING + DIP_SWITCH_HEIGHT);
		}
	}
}


/*-------------------------------------------------
    menu_settings_custom_render_one - draw a single
    DIP switch
-------------------------------------------------*/

static void menu_settings_custom_render_one(render_container *container, float x1, float y1, float x2, float y2, const dip_descriptor *dip, UINT32 selectedmask)
{
	float switch_field_width = SINGLE_TOGGLE_SWITCH_FIELD_WIDTH * container->manager().ui_aspect();
	float switch_width = SINGLE_TOGGLE_SWITCH_WIDTH * container->manager().ui_aspect();
	int numtoggles, toggle;
	float switch_toggle_gap;
	float y1_off, y1_on;

	/* determine the number of toggles in the DIP */
	numtoggles = 32 - count_leading_zeros(dip->mask);

	/* center based on the number of switches */
	x1 += (x2 - x1 - numtoggles * switch_field_width) / 2;

	/* draw the dip switch name */
	ui_draw_text_full(	container,
						dip->name,
						0,
						y1 + (DIP_SWITCH_HEIGHT - UI_TARGET_FONT_HEIGHT) / 2,
						x1 - ui_get_string_width(container->manager().machine(), " "),
						JUSTIFY_RIGHT,
						WRAP_NEVER,
						DRAW_NORMAL,
						UI_TEXT_COLOR,
						PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA),
						NULL ,
						NULL);

	/* compute top and bottom for on and off positions */
	switch_toggle_gap = ((DIP_SWITCH_HEIGHT/2) - SINGLE_TOGGLE_SWITCH_HEIGHT)/2;
	y1_off = y1 + UI_LINE_WIDTH + switch_toggle_gap;
	y1_on = y1 + DIP_SWITCH_HEIGHT/2 + switch_toggle_gap;

	/* iterate over toggles */
	for (toggle = 0; toggle < numtoggles; toggle++)
	{
		float innerx1;

		/* first outline the switch */
		ui_draw_outlined_box(container, x1, y1, x1 + switch_field_width, y2, UI_BACKGROUND_COLOR);

		/* compute x1/x2 for the inner filled in switch */
		innerx1 = x1 + (switch_field_width - switch_width) / 2;

		/* see if the switch is actually used */
		if (dip->mask & (1 << toggle))
		{
			float innery1 = (dip->state & (1 << toggle)) ? y1_on : y1_off;
			container->add_rect(innerx1, innery1, innerx1 + switch_width, innery1 + SINGLE_TOGGLE_SWITCH_HEIGHT,
							   (selectedmask & (1 << toggle)) ? UI_DIPSW_COLOR : UI_TEXT_COLOR,
							   PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		}
		else
		{
			container->add_rect(innerx1, y1_off, innerx1 + switch_width, y1_on + SINGLE_TOGGLE_SWITCH_HEIGHT,
							   UI_UNAVAILABLE_COLOR,
							   PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		}

		/* advance to the next switch */
		x1 += switch_field_width;
	}
}


/*-------------------------------------------------
    menu_analog - handle the analog settings menu
-------------------------------------------------*/

static void menu_analog(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_analog_populate(machine, menu);

	/* process the menu */
	menu_event = menu->process(UI_MENU_PROCESS_LR_REPEAT);

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		analog_item_data *data = (analog_item_data *)menu_event->itemref;
		int newval = data->cur;

		switch (menu_event->iptkey)
		{
			/* if selected, reset to default value */
			case IPT_UI_SELECT:
				newval = data->defvalue;
				break;

			/* left decrements */
			case IPT_UI_LEFT:
				newval -= machine.input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;

			/* right increments */
			case IPT_UI_RIGHT:
				newval += machine.input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;
		}

		/* clamp to range */
		if (newval < data->min)
			newval = data->min;
		if (newval > data->max)
			newval = data->max;

		/* if things changed, update */
		if (newval != data->cur)
		{
			input_field_user_settings settings;

			/* get the settings and set the new value */
			input_field_get_user_settings(data->field, &settings);
			switch (data->type)
			{
				case ANALOG_ITEM_KEYSPEED:		settings.delta = newval;		break;
				case ANALOG_ITEM_CENTERSPEED:	settings.centerdelta = newval;	break;
				case ANALOG_ITEM_REVERSE:		settings.reverse = newval;		break;
				case ANALOG_ITEM_SENSITIVITY:	settings.sensitivity = newval;	break;
			}
			input_field_set_user_settings(data->field, &settings);

			/* rebuild the menu */
			menu->reset(UI_MENU_RESET_REMEMBER_POSITION);
		}
	}
}


/*-------------------------------------------------
    menu_analog_populate - populate the analog
    settings menu
-------------------------------------------------*/

static void menu_analog_populate(running_machine &machine, ui_menu *menu)
{
	input_field_config *field;
	input_port_config *port;
	astring subtext;
	astring text;

	/* loop over input ports and add the items */
	for (port = machine.m_portlist.first(); port != NULL; port = port->next())
		for (field = port->fieldlist().first(); field != NULL; field = field->next())
			if (input_type_is_analog(field->type) && input_condition_true(machine, &field->condition, port->owner()))
			{
				input_field_user_settings settings;
				int use_autocenter = FALSE;
				int type;

				/* based on the type, determine if we enable autocenter */
				switch (field->type)
				{
					case IPT_POSITIONAL:
					case IPT_POSITIONAL_V:
						if (field->flags & ANALOG_FLAG_WRAPS)
							break;

					case IPT_AD_STICK_X:
					case IPT_AD_STICK_Y:
					case IPT_AD_STICK_Z:
					case IPT_PADDLE:
					case IPT_PADDLE_V:
					case IPT_PEDAL:
					case IPT_PEDAL2:
					case IPT_PEDAL3:
						use_autocenter = TRUE;
						break;
				}

				/* get the user settings */
				input_field_get_user_settings(field, &settings);

				/* iterate over types */
				for (type = 0; type < ANALOG_ITEM_COUNT; type++)
					if (type != ANALOG_ITEM_CENTERSPEED || use_autocenter)
					{
						analog_item_data *data;
						UINT32 flags = 0;

						/* allocate a data item for tracking what this menu item refers to */
						data = (analog_item_data *)menu->m_pool_alloc(sizeof(*data));
						data->field = field;
						data->type = type;

						/* determine the properties of this item */
						switch (type)
						{
							default:
							case ANALOG_ITEM_KEYSPEED:
								text.printf("%s Digital Speed", input_field_name(field));
								subtext.printf("%d", settings.delta);
								data->min = 0;
								data->max = 255;
								data->cur = settings.delta;
								data->defvalue = field->delta;
								break;

							case ANALOG_ITEM_CENTERSPEED:
								text.printf("%s Autocenter Speed", input_field_name(field));
								subtext.printf("%d", settings.centerdelta);
								data->min = 0;
								data->max = 255;
								data->cur = settings.centerdelta;
								data->defvalue = field->centerdelta;
								break;

							case ANALOG_ITEM_REVERSE:
								text.printf("%s Reverse", input_field_name(field));
								subtext.cpy(settings.reverse ? "On" : "Off");
								data->min = 0;
								data->max = 1;
								data->cur = settings.reverse;
								data->defvalue = ((field->flags & ANALOG_FLAG_REVERSE) != 0);
								break;

							case ANALOG_ITEM_SENSITIVITY:
								text.printf("%s Sensitivity", input_field_name(field));
								subtext.printf("%d", settings.sensitivity);
								data->min = 1;
								data->max = 255;
								data->cur = settings.sensitivity;
								data->defvalue = field->sensitivity;
								break;
						}

						/* put on arrows */
						if (data->cur > data->min)
							flags |= MENU_FLAG_LEFT_ARROW;
						if (data->cur < data->max)
							flags |= MENU_FLAG_RIGHT_ARROW;

						/* append a menu item */
						menu->item_append(text, subtext, flags, data);
					}
			}
}


/*-------------------------------------------------
    menu_bookkeeping - handle the bookkeeping
    information menu
-------------------------------------------------*/

static void menu_bookkeeping(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	attotime *prevtime;
	attotime curtime;

	/* if no state, allocate some */
	if (state == NULL)
		state = menu->alloc_state(sizeof(*prevtime), NULL);
	prevtime = (attotime *)state;

	/* if the time has rolled over another second, regenerate */
	curtime = machine.time();
	if (prevtime->seconds != curtime.seconds)
	{
		menu->reset(UI_MENU_RESET_SELECT_FIRST);
		*prevtime = curtime;
		menu_bookkeeping_populate(machine, menu, prevtime);
	}

	/* process the menu */
	menu->process(0);
}


/*-------------------------------------------------
    menu_bookkeeping - handle the bookkeeping
    information menu
-------------------------------------------------*/

static void menu_bookkeeping_populate(running_machine &machine, ui_menu *menu, attotime *curtime)
{
	int tickets = get_dispensed_tickets(machine);
	astring tempstring;
	int ctrnum;

	/* show total time first */
	if (curtime->seconds >= 60 * 60)
		tempstring.catprintf("Uptime: %d:%02d:%02d\n\n", curtime->seconds / (60*60), (curtime->seconds / 60) % 60, curtime->seconds % 60);
	else
		tempstring.catprintf("Uptime: %d:%02d\n\n", (curtime->seconds / 60) % 60, curtime->seconds % 60);

	/* show tickets at the top */
	if (tickets > 0)
		tempstring.catprintf("Tickets dispensed: %d\n\n", tickets);

	/* loop over coin counters */
	for (ctrnum = 0; ctrnum < COIN_COUNTERS; ctrnum++)
	{
		int count = coin_counter_get_count(machine, ctrnum);

		/* display the coin counter number */
		tempstring.catprintf("Coin %c: ", ctrnum + 'A');

		/* display how many coins */
		if (count == 0)
			tempstring.cat("NA");
		else
			tempstring.catprintf("%d", count);

		/* display whether or not we are locked out */
		if (coin_lockout_get_state(machine, ctrnum))
			tempstring.cat(" (locked)");
		tempstring.cat("\n");
	}

	/* append the single item */
	menu->item_append(tempstring, NULL, MENU_FLAG_MULTILINE, NULL);
}


/*-------------------------------------------------
    menu_game_info - handle the game information
    menu
-------------------------------------------------*/

static void menu_game_info(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	/* if the menu isn't built, populate now */
	if (!menu->populated())
	{
		astring tempstring;
		menu->item_append(game_info_astring(machine, tempstring), NULL, MENU_FLAG_MULTILINE, NULL);
	}

	/* process the menu */
	menu->process(0);
}


/*-------------------------------------------------
    menu_cheat - handle the cheat menu
-------------------------------------------------*/

static void menu_cheat(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_cheat_populate(machine, menu);

	/* process the menu */
	menu_event = menu->process(UI_MENU_PROCESS_LR_REPEAT);

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		bool changed = false;

		/* clear cheat comment on any movement or keypress */
		popmessage(NULL);

		/* handle reset all + reset all cheats for reload all option */
		if ((FPTR)menu_event->itemref < 3 && menu_event->iptkey == IPT_UI_SELECT)
		{
			for (cheat_entry *curcheat = machine.cheat().first(); curcheat != NULL; curcheat = curcheat->next())
				if (curcheat->select_default_state())
					changed = true;
		}


		/* handle individual cheats */
		else if ((FPTR)menu_event->itemref > 2)
		{
			cheat_entry *curcheat = reinterpret_cast<cheat_entry *>(menu_event->itemref);
			const char *string;
			switch (menu_event->iptkey)
			{
				/* if selected, activate a oneshot */
				case IPT_UI_SELECT:
					changed = curcheat->activate();
					break;

				/* if cleared, reset to default value */
				case IPT_UI_CLEAR:
					changed = curcheat->select_default_state();
					break;

				/* left decrements */
				case IPT_UI_LEFT:
					changed = curcheat->select_previous_state();
					break;

				/* right increments */
				case IPT_UI_RIGHT:
					changed = curcheat->select_next_state();
					break;

				/* bring up display comment if one exists */
				case IPT_UI_DISPLAY_COMMENT:
				case IPT_UI_UP:
				case IPT_UI_DOWN:
					string = curcheat->comment();
					if (string != NULL && string[0] != 0)
						popmessage("Cheat Comment:\n%s", string);
					break;
			}
		}

		/* handle reload all  */
		if ((FPTR)menu_event->itemref == 2 && menu_event->iptkey == IPT_UI_SELECT)
		{
			/* re-init cheat engine and thus reload cheats/cheats have already been turned off by here */
			machine.cheat().reload();

			/* display the reloaded cheats */
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
			popmessage("All cheats reloaded");
		}

		/* if things changed, update */
		if (changed)
			menu->reset(UI_MENU_RESET_REMEMBER_REF);
	}
}


/*-------------------------------------------------
    menu_cheat_populate - populate the cheat menu
-------------------------------------------------*/

static void menu_cheat_populate(running_machine &machine, ui_menu *menu)
{
	/* iterate over cheats */
	astring text;
	astring subtext;
	for (cheat_entry *curcheat = machine.cheat().first(); curcheat != NULL; curcheat = curcheat->next())
	{
		UINT32 flags;
		curcheat->menu_text(text, subtext, flags);
		menu->item_append(text, subtext, flags, curcheat);
	}

	/* add a separator */
	menu->item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);

	/* add a reset all option */
	menu->item_append("Reset All", NULL, 0, (void *)1);

	/* add a reload all cheats option */
	menu->item_append("Reload All", NULL, 0, (void *)2);
}


/*-------------------------------------------------
    menu_memory_card - handle the memory card
    menu
-------------------------------------------------*/

static void menu_memory_card(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;
	int *cardnum;

	/* if no state, allocate some */
	if (state == NULL)
		state = menu->alloc_state(sizeof(*cardnum), NULL);
	cardnum = (int *)state;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_memory_card_populate(machine, menu, *cardnum);

	/* process the menu */
	menu_event = menu->process(UI_MENU_PROCESS_LR_REPEAT);

	/* if something was selected, act on it */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		FPTR item = (FPTR)menu_event->itemref;

		/* select executes actions on some of the items */
		if (menu_event->iptkey == IPT_UI_SELECT)
		{
			switch (item)
			{
				/* handle card loading; if we succeed, clear the menus */
				case MEMCARD_ITEM_LOAD:
					if (memcard_insert(menu->machine(), *cardnum) == 0)
					{
						popmessage("Memory card loaded");
						ui_menu::stack_reset(menu->machine());
					}
					else
						popmessage("Error loading memory card");
					break;

				/* handle card ejecting */
				case MEMCARD_ITEM_EJECT:
					memcard_eject(menu->machine());
					popmessage("Memory card ejected");
					break;

				/* handle card creating */
				case MEMCARD_ITEM_CREATE:
					if (memcard_create(menu->machine(), *cardnum, FALSE) == 0)
						popmessage("Memory card created");
					else
						popmessage("Error creating memory card\n(Card may already exist)");
					break;
			}
		}

		/* the select item has extra keys */
		else if (item == MEMCARD_ITEM_SELECT)
		{
			switch (menu_event->iptkey)
			{
				/* left decrements the card number */
				case IPT_UI_LEFT:
					*cardnum -= 1;
					menu->reset(UI_MENU_RESET_REMEMBER_REF);
					break;

				/* right decrements the card number */
				case IPT_UI_RIGHT:
					*cardnum += 1;
					menu->reset(UI_MENU_RESET_REMEMBER_REF);
					break;
			}
		}
	}
}


/*-------------------------------------------------
    menu_memory_card_populate - populate the
    memory card menu
-------------------------------------------------*/

static void menu_memory_card_populate(running_machine &machine, ui_menu *menu, int cardnum)
{
	char tempstring[20];
	UINT32 flags = 0;

	/* add the card select menu */
	sprintf(tempstring, "%d", cardnum);
	if (cardnum > 0)
		flags |= MENU_FLAG_LEFT_ARROW;
	if (cardnum < 1000)
		flags |= MENU_FLAG_RIGHT_ARROW;
	menu->item_append("Card Number:", tempstring, flags, (void *)MEMCARD_ITEM_SELECT);

	/* add the remaining items */
	menu->item_append("Load Selected Card", NULL, 0, (void *)MEMCARD_ITEM_LOAD);
	if (memcard_present(machine) != -1)
		menu->item_append("Eject Current Card", NULL, 0, (void *)MEMCARD_ITEM_EJECT);
	menu->item_append("Create New Card", NULL, 0, (void *)MEMCARD_ITEM_CREATE);
}


/*-------------------------------------------------
    menu_sliders - handle the sliders menu
-------------------------------------------------*/

static void menu_sliders(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	int menuless_mode = (parameter != NULL);
	const ui_menu_event *menu_event;
	UINT8 *hidden = (UINT8 *)state;

	/* if no state, allocate some */
	if (hidden == NULL)
		hidden = (UINT8 *)menu->alloc_state(sizeof(*hidden), NULL);
	if (menuless_mode)
		*hidden = TRUE;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_sliders_populate(machine, menu, menuless_mode);

	/* process the menu */
	menu_event = menu->process(UI_MENU_PROCESS_LR_REPEAT | (*hidden ? UI_MENU_PROCESS_CUSTOM_ONLY : 0));
	if (menu_event != NULL)
	{
		/* handle keys if there is a valid item selected */
		if (menu_event->itemref != NULL)
		{
			const slider_state *slider = (const slider_state *)menu_event->itemref;
			INT32 curvalue = (*slider->update)(machine, slider->arg, NULL, SLIDER_NOCHANGE);
			INT32 increment = 0;

			switch (menu_event->iptkey)
			{
				/* toggle visibility */
				case IPT_UI_ON_SCREEN_DISPLAY:
					if (menuless_mode)
						ui_menu::stack_pop(machine);
					else
						*hidden = !*hidden;
					break;

				/* decrease value */
				case IPT_UI_LEFT:
					if (machine.input().code_pressed(KEYCODE_LALT) || machine.input().code_pressed(KEYCODE_RALT))
						increment = -1;
					else if (machine.input().code_pressed(KEYCODE_LSHIFT) || machine.input().code_pressed(KEYCODE_RSHIFT))
						increment = (slider->incval > 10) ? -(slider->incval / 10) : -1;
					else if (machine.input().code_pressed(KEYCODE_LCONTROL) || machine.input().code_pressed(KEYCODE_RCONTROL))
						increment = -slider->incval * 10;
					else
						increment = -slider->incval;
					break;

				/* increase value */
				case IPT_UI_RIGHT:
					if (machine.input().code_pressed(KEYCODE_LALT) || machine.input().code_pressed(KEYCODE_RALT))
						increment = 1;
					else if (machine.input().code_pressed(KEYCODE_LSHIFT) || machine.input().code_pressed(KEYCODE_RSHIFT))
						increment = (slider->incval > 10) ? (slider->incval / 10) : 1;
					else if (machine.input().code_pressed(KEYCODE_LCONTROL) || machine.input().code_pressed(KEYCODE_RCONTROL))
						increment = slider->incval * 10;
					else
						increment = slider->incval;
					break;

				/* restore default */
				case IPT_UI_SELECT:
					increment = slider->defval - curvalue;
					break;
			}

			/* handle any changes */
			if (increment != 0)
			{
				INT32 newvalue = curvalue + increment;

				/* clamp within bounds */
				if (newvalue < slider->minval)
					newvalue = slider->minval;
				if (newvalue > slider->maxval)
					newvalue = slider->maxval;

				/* update the slider and recompute the menu */
				(*slider->update)(machine, slider->arg, NULL, newvalue);
				menu->reset(UI_MENU_RESET_REMEMBER_REF);
			}
		}

		/* if we are selecting an invalid item and we are hidden, skip to the next one */
		else if (*hidden)
		{
			/* if we got here via up or page up, select the previous item */
			if (menu_event->iptkey == IPT_UI_UP || menu_event->iptkey == IPT_UI_PAGE_UP)
			{
				menu->selected = (menu->selected + menu->numitems - 1) % menu->numitems;
				menu->validate_selection(-1);
			}

			/* otherwise select the next item */
			else if (menu_event->iptkey == IPT_UI_DOWN || menu_event->iptkey == IPT_UI_PAGE_DOWN)
			{
				menu->selected = (menu->selected + 1) % menu->numitems;
				menu->validate_selection(1);
			}
		}
	}
}


/*-------------------------------------------------
    menu_sliders_populate - populate the sliders
    menu
-------------------------------------------------*/

static void menu_sliders_populate(running_machine &machine, ui_menu *menu, int menuless_mode)
{
	const slider_state *curslider;
	astring tempstring;

	/* add all sliders */
	for (curslider = ui_get_slider_list(); curslider != NULL; curslider = curslider->next)
	{
		INT32 curval = (*curslider->update)(machine, curslider->arg, &tempstring, SLIDER_NOCHANGE);
		UINT32 flags = 0;
		if (curval > curslider->minval)
			flags |= MENU_FLAG_LEFT_ARROW;
		if (curval < curslider->maxval)
			flags |= MENU_FLAG_RIGHT_ARROW;
		menu->item_append(curslider->description, tempstring, flags, (void *)curslider);

		if (menuless_mode)
			break;
	}

	/* add all sliders */
	for (curslider = (slider_state*)machine.osd().get_slider_list(); curslider != NULL; curslider = curslider->next)
	{
		INT32 curval = (*curslider->update)(machine, curslider->arg, &tempstring, SLIDER_NOCHANGE);
		UINT32 flags = 0;
		if (curval > curslider->minval)
			flags |= MENU_FLAG_LEFT_ARROW;
		if (curval < curslider->maxval)
			flags |= MENU_FLAG_RIGHT_ARROW;
		menu->item_append(curslider->description, tempstring, flags, (void *)curslider);
	}

	menu->set_custom_render(menu_sliders_custom_render, 0.0f, 2.0f * ui_get_line_height(machine) + 2.0f * UI_BOX_TB_BORDER);
}


/*-------------------------------------------------
    menu_sliders_custom_render - perform our special
    rendering
-------------------------------------------------*/

static void menu_sliders_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float x1, float y1, float x2, float y2)
{
	const slider_state *curslider = (const slider_state *)selectedref;
	if (curslider != NULL)
	{
		float bar_left, bar_area_top, bar_width, bar_area_height, bar_top, bar_bottom, default_x, current_x;
		float line_height = ui_get_line_height(machine);
		float percentage, default_percentage;
		astring tempstring;
		float text_height;
		INT32 curval;

		/* determine the current value and text */
		curval = (*curslider->update)(machine, curslider->arg, &tempstring, SLIDER_NOCHANGE);

		/* compute the current and default percentages */
		percentage = (float)(curval - curslider->minval) / (float)(curslider->maxval - curslider->minval);
		default_percentage = (float)(curslider->defval - curslider->minval) / (float)(curslider->maxval - curslider->minval);

		/* assemble the the text */
		tempstring.ins(0, " ").ins(0, curslider->description);

		/* move us to the bottom of the screen, and expand to full width */
		y2 = 1.0f - UI_BOX_TB_BORDER;
		y1 = y2 - bottom;
		x1 = UI_BOX_LR_BORDER;
		x2 = 1.0f - UI_BOX_LR_BORDER;

		/* draw extra menu area */
		ui_draw_outlined_box(&machine.render().ui_container(), x1, y1, x2, y2, UI_BACKGROUND_COLOR);
		y1 += UI_BOX_TB_BORDER;

		/* determine the text height */
		ui_draw_text_full(&machine.render().ui_container(), tempstring, 0, 0, x2 - x1 - 2.0f * UI_BOX_LR_BORDER,
					JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NONE, ARGB_WHITE, ARGB_BLACK, NULL, &text_height);

		/* draw the thermometer */
		bar_left = x1 + UI_BOX_LR_BORDER;
		bar_area_top = y1;
		bar_width = x2 - x1 - 2.0f * UI_BOX_LR_BORDER;
		bar_area_height = line_height;

		/* compute positions */
		bar_top = bar_area_top + 0.125f * bar_area_height;
		bar_bottom = bar_area_top + 0.875f * bar_area_height;
		default_x = bar_left + bar_width * default_percentage;
		current_x = bar_left + bar_width * percentage;

		/* fill in the percentage */
		machine.render().ui_container().add_rect(bar_left, bar_top, current_x, bar_bottom, UI_SLIDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

		/* draw the top and bottom lines */
		machine.render().ui_container().add_line(bar_left, bar_top, bar_left + bar_width, bar_top, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		machine.render().ui_container().add_line(bar_left, bar_bottom, bar_left + bar_width, bar_bottom, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

		/* draw default marker */
		machine.render().ui_container().add_line(default_x, bar_area_top, default_x, bar_top, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
		machine.render().ui_container().add_line(default_x, bar_bottom, default_x, bar_area_top + bar_area_height, UI_LINE_WIDTH, UI_BORDER_COLOR, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

		/* draw the actual text */
		ui_draw_text_full(&machine.render().ui_container(), tempstring, x1 + UI_BOX_LR_BORDER, y1 + line_height, x2 - x1 - 2.0f * UI_BOX_LR_BORDER,
					JUSTIFY_CENTER, WRAP_WORD, DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, &text_height);
	}
}


/*-------------------------------------------------
    menu_video_targets - handle the video targets
    menu
-------------------------------------------------*/

static void menu_video_targets(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_video_targets_populate(machine, menu);

	/* process the menu */
	menu_event = menu->process(0);
	if (menu_event != NULL && menu_event->iptkey == IPT_UI_SELECT)
		ui_menu::stack_push(ui_menu_alloc(machine, menu->container, menu_video_options, menu_event->itemref));
}


/*-------------------------------------------------
    menu_video_targets_populate - populate the
    video targets menu
-------------------------------------------------*/

static void menu_video_targets_populate(running_machine &machine, ui_menu *menu)
{
	int targetnum;

	/* find the targets */
	for (targetnum = 0; ; targetnum++)
	{
		render_target *target = machine.render().target_by_index(targetnum);
		char buffer[40];

		/* stop when we run out */
		if (target == NULL)
			break;

		/* add a menu item */
		sprintf(buffer, "Screen #%d", targetnum);
		menu->item_append(buffer, NULL, 0, target);
	}
}


/*-------------------------------------------------
    menu_video_options - handle the video options
    menu
-------------------------------------------------*/

static void menu_video_options(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	render_target *target = (parameter != NULL) ? (render_target *)parameter : machine.render().first_target();
	const ui_menu_event *menu_event;
	int changed = FALSE;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_video_options_populate(machine, menu, target);

	/* process the menu */
	menu_event = menu->process(0);
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		switch ((FPTR)menu_event->itemref)
		{
			/* rotate adds rotation depending on the direction */
			case VIDEO_ITEM_ROTATE:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					int delta = (menu_event->iptkey == IPT_UI_LEFT) ? ROT270 : ROT90;
					target->set_orientation(orientation_add(delta, target->orientation()));
					if (target->is_ui_target())
					{
						render_container::user_settings settings;
						machine.render().ui_container().get_user_settings(settings);
						settings.m_orientation = orientation_add(delta ^ ROT180, settings.m_orientation);
						machine.render().ui_container().set_user_settings(settings);
					}
					changed = TRUE;
				}
				break;

			/* layer config bitmasks handle left/right keys the same (toggle) */
			case VIDEO_ITEM_BACKDROPS:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_backdrops_enabled(!target->backdrops_enabled());
					changed = TRUE;
				}
				break;

			case VIDEO_ITEM_OVERLAYS:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_overlays_enabled(!target->overlays_enabled());
					changed = TRUE;
				}
				break;

			case VIDEO_ITEM_BEZELS:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_bezels_enabled(!target->bezels_enabled());
					changed = TRUE;
				}
				break;

			case VIDEO_ITEM_CPANELS:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_cpanels_enabled(!target->cpanels_enabled());
					changed = TRUE;
				}
				break;

			case VIDEO_ITEM_MARQUEES:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_marquees_enabled(!target->marquees_enabled());
					changed = TRUE;
				}
				break;

			case VIDEO_ITEM_ZOOM:
				if (menu_event->iptkey == IPT_UI_LEFT || menu_event->iptkey == IPT_UI_RIGHT)
				{
					target->set_zoom_to_screen(!target->zoom_to_screen());
					changed = TRUE;
				}
				break;

			/* anything else is a view item */
			default:
				if (menu_event->iptkey == IPT_UI_SELECT && (int)(FPTR)menu_event->itemref >= VIDEO_ITEM_VIEW)
				{
					target->set_view((FPTR)menu_event->itemref - VIDEO_ITEM_VIEW);
					changed = TRUE;
				}
				break;
		}
	}

	/* if something changed, rebuild the menu */
	if (changed)
		menu->reset(UI_MENU_RESET_REMEMBER_REF);
}


/*-------------------------------------------------
    menu_video_options_populate - populate the
    video options menu
-------------------------------------------------*/

static void menu_video_options_populate(running_machine &machine, ui_menu *menu, render_target *target)
{
	const char *subtext = "";
	astring tempstring;
	int viewnum;
	int enabled;

	/* add items for each view */
	for (viewnum = 0; ; viewnum++)
	{
		const char *name = target->view_name(viewnum);
		if (name == NULL)
			break;

		/* create a string for the item, replacing underscores with spaces */
		tempstring.cpy(name).replace(0, "_", " ");
		menu->item_append(tempstring, NULL, 0, (void *)(FPTR)(VIDEO_ITEM_VIEW + viewnum));
	}

	/* add a separator */
	menu->item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);

	/* add a rotate item */
	switch (target->orientation())
	{
		case ROT0:		subtext = "None";					break;
		case ROT90:		subtext = "CW 90" UTF8_DEGREES; 	break;
		case ROT180:	subtext = "180" UTF8_DEGREES;		break;
		case ROT270:	subtext = "CCW 90" UTF8_DEGREES;	break;
	}
	menu->item_append("Rotate", subtext, MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_ROTATE);

	/* backdrop item */
	enabled = target->backdrops_enabled();
	menu->item_append("Backdrops", enabled ? "Enabled" : "Disabled", enabled ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_BACKDROPS);

	/* overlay item */
	enabled = target->overlays_enabled();
	menu->item_append("Overlays", enabled ? "Enabled" : "Disabled", enabled ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_OVERLAYS);

	/* bezel item */
	enabled = target->bezels_enabled();
	menu->item_append("Bezels", enabled ? "Enabled" : "Disabled", enabled ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_BEZELS);

	/* cpanel item */
	enabled = target->cpanels_enabled();
	menu->item_append("CPanels", enabled ? "Enabled" : "Disabled", enabled ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_CPANELS);

	/* marquee item */
	enabled = target->marquees_enabled();
	menu->item_append("Marquees", enabled ? "Enabled" : "Disabled", enabled ? MENU_FLAG_LEFT_ARROW : MENU_FLAG_RIGHT_ARROW, (void *)VIDEO_ITEM_MARQUEES);

	/* cropping */
	enabled = target->zoom_to_screen();
	menu->item_append("View", enabled ? "Cropped" : "Full", enabled ? MENU_FLAG_RIGHT_ARROW : MENU_FLAG_LEFT_ARROW, (void *)VIDEO_ITEM_ZOOM);
}


/*-------------------------------------------------
    menu_crosshair - handle the crosshair settings
    menu
-------------------------------------------------*/

static void menu_crosshair(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	const ui_menu_event *menu_event;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_crosshair_populate(machine, menu);

	/* process the menu */
	menu_event = menu->process(UI_MENU_PROCESS_LR_REPEAT);

	/* handle events */
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		crosshair_user_settings settings;
		crosshair_item_data *data = (crosshair_item_data *)menu_event->itemref;
		int changed = FALSE;
		//int set_def = FALSE;
		int newval = data->cur;

		/* retreive the user settings */
		crosshair_get_user_settings(machine, data->player, &settings);

		switch (menu_event->iptkey)
		{
			/* if selected, reset to default value */
			case IPT_UI_SELECT:
				newval = data->defvalue;
				//set_def = TRUE;
				break;

			/* left decrements */
			case IPT_UI_LEFT:
				newval -= machine.input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;

			/* right increments */
			case IPT_UI_RIGHT:
				newval += machine.input().code_pressed(KEYCODE_LSHIFT) ? 10 : 1;
				break;
		}

		/* clamp to range */
		if (newval < data->min)
			newval = data->min;
		if (newval > data->max)
			newval = data->max;

		/* if things changed, update */
		if (newval != data->cur)
		{
			switch (data->type)
			{
				/* visibility state */
				case CROSSHAIR_ITEM_VIS:
					settings.mode = newval;
					changed = TRUE;
					break;

				/* auto time */
				case CROSSHAIR_ITEM_AUTO_TIME:
					settings.auto_time = newval;
					changed = TRUE;
					break;
			}
		}

		/* crosshair graphic name */
		if (data->type == CROSSHAIR_ITEM_PIC)
		{
			if (menu_event->iptkey == IPT_UI_SELECT)
			{
				/* clear the name string to reset to default crosshair */
				settings.name[0] = 0;
				changed = TRUE;
			}
			else if (menu_event->iptkey == IPT_UI_LEFT)
			{
				strcpy(settings.name, data->last_name);
				changed = TRUE;
			}
			else if (menu_event->iptkey == IPT_UI_RIGHT)
			{
				strcpy(settings.name, data->next_name);
				changed = TRUE;
			}
		}

		if (changed)
		{
			/* save the user settings */
			crosshair_set_user_settings(machine, data->player, &settings);

			/* rebuild the menu */
			menu->reset(UI_MENU_RESET_REMEMBER_POSITION);
		}
	}
}


/*-------------------------------------------------
    menu_crosshair_populate - populate the
    crosshair settings menu
-------------------------------------------------*/

static void menu_crosshair_populate(running_machine &machine, ui_menu *menu)
{
	crosshair_user_settings settings;
	crosshair_item_data *data;
	char temp_text[16];
	int player;
	UINT8 use_auto = FALSE;
	UINT32 flags = 0;

	/* loop over player and add the manual items */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		/* get the user settings */
		crosshair_get_user_settings(machine, player, &settings);

		/* add menu items for usable crosshairs */
		if (settings.used)
		{
			/* Make sure to keep these matched to the CROSSHAIR_VISIBILITY_xxx types */
			static const char *const vis_text[] = { "Off", "On", "Auto" };

			/* track if we need the auto time menu */
			if (settings.mode == CROSSHAIR_VISIBILITY_AUTO) use_auto = TRUE;

			/* CROSSHAIR_ITEM_VIS - allocate a data item and fill it */
			data = (crosshair_item_data *)menu->m_pool_alloc(sizeof(*data));
			data->type = CROSSHAIR_ITEM_VIS;
			data->player = player;
			data->min = CROSSHAIR_VISIBILITY_OFF;
			data->max = CROSSHAIR_VISIBILITY_AUTO;
			data->defvalue = CROSSHAIR_VISIBILITY_DEFAULT;
			data->cur = settings.mode;

			/* put on arrows */
			if (data->cur > data->min)
				flags |= MENU_FLAG_LEFT_ARROW;
			if (data->cur < data->max)
				flags |= MENU_FLAG_RIGHT_ARROW;

			/* add CROSSHAIR_ITEM_VIS menu */
			sprintf(temp_text, "P%d Visibility", player + 1);
			menu->item_append(temp_text, vis_text[settings.mode], flags, data);

			/* CROSSHAIR_ITEM_PIC - allocate a data item and fill it */
			data = (crosshair_item_data *)menu->m_pool_alloc(sizeof(*data));
			data->type = CROSSHAIR_ITEM_PIC;
			data->player = player;
			data->last_name[0] = 0;
			/* other data item not used by this menu */

			/* search for crosshair graphics */

			/* open a path to the crosshairs */
			file_enumerator path(machine.options().crosshair_path());
			const osd_directory_entry *dir;
			/* reset search flags */
			int using_default = FALSE;
			int finished = FALSE;
			int found = FALSE;

			/* if we are using the default, then we just need to find the first in the list */
			if (strlen(settings.name) == 0)
				using_default = TRUE;

			/* look for the current name, then remember the name before */
			/* and find the next name */
			while (((dir = path.next()) != NULL) && !finished)
			{
				int length = strlen(dir->name);

				/* look for files ending in .png with a name not larger then 9 chars*/
				if ((length > 4) && (length <= CROSSHAIR_PIC_NAME_LENGTH + 4) &&
					dir->name[length - 4] == '.' &&
					tolower((UINT8)dir->name[length - 3]) == 'p' &&
					tolower((UINT8)dir->name[length - 2]) == 'n' &&
					tolower((UINT8)dir->name[length - 1]) == 'g')

				{
					/* remove .png from length */
					length -= 4;

					if (found || using_default)
					{
						/* get the next name */
						strncpy(data->next_name, dir->name, length);
						data->next_name[length] = 0;
						finished = TRUE;
					}
					else if (!strncmp(dir->name, settings.name, length))
					{
						/* we found the current name */
						/* so loop once more to find the next name */
						found = TRUE;
					}
					else
						/* remember last name */
						/* we will do it here in case files get added to the directory */
					{
						strncpy(data->last_name, dir->name, length);
						data->last_name[length] = 0;
					}
				}
			}
			/* if name not found then next item is DEFAULT */
			if (!found && !using_default)
			{
				data->next_name[0] = 0;
				finished = TRUE;
			}
			/* setup the selection flags */
			flags = 0;
			if (finished)
				flags |= MENU_FLAG_RIGHT_ARROW;
			if (found)
				flags |= MENU_FLAG_LEFT_ARROW;

			/* add CROSSHAIR_ITEM_PIC menu */
			sprintf(temp_text, "P%d Crosshair", player + 1);
			menu->item_append(temp_text, using_default ? "DEFAULT" : settings.name, flags, data);
		}
	}
	if (use_auto)
	{
		/* any player can be used to get the autotime */
		crosshair_get_user_settings(machine, 0, &settings);

		/* CROSSHAIR_ITEM_AUTO_TIME - allocate a data item and fill it */
		data = (crosshair_item_data *)menu->m_pool_alloc(sizeof(*data));
		data->type = CROSSHAIR_ITEM_AUTO_TIME;
		data->min = CROSSHAIR_VISIBILITY_AUTOTIME_MIN;
		data->max = CROSSHAIR_VISIBILITY_AUTOTIME_MAX;
		data->defvalue = CROSSHAIR_VISIBILITY_AUTOTIME_DEFAULT;
		data->cur = settings.auto_time;

		/* put on arrows in visible menu */
		if (data->cur > data->min)
			flags |= MENU_FLAG_LEFT_ARROW;
		if (data->cur < data->max)
			flags |= MENU_FLAG_RIGHT_ARROW;

		/* add CROSSHAIR_ITEM_AUTO_TIME menu */
		sprintf(temp_text, "%d", settings.auto_time);
		menu->item_append("Visible Delay", temp_text, flags, data);
	}
//  else
//      /* leave a blank filler line when not in auto time so size does not rescale */
//      menu->item_append("", "", NULL, NULL);
}


/*-------------------------------------------------
    menu_quit_game - handle the "menu" for
    quitting the game
-------------------------------------------------*/

static void menu_quit_game(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	/* request a reset */
	machine.schedule_exit();

	/* reset the menu stack */
	ui_menu::stack_reset(machine);
}


/*-------------------------------------------------
    menu_select_game - handle the game select
    menu
-------------------------------------------------*/

static void menu_select_game(running_machine &machine, ui_menu *menu, void *parameter, void *state)
{
	select_game_state *menustate;
	const ui_menu_event *menu_event;

	/* if no state, allocate some */
	if (state == NULL)
	{
		state = menu->alloc_state(sizeof(*menustate) + sizeof(menustate->driverlist) * driver_list::total(), NULL);
		if (parameter != NULL)
			strcpy(((select_game_state *)state)->search, (const char *)parameter);
		((select_game_state *)state)->matchlist[0] = -1;
	}
	menustate = (select_game_state *)state;

	/* if the menu isn't built, populate now */
	if (!menu->populated())
		menu_select_game_populate(machine, menu, menustate);

	/* ignore pause keys by swallowing them before we process the menu */
	ui_input_pressed(machine, IPT_UI_PAUSE);

	/* process the menu */
	menu_event = menu->process(0);
	if (menu_event != NULL && menu_event->itemref != NULL)
	{
		/* reset the error on any future menu_event */
		if (menustate->error)
			menustate->error = FALSE;

		/* handle selections */
		else if (menu_event->iptkey == IPT_UI_SELECT)
		{
			const game_driver *driver = (const game_driver *)menu_event->itemref;

			/* special case for configure inputs */
			if ((FPTR)driver == 1)
				ui_menu::stack_push(ui_menu_alloc(menu->machine(), menu->container, menu_input_groups, NULL));

			/* anything else is a driver */
			else
			{
				// audit the game first to see if we're going to work
				driver_enumerator enumerator(machine.options(), *driver);
				enumerator.next();
				media_auditor auditor(enumerator);
				media_auditor::summary summary = auditor.audit_media(AUDIT_VALIDATE_FAST);

				// if everything looks good, schedule the new driver
				if (summary == media_auditor::CORRECT || summary == media_auditor::BEST_AVAILABLE)
				{
					machine.schedule_new_driver(*driver);
					ui_menu::stack_reset(machine);
				}

				// otherwise, display an error
				else
				{
					menu->reset(UI_MENU_RESET_REMEMBER_REF);
					menustate->error = TRUE;
				}
			}
		}

		/* escape pressed with non-empty text clears the text */
		else if (menu_event->iptkey == IPT_UI_CANCEL && menustate->search[0] != 0)
		{
			/* since we have already been popped, we must recreate ourself from scratch */
			ui_menu::stack_push(ui_menu_alloc(menu->machine(), menu->container, menu_select_game, NULL));
		}

		/* typed characters append to the buffer */
		else if (menu_event->iptkey == IPT_SPECIAL)
		{
			int buflen = strlen(menustate->search);

			/* if it's a backspace and we can handle it, do so */
			if ((menu_event->unichar == 8 || menu_event->unichar == 0x7f) && buflen > 0)
			{
				*(char *)utf8_previous_char(&menustate->search[buflen]) = 0;
				menustate->rerandomize = TRUE;
				menu->reset(UI_MENU_RESET_SELECT_FIRST);
			}

			/* if it's any other key and we're not maxed out, update */
			else if (menu_event->unichar >= ' ' && menu_event->unichar < 0x7f)
			{
				buflen += utf8_from_uchar(&menustate->search[buflen], ARRAY_LENGTH(menustate->search) - buflen, menu_event->unichar);
				menustate->search[buflen] = 0;
				menu->reset(UI_MENU_RESET_SELECT_FIRST);
			}
		}
	}

	/* if we're in an error state, overlay an error message */
	if (menustate->error)
		ui_draw_text_box(&machine.render().ui_container(),
						 "The selected game is missing one or more required ROM or CHD images. "
		                 "Please select a different game.\n\nPress any key to continue.",
		                 JUSTIFY_CENTER, 0.5f, 0.5f, UI_RED_COLOR);
}


/*-------------------------------------------------
    menu_select_game_populate - populate the game
    select menu
-------------------------------------------------*/

static void menu_select_game_populate(running_machine &machine, ui_menu *menu, select_game_state *menustate)
{
	int matchcount;
	int curitem;

	/* update our driver list if necessary */
	if (menustate->driverlist[0] == NULL)
		menu_select_game_build_driver_list(menu, menustate);
	for (curitem = matchcount = 0; menustate->driverlist[curitem] != NULL && matchcount < VISIBLE_GAMES_IN_LIST; curitem++)
		if (!(menustate->driverlist[curitem]->flags & GAME_NO_STANDALONE))
			matchcount++;

	/* if nothing there, add a single multiline item and return */
	if (matchcount == 0)
	{
		astring txt;
		txt.printf("No %s found. Please check the rompath specified in the %s.ini file.\n\n"
					"If this is your first time using %s, please see the config.txt file in "
					"the docs directory for information on configuring %s.",
					emulator_info::get_gamesnoun(),
					emulator_info::get_configname(),
					emulator_info::get_appname(),emulator_info::get_appname() );
		menu->item_append(txt.cstr(), NULL, MENU_FLAG_MULTILINE | MENU_FLAG_REDTEXT, NULL);
		return;
	}

	/* otherwise, rebuild the match list */
	assert(drivlist != NULL);
	if (menustate->search[0] != 0 || menustate->matchlist[0] == -1 || menustate->rerandomize)
		drivlist->find_approximate_matches(menustate->search, matchcount, menustate->matchlist);
	menustate->rerandomize = FALSE;

	/* iterate over entries */
	for (curitem = 0; curitem < matchcount; curitem++)
	{
		int curmatch = menustate->matchlist[curitem];
		if (curmatch != -1)
		{
			int cloneof = drivlist->non_bios_clone(curmatch);
			menu->item_append(drivlist->driver(curmatch).name, drivlist->driver(curmatch).description, (cloneof == -1) ? 0 : MENU_FLAG_INVERT, (void *)&drivlist->driver(curmatch));
		}
	}

	/* if we're forced into this, allow general input configuration as well */
	if (ui_menu::stack_has_special_main_menu())
	{
		menu->item_append(MENU_SEPARATOR_ITEM, NULL, 0, NULL);
		menu->item_append("Configure General Inputs", NULL, 0, (void *)1);
	}

	/* configure the custom rendering */
	menu->set_custom_render(menu_select_game_custom_render, ui_get_line_height(machine) + 3.0f * UI_BOX_TB_BORDER, 4.0f * ui_get_line_height(machine) + 3.0f * UI_BOX_TB_BORDER);
}


/*-------------------------------------------------
    menu_select_game_build_driver_list - build a
    list of available drivers
-------------------------------------------------*/

static void menu_select_game_build_driver_list(ui_menu *menu, select_game_state *menustate)
{
	// start with an empty list
	// hack alert: use new directly here to avoid reporting this one-time static memory as unfreed
	if (drivlist == NULL)
		drivlist = global_alloc(driver_enumerator(menu->machine().options()));
	drivlist->exclude_all();

	/* open a path to the ROMs and find them in the array */
	file_enumerator path(menu->machine().options().media_path());
	const osd_directory_entry *dir;

	/* iterate while we get new objects */
	while ((dir = path.next()) != NULL)
	{
		char drivername[50];
		char *dst = drivername;
		const char *src;

		/* build a name for it */
		for (src = dir->name; *src != 0 && *src != '.' && dst < &drivername[ARRAY_LENGTH(drivername) - 1]; src++)
			*dst++ = tolower((UINT8)*src);
		*dst = 0;

		int drivnum = drivlist->find(drivername);
		if (drivnum != -1)
			drivlist->include(drivnum);
	}

	/* now build the final list */
	drivlist->reset();
	int listnum = 0;
	while (drivlist->next())
		menustate->driverlist[listnum++] = &drivlist->driver();

	/* NULL-terminate */
	menustate->driverlist[listnum] = NULL;
}


/*-------------------------------------------------
    menu_select_game_custom_render - perform our
    special rendering
-------------------------------------------------*/

static void menu_select_game_custom_render(running_machine &machine, ui_menu *menu, void *state, void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	select_game_state *menustate = (select_game_state *)state;
	const game_driver *driver;
	float width, maxwidth;
	float x1, y1, x2, y2;
	char tempbuf[4][256];
	rgb_t color;
	int line;

	/* display the current typeahead */
	if (menustate->search[0] != 0)
		sprintf(&tempbuf[0][0], "Type name or select: %s_", menustate->search);
	else
		sprintf(&tempbuf[0][0], "Type name or select: (random)");

	/* get the size of the text */
	ui_draw_text_full(&machine.render().ui_container(), &tempbuf[0][0], 0.0f, 0.0f, 1.0f, JUSTIFY_CENTER, WRAP_TRUNCATE,
					  DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &width, NULL);
	width += 2 * UI_BOX_LR_BORDER;
	maxwidth = MAX(width, origx2 - origx1);

	/* compute our bounds */
	x1 = 0.5f - 0.5f * maxwidth;
	x2 = x1 + maxwidth;
	y1 = origy1 - top;
	y2 = origy1 - UI_BOX_TB_BORDER;

	/* draw a box */
	ui_draw_outlined_box(&machine.render().ui_container(), x1, y1, x2, y2, UI_BACKGROUND_COLOR);

	/* take off the borders */
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	/* draw the text within it */
	ui_draw_text_full(&machine.render().ui_container(), &tempbuf[0][0], x1, y1, x2 - x1, JUSTIFY_CENTER, WRAP_TRUNCATE,
					  DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, NULL);

	/* determine the text to render below */
	driver = ((FPTR)selectedref > 1) ? (const game_driver *)selectedref : NULL;
	if ((FPTR)driver > 1)
	{
		const char *gfxstat, *soundstat;

		/* first line is game name */
		sprintf(&tempbuf[0][0], "%-.100s", driver->description);

		/* next line is year, manufacturer */
		sprintf(&tempbuf[1][0], "%s, %-.100s", driver->year, driver->manufacturer);

		/* next line is overall driver status */
		if (driver->flags & GAME_NOT_WORKING)
			strcpy(&tempbuf[2][0], "Overall: NOT WORKING");
		else if (driver->flags & GAME_UNEMULATED_PROTECTION)
			strcpy(&tempbuf[2][0], "Overall: Unemulated Protection");
		else
			strcpy(&tempbuf[2][0], "Overall: Working");

		/* next line is graphics, sound status */
		if (driver->flags & (GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_IMPERFECT_COLORS))
			gfxstat = "Imperfect";
		else
			gfxstat = "OK";

		if (driver->flags & GAME_NO_SOUND)
			soundstat = "Unimplemented";
		else if (driver->flags & GAME_IMPERFECT_SOUND)
			soundstat = "Imperfect";
		else
			soundstat = "OK";

		sprintf(&tempbuf[3][0], "Gfx: %s, Sound: %s", gfxstat, soundstat);
	}
	else
	{
		const char *s = emulator_info::get_copyright();
		line = 0;
		int col = 0;

		/* first line is version string */
		sprintf(&tempbuf[line++][0], "%s %s", emulator_info::get_applongname(), build_version);

		/* output message */
		while (line < ARRAY_LENGTH(tempbuf))
		{
			if (*s == 0 || *s == '\n')
			{
				tempbuf[line++][col] = 0;
				col = 0;
			}
			else
				tempbuf[line][col++] = *s;

			if (*s != 0)
				s++;
		}
	}

	/* get the size of the text */
	maxwidth = origx2 - origx1;
	for (line = 0; line < 4; line++)
	{
		ui_draw_text_full(&machine.render().ui_container(), &tempbuf[line][0], 0.0f, 0.0f, 1.0f, JUSTIFY_CENTER, WRAP_TRUNCATE,
						  DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &width, NULL);
		width += 2 * UI_BOX_LR_BORDER;
		maxwidth = MAX(maxwidth, width);
	}

	/* compute our bounds */
	x1 = 0.5f - 0.5f * maxwidth;
	x2 = x1 + maxwidth;
	y1 = origy2 + UI_BOX_TB_BORDER;
	y2 = origy2 + bottom;

	/* draw a box */
	color = UI_BACKGROUND_COLOR;
	if (driver != NULL)
		color = UI_GREEN_COLOR;
	if (driver != NULL && (driver->flags & (GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_IMPERFECT_COLORS | GAME_NO_SOUND | GAME_IMPERFECT_SOUND)) != 0)
		color = UI_YELLOW_COLOR;
	if (driver != NULL && (driver->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) != 0)
		color = UI_RED_COLOR;
	ui_draw_outlined_box(&machine.render().ui_container(), x1, y1, x2, y2, color);

	/* take off the borders */
	x1 += UI_BOX_LR_BORDER;
	x2 -= UI_BOX_LR_BORDER;
	y1 += UI_BOX_TB_BORDER;
	y2 -= UI_BOX_TB_BORDER;

	/* draw all lines */
	for (line = 0; line < 4; line++)
	{
		ui_draw_text_full(&machine.render().ui_container(), &tempbuf[line][0], x1, y1, x2 - x1, JUSTIFY_CENTER, WRAP_TRUNCATE,
						  DRAW_NORMAL, UI_TEXT_COLOR, UI_TEXT_BG_COLOR, NULL, NULL);
		y1 += ui_get_line_height(machine);
	}
}



