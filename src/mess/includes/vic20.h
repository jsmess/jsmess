#ifndef __VIC20__
#define __VIC20__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"ue10"
#define M6522_0_TAG		"uab3"
#define M6522_1_TAG		"uab1"
#define M6560_TAG		"ub7"
#define M6561_TAG		"ub7"
#define C1540_TAG		"c1540"
#define IEC_TAG			"iec"
#define CASSETTE_TAG	"cassette"
#define TIMER_C1530_TAG	"c1530"

typedef struct _vic20_state vic20_state;
struct _vic20_state
{
	/* keyboard state */
	int key_col;

	/* devices */
	const device_config *via0;
	const device_config *via1;
	const device_config *iec;
	const device_config *cassette;

	/* timers */
	const device_config *cassette_timer;
};

#endif
