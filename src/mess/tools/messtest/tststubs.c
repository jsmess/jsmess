#include <stdlib.h>
#include "mame.h"
#include "osdepend.h"

#ifdef WIN32
#include <windows.h>
#endif

void osd_set_mastervolume(int attenuation)
{
}

void osd_customize_input_type_list(input_type_desc *typelist)
{
}

void osd_wait_for_debugger(const device_config *device, int firststop)
{
}

void osd_break_into_debugger(const char *message)
{
}


//============================================================
//  osd_alloc_executable
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
//  osd_free_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
#ifdef WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	free(ptr);
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
