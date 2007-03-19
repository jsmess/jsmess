/******************************************************************************

	i82720cm.h
	Video driver for the Intel 82720 and NEC uPD7220 GDC.

	Per Ola Ingvarsson
	Tomas Karlsson
			
 ******************************************************************************/

enum I82720_GDC_COMMANDS
{
	/* Video control commands */
	CMD_RESET	 = 0x00,	/* Resets the GDC to its idle mode */
	CMD_SYNC_OFF	 = 0x0e,	/* Specifies the video format and disables the display */
	CMD_SYNC_ON	 = 0x0f,	/* Specifies the video format and enables the display */
	CMD_VSYNC_SLAVE	 = 0x6e,	/* Selects slave video synchronization mode */
	CMD_VSYNC_MASTER = 0x6f,	/* Selects master video synchronization mode */
	CMD_CCHAR	 = 0x4b,	/* Specifies the cursor and character row heights */
                           
	/* Display control commands */
	CMD_START	 = 0x6b,	/* Ends idle mode and unblanks the display */
	CMD_BCTRL_OFF	 = 0x0c,	/* Controls the blanking and unblanking of the display */
	CMD_BCTRL_ON	 = 0x0d,	/* Controls the blanking and unblanking of the display */
	CMD_ZOOM	 = 0x46,	/* Specifies zoom factors for the display and graphics characters writing */
	CMD_CURS	 = 0x49,	/* Sets the position of the cursor in display memory */
	CMD_PRAM_BITMASK = 0x70,	/* Defines starting addresses and lengths of the display areas and specifies the eight bytes for the graphics character */
	CMD_PITCH	 = 0x47,	/* Specifies the width of the X dimension of the display memory */
	
	/* Drawing control commands */
	CMD_WDAT_BITMASK = 0x20,	/* Writes data words or bytes into display memory */
	CMD_MASK	 = 0x4a,	/* Sets the MASK register contents */
	CMD_FIGS	 = 0x4c,	/* Specifies the parameters for the drawing processor */
	CMD_FIGD	 = 0x6c,	/* Draws the figure as specified above */
	CMD_GCHRD	 = 0x68,	/* Draws the graphics character into display */

	/* Memory data read commands */
	CMD_RDAT_BITMASK = 0xa0,	/* Reads data words or bytes from display */
	CMD_CURD	 = 0xe0,	/* Reads the cursor position */
	CMD_LPRD	 = 0xc0,	/* Reads the light pen address */
	
	/* Data control commands */
	CMD_DMAR_BITMASK = 0xa4,	/* Requests a DMA read transfer */
	CMD_DMAW_BITMASK = 0x24		/* Requests a DMA write transfer */
};


enum I82720_GDC_STATUS

{

	/* Status register flags */

	GDC_DATA_READY	 = 0x01,	/* Data ready			*/

	GDC_FIFO_FULL	 = 0x02,	/* FIFO full			*/

	GDC_FIFO_EMPTY	 = 0x04,	/* FIFO empty			*/

	GDC_DRAWING	 = 0x08,	/* Drawing in progress		*/

	GDC_DMA_EXEC	 = 0x10,	/* DMA execute			*/

	GDC_VSYNC	 = 0x20,	/* Vertical sync active		*/

	GDC_HBLANK	 = 0x40,	/* Horizontal blanking active	*/

	GDC_LIGHT_PEN	 = 0x80		/* Light pen detect		*/

};

