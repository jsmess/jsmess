#include <stdlib.h>
#include "emu.h"
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
