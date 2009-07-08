/***************************************************************************

    Visual 1050

    12/05/2009 Skeleton driver.

****************************************************************************/

/*

	TODO:

	- video
	- µPB8214 chip
	- RTC58321 chip
	- floppy
	- keyboard
	- centronics
	- winchester

*/

#include "driver.h"
#include "includes/v1050.h"
#include "cpu/z80/z80.h"
#include "cpu/m6502/m6502.h"
#include "cpu/mcs48/mcs48.h"
#include "devices/basicdsk.h"
#include "machine/ctronics.h"
#include "machine/8255ppi.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "video/mc6845.h"

INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return image_from_devtype_and_index(machine, IO_FLOPPY, drive);
}

static void v1050_bankswitch(running_machine *machine)
{
	v1050_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	int bank = (state->bank >> 1) & 0x03;

	if (BIT(state->bank, 0))
	{
		memory_install_readwrite8_handler(program, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_set_bank(machine, 1, bank);
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		memory_set_bank(machine, 1, 3);
	}

	memory_set_bank(machine, 2, bank);

	if (bank == 2)
	{
		memory_install_readwrite8_handler(program, 0x4000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
	else
	{
		memory_install_readwrite8_handler(program, 0x4000, 0x7fff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bank(machine, 3, bank);
		memory_set_bank(machine, 4, bank);
	}

	memory_set_bank(machine, 5, bank);
}

/* Read/Write Handlers */

static WRITE8_HANDLER( dvint_clr_w )
{
	cputag_set_input_line(space->machine, M6502_TAG, INPUT_LINE_IRQ0, CLEAR_LINE);
}

static WRITE8_HANDLER( bank_w )
{
	v1050_state *state = space->machine->driver_data;

	state->bank = data;

	v1050_bankswitch(space->machine);
}

/* Memory Maps */

static ADDRESS_MAP_START( v1050_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK(1)
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK(2)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK(3)
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK(4)
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK(5)
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x84, 0x87) AM_DEVREADWRITE(CRT_Z80_PPI8255_TAG, ppi8255_r, ppi8255_w)
//	AM_RANGE(0x88, 0x88) AM_DEVREADWRITE(KB_MSM8251_TAG, msm8251_data_r, msm8251_data_w)
//	AM_RANGE(0x89, 0x89) AM_DEVREADWRITE(KB_MSM8251_TAG, msm8251_status_r, msm8251_control_w)
//	AM_RANGE(0x8c, 0x8c) AM_DEVREADWRITE(SIO_MSM8251_TAG, msm8251_data_r, msm8251_data_w)
//	AM_RANGE(0x8d, 0x8d) AM_DEVREADWRITE(SIO_MSM8251_TAG, msm8251_status_r, msm8251_control_w)
//	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE(FDC_PPI8255_TAG, ppi8255_r, ppi8255_w)
	AM_RANGE(0x94, 0x97) AM_DEVREADWRITE(MB8877_TAG, wd17xx_r, wd17xx_w)
//	AM_RANGE(0x9c, 0x9f) AM_DEVREADWRITE(RTC_PPI8255_TAG, ppi8255_r, ppi8255_w)
//	AM_RANGE(0xa0, 0xa0) AM_READWRITE(vint_clr_r, vint_clr_w)
//	AM_RANGE(0xb0, 0xb0) AM_READWRITE(dint_clr_r, dint_clr_w)
//	AM_RANGE(0xc0, 0xc0) AM_DEVREADWRITE(UPB8214_TAG, pic8214_r, pic8214_w) 
	AM_RANGE(0xd0, 0xd0) AM_WRITE(bank_w)
//	AM_RANGE(0xe0, 0xe1) AM_READWRITE(sasi_r, sasi_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_crt_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x7fff) AM_RAM AM_READWRITE(v1050_videoram_r, v1050_videoram_w) AM_BASE_MEMBER(v1050_state, video_ram)
	AM_RANGE(0x8000, 0x8000) AM_DEVWRITE(HD6845_TAG, mc6845_address_w)
	AM_RANGE(0x8001, 0x8001) AM_DEVREADWRITE(HD6845_TAG, mc6845_register_r, mc6845_register_w)
//	AM_RANGE(0x9000, 0x9003) AM_DEVREADWRITE(CRT_M6502_PPI8255_TAG, ppi8255_r, ppi8255_w)
	AM_RANGE(0xa000, 0xa000) AM_READWRITE(v1050_attr_r, v1050_attr_w)
//	AM_RANGE(0xb000, 0xb000) AM_WRITE(z80int_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(dvint_clr_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_kbd_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) 
//	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) 
//	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) 
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( v1050 )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("XA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("XB")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
INPUT_PORTS_END

/* Miscellanous 8255A Interface */

static WRITE8_DEVICE_HANDLER( misc_ppi8255_a_w )
{
	/*

        bit		signal		description

		PA0     f_ds<0>		drive 0 select
        PA1     f_ds<1>		drive 1 select
        PA2     f_ds<2>		drive 2 select
        PA3     f_ds<3>		drive 3 select
        PA4		f_side_1	floppy side select
        PA5     f_pre_comp	precompensation
        PA6		f_motor_on*	floppy motor
        PA7     f_dden*		double density select
    
	*/

	v1050_state *state = device->machine->driver_data;

	int f_motor_on = !BIT(data, 6);

	/* floppy drive select */
	if (BIT(data, 0)) wd17xx_set_drive(state->mb8877, 0);
	if (BIT(data, 1)) wd17xx_set_drive(state->mb8877, 1);
	if (BIT(data, 2)) wd17xx_set_drive(state->mb8877, 2);
	if (BIT(data, 3)) wd17xx_set_drive(state->mb8877, 3);

	/* floppy side select */
	wd17xx_set_side(state->mb8877, BIT(data, 4));

	/* floppy motor */
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 0), f_motor_on);
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 1), f_motor_on);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 0), f_motor_on, 1);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 1), f_motor_on, 1);

	/* density select */
	wd17xx_set_density(state->mb8877, BIT(data, 7) ? DEN_FM_LO : DEN_FM_HI);
}

