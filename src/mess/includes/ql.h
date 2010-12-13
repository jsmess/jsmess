#ifndef __QL__
#define __QL__

#define SCREEN_TAG	"screen"

#define M68008_TAG	"ic18"
#define I8749_TAG	"ic24"
#define I8051_TAG	"i8051"
#define ZX8301_TAG	"ic22"
#define ZX8302_TAG	"ic23"
#define SPEAKER_TAG	"speaker"

#define MDV1_TAG	"mdv1"
#define MDV2_TAG	"mdv2"

#define X1 XTAL_15MHz
#define X2 XTAL_32_768kHz
#define X3 XTAL_4_436MHz
#define X4 XTAL_11MHz


class ql_state : public driver_device
{
public:
	ql_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, M68008_TAG),
		  m_ipc(*this, I8749_TAG),
		  m_zx8301(*this, ZX8301_TAG),
		  m_zx8302(*this, ZX8302_TAG),
		  m_speaker(*this, SPEAKER_TAG),
		  m_ram(*this, "messram")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_ipc;
	required_device<zx8301_device> m_zx8301;
	required_device<zx8302_device> m_zx8302;
	required_device<running_device> m_speaker;
	required_device<running_device> m_ram;

	virtual void machine_start();

	virtual bool video_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER( ipc_w );
	DECLARE_WRITE8_MEMBER( ipc_port1_w );
	DECLARE_WRITE8_MEMBER( ipc_port2_w );
	DECLARE_READ8_MEMBER( ipc_port2_r );
	DECLARE_READ8_MEMBER( ipc_t1_r );
	DECLARE_READ8_MEMBER( ipc_bus_r );
	DECLARE_READ8_MEMBER( ql_ram_r );
	DECLARE_WRITE8_MEMBER( ql_ram_w );
	DECLARE_WRITE_LINE_MEMBER( ql_baudx4_w );
	DECLARE_WRITE_LINE_MEMBER( ql_comdata_w );

	/* IPC state */
	UINT8 m_keylatch;
	int m_ser1_txd;
	int m_ser1_dtr;
	int m_ser2_rxd;
	int m_ser2_cts;
	int m_ipl;
	int m_comdata;
	int m_baudx4;
};

#endif
