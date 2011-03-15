/*

	Luxor ABC 830

    Type                    Size        Tracks  Sides   Sectors/track   Sectors     Speed           Drives

    ABC-830     Floppy      160 KB      40      1       16              640         250 Kbps        MPI 51, BASF 6106
                Floppy      80 KB       40      1       8               320         250 Kbps        Scandia Metric FD2
    ABC-832     Floppy      640 KB      80      2       16              2560        250 Kbps        Micropolis 1015, Micropolis 1115, BASF 6118
    ABC-834     Floppy      640 KB      80      2       16              2560        250 Kbps        Teac FD 55 F
    ABC-838     Floppy      1 MB        77      2       25              3978        500 Kbps        BASF 6104, BASF 6115
    ABC-850     Floppy      640 KB      80      2       16              2560        250 Kbps        TEAC FD 55 F, BASF 6136
                HDD         10 MB       320     4       32              40960       5 Mbps          Rodime 202, BASF 6186
    ABC-852     Floppy      640 KB      80      2       16              2560        250 Kbps        TEAC FD 55 F, BASF 6136
                HDD         20 MB       615     4       32              78720       5 Mbps          NEC 5126
                Streamer    45 MB       9                                           90 Kbps         Archive 5945-C (tape: DC300XL 450ft)
    ABC-856     Floppy      640 KB      80      2       16              2560        250 Kbps        TEAC FD 55 F
                HDD         64 MB       1024    8       32              262144      5 Mbps          Micropolis 1325
                Streamer    45 MB       9                                           90 Kbps         Archive 5945-C (tape: DC300XL 450ft)

*/

#include "abc830.h"



//**************************************************************************
//  FLOPPY CONFIGURATIONS
//**************************************************************************

//-------------------------------------------------
//  floppy_config abc830_floppy_config
//-------------------------------------------------

static FLOPPY_OPTIONS_START( abc830 )
	// NOTE: Real ABC 830 (160KB) disks use a 7:1 sector interleave.
	//
	// The controller ROM is patched to remove the interleaving so
	// you can use disk images with logical sector layout instead.
	//
	// Specify INTERLEAVE([7]) below if you prefer the physical layout.
	FLOPPY_OPTION(abc830, "dsk", "Luxor ABC 830", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

const floppy_config abc830_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSDD_40,
    FLOPPY_OPTIONS_NAME(abc830),
    "abc830"
};


//-------------------------------------------------
//  floppy_config abc832_floppy_config
//-------------------------------------------------

static FLOPPY_OPTIONS_START( abc832 )
	FLOPPY_OPTION(abc832, "dsk", "Luxor ABC 832/834", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

const floppy_config abc832_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_DSQD,
    FLOPPY_OPTIONS_NAME(abc832),
    "abc832"
};


//-------------------------------------------------
//  floppy_config abc838_floppy_config
//-------------------------------------------------

static FLOPPY_OPTIONS_START( abc838 )
	FLOPPY_OPTION(abc838, "dsk", "Luxor ABC 838", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

const floppy_config abc838_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_8_DSDD,
    FLOPPY_OPTIONS_NAME(abc838),
    "abc838"
};


//-------------------------------------------------
//  floppy_config fd2_floppy_config
//-------------------------------------------------

static FLOPPY_OPTIONS_START( fd2 )
	// NOTE: FD2 cannot be used with the Luxor controller card,
	// it has a proprietary one. This is just for reference.
	FLOPPY_OPTION(fd2, "dsk", "Scandia Metric FD2", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

const floppy_config fd2_floppy_config =
{
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    DEVCB_NULL,
    FLOPPY_STANDARD_5_25_SSSD,
    FLOPPY_OPTIONS_NAME(fd2),
    "floppy_5_25"
};



//**************************************************************************
//  LUXOR 55 10828 INTERFACES
//**************************************************************************

//-------------------------------------------------
//  LUXOR_55_10828_INTERFACE( abc830_slow_intf )
//-------------------------------------------------

LUXOR_55_10828_INTERFACE( abc830_slow_intf )
{
	0x03,
	DRIVE_MPI_51,
	ADDRESS_ABC830
};



//**************************************************************************
//  LUXOR 55 21046 INTERFACES
//**************************************************************************

//-------------------------------------------------
//  LUXOR_55_21046_INTERFACE( abc830_fast_intf )
//-------------------------------------------------

LUXOR_55_21046_INTERFACE( abc830_fast_intf )
{
	0x03,
	DRIVE_BASF_6106_08,
	ADDRESS_ABC830
};


//-------------------------------------------------
//  LUXOR_55_21046_INTERFACE( abc832_fast_intf )
//-------------------------------------------------

LUXOR_55_21046_INTERFACE( abc832_fast_intf )
{
	0x03,
	DRIVE_TEAC_FD55F,
	ADDRESS_ABC832
};


//-------------------------------------------------
//  LUXOR_55_21046_INTERFACE( abc834_fast_intf )
//-------------------------------------------------

LUXOR_55_21046_INTERFACE( abc834_fast_intf )
{
	0x03,
	DRIVE_TEAC_FD55F,
	ADDRESS_ABC832
};


//-------------------------------------------------
//  LUXOR_55_21046_INTERFACE( abc838_fast_intf )
//-------------------------------------------------

LUXOR_55_21046_INTERFACE( abc838_fast_intf )
{
	0x03,
	DRIVE_BASF_6105,
	ADDRESS_ABC838
};
