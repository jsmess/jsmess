//============================================================
//
//  debugosx.c - MacOS X debug window handling
//
//  Copyright (c) 1996-2008, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// NOTES:
//
// debug_window_proc is called for events on the debugger windows: Must install carbon handler on it
// debugwin_view_proc is called for events on the debugger controls: Must install carbon handler on it
//


#define OSX_DEBUGGER_FONT_NAME		"Monaco"
#define OSX_DEBUGGER_FONT_SIZE		10

#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050
#define OSX_LEOPARD 1
#else
#define OSX_LEOPARD 0
#endif

// standard windows headers
#include <Carbon/Carbon.h>

// MAME headers
#include <SDL/SDL.h>
#include "driver.h"
#include "debug/debugvw.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"
#include "debugger.h"
#include "uiinput.h"

// MAMEOS headers
#include "debugwin.h"
#include "window.h"
#include "video.h"
#include "config.h"

#define OSX_POPUP_COMMAND		'popu'
#define OSX_BASE_ID	    		'cmd0'

enum
{
	KEY_F1			= 0x7A,
	KEY_F2			= 0x78,
	KEY_F3			= 0x63,
	KEY_F4			= 0x76,
	KEY_F5			= 0x60,
	KEY_F6			= 0x61,
	KEY_F7			= 0x62,
	KEY_F8			= 0x64,
	KEY_F9			= 0x65,
	KEY_F10       		= 0x6D,
	KEY_F11			= 0x67,
	KEY_F12       		= 0x6F,
	KEY_F13			= 0x69,
	KEY_F14			= 0x6B,
	KEY_F15			= 0x71
};


//============================================================
//  PARAMETERS
//============================================================

#define MAX_VIEWS				4
#define EDGE_WIDTH				3
#define MAX_EDIT_STRING			256
#define HISTORY_LENGTH			20
#define MAX_OTHER_WND			4

// debugger window styles
#define DEBUG_WINDOW_STYLE		( kWindowCloseBoxAttribute | kWindowCollapseBoxAttribute | kWindowStandardHandlerAttribute | kWindowCompositingAttribute | kWindowResizableAttribute | kWindowLiveResizeAttribute )

// debugger window events
static EventTypeSpec debug_window_events[] =
{
	{ kEventClassWindow, kEventWindowActivated },
	{ kEventClassWindow, kEventWindowGetMinimumSize },
	{ kEventClassWindow, kEventWindowGetMaximumSize },
	{ kEventClassWindow, kEventWindowBoundsChanging },
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowDispose },
	{ kEventClassCommand, kEventCommandProcess }
};

// debugger view events
static EventTypeSpec debug_view_events[] =
{
	{ kEventClassControl, kEventControlDraw },
	{ kEventClassControl, kEventControlClick },
	{ kEventClassControl, kEventControlContextualMenuClick },
	{ kEventClassControl, kEventControlSetFocusPart },
	{ kEventClassKeyboard, kEventRawKeyDown },
	{ kEventClassKeyboard, kEventRawKeyRepeat },
	{ kEventClassMouse, kEventMouseWheelMoved }
};

// debugger keyboard events
static EventTypeSpec debug_keyboard_events[] =
{
	{ kEventClassKeyboard, kEventRawKeyDown },
	{ kEventClassKeyboard, kEventRawKeyRepeat }
};

enum
{
	ID_NEW_MEMORY_WND = OSX_BASE_ID,
	ID_NEW_DISASM_WND,
	ID_NEW_LOG_WND,
	ID_RUN,
	ID_RUN_AND_HIDE,
	ID_RUN_VBLANK,
	ID_RUN_IRQ,
	ID_NEXT_CPU,
	ID_STEP,
	ID_STEP_OVER,
	ID_STEP_OUT,
	ID_HARD_RESET,
	ID_SOFT_RESET,
	ID_EXIT,

	ID_1_BYTE_CHUNKS,
	ID_2_BYTE_CHUNKS,
	ID_4_BYTE_CHUNKS,
	ID_8_BYTE_CHUNKS,
	ID_LOGICAL_ADDRESSES,
	ID_PHYSICAL_ADDRESSES,
	ID_REVERSE_VIEW,
	ID_INCREASE_MEM_WIDTH,
	ID_DECREASE_MEM_WIDTH,

	ID_SHOW_RAW,
	ID_SHOW_ENCRYPTED,
	ID_SHOW_COMMENTS,
	ID_RUN_TO_CURSOR,
	ID_TOGGLE_BREAKPOINT
};



//============================================================
//  TYPES
//============================================================

typedef struct _debugview_info debugview_info;
typedef struct _debugwin_info debugwin_info;


struct _debugview_info
{
	debugwin_info *			owner;
	debug_view *			view;
	HIViewRef				wnd;
	HIViewRef				hscroll;
	HIViewRef				vscroll;
	UINT8					is_textbuf;
};


struct _debugwin_info
{
	debugwin_info *			next;
	WindowRef				wnd;
	HIViewRef				focuswnd;
//	WNDPROC					handler;

	UINT32					minwidth, maxwidth;
	UINT32					minheight, maxheight;
	void					(*recompute_children)(debugwin_info *);

	int						(*handle_command)(debugwin_info *, EventRef);
	int						(*handle_key)(debugwin_info *, EventRef);
	UINT16					ignore_char_lparam;

	debugview_info			view[MAX_VIEWS];

	HIViewRef				editwnd;
	HIViewRef				menuwnd;
	char					edit_defstr[256];
	void					(*process_string)(debugwin_info *, const char *);
//	WNDPROC 				original_editproc;
	char					history[HISTORY_LENGTH][MAX_EDIT_STRING];
	int						history_count;
	int						last_history;

	HIViewRef				otherwnd[MAX_OTHER_WND];
	
	MenuRef					contextualMenu;
	HIViewRef				contentView;

	running_machine				*machine;
};


//============================================================
//  GLOBAL VARIABLES
//============================================================



//============================================================
//  LOCAL VARIABLES
//============================================================

static debugwin_info *window_list;
static debugwin_info *main_console = NULL;

static UINT8 debugger_active_countdown;
static UINT8 waiting_for_debugger;

static UINT32 debug_font_height;
static UINT32 debug_font_width;
static UINT32 debug_font_ascent;

static UINT32 hscroll_height;
static UINT32 vscroll_width;

static int temporarily_fake_that_we_are_not_visible;

static EventHandlerRef keyEventHandler = NULL;

static int osx_inited_debugger = 0;

//============================================================
//  PROTOTYPES
//============================================================

static int debugwin_init_windows(running_machine *machine);
static debugwin_info *debug_window_create(running_machine *machine, const char * title, void *unused);
static void debug_window_free(debugwin_info *info);
static OSStatus debug_window_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData);

static void debugwin_view_draw_contents(debugview_info *view, CGContextRef dc);
static debugview_info *debug_view_find(debug_view *view);
static OSStatus debugwin_view_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData);
static void debugwin_view_update(debug_view *view, void *osdprivate);
static int debugwin_view_create(debugwin_info *info, int which, int type);
static void debugwin_view_process_scroll(HIViewRef inView, ControlPartCode partCode);

static OSStatus debugwin_edit_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData);

#if 0
static void generic_create_window(int type);
#endif
static void generic_recompute_children(debugwin_info *info);

static void memory_create_window(running_machine *machine);
static void memory_recompute_children(debugwin_info *info);
static void memory_process_string(debugwin_info *info, const char *string);
static void memory_update_checkmarks(debugwin_info *info);
static int memory_handle_command(debugwin_info *info, EventRef inEvent);
static int memory_handle_key(debugwin_info *info, EventRef inEvent);
static void memory_update_caption(debugwin_info *info);

static void disasm_create_window(running_machine *machine);
static void disasm_recompute_children(debugwin_info *info);
static void disasm_process_string(debugwin_info *info, const char *string);
static void disasm_update_checkmarks(debugwin_info *info);
static int disasm_handle_command(debugwin_info *info, EventRef inEvent);
static int disasm_handle_key(debugwin_info *info, EventRef inEvent);
static void disasm_update_caption(debugwin_info *info);

static void console_create_window(running_machine *machine);
static void console_recompute_children(debugwin_info *info);
static void console_process_string(debugwin_info *info, const char *string);
static void console_set_cpu(const device_config *device);

static MenuRef create_standard_menubar(void);
static int global_handle_command(debugwin_info *info, EventRef inEvent);
static int global_handle_key(debugwin_info *info, EventRef inEvent);
static void smart_set_view_bounds(HIViewRef wnd, HIRect *bounds);
static void smart_show_window(WindowRef wnd, int show);
static void smart_show_view(HIViewRef wnd, int show);
static void smart_show_all(int show);

//============================================================
//  OSX stubs
//============================================================

INLINE void osx_init_debugger(running_machine *machine)
{
	if ( osx_inited_debugger == 0 )
	{
		if ( debugwin_init_windows(machine) != 0 )
		{
			mame_printf_error( "Could not init debugger\n" );
			exit( -1 );
		}
		osx_inited_debugger = 1;
	}
}


static void osx_SetFocus( HIViewRef inView )
{
	HIViewRef	focus;
	WindowRef	parent = HIViewGetWindow( inView );

	if ( GetKeyboardFocus(parent, &focus) == noErr )
	{
		if ( focus == inView )
			return;
	}

	SetKeyboardFocus(parent, inView, kControlFocusNextPart);
}

static void osx_GlobalToViewPoint( HIViewRef inView, const HIPoint * inGlobalPoint, HIPoint * outViewPoint )
{
	Rect		windBounds;
	
	GetWindowBounds( GetControlOwner( inView ), kWindowStructureRgn, &windBounds );

	*outViewPoint = *inGlobalPoint;
	
	outViewPoint->x -= windBounds.left;
	outViewPoint->y -= windBounds.top;
	
	HIViewConvertPoint( outViewPoint, NULL, inView );
}

static void osx_ViewToGlobalPoint( HIViewRef inView, const HIPoint * inViewPoint, HIPoint * outGlobalPoint )
{
	Rect		windBounds;
	
	GetWindowBounds( GetControlOwner( inView ), kWindowStructureRgn, &windBounds );
	
	*outGlobalPoint = *inViewPoint;
	
	HIViewConvertPoint( outGlobalPoint, inView, NULL );

	outGlobalPoint->x += windBounds.left;
	outGlobalPoint->y += windBounds.top;
}

INLINE void osx_RectToHIRect( const Rect *inRect, HIRect *outRect )
{
	outRect->origin.x = inRect->left;
	outRect->origin.y = inRect->top;
	outRect->size.width = inRect->right - inRect->left;
	outRect->size.height = inRect->bottom - inRect->top;
}

//============================================================
//  osd_wait_for_debugger
//============================================================

void osd_wait_for_debugger(const device_config *device, int firststop)
{
	EventRef		message;
	EventTargetRef	target = GetEventDispatcherTarget();

	osx_init_debugger(device->machine);

	// create a console window
	if (!main_console)
		console_create_window(device->machine);

	// update the views in the console to reflect the current CPU
	if (main_console)
		console_set_cpu(device);

	// make sure the debug windows are visible
	waiting_for_debugger = TRUE;
	smart_show_all(TRUE);
	
	// get and process messages
	if (ReceiveNextEvent(0, NULL, kEventDurationForever, TRUE, &message) == noErr)
	{
		SendEventToEventTarget(message, target);
		ReleaseEvent(message);
	}

	// mark the debugger as active
	debugger_active_countdown = 2;
	waiting_for_debugger = FALSE;
}



//============================================================
//  debugwin_init_windows
//============================================================

static int debugwin_init_windows(running_machine *machine)
{
	SInt32		val;

	// get the font metrics
	{
		ATSFontMetrics		metrics;
		ATSFontRef			fontid = ATSFontFindFromName( CFSTR(OSX_DEBUGGER_FONT_NAME), kATSOptionFlagsDefault );
	
		if (!fontid)
			return 1;
		
		if ( ATSFontGetHorizontalMetrics( fontid, kATSOptionFlagsDefault, &metrics ) != noErr )
			return 1;
		
		debug_font_width = metrics.maxAdvanceWidth * (float)OSX_DEBUGGER_FONT_SIZE;
		debug_font_height = ( metrics.ascent + -metrics.descent ) * (float)OSX_DEBUGGER_FONT_SIZE;
		debug_font_ascent = metrics.ascent * (float)OSX_DEBUGGER_FONT_SIZE;
	}

	// get other metrics
	hscroll_height = 15;
	vscroll_width = 15;
		
	if ( GetThemeMetric( kThemeMetricScrollBarWidth, &val ) == noErr )
	{
		hscroll_height = val;
		vscroll_width = val;
	}

	return 0;
}



#if 0
//============================================================
//  debugwin_destroy_windows
//============================================================

void debugwin_destroy_windows(void)
{
	// loop over windows and free them
	while (window_list)
		DisposeWindow(window_list->wnd);

	// free the combobox info
	while (memorycombo)
	{
		void *temp = memorycombo;
		memorycombo = memorycombo->next;
		free(temp);
	}

	main_console = NULL;
}



//============================================================
//  debugwin_show
//============================================================

void debugwin_show(int type)
{
	debugwin_info *info;

	// loop over windows and show/hide them
	for (info = window_list; info; info = info->next)
		ShowWindow(info->wnd, type);
}

#endif

//============================================================
//  debugwin_update_during_game
//============================================================

