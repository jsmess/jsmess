/*************************************************************************

    audio\vicdual.c

*************************************************************************/

#include "driver.h"
#include "vicdual.h"
#include "sound/discrete.h"
#include "sound/samples.h"


/************************************************************************
 * frogs Sound System Analog emulation
 * Oct 2004, Derrick Renaud
 ************************************************************************/

/* Discrete Sound Input Nodes */
#define FROGS_FLY_EN		NODE_01
#define FROGS_JUMP_EN		NODE_03
#define FROGS_HOP_EN		NODE_04
#define FROGS_TONGUE_EN		NODE_05
#define FROGS_CAPTURE_EN	NODE_06
#define FROGS_SPLASH_EN		NODE_08

/* Nodes - Sounds */
#define FROGS_BUZZZ_SND		NODE_11
#define FROGS_BOING_SND		NODE_13
#define FROGS_HOP_SND		NODE_14
#define FROGS_ZIP_SND		NODE_15
#define FROGS_CROAK_SND		NODE_16
#define FROGS_SPLASH_SND	NODE_18
/* VRs */
#define FROGS_R93			NODE_25

static const discrete_555_desc frogsZip555m =
{
	DISC_555_TRIGGER_IS_LOGIC | DISC_555_OUT_DC | DISC_555_OUT_CAP,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

static const discrete_555_cc_desc frogsZip555cc =
{
	DISC_555_OUT_CAP | DISC_555_OUT_DC,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES,
	12,		// B+ voltage of the Constant Current source
	0.6		// Q13 Vbe
};

static const discrete_mixer_desc frogsMixer =
{
	DISC_MIXER_IS_OP_AMP,
	{RES_K(1), RES_K(5)},
	{FROGS_R93, 0},
	{CAP_U(0.01), CAP_U(0.01)},
	0, RES_K(56), 0, CAP_U(0.1), 0, 10000
};

DISCRETE_SOUND_START(frogs_discrete_interface)
	/************************************************
     * Input register mapping for frogs
     *
     * All inputs are inverted by initial transistor.
     ************************************************/
	DISCRETE_INPUT_LOGIC(FROGS_FLY_EN)
	DISCRETE_INPUT_NOT(FROGS_JUMP_EN)
	DISCRETE_INPUT_NOT(FROGS_HOP_EN)
	DISCRETE_INPUT_NOT(FROGS_TONGUE_EN)
	DISCRETE_INPUT_NOT(FROGS_CAPTURE_EN)
	DISCRETE_INPUT_NOT(FROGS_SPLASH_EN)

	DISCRETE_ADJUSTMENT_TAG(FROGS_R93, 1, RES_M(1), RES_K(10), DISC_LOGADJ, "R93")

	DISCRETE_555_MSTABLE(NODE_30, 1, FROGS_TONGUE_EN, RES_K(100), CAP_U(1), &frogsZip555m)

	/* Q11 & Q12 transform the voltage from the oneshot U4, to what is
     * needed by the 555CC circuit.  Vin to R29 must be > 1V for things
     * to change.  <=1 then The Vout of this circuit is 12V.
     * The Current thru R28 equals current thru R51. iR28 = iR51
     * So when Vin>.5, iR51 = (Vin-.5)/39k.  =0 when Vin<=.5
     * So the voltage drop across R28 is vR28 = iR51 * 22k.
     * Finally the Vout = 12 - vR28.
     * Note this formula only works when Vin < 39/(22+39)*12V+1.
     * Which it always is, due to the 555 clamping to 12V*2/3.
     * The Zip effect is hard to emulate 100% due to loading effects
     * of the output stage on the charge stage.  So I added some values
     * to get a similar waveshape to the breadboarded circuit.
     */
	DISCRETE_TRANSFORM5(NODE_31, 1, 12, NODE_30, .5, RES_K(22)/RES_K(39), 0, "012-P4>*3*-")

	DISCRETE_555_CC(NODE_32, 1, NODE_31, RES_K(1.1), CAP_U(0.14), 0, RES_K(100), 500, &frogsZip555cc)

	DISCRETE_MIXER2(NODE_90, 1, NODE_32, 0, &frogsMixer)

	DISCRETE_OUTPUT(NODE_90, 1)

DISCRETE_SOUND_END

static const char *frogs_sample_names[] =
{
	"*frogs",
	"boing.wav",
	"buzzz.wav",
	"croak.wav",
	"hop.wav",
	"splash.wav",
	"zip.wav",
	0       /* end of array */
};

struct Samplesinterface frogs_samples_interface =
{
	5,	/* 5 channels */
	frogs_sample_names
};

void croak_callback(int param)
{
	sample_stop(2);
}

WRITE8_HANDLER( frogs_sh_port2_w )
{
	static int last_croak = 0;
	static int last_buzzz = 0;
	int new_croak = data & 0x08;
	int new_buzzz = data & 0x10;

//  discrete_sound_w(FROGS_HOP_EN, data & 0x01);
//  discrete_sound_w(FROGS_JUMP_EN, data & 0x02);
	discrete_sound_w(FROGS_TONGUE_EN, data & 0x04);
//  discrete_sound_w(FROGS_CAPTURE_EN, data & 0x08);
//  discrete_sound_w(FROGS_FLY_EN, data & 0x10);
//  discrete_sound_w(FROGS_SPLASH_EN, data & 0x80);

	if (data & 0x01)
		sample_start (3, 3, 0);	// Hop
	if (data & 0x02)
		sample_start (0, 0, 0);	// Boing
	if (new_croak)
		sample_start (2, 2, 0);	// Croak
	else
	{
		if (last_croak)
		{
			/* The croak will keep playing until .429s after being disabled */
			timer_adjust(croak_timer, 1.1 * RES_K(390) * CAP_U(1), 0, 0);
		}
	}
	if (new_buzzz)
	{
		/* The Buzzz sound starts off a little louder in volume then
         * settles down to a steady buzzz.  Whenever the trigger goes
         * low, the sound is disabled.  If it then goes high, the buzzz
         * then starts off louder again.  The games does this every time
         * the fly moves.
         * So I made the sample start with the louder effect and then play
         * for 12 seconds.  A fly should move before this.  If not the
         * sample loops, adding the loud part as if the fly moved.
         * This is obviously incorrect, but a fly never stands still for
         * 12 seconds.
         */
		if (!last_buzzz)
			sample_start (1, 1, 1);	// Buzzz
	}
	else
		sample_stop(1);
	if (data & 0x80)
		sample_start (4, 4, 0);	// Splash

	last_croak = new_croak;
	last_buzzz = new_buzzz;
}



/************************************************************************
 * brdrline Sound System Analog emulation
 * May 2006, Derrick Renaud
 ************************************************************************/
#if 0


/* Discrete Sound Input Nodes */
#define BRDRLINE_GUN_TRG_EN			NODE_01
#define BRDRLINE_JEEP_ON_EN			NODE_02
#define BRDRLINE_POINT_TRG_EN		NODE_03
#define BRDRLINE_HIT_TRG_EN			NODE_04
#define BRDRLINE_ANIMAL_TRG_EN		NODE_05
#define BRDRLINE_EMAR_TRG_EN		NODE_06
#define BRDRLINE_WALK_TRG_EN		NODE_07
#define BRDRLINE_CRY_TRG_EN			NODE_08

/* Nodes - Sounds */
#define BRDRLINE_GUN_TRG_SND		NODE_91
#define BRDRLINE_JEEP_ON_SND		NODE_92
#define BRDRLINE_POINT_TRG_SND		NODE_93
#define BRDRLINE_HIT_TRG_SND		NODE_94
#define BRDRLINE_ANIMAL_TRG_SND		NODE_95
#define BRDRLINE_EMAR_TRG_SND		NODE_96
#define BRDRLINE_WALK_TRG_SND		NODE_97
#define BRDRLINE_CRY_TRG_SND		NODE_98

DISCRETE_SOUND_START(brdrline_discrete_interface)
	/************************************************
     * Input register mapping
     ************************************************/
	DISCRETE_INPUT_LOGIC(BRDRLINE_GUN_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_JEEP_ON_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_POINT_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_HIT_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_ANIMAL_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_EMAR_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_WALK_TRG_EN)
	DISCRETE_INPUT_LOGIC(BRDRLINE_CRY_TRG_EN)

	/************************************************
     * GUN TRG
     ************************************************/
	DISCRETE_LFSR_NOISE(NODE_10, 1, 1,CLK,AMPL,FEED,BIAS,LFSRTB)
	DISCRETE_MIXER2(NODE_11, 1, NODE_10,IN1,INFO)
	DISCRETE_FILTER2(NODE_12, 1, NODE_11,FREQ,DAMP,TYPE)
	DISCRETE_ONESHOT(NODE_13, BRDRLINE_GUN_TRG_EN, DEFAULT_TTL_V_LOGIC_1,
		TIME_OF_74LS123(RES_K(47), CAP_U(1)),	// R155, C73
		DISC_ONESHOT_FEDGE | DISC_ONESHOT_RETRIG | DISC_OUT_ACTIVE_LOW)
	DISCRETE_RCDISC4(NODE_14, 1, NODE_13,RVAL0,RVAL1,RVAL2,CVAL,VP,TYPE)
	DISCRETE_VCA(BRDRLINE_GUN_TRG_SND, 1, NODE_12, NODE_14,TYPE)

	/************************************************
     * JEEP ON
     ************************************************/
	DISCRETE_555_ASTABLE(NODE_20, BRDRLINE_JEEP_ON_EN,
		RES_K(1),	// R150
		RES_K(33),	// R153
		CAP_U(.1),	// C72
		OPTIONS)
	DISCRETE_COUNTER(NODE_21, 1, 1, NODE_20,MAX,DIR,INIT0, DISC_CLK_BY_COUNT)
	DISCRETE_COUNTER(NODE_22, 1, 1, NODE_20,MAX,DIR,INIT0, DISC_CLK_BY_COUNT)
	DISCRETE_TRANSFORM3(NODE,ENAB,INP0,INP1,INP2,FUNCT)
	DISCRETE_DAC_R1(NODE,ENAB,DATA,VDATA,LADDER)

	/************************************************
     * POINT TRG
     ************************************************/

	/************************************************
     * HIT TRG
     ************************************************/

	/************************************************
     * ANIMAL TRG
     ************************************************/

	/************************************************
     * EMAR TRG
     ************************************************/

	/************************************************
     * WALK TRG
     ************************************************/

	/************************************************
     * CRY TRG
     ************************************************/

	/************************************************
     * Mixer
     ************************************************/

	DISCRETE_OUTPUT(NODE_90, 1)

DISCRETE_SOUND_END
#endif
