/*

		              a     b
		-12 V	<--   *  1  *   --> -12V
		0 V		---   *  2  *   --- 0 V
		RESIN_	-->   *  3  *   --> XMEMWR_
		0 V		---	  *  4  *   --> XMEMFL_
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
		IN1		<--   * 16  *   --> A13
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

READ8_HANDLER( abcbus_data_r );
WRITE8_HANDLER( abcbus_data_w );
READ8_HANDLER( abcbus_status_r );
WRITE8_HANDLER( abcbus_select_w );
WRITE8_HANDLER( abcbus_command_w );
READ8_HANDLER( abcbus_reset_r );
READ8_HANDLER( abcbus_strobe_r );
WRITE8_HANDLER( abcbus_strobe_w );
