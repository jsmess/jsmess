/*

	X68000 Custom SASI HD controller

*/

#include "driver.h"

enum
{
	SASI_PHASE_BUSFREE = 0,
	SASI_PHASE_ARBITRATION,
	SASI_PHASE_SELECTION,
	SASI_PHASE_RESELECTION,
	SASI_PHASE_COMMAND,
	SASI_PHASE_DATA,
	SASI_PHASE_STATUS,
	SASI_PHASE_MESSAGE
};

typedef struct _sasi_ctrl_t sasi_ctrl_t;
struct _sasi_ctrl_t
{
	int phase;
	unsigned char status_port;  // read at 0xe96003
	unsigned char status;       // status phase output
	unsigned char message;
	unsigned char command[10];
};

DEVICE_START( x68k_hdc );
DEVICE_GET_INFO( x68k_hdc );

#define X68KHDC DEVICE_GET_INFO_NAME(x68k_hdc)

WRITE16_DEVICE_HANDLER( x68k_hdc_w );
READ16_DEVICE_HANDLER( x68k_hdc_r );
