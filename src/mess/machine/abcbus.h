/**********************************************************************

    Luxor ABC-bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

                              a     b
                -12 V   <--   *  1  *   --> -12V
                0 V     ---   *  2  *   --- 0 V
                RESIN_  -->   *  3  *   --> XMEMWR_
                0 V     ---   *  4  *   --> XMEMFL_
                INT_    -->   *  5  *   --> phi
                D7      <->   *  6  *   --- 0 V
                D6      <->   *  7  *   --- 0 V
                D5      <->   *  8  *   --- 0 V
                D4      <->   *  9  *   --- 0 V
                D3      <->   * 10  *   --- 0 V
                D2      <->   * 11  *   --- 0 V
                D1      <->   * 12  *   --- 0 V
                D0      <->   * 13  *   --- 0 V
                              * 14  *   --> A15
                RST_    <--   * 15  *   --> A14
                IN1     <--   * 16  *   --> A13
                IN0     <--   * 17  *   --> A12
                OUT5    <--   * 18  *   --> A11
                OUT4    <--   * 19  *   --> A10
                OUT3    <--   * 20  *   --> A9
                OUT2    <--   * 21  *   --> A8
                OUT0    <--   * 22  *   --> A7
                OUT1    <--   * 23  *   --> A6
                NMI_    -->   * 24  *   --> A5
                INP2_   <--   * 25  *   --> A4
               XINPSTB_ <--   * 26  *   --> A3
              XOUTPSTB_ <--   * 27  *   --> A2
                XM_     -->   * 28  *   --> A1
                RFSH_   <--   * 29  *   --> A0
                RDY     -->   * 30  *   --> MEMRQ_
                +5 V    <--   * 31  *   --> +5 V
                +12 V   <--   * 32  *   --> +12 V

**********************************************************************/

/*

    OUT 0   _OUT    data output
    OUT 1   _CS     card select
    OUT 2   _C1     command 1
    OUT 3   _C2     command 2
    OUT 4   _C3     command 3
    OUT 5   _C4     command 4

    IN 0    _INP    data input
    IN 1    _STAT   status in
    IN 7    RST     reset

*/

#pragma once

#ifndef __ABCBUS__
#define __ABCBUS__

#include "emu.h"
#include "machine/devhelpr.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ABCBUS_ADD(_tag, _config) \
	MCFG_DEVICE_ADD(_tag, ABCBUS, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define ABCBUS_DAISY(_name) \
	const abcbus_config (_name)[] =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> abcbus_config

struct abcbus_config
{
	const char *	devname;					// name of the device
};


// ======================> device_abcbus_interface

class device_abcbus_interface : public device_interface
{
public:
	// construction/destruction
	device_abcbus_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_abcbus_interface();

	// required operation overrides
	virtual void abcbus_cs(UINT8 data) = 0;

	// optional operation overrides
	virtual void abcbus_rst(int state) { };
	virtual UINT8 abcbus_inp() { return 0xff; };
	virtual void abcbus_utp(UINT8 data) { };
	virtual UINT8 abcbus_stat() { return 0xff; };
	virtual void abcbus_c1(UINT8 data) { };
	virtual void abcbus_c2(UINT8 data) { };
	virtual void abcbus_c3(UINT8 data) { };
	virtual void abcbus_c4(UINT8 data) { };
};

// ======================> abcbus_device

class abcbus_device :  public device_t,
					   public abcbus_config
{
public:
    // construction/destruction
    abcbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER( cs_w );
	DECLARE_READ8_MEMBER( rst_r );
	DECLARE_READ8_MEMBER( inp_r );
	DECLARE_WRITE8_MEMBER( utp_w );
	DECLARE_READ8_MEMBER( stat_r );
	DECLARE_WRITE8_MEMBER( c1_w );
	DECLARE_WRITE8_MEMBER( c2_w );
	DECLARE_WRITE8_MEMBER( c3_w );
	DECLARE_WRITE8_MEMBER( c4_w );

	DECLARE_WRITE_LINE_MEMBER( resin_w );
	DECLARE_WRITE_LINE_MEMBER( int_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );
	DECLARE_WRITE_LINE_MEMBER( rdy_w );

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_config_complete();

	class daisy_entry
	{
	public:
		daisy_entry(device_t *device);

		daisy_entry *				m_next;			// next device
		device_t *					m_device;		// associated device
		device_abcbus_interface *	m_interface;	// associated device's daisy interface
	};

	daisy_entry *			m_daisy_list;	// head of the daisy chain

private:
    const abcbus_config *m_daisy;
};


// device type definition
extern const device_type ABCBUS;


#endif
