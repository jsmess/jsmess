/***************************************************************************

	machine/nec765.h

	Functions to emulate a NEC765/Intel 8272 compatible floppy disk controller

***************************************************************************/

#ifndef NEC765_H
#define NEC765_H

#include "devcb.h"
#include "devices/flopdrv.h"

/***************************************************************************
    MACROS
***************************************************************************/

#define NEC765A		DEVICE_GET_INFO_NAME(nec765a)
#define NEC765B		DEVICE_GET_INFO_NAME(nec765b)
#define SMC37C78	DEVICE_GET_INFO_NAME(smc37c78)
#define NEC72065	DEVICE_GET_INFO_NAME(nec72065)

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* RDY pin connected state */
typedef enum
{
	NEC765_RDY_PIN_NOT_CONNECTED = 0,
	NEC765_RDY_PIN_CONNECTED = 1
} NEC765_RDY_PIN;

#define NEC765_DAM_DELETED_DATA 0x0f8
#define NEC765_DAM_DATA 0x0fb

typedef void (*nec765_dma_drq_func)(const device_config *device, int state,int read_write);
#define NEC765_DMA_REQUEST(name)	void name(const device_config *device, int state,int read_write )

typedef const device_config *(*nec765_get_image_func)(const device_config *device, int floppy_index);
#define NEC765_GET_IMAGE(name)	const device_config *name(const device_config *device, int floppy_index )


typedef struct nec765_interface
{
	/* interrupt issued */
	devcb_write_line out_int_func;

	/* dma data request */
	nec765_dma_drq_func dma_drq;

	/* image lookup */
	nec765_get_image_func get_image;
	
	NEC765_RDY_PIN rdy_pin;
	
	const char *floppy_drive_tags[4];
} nec765_interface;


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( nec765a );
DEVICE_GET_INFO( nec765b );
DEVICE_GET_INFO( smc37c78 );
DEVICE_GET_INFO( nec72065 );

/* read of data register */
READ8_DEVICE_HANDLER(nec765_data_r);
/* write to data register */
WRITE8_DEVICE_HANDLER(nec765_data_w);
/* read of main status register */
READ8_DEVICE_HANDLER(nec765_status_r);

/* dma acknowledge with write */
WRITE8_DEVICE_HANDLER(nec765_dack_w);
/* dma acknowledge with read */
READ8_DEVICE_HANDLER(nec765_dack_r);

/* reset nec765 */
void nec765_reset(const device_config *device, int);

/* reset pin of nec765 */
WRITE_LINE_DEVICE_HANDLER(nec765_reset_w);

/* set nec765 terminal count input state */
WRITE_LINE_DEVICE_HANDLER(nec765_tc_w);

/* set nec765 ready input*/
WRITE_LINE_DEVICE_HANDLER(nec765_ready_w);

void nec765_idle(const device_config *device);

/*********************/
/* STATUS REGISTER 1 */

/* this is set if a TC signal was not received after the sector data was read */
#define NEC765_ST1_END_OF_CYLINDER (1<<7)
/* this is set if the sector ID being searched for is not found */
#define NEC765_ST1_NO_DATA (1<<2)
/* set if disc is write protected and a write/format operation was performed */
#define NEC765_ST1_NOT_WRITEABLE (1<<1)

/*********************/
/* STATUS REGISTER 2 */

/* C parameter specified did not match C value read from disc */
#define NEC765_ST2_WRONG_CYLINDER (1<<4)
/* C parameter specified did not match C value read from disc, and C read from disc was 0x0ff */
#define NEC765_ST2_BAD_CYLINDER (1<<1)
/* this is set if the FDC encounters a Deleted Data Mark when executing a read data
command, or FDC encounters a Data Mark when executing a read deleted data command */
#define NEC765_ST2_CONTROL_MARK (1<<6)

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_NEC765A_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, NEC765A, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC765A_MODIFY(_tag, _intrf) \
  MDRV_DEVICE_MODIFY(_tag)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC765A_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)

#define MDRV_NEC765B_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, NEC765B, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC765B_MODIFY(_tag, _intrf) \
  MDRV_DEVICE_MODIFY(_tag)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC765B_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)

#define MDRV_SMC37C78_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, SMC37C78, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_SMC37C78_MODIFY(_tag, _intrf) \
  MDRV_DEVICE_MODIFY(_tag)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_SMC37C78_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)

#define MDRV_NEC72065_ADD(_tag, _intrf) \
	MDRV_DEVICE_ADD(_tag, NEC72065, 0) \
	MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC72065_MODIFY(_tag, _intrf) \
  MDRV_DEVICE_MODIFY(_tag)	      \
  MDRV_DEVICE_CONFIG(_intrf)

#define MDRV_NEC72065_REMOVE(_tag)		\
  MDRV_DEVICE_REMOVE(_tag)

#endif /* NEC765_H */