void debugwin_update_during_game(running_machine *machine)
{
	osx_init_debugger(machine);

	if (!machine->debugcpu_data) return;

	// if we're running live, do some checks
	if ( ! debug_cpu_is_stopped(machine) )
	{
		// see if the interrupt key is pressed and break if it is
		temporarily_fake_that_we_are_not_visible = TRUE;
		if (ui_input_pressed(machine, IPT_UI_DEBUG_BREAK))
		{
			debugwin_info *info;
			HIViewRef	focuswnd;
			
			if ( GetKeyboardFocus( GetFrontWindowOfClass( kDocumentWindowClass, TRUE ), &focuswnd ) != noErr )
			{
				focuswnd = NULL;
			}
			
			debug_cpu_halt_on_next_instruction(debug_cpu_get_visible_cpu(machine), "User-initiated break\n");

			// if we were focused on some window's edit box, reset it to default
			for (info = window_list; info; info = info->next)
				if (focuswnd == info->editwnd)
				{
					ControlEditTextSelectionRec	sel;
					
					sel.selStart = 0;
					sel.selEnd = strlen(info->edit_defstr);
				
					SetControlData(focuswnd, kControlEntireControl, kControlEditTextTextTag, strlen(info->edit_defstr), info->edit_defstr);
					SetControlData(focuswnd, kControlEntireControl, kControlEditTextSelectionTag, sizeof( sel ), &sel );
				}
		}
		temporarily_fake_that_we_are_not_visible = FALSE;
	}
}


//============================================================
//  debug_window_create
//============================================================

static debugwin_info *debug_window_create(running_machine *machine, const char *title, void *unused)
{
	CGDirectDisplayID	mainID = CGMainDisplayID();

	debugwin_info *info = NULL;
	Rect work_bounds;

	(void)unused;

	// allocate memory
	info = malloc(sizeof(*info));
	if (!info)
		return NULL;
	memset(info, 0, sizeof(*info));

	info->wnd = NULL;
	info->machine = machine;

	// create the window
	work_bounds.top = work_bounds.left = 0;
	work_bounds.bottom = work_bounds.right = 100;

	if ( CreateNewWindow(kDocumentWindowClass, DEBUG_WINDOW_STYLE, &work_bounds, &info->wnd) != noErr )
		goto cleanup;

	// install keyboard handler
	if ( keyEventHandler == NULL )
	{
		if ( InstallEventHandler( GetApplicationEventTarget(), NewEventHandlerUPP(debug_window_proc), sizeof( debug_keyboard_events ) / sizeof( EventTypeSpec ), debug_keyboard_events, info, &keyEventHandler ) != noErr )
			goto cleanup;
	}

	// setup event handler
	if ( InstallEventHandler( GetWindowEventTarget( info->wnd ), NewEventHandlerUPP(debug_window_proc), sizeof( debug_window_events ) / sizeof( EventTypeSpec ), debug_window_events, info, NULL ) != noErr )
		goto cleanup;
	
	// find the content view, and install draw handler
	{
		EventTypeSpec	spec = { kEventClassControl, kEventControlDraw };
	
		if ( HIViewFindByID( HIViewGetRoot( info->wnd ), kHIViewWindowContentID, &info->contentView ) != noErr )
			goto cleanup;
	
		if ( InstallEventHandler( GetControlEventTarget( info->contentView ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL ) != noErr )
			goto cleanup;
	}
	
	// fill in some defaults
	{
#if OSX_LEOPARD
		HIRect		mainBounds;
		HIWindowGetAvailablePositioningBounds( mainID, kHICoordSpaceScreenPixel, &mainBounds );
		work_bounds.left = mainBounds.origin.x;
		work_bounds.top = mainBounds.origin.y;
		work_bounds.right = mainBounds.origin.x + mainBounds.size.width;
		work_bounds.bottom = mainBounds.origin.y + mainBounds.size.height;
#else
		GDHandle	mainDevice;
		DMGetGDeviceByDisplayID((DisplayIDType)mainID, &mainDevice, TRUE);
		GetAvailableWindowPositioningBounds(mainDevice, &work_bounds);
#endif
	}
	info->minwidth = 200;
	info->minheight = 200;
	info->maxwidth = work_bounds.right - work_bounds.left;
	info->maxheight = work_bounds.bottom - work_bounds.top;

	// set the default handlers
	info->handle_command = global_handle_command;
	info->handle_key = global_handle_key;
	strcpy(info->edit_defstr, "");

	// hook us in
	info->next = window_list;
	window_list = info;
	return info;

cleanup:
	if (info->wnd)
		DisposeWindow(info->wnd);
	free(info);
	return NULL;
}



//============================================================
//  debug_window_free
//============================================================

static void debug_window_free(debugwin_info *info)
{
	debugwin_info *prev, *curr;

	// first unlink us from the list
	for (curr = window_list, prev = NULL; curr; prev = curr, curr = curr->next)
		if (curr == info)
		{
			if (prev)
				prev->next = curr->next;
			else
				window_list = curr->next;
			break;
		}

#if 0
	{
		int viewnum;

		// free any views
		for (viewnum = 0; viewnum < MAX_VIEWS; viewnum++)
			if (info->view[viewnum].view)
			{
				debug_view_free(info->view[viewnum].view);
				info->view[viewnum].view = NULL;
			}
	}
#endif

	// free our memory
	free(info);
}



//============================================================
//  debug_window_draw_contents
//============================================================

static void debug_window_draw_contents(debugwin_info *info, CGContextRef dc)
{
	HIRect bounds;
//	int curview, curwnd;

	// find the content view
	HIViewRef		content = info->contentView;
	
	CGContextSaveGState(dc);
	
	// fill the background with light gray
	HIViewGetBounds(content, &bounds);
	CGContextSetRGBFillColor(dc, 0.75, 0.75, 0.75, 1.0);
	CGContextFillRect(dc,bounds);

#if 0
	// draw edges around all views
	for (curview = 0; curview < MAX_VIEWS; curview++)
		if (info->view[curview].wnd)
		{
			HIViewGetFrame(info->view[curview].wnd, &bounds);
			bounds = CGRectInset(bounds, -EDGE_WIDTH, -EDGE_WIDTH);
			CGContextSetRGBStrokeColor(dc, 0.0, 0.0, 0.0, 1.0);
			CGContextStrokeRect(dc,bounds);
		}

	// draw edges around all children
	if (info->editwnd)
	{
		HIViewGetFrame(info->editwnd, &bounds);
		bounds = CGRectInset(bounds, -EDGE_WIDTH, -EDGE_WIDTH);
		CGContextSetRGBStrokeColor(dc, 0.0, 0.0, 0.0, 1.0);
		CGContextStrokeRect(dc,bounds);
	}

	for (curwnd = 0; curwnd < MAX_OTHER_WND; curwnd++)
		if (info->otherwnd[curwnd])
		{
			HIViewGetFrame(info->otherwnd[curwnd], &bounds);
			bounds = CGRectInset(bounds, -EDGE_WIDTH, -EDGE_WIDTH);
			CGContextSetRGBStrokeColor(dc, 0.0, 0.0, 0.0, 1.0);
			CGContextStrokeRect(dc,bounds);
		}
#endif
	CGContextRestoreGState(dc);
}



//============================================================
//  debug_window_proc
//============================================================

static OSStatus debug_window_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData)
{
	debugwin_info *info = (debugwin_info *)inUserData;
	UInt32			eventClass = GetEventClass( inEvent );
	UInt32			eventKind = GetEventKind( inEvent );
	
	// handle a few messages
	if ( eventClass == kEventClassControl && eventKind == kEventControlDraw )
	{
		CGContextRef	dc;
				
		if ( GetEventParameter( inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(dc), NULL, &dc ) != noErr )
		{
			mame_printf_error( "Compositing attribute not set!\n" );
			exit( -1 );
		}
		
		debug_window_draw_contents(info, dc);
		return noErr;
		// don't return noErr, so the rest of the views can be updated
	}
	else if ( eventClass == kEventClassWindow )
	{
		switch( eventKind )
		{
			case kEventWindowActivated:
			{
				if ( info->focuswnd != NULL )
				{
					osx_SetFocus( info->focuswnd );
				}
				return noErr;
			}
			break;
			
			case kEventWindowGetMinimumSize:
			case kEventWindowGetMaximumSize:
			{
				Point	pt;
				
				if ( eventKind == kEventWindowGetMinimumSize )
				{
					pt.h = info->minwidth;
					pt.v = info->minheight;
				}
				else
				{
					pt.h = info->maxwidth;
					pt.v = info->maxheight;
				}
			
				SetEventParameter( inEvent, kEventParamDimensions, typeQDPoint, sizeof( pt ), &pt );
				return noErr;
			}
			break;
			
			case kEventWindowBoundsChanging:
			{
				Rect	prev, cur;
			
				if ( GetEventParameter( inEvent, kEventParamPreviousBounds, typeQDRectangle, NULL, sizeof(prev), NULL, &prev ) == noErr )
				{
					if ( GetEventParameter( inEvent, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(cur), NULL, &cur ) == noErr )
					{
						Point	old, new;
						
						old.h = prev.right - prev.left; old.v = prev.bottom - prev.top;
						new.h = cur.right - cur.left; new.v = cur.bottom - cur.top;
						
						if ( old.h != new.h || old.v != new.v )
						{
							if (info->recompute_children)
							{
								(*info->recompute_children)(info);
							}
						}
					}
				}
				
				return noErr;
			}
			break;
			
			// TODO: mousewheel support
			
			case kEventWindowClose:
			{
				if (main_console && main_console->wnd == info->wnd)
				{
					smart_show_all(FALSE);
					debug_cpu_go(info->machine, ~0);
					return noErr;
				}
				
				// return not handled, so the system calls DisposeWindow() on it's own
				return eventNotHandledErr;
			}
			break;
			
			case kEventWindowDispose:
			{
				debug_window_free(info);
				return noErr;
			}
			break;
		}
	}
	else if ( eventClass == kEventClassCommand && eventKind == kEventCommandProcess )
	{
		if ( (*info->handle_command)(info, inEvent) )
			return noErr;
	}
	else if ( eventClass == kEventClassKeyboard && ( eventKind == kEventRawKeyDown || eventKind == kEventRawKeyRepeat ) )
	{
		WindowRef	win = GetFrontWindowOfClass( kDocumentWindowClass, TRUE );
		
		for (info = window_list; info; info = info->next)
		{
			if ( info->wnd == win )
			{
				if ( IsWindowActive( win ) )
				{
					HIViewRef		focusChild;
					
					// Get the current control focus
					if ( GetKeyboardFocus( win, &focusChild ) != noErr )
					{
						focusChild = NULL;
					}
								
					// window gets first dibs to the event
					if ( (*info->handle_key)(info, inEvent) )
						return noErr;
				
					// then any of the childs
					if ( focusChild != NULL )
					{
						int		curview;

						for (curview = 0; curview < MAX_VIEWS; curview++)
						{
							if (info->view[curview].wnd == focusChild )
							{
								return debugwin_view_proc( inHandler, inEvent, &info->view[curview] );
							}
						}
					}
				}
			}
		}
	}
	
	return eventNotHandledErr;
}



//============================================================
//  debugwin_view_create
//============================================================

static int debugwin_view_create(debugwin_info *info, int which, int type)
{
	Rect	bounds;
	debugview_info *view = &info->view[which];

	// set the owner
	view->owner = info;

	bounds.top = bounds.left = 0;
	bounds.right = bounds.bottom = 100;

	// create the child view
	if ( CreateUserPaneControl(info->wnd, &bounds, 0, &view->wnd) != noErr )
		goto cleanup;
	
	if ( InstallEventHandler( GetControlEventTarget( view->wnd ), NewEventHandlerUPP(debugwin_view_proc), sizeof( debug_view_events ) / sizeof( EventTypeSpec ), debug_view_events, view, NULL ) != noErr )
		goto cleanup;
	
	// create the scroll bars
	bounds.top = bounds.bottom - hscroll_height;
	
	if ( CreateScrollBarControl(info->wnd, &bounds, 0, 0, 0, 0, TRUE, NewControlActionUPP(debugwin_view_process_scroll), &view->hscroll ) != noErr )
		goto cleanup;
	
	SetControlProperty(view->hscroll, 'MAME', 'info', sizeof( view ), &view );
	
	bounds.top = 0;
	bounds.left = bounds.right - vscroll_width;
	
	if ( CreateScrollBarControl(info->wnd, &bounds, 0, 0, 0, 0, TRUE, NewControlActionUPP(debugwin_view_process_scroll), &view->vscroll ) != noErr )
		goto cleanup;
	
	SetControlProperty(view->vscroll, 'MAME', 'info', sizeof( view ), &view );

	// create the debug view
	view->view = debug_view_alloc(info->machine, type, debugwin_view_update, view);
	if (!view->view)
		goto cleanup;

	return 1;

cleanup:
	if (view->view)
		debug_view_free(view->view);
	if (view->hscroll)
		DisposeControl(view->hscroll);
	if (view->vscroll)
		DisposeControl(view->vscroll);
	if (view->wnd)
		DisposeControl(view->wnd);
	return 0;
}



//============================================================
//  debugwin_view_set_bounds
//============================================================

