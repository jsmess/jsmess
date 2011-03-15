#pragma once

#ifndef __LUXOR_55_10828__
#define __LUXOR_55_10828__

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
#include "machine/z80pio.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LUXOR_55_10828_TAG	"luxor_55_10828"


#define ADDRESS_ABC830			45
#define ADDRESS_ABC832			44
#define ADDRESS_ABC834			44
#define ADDRESS_ABC850			44


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

#define MCFG_LUXOR_55_10828_ADD(_config) \
    MCFG_DEVICE_ADD(LUXOR_55_10828_TAG, LUXOR_55_10828, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define LUXOR_55_10828_INTERFACE(name) \
	const luxor_55_10828_interface (name) =


#define LUXOR_55_10828_ABCBUS(_tag) \
	_tag, DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, cs_w), DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, stat_r), \
	DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, inp_r), DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, utp_w), DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, c1_w), \
	DEVCB_NULL, DEVCB_DEVICE_MEMBER(_tag, luxor_55_10828_device, c3_w), DEVCB_NULL, DEVCB_DEVICE_LINE_MEMBER(_tag, luxor_55_10828_device, rst_w)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> luxor_55_10828_interface

struct luxor_55_10828_interface
{
	UINT8 m_sw1;				// single/double sided/density
	UINT8 m_drive_type;			// drive type
	UINT8 m_s1;					// ABC bus address
};


// ======================> luxor_55_10828_device_config

class luxor_55_10828_device_config :   public device_config,
									   public luxor_55_10828_interface
{
    friend class luxor_55_10828_device;

    // construction/destruction
    luxor_55_10828_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// optional information overrides
	virtual const rom_entry *rom_region() const;
	virtual machine_config_constructor machine_config_additions() const;
	virtual const input_port_token *input_ports() const;

protected:
    // device_config overrides
    virtual void device_config_complete();
	
private:
	UINT8 m_sw1;				// single/double sided/density
	UINT8 m_drive_type;			// drive type
	UINT8 m_s1;					// ABC bus address
};


// ======================> luxor_55_10828_device

class luxor_55_10828_device :  public device_t
{
    friend class luxor_55_10828_device_config;

    // construction/destruction
    luxor_55_10828_device(running_machine &_machine, const luxor_55_10828_device_config &_config);

public:
	DECLARE_WRITE8_MEMBER( cs_w );
	DECLARE_READ8_MEMBER( stat_r );
	DECLARE_READ8_MEMBER( inp_r );
	DECLARE_WRITE8_MEMBER( utp_w );
	DECLARE_WRITE8_MEMBER( c1_w );
	DECLARE_WRITE8_MEMBER( c3_w );
	DECLARE_WRITE_LINE_MEMBER( rst_w );

	DECLARE_WRITE8_MEMBER( ctrl_w );
	DECLARE_WRITE8_MEMBER( status_w );
	DECLARE_READ8_MEMBER( fdc_r );
	DECLARE_WRITE8_MEMBER( fdc_w );

	DECLARE_READ8_MEMBER( pio_pa_r );
	DECLARE_WRITE8_MEMBER( pio_pa_w );
	DECLARE_READ8_MEMBER( pio_pb_r );
	DECLARE_WRITE8_MEMBER( pio_pb_w );

	DECLARE_WRITE_LINE_MEMBER( fdc_intrq_w );
	DECLARE_WRITE_LINE_MEMBER( fdc_drq_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

private:
	required_device<cpu_device> m_maincpu;
	required_device<z80pio_device> m_pio;
	required_device<device_t> m_fdc;
	device_t *m_image0;
	device_t *m_image1;

	bool m_cs;				// card selected
	UINT8 m_status;			// ABC BUS status
	UINT8 m_data;			// ABC BUS data
	int m_fdc_irq;			// floppy interrupt
	int m_fdc_drq;			// floppy data request
	int m_wait_enable;		// wait enable
	int m_sel0;				// drive select 0
	int m_sel1;				// drive select 1

    const luxor_55_10828_device_config &m_config;
};


// device type definition
extern const device_type LUXOR_55_10828;



#endif
