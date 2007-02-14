#define M	program_read_byte
#define MW	program_write_byte

#define D	cdp1802.d
#define DF	cdp1802.df
#define P	cdp1802.p
#define X	cdp1802.x
#define Q	cdp1802.q
#define I	cdp1802.i
#define N	cdp1802.n
#define T	cdp1802.t
#define IE	cdp1802.ie

#define R(index)	cdp1802.reg[index].w.l
#define R0(index)	cdp1802.reg[index].b.l
#define R1(index)	cdp1802.reg[index].b.h

#define PC			R(P)


//#define LOG_ICODE
#ifdef LOG_ICODE
static void cdp1802_log_icode(void)
{
	// logs the i-code
	UINT16 adr = R(5);
	UINT8 u = M(adr), u2 = M(adr + 1);
	UINT16 i = (u << 8) | u2;

	switch (u&0xf0) {
	case 0:
		logerror("chip 8 icode %.4x: %.4x call 1802 %.3x\n",adr, i, i&0xfff);
		break;
	case 0x10:
		logerror("chip 8 icode %.4x: %.4x jmp %.3x\n",adr, i, i&0xfff);
		break;
	case 0x20:
		logerror("chip 8 icode %.4x: %.4x call %.3x\n",adr, i, i&0xfff);
		break;
	case 0x30:
		logerror("chip 8 icode %.4x: %.4x jump if r%x !=0  %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x40:
		logerror("chip 8 icode %.4x: %.4x jump if r%x ==0  %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x50:
		logerror("chip 8 icode %.4x: %.4x skip if r%x!=%.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x60:
		logerror("chip 8 icode %.4x: %.4x load r%x,%.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x70:
		if (u!=0x70)
			logerror("chip 8 icode %.4x: %.4x add r%x,%.2x\n",adr, i,
					 (i&0xf00)>>8,i&0xff);
		else
			logerror("chip 8 icode %.4x: %.4x dec r0, jump if not zero %.2x\n",adr, i,
					 i&0xff);
		break;
	case 0x80:
		switch (u2&0xf) {
		case 0:
			logerror("chip 8 icode %.4x: %.4x r%x:=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 1:
			logerror("chip 8 icode %.4x: %.4x r%x|=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 2:
			logerror("chip 8 icode %.4x: %.4x r%x&=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 3:
			logerror("chip 8 icode %.4x: %.4x r%x^=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 4:
			logerror("chip 8 icode %.4x: %.4x r%x+=r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
				break;
		case 5:
			logerror("chip 8 icode %.4x: %.4x r%x=r%x-r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4,(i&0xf00)>>8);
			break;
		case 6:
			logerror("chip 8 icode %.4x: %.4x r%x>>=1, rb LSB\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 7:
			logerror("chip 8 icode %.4x: %.4x r%x-=r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 0xe:
			logerror("chip 8 icode %.4x: %.4x r%x<<=1, rb MSB\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		default:
			logerror("chip 8 i-code %.4x %.2x %.2x\n",R(5),
					 M(R(5)),
					 M(R(5)+1));
		}
		break;
	case 0x90:
		switch (u2&0xf) {
		case 0:
			logerror("chip 8 icode %.4x: %.4x skip if r%x!=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 1:case 3: case 5: case 7: case 9: case 0xb: case 0xd: case 0xf:
			logerror("chip 8 icode %.4x: %.4x r%x:=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 2:case 6: case 0xa: case 0xe:
			logerror("chip 8 icode %.4x: %.4x r%x:=Memory[r%x]\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 4:case 0xc:
			logerror("chip 8 icode %.4x: %.4x Memory[r%x]\n:=r%x",adr, i,
					 (i&0xf0)>>4,(i&0xf00)>>8);
			break;
		case 8:
			logerror("chip 8 icode %.4x: %.4x Memory[r%x]\n:=BCD(r%x), r%x+=3",adr, i,
					 (i&0xf0)>>4,(i&0xf00)>>8,(i&0xf0)>>4);
			break;
		default:
			logerror("chip 8 i-code %.4x %.2x %.2x\n",R(5),
					 M(R(5)),
					 M(R(5)+1));
		}
		break;
	case 0xa0:
		logerror("chip 8 icode %.4x: %.4x index register:=%.3x\n",adr, i,
				 i&0xfff);
		break;
	case 0xb0:
		logerror("chip 8 icode %.4x: %.4x store %.2x at index register,  add %x to index register\n",adr, i,
				 i&0xff,(i&0xf00)>>8);
		break;
	case 0xc0:
		if (u==0xc0)
			logerror("chip 8 icode %.4x: %.4x return from subroutine\n",adr, i);
		else
			logerror("chip 8 icode %.4x: %.4x r%x:=random&%.2x\n",adr, i,
					 (i&0xf00)>>8,i&0xff);
		break;
	case 0xd0:
		logerror("chip 8 icode %.4x: %.4x if key %x goto %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	default:
		logerror("chip 8 i-code %.4x %.2x %.2x\n",R(5),
				 M(R(5)),
				 M(R(5)+1));
	}
}
#endif

INLINE void cdp1802_add(UINT8 data)
{
	int i;

	i = D + data;
	D = i & 0xff;
	DF = i & 0x100;
}

INLINE void cdp1802_add_carry(UINT8 data)
{
	int i;

	if (DF)
		i = D + data + 1;
	else
		i = D + data;

	D = i & 0xff;
	DF = i & 0x100;
}

INLINE void cdp1802_sub(UINT8 left, UINT8 right)
{
	int i;

	i = left - right;

	D = i & 0xff;
	DF = (i >= 0);
}

INLINE void cdp1802_sub_carry(UINT8 left, UINT8 right)
{
	int i;

	if (DF)
		i = left - right;
	else
		i = left - right - 1;

	D = i & 0xff;
	DF = (i >= 0);
}

INLINE UINT16 cdp1802_read_operand_word(void)
{
	UINT16 a = cpu_readop(PC++) << 8;
	a |= cpu_readop(PC++);
	return a;
}

INLINE void cdp1802_long_branch(int flag)
{
	UINT16 new = cdp1802_read_operand_word();
	if (flag) PC = new;
}

INLINE void cdp1802_long_skip(int flag)
{
	if (flag) PC += 2;
}

INLINE void cdp1802_short_branch(int flag)
{
	UINT8 new = cpu_readop(PC++);
	if (flag) R0(P) = new;
}

INLINE void cdp1802_short_branch_ef(int flag, int mask)
{
	int b = 0;
	UINT8 new = cpu_readop(PC++);

	if (cdp1802.config && cdp1802.config->in_ef)
		b = cdp1802.config->in_ef() & mask ? 1 : 0;

	if (flag == b) R0(P) = new;
}

INLINE void cdp1802_q(int level)
{
	Q = level;

	if (cdp1802.config && cdp1802.config->out_q)
		cdp1802.config->out_q(level);
}

INLINE void cdp1802_read_px(void)
{
	UINT8 i = M(R(X)++);

	P = i & 0xf;
	X = i >> 4;
	change_pc(PC);
}

INLINE void cdp1802_out_n(int n)
{
	UINT8 i = M(R(X));
	io_write_byte(n, i);
	R(X)++;
}

INLINE void cdp1802_in_n(int n)
{
	UINT8 i = io_read_byte(n);

	D = i;
	MW(R(X), i);
}

static void cdp1802_instruction(void)
{
	int oper;
	int b;

#ifdef LOG_ICODE
	if (PC == 0x6b) cdp1802_log_icode(); // if you want to have the icodes in the debuglog
#endif

	oper = cpu_readop(PC++);

	I = (oper & 0xf0) >> 4;
	N = oper & 0x0f;

	switch (oper & 0xf0)
	{
	case 0:
		if (oper == 0)
			cdp1802.idle = 1;
		else
			D = M(R(N));
		break;
	case 0x10: R(N)++;					break;
	case 0x20: R(N)--;					break;
	case 0x40: D = M(R(N)++);			break;
	case 0x50: MW(R(N), D);				break;
	case 0x80: D = R0(N);				break;
	case 0x90: D = R1(N);				break;
	case 0xa0: R0(N) = D;				break;
	case 0xb0: R1(N) = D;				break;
	case 0xd0: P = N; change_pc(PC);	break;
	case 0xe0: X = N;					break;
	default:
		switch (oper & 0xf8)
		{
		case 0x60:
			if (oper == 0x60)
				R(X)++;
			else
				cdp1802_out_n(oper & 7);
			break;
		case 0x68:
			/*

                A note about INP 0 (0x68) from Tom Pittman's "A Short Course in Programming":

                If you look carefully, you will notice that we never studied the opcode "68".
                That's because it is not a defined 1802 instruction. It has the form of an INP
                instruction, but 0 is not a defined input port, so if you execute it (try it!)
                nothing is input. "Nothing" is the answer to a question; it is data, and something
                will be put in the accumulator and memory (so now you know what the computer uses
                to mean "nothing").

                However, since the result of the "68" opcode is unpredictable, it should not be
                used in your programs. In fact, "68" is the first byte of a series of additional
                instructions for the 1804 and 1805 microprocessors.

                http://www.ittybittycomputers.com/IttyBitty/ShortCor.htm

            */
			cdp1802_in_n(oper & 7);
			break;
		default:
			switch (oper)
			{
			case 0x30: cdp1802_short_branch(1);			break;
			case 0x31: cdp1802_short_branch(Q);			break;
			case 0x32: cdp1802_short_branch(D == 0);	break;
			case 0x33: cdp1802_short_branch(DF);		break;

			case 0x34: cdp1802_short_branch_ef(1, EF1);	break;
			case 0x35: cdp1802_short_branch_ef(1, EF2);	break;
			case 0x36: cdp1802_short_branch_ef(1, EF3);	break;
			case 0x37: cdp1802_short_branch_ef(1, EF4);	break;

			case 0x38: cdp1802_short_branch(0);			break;
			case 0x39: cdp1802_short_branch(!Q);		break;
			case 0x3a: cdp1802_short_branch(D != 0);	break;
			case 0x3b: cdp1802_short_branch(!DF);		break;

			case 0x3c: cdp1802_short_branch_ef(0, EF1);	break;
			case 0x3d: cdp1802_short_branch_ef(0, EF2);	break;
			case 0x3e: cdp1802_short_branch_ef(0, EF3);	break;
			case 0x3f: cdp1802_short_branch_ef(0, EF4);	break;

			case 0x70: cdp1802_read_px(); IE = 1;		break;
			case 0x71: cdp1802_read_px(); IE = 0;		break;

			case 0x72: D = M(R(X)++);					break;
			case 0x73: MW(R(X)--, D);					break;

			case 0x74: cdp1802_add_carry(M(R(X)));		break;
			case 0x75: cdp1802_sub_carry(M(R(X)), D);	break;

			case 0x76:
				b = DF;
				DF = D & 1;
				D >>= 1;
				if (b) D |= 0x80;
				break;

			case 0x77: cdp1802_sub_carry(D, M(R(X)));	break;
			case 0x78: MW(R(X), T);						break;

			case 0x79:
				T = (X << 4) | P;
				MW(R(2), T);
				X = P;
				R(2)--;
				break;

			case 0x7a: cdp1802_q(0);					break;
			case 0x7b: cdp1802_q(1);					break;

			case 0x7c: cdp1802_add_carry(M(PC++));		break;
			case 0x7d: cdp1802_sub_carry(M(PC++), D);	break;

			case 0x7e:
				b = DF;
				DF = D & 0x80;
				D <<= 1;
				if (b) D |= 1;
				break;

			case 0x7f: cdp1802_sub_carry(D, M(PC++));	break;

			case 0xc0: cdp1802_long_branch(1);		cdp1802_ICount -= 1; break;
			case 0xc1: cdp1802_long_branch(Q);		cdp1802_ICount -= 1; break;
			case 0xc2: cdp1802_long_branch(D == 0);	cdp1802_ICount -= 1; break;
			case 0xc3: cdp1802_long_branch(DF);		cdp1802_ICount -= 1; break;

			case 0xc4: /* NOP */ cdp1802_ICount -= 1;	break;

			case 0xc5: cdp1802_long_skip(!Q);		cdp1802_ICount -= 1; break;
			case 0xc6: cdp1802_long_skip(D != 0);	cdp1802_ICount -= 1; break;
			case 0xc7: cdp1802_long_skip(!DF);		cdp1802_ICount -= 1; break;
			case 0xc8: cdp1802_long_skip(1);		cdp1802_ICount -= 1; break;

			case 0xc9: cdp1802_long_branch(!Q);		cdp1802_ICount -= 1; break;
			case 0xca: cdp1802_long_branch(D != 0); cdp1802_ICount -= 1; break;
			case 0xcb: cdp1802_long_branch(!DF);	cdp1802_ICount -= 1; break;

			case 0xcc: cdp1802_long_skip(IE);		cdp1802_ICount -= 1; break;
			case 0xcd: cdp1802_long_skip(Q);		cdp1802_ICount -= 1; break;
			case 0xce: cdp1802_long_skip(D == 0);	cdp1802_ICount -= 1; break;
			case 0xcf: cdp1802_long_skip(DF);		cdp1802_ICount -= 1; break;

			case 0xf0: D = M(R(X));						break;
			case 0xf1: D |= M(R(X));					break;
			case 0xf2: D &= M(R(X));					break;
			case 0xf3: D ^= M(R(X));					break;

			case 0xf4: cdp1802_add(M(R(X)));			break;
			case 0xf5: cdp1802_sub(M(R(X)), D);			break;
			case 0xf6: DF = D & 1; D >>= 1;				break;
			case 0xf7: cdp1802_sub(D, M(R(X)));			break;

			case 0xf8: D = M(PC++);						break;
			case 0xf9: D |= M(PC++);					break;
			case 0xfa: D &= M(PC++);					break;
			case 0xfb: D ^= M(PC++);					break;

			case 0xfc: cdp1802_add(M(PC++));			break;
			case 0xfd: cdp1802_sub(M(PC++), D);			break;
			case 0xfe: DF = D & 0x80; D <<= 1;			break;
			case 0xff: cdp1802_sub(D, M(PC++));			break;

			default:
				logerror("cpu cdp1802 unknown opcode %.2x at %.4x\n",oper, PC-1);
				break;
			}
			break;
		}
		break;
	}

	cdp1802_ICount -= 2;
}
