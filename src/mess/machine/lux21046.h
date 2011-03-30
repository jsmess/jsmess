#pragma once

#ifndef __LUXOR_55_21046__
#define __LUXOR_55_21046__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"
#include "imagedev/flopimg.h"
#include "machine/abcbus.h"
#include "machine/devhelpr.h"
#include "machine/wd17xx.h"
#include "machine/z80dma.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LUXOR_55_21046_TAG	"luxor_55_21046"


#define ADDRESS_ABC832			44
#define ADDRESS_ABC830			45
#define ADDRESS_ABC838			46


#define DRIVE_TEAC_FD55F		0x01
#define DRIVE_BASF_6138			0x02
#define DRIVE_MICROPOLIS_1015F	0x03
#define DRIVE_BASF_6118			0x04
#define DRIVE_MICROPOLIS_1115F	0x05
#define DRIVE_BASF_6106_08		0x08
#define DRIVE_MPI_51			0x09
#define DRIVE_BASF_6105			0x0e
#define DRIVE_BASF_6106			0x0f



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_LUXOR_55_21046_ADD(_config) \
    MCFG_DEVICE_ADD(LUXOR_55_21046_TAG, LUXOR_55_21046, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define LUXOR_55_21046_INTERFACE(name) \
	const luxor_55_21046_interface (name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> luxor_55_21046_interface

struct luxor_55_21046_interface
{
	UINT8 m_sw1;				// single/double sided/density
	UINT8 m_sw2;				// drive type
	UINT8 m_sw3;				// ABC bus address
};


// ======================> luxor_55_21046_device_config

class luxor_55_21046_device_config :   public device_config,
									   public device_config_abcbus_interface,
									   public luxor_55_21046_interface
{
    friend class luxor_55_21046_device;

    // construction/destruction
    luxor_55_21046_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const input_port_token *device_input_ports() const;

protected:
    // device_config overrides
    virtual void device_config_complete();
	
private:
	UINT8 m_sw1;				// DS/DD
	UINT8 m_sw2;				// drive type
	UINT8 m_sw3;				// ABC bus address
};


// ======================> luxor_55_21046_device

class luxor_55_21046_device :  public device_t,
							   public device_abcbus_interface 
{
    friend class luxor_55_21046_device_config;

    // construction/destruction
    luxor_55_21046_device(running_machine &_machine, const luxor_55_21046_device_config &_config);

public:
	DECLARE_READ8_MEMBER( _3d_r );
	DECLARE_WRITE8_MEMBER( _4d_w );
	DECLARE_WRITE8_MEMBER( _4b_w );
	DECLARE_WRITE8_MEMBER( _9b_w );
	DECLARE_WRITE8_MEMBER( _8a_w );
	DECLARE_READ8_MEMBER( _9a_r );

	DECLARE_WRITE_LINE_MEMBER( dma_int_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_abcbus_interface overrides
	virtual void abcbus_cs(UINT8 data);
	virtual void abcbus_rst(int state);
	virtual UINT8 abcbus_inp();
	virtual void abcbus_utp(UINT8 data);
	virtual UINT8 abcbus_stat();
	virtual void abcbus_c1(UINT8 data);
	virtual void abcbus_c3(UINT8 data);

private:
	required_device<cpu_device> m_maincpu;
	required_device<z80dma_device> m_dma;
	required_device<device_t> m_fdc;
	device_t *m_image0;
	device_t *m_image1;

	bool m_cs;					// card selected
	UINT8 m_status;				// ABC BUS status
	UINT8 m_data_in;			// ABC BUS data in
	UINT8 m_data_out;			// ABC BUS data out
	int m_fdc_irq;				// FDC interrupt
	int m_dma_irq;				// DMA interrupt
	int m_busy;					// busy bit
	int m_force_busy;			// force busy bit

    const luxor_55_21046_device_config &m_config;
};


// device type definition
extern const device_type LUXOR_55_21046;



#endif
