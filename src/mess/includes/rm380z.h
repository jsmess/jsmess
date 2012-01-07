
/*

Research Machines RM 380Z

*/

#ifndef RM380Z_H_
#define RM380Z_H_

//
//
//

#define MAINCPU_TAG		"maincpu"
#define PORTS_ENABLED_HIGH	( m_port0 & 0x80 )
#define PORTS_ENABLED_LOW	( ( m_port0 & 0x80 ) == 0x00 )

#define chdimx 5
#define chdimy 9
#define ncx 8
#define ncy 16
#define screencols 40
#define screenrows 24

//
//
//


class rm380z_state : public driver_device
{
private: 
	
	UINT8 decode_videoram_char(UINT8 ch1,UINT8 ch2);
	
protected:
	virtual void machine_reset();

public:

	UINT8 m_port0;
	UINT8 m_port0_kbd;
	UINT8 m_port1;
	UINT8 m_fbfd;
	UINT8 m_fbfe;

	UINT8	m_vram[0x600];
	UINT8 m_mainVideoram[0x600];

	UINT8	m_vramchars[0x600];
	UINT8	m_vramattribs[0x600];

	int m_rasterlineCtr;
	emu_timer* m_vblankTimer;
	int m_old_fbfd;

	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_messram;
	optional_device<device_t> m_fdc;

	rm380z_state(const machine_config &mconfig, device_type type, const char *tag): driver_device(mconfig, type, tag), 
		m_maincpu(*this, MAINCPU_TAG),
		m_messram(*this, RAM_TAG),
		m_fdc(*this, "wd1771")
	{ 
	}

	DECLARE_WRITE8_MEMBER( port_write );
	DECLARE_READ8_MEMBER( port_read );

	DECLARE_READ8_MEMBER( main_read );
	DECLARE_WRITE8_MEMBER( main_write );

	DECLARE_READ8_MEMBER( videoram_read );
	DECLARE_WRITE8_MEMBER( videoram_write );

	//DECLARE_READ8_MEMBER(rm380z_io_r);
	//DECLARE_WRITE8_MEMBER(rm380z_io_w);

	DECLARE_WRITE8_MEMBER(disk_0_control);
	
	DECLARE_WRITE8_MEMBER( keyboard_put );

	void config_memory_map();
	int keyboard_decode();
	void update_screen(bitmap_t &bitmap);
};


#endif
