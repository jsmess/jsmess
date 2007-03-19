#ifndef FLT_RC_H
#define FLT_RC_H

/*
signal >--R1--+--R2--+
              |      |
              C      R3---> amp
              |      |
             GND    GND
*/

/* R1, R2, R3 in Ohm; C in pF */
/* set C = 0 to disable the filter */
void filter_rc_set_RC(int num, int R1, int R2, int R3, int C);

#endif
