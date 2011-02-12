/*********************************************************************

    tc8521.h

    Toshiba TC8251 Real Time Clock code

*********************************************************************/

#ifndef __TC8521_H__
#define __TC8525_H__


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(TC8521, tc8521);

#define MCFG_TC8521_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, TC8521, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MCFG_TC8521_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tc8521_interface tc8521_interface;
struct _tc8521_interface
{
	/* output of alarm */
	void (*alarm_output_callback)(device_t *device, int);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

extern const tc8521_interface default_tc8521_interface;

READ8_DEVICE_HANDLER(tc8521_r);
WRITE8_DEVICE_HANDLER(tc8521_w);

void tc8521_load_stream(device_t *device, emu_file *file);
void tc8521_save_stream(device_t *device, emu_file *file);


#endif /* __TC8521_H__ */
