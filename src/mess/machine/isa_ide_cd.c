/***************************************************************************

  ISA 16 bit IDE controller

***************************************************************************/

#include "emu.h"
#include "isa_ide_cd.h"

static void atapi_irq(device_t *device, int state)
{
	isa16_ide_cd_device *ide  = downcast<isa16_ide_cd_device *>(device);
	if (ide->is_primary()) {
		ide->m_isa->irq14_w(state);
	} else {
		ide->m_isa->irq15_w(state);
	}
}

WRITE16_MEMBER( isa16_ide_cd_device::atapi_cmd_w )
{
}
READ16_MEMBER( isa16_ide_cd_device::atapi_status_r )
{
	UINT8 *atapi_regs = m_atapi_regs;
	int shift;
	shift = 0;
	int reg = offset<<1;
	switch(mem_mask)
	{
	case 0x000000ff:
		break;	
	case 0x0000ff00:
		reg+=1;
		shift=8;
		break;
	}			
	UINT32 data = atapi_regs[ATAPI_REG_CMDSTATUS];
	data <<= shift;
	return  data;
}

READ16_MEMBER( isa16_ide_cd_device::atapi_r )
{
	UINT8 *atapi_regs = m_atapi_regs;
	//running_machine &machine = machine();
	int reg, data;
	if (mem_mask == 0x0000ffff)	// word-wide command read
	{
      logerror("ATAPI: packet read = %04x\n", m_atapi_data[m_atapi_data_ptr]);

		// assert IRQ and drop DRQ
		if (m_atapi_data_ptr == 0 && m_atapi_data_len == 0)
		{
			// get the data from the device
			if( m_atapi_xferlen > 0 )
			{
				SCSIReadData( m_inserted_cdrom, m_atapi_data, m_atapi_xferlen );
				m_atapi_data_len = m_atapi_xferlen;
			}

			if (m_atapi_xfermod > MAX_TRANSFER_SIZE)
			{
				m_atapi_xferlen = MAX_TRANSFER_SIZE;
				m_atapi_xfermod = m_atapi_xfermod - MAX_TRANSFER_SIZE;
			}
			else
			{
				m_atapi_xferlen = m_atapi_xfermod;
				m_atapi_xfermod = 0;
			}

			//verboselog\\( machine, 2, "atapi_r: atapi_xferlen=%d\n", m_atapi_xferlen );
			if( m_atapi_xferlen != 0 )
			{
				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
				atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
			}
			else
			{
				logerror("ATAPI: dropping DRQ\n");
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
				atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
			}

			atapi_regs[ATAPI_REG_COUNTLOW] = m_atapi_xferlen & 0xff;
			atapi_regs[ATAPI_REG_COUNTHIGH] = (m_atapi_xferlen>>8)&0xff;

			atapi_irq(this, ASSERT_LINE);
		}

		if( m_atapi_data_ptr < m_atapi_data_len )
		{
			data = m_atapi_data[m_atapi_data_ptr++];
			data |= ( m_atapi_data[m_atapi_data_ptr++] << 8 );
			if( m_atapi_data_ptr >= m_atapi_data_len )
			{
              
				m_atapi_data_ptr = 0;
				m_atapi_data_len = 0;

				if( m_atapi_xferlen == 0 )
				{
					atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
					atapi_irq(this, ASSERT_LINE);
				}
			}
		}
		else
		{
			data = 0;
		}
	}
	else
	{
		atapi_irq(this, CLEAR_LINE);
		int shift;
		shift = 0;
		reg = offset<<1;
		switch(mem_mask)
		{
		case 0x000000ff:
			break;	
		case 0x0000ff00:
			reg+=1;
			shift=8;
			break;
		}			
		data = atapi_regs[reg];
   		logerror("ATAPI: reg %d = %x (offset %x mask %x) [read]\n", reg, data, offset, mem_mask);		
      	data <<= shift;
	}
	return data;
}

