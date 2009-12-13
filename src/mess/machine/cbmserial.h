/**********************************************************************

    Commodore Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __CBMSERIAL__
#define __CBMSERIAL__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBMSERIAL DEVICE_GET_INFO_NAME( cbmserial )

#define MDRV_CBMSERIAL_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, CBMSERIAL, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define CBMSERIAL_DAISY(_name) \
	const cbmserial_daisy_chain (_name)[] =

enum
{
	DEVINFO_FCT_CBM_SERIAL_SRQ = DEVINFO_FCT_DEVICE_SPECIFIC,
	DEVINFO_FCT_CBM_SERIAL_ATN,
	DEVINFO_FCT_CBM_SERIAL_CLK,
	DEVINFO_FCT_CBM_SERIAL_DATA,
	DEVINFO_FCT_CBM_SERIAL_RESET
};

#define CBMSERIAL_SRQ_NAME(name)	cbmserial_srq_##name
#define CBMSERIAL_SRQ(name)			void CBMSERIAL_SRQ_NAME(name)(const device_config *device, int state)

#define CBMSERIAL_ATN_NAME(name)	cbmserial_atn_##name
#define CBMSERIAL_ATN(name)			void CBMSERIAL_ATN_NAME(name)(const device_config *device, int state)

#define CBMSERIAL_CLK_NAME(name)	cbmserial_clk_##name
#define CBMSERIAL_CLK(name)			void CBMSERIAL_CLK_NAME(name)(const device_config *device, int state)

#define CBMSERIAL_DATA_NAME(name)	cbmserial_data_##name
#define CBMSERIAL_DATA(name)		void CBMSERIAL_DATA_NAME(name)(const device_config *device, int state)

#define CBMSERIAL_RESET_NAME(name)	cbmserial_reset_##name
#define CBMSERIAL_RESET(name)		void CBMSERIAL_RESET_NAME(name)(const device_config *device, int state)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbmserial_daisy_chain cbmserial_daisy_chain;
struct _cbmserial_daisy_chain
{
	const char *tag;	/* device tag */
};

typedef void (*cbmserial_line)(const device_config *device, int state);

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cbmserial );

/* serial service request */
void cbmserial_srq_w(const device_config *serial_bus_device, const device_config *calling_device, int state);
READ_LINE_DEVICE_HANDLER( cbmserial_srq_r );

/* attention */
void cbmserial_atn_w(const device_config *serial_bus_device, const device_config *calling_device, int state);
READ_LINE_DEVICE_HANDLER( cbmserial_atn_r );

/* clock */
void cbmserial_clk_w(const device_config *serial_bus_device, const device_config *calling_device, int state);
READ_LINE_DEVICE_HANDLER( cbmserial_clk_r );

/* data */
void cbmserial_data_w(const device_config *serial_bus_device, const device_config *calling_device, int state);
READ_LINE_DEVICE_HANDLER( cbmserial_data_r );

/* reset */
void cbmserial_reset_w(const device_config *serial_bus_device, const device_config *calling_device, int state);
READ_LINE_DEVICE_HANDLER( cbmserial_reset_r );

#endif
