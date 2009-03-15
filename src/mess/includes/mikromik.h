#ifndef __MIKROMIK__
#define __MIKROMIK__

#define SCREEN_TAG			"screen"
#define I8085_TAG			"i8085"
#define I8275_TAG			"i8275"
#define D7220_TAG			"d7220"

typedef struct _mm1_state mm1_state;
struct _mm1_state
{
	/* devices */
	const device_config		*i8275;
};

#endif
