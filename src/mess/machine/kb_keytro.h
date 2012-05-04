/***************************************************************************

    Keytronic Keyboard

***************************************************************************/

#ifndef __KB_KEYTRO_H__
#define __KB_KEYTRO_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _kb_keytronic_interface kb_keytronic_interface;
struct _kb_keytronic_interface
{
	devcb_write_line out_clock_func;
	devcb_write_line out_data_func;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
WRITE_LINE_DEVICE_HANDLER( kb_keytronic_clock_w );
WRITE_LINE_DEVICE_HANDLER( kb_keytronic_data_w );


/***************************************************************************
    INPUT PORTS
***************************************************************************/

INPUT_PORTS_EXTERN( kb_keytronic_pc );
INPUT_PORTS_EXTERN( kb_keytronic_at );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(KB_KEYTRONIC, kb_keytr);

#define MCFG_KB_KEYTRONIC_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, KB_KEYTRONIC, 0) \
	MCFG_DEVICE_CONFIG(_interface)


#include "machine/pc_kbdc.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> pc_kbd_keytronic_pc3270_device

class pc_kbd_keytronic_pc3270_device :	public device_t,
										public device_pc_kbd_interface
{
public:
	// construction/destruction
	pc_kbd_keytronic_pc3270_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	required_device<cpu_device> m_cpu;

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;
	virtual const rom_entry *device_rom_region() const;

	virtual DECLARE_WRITE_LINE_MEMBER(clock_write);
	virtual DECLARE_WRITE_LINE_MEMBER(data_write);

	DECLARE_READ8_MEMBER( internal_data_read );
	DECLARE_WRITE8_MEMBER( internal_data_write );
	DECLARE_READ8_MEMBER( p1_read );
	DECLARE_WRITE8_MEMBER( p1_write );
	DECLARE_READ8_MEMBER( p2_read );
	DECLARE_WRITE8_MEMBER( p2_write );
	DECLARE_READ8_MEMBER( p3_read );
	DECLARE_WRITE8_MEMBER( p3_write );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	UINT8	m_p1;
	UINT8	m_p1_data;
	UINT8	m_p2;
	UINT8	m_p3;
	UINT16	m_last_write_addr;
};


// device type definition
extern const device_type PC_KBD_KEYTRONIC_PC3270;

#endif  /* __KB_KEYTRO_H__ */
