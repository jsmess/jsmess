/***************************************************************************

    GROM emulation
    See grom.c for documentation,

    Michael Zapf

    February 2012: Rewritten as class

***************************************************************************/
#ifndef __TI99GROM__
#define __TI99GROM__

#include "ti99defs.h"

typedef struct _ti99grom_config
{
	bool				m_writable;
	int 				m_ident;
	const char			*m_regionname;
	offs_t				m_offset_reg;
	// If the GROM has only 6 KiB, the remaining 2 KiB are filled with a
	// specific byte pattern which is created by a logical OR of lower
	// regions
	int					m_size;
	bool				m_rollover;	// some GRAM simulations do not implement rollover
	devcb_write_line	m_ready;
} ti99grom_config;

#define GROM_CONFIG(name) \
	const ti99grom_config(name) =


extern const device_type GROM;

/*
    ti99_grom. For bus8z_device see ti99defs.h
*/
class ti99_grom_device : public bus8z_device, ti99grom_config
{
public:
	ti99_grom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);

private:
	/* Address pointer. */
	// This value is always expected to be in the range 0x0000 - 0xffff, even
	// when this GROM is not addressed.
	UINT16 m_address;

	/* GROM data buffer. */
	UINT8 m_buffer;

	/* Internal flip-flop. Used when retrieving the address counter. */
	bool m_raddr_LSB;

	/* Internal flip-flops. Used when writing the address counter.*/
	bool m_waddr_LSB;

	/* Pointer to the memory region contained in this GROM. */
	UINT8 *m_memptr;

	/* Ready callback. This line is usually connected to the READY pin of the CPU. */
	devcb_resolved_write_line m_gromready;

	/* Indicates whether this device will react on the next read/write data access. */
	int is_selected()
	{
		return (((m_address >> 13)&0x07)==m_ident);
	}

	void device_start(void);
	void device_reset(void);
	void device_config_complete(void);
};


#define MCFG_GROM_ADD(_tag, _config)	\
	MCFG_DEVICE_ADD(_tag, GROM, 0)	\
	MCFG_DEVICE_CONFIG(_config)

#endif
