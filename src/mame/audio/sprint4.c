/*************************************************************************

    audio\sprint4.c

*************************************************************************/
#include "driver.h"
#include "sprint4.h"
#include "sound/discrete.h"


/************************************************************************/
/* sprint4 Sound System Analog emulation                                */
/************************************************************************/

static const discrete_lfsr_desc sprint4_lfsr =
{
	DISC_CLK_IS_FREQ,
	16,                 /* Bit Length */
	0,                  /* Reset Value */
	0,                  /* Use Bit 0 as XOR input 0 */
	14,                 /* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,     /* Feedback stage1 is XNOR */
	DISC_LFSR_OR,       /* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,  /* Feedback stage3 replaces the shifted register contents */
	0x000001,           /* Everything is shifted into the first bit only */
	0,                  /* Output is not inverted */
	15                  /* Output bit */
};

/* Nodes - Sounds */
#define SPRINT4_MOTORSND1   NODE_20
#define SPRINT4_MOTORSND2   NODE_21
#define SPRINT4_MOTORSND3   NODE_22
#define SPRINT4_MOTORSND4   NODE_23
#define SPRINT4_CRASHSND    NODE_24
#define SPRINT4_SKIDSND1    NODE_25
#define SPRINT4_SKIDSND2    NODE_26
#define SPRINT4_SKIDSND3    NODE_27
#define SPRINT4_SKIDSND4    NODE_28
#define SPRINT4_NOISE       NODE_29

DISCRETE_SOUND_START(sprint4_discrete_interface)
	/************************************************/
	/* sprint4 sound system: 5 sources per speaker  */
	/*                     Relative Volume          */
	/*    1/2) motor           77.72%               */
	/*      3) crash          100.00%               */
	/*    4/5) skid            29.89%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for sprint4           */
	/************************************************/
	/*                   NODE                GAIN    OFFSET  INIT */
	DISCRETE_INPUT_LOGIC(SPRINT4_SKID1_EN)
	DISCRETE_INPUT_LOGIC(SPRINT4_SKID2_EN)
	DISCRETE_INPUT_LOGIC(SPRINT4_SKID3_EN)
	DISCRETE_INPUT_LOGIC(SPRINT4_SKID4_EN)
	DISCRETE_INPUTX_DATA(SPRINT4_MOTOR1_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(SPRINT4_MOTOR2_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(SPRINT4_MOTOR3_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(SPRINT4_MOTOR4_DATA, -1, 0x0f, 0)
	DISCRETE_INPUTX_DATA(SPRINT4_CRASH_DATA, 1000.0/15.0, 0,  0.0)
	DISCRETE_INPUT_LOGIC(SPRINT4_ATTRACT_EN)

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
	DISCRETE_RCFILTER(NODE_40, 1, SPRINT4_MOTOR1_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 5)
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

	DISCRETE_ADDER3(SPRINT4_MOTORSND1, SPRINT4_ATTRACT_EN, NODE_45, NODE_48, NODE_51)

	/******************/
	/* motor sound #2 */
	/******************/
	DISCRETE_RCFILTER(NODE_60, 1, SPRINT4_MOTOR2_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_61, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 6)
	DISCRETE_MULTIPLY(NODE_62, 1, NODE_60, NODE_61)

	DISCRETE_MULTADD(NODE_63, 1, NODE_62, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_64, 1, NODE_63, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_65, 1, NODE_64, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_66, 1, NODE_62, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_67, 1, NODE_66, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_68, 1, NODE_67, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_69, 1, NODE_62, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_70, 1, NODE_69, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_71, 1, NODE_70, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT4_MOTORSND2, SPRINT4_ATTRACT_EN, NODE_65, NODE_68, NODE_71)

	/******************/
	/* motor sound #3 */
	/******************/
	DISCRETE_RCFILTER(NODE_80, 1, SPRINT4_MOTOR3_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_81, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 7)
	DISCRETE_MULTIPLY(NODE_82, 1, NODE_80, NODE_81)

	DISCRETE_MULTADD(NODE_83, 1, NODE_82, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_84, 1, NODE_83, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_85, 1, NODE_84, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_86, 1, NODE_82, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_87, 1, NODE_86, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_88, 1, NODE_87, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_89, 1, NODE_82, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_90, 1, NODE_89, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_91, 1, NODE_90, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT4_MOTORSND3, SPRINT4_ATTRACT_EN, NODE_85, NODE_88, NODE_91)

	/******************/
	/* motor sound #4 */
	/******************/
	DISCRETE_RCFILTER(NODE_100, 1, SPRINT4_MOTOR4_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_101, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_102, 1, NODE_100, NODE_101)

	DISCRETE_MULTADD(NODE_103, 1, NODE_102, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_104, 1, NODE_103, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_105, 1, NODE_104, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_106, 1, NODE_102, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_107, 1, NODE_106, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_108, 1, NODE_107, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_109, 1, NODE_102, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_110, 1, NODE_109, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_111, 1, NODE_110, 10000, 1e-7)

	DISCRETE_ADDER3(SPRINT4_MOTORSND4, SPRINT4_ATTRACT_EN, NODE_105, NODE_108, NODE_111)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SPRINT4_NOISE, SPRINT4_ATTRACT_EN, SPRINT4_ATTRACT_EN, 15750.0/4, 1.0, 0, 0, &sprint4_lfsr)

	DISCRETE_MULTIPLY(NODE_120, 1, SPRINT4_NOISE, SPRINT4_CRASH_DATA)
	DISCRETE_RCFILTER(SPRINT4_CRASHSND, 1, NODE_120, 545, 1e-7)

	/************************************************/
	/* Skid circuit takes the noise output from     */
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
	DISCRETE_INVERT(NODE_130, SPRINT4_NOISE)
	DISCRETE_MULTADD(NODE_131, 1, NODE_130, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_132, 1, NODE_131, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_133, 1, NODE_132, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(SPRINT4_SKIDSND1, SPRINT4_SKID1_EN, NODE_133)
	DISCRETE_ONOFF(SPRINT4_SKIDSND2, SPRINT4_SKID2_EN, NODE_133)
	DISCRETE_ONOFF(SPRINT4_SKIDSND3, SPRINT4_SKID3_EN, NODE_133)
	DISCRETE_ONOFF(SPRINT4_SKIDSND4, SPRINT4_SKID4_EN, NODE_133)

	/************************************************/
	/* Combine 5 sound sources.per speaker          */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_140, 1, SPRINT4_MOTORSND1, SPRINT4_CRASHSND, SPRINT4_SKIDSND1)
	DISCRETE_ADDER3(NODE_141, 1, NODE_140, SPRINT4_MOTORSND2, SPRINT4_SKIDSND2)

	DISCRETE_ADDER3(NODE_150, 1, SPRINT4_MOTORSND3, SPRINT4_CRASHSND, SPRINT4_SKIDSND3)
	DISCRETE_ADDER3(NODE_151, 1, NODE_150, SPRINT4_MOTORSND4, SPRINT4_SKIDSND4)

	DISCRETE_OUTPUT(NODE_141, 65534.0/(777.2+777.2+1000.0+298.9+298.9))
	DISCRETE_OUTPUT(NODE_151, 65534.0/(777.2+777.2+1000.0+298.9+298.9))
DISCRETE_SOUND_END
