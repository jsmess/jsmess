/***************************************************************************

    Midway 8080-based black and white hardware

****************************************************************************/

#include "driver.h"
#include "mw8080bw.h"
#include "sound/samples.h"
#include "sound/sn76477.h"
#include "sound/discrete.h"



/*************************************
 *
 *  Globals
 *
 *************************************/


static UINT8 port_1_last;
static UINT8 port_2_last;
static UINT8 port_3_last;
static UINT8 port_4_last;



/*************************************
 *
 *  Machine setup
 *
 *************************************/


MACHINE_START( mw8080bw_audio )
{
	/* setup for save states */
	state_save_register_global(port_1_last);
	state_save_register_global(port_2_last);
	state_save_register_global(port_3_last);
	state_save_register_global(port_4_last);

	return 0;
}



/*************************************
 *
 *  Implementation of tone generator used
 *  by a few of these games
 *
 *************************************/


#define MIDWAY_TONE_EN				NODE_100
#define MIDWAY_TONE_DATA_L			NODE_101
#define MIDWAY_TONE_DATA_H			NODE_102
#define MIDWAY_TONE_SND				NODE_103
#define MIDWAY_TONE_TRASFORM_OUT	NODE_104
#define MIDWAY_TONE_BEFORE_AMP_SND	NODE_105


#define MIDWAY_TONE_GENERATOR(discrete_op_amp_tvca_info) \
		/* bit 0 of tone data is always 0 */ \
		/* join the L & H tone bits */ \
		DISCRETE_INPUT_LOGIC(MIDWAY_TONE_EN) \
		DISCRETE_INPUT_DATA (MIDWAY_TONE_DATA_L) \
		DISCRETE_INPUT_DATA (MIDWAY_TONE_DATA_H) \
		DISCRETE_TRANSFORM4(MIDWAY_TONE_TRASFORM_OUT, 1, MIDWAY_TONE_DATA_H, 0x40, MIDWAY_TONE_DATA_L, 0x02, "01*23*+") \
		DISCRETE_NOTE(MIDWAY_TONE_BEFORE_AMP_SND, 1, (double)MW8080BW_MASTER_CLOCK/10/2, MIDWAY_TONE_TRASFORM_OUT, 0xfff, 1, DISC_CLK_IS_FREQ) \
		DISCRETE_OP_AMP_TRIG_VCA(MIDWAY_TONE_SND, MIDWAY_TONE_BEFORE_AMP_SND, MIDWAY_TONE_EN, 0, 12, 0, &discrete_op_amp_tvca_info)


WRITE8_HANDLER( midway_tone_generator_lo_w )
{
	discrete_sound_w(MIDWAY_TONE_EN, (data >> 0) & 0x01);

	discrete_sound_w(MIDWAY_TONE_DATA_L, (data >> 1) & 0x1f);

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( midway_tone_generator_hi_w )
{
	discrete_sound_w(MIDWAY_TONE_DATA_H, data & 0x3f);

	/* D6 and D7 are not connected */
}



/*************************************
 *
 *  Sea Wolf
 *
 *************************************/


static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0
};


static struct Samplesinterface seawolf_samples_interface =
{
	5,	/* 5 channels */
	seawolf_sample_names
};


MACHINE_DRIVER_START( seawolf_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(seawolf_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.6)
MACHINE_DRIVER_END


WRITE8_HANDLER( seawolf_sh_port_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	/* if (data & 0x01)  enable SHIP HIT sound */
	if (rising_bits & 0x01) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x02)  enable TORPEDO sound */
	if (rising_bits & 0x02) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x04)  enable DIVE sound */
	if (rising_bits & 0x04) sample_start_n(0, 2, 2, 0);

	/* if (data & 0x08)  enable SONAR sound */
	if (rising_bits & 0x08) sample_start_n(0, 3, 3, 0);

	/* if (data & 0x10)  enable MINE HIT sound */
	if (rising_bits & 0x10) sample_start_n(0, 4, 4, 0);

	coin_counter_w(0, (data >> 5) & 0x01);

	/* D6 and D7 are not connected */

	port_1_last = data;
}



/*************************************
 *
 *  Gun Fight
 *
 *************************************/


static const char *gunfight_sample_names[] =
{
	"*gunfight",
	"gunshot.wav",
	"killed.wav",
	0
};


static struct Samplesinterface gunfight_samples_interface =
{
	1,	/* 1 channel */
	gunfight_sample_names
};


MACHINE_DRIVER_START( gunfight_sound )
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gunfight_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gunfight_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END


WRITE8_HANDLER( gunfight_sh_port_w )
{
	/* D0 and D1 are connected to something, most likely lights */
	/* the schematic shows them just tied to 1k resistors */

	coin_counter_w(0, (data >> 2) & 0x01);

	/* the 74175 latches and inverts the top 4 bits */
	switch ((~data >> 4) & 0x0f)
	{
	case 0x00:
		break;

	case 0x01:
		/* enable LEFT SHOOT sound (left speaker) */
		sample_start_n(0, 0, 0, 0);
		break;

	case 0x02:
		/* enable RIGHT SHOOT sound (right speaker) */
		sample_start_n(1, 0, 0, 0);
		break;

	case 0x03:
		/* enable LEFT HIT sound (left speaker) */
		sample_start_n(0, 0, 1, 0);
		break;

	case 0x04:
		/* enable RIGHT HIT sound (right speaker) */
		sample_start_n(1, 0, 1, 0);
		break;

	default:
		logerror("%04x:  Unknown sh port write %02x\n",activecpu_get_pc(),data);
		break;
	}
}



/*************************************
 *
 *  Tornado Baseball
 *
 *************************************/


#define TORNBASE_SQUAREW_240		NODE_01
#define TORNBASE_SQUAREW_960		NODE_02
#define TORNBASE_SQUAREW_120		NODE_03

#define TORNBASE_TONE_240_EN		NODE_04
#define TORNBASE_TONE_960_EN		NODE_05
#define TORNBASE_TONE_120_EN		NODE_06

#define TORNBASE_TONE_240_SND		NODE_07
#define TORNBASE_TONE_960_SND		NODE_08
#define TORNBASE_TONE_120_SND		NODE_09
#define TORNBASE_TONE_SND			NODE_10
#define TORNBASE_TONE_SND_FILT		NODE_11


static DISCRETE_SOUND_START(tornbase_discrete_interface)

	/* the 3 enable lines coming out of the 74175 flip-flop at G5 */
	DISCRETE_INPUT_LOGIC(TORNBASE_TONE_240_EN)		/* pin 2 */
	DISCRETE_INPUT_LOGIC(TORNBASE_TONE_960_EN)		/* pin 7 */
	DISCRETE_INPUT_LOGIC(TORNBASE_TONE_120_EN)		/* pin 5 */

	/* 3 different freq square waves (240, 960 and 120Hz).
       Originates from the CPU board via an edge connector.
       The wave is in the 0/+1 range */
	DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_240, 1, 240, 1.0, 50.0, 1.0/2, 0)	/* pin X */
	DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_960, 1, 960, 1.0, 50.0, 1.0/2, 0)	/* pin Y */
	DISCRETE_SQUAREWFIX(TORNBASE_SQUAREW_120, 1, 120, 1.0, 50.0, 1.0/2, 0)	/* pin V */

	/* 7403 O/C NAND gate at G6.  3 of the 4 gates used with their outputs tied together */
	DISCRETE_LOGIC_NAND(TORNBASE_TONE_240_SND, 1, TORNBASE_SQUAREW_240, TORNBASE_TONE_240_EN)	/* pins 4,5,6 */
	DISCRETE_LOGIC_NAND(TORNBASE_TONE_960_SND, 1, TORNBASE_SQUAREW_960, TORNBASE_TONE_960_EN)	/* pins 2,1,3 */
	DISCRETE_LOGIC_NAND(TORNBASE_TONE_120_SND, 1, TORNBASE_SQUAREW_120, TORNBASE_TONE_120_EN)	/* pins 13,12,11 */
	DISCRETE_LOGIC_AND3(TORNBASE_TONE_SND,     1, TORNBASE_TONE_240_SND, TORNBASE_TONE_960_SND, TORNBASE_TONE_120_SND)

	/* 47K resistor (R601) and 0.047uF capacitor (C601)
       There is also a 50K pot acting as a volume control, but we output at
       the maximum volume as MAME has its own volume adjustment */
	DISCRETE_CRFILTER(TORNBASE_TONE_SND_FILT, 1, TORNBASE_TONE_SND, RES_K(47), CAP_U(0.047))

	/* amplify for output */
	DISCRETE_OUTPUT(TORNBASE_TONE_SND_FILT, 32767)

DISCRETE_SOUND_END


