typedef struct spchroms_interface
{
	int memory_region;			/* memory region where the speech ROM is.  -1 means no speech ROM */
} spchroms_interface;

void spchroms_config(const spchroms_interface *intf);

int spchroms_read(int count);
void spchroms_load_address(int data);
void spchroms_read_and_branch(void);