static void debugwin_view_set_bounds(debugview_info *info, const Rect *newbounds)
{
	// account for the edges and set the bounds
	if (info->wnd)
	{
		HIRect		bounds;
		
		bounds.origin.x = newbounds->left;
		bounds.origin.y = newbounds->top;
		bounds.size.width = newbounds->right - newbounds->left;
		bounds.size.height = newbounds->bottom - newbounds->top;
		
		smart_set_view_bounds(info->wnd, &bounds);
	}

	// update
	debugwin_view_update(info->view, info);
}



//============================================================
//  debugwin_view_draw_contents
//============================================================

static void debugwin_view_draw_contents(debugview_info *view, CGContextRef dc)
{
	const debug_view_char *viewdata = debug_view_get_chars(view->view);
	debug_view_xy visarea = debug_view_get_visible_size(view->view);
	UINT32 col, row;
	HIRect client;

	CGContextSaveGState(dc);

	// get the client rect
	HIViewGetBounds(view->wnd,&client);
	
	if (!viewdata) return;

	// set the font
	CGContextSelectFont(dc, OSX_DEBUGGER_FONT_NAME, OSX_DEBUGGER_FONT_SIZE, kCGEncodingMacRoman);
	CGContextSetTextDrawingMode(dc,kCGTextFill);

	// Set the text transform
	{
		CGAffineTransform		mtx = CGAffineTransformIdentity;
		
		mtx.d = -mtx.d;
				
		CGContextSetTextMatrix(dc, mtx);
	}

	// iterate over rows and columns
	for (row = 0; row < visarea.y; row++)
	{
		int iter;

		// loop twice; once to fill the background and once to draw the text
		for (iter = 0; iter < 2; iter++)
		{
			float	fgcolor[4] = { 0, 0, 0, 1.0 };
			float	bgcolor[4] = { 1.0, 1.0, 1.0, 1.0 };
			int		last_attrib = -1;
			char	buffer[256];
			int		count = 0;
			Rect	bounds;

			// initialize the text bounds
			bounds.left = bounds.right = 0;
			bounds.top = row * debug_font_height;
			bounds.bottom = bounds.top + debug_font_height;

			// iterate over columns
			for (col = 0; col < visarea.x; col++)
			{
				// if the attribute changed, adjust the colors
				if (viewdata[col].attrib != last_attrib)
				{
					float oldbg[4];
					
					memcpy( oldbg, bgcolor, sizeof( oldbg ) );

					// reset to standard colors
					fgcolor[0] = fgcolor[1] = fgcolor[2] = 0;
					bgcolor[0] = bgcolor[1] = bgcolor[2] = 1.0;

					// pick new fg/bg colors
					if (viewdata[col].attrib & DCA_ANCILLARY) { bgcolor[0] = bgcolor[1] = bgcolor[2] = 0.875; }
					if (viewdata[col].attrib & DCA_SELECTED) { bgcolor[0] = 1.0; bgcolor[1] = bgcolor[2] = 0.5; }
					if (viewdata[col].attrib & DCA_CURRENT) { bgcolor[0] = bgcolor[1] = 1.0; bgcolor[2] = 0; }
					if ((viewdata[col].attrib & DCA_SELECTED) && (viewdata[col].attrib & DCA_CURRENT)) { bgcolor[0] = 1.0; bgcolor[1] = 0.75; bgcolor[2] = 0.5; }
					if (viewdata[col].attrib & DCA_CHANGED) { fgcolor[0] = 1.0; fgcolor[1] = fgcolor[2] = 0; }
					if (viewdata[col].attrib & DCA_INVALID) { fgcolor[0] = fgcolor[1] = 0; fgcolor[2] = 1.0; }
					if (viewdata[col].attrib & DCA_DISABLED) { fgcolor[0] = ( fgcolor[0] + bgcolor[0]) / 2; fgcolor[1] = ( fgcolor[1] + bgcolor[1]) / 2; fgcolor[2] = ( fgcolor[2] + bgcolor[2]) / 2; }
					if (viewdata[col].attrib & DCA_COMMENT) { fgcolor[0] = fgcolor[2] = 0; fgcolor[1] = 0.5; }

					// flush any pending drawing
					if (count > 0)
					{
						bounds.right = bounds.left + count * debug_font_width;
						if (iter == 0)
						{
							HIRect	frame;
							
							frame.origin.x = bounds.left; frame.origin.y = bounds.top;
							frame.size.width = bounds.right - bounds.left;
							frame.size.height = bounds.bottom - bounds.top;
													
							CGContextSetRGBFillColor(dc, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);
							CGContextFillRect(dc, frame);
						}
						else
						{
							CGContextSetRGBFillColor(dc, fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
							CGContextShowTextAtPoint(dc, bounds.left, bounds.top + debug_font_ascent, buffer, count);
						}
						bounds.left = bounds.right;
						count = 0;
					}

					last_attrib = viewdata[col].attrib;
				}

				// add this character to the buffer
				buffer[count++] = viewdata[col].byte;
			}

			// flush any remaining stuff
			if (count > 0)
			{
				bounds.right = bounds.left + count * debug_font_width;
				if (iter == 0)
				{
					HIRect	frame;
							
					frame.origin.x = bounds.left; frame.origin.y = bounds.top;
					frame.size.width = bounds.right - bounds.left;
					frame.size.height = bounds.bottom - bounds.top;
													
					CGContextSetRGBFillColor(dc, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);
					CGContextFillRect(dc, frame);
				}
				else
				{
					CGContextSetRGBFillColor(dc, fgcolor[0], fgcolor[1], fgcolor[2], fgcolor[3]);
					CGContextShowTextAtPoint(dc, bounds.left, bounds.top + debug_font_ascent, buffer, count);
				}
			}

			// erase to the end of the line
			if (iter == 0)
			{
				HIRect	frame;
				
				bounds.left = bounds.right;
				bounds.right = client.origin.x + client.size.width;
				
				frame.origin.x = bounds.left; frame.origin.y = bounds.top;
				frame.size.width = bounds.right - bounds.left;
				frame.size.height = bounds.bottom - bounds.top;
				
				CGContextSetRGBFillColor(dc, bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);
				CGContextFillRect(dc, frame);
			}
		}

		// advance viewdata
		viewdata += visarea.x;
	}

	// erase anything beyond the bottom with white
	HIViewGetBounds(view->wnd,&client);
	client.origin.y = visarea.y * debug_font_height;
	
	CGContextSetRGBFillColor(dc, 1.0, 1.0, 1.0, 1.0);
	CGContextFillRect(dc, client);
	CGContextRestoreGState(dc);
}



//============================================================
//  debugwin_view_update
//============================================================

static void debugwin_view_update(debug_view *view, void *osdprivate)
{
	debugview_info *info = debug_view_find(view);

	// if we have a view window, process it
	if (info && info->view)
	{
		HIRect bounds, vscroll_bounds, hscroll_bounds;
		debug_view_xy totalsize, visiblesize, topleft;
		int show_vscroll, show_hscroll;

		// get the view window bounds
		HIViewGetFrame(info->wnd,&bounds);
		visiblesize.x = bounds.size.width / debug_font_width;
		visiblesize.y = bounds.size.height / debug_font_height;

		// get the updated total rows/cols and left row/col
		totalsize = debug_view_get_total_size(view);
		topleft = debug_view_get_visible_position(view);

		// determine if we need to show the scrollbars
		show_vscroll = show_hscroll = 0;
		if (totalsize.x > visiblesize.x)
		{
			bounds.size.height -= hscroll_height;
			visiblesize.y = bounds.size.height / debug_font_height;
			show_hscroll = TRUE;
		}
		if (totalsize.y > visiblesize.y)
		{
			bounds.size.width -= vscroll_width;
			visiblesize.x = bounds.size.width / debug_font_width;
			show_vscroll = TRUE;
		}
		if (!show_vscroll && totalsize.y > visiblesize.y)
		{
			bounds.size.width -= vscroll_width;
			visiblesize.x = bounds.size.width / debug_font_width;
			show_vscroll = TRUE;
		}
		
		// compute the bounds of the scrollbars
		vscroll_bounds = bounds;
		vscroll_bounds.origin.x = bounds.origin.x + bounds.size.width;
		vscroll_bounds.size.width = vscroll_width;

		hscroll_bounds = bounds;
		hscroll_bounds.origin.y = bounds.origin.y + bounds.size.height;
		hscroll_bounds.size.height = hscroll_height;

		// if we hid the scrollbars, make sure we reset the top/left corners
		if (topleft.y + visiblesize.y > totalsize.y)
			topleft.y = MAX(totalsize.y - visiblesize.y, 0);
		if (topleft.x + visiblesize.x > totalsize.x)
			topleft.x = MAX(totalsize.x - visiblesize.x, 0);

		// fill out the scroll info struct for the vertical scrollbar
		if ( show_vscroll )
		{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			HIViewSetMinimum(info->vscroll,0);
			HIViewSetMaximum(info->vscroll,totalsize.y-visiblesize.y);
			HIViewSetValue(info->vscroll,topleft.y);
			HIViewSetViewSize(info->vscroll,visiblesize.y);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			SetControl32BitMinimum(info->vscroll,0);
			SetControl32BitMaximum(info->vscroll,totalsize.y-visiblesize.y);
			SetControl32BitValue(info->vscroll,topleft.y);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
		}
		
		// fill out the scroll info struct for the horizontal scrollbar
		if ( show_hscroll )
		{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			HIViewSetMinimum(info->hscroll,0);
			HIViewSetMaximum(info->hscroll,totalsize.x-visiblesize.x);
			HIViewSetValue(info->hscroll,topleft.x);
			HIViewSetViewSize(info->hscroll,visiblesize.x);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			SetControl32BitMinimum(info->hscroll,0);
			SetControl32BitMaximum(info->hscroll,totalsize.x-visiblesize.x);
			SetControl32BitValue(info->hscroll,topleft.x);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
		}

		// update window info
		visiblesize.y++;
		visiblesize.x++;
		debug_view_set_visible_size(view, visiblesize);
		debug_view_set_visible_position(view, topleft);

		// invalidate the bounds
		HIViewSetNeedsDisplay(info->wnd, TRUE);
		HIViewSetNeedsDisplay(info->hscroll, TRUE);
		HIViewSetNeedsDisplay(info->vscroll, TRUE);

		// adjust the bounds of the scrollbars and show/hide them
		if (info->vscroll)
		{
			if (show_vscroll)
			{
				// Shorten scroll bar if it intersects with grow box
				HIPoint scrollBottom, growTop;
				Rect growRect;

				GetWindowBounds( HIViewGetWindow( info->wnd ), kWindowGrowRgn, &growRect );
				
				growTop.x = growRect.left;
				growTop.y = growRect.top;
				
				scrollBottom.x = vscroll_bounds.origin.x+vscroll_bounds.size.width;
				scrollBottom.y = vscroll_bounds.origin.y+vscroll_bounds.size.height;
				
				osx_GlobalToViewPoint( HIViewGetSuperview(info->wnd), &growTop, &growTop );
				
				if( scrollBottom.y > growTop.y )
					vscroll_bounds.size.height -= hscroll_height;
					
				smart_set_view_bounds(info->vscroll, &vscroll_bounds);
			}
			
			smart_show_view(info->vscroll, show_vscroll);
		}
		if (info->hscroll)
		{
			if (show_hscroll)
				smart_set_view_bounds(info->hscroll, &hscroll_bounds);
			smart_show_view(info->hscroll, show_hscroll);
		}
	}
}



//============================================================
//  debug_view_find
//============================================================

static debugview_info *debug_view_find(debug_view *view)
{
	debugwin_info *info;
	int curview;

	// loop over windows and find the view
	for (info = window_list; info; info = info->next)
		for (curview = 0; curview < MAX_VIEWS; curview++)
			if (info->view[curview].view == view)
				return &info->view[curview];
	return NULL;
}

//============================================================
//  debugwin_view_process_scroll
//============================================================

static void debugwin_view_process_scroll(HIViewRef inView, ControlPartCode partCode)
{
	debugview_info *info;
	INT32		result;
	INT32		maxval;
	int			isHorz;
	
	if ( GetControlProperty(inView, 'MAME', 'info', sizeof( info ), NULL, &info ) != noErr)
		return;
	
	isHorz = (inView == info->hscroll) ? 1 : 0;
		
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	maxval = HIViewGetMaximum(inView);
	result = HIViewGetValue(inView);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	maxval = GetControl32BitMaximum(inView);
	result = GetControl32BitValue(inView);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	
	switch( partCode )
	{
		case kAppearancePartUpButton:
			result--;
		break;
		
		case kAppearancePartDownButton:
			result++;
		break;
		
		case kAppearancePartIndicator:
			// nothing if on the thumb
		break;
		
		case kAppearancePartPageUpArea:
			if ( isHorz )
				result -= debug_view_get_visible_size(info->view).x;
			else
				result -= debug_view_get_visible_size(info->view).y;
		break;
		
		case kAppearancePartPageDownArea:
			if ( isHorz )
				result += debug_view_get_visible_size(info->view).x;
			else
				result += debug_view_get_visible_size(info->view).y;
		break;
	}
	
	if ( result < 0 )
		result = 0;
	if ( result >= maxval )
		result = maxval;
	
	if ( isHorz )
	{
		debug_view_xy topleft = debug_view_get_visible_position(info->view);
		topleft.x = result;
		debug_view_set_visible_position(info->view, topleft);
	}
	else
	{
//		if (info->is_textbuf)
//		{
//			if( result == maxval )
//    	    	result = -1;
//    	    debug_view_set_property_UINT32(info->view, DVP_TEXTBUF_LINE_LOCK, result);
//		}
//		else
		{
			debug_view_xy topleft = debug_view_get_visible_position(info->view);
			topleft.y = result;
			debug_view_set_visible_position(info->view, topleft);
		}
	}
}


//============================================================
//  debugwin_view_prev_view
//============================================================

static void debugwin_view_prev_view(debugwin_info *info, debugview_info *curview)
{
	int curindex = 1;
	int numviews;

	// count the number of views
	for (numviews = 0; numviews < MAX_VIEWS; numviews++)
		if (info->view[numviews].wnd == NULL)
			break;

	// if we have a curview, find out its index
	if (curview)
		curindex = curview - &info->view[0];

	// loop until we find someone to take focus
	while (1)
	{
		// advance to the previous index
		curindex--;
		if (curindex < -1)
			curindex = numviews - 1;

		// negative numbers mean the focuswnd
		if (curindex < 0 && info->focuswnd != NULL)
		{
			osx_SetFocus(info->focuswnd);
			break;
		}

		// positive numbers mean a view
		else if (curindex >= 0 && info->view[curindex].wnd != NULL && debug_view_get_cursor_supported(info->view[curindex].view))
		{
			osx_SetFocus(info->view[curindex].wnd);
			break;
		}
	}
}



//============================================================
//  debugwin_view_next_view
//============================================================

static void debugwin_view_next_view(debugwin_info *info, debugview_info *curview)
{
	int curindex = -1;
	int numviews;

	// count the number of views
	for (numviews = 0; numviews < MAX_VIEWS; numviews++)
		if (info->view[numviews].wnd == NULL)
			break;

	// if we have a curview, find out its index
	if (curview)
		curindex = curview - &info->view[0];

	// loop until we find someone to take focus
	while (1)
	{
		// advance to the previous index
		curindex++;
		if (curindex >= numviews)
			curindex = -1;

		// negative numbers mean the focuswnd
		if (curindex < 0 && info->focuswnd != NULL)
		{
			osx_SetFocus(info->focuswnd);
			break;
		}

		// positive numbers mean a view
		else if (curindex >= 0 && info->view[curindex].wnd != NULL && debug_view_get_cursor_supported(info->view[curindex].view))
		{
			osx_SetFocus(info->view[curindex].wnd);
			HIViewSetNeedsDisplay(info->view[curindex].wnd, TRUE);
			break;
		}
	}
}



//============================================================
//  debugwin_view_proc
//============================================================

static OSStatus debugwin_view_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData)
{
	debugview_info *info = (debugview_info *)inUserData;
	UInt32			eventClass = GetEventClass( inEvent );
	UInt32			eventKind = GetEventKind( inEvent );
	
	if ( eventClass == kEventClassControl )
	{
		switch( eventKind )
		{
			// draw
			case kEventControlDraw:
			{
				CGContextRef	dc;
				
				if ( GetEventParameter( inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(dc), NULL, &dc ) != noErr )
				{
					mame_printf_error( "Compositing attribute not set!\n" );
					exit( -1 );
				}
			
				debugwin_view_draw_contents(info, dc);
				return noErr;
			}
			break;

			// focus
			case kEventControlSetFocusPart:
			{
				ControlPartCode		part;
				
				if ( GetEventParameter( inEvent, kEventParamControlPart, typeControlPartCode, NULL, sizeof(part), NULL, &part ) == noErr )
				{
					if ( part == kControlFocusNoPart )	// losing focus
					{
						if (debug_view_get_cursor_supported(info->view))
							debug_view_set_cursor_visible(info->view, FALSE);
					}
					else // gaining focus
					{
						if (debug_view_get_cursor_supported(info->view))
							debug_view_set_cursor_visible(info->view, TRUE);

						part = kControlLabelPart;
						SetEventParameter( inEvent, kEventParamControlPart, typeControlPartCode, sizeof(part), &part );
					}
					
					return noErr;
				}
			}
			break;
			
			// mouse click
			case kEventControlClick:
			{
				HIPoint		mousePos;
				HIPoint		viewPos;
				
				if ( GetEventParameter( inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(mousePos), NULL, &mousePos ) == noErr )
				{
					EventMouseButton		button;
					
					osx_GlobalToViewPoint( info->wnd, &mousePos, &viewPos );
				
					if ( GetEventParameter( inEvent, kEventParamMouseButton, typeMouseButton, NULL, sizeof(button), NULL, &button ) == noErr )
					{
						if ( button == kEventMouseButtonPrimary )
						{
							if (debug_view_get_cursor_supported(info->view))
							{
//								debug_view_xy topleft = debug_view_get_visible_position(info->view);
								debug_view_xy newpos;
								newpos.x = viewPos.x / debug_font_width;
								newpos.y = viewPos.y / debug_font_height;
								debug_view_set_cursor_position(info->view, newpos);
								osx_SetFocus(info->wnd);
							}
							return noErr;
						}
					}
				}
            }
			break;
			
			// contextual menu click
			case kEventControlContextualMenuClick:
			{
				HIPoint		viewPos;
				HIPoint		mousePos;
				
				if ( GetEventParameter( inEvent, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(viewPos), NULL, &viewPos ) == noErr )
				{
					if ( info->owner->contextualMenu )
					{
						osx_ViewToGlobalPoint( info->wnd, &viewPos, &mousePos );
					
						PopUpMenuSelect( info->owner->contextualMenu, mousePos.y, mousePos.x, 0 );
						return noErr;
					}
				}
			}
			break;
		}
	}
	else if ( eventClass == kEventClassMouse && eventKind == kEventMouseWheelMoved )
	{
		EventMouseWheelAxis		axis;
		SInt32					delta;
	
		if ( GetEventParameter( inEvent, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(axis), NULL, &axis ) == noErr )
		{
			if ( GetEventParameter( inEvent, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(delta), NULL, &delta ) == noErr )
			{
				INT32	maxval = 0, result = 0;
				
				if ( axis == kEventMouseWheelAxisX && info->hscroll != NULL )
				{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
					maxval = HIViewGetMaximum(info->hscroll);
					result = HIViewGetValue(info->hscroll);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
					maxval = GetControl32BitMaximum(info->hscroll);
					result = GetControl32BitValue(info->hscroll);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
				}
				else if ( axis == kEventMouseWheelAxisY && info->vscroll != NULL )
				{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
					maxval = HIViewGetMaximum(info->vscroll);
					result = HIViewGetValue(info->vscroll);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
					maxval = GetControl32BitMaximum(info->vscroll);
					result = GetControl32BitValue(info->vscroll);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
				}
				
				result -= delta;
				
				if ( result < 0 )
					result = 0;
				if ( result >= maxval )
					result = maxval;
				
				if ( axis == kEventMouseWheelAxisX && info->hscroll )
				{
					debug_view_xy topleft = debug_view_get_visible_position(info->view);
					topleft.x = result;
					debug_view_set_visible_position(info->view, topleft);
				}
				else if ( axis == kEventMouseWheelAxisY && info->vscroll != NULL )
				{
//					if (info->is_textbuf)
//						debug_view_set_property_UINT32(info->view, DVP_TEXTBUF_LINE_LOCK, result);
//					else
					{
						debug_view_xy topleft = debug_view_get_visible_position(info->view);
						topleft.y = result;
						debug_view_set_visible_position(info->view, topleft);
					}
				}
			}
		}
	}
	else if ( eventClass == kEventClassKeyboard && ( eventKind == kEventRawKeyDown || eventKind == kEventRawKeyRepeat ) )
	{
		EventRecord		event;
	
		if ( ConvertEventRefToEventRecord( inEvent, &event ) )
		{
			switch( event.message & charCodeMask )
			{
				case kUpArrowCharCode:
				{
					debug_view_type_character(info->view, DCH_UP);
					return noErr;
				}
				break;
				
				case kDownArrowCharCode:
				{
					debug_view_type_character(info->view, DCH_DOWN);
					return noErr;
				}
				break;
				
				case kLeftArrowCharCode:
				{
					if ( event.modifiers & cmdKey )
						debug_view_type_character(info->view, DCH_CTRLLEFT);
					else
						debug_view_type_character(info->view, DCH_LEFT);
					
					return noErr;
				}
				break;
				
				case kRightArrowCharCode:
				{
					if ( event.modifiers & cmdKey )
						debug_view_type_character(info->view, DCH_CTRLRIGHT);
					else
						debug_view_type_character(info->view, DCH_RIGHT);
					
					return noErr;
				}
				break;
				
				case kPageUpCharCode:
				{
					debug_view_type_character(info->view, DCH_PUP);
					return noErr;
				}
				break;
				
				case kPageDownCharCode:
				{
					debug_view_type_character(info->view, DCH_PDOWN);
					return noErr;
				}
				break;
				
				case kHomeCharCode:
				{
					if ( event.modifiers & cmdKey )
						debug_view_type_character(info->view, DCH_CTRLHOME);
					else
						debug_view_type_character(info->view, DCH_HOME);
					
					return noErr;
				}
				break;
				
				case kEndCharCode:
				{
					if ( event.modifiers & cmdKey )
						debug_view_type_character(info->view, DCH_CTRLEND);
					else
						debug_view_type_character(info->view, DCH_END);
					
					return noErr;
				}
				break;
				
				case kEscapeCharCode:
				{
					if (info->owner->focuswnd != NULL)
						osx_SetFocus(info->owner->focuswnd);
					return noErr;
				}
				break;
				
				case kTabCharCode:
				{
					if ( event.modifiers & shiftKey )
						debugwin_view_prev_view(info->owner, info);
					else
						debugwin_view_next_view(info->owner, info);
					return noErr;
				}
				break;
				
				default:
				{
					char	code = event.message & charCodeMask;
					
					if (code >= 32 && code < 127 && debug_view_get_cursor_supported(info->view))
					{
						debug_view_type_character(info->view, code);
						return noErr;
					}
				}
				break;
			}
		}
	}

	return eventNotHandledErr;
}



