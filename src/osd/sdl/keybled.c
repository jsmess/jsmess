#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#include "output.h"
#include "osdsdl.h"

#if defined(SDLMAME_X11)
static void led_change_notify(const char *outname, INT32 value, void *param);
#endif

void sdlled_init(void)
{
	#if defined(SDLMAME_X11)
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
		
	if ( SDL_GetWMInfo(&info) && (info.subsystem == SDL_SYSWM_X11) )
	{
		/* GRR leds 1 and 2 are swapped in X */
		output_set_notifier(SDL_LED(0), led_change_notify, (void *)2);
		output_set_notifier(SDL_LED(1), led_change_notify, (void *)1);
		output_set_notifier(SDL_LED(2), led_change_notify, (void *)3);
	}
	#endif
}

#if defined(SDLMAME_X11)
static void led_change_notify(const char *outname, INT32 value, void *param)
{
	XKeyboardControl values;
	SDL_SysWMinfo info;
	
	values.led = (long)param;
	if (value)
		values.led_mode = LedModeOn;
	else
		values.led_mode = LedModeOff;
	
	SDL_VERSION(&info.version);
	SDL_GetWMInfo(&info);
	info.info.x11.lock_func();		
	XChangeKeyboardControl(info.info.x11.display, KBLed|KBLedMode, &values);
	info.info.x11.unlock_func();
}
#endif
