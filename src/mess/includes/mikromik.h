#ifndef __MIKROMIKKO__
#define __MIKROMIKKO__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"ic40"
#define I8212_TAG		"ic12"
#define I8237_TAG		"ic45"
#define I8253_TAG		"ic6"
#define UPD765_TAG		"ic15"
#define I8275_TAG		"ic59"
#define UPD7201_TAG		"ic11"
#define UPD7220_TAG		"ic101"
#define SPEAKER_TAG		"speaker"

class mm1_state : public driver_device
{
public:
	mm1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, I8085A_TAG),
		  m_iop(*this, I8212_TAG),
		  m_dmac(*this, I8237_TAG),
		  m_crtc(*this, I8275_TAG),
		  m_fdc(*this, UPD765_TAG),
		  m_mpsc(*this, UPD7201_TAG),
		  m_hgdc(*this, UPD7220_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<i8212_device> m_iop;
	required_device<device_t> m_dmac;
	required_device<device_t> m_crtc;
	required_device<device_t> m_fdc;
	required_device<device_t> m_mpsc;
	required_device<device_t> m_hgdc;
	required_device<device_t> m_speaker;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( ls259_w );
	DECLARE_READ8_MEMBER( kb_r );
	DECLARE_WRITE_LINE_MEMBER( dma_hrq_changed );
	DECLARE_READ8_MEMBER( mpsc_dack_r );
	DECLARE_WRITE8_MEMBER( mpsc_dack_w );
	DECLARE_WRITE_LINE_MEMBER( tc_w );
	DECLARE_WRITE_LINE_MEMBER( dack3_w );
	DECLARE_WRITE_LINE_MEMBER( itxc_w );
	DECLARE_WRITE_LINE_MEMBER( irxc_w );
	DECLARE_WRITE_LINE_MEMBER( auxc_w );
	DECLARE_WRITE_LINE_MEMBER( drq2_w );
	DECLARE_WRITE_LINE_MEMBER( drq1_w );
	DECLARE_READ_LINE_MEMBER( dsra_r );

	void scan_keyboard();

	// keyboard state
	int m_sense;
	int m_drive;
	UINT8 m_keydata;
	UINT8 *m_key_rom;

	// video state
	UINT8 *m_char_rom;
	int m_llen;

	// serial state
	int m_intc;
	int m_rx21;
	int m_tx21;
	int m_rcl;

	// floppy state
	int m_recall;
	int m_dack3;
	int m_tc;
};

#endif
