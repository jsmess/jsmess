#include <stdlib.h>
#include "mame.h"
#include "osdepend.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef MAME_DEBUG
#include "debug/debugcpu.h"
#endif /* MAME_DEBUG */

#ifdef UNUSED_FUNCTION
int osd_create_display(const osd_create_params *params, UINT32 *rgb_components)
{
	rgb_components[0] = 0xff0000;
	rgb_components[1] = 0x00ff00;
	rgb_components[2] = 0x0000ff;
	return 0;
}

void osd_close_display(void)
{
}

int osd_skip_this_frame(void)
{
	return 0;
}

mame_bitmap *osd_override_snapshot(mame_bitmap *bitmap, rectangle *bounds)
{
	return NULL;
}
#endif

void osd_set_mastervolume(int attenuation)
{
}

#ifdef UNUSED_FUNCTION
int osd_get_mastervolume(void)
{
	return 0;
}

void osd_sound_enable(int enable)
{
}

int osd_is_code_pressed(int code)
{
	return 0;
}

int osd_is_key_pressed(int keycode)
{
	return 0;
}

int osd_readkey_unicode(int flush)
{
	return 0;
}

int osd_is_joystick_axis_code(int joycode)
{
	return 0;
}

int osd_joystick_needs_calibration(void)
{
	return 0;
}

void osd_joystick_start_calibration(void)
{
}

const char *osd_joystick_calibrate_next(void)
{
	return NULL;
}

void osd_joystick_calibrate(void)
{
}

void osd_joystick_end_calibration(void)
{
}

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
}

void osd_trak_read(int player, int *deltax, int *deltay)
{
}
#endif

void osd_customize_inputport_list(input_port_default_entry *defaults)
{
}

#ifdef UNUSED_FUNCTION
void osd_logerror(const char *text)
{
}
#endif

int osd_keyboard_disabled(void)
{
	return 0;
}

#ifdef UNUSED_FUNCTION
void osd_begin_final_unloading(void)
{
}

void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
}
#endif

#ifdef MAME_DEBUG
void osd_wait_for_debugger(void)
{
	debug_cpu_go(~0);
}
#endif // MAME_DEBUG

#ifdef UNUSED_FUNCTION
int win_mess_config_init(void)
{
	return 0;
}
#endif

void osd_break_into_debugger(const char *message)
{
}

#ifdef UNUSED_FUNCTION
void win_mess_options_init(void)
{
}
#endif

//============================================================
//	osd_alloc_executable
//============================================================

void *osd_alloc_executable(size_t size)
{
#ifdef WIN32
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
	return malloc(size);
#endif
}



//============================================================
//	osd_free_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
#ifdef WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	free(ptr);
#endif
}



//============================================================
//	osd_is_bad_read_ptr
//============================================================

int osd_is_bad_read_ptr(const void *ptr, size_t size)
{
#ifdef WIN32
	return IsBadReadPtr(ptr, size);
#else
	return !ptr;
#endif
}


void osd_get_emulator_directory(char *dir, size_t dir_size)
{
}



void osd_mess_options_init(core_options *options)
{
}


#ifdef UNUSED_FUNCTION
void osd_mess_config_init(running_machine *machine)
{
}
#endif
