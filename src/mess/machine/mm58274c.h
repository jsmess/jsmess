#ifndef MM58274C_H
#define MM58274C_H


extern void mm58274c_init(running_machine *machine, int which, int mode24);
extern int mm58274c_r(int which, int offset);
extern void mm58274c_w(int which, int offset, int data);

#endif /* MM58274C_H */
