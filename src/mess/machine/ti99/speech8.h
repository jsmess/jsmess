/****************************************************************************

    TI-99/8 Speech Synthesizer
    See speech8.c for documentation

    Michael Zapf
    October 2010
    February 2012: Rewritten as class

*****************************************************************************/

#ifndef __TISPEECH8__
#define __TISPEECH8__

#include "emu.h"
#include "ti99defs.h"
#include "sound/tms5220.h"

extern const device_type TI99_SPEECH8;

class ti998_spsyn_device : public bus8z_device
{
public:
	ti998_spsyn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	DECLARE_READ8Z_MEMBER(readz);
	DECLARE_WRITE8_MEMBER(write);

	void crureadz(offs_t offset, UINT8 *value) { };
	void cruwrite(offs_t offset, UINT8 value) { };

	DECLARE_WRITE_LINE_MEMBER( speech8_ready );

	UINT8			*m_speechrom;			// pointer to speech ROM data
	int 			m_load_pointer;			// which 4-bit nibble will be affected by load address
	int 			m_rombits_count;		// current bit position in ROM
	UINT32			m_sprom_address;		// 18 bit pointer in ROM
	UINT32			m_sprom_length;			// length of data pointed by speechrom_data, from 0 to 2^18

	int				m_select_mask;
	int				m_select_value;

protected:
	void			device_start();
	void			device_reset(void);
	const rom_entry *device_rom_region() const;
	machine_config_constructor device_mconfig_additions() const;
	void			device_config_complete();

private:
	device_t		*m_vsp;
};

#define MCFG_TISPEECH8_ADD(_tag)		\
	MCFG_DEVICE_ADD(_tag, TI99_SPEECH8, 0)
#endif
