/*************************************************************************

    sndhrdw\ultratnk.c

*************************************************************************/
#include "driver.h"
#include "ultratnk.h"
#include "sound/discrete.h"


/************************************************************************/
/* ultratnk Sound System Analog emulation                               */
/************************************************************************/

const discrete_lfsr_desc ultratnk_lfsr={
	DISC_CLK_IS_FREQ,
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	15			/* Output bit */
};

/* Nodes - Sounds */
#define ULTRATNK_MOTORSND1		NODE_10
#define ULTRATNK_MOTORSND2		NODE_11
#define ULTRATNK_EXPLOSIONSND	NODE_12
#define ULTRATNK_FIRESND1		NODE_13
#define ULTRATNK_FIRESND2		NODE_14
#define ULTRATNK_NOISE			NODE_15
#define ULTRATNK_FINAL_MIX		NODE_16

DISCRETE_SOUND_START(ultratnk_discrete_interface)
	/************************************************/
	/* Ultratnk sound system: 5 Sound Sources       */
	/*                     Relative Volume          */
	/*    1/2) Motor           77.72%               */
	/*      2) Explosion      100.00%               */
	/*    4/5) Fire            29.89%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for ultratnk           */
	/************************************************/
	/*                   NODE                      GAIN    OFFSET  INIT */
	DISCRETE_INPUT_LOGIC(ULTRATNK_FIRE1_EN)
	DISCRETE_INPUT_LOGIC(ULTRATNK_FIRE2_EN)
	DISCRETE_INPUTX_DATA(ULTRATNK_MOTOR1_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(ULTRATNK_MOTOR2_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(ULTRATNK_EXPLOSION_DATA, 1000.0/15.0, 0,  0.0)
	DISCRETE_INPUT_LOGIC(ULTRATNK_ATTRACT_EN)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 2.2uf capacitor on the R-ladder.        */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, ULTRATNK_MOTOR1_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT_TAG(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, "MOTOR1")
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(ULTRATNK_MOTORSND1, ULTRATNK_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Tank2 motor sound is basically the same as   */
	/* Tank1.  But I shifted the frequencies up for */
	/* it to sound different from Tank1.            */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, ULTRATNK_MOTOR2_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT_TAG(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, "MOTOR2")
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(ULTRATNK_MOTORSND2, ULTRATNK_ATTRACT_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Explosion circuit is built around a noise    */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(ULTRATNK_NOISE, ULTRATNK_ATTRACT_EN, ULTRATNK_ATTRACT_EN, 15750.0/4, 1.0, 0, 0, &ultratnk_lfsr)

	DISCRETE_MULTIPLY(NODE_60, 1, ULTRATNK_NOISE, ULTRATNK_EXPLOSION_DATA)
	DISCRETE_RCFILTER(ULTRATNK_EXPLOSIONSND, 1, NODE_60, 545, 1e-7)

	/************************************************/
	/* Fire circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback circuit. */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, ULTRATNK_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_73, 1, NODE_72, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(ULTRATNK_FIRESND1, ULTRATNK_FIRE1_EN, NODE_73)
	DISCRETE_ONOFF(ULTRATNK_FIRESND2, ULTRATNK_FIRE2_EN, NODE_73)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, ULTRATNK_MOTORSND1, ULTRATNK_EXPLOSIONSND, ULTRATNK_FIRESND1)
	DISCRETE_ADDER3(NODE_91, 1, NODE_90, ULTRATNK_MOTORSND2, ULTRATNK_FIRESND2)

	DISCRETE_OUTPUT(NODE_91, 65534.0/(777.2+777.2+1000.0+298.9+298.9))
DISCRETE_SOUND_END
