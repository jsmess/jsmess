#ifndef __SPCHROMS_H
#define __SPCHROMS_H

typedef struct spchroms_interface
{
	const char *memory_region;			/* memory region where the speech ROM is.  NULL means no speech ROM */
} spchroms_interface;

void spchroms_config(running_machine *machine, const spchroms_interface *intf);

int spchroms_read(running_device *device, int count);
void spchroms_load_address(running_device *device, int data);
void spchroms_read_and_branch(running_device *device);

#endif