WRITE16_MEMBER( isa16_ide_cd_device::atapi_w )
{
	UINT8 *atapi_regs = m_atapi_regs;
	UINT8 *atapi_data = m_atapi_data;
	int reg;
	if (mem_mask == 0x0000ffff)	// word-wide command write
	{
		atapi_data[m_atapi_data_ptr++] = data & 0xff;
		atapi_data[m_atapi_data_ptr++] = data >> 8;

		if (m_atapi_cdata_wait)
		{
          	logerror("ATAPI: waiting, ptr %d wait %d\n", m_atapi_data_ptr, m_atapi_cdata_wait);
			if (m_atapi_data_ptr == m_atapi_cdata_wait)
			{
				// send it to the device
				SCSIWriteData( m_inserted_cdrom, atapi_data, m_atapi_cdata_wait );

				// assert IRQ
				atapi_irq(this, ASSERT_LINE);

				// not sure here, but clear DRQ at least?
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
			}
		}

		else if ( m_atapi_data_ptr == 12 )
		{
			int phase;
			// reset data pointer for reading SCSI results
			m_atapi_data_ptr = 0;
			m_atapi_data_len = 0;

			// send it to the SCSI device
			SCSISetCommand( m_inserted_cdrom, m_atapi_data, 12 );
			SCSIExecCommand( m_inserted_cdrom, &m_atapi_xferlen );
			SCSIGetPhase( m_inserted_cdrom, &phase );

			if (m_atapi_xferlen != -1)
			{
              	logerror("ATAPI: SCSI command %02x returned %d bytes from the device\n", atapi_data[0]&0xff, m_atapi_xferlen);
				// store the returned command length in the ATAPI regs, splitting into
				// multiple transfers if necessary
				m_atapi_xfermod = 0;
				if (m_atapi_xferlen > MAX_TRANSFER_SIZE)
				{
					m_atapi_xfermod = m_atapi_xferlen - MAX_TRANSFER_SIZE;
					m_atapi_xferlen = MAX_TRANSFER_SIZE;
				}
 
				atapi_regs[ATAPI_REG_COUNTLOW] = m_atapi_xferlen & 0xff;
				atapi_regs[ATAPI_REG_COUNTHIGH] = (m_atapi_xferlen>>8)&0xff;

				if (m_atapi_xferlen == 0)
				{
					// if no data to return, set the registers properly
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRDY;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO|ATAPI_INTREASON_COMMAND;
				}
				else
				{
					// indicate data ready: set DRQ and DMA ready, and IO in INTREASON
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;
				}

				switch( phase )
				{
				case SCSI_PHASE_DATAOUT:
					m_atapi_cdata_wait = m_atapi_xferlen;
					break;
				}

				// perform special ATAPI processing of certain commands
				switch (atapi_data[0]&0xff)
				{
					case 0x00: // BUS RESET / TEST UNIT READY
					case 0xbb: // SET CDROM SPEED
						atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
						break;

					case 0x45: // PLAY
						atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_BSY;
						//m_atapi_timer->adjust( downcast<cpu_device *>(this)->cycles_to_attotime( ATAPI_CYCLES_PER_SECTOR ) );
						break;
				}

				// assert IRQ
				atapi_irq(this, ASSERT_LINE);
				logerror("IRQ\n");
			}
			else
			{
              	logerror("ATAPI: SCSI device returned error!\n");

				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_CHECK;
				atapi_regs[ATAPI_REG_ERRFEAT] = 0x50;	// sense key = ILLEGAL REQUEST
				atapi_regs[ATAPI_REG_COUNTLOW] = 0;
				atapi_regs[ATAPI_REG_COUNTHIGH] = 0;
			}
		}
	}
	else
	{
		reg = offset<<1;
		switch(mem_mask)
		{
		case 0x000000ff:
			break;	
		case 0x0000ff00:
			reg+=1;
			data >>= 8;
			break;
		}			

		atapi_regs[reg] = data;
      	logerror("ATAPI: reg %d = %x (offset %x mask %x)\n", reg, data, offset, mem_mask);

		if (reg == ATAPI_REG_CMDSTATUS)
		{
          	logerror("ATAPI command %x issued!\n", data);
			switch (data)
			{
				case 0xa0:	// PACKET
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;
					atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_COMMAND;

					m_atapi_data_ptr = 0;
					m_atapi_data_len = 0;

					/* we have no data */
					m_atapi_xferlen = 0;
					m_atapi_xfermod = 0;

					m_atapi_cdata_wait = 0;
					break;

				case 0xa1:	// IDENTIFY PACKET DEVICE
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ;

					m_atapi_data_ptr = 0;
					m_atapi_data_len = 512;

					/* we have no data */
					m_atapi_xferlen = 0;
					m_atapi_xfermod = 0;

					memset( atapi_data, 0, m_atapi_data_len );

					atapi_data[ 0 ^ 1 ] = 0x85;	// ATAPI device, cmd set 5 compliant, DRQ within 3 ms of PACKET command
					atapi_data[ 1 ^ 1 ] = 0x80;	// ATAPI device, removable media

					memset( &atapi_data[ 46 ], ' ', 8 );
					atapi_data[ 46 ^ 1 ] = '1';
					atapi_data[ 47 ^ 1 ] = '.';
					atapi_data[ 48 ^ 1 ] = '0';
					
					
					memset( &atapi_data[ 54 ], ' ', 40 );
					atapi_data[ 54 ^ 1 ] = 'T';
					atapi_data[ 55 ^ 1 ] = 'O';
					atapi_data[ 56 ^ 1 ] = 'S';
					atapi_data[ 57 ^ 1 ] = 'H';
					atapi_data[ 58 ^ 1 ] = 'I';
					atapi_data[ 59 ^ 1 ] = 'B';
					atapi_data[ 60 ^ 1 ] = 'A';
					atapi_data[ 61 ^ 1 ] = ' ';
					atapi_data[ 62 ^ 1 ] = 'X';
					atapi_data[ 63 ^ 1 ] = 'M';
					atapi_data[ 64 ^ 1 ] = '-';
					atapi_data[ 65 ^ 1 ] = '3';
					atapi_data[ 66 ^ 1 ] = '3';
					atapi_data[ 67 ^ 1 ] = '0';
					atapi_data[ 68 ^ 1 ] = '1';
					atapi_data[ 69 ^ 1 ] = ' ';

					atapi_data[ 98 ^ 1 ] = 0x06; // Word 49=Capabilities, IORDY may be disabled (bit_10), LBA Supported mandatory (bit_9)
					atapi_data[ 99 ^ 1 ] = 0x00;

					atapi_regs[ATAPI_REG_COUNTLOW] = 0;
					atapi_regs[ATAPI_REG_COUNTHIGH] = 2;

					atapi_irq(this, ASSERT_LINE);
					break;
				case 0xec:	//IDENTIFY DEVICE - Must abort here and set for packet data
					atapi_regs[ATAPI_REG_ERRFEAT] = ATAPI_ERRFEAT_ABRT;
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_CHECK;
					
					atapi_irq(this, ASSERT_LINE);
					break;
				case 0xef:	// SET FEATURES
					atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

					m_atapi_data_ptr = 0;
					m_atapi_data_len = 0;

					atapi_irq(this, ASSERT_LINE);
					break;

				case 0x08:	// ATAPI RESET
					atapi_regs[ATAPI_REG_CMDSTATUS] = 0x00;
					atapi_regs[ATAPI_REG_ERRFEAT]   = 0x01;
					atapi_regs[ATAPI_REG_INTREASON] = 0x01; // SECTOR_COUNT
					atapi_regs[ATAPI_REG_SAMTAG]    = 0x01; // SECTOR_NUMBER
					atapi_regs[ATAPI_REG_COUNTLOW]  = 0x14; // CYLINDER_LSB
					atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb; // CYLINDER_MSB
					atapi_regs[ATAPI_REG_DRIVESEL]  &= 0xf0; // HEAD_NUMBER
					
					atapi_irq(this, ASSERT_LINE);
					break;
				default:
					logerror("ATAPI: Unknown IDE command %x\n", data);
					atapi_regs[ATAPI_REG_ERRFEAT] = ATAPI_ERRFEAT_ABRT;
					atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_CHECK;
				
					atapi_irq(this, ASSERT_LINE);
					break;
			}
		}
	}
}

