/*****************************************************************************
 *
 * machine/74145.h
 *
 ****************************************************************************/

#ifndef TTL74145_H_
#define TTL74145_H_

/***************************************************************************
    MACROS
***************************************************************************/

#define TTL74145		DEVICE_GET_INFO_NAME(ttl74145)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*ttl74145_output_line_func)(const device_config *device, int state);
#define TTL74145_OUTPUT_LINE(name)	void name(const device_config *device, int state )


/* Interface */
typedef struct _ttl74145_interface ttl74145_interface;
struct _ttl74145_interface
{
	/* Outputs */
	ttl74145_output_line_func output_line_0;
	ttl74145_output_line_func output_line_1;
	ttl74145_output_line_func output_line_2;
	ttl74145_output_line_func output_line_3;
	ttl74145_output_line_func output_line_4;
	ttl74145_output_line_func output_line_5;
	ttl74145_output_line_func output_line_6;
	ttl74145_output_line_func output_line_7;
	ttl74145_output_line_func output_line_8;
	ttl74145_output_line_func output_line_9;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( ttl74145 );

/* Standard handlers */
WRITE8_DEVICE_HANDLER( ttl74145_w );
READ16_DEVICE_HANDLER( ttl74145_r );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_TTL74145_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, TTL74145, 0) \
	MDRV_DEVICE_CONFIG(_intrf)


#endif /*TTL74145_H_*/
