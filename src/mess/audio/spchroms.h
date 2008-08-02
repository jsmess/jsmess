#include "mame.h"

typedef struct spchroms_interface
{
	const char *memory_region;			/* memory region where the speech ROM is.  NULL means no speech ROM */
} spchroms_interface;

void spchroms_config(running_machine *machine, const spchroms_interface *intf);

int spchroms_read(int count);
void spchroms_load_address(int data);
void spchroms_read_and_branch(void);
