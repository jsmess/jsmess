/***************************************************************************

    machine/mpc105.h

    Motorola MPC105 PCI bridge

***************************************************************************/

#ifndef MPC105_H
#define MPC105_H

#define MPC105_MEMORYBANK_COUNT		8

#define MCFG_MPC105_ADD(_tag, _cputag, _bankbase) \
    MCFG_DEVICE_ADD(_tag, MPC105, 0) \
    mpc105_device_config::static_set_cputag(device, _cputag); \
    mpc105_device_config::static_set_bank_base(device, _bankbase); \
	
// ======================> mpc105_device_config

class mpc105_device_config :  public device_config
{
    friend class mpc105_device;

    // construction/destruction
    mpc105_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_bank_base(device_config *device, int bank_base);
	static void static_set_cputag(device_config *device, const char *tag);
	
protected:
    // internal state goes here
    int m_bank_base;
	const char *m_cputag;
};



// ======================> mpc105_device

class mpc105_device : public device_t
{
    friend class mpc105_device_config;

    // construction/destruction
    mpc105_device(running_machine &_machine, const mpc105_device_config &config);

public:
	UINT32 pci_read(device_t *busdevice, int function, int offset, UINT32 mem_mask);
	void   pci_write(device_t *busdevice, int function, int offset, UINT32 data, UINT32 mem_mask);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();
	
	void update_memory();

    // internal state
    const mpc105_device_config &m_config;

private:
	int m_bank_base;
	UINT8 m_bank_enable;
	UINT32 m_bank_registers[8];	
	
	required_device<device_t>   m_maincpu;
};


// device type definition
extern const device_type MPC105;

UINT32 mpc105_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void mpc105_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

#endif /* MPC105_H */
