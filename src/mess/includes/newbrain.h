#ifndef __NEWBRAIN__
#define __NEWBRAIN__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"409"
#define COP420_TAG		"419"
#define MC6850_TAG		"459"
#define ADC0809_TAG		"427"
#define DAC0808_TAG		"461"
#define Z80CTC_TAG		"458"
#define FDC_Z80_TAG		"416"
#define UPD765_TAG		"418"
#define CASSETTE1_TAG	"cassette1"
#define CASSETTE2_TAG	"cassette2"

#define NEWBRAIN_EIM_RAM_SIZE			0x10000

#define NEWBRAIN_ENRG1_CLK				0x01
#define NEWBRAIN_ENRG1_TVP				0x04
#define NEWBRAIN_ENRG1_CTS				0x10
#define NEWBRAIN_ENRG1_DO				0x20
#define NEWBRAIN_ENRG1_PO				0x80
#define NEWBRAIN_ENRG1_UST_BIT_1_MASK	0x30
#define NEWBRAIN_ENRG1_UST_BIT_0_MASK	0xc0

#define NEWBRAIN_ENRG2_USERP			0x01
#define NEWBRAIN_ENRG2_ANP				0x02
#define NEWBRAIN_ENRG2_MLTMD			0x04
#define NEWBRAIN_ENRG2_MSPD				0x08
#define NEWBRAIN_ENRG2_ENOR				0x10
#define NEWBRAIN_ENRG2_ANSW				0x20
#define NEWBRAIN_ENRG2_ENOT				0x40
#define NEWBRAIN_ENRG2_CENTRONICS_OUT	0x80

#define NEWBRAIN_VIDEO_RV				0x01
#define NEWBRAIN_VIDEO_FS				0x02
#define NEWBRAIN_VIDEO_32_40			0x04
#define NEWBRAIN_VIDEO_UCR				0x08
#define NEWBRAIN_VIDEO_80L				0x40

class newbrain_state : public driver_device
{
public:
	newbrain_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* processor state */
	int pwrup;				/* power up */
	int userint;			/* user interrupt */
	int userint0;			/* parallel port interrupt */
	int clkint;				/* clock interrupt */
	int aciaint;			/* ACIA interrupt */
	int copint;				/* COP interrupt */
	int anint;				/* A/DC interrupt */
	int bee;				/* identity */
	UINT8 enrg1;			/* enable register 1 */
	UINT8 enrg2;			/* enable register 2 */
	int acia_rxd;			/* ACIA receive */
	int acia_txd;			/* ACIA transmit */

	/* COP420 state */
	UINT8 cop_bus;			/* data bus */
	int cop_so;				/* serial out */
	int cop_tdo;			/* tape data output */
	int cop_tdi;			/* tape data input */
	int cop_rd;				/* memory read */
	int cop_wr;				/* memory write */
	int cop_access;			/* COP access */

	/* keyboard state */
	int keylatch;			/* keyboard row */
	int keydata;			/* keyboard column */

	/* paging state */
	int paging;				/* paging enabled */
	int mpm;				/* multi paging mode ? */
	int a16;				/* address line 16 */
	UINT8 pr[16];			/* expansion interface paging register */
	UINT8 *eim_ram;			/* expansion interface RAM */

	/* floppy state */
	int fdc_int;			/* interrupt */
	int fdc_att;			/* attention */

	/* video state */
	int segment_data[16];	/* VF segment data */
	int tvcnsl;				/* TV console required */
	int tvctl;				/* TV control register */
	UINT16 tvram;			/* TV start address */
	UINT8 *char_rom;		/* character ROM */

	/* user bus state */
	UINT8 user;

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */
	emu_timer *pwrup_timer;	/* power up timer */

	/* devices */
	running_device *mc6850;
	running_device *z80ctc;
	running_device *upd765;
	running_device *cassette1;
	running_device *cassette2;
};

/* ---------- defined in video/newbrain.c ---------- */

MACHINE_CONFIG_EXTERN( newbrain_video );

#endif
