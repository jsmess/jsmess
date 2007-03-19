#ifndef _res_net_h_
#define _res_net_h_


double compute_resistor_weights(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * weights_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * weights_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * weights_3, int pulldown_3, int pullup_3 );

#define combine_8_weights(tab,w0,w1,w2,w3,w4,w5,w6,w7)	((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5 + tab[6]*w6 + tab[7]*w7) + 0.5))
#define combine_7_weights(tab,w0,w1,w2,w3,w4,w5,w6)		((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5 + tab[6]*w6) + 0.5))
#define combine_6_weights(tab,w0,w1,w2,w3,w4,w5)		((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5) + 0.5))
#define combine_5_weights(tab,w0,w1,w2,w3,w4)			((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4) + 0.5))
#define combine_4_weights(tab,w0,w1,w2,w3)				((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3) + 0.5))
#define combine_3_weights(tab,w0,w1,w2)					((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2) + 0.5))
#define combine_2_weights(tab,w0,w1)					((int)((tab[0]*w0 + tab[1]*w1) + 0.5))



/* this should be moved to one of the core files */

#define MAX_NETS 3
#define MAX_RES_PER_NET 18




/* for the open collector outputs PROMs */

double compute_resistor_net_outputs(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * outputs_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * outputs_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * outputs_3, int pulldown_3, int pullup_3 );



#endif /*_res_net_h_*/