//============================================================
//  debugwin_edit_proc
//============================================================

static OSStatus debugwin_edit_proc(EventHandlerCallRef inHandler, EventRef inEvent, void * inUserData)
{
	debugwin_info *info = (debugwin_info *)inUserData;
	char buffer[MAX_EDIT_STRING];
	
	UInt32			eventClass = GetEventClass( inEvent );
	UInt32			eventKind = GetEventKind( inEvent );

	if ( eventClass == kEventClassKeyboard && ( eventKind == kEventRawKeyDown || eventKind == kEventRawKeyRepeat ) )
	{
		EventRecord		event;
	
		if ( ConvertEventRefToEventRecord( inEvent, &event ) )
		{
			switch( event.message & charCodeMask )
			{
				case kUpArrowCharCode:
				{
					ControlEditTextSelectionRec	sel;
					
					if (info->last_history < info->history_count - 1)
						info->last_history++;
					else
						info->last_history = 0;
					
					sel.selStart = strlen(&info->history[info->last_history][0]);
					sel.selEnd = sel.selStart;
				
					SetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, strlen(&info->history[info->last_history][0]), &info->history[info->last_history][0]);
					SetControlData(info->editwnd, kControlEntireControl, kControlEditTextSelectionTag, sizeof( sel ), &sel );
					return noErr;
				}
				break;
				
				case kDownArrowCharCode:
				{
					ControlEditTextSelectionRec	sel;
					
					if (info->last_history > 0)
						info->last_history--;
					else if (info->history_count > 0)
						info->last_history = info->history_count - 1;
					else
						info->last_history = 0;
					
					sel.selStart = strlen(&info->history[info->last_history][0]);
					sel.selEnd = sel.selStart;
				
					SetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, strlen(&info->history[info->last_history][0]), &info->history[info->last_history][0]);
					SetControlData(info->editwnd, kControlEntireControl, kControlEditTextSelectionTag, sizeof( sel ), &sel );
					return noErr;
				}
				break;
				
				case kTabCharCode:
				{
					if (event.modifiers & shiftKey)
						debugwin_view_prev_view(info, NULL);
					else
						debugwin_view_next_view(info, NULL);
					
					return noErr;
				}
				break;
												
				case kReturnCharCode:
				{
					// fetch the text
					Size		actualSize;
					GetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, sizeof(buffer), buffer, &actualSize);
					
					buffer[actualSize] = 0;

					// add to the history if it's not a repeat of the last one
					if (buffer[0] != 0 && strcmp(buffer, &info->history[0][0]))
					{
						memmove(&info->history[1][0], &info->history[0][0], (HISTORY_LENGTH - 1) * MAX_EDIT_STRING);
						strcpy(&info->history[0][0], buffer);
						if (info->history_count < HISTORY_LENGTH)
							info->history_count++;
					}
					info->last_history = info->history_count - 1;

					// process
					if (info->process_string)
						(*info->process_string)(info, buffer);
					return noErr;
				}
				break;
				
				case kEscapeCharCode:
				{
					// fetch the text
					Size		actualSize;
					GetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, sizeof(buffer), buffer, &actualSize);
					
					buffer[actualSize] = 0;
					
					if (strlen(buffer) > 0)
					{
						SetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, 0, buffer);
					}
					return noErr;
				}
				break;
			}
		}
	}
	
	return eventNotHandledErr;
}


//============================================================
//  generic_recompute_children
//============================================================

