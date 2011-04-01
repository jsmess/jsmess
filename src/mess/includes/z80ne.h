/*****************************************************************************
 *
 * includes/z80ne.h
 *
 * Nuova Elettronica Z80NE
 *
 * http://www.z80ne.com/
 *
 ****************************************************************************/

#ifndef Z80NE_H_
#define Z80NE_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define Z80NE_CPU_SPEED_HZ		1920000	/* 1.92 MHz */

#define LX383_KEYS			16
#define LX383_DOWNSAMPLING	16

#define LX385_TAPE_SAMPLE_FREQ 38400

/* wave duration threshold */
typedef enum
{
	TAPE_300BPS  = 300, /*  300 bps */
	TAPE_600BPS  = 600, /*  600 bps */
	TAPE_1200BPS = 1200 /* 1200 bps */
} z80netape_speed;

typedef struct {
	struct {
		int length;		/* time cassette level is at input.level */
		int level;		/* cassette level */
		int bit;		/* bit being read */
	} input;
	struct {
		int length;		/* time cassette level is at output.level */
		int level;		/* cassette level */
		int bit;		/* bit to to output */
	} output;
	z80netape_speed speed;			/* 300 - 600 - 1200 */
	int wave_filter;
	int wave_length;
	int wave_short;
	int wave_long;
} cass_data_t;

typedef struct {
	int drq;
	int intrq;
	UINT8 drive; /* current drive */
	UINT8 head;  /* current head */
} wd17xx_state_t;


class z80ne_state : public driver_device
{
public:
	z80ne_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_videoram;
	UINT8 m_lx383_scan_counter;
	UINT8 m_lx383_key[LX383_KEYS];
	int m_lx383_downsampler;
	int m_nmi_delay_counter;
	int m_reset_delay_counter;
	device_t *m_ay31015;
	UINT8 m_lx385_ctrl;
	device_t *m_lx388_kr2376;
	emu_timer *m_cassette_timer;
	cass_data_t m_cass_data;
	wd17xx_state_t m_wd17xx_state;
};


/*----------- defined in machine/z80ne.c -----------*/

READ8_HANDLER(lx383_r);
WRITE8_HANDLER(lx383_w);
READ8_HANDLER(lx385_data_r);
WRITE8_HANDLER(lx385_data_w);
READ8_HANDLER(lx385_ctrl_r);
WRITE8_HANDLER(lx385_ctrl_w);
READ8_DEVICE_HANDLER(lx388_mc6847_videoram_r);
SCREEN_UPDATE(lx388);
READ8_HANDLER(lx388_data_r);
READ8_HANDLER(lx388_read_field_sync);
READ8_DEVICE_HANDLER(lx390_fdc_r);
WRITE8_DEVICE_HANDLER(lx390_fdc_w);
READ8_DEVICE_HANDLER(lx390_reset_bank);
WRITE8_DEVICE_HANDLER(lx390_motor_w);


DRIVER_INIT(z80ne);
DRIVER_INIT(z80net);
DRIVER_INIT(z80netb);
DRIVER_INIT(z80netf);
MACHINE_RESET(z80ne);
MACHINE_RESET(z80net);
MACHINE_RESET(z80netb);
MACHINE_RESET(z80netf);
MACHINE_START(z80ne);
MACHINE_START(z80net);
MACHINE_START(z80netb);
MACHINE_START(z80netf);

INPUT_CHANGED(z80ne_reset);
INPUT_CHANGED(z80ne_nmi);

#endif /* Z80NE_H_ */
