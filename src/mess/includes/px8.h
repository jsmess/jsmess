#ifndef __PX8__
#define __PX8__

#define UPD70008_TAG	"4a"
#define UPD7508_TAG		"2e"
#define HD6303_TAG		"13d"
#define SED1320_TAG		"7c"
#define I8251_TAG		"13e"
#define UPD7001_TAG		"1d"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"

#define PX8_VIDEORAM_MASK	0x17ff

class px8_state : public driver_device
{
public:
	px8_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* GAH40M state */
	UINT16 m_icr;				/* input capture register */
	UINT16 m_frc;				/* free running counter */
	UINT8 m_ier;				/* interrupt acknowledge register */
	UINT8 m_isr;				/* interrupt status register */
	UINT8 m_sio;				/* serial I/O register */
	int m_bank0;				/* */

	/* GAH40S state */
	UINT16 m_cnt;				/* microcassette tape counter */
	int m_swpr;				/* P-ROM power switch */
	UINT16 m_pra;				/* P-ROM address */
	UINT8 m_prd;				/* P-ROM data */

	/* memory state */
	int m_bk2;				/* */

	/* keyboard state */
	int m_ksc;				/* keyboard scan column */

	/* video state */
	UINT8 *m_video_ram;		/* LCD video RAM */

	/* devices */
	device_t *m_sed1320;
	device_t *m_cassette;
};

#endif
