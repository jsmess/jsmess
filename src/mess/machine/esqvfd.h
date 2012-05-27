#ifndef ESQVFD_H
#define ESQVFD_H

#include "emu.h"

#define MCFG_ESQ2x40_ADD(_tag)	\
	MCFG_DEVICE_ADD(_tag, ESQ2x40, 60)

class esqvfd_t : public device_t {
public:
	typedef delegate<void (bool state)> line_cb;

	esqvfd_t(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER( write ) { write_char(data); }

    virtual void write_char(int data) = 0;
    virtual void update_display() = 0;

    UINT32 conv_segments(UINT16 segin);

protected:
    static const UINT8 AT_NORMAL      = 0x00;
    static const UINT8 AT_BOLD        = 0x01;
    static const UINT8 AT_UNDERLINE   = 0x02;
    static const UINT8 AT_BLINK       = 0x04;

	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

    int m_cursx, m_cursy;
    UINT8 m_chars[2][40];
    UINT8 m_attrs[2][40];
    UINT8 m_dirty[2][40];
};

class esq2x40_t : public esqvfd_t {
public:
	esq2x40_t(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void write_char(int data);
    virtual void update_display();

protected:
    virtual machine_config_constructor device_mconfig_additions() const;

private:
};

extern const device_type ESQ2x40;

#endif
