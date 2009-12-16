/**********************************************************************

    Commodore IEC Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __CBM_IEC__
#define __CBM_IEC__

#include "driver.h"

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CBM_IEC DEVICE_GET_INFO_NAME( cbm_iec )

#define MDRV_CBM_IEC_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, CBM_IEC, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define CBM_IEC_DAISY(_name) \
	const cbm_iec_daisy_chain (_name)[] =

enum
{
	DEVINFO_FCT_CBM_IEC_SRQ = DEVINFO_FCT_DEVICE_SPECIFIC,
	DEVINFO_FCT_CBM_IEC_ATN,
	DEVINFO_FCT_CBM_IEC_CLK,
	DEVINFO_FCT_CBM_IEC_DATA,
	DEVINFO_FCT_CBM_IEC_RESET
};

#define CBM_IEC_SRQ_NAME(name)		cbm_iec_srq_##name
#define CBM_IEC_SRQ(name)			void CBM_IEC_SRQ_NAME(name)(const device_config *device, int state)

#define CBM_IEC_ATN_NAME(name)		cbm_iec_atn_##name
#define CBM_IEC_ATN(name)			void CBM_IEC_ATN_NAME(name)(const device_config *device, int state)

#define CBM_IEC_CLK_NAME(name)		cbm_iec_clk_##name
#define CBM_IEC_CLK(name)			void CBM_IEC_CLK_NAME(name)(const device_config *device, int state)

#define CBM_IEC_DATA_NAME(name)		cbm_iec_data_##name
#define CBM_IEC_DATA(name)			void CBM_IEC_DATA_NAME(name)(const device_config *device, int state)

#define CBM_IEC_RESET_NAME(name)	cbm_iec_reset_##name
#define CBM_IEC_RESET(name)			void CBM_IEC_RESET_NAME(name)(const device_config *device, int state)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbm_iec_daisy_chain cbm_iec_daisy_chain;
struct _cbm_iec_daisy_chain
{
	const char *tag;	/* device tag */
};

typedef void (*cbm_iec_line)(const device_config *device, int state);

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cbm_iec );

/* service request */
void cbm_iec_srq_w(const device_config *iec, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_srq_r );

/* attention */
void cbm_iec_atn_w(const device_config *iec, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_atn_r );

/* clock */
void cbm_iec_clk_w(const device_config *iec, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_clk_r );

/* data */
void cbm_iec_data_w(const device_config *iec, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_data_r );

/* reset */
void cbm_iec_reset_w(const device_config *iec, const device_config *device, int state);
READ_LINE_DEVICE_HANDLER( cbm_iec_reset_r );

#endif
