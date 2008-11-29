/*********************************************************************

	8530scc.h

	Zilog 8530 SCC (Serial Control Chip) code

*********************************************************************/

#ifndef __8630SCC_H__
#define __8630SCC_H__


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
	void (*acknowledge)(const device_config *device);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void scc_set_status(const device_config *device, int status);

UINT8 scc_get_reg_a(const device_config *device, int reg);
UINT8 scc_get_reg_b(const device_config *device, int reg);
void scc_set_reg_a(const device_config *device, int reg, UINT8 data);
void scc_set_reg_b(const device_config *device, int reg, UINT8 data);

DEVICE_GET_INFO(scc8530);

READ8_DEVICE_HANDLER(scc_r);
WRITE8_DEVICE_HANDLER(scc_w);

#endif /* __8630SCC_H__ */
