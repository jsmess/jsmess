#ifndef __TANDY2K__
#define __TANDY2K__

#include "machine/ram.h"

#define SCREEN_TAG		"screen"
#define I80186_TAG		"u76"
#define I8048_TAG		"m1"
#define I8255A_TAG		"u75"
#define I8251A_TAG		"u41"
#define I8253_TAG		"u40"
#define I8259A_0_TAG	"u42"
#define I8259A_1_TAG	"u43"
#define I8272A_TAG		"u121"
#define CRT9007_TAG		"u16"
#define CRT9212_0_TAG	"u55"
#define CRT9212_1_TAG	"u15"
#define CRT9021B_TAG	"u14"
#define WD1010_TAG		"u18"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

class tandy2k_state : public driver_device
{
public:
	tandy2k_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, I80186_TAG),
		  m_uart(*this, I8251A_TAG),
		  m_pit(*this, I8253_TAG),
		  m_fdc(*this, I8272A_TAG),
		  m_pic0(*this, I8259A_0_TAG),
		  m_pic1(*this, I8259A_1_TAG),
		  m_vpac(*this, CRT9007_TAG),
		  m_drb0(*this, CRT9212_0_TAG),
		  m_drb1(*this, CRT9212_1_TAG),
		  m_vac(*this, CRT9021B_TAG),
		  m_centronics(*this, CENTRONICS_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy0(*this, FLOPPY_0),
		  m_floppy1(*this, FLOPPY_1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_uart;
	required_device<device_t> m_pit;
	required_device<device_t> m_fdc;
	required_device<device_t> m_pic0;
	required_device<device_t> m_pic1;
	required_device<crt9007_device> m_vpac;
	required_device<crt9212_device> m_drb0;
	required_device<crt9212_device> m_drb1;
	required_device<crt9021_device> m_vac;
	required_device<device_t> m_centronics;
	required_device<device_t> m_speaker;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy0;
	required_device<device_t> m_floppy1;

	virtual void machine_start();

	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	void speaker_update();
	void dma_request(int line, int state);

	DECLARE_READ8_MEMBER( videoram_r );
	DECLARE_READ8_MEMBER( enable_r );
	DECLARE_WRITE8_MEMBER( enable_w );
	DECLARE_WRITE8_MEMBER( dma_mux_w );
	DECLARE_READ16_MEMBER( vpac_r );
	DECLARE_WRITE16_MEMBER( vpac_w );
	DECLARE_WRITE8_MEMBER( addr_ctrl_w );
	DECLARE_READ8_MEMBER( keyboard_x0_r );
	DECLARE_WRITE8_MEMBER( keyboard_y0_w );
	DECLARE_WRITE8_MEMBER( keyboard_y8_w );
	DECLARE_WRITE_LINE_MEMBER( busdmarq0_w );
	DECLARE_WRITE_LINE_MEMBER( rxrdy_w );
	DECLARE_WRITE_LINE_MEMBER( txrdy_w );
	DECLARE_WRITE_LINE_MEMBER( outspkr_w );
	DECLARE_WRITE_LINE_MEMBER( intbrclk_w );
	DECLARE_WRITE_LINE_MEMBER( rfrqpulse_w );
	DECLARE_READ8_MEMBER( ppi_pb_r );
	DECLARE_WRITE8_MEMBER( ppi_pc_w );
	DECLARE_WRITE_LINE_MEMBER( vpac_vlt_w );
	DECLARE_WRITE_LINE_MEMBER( vpac_drb_w );
	DECLARE_WRITE_LINE_MEMBER( vac_ld_ht_w );

	/* DMA state */
	UINT8 m_dma_mux;

	/* keyboard state */
	int m_kben;
	UINT16 m_keylatch;

	/* serial state */
	int m_extclk;
	int m_rxrdy;
	int m_txrdy;

	/* PPI state */
	int m_pb_sel;

	/* video state */
	UINT16 *m_char_ram;
	UINT16 *m_hires_ram;
	UINT16 m_palette[16];
	UINT8 m_vram_base;
	int m_vidouts;
	int m_clkspd;
	int m_clkcnt;

	/* sound state */
	int m_outspkr;
	int m_spkrdata;
};

#endif