static void generic_recompute_children(debugwin_info *info)
{
	debug_view_xy totalsize = debug_view_get_total_size(info->view[0].view);
	Rect parent;
	Rect bounds;
	HIRect	hibounds;

	// compute a client rect
	bounds.top = bounds.left = 0;
	bounds.right = totalsize.x * debug_font_width + vscroll_width + 2 * EDGE_WIDTH;
	bounds.bottom = 200;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);

	// clamp the min/max size
	info->maxwidth = bounds.right - bounds.left;

	// get the parent's dimensions
	HIViewGetBounds(info->contentView,&hibounds);
	parent.top = hibounds.origin.y;
	parent.bottom = hibounds.origin.y + hibounds.size.height;
	parent.left = hibounds.origin.x;
	parent.right = hibounds.origin.x + hibounds.size.width;

	// view gets the remaining space
	parent.top += EDGE_WIDTH; parent.left += EDGE_WIDTH;
	parent.bottom -= EDGE_WIDTH; parent.right -= EDGE_WIDTH;
	
	debugwin_view_set_bounds(&info->view[0], &parent);
}



//============================================================
//  log_create_window
//============================================================

static void log_create_window(running_machine *machine)
{
	debug_view_xy totalsize;
	debugwin_info *info;
	char title[256];
	UINT32 width, height;
	Rect bounds;

	// create the window
	snprintf(title, ARRAY_LENGTH(title), "Errorlog: %s [%s]", machine->gamedrv->description, machine->gamedrv->name);
	info = debug_window_create(machine, title, NULL);
	if (info == NULL || !debugwin_view_create(info, 0, DVT_LOG))
		return;

	// set the child function
	info->recompute_children = generic_recompute_children;

	// get the view width
	totalsize = debug_view_get_total_size(info->view[0].view);

	// compute a client rect
	bounds.top = bounds.left = 0;
	bounds.right = totalsize.x * debug_font_width + vscroll_width + 2 * EDGE_WIDTH;
	bounds.bottom = 200;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);

	// clamp the min/max size
	info->maxwidth = bounds.right - bounds.left;
	
	// position the window at the bottom-right
	width = bounds.right - bounds.left;
	height = bounds.bottom - bounds.top;
		
	bounds.top = bounds.left = 100;
	bounds.right = bounds.left + width;
	bounds.bottom = bounds.top + height;
	
	SetWindowBounds(info->wnd, kWindowStructureRgn, &bounds );
	ShowWindow(info->wnd);
	SelectWindow(info->wnd);
	
	// recompute the children and set focus on the edit box
	generic_recompute_children(info);
}



//============================================================
//  memory_create_window
//============================================================

static void memory_create_window(running_machine *machine)
{
	const device_config *curcpu = debug_cpu_get_visible_cpu(machine);
	const memory_subview_item *subview;
	int cursel = -1;
	debugwin_info *info;
	MenuRef optionsmenu,popupmenu;
	Rect	bounds;
	ControlFontStyleRec		style;
	Str255		fontName;
	MenuItemIndex	menuIndex;
	ControlButtonContentInfo	content;

	// create the window
	info = debug_window_create(machine, "Memory", NULL);
	if (!info || !debugwin_view_create(info, 0, DVT_MEMORY))
		return;

	// set the handlers
	info->handle_command = memory_handle_command;
	info->handle_key = memory_handle_key;

	// create the options menu
	if ( CreateNewMenu( 0, 0, &optionsmenu ) != noErr )
	{
		mame_printf_error( "Could not create menu\n" );
		exit( -1 );
	}
	
	{
		EventTypeSpec	spec = { kEventClassCommand, kEventCommandProcess };
	
		InstallEventHandler( GetMenuEventTarget( optionsmenu ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL );
	}

	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("1-byte chunks"), 0, ID_1_BYTE_CHUNKS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, '1');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("2-byte chunks"), 0, ID_2_BYTE_CHUNKS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, '2');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("4-byte chunks"), 0, ID_4_BYTE_CHUNKS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, '4');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("8-byte chunks"), 0, ID_8_BYTE_CHUNKS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, '8');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Logical Addresses"), 0, ID_LOGICAL_ADDRESSES, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'L');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Physical Addresses"), 0, ID_PHYSICAL_ADDRESSES, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'Y');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Reverse View"), 0, ID_REVERSE_VIEW, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'R');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Increase bytes per line"), 0, ID_INCREASE_MEM_WIDTH, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'P');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Decrease bytes per line"), 0, ID_DECREASE_MEM_WIDTH, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'O');
	info->contextualMenu = optionsmenu;
	memory_update_checkmarks(info);

	// set up the view to track the initial expression
	memory_view_set_expression(info->view[0].view, "0");
	strcpy(info->edit_defstr, "0");

	fontName[0] = strlen( OSX_DEBUGGER_FONT_NAME );
	strcpy( (char*)&fontName[1], OSX_DEBUGGER_FONT_NAME );

	style.flags = kControlUseFontMask | kControlUseSizeMask;
	style.font = ATSFontFamilyFindFromQuickDrawName(fontName);
	style.size = OSX_DEBUGGER_FONT_SIZE;
	
	// create an edit box and override its key handling
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateEditUnicodeTextControl(info->wnd, &bounds, CFSTR("0"), false, NULL, &info->editwnd);
	SetControlFontStyle(info->editwnd,&style);
	InstallEventHandler( GetControlEventTarget(info->editwnd), NewEventHandlerUPP(debugwin_edit_proc), sizeof( debug_keyboard_events ) / sizeof( EventTypeSpec ), debug_keyboard_events, info, NULL );

	// create action menu and attach the contextual menu
	content.contentType = kControlContentTextOnly;
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateBevelButtonControl(info->wnd, &bounds, CFSTR(""), kControlBevelButtonSmallBevel, kControlBehaviorPushbutton, &content, 1, kControlBehaviorMultiValueMenu, kControlBevelButtonMenuOnBottom, &info->menuwnd );
	SetControlData(info->menuwnd, kControlEntireControl, kControlBevelButtonMenuHandleTag, sizeof(optionsmenu), &optionsmenu);

	// create a popup control
	CreatePopupButtonControl(info->wnd, &bounds, CFSTR(""), -12345, FALSE, 0, teFlushRight, normal, &info->otherwnd[0]);
	SetControlFontStyle(info->otherwnd[0],&style);
	SetControlCommandID(info->otherwnd[0], OSX_POPUP_COMMAND);
	
	if ( CreateNewMenu( 0, 0, &popupmenu ) != noErr )
	{
		mame_printf_error( "Could not create menu\n" );
		exit( -1 );
	}
	
	{
		EventTypeSpec	spec = { kEventClassCommand, kEventCommandProcess };
	
		InstallEventHandler( GetMenuEventTarget( popupmenu ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL );
	}

	// populate the combobox
	for (subview = memory_view_get_subview_list(info->view[0].view); subview != NULL; subview = subview->next)
	{
		CFStringRef		cfName = CFStringCreateWithCString( NULL, subview->name, CFStringGetSystemEncoding() );
		UInt16	item;
		
		AppendMenuItemTextWithCFString( popupmenu, cfName, 0, 0, &item );
		
		CFRelease( cfName );
		
		if (cursel == -1 && subview->space != NULL && subview->space->cpu == curcpu)
		{
			cursel = item;
		}
	}
	if (cursel == -1)	cursel = 0;
	
	SetControlData(info->otherwnd[0], kControlEntireControl, kControlPopupButtonOwnedMenuRefTag, sizeof(popupmenu),&popupmenu);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	HIViewSetMinimum(info->otherwnd[0], 1);
	HIViewSetMaximum(info->otherwnd[0], CountMenuItems(popupmenu));
	HIViewSetValue(info->otherwnd[0],cursel);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	SetControl32BitMinimum(info->otherwnd[0], 1);
	SetControl32BitMaximum(info->otherwnd[0], CountMenuItems(popupmenu));
	SetControl32BitValue(info->otherwnd[0],cursel);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4

	// set the child functions
	info->recompute_children = memory_recompute_children;
	info->process_string = memory_process_string;

	// set the caption
	memory_update_caption(info);

	// recompute the children once to get the maxwidth
	memory_recompute_children(info);

	// position the window and recompute children again
	bounds.top = bounds.left = 100;
	bounds.right = bounds.left + info->maxwidth;
	bounds.bottom = bounds.top + 200;
	
	SetWindowBounds(info->wnd, kWindowStructureRgn, &bounds );
	ShowWindow(info->wnd);
	SelectWindow(info->wnd);
	
	memory_recompute_children(info);

	// mark the edit box as the default focus and set it
	info->focuswnd = info->editwnd;
	osx_SetFocus(info->editwnd);
}



//============================================================
//  memory_recompute_children
//============================================================

static void memory_recompute_children(debugwin_info *info)
{
	debug_view_xy totalsize = debug_view_get_total_size(info->view[0].view);
	Rect		parent, memrect, editrect, comborect, actionrect;
	Rect		bounds;
	HIRect		hibounds;

	// compute a client rect
	bounds.top = bounds.left = 0;
	bounds.right = totalsize.x * debug_font_width + vscroll_width + 2 * EDGE_WIDTH;
	bounds.bottom = 200;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);

	// clamp the min/max size
	info->maxwidth = bounds.right - bounds.left;

	// get the parent's dimensions
	HIViewGetBounds(info->contentView,&hibounds);
	parent.top = hibounds.origin.y;
	parent.bottom = hibounds.origin.y + hibounds.size.height;
	parent.left = hibounds.origin.x;
	parent.right = hibounds.origin.x + hibounds.size.width;

	// edit box gets half of the width (minus a little for the action button)
	editrect.top = parent.top + EDGE_WIDTH;
	editrect.bottom = editrect.top + debug_font_height + 4;
	editrect.left = parent.left + EDGE_WIDTH;
	editrect.right = parent.left + (parent.right - parent.left) / 2 - EDGE_WIDTH - 20 - EDGE_WIDTH;

	// action button goes right next to the edit box
	
	actionrect.top = editrect.top - 1;
	actionrect.bottom = editrect.bottom + 1;
	actionrect.left = editrect.right + EDGE_WIDTH + 2;
	actionrect.right = actionrect.left + 20;
	
	// combo box gets the other half of the width
	comborect.top = editrect.top;
	comborect.bottom = editrect.bottom;
	comborect.left = actionrect.right + EDGE_WIDTH;
	comborect.right = parent.right - EDGE_WIDTH;

	// memory view gets the rest
	memrect.top = editrect.bottom + 2 * EDGE_WIDTH;
	memrect.bottom = parent.bottom - EDGE_WIDTH;
	memrect.left = parent.left + EDGE_WIDTH;
	memrect.right = parent.right - EDGE_WIDTH;

	// set the bounds of things
	debugwin_view_set_bounds(&info->view[0], &memrect);
	
	osx_RectToHIRect( &editrect, &hibounds );
	smart_set_view_bounds(info->editwnd, &hibounds);

	osx_RectToHIRect( &comborect, &hibounds );
	smart_set_view_bounds(info->otherwnd[0], &hibounds);

	osx_RectToHIRect( &actionrect, &hibounds );
	smart_set_view_bounds(info->menuwnd, &hibounds);
}

//============================================================
//  memory_process_string
//============================================================

static void memory_process_string(debugwin_info *info, const char *string)
{
	// set the string to the memory view
	memory_view_set_expression(info->view[0].view, string);

	// select everything in the edit text box
	{
		ControlEditTextSelectionRec	sel;
					
		sel.selStart = 0;
		sel.selEnd = strlen(string);
				
		SetControlData(info->editwnd, kControlEntireControl, kControlEditTextSelectionTag, sizeof( sel ), &sel );
	}

	// update the default string to match
	strncpy(info->edit_defstr, string, sizeof(info->edit_defstr) - 1);
}



//============================================================
//  memory_update_checkmarks
//============================================================

static void memory_update_checkmarks(debugwin_info *info)
{
	if ( info->contextualMenu != NULL )
	{
		MenuItemIndex	menuindex;
		
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_1_BYTE_CHUNKS, 1, NULL, &menuindex );
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_bytes_per_chunk(info->view[0].view) == 1));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_2_BYTE_CHUNKS, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_bytes_per_chunk(info->view[0].view) == 2));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_4_BYTE_CHUNKS, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_bytes_per_chunk(info->view[0].view) == 4));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_8_BYTE_CHUNKS, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_bytes_per_chunk(info->view[0].view) == 8));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_LOGICAL_ADDRESSES, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_physical(info->view[0].view) == 0));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_PHYSICAL_ADDRESSES, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, (memory_view_get_physical(info->view[0].view) != 0));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_REVERSE_VIEW, 1, NULL, &menuindex);
		CheckMenuItem(info->contextualMenu, menuindex, memory_view_get_reverse(info->view[0].view));
	}
}



//============================================================
//  memory_handle_command
//============================================================

