#pragma once

#ifndef __ABC1600__
#define __ABC1600__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "imagedev/flopdrv.h"
#include "machine/ram.h"
#include "machine/8530scc.h"
#include "machine/abc99.h"
#include "machine/e0516.h"
#include "machine/nmc9306.h"
#include "machine/s1410.h"
#include "machine/wd17xx.h"
#include "machine/z80dart.h"
#include "machine/z80dma.h"
#include "machine/z8536.h"
#include "video/mc6845.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define MC68008P8_TAG		"3f"
#define Z8410AB1_0_TAG		"5g"
#define Z8410AB1_1_TAG		"7g"
#define Z8410AB1_2_TAG		"8g"
#define Z8470AB1_TAG		"17b"
#define Z8530B1_TAG			"2a"
#define Z8536B1_TAG			"15b"
#define SAB1797_02P_TAG		"5a"
#define FDC9229BT_TAG		"7a"
#define E050_C16PC_TAG		"13b"
#define NMC9306_TAG			"14c"
#define SY6845E_TAG			"sy6845e"
#define SCREEN_TAG			"screen"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> abc1600_state
class abc1600_state : public driver_device
{
public:
	abc1600_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, MC68008P8_TAG),
		  m_dma0(*this, Z8410AB1_0_TAG),
		  m_dma1(*this, Z8410AB1_1_TAG),
		  m_dma2(*this, Z8410AB1_2_TAG),
		  m_dart(*this, Z8470AB1_TAG),
		  m_scc(*this, Z8530B1_TAG),
		  m_cio(*this, Z8536B1_TAG),
		  m_fdc(*this, SAB1797_02P_TAG),
		  m_rtc(*this, E050_C16PC_TAG),
		  m_nvram(*this, NMC9306_TAG),
		  m_crtc(*this, SY6845E_TAG),
		  m_ram(*this, RAM_TAG),
		  m_floppy(*this, FLOPPY_0)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<z80dma_device> m_dma0;
	required_device<z80dma_device> m_dma1;
	required_device<z80dma_device> m_dma2;
	required_device<z80dart_device> m_dart;
	required_device<device_t> m_scc;
	required_device<z8536_device> m_cio;
	required_device<device_t> m_fdc;
	required_device<device_t> m_rtc;
	required_device<nmc9306_device> m_nvram;
	required_device<device_t> m_crtc;
	required_device<device_t> m_ram;
	required_device<device_t> m_floppy;

	virtual void machine_start();
	virtual void machine_reset();

	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	
	DECLARE_READ8_MEMBER( mac_r );
	DECLARE_WRITE8_MEMBER( mac_w );
	DECLARE_READ8_MEMBER( cause_r );
	DECLARE_WRITE8_MEMBER( task_w );
	DECLARE_READ8_MEMBER( segment_r );
	DECLARE_WRITE8_MEMBER( segment_w );
	DECLARE_READ8_MEMBER( page_r );
	DECLARE_WRITE8_MEMBER( page_w );
	DECLARE_WRITE8_MEMBER( fw0_w );
	DECLARE_WRITE8_MEMBER( fw1_w );
	DECLARE_WRITE8_MEMBER( en_spec_contr_reg_w );

	DECLARE_WRITE8_MEMBER( dmamap_w );
	DECLARE_WRITE_LINE_MEMBER( dbrq_w );
	DECLARE_WRITE_LINE_MEMBER( drq_w );
	DECLARE_READ8_MEMBER( dma0_mreq_r );
	DECLARE_WRITE8_MEMBER( dma0_mreq_w );
	DECLARE_READ8_MEMBER( dma0_iorq_r );
	DECLARE_WRITE8_MEMBER( dma0_iorq_w );
	DECLARE_READ8_MEMBER( dma1_mreq_r );
	DECLARE_WRITE8_MEMBER( dma1_mreq_w );
	DECLARE_READ8_MEMBER( dma1_iorq_r );
	DECLARE_WRITE8_MEMBER( dma1_iorq_w );
	DECLARE_READ8_MEMBER( dma2_mreq_r );
	DECLARE_WRITE8_MEMBER( dma2_mreq_w );
	DECLARE_READ8_MEMBER( dma2_iorq_r );
	DECLARE_WRITE8_MEMBER( dma2_iorq_w );

	DECLARE_READ8_MEMBER( video_ram_r );
	DECLARE_WRITE8_MEMBER( video_ram_w );
	DECLARE_READ8_MEMBER( iord0_r );
	DECLARE_WRITE8_MEMBER( iowr0_w );
	DECLARE_WRITE8_MEMBER( iowr1_w );
	DECLARE_WRITE8_MEMBER( iowr2_w );

	DECLARE_READ8_MEMBER( cio_pa_r );
	DECLARE_READ8_MEMBER( cio_pb_r );
	DECLARE_WRITE8_MEMBER( cio_pb_w );
	DECLARE_READ8_MEMBER( cio_pc_r );
	DECLARE_WRITE8_MEMBER( cio_pc_w );

	DECLARE_WRITE_LINE_MEMBER( vs_w );

	UINT8 read_ram(offs_t offset);
	void write_ram(offs_t offset, UINT8 data);
	UINT8 read_io(offs_t offset);
	void write_io(offs_t offset, UINT8 data);
	UINT8 read_user_memory(offs_t offset);
	void write_user_memory(offs_t offset, UINT8 data);
	UINT8 read_supervisor_memory(offs_t offset);
	void write_supervisor_memory(offs_t offset, UINT8 data);
	inline offs_t get_dma_address(int index, UINT16 offset);
	inline UINT8 dma_mreq_r(int index, UINT16 offset);
	inline void dma_mreq_w(int index, UINT16 offset, UINT8 data);
	inline UINT8 dma_iorq_r(int index, UINT16 offset);
	inline void dma_iorq_w(int index, UINT16 offset, UINT8 data);

	inline UINT16 get_crtca(UINT16 ma, UINT8 ra, UINT8 column);
	void crtc_update_row(device_t *device, bitmap_t *bitmap, const rectangle *cliprect, UINT16 ma, UINT8 ra, UINT16 y, UINT8 x_count, INT8 cursor_x, void *param);

	// memory access controller
	UINT8 m_task;
	UINT8 m_segment_ram[0x400];
	UINT16 m_page_ram[0x400];
	
	// DMA
	UINT8 m_dmamap[8];
	int m_dmadis;
	int m_sysscc;
	int m_sysfs;
	UINT8 m_cause;

	// video
	UINT8 *m_video_ram;			// video RAM
	int m_endisp;				// enable display
	int m_clocks_disabled;		// clocks disabled
	int m_vs;					// vertical sync
	UINT16 m_gmdi;				// video RAM data latch
	UINT16 m_wrm;				// write mask latch
	UINT8 m_ms[16];				// 
	UINT8 m_ds[16];				// 
	UINT8 m_flag;				// flags
	UINT16 m_sx;				// X size
	UINT16 m_sy;				// Y size
	UINT16 m_tx;				// X destination
	UINT16 m_ty;				// Y destination
	UINT16 m_fx;				// X source
	UINT16 m_fy;				// Y source
};



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

/*----------- defined in video/abc1600.c -----------*/

MACHINE_CONFIG_EXTERN( abc1600_video );



#endif
