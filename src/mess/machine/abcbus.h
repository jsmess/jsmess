/**********************************************************************

    Luxor ABC-bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

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

*/

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


    ABCBUS_CHANNEL_ABC850_852_856 = 36,
    ABCBUS_CHANNEL_ABC832_834_850 = 44,
    ABCBUS_CHANNEL_ABC830 = 45,
    ABCBUS_CHANNEL_ABC838 = 46

*/

/*

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

#ifndef __ABCBUS__
#define __ABCBUS__

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

DECLARE_LEGACY_DEVICE(ABCBUS, abcbus);

#define MDRV_ABCBUS_ADD(_tag, _daisy_chain) \
	MDRV_DEVICE_ADD(_tag, ABCBUS, 0) \
	MDRV_DEVICE_CONFIG(_daisy_chain)

#define ABCBUS_DAISY(_name) \
	const abcbus_daisy_chain (_name)[] =

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _abcbus_daisy_chain abcbus_daisy_chain;
struct _abcbus_daisy_chain
{
	const char *tag;	/* device tag */

	devcb_write8		out_cs_func;

	devcb_read8			in_stat_func;

	devcb_read8			in_inp_func;
	devcb_write8		out_utp_func;

	devcb_write8		out_c1_func;
	devcb_write8		out_c2_func;
	devcb_write8		out_c3_func;
	devcb_write8		out_c4_func;

	devcb_write_line	out_rst_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/
/* card select */
WRITE8_DEVICE_HANDLER( abcbus_cs_w );

/* reset */
READ8_DEVICE_HANDLER( abcbus_rst_r );

/* data */
READ8_DEVICE_HANDLER( abcbus_inp_r );
WRITE8_DEVICE_HANDLER( abcbus_utp_w );

/* status */
READ8_DEVICE_HANDLER( abcbus_stat_r );

/* commands */
WRITE8_DEVICE_HANDLER( abcbus_c1_w );
WRITE8_DEVICE_HANDLER( abcbus_c2_w );
WRITE8_DEVICE_HANDLER( abcbus_c3_w );
WRITE8_DEVICE_HANDLER( abcbus_c4_w );

#endif