static int memory_handle_command(debugwin_info *info, EventRef inEvent)
{
	HICommand	cmd;
	
	if ( GetEventParameter( inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd ) != noErr )
		return global_handle_command(info, inEvent);
	
	switch( cmd.commandID )
	{
		case OSX_POPUP_COMMAND:
		{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			int	sel = HIViewGetValue(info->otherwnd[0]);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			int	sel = GetControl32BitValue(info->otherwnd[0]);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			
			if ( sel > 0 )
			{
				memory_view_set_subview(info->view[0].view, sel-1);
				memory_update_caption(info);

				// reset the focus
				osx_SetFocus(info->focuswnd);
				return 1;
			}
		}
		break;
	
	
		case ID_1_BYTE_CHUNKS:
			memory_view_set_bytes_per_chunk(info->view[0].view, 1);
			return 1;

		case ID_2_BYTE_CHUNKS:
			memory_view_set_bytes_per_chunk(info->view[0].view, 2);
			return 1;

		case ID_4_BYTE_CHUNKS:
			memory_view_set_bytes_per_chunk(info->view[0].view, 4);
			return 1;

		case ID_8_BYTE_CHUNKS:
			memory_view_set_bytes_per_chunk(info->view[0].view, 8);
			return 1;

		case ID_LOGICAL_ADDRESSES:
			memory_view_set_physical(info->view[0].view, FALSE);
			return 1;

		case ID_PHYSICAL_ADDRESSES:
			memory_view_set_physical(info->view[0].view, TRUE);
			return 1;

		case ID_REVERSE_VIEW:
			memory_view_set_reverse(info->view[0].view, !memory_view_get_reverse(info->view[0].view));
			return 1;

		case ID_INCREASE_MEM_WIDTH:
			memory_view_set_chunks_per_row(info->view[0].view, memory_view_get_chunks_per_row(info->view[0].view) + 1);
			return 1;

		case ID_DECREASE_MEM_WIDTH:
			memory_view_set_chunks_per_row(info->view[0].view, memory_view_get_chunks_per_row(info->view[0].view) - 1);
			return 1;
	}
	
	return global_handle_command(info, inEvent);
}



//============================================================
//  memory_handle_key
//============================================================

static int memory_handle_key(debugwin_info *info, EventRef inEvent)
{
	EventRecord		event;
	
	if ( ConvertEventRefToEventRecord( inEvent, &event ) )
	{
		if ( event.modifiers & cmdKey )
		{
			switch( event.message & charCodeMask )
			{
				case '1':
					memory_view_set_bytes_per_chunk(info->view[0].view, 1);
					return 1;

				case '2':
					memory_view_set_bytes_per_chunk(info->view[0].view, 2);
					return 1;

				case '4':
					memory_view_set_bytes_per_chunk(info->view[0].view, 4);
					return 1;

				case '8':
					memory_view_set_bytes_per_chunk(info->view[0].view, 8);
					return 1;

				case 'l':
				case 'L':
					memory_view_set_physical(info->view[0].view, FALSE);
					return 1;

				case 'y':
				case 'Y':
					memory_view_set_physical(info->view[0].view, TRUE);
					return 1;

				case 'r':
				case 'R':
					memory_view_set_reverse(info->view[0].view, !memory_view_get_reverse(info->view[0].view));
					return 1;

				case 'p':
				case 'P':
					memory_view_set_chunks_per_row(info->view[0].view, memory_view_get_chunks_per_row(info->view[0].view) + 1);
					return 1;

				case 'o':
				case 'O':
					memory_view_set_chunks_per_row(info->view[0].view, memory_view_get_chunks_per_row(info->view[0].view) - 1);
					return 1;
			}
		}
	}
	
	return global_handle_key(info, inEvent);
}



//============================================================
//  memory_update_caption
//============================================================

static void memory_update_caption(debugwin_info *info)
{
	const memory_subview_item *subview = memory_view_get_current_subview(info->view[0].view);
	CFStringRef cftitle;
	char title[256];

	// update the caption
	snprintf(title, ARRAY_LENGTH(title), "Memory: %s", subview->name);

	cftitle = CFStringCreateWithCString( NULL, title, CFStringGetSystemEncoding() );

	if ( cftitle )
	{
		SetWindowTitleWithCFString( info->wnd, cftitle);
		CFRelease( cftitle ); 
	}
}



//============================================================
//  disasm_create_window
//============================================================

static void disasm_create_window(running_machine *machine)
{
	const device_config *curcpu = debug_cpu_get_visible_cpu(machine);
	const disasm_subview_item *subview;
	int cursel = 0;
	debugwin_info *info;
	MenuRef optionsmenu, popupmenu;
	Rect	bounds;
	ControlFontStyleRec		style;
	Str255		fontName;
	MenuItemIndex	menuIndex;
	ControlButtonContentInfo	content;

	// create the window
	info = debug_window_create(machine, "Disassembly", NULL);
	if (!info || !debugwin_view_create(info, 0, DVT_DISASSEMBLY))
		return;

	// create the options menu
	if ( CreateNewMenu( 0, 0, &optionsmenu ) != noErr )
	{
		mame_printf_error( "Could not create menu\n" );
		exit( -1 );
	}
	
	{
		EventTypeSpec	spec = { kEventClassCommand, kEventCommandProcess };
	
		InstallEventHandler( GetMenuEventTarget( optionsmenu ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL );
	}
	
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Set breakpoint at cursor"), 0, ID_TOGGLE_BREAKPOINT, &menuIndex );
	SetMenuItemKeyGlyph(optionsmenu, menuIndex, kMenuF9Glyph); SetMenuItemModifiers(optionsmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Run to cursor"), 0, ID_RUN_TO_CURSOR, &menuIndex );
	SetMenuItemKeyGlyph(optionsmenu, menuIndex, kMenuF4Glyph); SetMenuItemModifiers(optionsmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Raw opcodes"), 0, ID_SHOW_RAW, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'R');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Encrypted opcodes"), 0, ID_SHOW_ENCRYPTED, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'E');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Comments"), 0, ID_SHOW_COMMENTS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'N');
	info->contextualMenu = optionsmenu;
	disasm_update_checkmarks(info);

	// set the handlers
	info->handle_command = disasm_handle_command;
	info->handle_key = disasm_handle_key;

	// set up the view to track the initial expression
	disasm_view_set_expression(info->view[0].view, "curpc");
	strcpy(info->edit_defstr, "curpc");
	
	fontName[0] = strlen( OSX_DEBUGGER_FONT_NAME );
	strcpy( (char*)&fontName[1], OSX_DEBUGGER_FONT_NAME );

	style.flags = kControlUseFontMask | kControlUseSizeMask;
	style.font = ATSFontFamilyFindFromQuickDrawName(fontName);
	style.size = OSX_DEBUGGER_FONT_SIZE;

	// create an edit box and override its key handling
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateEditUnicodeTextControl(info->wnd, &bounds, CFSTR("0"), false, NULL, &info->editwnd);
	SetControlFontStyle(info->editwnd,&style);
	InstallEventHandler( GetControlEventTarget(info->editwnd), NewEventHandlerUPP(debugwin_edit_proc), sizeof( debug_keyboard_events ) / sizeof( EventTypeSpec ), debug_keyboard_events, info, NULL );
	
	// create action menu and attach the contextual menu
	content.contentType = kControlContentTextOnly;
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateBevelButtonControl(info->wnd, &bounds, CFSTR(""), kControlBevelButtonSmallBevel, kControlBehaviorPushbutton, &content, 1, kControlBehaviorMultiValueMenu, kControlBevelButtonMenuOnBottom, &info->menuwnd );
	SetControlData(info->menuwnd, kControlEntireControl, kControlBevelButtonMenuHandleTag, sizeof(optionsmenu), &optionsmenu);

	// create a popup control
	CreatePopupButtonControl(info->wnd, &bounds, CFSTR(""), -12345, FALSE, 0, teFlushRight, normal, &info->otherwnd[0]);
	SetControlFontStyle(info->otherwnd[0],&style);
	SetControlCommandID(info->otherwnd[0], OSX_POPUP_COMMAND);

	if ( CreateNewMenu( 0, 0, &popupmenu ) != noErr )
	{
		mame_printf_error( "Could not create menu\n" );
		exit( -1 );
	}
	
	{
		EventTypeSpec	spec = { kEventClassCommand, kEventCommandProcess };
	
		InstallEventHandler( GetMenuEventTarget( popupmenu ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL );
	}

	// populate the combobox
	for (subview = disasm_view_get_subview_list(info->view[0].view); subview != NULL; subview = subview->next)
	{
		CFStringRef		cfName;
		char			name[100];
		UInt16			item;

		snprintf(name, ARRAY_LENGTH(name), "%s", subview->name);
				
		cfName = CFStringCreateWithCString( NULL, name, CFStringGetSystemEncoding() );
		AppendMenuItemTextWithCFString( popupmenu, cfName, 0, 0, &item );
		CFRelease( cfName );
				
		if (cursel == 0 && subview->space->cpu == curcpu)
			cursel = item;
	}
	disasm_view_set_subview(info->view[0].view, cursel-1);
	
	SetControlData(info->otherwnd[0], kControlEntireControl, kControlPopupButtonOwnedMenuRefTag, sizeof(popupmenu),&popupmenu);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	HIViewSetMinimum(info->otherwnd[0], 1);
	HIViewSetMaximum(info->otherwnd[0], CountMenuItems(popupmenu));
	HIViewSetValue(info->otherwnd[0],cursel);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	SetControl32BitMinimum(info->otherwnd[0], 1);
	SetControl32BitMaximum(info->otherwnd[0], CountMenuItems(popupmenu));
	SetControl32BitValue(info->otherwnd[0],cursel);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
	
	// set the child functions
	info->recompute_children = disasm_recompute_children;
	info->process_string = disasm_process_string;

	// set the caption
	disasm_update_caption(info);

	// recompute the children once to get the maxwidth
	disasm_recompute_children(info);

	// position the window and recompute children again
	bounds.top = bounds.left = 100;
	bounds.right = bounds.left + info->maxwidth;
	bounds.bottom = bounds.top + 200;
	
	SetWindowBounds(info->wnd, kWindowStructureRgn, &bounds );
	ShowWindow(info->wnd);
	SelectWindow(info->wnd);
	
	disasm_recompute_children(info);

	// mark the edit box as the default focus and set it
	info->focuswnd = info->editwnd;
	osx_SetFocus(info->editwnd);
}



//============================================================
//  disasm_recompute_children
//============================================================

static void disasm_recompute_children(debugwin_info *info)
{
	debug_view_xy totalsize = debug_view_get_total_size(info->view[0].view);
	Rect		parent, dasmrect, editrect, comborect, actionrect;
	Rect		bounds;
	HIRect		hibounds;

	// compute a client rect
	bounds.top = bounds.left = 0;
	bounds.right = totalsize.x * debug_font_width + vscroll_width + 2 * EDGE_WIDTH;
	bounds.bottom = 200;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);

	// clamp the min/max size
	info->maxwidth = bounds.right - bounds.left;

	// get the parent's dimensions
	HIViewGetBounds(info->contentView,&hibounds);
	parent.top = hibounds.origin.y;
	parent.bottom = hibounds.origin.y + hibounds.size.height;
	parent.left = hibounds.origin.x;
	parent.right = hibounds.origin.x + hibounds.size.width;

	// edit box gets half of the width
	editrect.top = parent.top + EDGE_WIDTH;
	editrect.bottom = editrect.top + debug_font_height + 4;
	editrect.left = parent.left + EDGE_WIDTH;
	editrect.right = parent.left + (parent.right - parent.left) / 2 - EDGE_WIDTH - 20 - EDGE_WIDTH;

	// action button goes right next to the edit box
	actionrect.top = editrect.top - 1;
	actionrect.bottom = editrect.bottom + 1;
	actionrect.left = editrect.right + EDGE_WIDTH + 2;
	actionrect.right = actionrect.left + 20;

	// combo box gets the other half of the width
	comborect.top = editrect.top;
	comborect.bottom = editrect.bottom;
	comborect.left = actionrect.right + EDGE_WIDTH;
	comborect.right = parent.right - EDGE_WIDTH;

	// disasm view gets the rest
	dasmrect.top = editrect.bottom + 2 * EDGE_WIDTH;
	dasmrect.bottom = parent.bottom - EDGE_WIDTH;
	dasmrect.left = parent.left + EDGE_WIDTH;
	dasmrect.right = parent.right - EDGE_WIDTH;

	// set the bounds of things
	debugwin_view_set_bounds(&info->view[0], &dasmrect);
	
	osx_RectToHIRect( &editrect, &hibounds );
	smart_set_view_bounds(info->editwnd, &hibounds);
	
	osx_RectToHIRect( &comborect, &hibounds );
	smart_set_view_bounds(info->otherwnd[0], &hibounds);

	osx_RectToHIRect( &actionrect, &hibounds );
	smart_set_view_bounds(info->menuwnd, &hibounds);
}



//============================================================
//  disasm_process_string
//============================================================

static void disasm_process_string(debugwin_info *info, const char *string)
{
	// set the string to the disasm view
	disasm_view_set_expression(info->view[0].view, string);

	// select everything in the edit text box
	{
		ControlEditTextSelectionRec	sel;
					
		sel.selStart = 0;
		sel.selEnd = strlen(string);
				
		SetControlData(info->editwnd, kControlEntireControl, kControlEditTextSelectionTag, sizeof( sel ), &sel );
	}

	// update the default string to match
	strncpy(info->edit_defstr, string, sizeof(info->edit_defstr) - 1);
}



//============================================================
//  disasm_update_checkmarks
//============================================================

static void disasm_update_checkmarks(debugwin_info *info)
{
	if ( info->contextualMenu != NULL )
	{
		MenuItemIndex	menuindex;
		
		disasm_right_column rightcol = disasm_view_get_right_column(info->view[0].view);
				
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_SHOW_RAW, 1, NULL, &menuindex );
		CheckMenuItem(info->contextualMenu, menuindex, (rightcol == DASM_RIGHTCOL_RAW));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_SHOW_ENCRYPTED, 1, NULL, &menuindex );
		CheckMenuItem(info->contextualMenu, menuindex, (rightcol == DASM_RIGHTCOL_ENCRYPTED));
		GetIndMenuItemWithCommandID( info->contextualMenu, ID_SHOW_COMMENTS, 1, NULL, &menuindex );
		CheckMenuItem(info->contextualMenu, menuindex, (rightcol == DASM_RIGHTCOL_COMMENTS));
	}
}



