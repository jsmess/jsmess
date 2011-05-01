/***************************************************************************

    machine/mpc105.h

    Motorola MPC105 PCI bridge

***************************************************************************/

#ifndef MPC105_H
#define MPC105_H

#define MPC105_MEMORYBANK_COUNT		8

#define MCFG_MPC105_ADD(_tag, _cputag, _bankbase) \
    MCFG_DEVICE_ADD(_tag, MPC105, 0) \
    mpc105_device::static_set_cputag(*device, _cputag); \
    mpc105_device::static_set_bank_base(*device, _bankbase); \

// ======================> mpc105_device

class mpc105_device : public device_t
{
public:
    // construction/destruction
    mpc105_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	UINT32 pci_read(device_t *busdevice, int function, int offset, UINT32 mem_mask);
	void   pci_write(device_t *busdevice, int function, int offset, UINT32 data, UINT32 mem_mask);

	// inline configuration helpers
	static void static_set_bank_base(device_t &device, int bank_base);
	static void static_set_cputag(device_t &device, const char *tag);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

	void update_memory();

private:
	int m_bank_base;
	int m_bank_base_default;
	UINT8 m_bank_enable;
	UINT32 m_bank_registers[8];

	const char *m_cputag;

	device_t*   m_maincpu;
};


// device type definition
extern const device_type MPC105;

UINT32 mpc105_pci_read(device_t *busdevice, device_t *device, int function, int offset, UINT32 mem_mask);
void mpc105_pci_write(device_t *busdevice, device_t *device, int function, int offset, UINT32 data, UINT32 mem_mask);

#endif /* MPC105_H */
