/*****************************************************************************
 *
 * includes/ibmpc.h
 *
 ****************************************************************************/

#ifndef IBMPC_H_
#define IBMPC_H_


/*----------- defined in machine/ibmpc.c -----------*/

READ8_HANDLER( pc_rtc_r );
WRITE8_HANDLER( pc_rtc_w );

READ16_HANDLER( pc16le_rtc_r );
WRITE16_HANDLER( pc16le_rtc_w );

void pc_rtc_init(void);

READ8_HANDLER ( pc_EXP_r );
WRITE8_HANDLER ( pc_EXP_w );


#endif /* IBMPC_H_ */