//============================================================
//  disasm_handle_command
//============================================================

static int disasm_handle_command(debugwin_info *info, EventRef inEvent)
{
	HICommand	cmd;
	char command[64];

	if ( GetEventParameter( inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd ) != noErr )
		return global_handle_command(info, inEvent);

	switch( cmd.commandID )
	{
		case OSX_POPUP_COMMAND:
		{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			int	sel = HIViewGetValue(info->otherwnd[0]);
#else // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			int	sel = GetControl32BitValue(info->otherwnd[0]);
#endif // MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
			
			if ( sel > 0 )
			{
				disasm_view_set_subview(info->view[0].view, sel-1);
				disasm_update_caption(info);

				// reset the focus
				osx_SetFocus(info->focuswnd);
				return 1;
			}
		}
		break;
				
		case ID_SHOW_RAW:
			disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_RAW);
			(*info->recompute_children)(info);
			return 1;
		
		case ID_SHOW_ENCRYPTED:
			disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_ENCRYPTED);
			(*info->recompute_children)(info);
			return 1;

		case ID_SHOW_COMMENTS:
			disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_COMMENTS);
			(*info->recompute_children)(info);
			return 1;

		case ID_RUN_TO_CURSOR:
			if (debug_view_get_cursor_visible(info->view[0].view))
			{
				const address_space *space = disasm_view_get_current_subview(info->view[0].view)->space;
				if (debug_cpu_get_visible_cpu(info->machine) == space->cpu)
				{
					offs_t address = memory_byte_to_address(space, disasm_view_get_selected_address(info->view[0].view));
					sprintf(command, "go %X", address);
					debug_console_execute_command(info->machine, command, 1);
				}
			}
			return 1;

		case ID_TOGGLE_BREAKPOINT:
			if (debug_view_get_cursor_visible(info->view[0].view))
			{
				const address_space *space = disasm_view_get_current_subview(info->view[0].view)->space;
				if (debug_cpu_get_visible_cpu(info->machine) == space->cpu)
				{
					offs_t address = memory_byte_to_address(space, disasm_view_get_selected_address(info->view[0].view));
					cpu_debug_data *cpuinfo = cpu_get_debug_data(space->cpu);
					debug_cpu_breakpoint *bp;
					INT32 bpindex = -1;

					/* first find an existing breakpoint at this address */
					for (bp = cpuinfo->bplist; bp != NULL; bp = bp->next)
						if (address == bp->address)
						{
							bpindex = bp->index;
							break;
						}

					/* if it doesn't exist, add a new one */
					if (bpindex == -1)
						sprintf(command, "bpset %X", address);
					else
						sprintf(command, "bpclear %X", bpindex);
					debug_console_execute_command(info->machine, command, 1);
				}
			}
			return 1;
	}
	
	return global_handle_command(info, inEvent);
}

//============================================================
//  disasm_handle_key
//============================================================

static int disasm_handle_key(debugwin_info *info, EventRef inEvent)
{
	EventRecord		event;
	char command[64];

	if ( ConvertEventRefToEventRecord( inEvent, &event ) )
	{
		if ( event.modifiers & cmdKey )
		{
			switch( event.message & charCodeMask )
			{
				case 'r':
				case 'R':
					disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_RAW);
					(*info->recompute_children)(info);
					return 1;
				
				case 'e':
				case 'E':
					disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_ENCRYPTED);
					(*info->recompute_children)(info);
					return 1;
				
				case 'n':
				case 'N':
					disasm_view_set_right_column(info->view[0].view, DASM_RIGHTCOL_COMMENTS);
					(*info->recompute_children)(info);
					return 1;
			}
		}
		
		switch( ( ( event.message & keyCodeMask ) >> 8 ) & 0xFF )
		{
			case KEY_F4:
				if (debug_view_get_cursor_visible(info->view[0].view))
				{
					const address_space *space = disasm_view_get_current_subview(info->view[0].view)->space;
					if (debug_cpu_get_visible_cpu(info->machine) == space->cpu)
					{
						offs_t address = memory_byte_to_address(space, disasm_view_get_selected_address(info->view[0].view));
						sprintf(command, "go %X", address);
						debug_console_execute_command(info->machine, command, 1);
					}
				}
				return 1;
			
			case KEY_F9:
				if (debug_view_get_cursor_visible(info->view[0].view))
				{
					const address_space *space = disasm_view_get_current_subview(info->view[0].view)->space;
					if (debug_cpu_get_visible_cpu(info->machine) == space->cpu)
					{
						offs_t address = memory_byte_to_address(space, disasm_view_get_selected_address(info->view[0].view));
						cpu_debug_data *cpuinfo = cpu_get_debug_data(space->cpu);
						debug_cpu_breakpoint *bp;
						INT32 bpindex = -1;

						/* first find an existing breakpoint at this address */
						for (bp = cpuinfo->bplist; bp != NULL; bp = bp->next)
							if (address == bp->address)
							{
								bpindex = bp->index;
								break;
							}

						/* if it doesn't exist, add a new one */
						if (bpindex == -1)
							sprintf(command, "bpset %X", address);
						else
							sprintf(command, "bpclear %X", bpindex);
						debug_console_execute_command(info->machine, command, 1);
					}
				}
				return 1;
		}
		
		if ( ( event.message & charCodeMask ) == kReturnCharCode )
		{
			if (debug_view_get_cursor_visible(info->view[0].view))
			{
				debug_cpu_single_step(info->machine, 1);
				return 1;
			}
		}
	}

	return global_handle_key(info, inEvent);
}



//============================================================
//  disasm_update_caption
//============================================================

static void disasm_update_caption(debugwin_info *info)
{
	const disasm_subview_item *subview = disasm_view_get_current_subview(info->view[0].view);
	CFStringRef		cftitle;
	char			title[100];

	// update the caption
	snprintf(title, ARRAY_LENGTH(title), "Disassembly: %s", subview->name);

	cftitle = CFStringCreateWithCString( NULL, title, CFStringGetSystemEncoding() );
	
	if ( cftitle )
	{
		SetWindowTitleWithCFString( info->wnd, cftitle);
		CFRelease( cftitle );
	}
}



//============================================================
//  console_create_window
//============================================================

void console_create_window(running_machine *machine)
{
	const registers_subview_item *regsubview;
	const disasm_subview_item *dasmsubview;
	debugwin_info *info;
	int bestwidth, bestheight;
	Rect bounds, work_bounds;
	MenuRef optionsmenu, popupmenu;
	ControlFontStyleRec		style;
	Str255		fontName;
	CGDirectDisplayID	mainID = CGMainDisplayID();
	MenuItemIndex	menuIndex;
	ControlButtonContentInfo	content;

	// create the window
	info = debug_window_create(machine, "Debug", NULL);
	if (info == NULL)
		return;
	main_console = info;
	console_set_cpu(machine->firstcpu);

	// create the views
	if (!debugwin_view_create(info, 0, DVT_DISASSEMBLY))
		goto cleanup;
	if (!debugwin_view_create(info, 1, DVT_REGISTERS))
		goto cleanup;
	if (!debugwin_view_create(info, 2, DVT_CONSOLE))
		goto cleanup;

	// create the options menu
	if ( CreateNewMenu( 0, 0, &optionsmenu ) != noErr )
	{
		mame_printf_error( "Could not create menu\n" );
		exit( -1 );
	}
	
	popupmenu = create_standard_menubar();
	
	if ( popupmenu )
	{
		MenuItemIndex	menuindex;
		AppendMenuItemTextWithCFString(optionsmenu, CFSTR("Main Options"), 0, 0, &menuindex);
		SetMenuItemHierarchicalMenu(optionsmenu, menuindex, popupmenu);
	}
	
	{
		EventTypeSpec	spec = { kEventClassCommand, kEventCommandProcess };
	
		InstallEventHandler( GetMenuEventTarget( popupmenu ), NewEventHandlerUPP(debug_window_proc), 1, &spec, info, NULL );
	}
		
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Set breakpoint at cursor"), 0, ID_TOGGLE_BREAKPOINT, &menuIndex );
	SetMenuItemKeyGlyph(optionsmenu, menuIndex, kMenuF9Glyph); SetMenuItemModifiers(optionsmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Run to cursor"), 0, ID_RUN_TO_CURSOR, &menuIndex );
	SetMenuItemKeyGlyph(optionsmenu, menuIndex, kMenuF4Glyph); SetMenuItemModifiers(optionsmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Raw opcodes"), 0, ID_SHOW_RAW, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'R');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Encrypted opcodes"), 0, ID_SHOW_ENCRYPTED, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'E');
	AppendMenuItemTextWithCFString( optionsmenu, CFSTR("Comments"), 0, ID_SHOW_COMMENTS, &menuIndex );
	SetMenuItemCommandKey(optionsmenu, menuIndex, FALSE, 'C');
	info->contextualMenu = optionsmenu;
	disasm_update_checkmarks(info);
	
	// set the handlers
	info->handle_command = disasm_handle_command;
	info->handle_key = disasm_handle_key;

	// lock us to the bottom of the console by default
	info->view[2].is_textbuf = TRUE;

	// set up the disassembly view to track the current pc
	disasm_view_set_expression(info->view[0].view, "curpc");
	
	fontName[0] = strlen( OSX_DEBUGGER_FONT_NAME );
	strcpy( (char*)&fontName[1], OSX_DEBUGGER_FONT_NAME );

	style.flags = kControlUseFontMask | kControlUseSizeMask;
	style.font = ATSFontFamilyFindFromQuickDrawName(fontName);
	style.size = OSX_DEBUGGER_FONT_SIZE;
	
	// create an edit box and override its key handling
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateEditUnicodeTextControl(info->wnd, &bounds, CFSTR("0"), false, NULL, &info->editwnd);
	SetControlFontStyle(info->editwnd,&style);
	InstallEventHandler( GetControlEventTarget(info->editwnd), NewEventHandlerUPP(debugwin_edit_proc), sizeof( debug_keyboard_events ) / sizeof( EventTypeSpec ), debug_keyboard_events, info, NULL );

	// create popup menu and attach the contextual menu
	content.contentType = kControlContentTextOnly;
	bounds.top = bounds.left = 0;
	bounds.bottom = bounds.right = 100;
	CreateBevelButtonControl(info->wnd, &bounds, CFSTR(""), kControlBevelButtonSmallBevel, kControlBehaviorPushbutton, &content, 1, kControlBehaviorMultiValueMenu, kControlBevelButtonMenuOnBottom, &info->menuwnd );
	SetControlData(info->menuwnd, kControlEntireControl, kControlBevelButtonMenuHandleTag, sizeof(optionsmenu), &optionsmenu);
	
	// set the child functions
	info->recompute_children = console_recompute_children;
	info->process_string = console_process_string;

	// loop over all CPUs and compute the sizes
	info->minwidth = 0;
	info->maxwidth = 0;
	for (dasmsubview = disasm_view_get_subview_list(info->view[0].view); dasmsubview != NULL; dasmsubview = dasmsubview->next)
		for (regsubview = registers_view_get_subview_list(info->view[1].view); regsubview != NULL; regsubview = regsubview->next)
			if (dasmsubview->space->cpu == regsubview->device)
			{
				UINT32 regchars, dischars, conchars;
				UINT32 minwidth, maxwidth;

				// point all views to the new CPU number
				disasm_view_set_subview(info->view[0].view, dasmsubview->index);
				registers_view_set_subview(info->view[1].view, regsubview->index);

				// get the total width of all three children
				dischars = debug_view_get_total_size(info->view[0].view).x;
				regchars = debug_view_get_total_size(info->view[1].view).x;
				conchars = debug_view_get_total_size(info->view[2].view).x;

				// compute the preferred width
				minwidth = EDGE_WIDTH + regchars * debug_font_width + vscroll_width + 2 * EDGE_WIDTH + 100 + EDGE_WIDTH;
				maxwidth = EDGE_WIDTH + regchars * debug_font_width + vscroll_width + 2 * EDGE_WIDTH + ((dischars > conchars) ? dischars : conchars) * debug_font_width + vscroll_width + EDGE_WIDTH;
				if (minwidth > info->minwidth)
					info->minwidth = minwidth;
				if (maxwidth > info->maxwidth)
					info->maxwidth = maxwidth;
			}

	// get the work bounds
	{
#if OSX_LEOPARD
		HIRect		mainBounds;
		HIWindowGetAvailablePositioningBounds( mainID, kHICoordSpaceScreenPixel, &mainBounds );
		work_bounds.left = mainBounds.origin.x;
		work_bounds.top = mainBounds.origin.y;
		work_bounds.right = mainBounds.origin.x + mainBounds.size.width;
		work_bounds.bottom = mainBounds.origin.y + mainBounds.size.height;
#else
		GDHandle	mainDevice;
		DMGetGDeviceByDisplayID((DisplayIDType)mainID, &mainDevice, TRUE);
		GetAvailableWindowPositioningBounds(mainDevice, &work_bounds);
#endif
}

	// adjust the min/max sizes for the window style
	bounds.top = bounds.left = 0;
	bounds.right = bounds.bottom = info->minwidth;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);
	info->minwidth = bounds.right - bounds.left;

	bounds.top = bounds.left = 0;
	bounds.right = bounds.bottom = info->maxwidth;