static struct cdrom_interface ide_cdrom =
{
	NULL,
	NULL
};

static MACHINE_CONFIG_FRAGMENT( ide )	
	MCFG_CDROM_ADD( "cdrom", ide_cdrom )
MACHINE_CONFIG_END

static INPUT_PORTS_START( ide )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x00, "IDE Configuration")
	PORT_DIPSETTING(	0x00, "Primary" )
	PORT_DIPSETTING(	0x01, "Secondary" )
INPUT_PORTS_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA16_IDE_CD = &device_creator<isa16_ide_cd_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa16_ide_cd_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ide );
}

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor isa16_ide_cd_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( ide );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa16_ide_cd_device - constructor
//-------------------------------------------------

isa16_ide_cd_device::isa16_ide_cd_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
      : device_t(mconfig, ISA16_IDE_CD, "IDE CD Drive Adapter", tag, owner, clock),
		device_isa16_card_interface( mconfig, *this ),
		m_is_primary(true),
		m_inserted_cdrom(NULL)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa16_ide_cd_device::device_start()
{
	set_isa_device();
}

//-------------------------------------------------
//  device_stop - device-specific startup
//-------------------------------------------------

void isa16_ide_cd_device::device_stop()
{
	SCSIDeleteInstance(m_inserted_cdrom);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa16_ide_cd_device::device_reset()
{
	astring tempstring;
	if (m_inserted_cdrom) SCSIDeleteInstance(m_inserted_cdrom);
	SCSIAllocInstance( machine(), &SCSIClassCr589, &m_inserted_cdrom, subtag(tempstring, "cdrom"));
	m_is_primary = (input_port_read(*this, "DSW") & 1) ? false : true;
	if (m_is_primary) {	
		m_isa->install16_device(0x01f0, 0x01f7, 0, 0, read16_delegate(FUNC(isa16_ide_cd_device::atapi_r), this), write16_delegate(FUNC(isa16_ide_cd_device::atapi_w), this));		
		//m_isa->install16_device(0x03f0, 0x03f7, 0, 0, read16_delegate(FUNC(isa16_ide_cd_device::atapi_status_r), this), write16_delegate(FUNC(isa16_ide_cd_device::atapi_cmd_w), this));		
	} else {
		m_isa->install16_device(0x0170, 0x0177, 0, 0, read16_delegate(FUNC(isa16_ide_cd_device::atapi_r), this), write16_delegate(FUNC(isa16_ide_cd_device::atapi_w), this));		
		m_isa->install16_device(0x0370, 0x0377, 0, 0, read16_delegate(FUNC(isa16_ide_cd_device::atapi_status_r), this), write16_delegate(FUNC(isa16_ide_cd_device::atapi_cmd_w), this));		
	}
}
