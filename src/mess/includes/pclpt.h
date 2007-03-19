#ifndef __PC_LPT_H_
#define __PC_LPT_H_

#include "driver.h"
#include "includes/centroni.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { 
	LPT_UNIDIRECTIONAL
//	, LPT_BIDIRECTIONAL
// epp, ecp
} LPT_TYPE;

typedef struct {
	int on;
	LPT_TYPE type;
	void (*irq)(int nr, int level);
	// dma for ecp
} PC_LPT_CONFIG;

void pc_lpt_config(int nr, PC_LPT_CONFIG *config);

void pc_lpt_set_device(int nr, CENTRONICS_DEVICE *device);

/* line definitions in centroni.h */
/* only those lines in mask are modified */
void pc_lpt_handshake_in(int nr, int data, int mask);

READ8_HANDLER ( pc_parallelport0_r );
READ8_HANDLER ( pc_parallelport1_r );
READ8_HANDLER ( pc_parallelport2_r );
WRITE8_HANDLER( pc_parallelport0_w );
WRITE8_HANDLER( pc_parallelport1_w );
WRITE8_HANDLER( pc_parallelport2_w );

READ32_HANDLER ( pc32_parallelport0_r );
READ32_HANDLER ( pc32_parallelport1_r );
READ32_HANDLER ( pc32_parallelport2_r );
WRITE32_HANDLER( pc32_parallelport0_w );
WRITE32_HANDLER( pc32_parallelport1_w );
WRITE32_HANDLER( pc32_parallelport2_w );

#ifdef __cplusplus
}
#endif

#endif

