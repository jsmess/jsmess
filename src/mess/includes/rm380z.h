
/*

Research Machines RM 380Z

*/

#ifndef RM380Z_H_
#define RM380Z_H_

//
//
//

#define RM380Z_MAINCPU_TAG		"maincpu"
#define RM380Z_PORTS_ENABLED_HIGH	( m_port0 & 0x80 )
#define RM380Z_PORTS_ENABLED_LOW	( ( m_port0 & 0x80 ) == 0x00 )

#define RM380Z_VIDEOMODE_40COL	0x01
#define RM380Z_VIDEOMODE_80COL	0x02

#define RM380Z_CHDIMX 5
#define RM380Z_CHDIMY 9
#define RM380Z_NCX 8
#define RM380Z_NCY 16
#define RM380Z_SCREENCOLS 80
#define RM380Z_SCREENROWS 24
#define RM380Z_SCREENSIZE 0x1200

//
//
//


class rm380z_state : public driver_device
{
private:

	void putChar(int charnum,int attribs,int x,int y,UINT16* pscr,unsigned char* chsb,int vmode);
	void decode_videoram_char(int pos,UINT8& chr,UINT8& attrib);
	void scroll_videoram();
	void config_videomode();

protected:
	virtual void machine_reset();

public:

	UINT8 m_port0;
	UINT8 m_port0_kbd;
	UINT8 m_port1;
	UINT8 m_fbfd;
	UINT8 m_fbfe;

	UINT8	m_mainVideoram[0x600];

	UINT8	m_vramchars[RM380Z_SCREENSIZE];
	UINT8	m_vramattribs[RM380Z_SCREENSIZE];
	UINT8	m_vram[RM380Z_SCREENSIZE];

	int m_rasterlineCtr;
	emu_timer* m_vblankTimer;
	int m_old_fbfd;
	int maxrow; // maximum row where the code wrote to videoram
	int m_pageAdder; // pages to add (incremented by one page when FBFD wraps)
	
	int m_videomode;
	int m_old_videomode;

	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_messram;
	optional_device<device_t> m_fdc;

	rm380z_state(const machine_config &mconfig, device_type type, const char *tag): driver_device(mconfig, type, tag),
		m_maincpu(*this, RM380Z_MAINCPU_TAG),
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

	DECLARE_WRITE8_MEMBER(disk_0_control);

	DECLARE_WRITE8_MEMBER( keyboard_put );

	void config_memory_map();
	int keyboard_decode();
	void update_screen(bitmap_t &bitmap);
};


#endif
