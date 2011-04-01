#ifndef __VIC20__
#define __VIC20__

#include "machine/6522via.h"

#define SCREEN_TAG		"screen"
#define M6502_TAG		"ue10"
#define M6522_0_TAG		"uab3"
#define M6522_1_TAG		"uab1"
#define M6560_TAG		"ub7"
#define M6561_TAG		"ub7"
#define C1540_TAG		"c1540"
#define C2031_TAG		"c2031"
#define IEC_TAG			"iec"
#define IEEE488_TAG		"ieee488"
#define CASSETTE_TAG	"cassette"
#define TIMER_C1530_TAG	"c1530"

class vic20_state : public driver_device
{
public:
	vic20_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int m_key_col;

	/* devices */
	via6522_device *m_via0;
	via6522_device *m_via1;
	device_t *m_iec;
	device_t *m_cassette;
	device_t *m_mos6560;

	/* timers */
	timer_device *m_cassette_timer;
};

#endif
