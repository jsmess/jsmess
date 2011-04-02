#pragma once

#ifndef _PSXCARD_
#define _PSXCARD_

#include "emu.h"

#define MCFG_PSXCARD_ADD(_tag) \
    MCFG_DEVICE_ADD(_tag, PSXCARD, 0)

struct psxcard_interface
{
};

class psxcard_device_config :   public device_config,
                                public psxcard_interface
{
    friend class psxcard_device;

    // construction/destruction
    psxcard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;
};

class psxcard_device : public device_t
{
	friend class psxcard_device_config;

    psxcard_device(running_machine &_machine, const psxcard_device_config &_config);

	unsigned char pkt[0x8b], pkt_ptr, pkt_sz, cmd, *cache;
	unsigned short addr;
	int state;

	void read_card(const unsigned short addr, unsigned char *buf);
	void write_card(const unsigned short addr, unsigned char *buf);
	unsigned char checksum_data(const unsigned char *buf, const unsigned int sz);

public:
	virtual void device_start();
	virtual void device_reset();

	bool transfer(UINT8 to, UINT8 *from);
	void mess_io(running_machine *machine, UINT8 n_data);
private:
    const psxcard_device_config &m_config;
};

// device type definition
extern const device_type PSXCARD;

#endif