//	AdjustWindowRectEx(&bounds, DEBUG_WINDOW_STYLE, FALSE, DEBUG_WINDOW_STYLE_EX);
	info->maxwidth = bounds.right - bounds.left;

	// position the window at the bottom-right
	bestwidth = (info->maxwidth < (work_bounds.right - work_bounds.left)) ? info->maxwidth : (work_bounds.right - work_bounds.left);
	bestheight = (500 < (work_bounds.bottom - work_bounds.top)) ? 500 : (work_bounds.bottom - work_bounds.top);

	bounds.left = work_bounds.right - bestwidth;
	bounds.top = work_bounds.bottom - bestheight;
	bounds.right = bounds.left + bestwidth;
	bounds.bottom = bounds.top + bestheight;

	SetWindowBounds(info->wnd, kWindowStructureRgn, &bounds );

	// recompute the children
	console_recompute_children(info);

	// mark the edit box as the default focus and set it
	info->focuswnd = info->editwnd;
	osx_SetFocus(info->editwnd);
	return;

cleanup:
	if (info->view[2].view)
		debug_view_free(info->view[2].view);
	if (info->view[1].view)
		debug_view_free(info->view[1].view);
	if (info->view[0].view)
		debug_view_free(info->view[0].view);
}



//============================================================
//  console_recompute_children
//============================================================

static void console_recompute_children(debugwin_info *info)
{
	Rect parent, regrect, disrect, conrect, editrect, actionrect;
	UINT32 regchars, dischars, conchars;
	HIRect		hibounds;

	// get the parent's dimensions
	HIViewGetBounds(info->contentView,&hibounds);
	parent.top = hibounds.origin.y;
	parent.bottom = hibounds.origin.y + hibounds.size.height;
	parent.left = hibounds.origin.x;
	parent.right = hibounds.origin.x + hibounds.size.width;
	
	// get the total width of all three children
	dischars = debug_view_get_total_size(info->view[0].view).x;
	regchars = debug_view_get_total_size(info->view[1].view).x;
	conchars = debug_view_get_total_size(info->view[2].view).x;

	// registers always get their desired width, and span the entire height
	regrect.top = parent.top + EDGE_WIDTH;
	regrect.bottom = parent.bottom - EDGE_WIDTH;
	regrect.left = parent.left + EDGE_WIDTH;
	regrect.right = regrect.left + debug_font_width * regchars + vscroll_width;
	
	// If there is no room for the full register width, force them to half the window
	if( regrect.right > parent.right )
		regrect.right = parent.right / 2;

	// edit box goes at the bottom of the remaining area (minus a little for a popup menu)
	editrect.bottom = parent.bottom - EDGE_WIDTH;
	editrect.top = editrect.bottom - debug_font_height - 4;
	editrect.left = regrect.right + EDGE_WIDTH * 2;
	editrect.right = parent.right - EDGE_WIDTH - 35;
	
	// Popup button goes to the right of the edit box
	actionrect.top = editrect.top-1;
	actionrect.bottom = editrect.bottom+1;
	actionrect.left = editrect.right + EDGE_WIDTH;
	actionrect.right = parent.right - 16;

	// console and disassembly split the difference
	disrect.top = parent.top + EDGE_WIDTH;
	disrect.bottom = (editrect.top - parent.top) / 2 - EDGE_WIDTH;
	disrect.left = regrect.right + EDGE_WIDTH * 2;
	disrect.right = parent.right - EDGE_WIDTH;

	conrect.top = disrect.bottom + EDGE_WIDTH * 2;
	conrect.bottom = editrect.top - EDGE_WIDTH;
	conrect.left = regrect.right + EDGE_WIDTH * 2;
	conrect.right = parent.right - EDGE_WIDTH;

	// set the bounds of things
	debugwin_view_set_bounds(&info->view[0], &disrect);
	debugwin_view_set_bounds(&info->view[1], &regrect);
	debugwin_view_set_bounds(&info->view[2], &conrect);
	
	osx_RectToHIRect( &editrect, &hibounds );
	smart_set_view_bounds(info->editwnd, &hibounds);

	osx_RectToHIRect( &actionrect, &hibounds );
	smart_set_view_bounds(info->menuwnd, &hibounds);
}



//============================================================
//  console_process_string
//============================================================

static void console_process_string(debugwin_info *info, const char *string)
{
	char buffer = 0;

	// an empty string is a single step
	if (string[0] == 0)
		debug_cpu_single_step(info->machine, 1);

	// otherwise, just process the command
	else
		debug_console_execute_command(info->machine, string, 1);

	// clear the edit text box
	SetControlData(info->editwnd, kControlEntireControl, kControlEditTextTextTag, 0, &buffer);
}



//============================================================
//  console_set_cpu
//============================================================

static void console_set_cpu(const device_config *device)
{
	const registers_subview_item *regsubitem = NULL;
	const disasm_subview_item *dasmsubitem;
	char title[256];
	CFStringRef		cftitle;

	// first set all the views to the new cpu number
	if (main_console->view[0].view != NULL)
		for (dasmsubitem = disasm_view_get_subview_list(main_console->view[0].view); dasmsubitem != NULL; dasmsubitem = dasmsubitem->next)
			if (dasmsubitem->space->cpu == device)
			{
				disasm_view_set_subview(main_console->view[0].view, dasmsubitem->index);
				break;
			}
	if (main_console->view[1].view != NULL)
		for (regsubitem = registers_view_get_subview_list(main_console->view[1].view); regsubitem != NULL; regsubitem = regsubitem->next)
			if (regsubitem->device == device)
			{
				registers_view_set_subview(main_console->view[1].view, regsubitem->index);
				break;
			}

	// then update the caption
	if (regsubitem != NULL)
	{
		snprintf(title, ARRAY_LENGTH(title), "Debug: %s - %s", device->machine->gamedrv->name, regsubitem->name);
	
		cftitle = CFStringCreateWithCString( NULL, title, CFStringGetSystemEncoding() );
	
		if ( cftitle )
		{
			SetWindowTitleWithCFString( main_console->wnd, cftitle);
			CFRelease( cftitle );
		}
	}
}



//============================================================
//  create_standard_menubar
//============================================================

static MenuRef create_standard_menubar(void)
{
	MenuRef debugmenu;
	MenuItemIndex	menuIndex;

	// create the debug menu
	if ( CreateNewMenu( 0, 0, &debugmenu ) != noErr )
	{
		return NULL;
	}

	AppendMenuItemTextWithCFString( debugmenu, CFSTR("New Memory Window"), 0, ID_NEW_MEMORY_WND, &menuIndex );
	SetMenuItemCommandKey(debugmenu, menuIndex, FALSE, 'M');
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("New Disassembly Window"), 0, ID_NEW_DISASM_WND, &menuIndex );
	SetMenuItemCommandKey(debugmenu, menuIndex, FALSE, 'D');
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("New Error Log Window"), 0, ID_NEW_LOG_WND, &menuIndex );
	SetMenuItemCommandKey(debugmenu, menuIndex, FALSE, 'L');
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Run"), 0, ID_RUN, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF5Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Run and Hide Debugger"), 0, ID_RUN_AND_HIDE, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF12Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Run to Next CPU"), 0, ID_NEXT_CPU, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF6Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Run until Next Interrupt on This CPU"), 0, ID_RUN_IRQ, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF7Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Run until Next VBLANK"), 0, ID_RUN_VBLANK, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF8Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Step Into"), 0, ID_STEP, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF11Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Step Over"), 0, ID_STEP_OVER, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF10Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Step Out"), 0, ID_STEP_OUT, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF10Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuShiftModifier | kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL );
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Soft Reset"), 0, ID_SOFT_RESET, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF3Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Hard Reset"), 0, ID_HARD_RESET, &menuIndex );
	SetMenuItemKeyGlyph(debugmenu, menuIndex, kMenuF3Glyph); SetMenuItemModifiers(debugmenu, menuIndex, kMenuShiftModifier | kMenuNoCommandModifier);
	AppendMenuItemTextWithCFString( debugmenu, CFSTR("Exit"), 0, ID_EXIT, &menuIndex );

	return debugmenu;
}



//============================================================
//  global_handle_command
//============================================================

static int global_handle_command(debugwin_info *info, EventRef inEvent)
{
	HICommand	cmd;

	if ( GetEventParameter( inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd ) != noErr )
		return 0;

	switch( cmd.commandID )
	{
		case ID_NEW_MEMORY_WND:
			memory_create_window(info->machine);
			return 1;

		case ID_NEW_DISASM_WND:
			disasm_create_window(info->machine);
			return 1;

		case ID_NEW_LOG_WND:
			log_create_window(info->machine);
			return 1;

		case ID_RUN_AND_HIDE:
			smart_show_all(FALSE);
		case ID_RUN:
			debug_cpu_go(info->machine, ~0);
			return 1;

		case ID_NEXT_CPU:
			debug_cpu_next_cpu(info->machine);
			return 1;

		case ID_RUN_VBLANK:
			debug_cpu_go_vblank(info->machine);
			return 1;

		case ID_RUN_IRQ:
			debug_cpu_go_interrupt(info->machine, -1);
			return 1;

		case ID_STEP:
			debug_cpu_single_step(info->machine, 1);
			return 1;

		case ID_STEP_OVER:
			debug_cpu_single_step_over(info->machine, 1);
			return 1;

		case ID_STEP_OUT:
			debug_cpu_single_step_out(info->machine);
			return 1;

		case ID_HARD_RESET:
			mame_schedule_hard_reset(info->machine);
			return 1;

		case ID_SOFT_RESET:
			mame_schedule_soft_reset(info->machine);
			debug_cpu_go(info->machine, ~0);
			return 1;

		case ID_EXIT:
			mame_schedule_exit(info->machine);
			return 1;
	}
	
	return 0;
}



//============================================================
//  global_handle_key
//============================================================

static int global_handle_key(debugwin_info *info, EventRef inEvent)
{
	EventRecord		event;
	int ignoreme;

	/* ignore any keys that are received while the debug key is down */
	ignoreme = ui_input_pressed(info->machine, IPT_UI_DEBUG_BREAK);
	if (ignoreme)
		return 1;

	if ( ConvertEventRefToEventRecord( inEvent, &event ) )
	{
		switch( ( ( event.message & keyCodeMask ) >> 8 ) & 0xFF )
		{
			case KEY_F3:
				if ( event.modifiers & shiftKey )
					mame_schedule_hard_reset(info->machine);
				else
				{
					mame_schedule_soft_reset(info->machine);
					debug_cpu_go(info->machine,~0);
				}
				return 1;
			
			case KEY_F4:
				if ( event.modifiers & optionKey )
					mame_schedule_exit(info->machine);
				return 1;
				
			case KEY_F5:
				debug_cpu_go(info->machine, ~0);
				return 1;
			
			case KEY_F6:
				debug_cpu_next_cpu(info->machine);
				return 1;
			
			case KEY_F7:
				debug_cpu_go_interrupt(info->machine, -1);
				return 1;
			
			case KEY_F8:
				debug_cpu_go_vblank(info->machine);
				return 1;
			
			case KEY_F10:
				debug_cpu_single_step_over(info->machine, 1);
				return 1;
			
			case KEY_F11:
				if ( event.modifiers & shiftKey )
					debug_cpu_single_step_out(info->machine);
				else
					debug_cpu_single_step(info->machine, 1);
				return 1;
			
			case KEY_F12:
				smart_show_all(FALSE);
				debug_cpu_go(info->machine, ~0);
				return 1;
		}
		
		if ( event.modifiers & cmdKey )
		{
			switch( event.message & charCodeMask )
			{
				case 'm':
				case 'M':
					memory_create_window(info->machine);
					return 1;
				
				case 'd':
				case 'D':
					disasm_create_window(info->machine);
					return 1;
				
				case 'l':
				case 'L':
					log_create_window(info->machine);
					return 1;
			}
		}
	}
	
	return 0;			
}


//============================================================
//  smart_set_window_bounds
//============================================================

static void smart_set_view_bounds(HIViewRef wnd, HIRect *bounds)
{
	HIRect		finalRect;

	memcpy( &finalRect, bounds, sizeof( HIRect ) );
	
	if ( finalRect.size.width < 0 )
		finalRect.size.width = 0;
	
	if ( finalRect.size.height < 0 )
		finalRect.size.height = 0;

	HIViewSetFrame( wnd, &finalRect );
}


//============================================================
//  smart_show_window
//============================================================

static void smart_show_window(WindowRef wnd, int show)
{
	int	select = 0;

	if ( show && main_console && main_console->wnd && main_console->wnd == wnd && IsWindowVisible( wnd ) == false )
	{
		select = 1;
	}

	ShowHide(wnd, show ? TRUE : FALSE);

	if ( select )
	{
		SelectWindow( wnd );
	}
}

//============================================================
//  smart_show_view
//============================================================

static void smart_show_view(HIViewRef wnd, int show)
{
	HIViewSetVisible(wnd, show);
}


//============================================================
//  smart_show_all
//============================================================

static void smart_show_all(int show)
{
	debugwin_info *info;
	for (info = window_list; info; info = info->next)
		smart_show_window(info->wnd, show);
}
