/***************************************************************************

	machine/nec765.h

	Functions to emulate a NEC765/Intel 8272 compatible floppy disk controller

***************************************************************************/

#ifndef NEC765_H
#define NEC765_H

#include "devices/flopdrv.h"


/* supported versions */
typedef enum
{
	NEC765A = 0,
	NEC765B = 1,
	SMC37C78 = 2
} NEC765_VERSION;

#define NEC765_DAM_DELETED_DATA 0x0f8
#define NEC765_DAM_DATA 0x0fb

typedef struct nec765_interface
{
	/* interrupt issued */
	void	(*interrupt)(int state);

	/* dma data request */
	void	(*dma_drq)(int state,int read_write);

	/* image lookup */
	mess_image *(*get_image)(int floppy_index);
} nec765_interface;

/* init nec765 interface */
void nec765_init(const nec765_interface *iface, NEC765_VERSION version);
/* set nec765 terminal count input state */
void nec765_set_tc_state(int);
/* set nec765 ready input*/
void nec765_set_ready_state(int);

void nec765_set_ready_int(void);

void nec765_idle(void);

/* read of data register */
READ8_HANDLER(nec765_data_r);
/* write to data register */
WRITE8_HANDLER(nec765_data_w);
/* read of main status register */
READ8_HANDLER(nec765_status_r);

/* dma acknowledge with write */
WRITE8_HANDLER(nec765_dack_w);
/* dma acknowledge with read */
READ8_HANDLER(nec765_dack_r);

/* reset nec765 */
void nec765_reset(int);

/* reset pin of nec765 */
void nec765_set_reset_state(int);

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

#endif /* NEC765_H */

