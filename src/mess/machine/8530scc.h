/*********************************************************************

    8530scc.h

    Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/

#ifndef __8530SCC_H__
#define __8530SCC_H__


/***************************************************************************
    MACROS
***************************************************************************/

#define SCC8530			DEVICE_GET_INFO_NAME(scc8530)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _scc8530_interface scc8530_interface;
struct _scc8530_interface
{
	void (*acknowledge)(running_device *device);
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_SCC8530_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, SCC8530, 0)

#define MDRV_SCC8530_ACK(_acknowledge) \
	MDRV_DEVICE_CONFIG_DATAPTR(scc8530_interface, acknowledge, _acknowledge)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void scc8530_set_status(running_device *device, int status);

UINT8 scc8530_get_reg_a(running_device *device, int reg);
UINT8 scc8530_get_reg_b(running_device *device, int reg);
void scc8530_set_reg_a(running_device *device, int reg, UINT8 data);
void scc8530_set_reg_b(running_device *device, int reg, UINT8 data);

DEVICE_GET_INFO(scc8530);

READ8_DEVICE_HANDLER(scc8530_r);
WRITE8_DEVICE_HANDLER(scc8530_w);

#endif /* __8530SCC_H__ */
