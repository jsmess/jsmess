/*****************************************************************************
 *
 * machine/74145.h
 *
 ****************************************************************************/

#ifndef TTL74145_H_
#define TTL74145_H_

#include "driver.h"

/* Configuration */
void ttl74145_reset(int which);

/* Output pin states */
int ttl74145_output_0(int which);
int ttl74145_output_1(int which);
int ttl74145_output_2(int which);
int ttl74145_output_3(int which);
int ttl74145_output_4(int which);
int ttl74145_output_5(int which);
int ttl74145_output_6(int which);
int ttl74145_output_7(int which);
int ttl74145_output_8(int which);
int ttl74145_output_9(int which);

/* Standard handlers */
WRITE8_HANDLER( ttl74145_0_w );

#endif /*TTL74145_H_*/
