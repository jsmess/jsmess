#ifndef MM58274C_H
#define MM58274C_H


extern void mm58274c_init(running_machine *machine, int which, int mode24);
extern int mm58274c_r(int which, int offset);
extern void mm58274c_w(int which, int offset, int data);

#ifdef UNUSED_FUNCTION
extern  READ8_HANDLER(mm58274c_0_r);
extern WRITE8_HANDLER(mm58274c_0_w);

extern  READ8_HANDLER(mm58274c_1_r);
extern WRITE8_HANDLER(mm58274c_1_w);
#endif


#endif /* MM58274C_H */
