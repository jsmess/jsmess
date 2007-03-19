#ifndef _RSP_H
#define _RSP_H

void rsp_get_info(UINT32 state, cpuinfo *info);

#ifdef MAME_DEBUG
extern offs_t rsp_dasm_one(char *buffer, offs_t pc, UINT32 op);
#endif

#endif
