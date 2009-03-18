#ifndef __MIKROMIK__
#define __MIKROMIK__

#define SCREEN_TAG			"screen"
#define I8085_TAG			"i8085"
#define I8237_TAG			"i8237"
#define I8272_TAG			"i8272"
#define I8275_TAG			"i8275"
#define UPD7220_TAG			"upd7220"

typedef struct _mm1_state mm1_state;
struct _mm1_state
{
	/* devices */
	const device_config		*i8237;
	const device_config		*i8272;
	const device_config		*i8275;
	const device_config		*upd7220;
};

#endif
