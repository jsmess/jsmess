#include <stdlib.h>
#include "mame.h"
#include "osdepend.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef ENABLE_DEBUGGER
#include "cpuintrf.h"
#include "debug/debugcpu.h"
#endif /* ENABLE_DEBUGGER */

void osd_set_mastervolume(int attenuation)
{
}

void osd_customize_input_type_list(input_type_desc *typelist)
{
}

int osd_keyboard_disabled(void)
{
	return 0;
}

#ifdef ENABLE_DEBUGGER
void osd_wait_for_debugger(void)
{
	debug_cpu_go(~0);
}
#endif // ENABLE_DEBUGGER

void osd_break_into_debugger(const char *message)
{
}


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
void osd_paste(running_machine *machine)
{
}
#endif