static READ8_DEVICE_HANDLER( misc_ppi8255_c_r )
{
	/*

        bit		signal		description

		PC0     pr_strobe	printer strobe
        PC1     f_int_enb	floppy interrupt enable
        PC2     baud_sel_a
        PC3     baud_sel_b
        PC4		pr_busy*	printer busy
        PC5     pr_pe*		printer paper end
        PC6		
        PC7     
    
	*/

	v1050_state *state = device->machine->driver_data;

	UINT8 data = 0;

	data |= centronics_busy_r(state->centronics) << 4;
	data |= centronics_pe_r(state->centronics) << 5;

	return data;
}

static WRITE8_DEVICE_HANDLER( misc_ppi8255_c_w )
{
	/*

        bit		signal		description

		PC0     pr_strobe
        PC1     f_int_enb
        PC2     baud_sel_a
        PC3     baud_sel_b
        PC4		pr_busy*
        PC5     pr_pe*
        PC6		
        PC7     
    
	*/

	v1050_state *state = device->machine->driver_data;

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 0));

	/* floppy interrupt enable */
	state->f_int_enb = BIT(data, 1);
}

static const ppi8255_interface misc_ppi8255_intf =
{
	DEVCB_NULL,							// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_HANDLER(misc_ppi8255_c_r),	// Port C read
	DEVCB_HANDLER(misc_ppi8255_a_w),	// Port A write
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),// Port B write
	DEVCB_HANDLER(misc_ppi8255_c_w)		// Port C write
};

/* MB8877 Interface */

static WD17XX_CALLBACK( v1050_mb8877_callback )
{
//	v1050_state *driver_state = device->machine->driver_data;

	switch (state)
	{
	case WD17XX_IRQ_CLR:
//		if (driver_state->f_int_enb) upb8214_r2_w(driver_state->upb8214, 0);
		break;

	case WD17XX_IRQ_SET:
//		if (driver_state->f_int_enb) upb8214_r2_w(driver_state->upb8214, 1);
		break;

	case WD17XX_DRQ_CLR:
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
		break;

	case WD17XX_DRQ_SET:
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
		break;
	}
}

