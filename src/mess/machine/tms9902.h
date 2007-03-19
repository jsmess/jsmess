typedef struct tms9902reset_param
{
	double clock_rate;							/* clock rate (2MHz-3.3MHz, with 4MHz overclocking) */
	void (*int_callback)(int which, int INT);	/* called when interrupt pin state changes */
	void (*rts_callback)(int which, int RTS);	/* called when Request To Send pin state changes */
	void (*brk_callback)(int which, int BRK);	/* called when BReaK state changes */
	void (*xmit_callback)(int which, int data);	/* called when a character is transmitted */
} tms9902reset_param;


void tms9902_init(int which, const tms9902reset_param *param);
void tms9902_cleanup(int which);

void tms9902_set_cts(int which, int state);
void tms9902_set_dsr(int which, int state);

void tms9902_push_data(int which, int data);

int tms9902_cru_r(int which, int offset);
void tms9902_cru_w(int which, int offset, int data);

/*********************** Standard 8-bit CPU interfaces *********************/

 READ8_HANDLER ( tms9902_0_cru_r );
WRITE8_HANDLER ( tms9902_0_cru_w );
