/*****************************************************************************
 *
 * machine/74145.h
 *
 ****************************************************************************/

#ifndef TTL74145_H_
#define TTL74145_H_

#include "driver.h"

/* Interface */
typedef struct _ttl74145_interface ttl74145_interface;
struct _ttl74145_interface
{
	/* Outputs */
	void (*output_line_0)(int state);
	void (*output_line_1)(int state);
	void (*output_line_2)(int state);
	void (*output_line_3)(int state);
	void (*output_line_4)(int state);
	void (*output_line_5)(int state);
	void (*output_line_6)(int state);
	void (*output_line_7)(int state);
	void (*output_line_8)(int state);
	void (*output_line_9)(int state);
};

/* Configuration */
void ttl74145_config(int which, const ttl74145_interface *intf);
void ttl74145_reset(int which);

/* Standard handlers */
WRITE8_HANDLER( ttl74145_0_w );
READ16_HANDLER( ttl74145_0_r );

#endif /*TTL74145_H_*/
