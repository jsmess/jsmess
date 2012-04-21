/*********************************************************************

    a2videoterm.h

    Implementation of the Apple II Memory Expansion Card

*********************************************************************/

#ifndef __A2BUS_VIDEOTERM__
#define __A2BUS_VIDEOTERM__

#include "emu.h"
#include "machine/a2bus.h"
#include "video/mc6845.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class a2bus_videoterm_device:
    public device_t,
    public device_a2bus_card_interface
{
public:
    // construction/destruction
    a2bus_videoterm_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
    a2bus_videoterm_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

    // optional information overrides
    virtual machine_config_constructor device_mconfig_additions() const;
    virtual const rom_entry *device_rom_region() const;

    DECLARE_WRITE_LINE_MEMBER(vsync_changed);

    UINT8 *m_rom, *m_chrrom;
    UINT8 m_ram[512*4];
    int m_framecnt;

protected:
    virtual void device_start();
    virtual void device_reset();

    // overrides of standard a2bus slot functions
    virtual UINT8 read_c0nx(address_space &space, UINT8 offset);
    virtual void write_c0nx(address_space &space, UINT8 offset, UINT8 data);
    virtual UINT8 read_cnxx(address_space &space, UINT8 offset);
    virtual void write_cnxx(address_space &space, UINT8 offset, UINT8 data);
    virtual UINT8 read_c800(address_space &space, UINT16 offset);
    virtual void write_c800(address_space &space, UINT16 offset, UINT8 data);

    required_device<mc6845_device> m_crtc;

private:
    int m_rambank;
};

// device type definition
extern const device_type A2BUS_VIDEOTERM;

#endif /* __A2BUS_VIDEOTERM__ */

