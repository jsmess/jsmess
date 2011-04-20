#ifndef __VIC20__
#define __VIC20__

#include "machine/6522via.h"
#include "machine/cbmiec.h"

#define SCREEN_TAG		"screen"
#define M6502_TAG		"ue10"
#define M6522_0_TAG		"uab3"
#define M6522_1_TAG		"uab1"
#define M6560_TAG		"ub7"
#define M6561_TAG		"ub7"
#define IEC_TAG			"iec"
#define IEEE488_TAG		"ieee488"
#define CASSETTE_TAG	"cassette"
#define TIMER_C1530_TAG	"c1530"

class vic20_state : public driver_device
{
public:
	vic20_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M6502_TAG),
		  m_via0(*this, M6522_0_TAG),
		  m_via1(*this, M6522_1_TAG),
		  m_iec(*this, CBM_IEC_TAG),
		  m_cassette(*this, CASSETTE_TAG),
		  m_mos6560(*this, M6560_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<cbm_iec_device> m_iec;
	required_device<device_t> m_cassette;
	required_device<device_t> m_mos6560;

	/* keyboard state */
	int m_key_col;

	/* timers */
	timer_device *m_cassette_timer;
};

#endif
