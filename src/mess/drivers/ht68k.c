/***************************************************************************

        Hawthorne Technologies TinyGiant HT68k

        29/11/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/68681.h"
#include "devices/flopdrv.h"
#include "machine/wd17xx.h"
#include "machine/terminal.h"

static UINT16* ht68k_ram;

static ADDRESS_MAP_START(ht68k_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM AM_BASE(&ht68k_ram) // 512 KB RAM / ROM at boot
	//AM_RANGE(0x00080000, 0x000fffff) // Expansion
	//AM_RANGE(0x00d80000, 0x00d8ffff) // Printer
	AM_RANGE(0x00e00000, 0x00e00007) AM_MIRROR(0xfff8) AM_DEVREADWRITE8("wd1770", wd17xx_r, wd17xx_w, 0x00ff) // FDC WD1770
	AM_RANGE(0x00e80000, 0x00e800ff) AM_MIRROR(0xff00) AM_DEVREADWRITE8( "duart68681", duart68681_r, duart68681_w, 0xff )
	AM_RANGE(0x00f00000, 0x00f07fff) AM_ROM AM_MIRROR(0xf8000) AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( ht68k )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(ht68k)
{
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)ht68k_ram,user1,0x8000);

	devtag_get_device(machine, "maincpu")->reset();
}

static void duart_irq_handler(running_device *device, UINT8 vector)
{
	cputag_set_input_line_and_vector(device->machine, "maincpu", M68K_IRQ_3, HOLD_LINE, M68K_INT_ACK_AUTOVECTOR);
}

static void duart_tx(running_device *device, int channel, UINT8 data)
{
	running_device *devconf = devtag_get_device(device->machine, "terminal");
	terminal_write(devconf,0,data);
}

static UINT8 duart_input(running_device *device)
{
	return 0;
}

static void duart_output(running_device *device, UINT8 data)
{
	running_device *fdc = devtag_get_device(device->machine, "wd1770");
	wd17xx_set_side(fdc,BIT(data,3) ? 0 : 1);
	if (BIT(data,7)==0) {
 		wd17xx_set_drive(fdc,0);
	} else if (BIT(data,6)==0) {
 		wd17xx_set_drive(fdc,1);
	} else if (BIT(data,5)==0) {
 		wd17xx_set_drive(fdc,2);
	} else if (BIT(data,4)==0) {
 		wd17xx_set_drive(fdc,3);
	}
}

static WRITE8_DEVICE_HANDLER( ht68k_kbd_put )
{
	duart68681_rx_data(devtag_get_device(device->machine, "duart68681"), 0, data);
}

static GENERIC_TERMINAL_INTERFACE( ht68k_terminal_intf )
{
	DEVCB_HANDLER(ht68k_kbd_put)
};

static const duart68681_config ht68k_duart68681_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	duart_output
};

static WRITE_LINE_DEVICE_HANDLER( ht68k_fdc_intrq_w )
{
	//cputag_set_input_line_and_vector(device->machine, "maincpu", M68K_IRQ_4, HOLD_LINE, M68K_INT_ACK_AUTOVECTOR);
}

static const wd17xx_interface ht68k_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(ht68k_fdc_intrq_w),
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static const floppy_config ht68k_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( ht68k )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(ht68k_mem)

    MDRV_MACHINE_RESET(ht68k)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,ht68k_terminal_intf)

	MDRV_DUART68681_ADD( "duart68681", XTAL_8MHz / 2, ht68k_duart68681_config )
	MDRV_WD1770_ADD("wd1770", ht68k_wd17xx_interface )
	MDRV_FLOPPY_4_DRIVES_ADD(ht68k_floppy_config)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( ht68k )
    ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "ht68k-u4.bin", 0x0000, 0x4000, CRC(3fbcdd0a) SHA1(45fcbbf920dc1e9eada3b7b0a55f5720d08ffdd5))
	ROM_LOAD16_BYTE( "ht68k-u3.bin", 0x0001, 0x4000, CRC(1d85d101) SHA1(8ba01e1595b0b3c4fb128a4a50242f3588b89c43))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                     FULLNAME                    FLAGS */
COMP( 1987, ht68k,  0,       0, 	 ht68k, 	ht68k, 	 0,  	 "Hawthorne Technologies",   "TinyGiant HT68k",		GAME_NO_SOUND)

