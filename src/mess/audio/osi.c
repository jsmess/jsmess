#include "driver.h"
#include "sound/discrete.h"

const discrete_dac_r1_ladder sb2m600_dac =
{
	4,			// size of ladder
	{180, 180, 180, 180, 0, 0, 0, 0},	// R68, R69, R70, R71
	5,			// 5V
	RES_K(1),	// R67
	0,			// no rGnd
	0			// no cFilter
};

DISCRETE_SOUND_START(sb2m600_discrete_interface)
	DISCRETE_INPUT_DATA(NODE_01)
	DISCRETE_DAC_R1(NODE_02, 1, NODE_01, DEFAULT_TTL_V_LOGIC_1, &sb2m600_dac)
	DISCRETE_CRFILTER(NODE_03, 1, NODE_02, 1.0/(1.0/RES_K(1)+1.0/180+1.0/180+1.0/180+1.0/180), CAP_U(0.1))
	DISCRETE_OUTPUT(NODE_03, 100)
	DISCRETE_GAIN(NODE_04, NODE_03, 32767.0/5)
	DISCRETE_OUTPUT(NODE_04, 100)
DISCRETE_SOUND_END
