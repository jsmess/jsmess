#ifndef __MICRONIC__
#define __MICRONIC__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define MC146818_TAG	"mc146818"

class micronic_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, micronic_state(machine)); }

	micronic_state(running_machine &machine) { }

	running_device *hd61830;
};

#endif
