/***************************************************************************

    uimenu.h

    Internal MAME menus for the user interface.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __UIMENU_H__
#define __UIMENU_H__

#include "mamecore.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* flags for menu items */
#define MENU_FLAG_LEFT_ARROW	(1 << 0)
#define MENU_FLAG_RIGHT_ARROW	(1 << 1)
#define MENU_FLAG_INVERT		(1 << 2)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef UINT32 (*ui_menu_handler)(UINT32 state);

typedef struct _ui_menu_item ui_menu_item;
struct _ui_menu_item
{
   const char *text;
   const char *subtext;
   UINT32 flags;
   void *ref;
};


typedef struct _menu_augment_routines menu_augment_routines;
struct _menu_augment_routines
{
	void (*render_augmentation_menu)(float x, float y, float x2, float y2);
	float (*get_augmentation_menu_height)(void);
	float (*get_augmentation_menu_width)(void);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* initialization */
void ui_menu_init(running_machine *machine);

/* draw a menu */
void ui_menu_draw(const ui_menu_item *items, int numitems, int selected, menu_augment_routines *augmentation_routines);

/* master handler */
UINT32 ui_menu_ui_handler(UINT32 state);

/* menu keyboard handling */
int ui_menu_generic_keys(UINT32 *selected, int num_items);

/* menu stack management */
void ui_menu_stack_reset(void);
UINT32 ui_menu_stack_push(ui_menu_handler new_handler, UINT32 new_state);
UINT32 ui_menu_stack_pop(void);


#endif	/* __UIMENU_H__ */
