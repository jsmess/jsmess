/*********************************************************************

    a2scsi.c

    Implementation of the Apple II SCSI Card

    Schematic at:
    http://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Interface%20Cards/SCSI%20Controllers/Apple%20II%20SCSI%20Card/Schematics/Rev.%20C%20SCSI%20Schematic%20-%20Updated%202-23-6.jpg


    Notes:

    PAL U3C:
    I1 = A0
    I2 = A1
    I3 = A2
    I4 = A3
    I5 = O2 of PAL U3D
    I6 = (logic maze)
    I7 = R/W (also I7 of PAL U3D)
    I8 = PHI0 (via flip flops)
    I9 = RESET
    I10 = GND

    O1 = P on flip-flops U6DA/U6DB, I8 on PAL U3D
    O2 = N/C
    O3 = I6 on PAL U3D
    O4 = N/C
    O5 = /CS on NCR 5380
    O6 = /WR on NCR 5380
    O7 = /RD on NCR 5380
    O8 = /DACK on NCR 5380

    PAL U3D:
    I1 = A0-A10 NANDed together (goes low on CFFF access, i.e.)
    I2 = /IOSTB
    I3 = /IOSEL
    I4 = RESET
    I5 = A10 (splits C800 space at C800/CC00)
    I6 = O3 of PAL U3C
    I7 = R/W
    I8 = O1 of PAL U3C
    I9 = DRQ from 5380
    I10 = Y6 from 74LS138 at U3D (C0n6 access)

    O1 = G on 74LS245 main data bus buffer
    O2 = I5 of PAL U3C
    O3 = D to flip-flop at U6DB
    O4 = CLK to flip-flop at U6DA
    O5 = D7 (?)
    O6 = CS1 on RAM
    O7 = EPROM CE
    O8 = EPROM OE

*********************************************************************/

#include "a2scsi.h"
#include "includes/apple2.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_SCSI = &device_creator<a2bus_scsi_device>;

#define SCSI_ROM_REGION  "scsi_rom"
#define SCSI_5380_TAG    "ncr5380"

static const SCSIConfigTable dev_table =
{
	0,                                      /* 2 SCSI devices */
	{
//   { SCSI_ID_6, "harddisk1", SCSI_DEVICE_HARDDISK },  /* SCSI ID 6, using disk1, and it's a harddisk */
//   { SCSI_ID_5, "harddisk2", SCSI_DEVICE_HARDDISK }   /* SCSI ID 5, using disk2, and it's a harddisk */
	}
};

static const struct NCR5380interface a2scsi_5380_intf =
{
	&dev_table,	// SCSI device table
	NULL        // IRQ handler (unconnected according to schematic)
};

MACHINE_CONFIG_FRAGMENT( scsi )
	MCFG_NCR5380_ADD(SCSI_5380_TAG, (XTAL_28_63636MHz/4), a2scsi_5380_intf)
MACHINE_CONFIG_END

ROM_START( scsi )
	ROM_REGION(0x4000, SCSI_ROM_REGION, 0)  // this is the Rev. C ROM
    ROM_LOAD( "341-0437-a.bin", 0x0000, 0x4000, CRC(5aff85d3) SHA1(451c85c46b92e6ad2ad930f055ccf0fe3049936d) )
ROM_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor a2bus_scsi_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( scsi );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *a2bus_scsi_device::device_rom_region() const
{
	return ROM_NAME( scsi );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_scsi_device::a2bus_scsi_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this)
{
	m_shortname = "a2scsi";
}

a2bus_scsi_device::a2bus_scsi_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, A2BUS_SCSI, "Apple II SCSI Card", tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this)
{
	m_shortname = "a2scsi";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_scsi_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	astring tempstring;
	m_rom = device().machine().region(this->subtag(tempstring, SCSI_ROM_REGION))->base();

    memset(m_ram, 0, 8192);
}

void a2bus_scsi_device::device_reset()
{
    m_rambank = m_rombank = 0;      // CLR on 74LS273 at U3E is connected to RES, so these clear on reset
}


/*-------------------------------------------------
    read_c0nx - called for reads from this card's c0nx space
-------------------------------------------------*/

/*

From schematic (decoded by 74LS138 at U2D)

C0n1/9 = DIP switches
C0n2/a = clock to 74LS273 bank switcher (so data here should be the bank)
C0n3/b = reset 5380
C0n6/e = into the PAL stuff

*/

UINT8 a2bus_scsi_device::read_c0nx(address_space &space, UINT8 offset)
{
    printf("Read c0n%x (PC=%x)\n", offset, cpu_get_pc(&space.device()));

	return 0xff;
}


/*-------------------------------------------------
    write_c0nx - called for writes to this card's c0nx space
-------------------------------------------------*/

void a2bus_scsi_device::write_c0nx(address_space &space, UINT8 offset, UINT8 data)
{
    printf("Write %02x to c0n%x (PC=%x)\n", data, offset, cpu_get_pc(&space.device()));

    switch (offset & 7)
    {
        case 0x2:   // ROM and RAM banking (74LS273 at U3E)
            /*
                ROM banking:
                (bits EA8-EA13 are all zeroed when /IOSEL is asserted, so CnXX always gets the first page of the ROM)
                EA10 = bit 0
                EA11 = bit 1
                EA12 = bit 2
                EA13 = bit 3 (N/C)

                RAM banking:
                RA10 = bit 4
                RA11 = bit 5
                RA12 = bit 6
            */

            m_rambank = ((data>>4) & 0x7) * 0x400;
            m_rombank = (data & 0xf) * 0x400;
            printf("RAM bank to %x, ROM bank to %x\n", m_rambank, m_rombank);
            break;
    }
}

/*-------------------------------------------------
    read_cnxx - called for reads from this card's cnxx space
-------------------------------------------------*/

UINT8 a2bus_scsi_device::read_cnxx(address_space &space, UINT8 offset)
{
    // one slot image at the start of the ROM, it appears
    return m_rom[offset];
}

void a2bus_scsi_device::write_cnxx(address_space &space, UINT8 offset, UINT8 data)
{
    printf("Write %02x to cn%02x (PC=%x)\n", data, offset, cpu_get_pc(&space.device()));
}

/*-------------------------------------------------
    read_c800 - called for reads from this card's c800 space
-------------------------------------------------*/

UINT8 a2bus_scsi_device::read_c800(address_space &space, UINT16 offset)
{
    // bankswitched RAM at c800-cbff
    // bankswitched ROM at cc00-cfff
    if (offset < 0x400)
    {
        printf("Read RAM at %x = %02x\n", offset+m_rambank, m_ram[offset + m_rambank]);
        return m_ram[offset + m_rambank];
    }
    else
    {
        return m_rom[(offset-0x400) + m_rombank];
    }
}

/*-------------------------------------------------
    write_c800 - called for writes to this card's c800 space
-------------------------------------------------*/
void a2bus_scsi_device::write_c800(address_space &space, UINT16 offset, UINT8 data)
{
    if (offset < 0x400)
    {
        printf("%02x to RAM at %x\n", data, offset+m_rambank);
        m_ram[offset + m_rambank] = data;
    }
}
