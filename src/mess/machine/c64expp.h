/**********************************************************************

    Commodore 64 expansion port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

	1	GND		Ground				A	GND		Ground
	2	+5V		+5 Volts DC			B	/ROMH	ROM High
	3	+5V		+5 Volts DC			C	/RESET	Reset
	4	/IRQ	Interrupt Request	D	/NMI	Non Maskable Interrupt
	5	/CR/W						E	S02	
	6	DOTCLK	Dot Clock			F	CA15	Cartridge Address 15
	7	I/O 1						H	CA14	Cartridge Address 14
	8	/GAME	Game				J	CA13	Cartridge Address 13
	9	/EXROM						K	CA12	Cartridge Address 12
	10	I/O 2						L	CA11	Cartridge Address 11
	11	/ROML	ROM Low				M	CA10	Cartridge Address 10
	12	BA							N	CA9		Cartridge Address 9
	13	/DMA						P	CA8		Cartridge Address 8
	14	CD7		Cartridge Data 7	R	CA7		Cartridge Address 7
	15	CD6		Cartridge Data 6	S	CA6		Cartridge Address 6
	16	CD5		Cartridge Data 5	T	CA5		Cartridge Address 5
	17	CD4		Cartridge Data 4	U	CA4		Cartridge Address 4
	18	CD3		Cartridge Data 3	V	CA3		Cartridge Address 3
	19	CD2		Cartridge Data 2	W	CA2		Cartridge Address 2
	20	CD1		Cartridge Data 1	X	CA1		Cartridge Address 1
	21	CD0		Cartridge Data 0	Y	CA0		Cartridge Address 0
	22	GND		Ground				Z	GND		Ground

**********************************************************************/

#pragma once

#ifndef __C64_EXPANSION_PORT__
#define __C64_EXPANSION_PORT__

#include "emu.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C64_EXPANSION_PORT_TAG	"c64expport"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C64_EXPANSION_PORT_ADD(_config) \
	MCFG_DEVICE_ADD(C64_EXPANSION_PORT_TAG, C64_EXPANSION_PORT, 0) \
	MCFG_DEVICE_CONFIG(_config)

#define C64_EXPANSION_PORT_INTERFACE(name) \
	const c64_expansion_port_interface (name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_expansion_port_interface

struct c64_expansion_port_interface
{
	devcb_write_line	m_out_game_cb;
	devcb_write_line	m_out_exrom_cb;
	devcb_write_line	m_out_irq_cb;
	devcb_write_line	m_out_dma_cb;
	devcb_write_line	m_out_nmi_cb;
	devcb_write_line	m_out_reset_cb;
};


// ======================> device_c64_expansion_port_interface

class device_c64_expansion_port_interface : public device_interface
{
public:
	// construction/destruction
	device_c64_expansion_port_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_c64_expansion_port_interface();

	// required operation overrides
	virtual int c64_game_r() = 0;
	virtual int c64_exrom_r() = 0;

	// optional operation overrides
	virtual void c64_reset_w(int state) { };
	virtual UINT8 c64_io1_r(offs_t offset) { return 0; };
	virtual void c64_io1_w(offs_t offset, UINT8 data) { };
	virtual UINT8 c64_io2_r(offs_t offset) { return 0; };
	virtual void c64_io2_w(offs_t offset, UINT8 data) { };
	virtual UINT8 c64_roml_r(offs_t offset) { return 0; };
	virtual void c64_roml_w(offs_t offset, UINT8 data) { };
	virtual UINT8 c64_romh_r(offs_t offset) { return 0; };
	virtual void c64_romh_w(offs_t offset, UINT8 data) { };
};


// ======================> c64_expansion_port_device

class c64_expansion_port_device :  public device_t,
								   public c64_expansion_port_interface
{
public:
    // construction/destruction
    c64_expansion_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	void insert_cartridge(device_t *device);
	void remove_cartridge();

	// I/O 1 ($DE00-$DEFF)
	DECLARE_READ8_MEMBER( io1_r );
	DECLARE_WRITE8_MEMBER( io1_w );

	// I/O 2 ($DF00-$DFFF)
	DECLARE_READ8_MEMBER( io2_r );
	DECLARE_WRITE8_MEMBER( io2_w );

	// ROML ($8000-$9FFF)
	DECLARE_READ8_MEMBER( roml_r );
	DECLARE_WRITE8_MEMBER( roml_w );

	// ROMH ($A000-$BFFF or $E000-$FFFF)
	DECLARE_READ8_MEMBER( romh_r );
	DECLARE_WRITE8_MEMBER( romh_w );

	// GAME
	DECLARE_READ_LINE_MEMBER( game_r );
	DECLARE_WRITE_LINE_MEMBER( game_w );

	// EXROM
	DECLARE_READ_LINE_MEMBER( exrom_r );
	DECLARE_WRITE_LINE_MEMBER( exrom_w );

	// interrupts
	DECLARE_WRITE_LINE_MEMBER( irq_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );

	// DMA
	DECLARE_WRITE_LINE_MEMBER( dma_w );

	// reset
	DECLARE_WRITE_LINE_MEMBER( reset_w );

protected:
    // device-level overrides
    virtual void device_start();
	
	devcb_resolved_write_line	m_out_game_func;
	devcb_resolved_write_line	m_out_exrom_func;
	devcb_resolved_write_line	m_out_irq_func;
	devcb_resolved_write_line	m_out_dma_func;
	devcb_resolved_write_line	m_out_nmi_func;
	devcb_resolved_write_line	m_out_reset_func;

	device_t *								m_device;		// connected cartridge device
	device_c64_expansion_port_interface *	m_interface;	// connected cartridge device's interface
};


// device type definition
extern const device_type C64_EXPANSION_PORT;


#endif