MACHINE_DRIVER_START( tornbase_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(tornbase_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END


WRITE8_HANDLER( tornbase_sh_port_w )
{
	discrete_sound_w(TORNBASE_TONE_240_EN, (data >> 0) & 0x01);

	discrete_sound_w(TORNBASE_TONE_960_EN, (data >> 1) & 0x01);

	discrete_sound_w(TORNBASE_TONE_120_EN, (data >> 2) & 0x01);

	/* if (data & 0x08)  enable SIREN sound */

	/* if (data & 0x10)  enable CHEER sound */

	if (tornbase_get_cabinet_type() == TORNBASE_CAB_TYPE_UPRIGHT_OLD)
	{
		/* if (data & 0x20)  enable WHISTLE sound */

		/* D6 is not connected on this cabinet type */
	}
	else
	{
		/* D5 is not connected on this cabinet type */

		/* if (data & 0x40)  enable WHISTLE sound */
	}

	coin_counter_w(0, (data >> 7) & 0x01);
}



/*************************************
 *
 *  280 ZZZAP / Laguna Racer
 *
 *************************************/


MACHINE_DRIVER_START( zzzap_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


WRITE8_HANDLER( zzzap_sh_port_1_w )
{
	/* set ENGINE SOUND FREQ(data & 0x0f)  the value written is
                                           the gas pedal position */

	/* if (data & 0x10)  enable HI SHIFT engine sound modifier */

	/* if (data & 0x20)  enable LO SHIFT engine sound modifier */

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( zzzap_sh_port_2_w )
{
	/* if (data & 0x01)  enable BOOM sound */

	/* if (data & 0x02)  enable ENGINE sound (global) */

	/* if (data & 0x04)  enable CR 1 (screeching sound) */

	/* if (data & 0x08)  enable NOISE CR 2 (happens only after the car blows up, but
                                            before it appears again, not sure what
                                            it is supposed to sound like) */

	coin_counter_w(0, (data >> 5) & 0x01);

	/* D4, D6 and D7 are not connected */
}



/*************************************
 *
 *  Amazing Maze
 *
 *  Discrete sound emulation: Feb 2007, D.R.
 *
 *************************************/


/* nodes - inputs */
#define MAZE_P1_DATA			NODE_01
#define MAZE_P2_DATA			NODE_02
#define MAZE_TONE_TIMING		NODE_03
#define MAZE_COIN				NODE_04

/* nodes - other */
#define MAZE_JOYSTICK_IN_USE	NODE_11
#define MAZE_AUDIO_ENABLE		NODE_12
#define MAZE_TONE_ENABLE		NODE_13
#define MAZE_GAME_OVER			NODE_14
#define MAZE_R305_306_308		NODE_15
#define MAZE_R303_309			NODE_16
#define MAZE_PLAYER_SEL			NODE_17

/* nodes - sounds */
#define MAZE_SND				NODE_18


static const discrete_555_desc maze_555_F2 =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC | DISC_555_TRIGGER_IS_LOGIC,
	5,				/* B+ voltage of 555 */
	DEFAULT_555_VALUES
};


static const double maze_74147_table[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 1, 1, 2, 3
};


static const discrete_comp_adder_table maze_r305_306_308 =
{
	DISC_COMP_P_RESISTOR,	/* type of circuit */
	RES_K(100),				/* R308 */
	2,						/* length */
	{ RES_M(1.5),			/* R304 */
	  RES_K(820) }			/* R304 */
};


static const discrete_comp_adder_table maze_r303_309 =
{
	DISC_COMP_P_RESISTOR,	/* type of circuit */
	RES_K(330),				/* R309 */
	1,						/* length */
	{ RES_M(1) }			/* R303 */
};


static const discrete_op_amp_osc_info maze_op_amp_osc =
{
	DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON,	/* type */
	RES_M(1),			/* r306 */
	RES_K(430),			/* r307 */
	MAZE_R305_306_308,	/* r304, r305, r308 switchable circuit */
	MAZE_R303_309,		/* r303, r309 switchable circuit */
	RES_K(330),			/* r310 */
	0, 0, 0,			/* not used */
	CAP_P(3300),		/* c300 */
	5					/* vP */
};

static DISCRETE_SOUND_START(maze_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_DATA (MAZE_P1_DATA)
	DISCRETE_INPUT_DATA (MAZE_P2_DATA)
	DISCRETE_INPUT_LOGIC(MAZE_TONE_TIMING)
	DISCRETE_INPUT_LOGIC(MAZE_COIN)
	DISCRETE_INPUT_LOGIC(MAZE_JOYSTICK_IN_USE)	/* IC D2, pin 8 */

	DISCRETE_LOGIC_INVERT(NODE_20,				/* IC E2, pin 8 */
					1,							/* ENAB */
					MAZE_JOYSTICK_IN_USE)		/* IN0 */
	DISCRETE_555_MSTABLE(MAZE_GAME_OVER,		/* IC F2, pin 3 */
					1,							/* RESET */
					NODE_20,					/* TRIG */
					RES_K(270),					/* R203 */
					CAP_U(100),					/* C204 */
					&maze_555_F2)
	DISCRETE_LOGIC_JKFLIPFLOP(MAZE_AUDIO_ENABLE,/* IC F1, pin 5 */
					1,							/* ENAB */
					MAZE_COIN,					/* RESET */
					1,							/* SET */
					MAZE_GAME_OVER,				/* CLK */
					1,							/* J */
					0)							/* K */
	DISCRETE_LOGIC_INVERT(MAZE_TONE_ENABLE,		/* IC F1, pin 6 */
					1,							/* ENAB */
					MAZE_AUDIO_ENABLE)			/* IN0 */
	DISCRETE_LOGIC_AND3(NODE_21,
					1,							/* ENAB */
					MAZE_JOYSTICK_IN_USE,		/* INP0 */
					MAZE_TONE_ENABLE,			/* INP1 */
					MAZE_TONE_TIMING)			/* INP2 */

	DISCRETE_LOGIC_JKFLIPFLOP(MAZE_PLAYER_SEL,	/* IC C1, pin 3 */
					1,							/* ENAB */
					1,							/* RESET */
					1,							/* SET */
					MAZE_TONE_TIMING,			/* CLK */
					1,							/* J */
					1)							/* K */
	DISCRETE_MULTIPLEX2(NODE_31,				/* IC D1 */
					1,							/* ENAB */
					MAZE_PLAYER_SEL,			/* ADDR */
					MAZE_P1_DATA,				/* INP0 */
					MAZE_P2_DATA)				/* INP1 */
	DISCRETE_LOOKUP_TABLE(NODE_32,				/* IC E1 */
					1,							/* ENAB */
					NODE_31,					/* ADDR */
					16,							/* SIZE */
					&maze_74147_table)
	DISCRETE_COMP_ADDER(MAZE_R305_306_308,		/* value of selected parallel circuit r305, r306, r308 */
					1,							/* ENAB */
					NODE_32,					/* DATA */
					&maze_r305_306_308)
	DISCRETE_COMP_ADDER(MAZE_R303_309,			/* value of selected parallel circuit r303, r309 */
					1,							/* ENAB */
					MAZE_PLAYER_SEL,			/* DATA */
					&maze_r303_309)
	DISCRETE_OP_AMP_OSCILLATOR(NODE_36,			/* IC J1, pin 4 */
					1,					/* ENAB */
					&maze_op_amp_osc)

	DISCRETE_CRFILTER(NODE_40,
					NODE_21,					/* ENAB */
					NODE_36,					/* IN0 */
					RES_K(250),					/* r311, r312, r402, r403 in parallel */
					CAP_U(0.01)	)				/* c401 */
	DISCRETE_RCFILTER(NODE_41,
					1		,					/* ENAB */
					NODE_40,					/* IN0 */
					RES_K(56),					/* r404 */
					CAP_P(4700)	)				/* c400 */
	DISCRETE_SWITCH(MAZE_SND,					/* H3 saturates op-amp J3 when enabled, disabling audio */
					1,							/* ENAB */
					MAZE_AUDIO_ENABLE,			/* SWITCH */
					NODE_41,					/* INP0 */
					0)							/* INP1 */

	DISCRETE_OUTPUT(MAZE_SND, 36040)
DISCRETE_SOUND_END


MACHINE_DRIVER_START( maze_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(maze_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


void maze_write_discrete(UINT8 maze_tone_timing_state)
{
	/* controls need to be active low */
	int controls = ~readinputport(0) &0xff;

	discrete_sound_w(MAZE_TONE_TIMING, maze_tone_timing_state);
	discrete_sound_w(MAZE_P1_DATA, controls & 0x0f);
	discrete_sound_w(MAZE_P2_DATA, (controls >> 4) & 0x0f);
	discrete_sound_w(MAZE_JOYSTICK_IN_USE, controls != 0xff);

	/* the coin line is connected directly to the discrete circuit */
	/* we can't really do that, so updating it with the tone timing is close enough */
	discrete_sound_w(MAZE_COIN, (~readinputport(1) >> 3) & 0x01);
}



/*************************************
 *
 *  Boot Hill
 *
 *  Discrete sound emulation: Jan 2007, D.R.
 *
 *************************************/


/* nodes - inputs */
#define BOOTHILL_GAME_ON_EN			NODE_01
#define BOOTHILL_LEFT_SHOT_EN		NODE_02
#define BOOTHILL_RIGHT_SHOT_EN		NODE_03
#define BOOTHILL_LEFT_HIT_EN		NODE_04
#define BOOTHILL_RIGHT_HIT_EN		NODE_05

/* nodes - sounds */
#define BOOTHILL_NOISE				NODE_06
#define BOOTHILL_L_SHOT_SND			NODE_07
#define BOOTHILL_R_SHOT_SND			NODE_08
#define BOOTHILL_L_HIT_SND			NODE_09
#define BOOTHILL_R_HIT_SND			NODE_10

/* nodes - adjusters */
#define BOOTHILL_MUSIC_ADJ			NODE_11


static const discrete_lfsr_desc boothill_lfsr =
{
	DISC_CLK_IS_FREQ,
	17,					/* bit length */
						/* the RC network fed into pin 4, has the effect
                           of presetting all bits high at power up */
	0x1ffff,			/* reset value */
	4,					/* use bit 4 as XOR input 0 */
	16,					/* use bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* feedback stage1 is XOR */
	DISC_LFSR_OR,		/* feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* feedback stage3 replaces the shifted register contents */
	0x000001,			/* everything is shifted into the first bit only */
	0,					/* output is not inverted */
	12					/* output bit */
};


static const discrete_op_amp_tvca_info boothill_tone_tvca_info =
{
	RES_M(3.3),
	RES_K(100) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_op_amp_tvca_info boothill_shot_tvca_info =
{
	RES_M(2.7),
	RES_K(510),
	0,
	RES_K(510),
	RES_K(10),
	0,
	RES_K(510),
	0,
	0,
	0,
	0,
	CAP_U(0.22),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_op_amp_tvca_info boothill_hit_tvca_info =
{
	RES_M(2.7),
	RES_K(510),
	0,
	RES_K(510),
	RES_K(10),
	0,
	RES_K(510),
	0,
	0,
	0,
	0,
	CAP_U(1),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_mixer_desc boothill_l_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(12) + RES_K(100) + RES_K(33) },
	{ 0 },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	7200	/* final gain */
};


static const discrete_mixer_desc boothill_r_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(33),
	  RES_K(12) + RES_K(100) + RES_K(33),
	  RES_K(33) },
	{ 0,
	  0,
	  BOOTHILL_MUSIC_ADJ },
	{ 0 },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	7200	/* final gain */
};


static DISCRETE_SOUND_START(boothill_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC(BOOTHILL_GAME_ON_EN)
	DISCRETE_INPUT_LOGIC(BOOTHILL_LEFT_SHOT_EN)
	DISCRETE_INPUT_LOGIC(BOOTHILL_RIGHT_SHOT_EN)
	DISCRETE_INPUT_LOGIC(BOOTHILL_LEFT_HIT_EN)
	DISCRETE_INPUT_LOGIC(BOOTHILL_RIGHT_HIT_EN)

	/* The low value of the pot is set to 75000.  A real 1M pot will never go to 0 anyways.
       This will give the control more apparent volume range.
       The music way overpowers the rest of the sounds anyways. */
	DISCRETE_ADJUSTMENT(BOOTHILL_MUSIC_ADJ, 1, RES_M(1), 75000, DISC_LOGADJ, 3)

	/************************************************
     * Tone generator
     ************************************************/
	MIDWAY_TONE_GENERATOR(boothill_tone_tvca_info)

	/************************************************
     * Shot sounds
     ************************************************/
	/* Noise clock was breadboarded and measured at 7700Hz */
	DISCRETE_LFSR_NOISE(BOOTHILL_NOISE, 1, 1, 7700, 12.0, 0, 12.0/2, &boothill_lfsr)

	DISCRETE_OP_AMP_TRIG_VCA(NODE_30, BOOTHILL_LEFT_SHOT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_shot_tvca_info)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(12), CAP_U(.01))
	DISCRETE_CRFILTER(BOOTHILL_L_SHOT_SND, 1, NODE_31, RES_K(12) + RES_K(68), CAP_U(.0022))

	DISCRETE_OP_AMP_TRIG_VCA(NODE_35, BOOTHILL_RIGHT_SHOT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_shot_tvca_info)
	DISCRETE_RCFILTER(NODE_36, 1, NODE_35, RES_K(12), CAP_U(.01))
	DISCRETE_CRFILTER(BOOTHILL_R_SHOT_SND, 1, NODE_36, RES_K(12) + RES_K(68), CAP_U(.0033))

	/************************************************
     * Hit sounds
     ************************************************/
	DISCRETE_OP_AMP_TRIG_VCA(NODE_40, BOOTHILL_LEFT_HIT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_hit_tvca_info)
	DISCRETE_RCFILTER(NODE_41, 1, NODE_40, RES_K(12), CAP_U(.033))
	DISCRETE_CRFILTER(BOOTHILL_L_HIT_SND, 1, NODE_41, RES_K(12) + RES_K(100), CAP_U(.0033))

	DISCRETE_OP_AMP_TRIG_VCA(NODE_45, BOOTHILL_RIGHT_HIT_EN, 0, 0, BOOTHILL_NOISE, 0, &boothill_hit_tvca_info)
	DISCRETE_RCFILTER(NODE_46, 1, NODE_45, RES_K(12), CAP_U(.0033))
	DISCRETE_CRFILTER(BOOTHILL_R_HIT_SND, 1, NODE_46, RES_K(12) + RES_K(100), CAP_U(.0022))

	/************************************************
     * Combine all sound sources.
     ************************************************/
	DISCRETE_MIXER2(NODE_91, BOOTHILL_GAME_ON_EN, BOOTHILL_L_SHOT_SND, BOOTHILL_L_HIT_SND, &boothill_l_mixer)

	/* music is only added to the right channel */
	DISCRETE_MIXER3(NODE_92, BOOTHILL_GAME_ON_EN, BOOTHILL_R_SHOT_SND, BOOTHILL_R_HIT_SND, MIDWAY_TONE_SND, &boothill_r_mixer)

	DISCRETE_OUTPUT(NODE_91, 1)
	DISCRETE_OUTPUT(NODE_92, 1)
DISCRETE_SOUND_END


MACHINE_DRIVER_START( boothill_sound )
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(boothill_discrete_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


WRITE8_HANDLER( boothill_sh_port_w )
{
	/* D0 and D1 are not connected */

	coin_counter_w(0, (data >> 2) & 0x01);

	discrete_sound_w(BOOTHILL_GAME_ON_EN, (data >> 3) & 0x01);

	discrete_sound_w(BOOTHILL_LEFT_SHOT_EN, (data >> 4) & 0x01);

	discrete_sound_w(BOOTHILL_RIGHT_SHOT_EN, (data >> 5) & 0x01);

	discrete_sound_w(BOOTHILL_LEFT_HIT_EN, (data >> 6) & 0x01);

	discrete_sound_w(BOOTHILL_RIGHT_HIT_EN, (data >> 7) & 0x01);
}



/*************************************
 *
 *  Checkmate
 *
 *************************************/


MACHINE_DRIVER_START( checkmat_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


WRITE8_HANDLER( checkmat_sh_port_w )
{
	/* if (data & 0x01)  enable TONE */

	/* if (data & 0x02)  enable BOOM sound */

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	/* D4-D7  set TONE FREQ( (data >> 4) & 0x0f ) */
}



/*************************************
 *
 *  Desert Gun
 *
 *  Discrete sound emulation: Jan 2007, D.R.
 *
 *************************************/


/* nodes - inputs */
#define DESERTGU_GAME_ON_EN					NODE_01
#define DESERTGU_RIFLE_SHOT_EN				NODE_02
#define DESERTGU_BOTTLE_HIT_EN				NODE_03
#define DESERTGU_ROAD_RUNNER_HIT_EN			NODE_04
#define DESERTGU_CREATURE_HIT_EN			NODE_05
#define DESERTGU_ROADRUNNER_BEEP_BEEP_EN	NODE_06
#define DESERTGU_TRIGGER_CLICK_EN			NODE_07

/* nodes - sounds */
#define DESERTGU_NOISE						NODE_08
#define DESERTGU_RIFLE_SHOT_SND				NODE_09
#define DESERTGU_BOTTLE_HIT_SND				NODE_10
#define DESERTGU_ROAD_RUNNER_HIT_SND		NODE_11
#define DESERTGU_CREATURE_HIT_SND			NODE_12
#define DESERTGU_ROADRUNNER_BEEP_BEEP_SND	NODE_13
#define DESERTGU_TRIGGER_CLICK_SND			DESERTGU_TRIGGER_CLICK_EN

/* nodes - adjusters */
#define DESERTGU_MUSIC_ADJ					NODE_15


static const discrete_lfsr_desc desertgu_lfsr =
{
	DISC_CLK_IS_FREQ,
	17,					/* bit length */
						/* the RC network fed into pin 4, has the effect
                           of presetting all bits high at power up */
	0x1ffff,			/* reset value */
	4,					/* use bit 4 as XOR input 0 */
	16,					/* use bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* feedback stage1 is XOR */
	DISC_LFSR_OR,		/* feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* feedback stage3 replaces the shifted register contents */
	0x000001,			/* everything is shifted into the first bit only */
	0,					/* output is not inverted */
	12					/* output bit */
};


static const discrete_op_amp_tvca_info desertgu_tone_tvca_info =
{
	RES_M(3.3),					/* r502 */
	RES_K(10) + RES_K(680),		/* r505 + r506 */
	0,
	RES_K(680),					/* r503 */
	RES_K(10),					/* r500 */
	0,
	RES_K(680),					/* r501 */
	0,
	0,
	0,
	0,
	CAP_U(.001),				/* c500 */
	0, 0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_op_amp_tvca_info desertgu_rifle_shot_tvca_info =
{
	RES_M(2.7),
	RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(0.47),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_mixer_desc desertgu_filter_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{ RES_K(2),
	  RES_K(27),
	  RES_K(2) + RES_K(1) },
	{ 0 },
	{ 0 },
	0,
	0,
	0,
	0,
	0,
	1
};


static const discrete_op_amp_filt_info desertgu_filter =
{
	1.0 / ( 1.0 / RES_K(2) + 1.0 / RES_K(27) + 1.0 / (RES_K(2) + RES_K(1))),
	0,
	68,
	0,
	RES_K(39),
	CAP_U(0.033),
	CAP_U(0.033),
	0,
	0,
	12,
	0
};


static const discrete_mixer_desc desertgu_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(12) + RES_K(68) + RES_K(30),
	  RES_K(56),
	  RES_K(180),
	  RES_K(47),
	  RES_K(30) },
	{ 0,
	  0,
	  0,
	  0,
	  DESERTGU_MUSIC_ADJ },
	{ CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1) },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,
	6000	/* final gain */
};


static DISCRETE_SOUND_START(desertgu_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC(DESERTGU_GAME_ON_EN)
	DISCRETE_INPUT_LOGIC(DESERTGU_RIFLE_SHOT_EN)
	DISCRETE_INPUT_LOGIC(DESERTGU_BOTTLE_HIT_EN)
	DISCRETE_INPUT_LOGIC(DESERTGU_ROAD_RUNNER_HIT_EN)
	DISCRETE_INPUT_LOGIC(DESERTGU_CREATURE_HIT_EN)
	DISCRETE_INPUT_LOGIC(DESERTGU_ROADRUNNER_BEEP_BEEP_EN)
	DISCRETE_INPUTX_LOGIC(DESERTGU_TRIGGER_CLICK_SND, 12, 0, 0)

	/* The low value of the pot is set to 75000.  A real 1M pot will never go to 0 anyways. */
	/* This will give the control more apparent volume range. */
	/* The music way overpowers the rest of the sounds anyways. */
	DISCRETE_ADJUSTMENT(DESERTGU_MUSIC_ADJ, 1, RES_M(1), 75000, DISC_LOGADJ, 3)

	/************************************************
     * Tone generator
     ************************************************/
	MIDWAY_TONE_GENERATOR(desertgu_tone_tvca_info)

	/************************************************
     * Rifle shot sound
     ************************************************/
	/* Noise clock was breadboarded and measured at 7515Hz */
	DISCRETE_LFSR_NOISE(DESERTGU_NOISE, 1, 1, 7515, 12.0, 0, 12.0/2, &desertgu_lfsr)

	DISCRETE_OP_AMP_TRIG_VCA(NODE_30, DESERTGU_RIFLE_SHOT_EN, 0, 0, DESERTGU_NOISE, 0, &desertgu_rifle_shot_tvca_info)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(12), CAP_U(.01))
	DISCRETE_CRFILTER(DESERTGU_RIFLE_SHOT_SND, 1, NODE_31, RES_K(12) + RES_K(68), CAP_U(.0022))

	/************************************************
     * Bottle hit sound
     ************************************************/
	DISCRETE_CONSTANT(DESERTGU_BOTTLE_HIT_SND, 0)  /* placeholder for incomplete sound */

	/************************************************
     * Road Runner hit sound
     ************************************************/
	DISCRETE_CONSTANT(DESERTGU_ROAD_RUNNER_HIT_SND, 0)  /* placeholder for incomplete sound */

	/************************************************
     * Creature hit sound
     ************************************************/
	DISCRETE_CONSTANT(DESERTGU_CREATURE_HIT_SND, 0)  /* placeholder for incomplete sound */

	/************************************************
     * Beep-Beep sound
     ************************************************/
	DISCRETE_CONSTANT(DESERTGU_ROADRUNNER_BEEP_BEEP_SND, 0) /* placeholder for incomplete sound */

	/************************************************
     * Mix and filter
     ************************************************/
	DISCRETE_MIXER3(NODE_80, 1, DESERTGU_BOTTLE_HIT_SND, DESERTGU_ROADRUNNER_BEEP_BEEP_SND, DESERTGU_TRIGGER_CLICK_SND, &desertgu_filter_mixer)
	DISCRETE_OP_AMP_FILTER(NODE_81, 1, NODE_80, 0, DISC_OP_AMP_FILTER_IS_BAND_PASS_1, &desertgu_filter)

	/************************************************
     * Combine all sound sources.
     ************************************************/
	DISCRETE_MIXER5(NODE_91, DESERTGU_GAME_ON_EN, DESERTGU_RIFLE_SHOT_SND, DESERTGU_ROAD_RUNNER_HIT_SND, DESERTGU_CREATURE_HIT_SND, NODE_81, MIDWAY_TONE_SND, &desertgu_mixer)

	DISCRETE_OUTPUT(NODE_91, 1)
DISCRETE_SOUND_END


MACHINE_DRIVER_START( desertgu_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(desertgu_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.8)
MACHINE_DRIVER_END


WRITE8_HANDLER( desertgu_sh_port_1_w )
{
	/* D0 and D1 are not connected */

	coin_counter_w(0, (data >> 2) & 0x01);

	discrete_sound_w(DESERTGU_GAME_ON_EN, (data >> 3) & 0x01);

	discrete_sound_w(DESERTGU_RIFLE_SHOT_EN, (data >> 4) & 0x01);

	discrete_sound_w(DESERTGU_BOTTLE_HIT_EN, (data >> 5) & 0x01);

	discrete_sound_w(DESERTGU_ROAD_RUNNER_HIT_EN, (data >> 6) & 0x01);

	discrete_sound_w(DESERTGU_CREATURE_HIT_EN, (data >> 7) & 0x01);
}


WRITE8_HANDLER( desertgu_sh_port_2_w )
{
	discrete_sound_w(DESERTGU_ROADRUNNER_BEEP_BEEP_EN, (data >> 0) & 0x01);

	discrete_sound_w(DESERTGU_TRIGGER_CLICK_EN, (data >> 1) & 0x01);

	output_set_value("KICKER", (data >> 2) & 0x01);

	desertgun_set_controller_select((data >> 3) & 0x01);

	/* D4-D7 are not connected */
}



/*************************************
 *
 *  Double Play / Extra Inning
 *
 *  Discrete sound emulation: Jan 2007, D.R.
 *
 *************************************/


/* nodes - inputs */
#define DPLAY_GAME_ON_EN	NODE_01
#define DPLAY_TONE_ON_EN	NODE_02
#define DPLAY_SIREN_EN		NODE_03
#define DPLAY_WHISTLE_EN	NODE_04
#define DPLAY_CHEER_EN		NODE_05

/* nodes - sounds */
#define DPLAY_NOISE			NODE_06
#define DPLAY_TONE_SND		NODE_07
#define DPLAY_SIREN_SND		NODE_08
#define DPLAY_WHISTLE_SND	NODE_09
#define DPLAY_CHEER_SND		NODE_10

/* nodes - adjusters */
#define DPLAY_MUSIC_ADJ		NODE_11


static const discrete_lfsr_desc dplay_lfsr =
{
	DISC_CLK_IS_FREQ,
	17,					/* bit length */
						/* the RC network fed into pin 4, has the effect
                           of presetting all bits high at power up */
	0x1ffff,			/* reset value */
	4,					/* use bit 4 as XOR input 0 */
	16,					/* use bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* feedback stage1 is XOR */
	DISC_LFSR_OR,		/* feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* feedback stage3 replaces the shifted register contents */
	0x000001,			/* everything is shifted into the first bit only */
	0,					/* output is not inverted */
	8					/* output bit */
};


static const discrete_op_amp_tvca_info dplay_music_tvca_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_integrate_info dplay_siren_integrate_info =
{
	DISC_INTEGRATE_OP_AMP_1 | DISC_OP_AMP_IS_NORTON,
	RES_M(1),
	RES_K(100),
	0,
	CAP_U(3.3),
	12,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_integrate_info dplay_whistle_integrate_info =
{
	DISC_INTEGRATE_OP_AMP_1 | DISC_OP_AMP_IS_NORTON,
	RES_M(1),
	RES_K(220) + RES_K(10),
	0,
	CAP_U(3.3),
	12,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_integrate_info dplay_cheer_integrate_info =
{
	DISC_INTEGRATE_OP_AMP_1 | DISC_OP_AMP_IS_NORTON,
	RES_M(1.5),
	RES_K(100),
	0,
	CAP_U(4.7),
	12,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_op_amp_filt_info dplay_cheer_filter =
{
	RES_K(100),
	0,
	RES_K(100),
	0,
	RES_K(150),
	CAP_U(0.0047),
	CAP_U(0.0047),
	0,
	0,
	12,
	0
};


static const discrete_mixer_desc dplay_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(68),
	  RES_K(68),
	  RES_K(68),
	  RES_K(18),
	  RES_K(68) },
	{ 0,
	  0,
	  0,
	  0,
	  DPLAY_MUSIC_ADJ },
	{ CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1),
	  CAP_U(0.1) }
	, 0, RES_K(100), 0, CAP_U(0.1), 0,
	2000	/* final gain */
};


static DISCRETE_SOUND_START(dplay_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC (DPLAY_GAME_ON_EN)
	DISCRETE_INPUT_LOGIC (DPLAY_TONE_ON_EN)
	DISCRETE_INPUT_LOGIC (DPLAY_SIREN_EN)
	DISCRETE_INPUT_LOGIC (DPLAY_WHISTLE_EN)
	DISCRETE_INPUTX_LOGIC(DPLAY_CHEER_EN, 5, 0, 0)

	/* The low value of the pot is set to 1000.  A real 1M pot will never go to 0 anyways. */
	/* This will give the control more apparent volume range. */
	/* The music way overpowers the rest of the sounds anyways. */
	DISCRETE_ADJUSTMENT(DPLAY_MUSIC_ADJ, 1, RES_M(1), 1000, DISC_LOGADJ, 3)

	/************************************************
     * Music and Tone Generator
     ************************************************/
	MIDWAY_TONE_GENERATOR(dplay_music_tvca_info)

	DISCRETE_OP_AMP_TRIG_VCA(DPLAY_TONE_SND, MIDWAY_TONE_BEFORE_AMP_SND, DPLAY_TONE_ON_EN, 0, 12, 0, &dplay_music_tvca_info)

	/************************************************
     * Siren - INCOMPLETE
     ************************************************/
	DISCRETE_INTEGRATE(NODE_30, DPLAY_SIREN_EN, 0, &dplay_siren_integrate_info)
	DISCRETE_CONSTANT(DPLAY_SIREN_SND, 0)	/* placeholder for incomplete sound */

	/************************************************
     * Whistle - INCOMPLETE
     ************************************************/
	DISCRETE_INTEGRATE(NODE_40, DPLAY_WHISTLE_EN, 0, &dplay_whistle_integrate_info)
	DISCRETE_CONSTANT(DPLAY_WHISTLE_SND, 0)	/* placeholder for incomplete sound */

	/************************************************
     * Cheer
     ************************************************/
	/* Noise clock was breadboarded and measured at 7700Hz */
	DISCRETE_LFSR_NOISE(DPLAY_NOISE, 1, 1, 7700, 12.0, 0, 12.0/2, &dplay_lfsr)

	DISCRETE_INTEGRATE(NODE_50, DPLAY_CHEER_EN, 0, &dplay_cheer_integrate_info)
	DISCRETE_SWITCH(NODE_51, 1, DPLAY_NOISE, 0, NODE_50)
	DISCRETE_OP_AMP_FILTER(DPLAY_CHEER_SND, 1, NODE_51, 0, DISC_OP_AMP_FILTER_IS_BAND_PASS_1M, &dplay_cheer_filter)

	/************************************************
     * Combine all sound sources.
     ************************************************/
	DISCRETE_MIXER5(NODE_91, DPLAY_GAME_ON_EN, DPLAY_TONE_SND, DPLAY_SIREN_SND, DPLAY_WHISTLE_SND, DPLAY_CHEER_SND, MIDWAY_TONE_SND, &dplay_mixer)

	DISCRETE_OUTPUT(NODE_91, 1)
DISCRETE_SOUND_END


MACHINE_DRIVER_START( dplay_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG(dplay_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.8)
MACHINE_DRIVER_END


WRITE8_HANDLER( dplay_sh_port_w )
{
	discrete_sound_w(DPLAY_TONE_ON_EN, (data >> 0) & 0x01);

	discrete_sound_w(DPLAY_CHEER_EN, (data >> 1) & 0x01);

	/* discrete_sound_w(DPLAY_SIREN_EN, (data >> 2) & 0x01); */

	/* discrete_sound_w(DPLAY_WHISTLE_EN, (data >> 3) & 0x01); */

	discrete_sound_w(DPLAY_GAME_ON_EN, (data >> 4) & 0x01);

	coin_counter_w(0, (data >> 5) & 0x01);

	/* D6 and D7 are not connected */
}



/*************************************
 *
 *  Guided Missile
 *
 *************************************/


static const char *gmissile_sample_names[] =
{
	"*gmissile",
	"1.wav",	/* missle */
	"2.wav",	/* explosion */
	0
};


static struct Samplesinterface gmissile_samples_interface =
{
	1,	/* 1 channel */
	gmissile_sample_names
};


MACHINE_DRIVER_START( gmissile_sound )
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gmissile_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.9)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(gmissile_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.9)
MACHINE_DRIVER_END


WRITE8_HANDLER( gmissile_sh_port_1_w )
{
	/* note that the schematics shows the left and right explosions
       reversed (D5=R, D7=L), but the software confirms that
       ours is right */

	UINT8 rising_bits = data & ~port_1_last;

	/* D0 and D1 are not connected */

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	/* if (data & 0x10)  enable RIGHT MISSILE sound (goes to right speaker) */
	if (rising_bits & 0x10) sample_start_n(1, 0, 0, 0);

	/* if (data & 0x20)  enable LEFT EXPLOSION sound (goes to left speaker) */
	output_set_value("L_EXP_LIGHT", (data >> 5) & 0x01);
	if (rising_bits & 0x20) sample_start_n(0, 0, 1, 0);

	/* if (data & 0x40)  enable LEFT MISSILE sound (goes to left speaker) */
	if (rising_bits & 0x40) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x80)  enable RIGHT EXPLOSION sound (goes to right speaker) */
	output_set_value("R_EXP_LIGHT", (data >> 7) & 0x01);
	if (rising_bits & 0x80) sample_start_n(1, 0, 1, 0);

	port_1_last = data;
}


WRITE8_HANDLER( gmissile_sh_port_2_w )
{
	/* set AIRPLANE/COPTER/JET PAN(data & 0x07) */

	/* set TANK PAN((data >> 3) & 0x07) */

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( gmissile_sh_port_3_w )
{
	/* if (data & 0x01)  enable AIRPLANE (bi-plane) sound (goes to AIRPLANE/COPTER/JET panning circuit) */

	/* if (data & 0x02)  enable TANK sound (goes to TANK panning circuit) */

	/* if (data & 0x04)  enable COPTER sound (goes to AIRPLANE/COPTER/JET panning circuit) */

	/* D3 and D4 are not connected */

	/* if (data & 0x20)  enable JET (3 fighter jets) sound (goes to AIRPLANE/COPTER/JET panning circuit) */

	/* D6 and D7 are not connected */
}



/*************************************
 *
 *  M-4
 *
 *************************************/

/* Noise clock was breadboarded and measured at 3760Hz */


static const char *m4_sample_names[] =
{
	"*m4",
	"1.wav",	/* missle */
	"2.wav",	/* explosion */
	0
};


static struct Samplesinterface m4_samples_interface =
{
	2,	/* 2 channels */
	m4_sample_names
};


MACHINE_DRIVER_START( m4_sound )
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(m4_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(m4_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1)
MACHINE_DRIVER_END


WRITE8_HANDLER( m4_sh_port_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	/* D0 and D1 are not connected */

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	/* if (data & 0x10)  enable LEFT PLAYER SHOT sound (goes to left speaker) */
	if (rising_bits & 0x10) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x20)  enable RIGHT PLAYER SHOT sound (goes to right speaker) */
	if (rising_bits & 0x20) sample_start_n(1, 0, 0, 0);

	/* if (data & 0x40)  enable LEFT PLAYER EXPLOSION sound via 300K res (goes to left speaker) */
	if (rising_bits & 0x40) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x80)  enable RIGHT PLAYER EXPLOSION sound via 300K res (goes to right speaker) */
	if (rising_bits & 0x80) sample_start_n(1, 1, 1, 0);

	port_1_last = data;
}


WRITE8_HANDLER( m4_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	/* if (data & 0x01)  enable LEFT PLAYER EXPLOSION sound via 510K res (goes to left speaker) */
	if (rising_bits & 0x01) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x02)  enable RIGHT PLAYER EXPLOSION sound via 510K res (goes to right speaker) */
	if (rising_bits & 0x02) sample_start_n(1, 1, 1, 0);

	/* if (data & 0x04)  enable LEFT TANK MOTOR sound (goes to left speaker) */

	/* if (data & 0x08)  enable RIGHT TANK MOTOR sound (goes to right speaker) */

	/* if (data & 0x10)  enable sound that is playing while the right plane is
                         flying.  Circuit not named on schematics  (goes to left speaker) */

	/* if (data & 0x20)  enable sound that is playing while the left plane is
                         flying.  Circuit not named on schematics  (goes to right speaker) */

	/* D6 and D7 are not connected */

	port_2_last = data;
}



/*************************************
 *
 *  Clowns
 *
 *  Discrete sound emulation: Mar 2005, D.R.
 *
 *************************************/


/* nodes - inputs */
#define CLOWNS_POP_BOTTOM_EN		NODE_01
#define CLOWNS_POP_MIDDLE_EN		NODE_02
#define CLOWNS_POP_TOP_EN			NODE_03
#define CLOWNS_SPRINGBOARD_HIT_EN	NODE_04
#define CLOWNS_SPRINGBOARD_MISS_EN	NODE_05

/* nodes - sounds */
#define CLOWNS_NOISE				NODE_06
#define CLOWNS_POP_SND				NODE_07
#define CLOWNS_SB_HIT_SND			NODE_08
#define CLOWNS_SB_MISS_SND			NODE_09

/* nodes - adjusters */
#define CLOWNS_MUSIC_ADJ			NODE_11


static const discrete_lfsr_desc clowns_lfsr =
{
	DISC_CLK_IS_FREQ,
	17,					/* bit length */
						/* the RC network fed into pin 4, has the effect
                           of presetting all bits high at power up */
	0x1ffff,			/* reset value */
	4,					/* use bit 4 as XOR input 0 */
	16,					/* use bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* feedback stage1 is XOR */
	DISC_LFSR_OR,		/* feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* feedback stage3 replaces the shifted register contents */
	0x000001,			/* everything is shifted into the first bit only */
	0,					/* output is not inverted */
	12					/* output bit */
};


static const discrete_op_amp_tvca_info clowns_music_tvca_info =
{
	RES_M(3.3),				/* r502 */
	RES_K(10) + RES_K(680),	/* r505 + r506 */
	0,
	RES_K(680),				/* r503 */
	RES_K(10),				/* r500 */
	0,
	RES_K(680),				/* r501 */
	0,
	0,
	0,
	0,
	CAP_U(.001),			/* c500 */
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_op_amp_tvca_info clowns_pop_tvca_info =
{
	RES_M(2.7),		/* r304 */
	RES_K(680),		/* r303 */
	0,
	RES_K(680),		/* r305 */
	RES_K(1),		/* j3 */
	0,
	RES_K(470),		/* r300 */
	RES_K(1),		/* j3 */
	RES_K(510),		/* r301 */
	RES_K(1),		/* j3 */
	RES_K(680),		/* r302 */
	CAP_U(.015),	/* c300 */
	CAP_U(.1),		/* c301 */
	CAP_U(.082),	/* c302 */
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG2
};


static const discrete_op_amp_osc_info clowns_sb_hit_osc_info =
{
	DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON,
	RES_K(820),		/* r200 */
	RES_K(33),		/* r203 */
	RES_K(150),		/* r201 */
	RES_K(240),		/* r204 */
	RES_M(1),		/* r202 */
	0,
	0,
	0,
	CAP_U(0.01),	/* c200 */
	12
};


static const discrete_op_amp_tvca_info clowns_sb_hit_tvca_info =
{
	RES_M(2.7),		/* r207 */
	RES_K(680),		/* r205 */
	0,
	RES_K(680),		/* r208 */
	RES_K(1),		/* j3 */
	0,
	RES_K(680),		/* r206 */
	0,0,0,0,
	CAP_U(1),		/* c201 */
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static const discrete_mixer_desc clowns_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(10),
	  RES_K(10),
	  RES_K(10),
	  RES_K(1) },
	{ 0,
	  0,
	  0,
	  CLOWNS_MUSIC_ADJ },
	{ 0,
	  CAP_U(0.022),
	  0,
	  0 },
	0,
	RES_K(100),
	0,
	CAP_U(1),
	0,
	500
};


static DISCRETE_SOUND_START(clowns_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_BOTTOM_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_MIDDLE_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_POP_TOP_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_SPRINGBOARD_HIT_EN)
	DISCRETE_INPUT_LOGIC(CLOWNS_SPRINGBOARD_MISS_EN)

	/* The low value of the pot is set to 1000.  A real 1M pot will never go to 0 anyways. */
	/* This will give the control more apparent volume range. */
	/* The music way overpowers the rest of the sounds anyways. */
	DISCRETE_ADJUSTMENT(CLOWNS_MUSIC_ADJ, 1, RES_M(1), 1000, DISC_LOGADJ, 3)

	/************************************************
     * Tone generator
     ************************************************/
	MIDWAY_TONE_GENERATOR(clowns_music_tvca_info)

	/************************************************
     * Balloon hit sounds
     * The LFSR is the same as boothill
     ************************************************/
	/* Noise clock was breadboarded and measured at 7700Hz */
	DISCRETE_LFSR_NOISE(CLOWNS_NOISE, 1, 1, 7700, 12.0, 0, 12.0/2, &clowns_lfsr)

	DISCRETE_OP_AMP_TRIG_VCA(NODE_30, CLOWNS_POP_TOP_EN, CLOWNS_POP_MIDDLE_EN, CLOWNS_POP_BOTTOM_EN, CLOWNS_NOISE, 0, &clowns_pop_tvca_info)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, RES_K(15), CAP_U(.01))
	DISCRETE_CRFILTER(NODE_32, 1, NODE_31, RES_K(15) + RES_K(39), CAP_U(.01))
	DISCRETE_GAIN(CLOWNS_POP_SND, NODE_32, RES_K(39)/(RES_K(15) + RES_K(39)))

	/************************************************
     * Springboard hit
     ************************************************/
	DISCRETE_OP_AMP_OSCILLATOR(NODE_40, 1, &clowns_sb_hit_osc_info)
	DISCRETE_OP_AMP_TRIG_VCA(NODE_41, CLOWNS_SPRINGBOARD_HIT_EN, 0, 0, NODE_40, 0, &clowns_sb_hit_tvca_info)
	/* The rest of the circuit is a filter.  The frequency response was calculated with SPICE. */
	DISCRETE_FILTER2(NODE_42, 1, NODE_41, 500, 1.0/.8, DISC_FILTER_LOWPASS)
	/* The filter has a gain of 0.5 */
	DISCRETE_GAIN(CLOWNS_SB_HIT_SND, NODE_42, .5)

	/************************************************
     * Springboard miss - INCOMPLETE
     ************************************************/
	DISCRETE_CONSTANT(CLOWNS_SB_MISS_SND, 0) /* Placeholder for incomplete sound */

	/************************************************
     * Combine all sound sources.
     ************************************************/
	DISCRETE_MIXER4(NODE_91, 1, CLOWNS_SB_HIT_SND, CLOWNS_SB_MISS_SND, CLOWNS_POP_SND, MIDWAY_TONE_SND, &clowns_mixer)

	DISCRETE_OUTPUT(NODE_91, 1)
DISCRETE_SOUND_END


static const char *clowns_sample_names[] =
{
	"*clowns",
	"miss.wav",
	0
};

static struct Samplesinterface clowns_samples_interface =
{
	1,	/* 1 channel */
	clowns_sample_names
};


MACHINE_DRIVER_START( clowns_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(clowns_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.70)

	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(clowns_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


WRITE8_HANDLER( clowns_sh_port_1_w )
{
	coin_counter_w(0, (data >> 0) & 0x01);

	clowns_set_controller_select((data >> 1) & 0x01);

	/* D2-D7 are not connected */
}


WRITE8_HANDLER( clowns_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	discrete_sound_w(CLOWNS_POP_BOTTOM_EN, (data >> 0) & 0x01);

	discrete_sound_w(CLOWNS_POP_MIDDLE_EN, (data >> 1) & 0x01);

	discrete_sound_w(CLOWNS_POP_TOP_EN, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	discrete_sound_w(CLOWNS_SPRINGBOARD_HIT_EN, (data >> 4) & 0x01);

	if (rising_bits & 0x20) sample_start_n(0, 0, 0, 0);  /* springboard miss */

	/* D6 and D7 are not connected */

	port_2_last = data;
}



/*************************************
 *
 *  Shuffleboard
 *
 *************************************/

/* Noise clock was breadboarded and measured at 1210Hz */


MACHINE_DRIVER_START( shuffle_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


WRITE8_HANDLER( shuffle_sh_port_1_w )
{
	/* if (data & 0x01)  enable CLICK (balls collide) sound */

	/* if (data & 0x02)  enable SHUFFLE ROLLOVER sound */

	sound_global_enable((data >> 2) & 0x01);

	/* set SHUFFLE ROLLING sound((data >> 3) & 0x07)  0, if not rolling,
                                                      faster rolling = higher number */

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( shuffle_sh_port_2_w )
{
	/* if (data & 0x01)  enable FOUL sound */

	coin_counter_w(0, (data >> 1) & 0x01);

	/* D2-D7 are not connected */
}



/*************************************
 *
 *  Dog Patch
 *
 *  We don't have the schematics, so this is all questionable.
 *  This game is most likely stereo as well.
 *
 *************************************/


static const discrete_op_amp_tvca_info dogpatch_music_tvca_info =
{
	RES_M(3.3),
	RES_K(10) + RES_K(680),
	0,
	RES_K(680),
	RES_K(10),
	0,
	RES_K(680),
	0,
	0,
	0,
	0,
	CAP_U(.001),
	0,
	0,
	12,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG0,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_TRG1,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE,
	DISC_OP_AMP_TRIGGER_FUNCTION_NONE
};


static DISCRETE_SOUND_START(dogpatch_discrete_interface)

	/************************************************
     * Tone generator
     ************************************************/
	MIDWAY_TONE_GENERATOR(dogpatch_music_tvca_info)

	/************************************************
     * Filter it to be AC.
     ************************************************/
	DISCRETE_CRFILTER(NODE_91, 1, MIDWAY_TONE_SND, RES_K(100), CAP_U(0.1))

	DISCRETE_OUTPUT(NODE_91, 5000)

DISCRETE_SOUND_END


MACHINE_DRIVER_START( dogpatch_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(dogpatch_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.3)
MACHINE_DRIVER_END


WRITE8_HANDLER( dogpatch_sh_port_w )
{
	/* D0 and D1 are most likely not used */

	coin_counter_w(0, (data >> 2) & 0x01);

	sound_global_enable((data >> 3) & 0x01);

	/* if (data & 0x10)  enable LEFT SHOOT sound */

	/* if (data & 0x20)  enable RIGHT SHOOT sound */

	/* if (data & 0x40)  enable CAN HIT sound */

	/* D7 is most likely not used */
}



/*************************************
 *
 *  Space Encounters
 *
 *************************************/

/* Noise clock was breadboarded and measured at 7515Hz */


static struct SN76477interface spcenctr_sn76477_interface =
{
	0,				/*  4 noise_res (N/C)        */
	0,				/*  5 filter_res (N/C)       */
	0,				/*  6 filter_cap (N/C)       */
	0,				/*  7 decay_res (N/C)        */
	0,				/*  8 attack_decay_cap (N/C) */
	RES_K(100), 	/* 10 attack_res             */
	RES_K(56),		/* 11 amplitude_res          */
	RES_K(10),		/* 12 feedback_res           */
	0,				/* 16 vco_voltage (N/C)      */
	CAP_U(0.047),	/* 17 vco_cap                */
	RES_K(56),		/* 18 vco_res                */
	5.0,			/* 19 pitch_voltage          */
	RES_K(150),		/* 20 slf_res                */
	CAP_U(1.0),		/* 21 slf_cap                */
	0,				/* 23 oneshot_cap (N/C)      */
	0,				/* 24 oneshot_res (N/C)      */
	1,				/* 22 vco                    */
	0,				/* 26 mixer A                */
	0,				/* 25 mixer B                */
	0,				/* 27 mixer C                */
	1,				/* 1  envelope 1             */
	0,				/* 28 envelope 2             */
	1				/* 9  enable (variable)      */
};


static const char *spcenctr_sample_names[] =
{
	"*spcenctr",
	"1.wav",	/* shot */
	"2.wav",	/* explosion */
	0
};


static struct Samplesinterface spcenctr_samples_interface =
{
	2,	/* 2 channels */
	spcenctr_sample_names
};


MACHINE_DRIVER_START( spcenctr_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(spcenctr_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(spcenctr_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END



WRITE8_HANDLER( spcenctr_sh_port_1_w )
{
	sound_global_enable((data >> 0) & 0x01);

	/* D1 is marked as 'OPTIONAL SWITCH VIDEO FOR COCKTAIL',
       but it is never set my the software */

	/* if (data & 0x04)  enable CRASH sound */

	/* D3-D7 are not connected */
}


WRITE8_HANDLER( spcenctr_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	/* set WIND SOUND FREQ(data & 0x0f)  0, if no wind */

	/* if (data & 0x10)  enable EXPLOSION sound */
	if (rising_bits & 0x10) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x20)  enable PLAYER SHOT sound */
	if (rising_bits & 0x20) sample_start_n(0, 0, 0, 0);

	/* D6 and D7 are not connected */

	port_2_last = data;
}


WRITE8_HANDLER( spcenctr_sh_port_3_w )
{
	/* if (data & 0x01)  enable SCREECH (hit the sides) sound */

	/* if (data & 0x02)  enable ENEMY SHOT sound */

	spcenctr_set_strobe_state((data >> 2) & 0x01);

	output_set_value("LAMP", (data >> 3) & 0x01);

	/* if (data & 0x10)  enable BONUS sound */

	SN76477_enable_w(0, (data >> 5) & 0x01);	/* saucer sound */

	/* D6 and D7 are not connected */
}



/*************************************
 *
 *  Phantom II
 *
 *************************************/


static const char *phantom2_sample_names[] =
{
	"*phantom2",
	"1.wav",	/* shot */
	"2.wav",	/* explosion */
	0
};


static struct Samplesinterface phantom2_samples_interface =
{
	2,	/* 2 channels */
	phantom2_sample_names
};


MACHINE_DRIVER_START( phantom2_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(phantom2_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1)
MACHINE_DRIVER_END


WRITE8_HANDLER( phantom2_sh_port_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	/* if (data & 0x01)  enable PLAYER SHOT sound */
	if (rising_bits & 0x01) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x02)  enable ENEMY SHOT sound */

	sound_global_enable((data >> 2) & 0x01);

	coin_counter_w(0, (data >> 3) & 0x01);

	/* if (data & 0x10)  enable RADAR sound */

	/* D5-D7 are not connected */

	port_1_last = data;
}


WRITE8_HANDLER( phantom2_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	/* D0-D2 are not connected */

	/* if (data & 0x08)  enable EXPLOSION sound */
	if (rising_bits & 0x08) sample_start_n(0, 1, 1, 0);

	output_set_value("EXPLAMP", (data >> 4) & 0x01);

	/* set JET SOUND FREQ((data >> 5) & 0x07)  0, if no jet sound */

	port_2_last = data;
}



/*************************************
 *
 *  Bowling Alley
 *
 *************************************/


MACHINE_DRIVER_START( bowler_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


WRITE8_HANDLER( bowler_sh_port_1_w )
{
	/* D0 - selects controller on the cocktail PCB */

	coin_counter_w(0, (data >> 1) & 0x01);

	sound_global_enable((data >> 2) & 0x01);

	/* if (data & 0x08)  enable FOUL sound */

	/* D4 - appears to be a screen flip, but it's
            shown unconnected on the schematics for both the
            upright and cocktail PCB's */

	/* D5 - triggered on a 'strike', sound circuit not labeled */

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( bowler_sh_port_2_w )
{
	/* set BALL ROLLING SOUND FREQ(data & 0x0f)
       0, if no rolling, 0x08 used during ball return */

	/* D4 -  triggered when the ball crosses the foul line,
             sound circuit not labeled */

	/* D5 - triggered on a 'spare', sound circuit not labeled */

	/* D6 and D7 are not connected */
}


WRITE8_HANDLER( bowler_sh_port_3_w )
{
	/* regardless of the data, enable BALL HITS PIN 1 sound
       (top circuit on the schematics) */
}


WRITE8_HANDLER( bowler_sh_port_4_w )
{
	/* regardless of the data, enable BALL HITS PIN 2 sound
       (bottom circuit on the schematics) */
}


WRITE8_HANDLER( bowler_sh_port_5_w )
{
	/* not sure, appears to me trigerred alongside the two
       BALL HITS PIN sounds */
}


WRITE8_HANDLER( bowler_sh_port_6_w )
{
	/* D0 is not connected */

	/* D3 is not connected */

	/* D6 and D7 are not connected */

	/* D1, D2, D4 and D5 have something to do with a chime circuit.
       D1 and D4 are HI when a 'strike' happens, and D2 and D5 are
       HI on a 'spare' */
}



/*************************************
 *
 *  Space Invaders
 *
 *  Author      : Tormod Tjaberg
 *  Created     : 1997-04-09
 *  Description : Sound routines for the 'invaders' games
 *
 *  Note:
 *  The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 *  excellent space invader emulator and converted to signed samples so
 *  they would work under SEAL. The port info was also gleaned from
 *  his emulator. These sounds should also work on all the invader games.
 *
 *************************************/


static struct SN76477interface invaders_sn76477_interface =
{
	0,			/*  4 noise_res (N/C)        */
	0,			/*  5 filter_res (N/C)       */
	0,			/*  6 filter_cap (N/C)       */
	0,			/*  7 decay_res (N/C)        */
	0,			/*  8 attack_decay_cap (N/C) */
	RES_K(100), /* 10 attack_res             */
	RES_K(56),	/* 11 amplitude_res          */
	RES_K(10),	/* 12 feedback_res           */
	0,			/* 16 vco_voltage (N/C)      */
	CAP_U(0.1),	/* 17 vco_cap                */
	RES_K(8.2),	/* 18 vco_res                */
	5.0,		/* 19 pitch_voltage          */
	RES_K(120),	/* 20 slf_res                */
	CAP_U(1.0),	/* 21 slf_cap                */
	0,			/* 23 oneshot_cap (N/C)      */
	0,			/* 24 oneshot_res (N/C)      */
	1,			/* 22 vco                    */
	0,			/* 26 mixer A                */
	0,			/* 25 mixer B                */
	0,			/* 27 mixer C                */
	1,			/* 1  envelope 1             */
	0,			/* 28 envelope 2             */
	1			/* 9  enable (variable)      */
};



static const char *invaders_sample_names[] =
{
	"*invaders",
	"1.wav",		/* shot/missle */
	"2.wav",		/* base hit/explosion */
	"3.wav",		/* invader hit */
	"4.wav",		/* fleet move 1 */
	"5.wav",		/* fleet move 2 */
	"6.wav",		/* fleet move 3 */
	"7.wav",		/* fleet move 4 */
	"8.wav",		/* UFO/saucer hit */
	"9.wav",		/* bonus base */
	0
};


static struct Samplesinterface invaders_samples_interface =
{
	6,	/* 6 channels */
	invaders_sample_names
};


MACHINE_DRIVER_START( invaders_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invaders_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invaders_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END


WRITE8_HANDLER( invaders_sh_port_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	SN76477_enable_w(0, (~data >> 0) & 0x01);	/* saucer sound */

	/* if (data & 0x02)  enable MISSILE sound */
	if (rising_bits & 0x02) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x04)  enable EXPLOSION sound */
	if (rising_bits & 0x04) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x08)  enable INVADER HIT sound */
	if (rising_bits & 0x08) sample_start_n(0, 2, 2, 0);

	/* if (data & 0x10)  enable BONUS MISSILE BASE sound */
	if (rising_bits & 0x10) sample_start_n(0, 5, 8, 0);

	sound_global_enable(data & 0x20);

	/* D6 and D7 are not connected */

	port_1_last = data;
}


WRITE8_HANDLER( invaders_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	/* set FLEET SOUND FREQ(data & 0x0f)  0, if no sound */
	if (rising_bits & 0x01) sample_start_n(0, 4, 3, 0);
	if (rising_bits & 0x02) sample_start_n(0, 4, 4, 0);
	if (rising_bits & 0x04) sample_start_n(0, 4, 5, 0);
	if (rising_bits & 0x08) sample_start_n(0, 4, 6, 0);

	/* if (data & 0x10)  enable SAUCER HIT sound */
	if (rising_bits & 0x10) sample_start_n(0, 3, 7, 0);

	/* the flip screen line is only connected on the cocktail PCB */
	if (invaders_is_cabinet_cocktail())
	{
		invaders_set_flip_screen((data >> 5) & 0x01);
	}

	/* D6 and D7 are not connected */

	port_2_last = data;
}



/*************************************
 *
 *  Blue Shark
 *
 *  Discrete sound emulation: Jan 2007, D.R.
 *
 *************************************/

/* Noise clock was breadboarded and measured at 7700Hz */


/* nodes - inputs */
#define BLUESHRK_OCTOPUS_EN		NODE_01
#define BLUESHRK_HIT_EN			NODE_02
#define BLUESHRK_SHARK_EN		NODE_03
#define BLUESHRK_SHOT_EN		NODE_04
#define BLUESHRK_GAME_ON_EN		NODE_05

/* nodes - sounds */
#define BLUESHRK_NOISE_1		NODE_11
#define BLUESHRK_NOISE_2		NODE_12
#define BLUESHRK_OCTOPUS_SND	NODE_13
#define BLUESHRK_HIT_SND		NODE_14
#define BLUESHRK_SHARK_SND		NODE_15
#define BLUESHRK_SHOT_SND		NODE_16


static const discrete_555_desc blueshrk_555_H1A =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC | DISC_555_TRIGGER_IS_LOGIC,
	5,				/* B+ voltage of 555 */
	DEFAULT_555_VALUES
};


static const discrete_555_desc blueshrk_555_H1B =
{
	DISC_555_OUT_ENERGY | DISC_555_OUT_DC,
	5,				/* B+ voltage of 555 */
	12,				/* the OC buffer H2 converts the output voltage to 12V. */
	DEFAULT_555_THRESHOLD, DEFAULT_555_TRIGGER
};


static const discrete_mixer_desc blueshrk_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	{ RES_K(750),
	  1.0/(1.0/RES_K(510) + 1.0/RES_K(22)),
	  RES_M(1),
	  RES_K(56) },
	{ 0 },
	{ CAP_U(1),
	  CAP_U(1),
	  CAP_U(1),
	  CAP_U(1) },
	0,
	RES_K(100),
	0,
	CAP_U(0.1),
	0,		/* Vref */
	700		/* final gain */
};


static DISCRETE_SOUND_START(blueshrk_discrete_interface)

	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC(BLUESHRK_OCTOPUS_EN)
	DISCRETE_INPUT_LOGIC(BLUESHRK_HIT_EN)
	DISCRETE_INPUT_LOGIC(BLUESHRK_SHARK_EN)
	DISCRETE_INPUT_LOGIC(BLUESHRK_SHOT_EN)
	DISCRETE_INPUT_LOGIC(BLUESHRK_GAME_ON_EN)

	/************************************************
     * Octopus sound
     ************************************************/
	DISCRETE_CONSTANT(BLUESHRK_OCTOPUS_SND, 0)	/* placeholder for incomplete sound */

	/************************************************
     * Hit sound
     ************************************************/
	/* the 555 trigger is connected to the cap, so when reset goes high, the 555 is triggered */
	/* but the 555_MSTABLE does not currently allow connection of the trigger to the cap */
	/* so we will cheat and add a pulse 1 sample wide to trigger it */
	DISCRETE_ONESHOT(NODE_30, BLUESHRK_HIT_EN, 1, /* 1 sample wide */ 0, DISC_ONESHOT_REDGE | DISC_ONESHOT_NORETRIG | DISC_OUT_ACTIVE_LOW)
	DISCRETE_555_MSTABLE(NODE_31, BLUESHRK_HIT_EN, NODE_30, RES_K(47), CAP_U(2.2), &blueshrk_555_H1A)
	DISCRETE_LOGIC_INVERT(NODE_32, 1, BLUESHRK_HIT_EN)
	DISCRETE_COUNTER(NODE_33, 1, /*RST*/ NODE_32, /*CLK*/ NODE_31, 1, DISC_COUNT_UP, 0, DISC_CLK_ON_F_EDGE)
	DISCRETE_SWITCH(NODE_34, 1, NODE_33, CAP_U(0.015) + CAP_U(0.01), CAP_U(0.022))
	DISCRETE_555_ASTABLE(BLUESHRK_HIT_SND, BLUESHRK_HIT_EN, RES_K(22), RES_K(39), NODE_34, &blueshrk_555_H1B)

	/************************************************
     * Shark sound
     ************************************************/
	DISCRETE_CONSTANT(BLUESHRK_SHARK_SND, 0)	/* paceholder for incomplete sound */

	/************************************************
     * Shot sound
     ************************************************/
	DISCRETE_CONSTANT(BLUESHRK_SHOT_SND, 0)		/* placeholder for incomplete sound */

	/************************************************
     * Combine all sound sources.
     ************************************************/
	DISCRETE_MIXER4(NODE_91, BLUESHRK_GAME_ON_EN, BLUESHRK_OCTOPUS_SND, BLUESHRK_HIT_SND, BLUESHRK_SHARK_SND, BLUESHRK_SHOT_SND, &blueshrk_mixer)

	DISCRETE_OUTPUT(NODE_91, 1)
DISCRETE_SOUND_END


MACHINE_DRIVER_START( blueshrk_sound )
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(blueshrk_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


WRITE8_HANDLER( blueshrk_sh_port_w )
{
	discrete_sound_w(BLUESHRK_GAME_ON_EN, (data >> 0) & 0x01);

	/* discrete_sound_w(BLUESHRK_SHOT_EN, (data >> 1) & 0x01); */

	discrete_sound_w(BLUESHRK_HIT_EN, (data >> 2) & 0x01);

	/* discrete_sound_w(BLUESHRK_SHARK_EN, (data >> 3) & 0x01); */

	/* if (data & 0x10)  enable KILLED DIVER sound, this circuit
       doesn't appear to be on the schematics */

	/* discrete_sound_w(BLUESHRK_OCTOPUS_EN, (data >> 5) & 0x01); */

	/* D6 and D7 are not connected */
}



/*************************************
 *
 *  Space Invaders II (cocktail)
 *
 *************************************/


static struct SN76477interface invad2ct_p1_sn76477_interface =
{
	0,			/*  4 noise_res (N/C)        */
	0,			/*  5 filter_res (N/C)       */
	0,			/*  6 filter_cap (N/C)       */
	0,			/*  7 decay_res (N/C)        */
	0,			/*  8 attack_decay_cap (N/C) */
	RES_K(100), /* 10 attack_res             */
	RES_K(56),	/* 11 amplitude_res          */
	RES_K(10),	/* 12 feedback_res           */
	0,			/* 16 vco_voltage (N/C)      */
	CAP_U(0.1),	/* 17 vco_cap                */
	RES_K(8.2),	/* 18 vco_res                */
	5.0,		/* 19 pitch_voltage          */
	RES_K(120),	/* 20 slf_res                */
	CAP_U(1.0),	/* 21 slf_cap                */
	0,			/* 23 oneshot_cap (N/C)      */
	0,			/* 24 oneshot_res (N/C)      */
	1,			/* 22 vco                    */
	0,			/* 26 mixer A                */
	0,			/* 25 mixer B                */
	0,			/* 27 mixer C                */
	1,			/* 1  envelope 1             */
	0,			/* 28 envelope 2             */
	1			/* 9  enable (variable)      */
};


static struct SN76477interface invad2ct_p2_sn76477_interface =
{
	0,			  /*  4 noise_res (N/C)        */
	0,			  /*  5 filter_res (N/C)       */
	0,			  /*  6 filter_cap (N/C)       */
	0,			  /*  7 decay_res (N/C)        */
	0,			  /*  8 attack_decay_cap (N/C) */
	RES_K(100),   /* 10 attack_res             */
	RES_K(56),	  /* 11 amplitude_res          */
	RES_K(10),	  /* 12 feedback_res           */
	0,			  /* 16 vco_voltage (N/C)      */
	CAP_U(0.047), /* 17 vco_cap                */
	RES_K(39),	  /* 18 vco_res                */
	5.0,		  /* 19 pitch_voltage          */
	RES_K(120),	  /* 20 slf_res                */
	CAP_U(1.0),	  /* 21 slf_cap                */
	0,			  /* 23 oneshot_cap (N/C)      */
	0,			  /* 24 oneshot_res (N/C)      */
	1,			  /* 22 vco                    */
	0,			  /* 26 mixer A                */
	0,			  /* 25 mixer B                */
	0,			  /* 27 mixer C                */
	1,			  /* 1  envelope 1             */
	0,			  /* 28 envelope 2             */
	1			  /* 9  enable (variable)      */
};


static const char *invad2ct_sample_names[] =
{
	"*invad2ct",
	"1.wav",		/* shot/missle */
	"2.wav",		/* base hit/explosion */
	"3.wav",		/* invader hit */
	"4.wav",		/* fleet move 1 */
	"5.wav",		/* fleet move 2 */
	"6.wav",		/* fleet move 3 */
	"7.wav",		/* fleet move 4 */
	"8.wav",		/* UFO/saucer hit */
	"9.wav",		/* bonus base */
	"11.wav",		/* shot/missle - player 2 */
	"12.wav",		/* base hit/explosion - player 2 */
	"13.wav",		/* invader hit - player 2 */
	"14.wav",		/* fleet move 1 - player 2 */
	"15.wav",		/* fleet move 2 - player 2 */
	"16.wav",		/* fleet move 3 - player 2 */
	"17.wav",		/* fleet move 4 - player 2 */
	"18.wav",		/* UFO/saucer hit - player 2 */
	0
};


static struct Samplesinterface invad2ct_samples_interface =
{
	6,	/* 6 channels */
	invad2ct_sample_names
};


MACHINE_DRIVER_START( invad2ct_sound )
	MDRV_SPEAKER_STANDARD_STEREO("#1", "#2")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invad2ct_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "#1", 0.6)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(invad2ct_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "#2", 0.6)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invad2ct_p1_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "#1", 0.6)

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(invad2ct_p2_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "#2", 0.6)
MACHINE_DRIVER_END


WRITE8_HANDLER( invad2ct_sh_port_1_w )
{
	UINT8 rising_bits = data & ~port_1_last;

	SN76477_enable_w(0, (~data >> 0) & 0x01);	/* saucer sound */

	/* if (data & 0x02)  enable MISSILE sound (#1 speaker) */
	if (rising_bits & 0x02) sample_start_n(0, 0, 0, 0);

	/* if (data & 0x04)  enable EXPLOSION sound (#1 speaker) */
	if (rising_bits & 0x04) sample_start_n(0, 1, 1, 0);

	/* if (data & 0x08)  enable INVADER HIT sound (#1 speaker) */
	if (rising_bits & 0x08) sample_start_n(0, 2, 2, 0);

	/* if (data & 0x10)  enable BONUS MISSILE BASE sound (#1 speaker) */
	if (rising_bits & 0x10) sample_start_n(0, 5, 8, 0);		/* BONUS MISSILE BASE */

	sound_global_enable(data & 0x20);

	/* D6 and D7 are not connected */

	port_1_last = data;
}


WRITE8_HANDLER( invad2ct_sh_port_2_w )
{
	UINT8 rising_bits = data & ~port_2_last;

	/* set FLEET SOUND FREQ(data & 0x0f) (#1 speaker)  0, if no sound */
	if (rising_bits & 0x01) sample_start_n(0, 4, 3, 0);
	if (rising_bits & 0x02) sample_start_n(0, 4, 4, 0);
	if (rising_bits & 0x04) sample_start_n(0, 4, 5, 0);
	if (rising_bits & 0x08) sample_start_n(0, 4, 6, 0);

	/* if (data & 0x10)  enable SAUCER HIT sound (#1 speaker) */
	if (rising_bits & 0x10) sample_start_n(0, 3, 7, 0);

	/* D5-D7 are not connected */

	port_2_last = data;
}


WRITE8_HANDLER( invad2ct_sh_port_3_w )
{
	UINT8 rising_bits = data & ~port_3_last;

	SN76477_enable_w(1, (~data >> 0) & 0x01);	/* saucer sound */

	/* if (data & 0x02)  enable MISSILE sound (#2 speaker) */
	if (rising_bits & 0x02) sample_start_n(1, 0, 9, 0);

	/* if (data & 0x04)  enable EXPLOSION sound (#2 speaker) */
	if (rising_bits & 0x04) sample_start_n(1, 1, 10, 0);

	/* if (data & 0x08)  enable INVADER HIT sound (#2 speaker) */
	if (rising_bits & 0x08) sample_start_n(1, 2, 11, 0);

	/* if (data & 0x10)  enable BONUS MISSILE BASE sound (#2 speaker) */
	if (rising_bits & 0x10) sample_start_n(1, 5, 8, 0);

	/* D5-D7 are not connected */

	port_3_last = data;
}


WRITE8_HANDLER( invad2ct_sh_port_4_w )
{
	UINT8 rising_bits = data & ~port_4_last;

	/* set FLEET SOUND FREQ(data & 0x0f) (#2 speaker)  0, if no sound */
	if (rising_bits & 0x01) sample_start_n(1, 4, 12, 0);
	if (rising_bits & 0x02) sample_start_n(1, 4, 13, 0);
	if (rising_bits & 0x04) sample_start_n(1, 4, 14, 0);
	if (rising_bits & 0x08) sample_start_n(1, 4, 15, 0);

	/* if (data & 0x10)  enable SAUCER HIT sound (#2 speaker) */
	if (rising_bits & 0x10) sample_start_n(1, 3, 16, 0);

	/* D5-D7 are not connected */

	port_4_last = data;
}
