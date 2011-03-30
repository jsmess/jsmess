/***************************************************************************

        Psion Organiser II series

****************************************************************************/

#pragma once

#ifndef _PSION_H_
#define _PSION_H_

class datapack
{
public:
	UINT8 update(UINT8 data);
	void control_lines_w(UINT8 data);
	UINT8 control_lines_r();
	int load(device_image_interface &image);
	void unload(device_image_interface &image);
	void reset();

private:
	device_image_interface *d_image;
	UINT8 id;
	UINT8 len;
	UINT16 counter;
	UINT8 page;
	UINT8 segment;
	UINT8 control_lines;
	UINT8 *buffer;
	bool write_flag;
};

class psion_state : public driver_device
{
public:
	psion_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_lcdc(*this, "hd44780"),
		  m_beep(*this, "beep")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<hd44780_device> m_lcdc;
	required_device<device_t> m_beep;

	UINT16 m_kb_counter;
	UINT8 m_enable_nmi;
	UINT8 *m_sys_register;
	UINT8 m_tcsr_value;
	UINT8 m_stby_pwr;
	UINT8 m_pulse;

	UINT8 m_port2_ddr;	// datapack i/o ddr
	UINT8 m_port2;		// datapack i/o data bus
	UINT8 m_port6_ddr;	// datapack control lines ddr
	UINT8 m_port6;		// datapack control lines

	// RAM/ROM banks
	UINT8 *m_ram;
	size_t m_ram_size;
	UINT8 *m_paged_ram;
	UINT8 m_rom_bank;
	UINT8 m_ram_bank;
	UINT8 m_ram_bank_count;
	UINT8 m_rom_bank_count;

	datapack m_pack1;
	datapack m_pack2;

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	UINT8 kb_read(running_machine &machine);
	void update_banks(running_machine &machine);
	datapack *get_active_slot(UINT8 data);
	DECLARE_WRITE8_MEMBER( hd63701_int_reg_w );
	DECLARE_READ8_MEMBER( hd63701_int_reg_r );
	void io_rw(address_space &space, UINT16 offset);
	DECLARE_WRITE8_MEMBER( io_w );
	DECLARE_READ8_MEMBER( io_r );
};

#endif	// _PSION_H_
