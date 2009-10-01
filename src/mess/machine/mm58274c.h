#ifndef MM58274C_H
#define MM58274C_H

/***************************************************************************
    MACROS
***************************************************************************/

#define MM58274C		DEVICE_GET_INFO_NAME(mm58274c)

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( mm58274c );

/* interface */
/*
    Initializes the clock chip.
    day1 must be set to a value from 0 (sunday), 1 (monday) ...
    to 6 (saturday) and is needed to correctly retrieve the day-of-week
    from the host system clock.
*/
typedef struct _mm58274c_interface mm58274c_interface;
struct _mm58274c_interface
{
	int	mode24;		/* 24/12 mode */
	int	day1;		/* first day of week */
};

READ8_DEVICE_HANDLER ( mm58274c_r );
WRITE8_DEVICE_HANDLER( mm58274c_w );

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_MM58274C_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, MM58274C, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#endif /* MM58274C_H */
