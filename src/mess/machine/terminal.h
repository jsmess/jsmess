#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#include "emu.h"
#include "machine/serial.h"
#include "devcb.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _terminal_interface terminal_interface;
struct _terminal_interface
{
	devcb_write8 terminal_keyboard_func;
};

#define GENERIC_TERMINAL_INTERFACE(name) const terminal_interface (name) =

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/
#define TERMINAL_TAG "terminal"
#define TERMINAL_SCREEN_TAG "terminal_screen"

DECLARE_LEGACY_DEVICE(GENERIC_TERMINAL, terminal);

#define MCFG_GENERIC_TERMINAL_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, GENERIC_TERMINAL, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

#define MCFG_GENERIC_TERMINAL_REMOVE(_tag)		\
    MCFG_DEVICE_REMOVE(_tag)

#define MCFG_SERIAL_TERMINAL_ADD(_tag, _intrf, _baud) \
	MCFG_DEVICE_ADD(_tag, GENERIC_TERMINAL, _baud) \
	MCFG_DEVICE_CONFIG(_intrf)

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

READ_LINE_DEVICE_HANDLER( terminal_serial_r );
WRITE_LINE_DEVICE_HANDLER( terminal_serial_w );

WRITE8_DEVICE_HANDLER ( terminal_write );

INPUT_PORTS_EXTERN(generic_terminal);

UINT8 terminal_keyboard_handler(running_machine &machine, UINT8 last_code, UINT8 *scan_line, UINT8 *tx_shift, int *tx_state, device_t *device);

class serial_terminal_device :
      public device_t,
      public device_serial_port_interface
{
public:
	serial_terminal_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual void tx(UINT8 state) { terminal_serial_w(m_terminal, state); }
	DECLARE_WRITE8_MEMBER(serial_keyboard_func) { m_rbit = data&&1; m_owner->out_rx(m_rbit); }
protected:
	virtual void device_start() { m_owner = dynamic_cast<serial_port_device *>(owner()); }
	virtual void device_reset() { m_owner->out_rx(1); m_rbit = 1; }
private:
	required_device<device_t> m_terminal;
	serial_port_device *m_owner;
};

extern const device_type SERIAL_TERMINAL;

#endif /* __TERMINAL_H__ */