static const wd17xx_interface v1050_wd17xx_intf = 
{ 
	v1050_mb8877_callback
};

/* Machine Initialization */

static MACHINE_START( v1050 )
{
	v1050_state *state = machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->upb8214 = devtag_get_device(machine, UPB8214_TAG);
	state->mb8877 = devtag_get_device(machine, MB8877_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);

	/* setup memory banking */
	memory_configure_bank(machine, 1, 0, 2, mess_ram, 0x10000);
	memory_configure_bank(machine, 1, 2, 1, mess_ram + 0x1c000, 0);
	memory_configure_bank(machine, 1, 3, 1, memory_region(machine, Z80_TAG), 0);

	memory_install_readwrite8_handler(program, 0x2000, 0x3fff, 0, 0, SMH_BANK(2), SMH_BANK(2));
	memory_configure_bank(machine, 2, 0, 2, mess_ram + 0x2000, 0x10000);
	memory_configure_bank(machine, 2, 2, 1, mess_ram + 0x1e000, 0);

	memory_install_readwrite8_handler(program, 0x4000, 0x7fff, 0, 0, SMH_BANK(3), SMH_BANK(3));
	memory_configure_bank(machine, 3, 0, 2, mess_ram + 0x4000, 0x10000);

	memory_install_readwrite8_handler(program, 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
	memory_configure_bank(machine, 4, 0, 2, mess_ram + 0x8000, 0x10000);

	memory_install_readwrite8_handler(program, 0xc000, 0xffff, 0, 0, SMH_BANK(5), SMH_BANK(5));
	memory_configure_bank(machine, 5, 0, 3, mess_ram + 0xc000, 0);

	v1050_bankswitch(machine);

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static MACHINE_DRIVER_START( v1050 )
	MDRV_DRIVER_DATA(v1050_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
    MDRV_CPU_PROGRAM_MAP(v1050_mem)
    MDRV_CPU_IO_MAP(v1050_io)

    MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_15_36MHz/16)
    MDRV_CPU_PROGRAM_MAP(v1050_crt_mem)
/*
    MDRV_CPU_ADD(I8049_TAG, I8049, XTAL_4_608MHz)
	MDRV_CPU_IO_MAP(v1050_kbd_io)
*/
    MDRV_MACHINE_START(v1050)

    /* video hardware */
	MDRV_IMPORT_FROM(v1050_video)

	/* devices */
	MDRV_PPI8255_ADD(FDC_PPI8255_TAG, misc_ppi8255_intf)
	MDRV_WD1793_ADD(MB8877_TAG, v1050_wd17xx_intf )

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( v1050 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "244-032.u86", 0x0000, 0x2000, CRC(46f847a7) SHA1(374db7a38a9e9230834ce015006e2f1996b9609a) )

	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "244-033.u77", 0xe000, 0x2000, CRC(c0502b66) SHA1(bc0015f5b14f98110e652eef9f7c57c614683be5) )

	ROM_REGION( 0x800, I8049_TAG, 0 )
	ROM_LOAD( "20-08049-410.z5", 0x0000, 0x0800, NO_DUMP )
	ROM_LOAD( "22-02716-074.z6", 0x0000, 0x0800, NO_DUMP )
ROM_END

/* System Configuration */

static DEVICE_IMAGE_LOAD( v1050_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	return INIT_FAIL;
}

static void v1050_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(v1050_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( v1050 )
	CONFIG_RAM_DEFAULT( 128 * 1024 )
	CONFIG_DEVICE(v1050_floppy_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY								FULLNAME		FLAGS */
COMP( 1983, v1050,	0,		0,		v1050,	v1050,	0,		v1050,	"Visual Technology Incorporated",	"Visual 1050",	GAME_NOT_WORKING )
