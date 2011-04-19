/***************************************************************************

    ibm_vga.h

    IBM PC standard VGA adaptor

***************************************************************************/

#ifndef IBM_VGA_H
#define IBM_VGA_H


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_IBM_VGA_ADD(_tag) \
    MCFG_DEVICE_ADD(_tag, IBM_VGA, 0)


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vga_device_config

class ibm_vga_device_config :	public device_config
{
	friend class ibm_vga_device;

	// construction/destruction
	ibm_vga_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
};


// ======================> ibm_vga_device

class ibm_vga_device : public device_t
{
	friend class ibm_vga_device_config;

	// construction/destruction
	ibm_vga_device(running_machine &_machine, const ibm_vga_device_config &config);

public:

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	UINT8 vga_crtc_r(int offset);
	void vga_crtc_w(int offset, UINT8 data);

	DECLARE_READ8_MEMBER( vga_port_03b0_r );
	DECLARE_WRITE8_MEMBER( vga_port_03b0_w );
	DECLARE_READ8_MEMBER( vga_port_03c0_r );
	DECLARE_WRITE8_MEMBER( vga_port_03c0_w );
	DECLARE_READ8_MEMBER( vga_port_03d0_r );
	DECLARE_WRITE8_MEMBER( vga_port_03d0_w );
private:
	// internal state
	const ibm_vga_device_config &m_config;

	UINT8 m_misc_out_reg;

	UINT8 m_feature_ctrl;

	UINT8 m_seq_idx;
	UINT8 m_seq_data[0x1f];

	UINT8 m_attr_idx;
	UINT8 m_attr_data[0x1f];
	UINT8 m_attr_state;

	UINT8 m_gc_idx;
	UINT8 m_gc_data[0x1f];

	UINT8 m_dac_mask;
	UINT8 m_dac_write_idx;
	UINT8 m_dac_read_idx;
	UINT8 m_dac_read;
	UINT8 m_dac_state;
	UINT8 m_dac_color[3][0x100];
	UINT8 *m_videoram;
	required_device<device_t> m_crtc;
};


// device type definition
extern const device_type IBM_VGA;


#endif /* IBM_VGA_H */

