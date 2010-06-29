#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdlib.h>
#include "emu.h"
#include "osdepend.h"
void osd_set_mastervolume(int attenuation)
{
}

void osd_customize_input_type_list(input_type_desc *typelist)
{
}

void osd_wait_for_debugger(running_device *device, int firststop)
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
