/***************************************************************************

    NeXT

    TODO:

    - SCC
    - mouse
    - network
    - optical drive
    - floppy
    - hdd

    Memory map and other bits can be found here:
    http://fxr.watson.org/fxr/source/arch/next68k/include/cpu.h?v=NETBSD5#L366

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "includes/next.h"
#include "formats/pc_dsk.h"
#include "formats/mfi_dsk.h"

UINT32 next_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT32 count;
	int x,y,xi;

	count = 0;

	for(y=0;y<832;y++)
	{
		for(x=0;x<1152;x+=16)
		{
			for(xi=0;xi<16;xi++)
			{
				UINT8 pen = (vram[count] >> (30-(xi*2))) & 0x3;

				pen ^= 3;

				if(cliprect.contains(x+xi, y))
					bitmap.pix16(y, x+xi) = screen.machine().pens[pen];
			}

			count++;
		}
	}

    return 0;
}

/* Dummy catcher for any unknown r/w */
READ8_MEMBER( next_state::io_r )
{
	if(!space.debugger_access())
		printf("io_r %08x (%08x)\n",offset+0x02000000, cpu_get_pc(&space.device()));

	if(offset == 0xc0)
		return 0;

	return 0xff;
}

WRITE8_MEMBER( next_state::io_w )
{
	if(!space.debugger_access())
		printf("io_w %08x, %02x (%08x)\n",offset+0x02000000,data, cpu_get_pc(&space.device()));
}

/* map ROM at 0x01000000-0x0101ffff? */
READ32_MEMBER( next_state::rom_map_r )
{
	if(0 && !space.debugger_access())
		printf("%08x ROM MAP?\n",cpu_get_pc(&space.device()));
	return 0x01000000;
}

READ32_MEMBER( next_state::scr2_r )
{
	if(0 && !space.debugger_access())
		printf("%08x\n",cpu_get_pc(&space.device()));
	/*
    x--- ---- ---- ---- ---- ---- ---- ---- dsp reset
    -x-- ---- ---- ---- ---- ---- ---- ---- dsp block end
    --x- ---- ---- ---- ---- ---- ---- ---- dsp unpacked
    ---x ---- ---- ---- ---- ---- ---- ---- dsp mode b
    ---- x--- ---- ---- ---- ---- ---- ---- dsp mode a
    ---- -x-- ---- ---- ---- ---- ---- ---- remote int
    ---- ---x ---- ---- ---- ---- ---- ---- local int
    ---- ---- ---x ---- ---- ---- ---- ---- dram 256k
    ---- ---- ---- ---x ---- ---- ---- ---- dram 1m
    ---- ---- ---- ---- x--- ---- ---- ---- "timer on ipl7"
    ---- ---- ---- ---- -xxx ---- ---- ---- rom waitstates
    ---- ---- ---- ---- ---- x--- ---- ---- ROM 1M
    ---- ---- ---- ---- ---- -x-- ---- ---- MCS1850 rtdata
    ---- ---- ---- ---- ---- --x- ---- ---- MCS1850 rtclk
    ---- ---- ---- ---- ---- ---x ---- ---- MCS1850 rtce
    ---- ---- ---- ---- ---- ---- x--- ---- rom overlay
    ---- ---- ---- ---- ---- ---- -x-- ---- dsp ie
    ---- ---- ---- ---- ---- ---- --x- ---- mem en
    ---- ---- ---- ---- ---- ---- ---- ---x led

    68040-25, 100ns, 32M: 00000c80
    68040-25, 100ns, 20M: 00ff0c80

    */

	UINT32 data = scr2 & 0xfffffbff;

	data |= rtc->sdo_r() << 10;

	return data;
}

WRITE32_MEMBER( next_state::scr2_w )
{
	if(0 && !space.debugger_access())
		printf("scr2_w %08x (%08x)\n", data, cpu_get_pc(&space.device()));
	COMBINE_DATA(&scr2);

	rtc->ce_w(BIT(scr2, 8));
	rtc->sdi_w(BIT(scr2, 10));
	rtc->sck_w(BIT(scr2, 9));
}

READ32_MEMBER( next_state::scr1_r )
{
	/*
        xxxx ---- ---- ---- ---- ---- ---- ---- slot ID
        ---- ---- xxxx xxxx ---- ---- ---- ---- DMA type
        ---- ---- ---- ---- xxxx ---- ---- ---- cpu type
        ---- ---- ---- ---- ---- xxxx ---- ---- board revision
        ---- ---- ---- ---- ---- ---- -xx- ---- video mem speed
        ---- ---- ---- ---- ---- ---- ---x x--- mem speed
        ---- ---- ---- ---- ---- ---- ---- -xxx cpu speed

        68040-25: 00011102
        68040-25: 00013002 (non-turbo, color)
    */

	return 0x00012002; // TODO
}

// Interrupt subsystem
// source    bit   level
// nmi       31    7
// pfail     30    7
// timer     29    6
// enetxdma  28    6
// enetrdma  27    6
// scsidma   26    6
// diskdma   25    6
// prndma    24    6
// sndoutdma 23    6
// sndindma  22    6
// sccdma    21    6
// dspdma    20    6
// m2rdma    19    6
// r2mdma    18    6
// scc       17    5
// remote    16    5
// bus       15    5
// dsp4      14    4
// disk      13    3
// scsi      12    3
// printer   11    3
// enetx     10    3
// enetr      9    3
// soundovr   8    3
// phone      7    3 -- floppy
// dsp3       6    3
// video      5    3
// monitor    4    3
// kbdmouse   3    3
// power      2    3
// softint1   1    2
// softint0   0    1

void next_state::irq_set(int id, bool raise)
{
	UINT32 mask = 1U << id;
	if(raise)
		irq_status |= mask;
	else
		irq_status &= ~mask;
	irq_check();
}


READ32_MEMBER( next_state::irq_status_r )
{
	return irq_status;
}

READ32_MEMBER( next_state::irq_mask_r )
{
	return irq_mask;
}

WRITE32_MEMBER( next_state::irq_mask_w )
{
	COMBINE_DATA(&irq_mask);
	irq_check();
}

void next_state::irq_check()
{
	UINT32 act = irq_status & irq_mask;
	int bit;
	for(bit=31; bit >= 0 && !(act & (1U << bit)); bit--);

	int level;
	if     (bit <  0) level = 0;
	else if(bit <  1) level = 1;
	else if(bit <  2) level = 2;
	else if(bit < 14) level = 3;
	else if(bit < 15) level = 4;
	else if(bit < 18) level = 5;
	else if(bit < 30) level = 6;
	else              level = 7;

	fprintf(stderr, "IRQ info %08x/%08x - %d\n", irq_status, irq_mask, level);

	if(level != irq_level) {
		maincpu->set_input_line(irq_level, CLEAR_LINE);
		maincpu->set_input_line_and_vector(level, ASSERT_LINE, M68K_INT_ACK_AUTOVECTOR);
		irq_level = level;
	}
}

const char *next_state::dma_targets[0x20] = {
	0, "scsi", 0, 0, "soundout", "disk", 0, 0,
	"soundin", "printer", 0, 0, "scc", "dsp", 0, 0,
	"s-enetx", "enetx", 0, 0, "s-enetr", "enetr", 0, 0,
	"video", 0, 0, 0, "r2m", "m2r", 0, 0
};

const int next_state::dma_irqs[0x20] = {
	-1, 26, -1, -1, 23, 25, -1, -1, 22, 24, -1, -1, 21, 20, -1, -1,
	-1, 28, -1, -1, -1, 27, -1, -1, -2, -1, -1, -1, 18, 19, -1, -1
};

const char *next_state::dma_name(int slot)
{
	static char buf[32];
	if(dma_targets[slot])
		return dma_targets[slot];
	sprintf(buf, "<%02x>", slot);
	return buf;
}

void next_state::dma_drq_w(int slot, bool state)
{
	//  fprintf(stderr, "DMA drq_w %d, %d\n", slot, state);
	dma_slot &ds = dma_slots[slot];
	ds.drq = state;
	if(state && (ds.state & DMA_ENABLE)) {
		address_space *space = maincpu->memory().space(AS_PROGRAM);
		if(ds.state & DMA_READ) {
			while(ds.drq) {
				UINT8 val;
				bool eof;
				bool err;
				dma_read(slot, val, eof, err);
				if(err) {
					ds.state = (ds.state & ~DMA_ENABLE) | DMA_BUSEXC;
					fprintf(stderr, "DMA: bus error on read slot %d\n", slot);
					return;
				}
				if(0)
					fprintf(stderr, "DMA [%08x] = %02x\n", ds.current, val);
				space->write_byte(ds.current++, val);
				dma_check_end(slot, eof);
				if(!(ds.state & DMA_ENABLE))
					return;
			}
		} else {
			while(ds.drq) {
				UINT8 val = space->read_byte(ds.current++);
				bool eof = ds.current == (ds.limit & 0x7fffffff) && (ds.limit & 0x80000000);
				bool err;
				dma_write(slot, val, eof, err);
				if(err) {
					ds.state = (ds.state & ~DMA_ENABLE) | DMA_BUSEXC;
					fprintf(stderr, "DMA: bus error on write slot %d\n", slot);
					return;
				}
				dma_check_end(slot, false);
				if(!(ds.state & DMA_ENABLE))
					return;
			}
		}
	}
}

void next_state::dma_read(int slot, UINT8 &val, bool &eof, bool &err)
{
	err = false;
	eof = false;
	switch(slot) {
	case 1:
		val = fdc->dma_r();
		break;

	case 5:
		val = mo->dma_r();
		break;

	case 21:
		net->rx_dma_r(val, eof);
		break;

	default:
		err = true;
		val = 0;
		break;
	}
}

void next_state::dma_write(int slot, UINT8 data, bool eof, bool &err)
{
	err = false;
	switch(slot) {
	case 4:
		break;

	case 5:
		mo->dma_w(data);
		break;

	case 17:
		net->tx_dma_w(data, eof);
		break;

	default:
		err = true;
		break;
	}
}

void next_state::dma_end(int slot)
{
	dma_slot &ds = dma_slots[slot];
	if(ds.supdate) {
		dma_slot &ds1 = dma_slots[(slot-1) & 31];
		ds1.start = ds.start;
		ds1.limit = ds.current;
		ds.start = ds.chain_start;
		ds.limit = ds.chain_limit;
		ds.current = ds.start;
		ds.supdate = false;
	} else
		ds.state &= ~DMA_ENABLE;
	ds.state |= DMA_COMPLETE;
	if(dma_irqs[slot] >= 0)
		irq_set(dma_irqs[slot], true);
}

void next_state::dma_check_end(int slot, bool eof)
{
	dma_slot &ds = dma_slots[slot];
	if(eof || ds.current == (ds.limit & 0x7fffffff))
		dma_end(slot);
}

READ32_MEMBER( next_state::dma_regs_r)
{
	int slot = offset >> 2;
	int reg = offset & 3;

	const char *name = dma_name(slot);

	fprintf(stderr, "dma_regs_r %s:%d (%08x)\n", name, reg, cpu_get_pc(&space.device()));
	switch(reg) {
	case 0:
		return dma_slots[slot].start;
	case 1:
		return dma_slots[slot].limit;
	case 2:
		return dma_slots[slot].chain_start;
	case 3: default:
		return dma_slots[slot].chain_limit;
	}
}

WRITE32_MEMBER( next_state::dma_regs_w)
{
	int slot = offset >> 2;
	int reg = offset & 3;

	const char *name = dma_name(slot);

	fprintf(stderr, "dma_regs_w %s:%d %08x (%08x)\n", name, reg, data, cpu_get_pc(&space.device()));
	switch(reg) {
	case 0:
		dma_slots[slot].start = data;
		dma_slots[slot].current = data;
		break;
	case 1:
		dma_slots[slot].limit = data;
		break;
	case 2:
		dma_slots[slot].chain_start = data;
		break;
	case 3:
		dma_slots[slot].chain_limit = data;
		break;
	}
}

READ32_MEMBER( next_state::dma_ctrl_r)
{
	int slot = offset >> 2;
	int reg = offset & 3;

	const char *name = dma_name(slot);

	fprintf(stderr, "dma_ctrl_r %s:%d %02x (%08x)\n", name, reg, dma_slots[slot].state, cpu_get_pc(&space.device()));

	return reg ? 0 : dma_slots[slot].state << 24;
}

WRITE32_MEMBER( next_state::dma_ctrl_w)
{
	int slot = offset >> 2;
	int reg = offset & 3;
	const char *name = dma_name(slot);
	fprintf(stderr, "dma_ctrl_w %s:%d %08x @ %08x (%08x)\n", name, reg, data, mem_mask, cpu_get_pc(&space.device()));
	if(!reg) {
		if(ACCESSING_BITS_16_23)
			dma_do_ctrl_w(slot, data >> 16);
		else if(ACCESSING_BITS_24_31)
			dma_do_ctrl_w(slot, data >> 24);
	}
}

WRITE32_MEMBER( next_state::dma_040_ctrl_w)
{
	int slot = offset >> 2;
	int reg = offset & 3;
	if(!reg && ACCESSING_BITS_16_23)
		dma_do_ctrl_w(slot, data >> 16);
}

void next_state::dma_do_ctrl_w(int slot, UINT8 data)
{
	const char *name = dma_name(slot);
#if 0
	fprintf(stderr, "dma_ctrl_w %s %02x (%08x)\n", name, data, cpu_get_pc(maincpu));

	fprintf(stderr, "  ->%s%s%s%s%s%s%s\n",
			data & DMA_SETENABLE ? " enable" : "",
			data & DMA_SETSUPDATE ? " supdate" : "",
			data & DMA_SETREAD ? " read" : "",
			data & DMA_CLRCOMPLETE ? " complete" : "",
			data & DMA_RESET ? " reset" : "",
			data & DMA_INITBUF ? " initbuf" : "",
			data & DMA_INITBUFTURBO ? " initbufturbo" : "");
#endif
	if(data & DMA_SETENABLE)
		fprintf(stderr, "dma enable %s %s %08x (%08x)\n", name, data & DMA_SETREAD ? "read" : "write", (dma_slots[slot].limit-dma_slots[slot].start) & 0x7fffffff, cpu_get_pc(maincpu));

	dma_slot &ds = dma_slots[slot];
	if(data & (DMA_RESET|DMA_INITBUF|DMA_INITBUFTURBO)) {
		ds.state = 0;
		if(dma_irqs[slot] >= 0)
			irq_set(dma_irqs[slot], false);
	}
	if(data & DMA_SETSUPDATE) {
		ds.state |= DMA_SUPDATE;
		ds.supdate = true;
	}
	if(data & DMA_SETREAD)
		ds.state |= DMA_READ;
	if(data & DMA_CLRCOMPLETE) {
		ds.state &= ~DMA_COMPLETE;
		if(dma_irqs[slot] >= 0)
			irq_set(dma_irqs[slot], false);
	}
	if(data & DMA_SETENABLE) {
		ds.state |= DMA_ENABLE;
		//      fprintf(stderr, "dma slot %d drq=%s\n", slot, ds.drq ? "on" : "off");
		if(ds.drq)
			dma_drq_w(slot, ds.drq);
	}
}

const int next_state::scsi_clocks[4] = { 10000000, 12000000, 20000000, 16000000 };

READ32_MEMBER( next_state::scsictrl_r )
{
	fprintf(stderr, "read %08x\n", cpu_get_pc(&space.device()));
	return (scsictrl << 24) | (scsistat << 16);
}

WRITE32_MEMBER( next_state::scsictrl_w )
{
	if(ACCESSING_BITS_24_31) {
		scsictrl = data >> 24;
		if(scsictrl & 0x02)
			scsi->reset();
		device_t::static_set_clock(*scsi, scsi_clocks[scsictrl >> 6]);

		fprintf(stderr, "SCSIctrl %dMHz int=%s dma=%s dmadir=%s%s%s dest=%s (%08x)\n",
				scsi_clocks[scsictrl >> 6]/1000000,
				scsictrl & 0x20 ? "on" : "off",
				scsictrl & 0x10 ? "on" : "off",
				scsictrl & 0x08 ? "read" : "write",
				scsictrl & 0x04 ? " flush" : "",
				scsictrl & 0x02 ? " reset" : "",
				scsictrl & 0x01 ? "wd3392" : "ncr5390",
				cpu_get_pc(&space.device()));
	}
	if(ACCESSING_BITS_16_23) {
		scsistat = data >> 16;
		fprintf(stderr, "SCSIstat %02x (%08x)\n", data, cpu_get_pc(&space.device()));
	}
}

READ32_MEMBER( next_state::event_counter_r)
{
	// Event counters, around that time, are usually fixed-frequency counters.
	// This one being 1MHz seems to make sense

	// The nexttrb rom seems pretty convinced that it's 20 bits only.

	if(ACCESSING_BITS_24_31)
		eventc_latch = machine().time().as_ticks(1000000) & 0xfffff;
	return eventc_latch;
}

READ32_MEMBER( next_state::dsp_r)
{
	return 0x7fffffff;
}

WRITE32_MEMBER( next_state::fdc_control_w )
{
	fprintf(stderr, "FDC write %02x (%08x)\n", data >> 24, cpu_get_pc(&space.device()));
}

READ32_MEMBER( next_state::fdc_control_r )
{
	// Type of floppy present?
	// Seems to behave identically as long as b24-25 is non-zero
	// Zero means no floppy

	fprintf(stderr, "FDC read (%08x)\n", cpu_get_pc(&space.device()));
	return 0 << 24;
}

READ32_MEMBER( next_state::phy_r )
{
	logerror("phy_r %d %08x (%08x)\n", offset, phy[offset], cpu_get_pc(&space.device()));
	return phy[offset] | (0 << 24);
}

WRITE32_MEMBER( next_state::phy_w )
{
	COMBINE_DATA(phy+offset);
	logerror("phy_w %d %08x (%08x)\n", offset, phy[offset], cpu_get_pc(&space.device()));
}

void next_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	irq_set(29, true);
}

READ32_MEMBER( next_state::timer_data_r )
{
	if(timer_ctrl & 0x80000000)
		timer_update();
	return timer_data;
}

WRITE32_MEMBER( next_state::timer_data_w )
{
	if(timer_ctrl & 0x80000000)
		timer_update();
	COMBINE_DATA(&timer_data);
	timer_data &= 0xffff0000;
	if(timer_ctrl & 0x80000000)
		timer_start();
}

READ32_MEMBER( next_state::timer_ctrl_r )
{
	irq_set(29, false);
	return timer_ctrl;
}

WRITE32_MEMBER( next_state::timer_ctrl_w )
{
	bool oldact = timer_ctrl & 0x80000000;
	COMBINE_DATA(&timer_ctrl);
	bool newact = timer_ctrl & 0x80000000;
	if(oldact != newact) {
		if(oldact) {
			timer_update();
			irq_set(29, false);
		} else
			timer_start();
	}
}

void next_state::timer_update()
{
	int delta = timer_vbase - (machine().time() - timer_tbase).as_ticks(1000000);
	if(delta < 0)
		delta = 0;
	timer_data = delta << 16;
}

void next_state::timer_start()
{
	timer_tbase = machine().time();
	timer_vbase = timer_data >> 16;
	timer_tm->adjust(attotime::from_usec(timer_vbase));
}

void next_state::scc_irq(int state)
{
	irq_set(17, state);
}

void next_state::keyboard_irq(bool state)
{
	irq_set(3, state);
}

void next_state::fdc_irq(bool state)
{
	irq_set(7, state);
}

void next_state::fdc_drq(bool state)
{
	dma_drq_w(1, state);
}

void next_state::net_tx_irq(bool state)
{
	irq_set(10, state);
}

void next_state::net_rx_irq(bool state)
{
	irq_set(9, state);
}

void next_state::net_tx_drq(bool state)
{
	dma_drq_w(17, state);
}

void next_state::net_rx_drq(bool state)
{
	dma_drq_w(21, state);
}

void next_state::mo_irq(bool state)
{
	irq_set(13, state);
}

void next_state::mo_drq(bool state)
{
	dma_drq_w(5, state);
}

void next_state::scsi_irq(bool state)
{
	irq_set(12, state);
}

void next_state::scsi_drq(bool state)
{
	dma_drq_w(26, state);
}

void next_state::machine_start()
{
	save_item(NAME(scr2));
	save_item(NAME(irq_status));
	save_item(NAME(irq_mask));
	save_item(NAME(irq_level));
	save_item(NAME(phy));
	save_item(NAME(scsictrl));
	save_item(NAME(timer_tbase));
	save_item(NAME(timer_vbase));
	save_item(NAME(timer_data));
	save_item(NAME(timer_ctrl));
	save_item(NAME(eventc_latch));

	timer_tm = timer_alloc(0);

	if(fdc) {
		fdc->setup_intrq_cb(n82077aa_device::line_cb(FUNC(next_state::fdc_irq), this));
		fdc->setup_drq_cb(n82077aa_device::line_cb(FUNC(next_state::fdc_drq), this));
	}
}

void next_state::machine_reset()
{
	scr2 = 0;
	irq_status = 0;
	irq_mask = 0;
	irq_level = 0;
	esp = 0;
	scsictrl = 0;
	phy[0] = phy[1] = 0;
	eventc_latch = 0;
	timer_vbase = 0;
	timer_data = 0;
	timer_ctrl = 0;
	dma_drq_w(4, true); // soundout
}

static ADDRESS_MAP_START( next_mem, AS_PROGRAM, 32, next_state )
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x01000000, 0x0101ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x02000000, 0x020001ff) AM_MIRROR(0x300200) AM_READWRITE(dma_ctrl_r, dma_ctrl_w)
	AM_RANGE(0x02004000, 0x020041ff) AM_MIRROR(0x300200) AM_READWRITE(dma_regs_r, dma_regs_w)
	AM_RANGE(0x02006000, 0x0200600f) AM_MIRROR(0x300000) AM_DEVICE8("net", mb8795_device, map, 0xffffffff)
//  AM_RANGE(0x02006010, 0x02006013) AM_MIRROR(0x300000) memory timing
	AM_RANGE(0x02007000, 0x02007003) AM_MIRROR(0x300000) AM_READ(irq_status_r)
	AM_RANGE(0x02007800, 0x02007803) AM_MIRROR(0x300000) AM_READWRITE(irq_mask_r,irq_mask_w)
	AM_RANGE(0x02008000, 0x02008003) AM_MIRROR(0x300000) AM_READ(dsp_r)
	AM_RANGE(0x0200c000, 0x0200c003) AM_MIRROR(0x300000) AM_READ(scr1_r)
	AM_RANGE(0x0200c800, 0x0200c803) AM_MIRROR(0x300000) AM_READ(rom_map_r)
	AM_RANGE(0x0200d000, 0x0200d003) AM_MIRROR(0x300000) AM_READWRITE(scr2_r,scr2_w)
//  AM_RANGE(0x0200d800, 0x0200d803) AM_MIRROR(0x300000) RMTINT
	AM_RANGE(0x0200e000, 0x0200e00b) AM_MIRROR(0x300000) AM_DEVICE("keyboard", nextkbd_device, amap)
//  AM_RANGE(0x0200f000, 0x0200f003) AM_MIRROR(0x300000) printer
//  AM_RANGE(0x02010000, 0x02010003) AM_MIRROR(0x300000) brightness
	AM_RANGE(0x02012000, 0x0201201f) AM_MIRROR(0x300000) AM_DEVICE8("mo", nextmo_device, map, 0xffffffff)
	AM_RANGE(0x02014000, 0x0201400f) AM_MIRROR(0x300000) AM_DEVICE8("scsi", ncr5390_device, map, 0xffffffff)
	AM_RANGE(0x02114020, 0x02114023) AM_MIRROR(0x300000) AM_READWRITE(scsictrl_r, scsictrl_w)
	AM_RANGE(0x02016000, 0x02016003) AM_MIRROR(0x300000) AM_READWRITE(timer_data_r, timer_data_w)
	AM_RANGE(0x02016004, 0x02016007) AM_MIRROR(0x300000) AM_READWRITE(timer_ctrl_r, timer_ctrl_w)
	AM_RANGE(0x02018000, 0x02018003) AM_MIRROR(0x300000) AM_DEVREADWRITE8("scc", scc8530_t, reg_r, reg_w, 0xffffffff)
//  AM_RANGE(0x02018004, 0x02018007) AM_MIRROR(0x300000) SCC CLK
//  AM_RANGE(0x02018100, 0x02018103) AM_MIRROR(0x300000) Color RAMDAC
//  AM_RANGE(0x02018104, 0x02018107) AM_MIRROR(0x300000) Color CSR
//  AM_RANGE(0x02018190, 0x02018197) AM_MIRROR(0x300000) warp 9c DRAM timing
//  AM_RANGE(0x02018198, 0x0201819f) AM_MIRROR(0x300000) warp 9c VRAM timing
	AM_RANGE(0x0201a000, 0x0201a003) AM_MIRROR(0x300000) AM_READ(event_counter_r) // EVENTC
//  AM_RANGE(0x020c0000, 0x020c0004) AM_MIRROR(0x300000) BMAP
	AM_RANGE(0x020c0030, 0x020c0037) AM_MIRROR(0x300000) AM_READWRITE(phy_r, phy_w)
//  AM_RANGE(0x02000000, 0x0201ffff) AM_MIRROR(0x300000) AM_READWRITE8(io_r,io_w,0xffffffff) //intentional fall-through
	AM_RANGE(0x04000000, 0x07ffffff) AM_RAM //work ram
	AM_RANGE(0x0b000000, 0x0b03ffff) AM_RAM AM_BASE(vram) //vram
//  AM_RANGE(0x0c000000, 0x0c03ffff) video RAM w A+B-AB function
//  AM_RANGE(0x0d000000, 0x0d03ffff) video RAM w (1-A)B function
//  AM_RANGE(0x0e000000, 0x0e03ffff) video RAM w ceil(A+B) function
//  AM_RANGE(0x0f000000, 0x0f03ffff) video RAM w AB function
//  AM_RANGE(0x10000000, 0x1003ffff) main RAM w A+B-AB function
//  AM_RANGE(0x14000000, 0x1403ffff) main RAM w (1-A)B function
//  AM_RANGE(0x18000000, 0x1803ffff) main RAM w ceil(A+B) function
//  AM_RANGE(0x1c000000, 0x1c03ffff) main RAM w AB function
//  AM_RANGE(0x2c000000, 0x2c1fffff) Color VRAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( next_040_mem, AS_PROGRAM, 32, next_state )
	AM_RANGE(0x02000000, 0x020001ff) AM_MIRROR(0x300000) AM_READWRITE(dma_ctrl_r, dma_040_ctrl_w)
	AM_RANGE(0x02014100, 0x02014107) AM_MIRROR(0x300000) AM_DEVICE8("fdc", n82077aa_device, amap, 0xffffffff)
	AM_RANGE(0x02014108, 0x0201410b) AM_MIRROR(0x300000) AM_READWRITE(fdc_control_r, fdc_control_w)

	AM_IMPORT_FROM(next_mem)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( next )
INPUT_PORTS_END

static PALETTE_INIT(next)
{
	/* TODO: simplify this*/
	palette_set_color(machine, 0,MAKE_RGB(0x00,0x00,0x00));
	palette_set_color(machine, 1,MAKE_RGB(0x55,0x55,0x55));
	palette_set_color(machine, 2,MAKE_RGB(0xaa,0xaa,0xaa));
	palette_set_color(machine, 3,MAKE_RGB(0xff,0xff,0xff));
}

const floppy_format_type next_state::floppy_formats[] = {
	FLOPPY_PC_FORMAT,
	FLOPPY_MFI_FORMAT,
	NULL
};

static SLOT_INTERFACE_START( next_floppies )
	SLOT_INTERFACE( "35hd", FLOPPY_35_HD )
SLOT_INTERFACE_END

const SCSIConfigTable next_state::scsi_devices = {
	2,
	{
		{ SCSI_ID_0, "harddisk0", SCSI_DEVICE_HARDDISK },
		{ SCSI_ID_1, "cdrom",     SCSI_DEVICE_CDROM }
	}
};

const cdrom_interface next_state::cdrom_intf = { NULL, NULL };
const harddisk_interface next_state::harddisk_intf = { NULL, NULL, "next_hdd", NULL };

static MACHINE_CONFIG_START( next, next_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68030, XTAL_25MHz)
    MCFG_CPU_PROGRAM_MAP(next_mem)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(60)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(next_state, screen_update)
    MCFG_SCREEN_SIZE(1120, 832)
    MCFG_SCREEN_VISIBLE_AREA(0, 1120-1, 0, 832-1)

    MCFG_PALETTE_LENGTH(4)
    MCFG_PALETTE_INIT(next)

	// devices
	MCFG_MCCS1850_ADD("rtc", XTAL_32_768kHz,
					  mccs1850_device::cb_t(), mccs1850_device::cb_t(), mccs1850_device::cb_t())
	MCFG_SCC8530_ADD("scc", XTAL_25MHz, scc8530_t::intrq_cb_t(FUNC(next_state::scc_irq), static_cast<next_state *>(owner)))
	MCFG_NEXTKBD_ADD("keyboard", nextkbd_device::int_cb_t(FUNC(next_state::keyboard_irq), static_cast<next_state *>(owner)))
	MCFG_NCR5390_ADD("scsi", 10000000,
					 line_cb_t(FUNC(next_state::scsi_irq), static_cast<next_state *>(owner)),
					 line_cb_t(FUNC(next_state::scsi_drq), static_cast<next_state *>(owner)),
					 next_state::scsi_devices)
	MCFG_MB8795_ADD("net",
					line_cb_t(FUNC(next_state::net_tx_irq), static_cast<next_state *>(owner)),
					line_cb_t(FUNC(next_state::net_rx_irq), static_cast<next_state *>(owner)),
					line_cb_t(FUNC(next_state::net_tx_drq), static_cast<next_state *>(owner)),
					line_cb_t(FUNC(next_state::net_rx_drq), static_cast<next_state *>(owner)))
	MCFG_NEXTMO_ADD("mo",
					nextmo_device::cb_t(FUNC(next_state::mo_irq), static_cast<next_state *>(owner)),
					nextmo_device::cb_t(FUNC(next_state::mo_drq), static_cast<next_state *>(owner)))

	MCFG_CDROM_ADD("cdrom", next_state::cdrom_intf)
	MCFG_HARDDISK_CONFIG_ADD("harddisk0", next_state::harddisk_intf)

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "next")
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( next040, next )
	MCFG_CPU_REPLACE("maincpu", M68040, XTAL_25MHz)
	MCFG_CPU_PROGRAM_MAP(next_040_mem)

	MCFG_N82077AA_ADD("fdc", n82077aa_device::MODE_PS2)
	MCFG_FLOPPY_DRIVE_ADD("fd0", next_floppies, "35hd", 0, next_state::floppy_formats)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( next ) // 68030
    ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v12", "v1.2" ) // version? word at 0xC: 5AD0
	ROMX_LOAD( "rev_1.2.bin",     0x0000, 0x10000, CRC(7070bd78) SHA1(e34418423da61545157e36b084e2068ad41c9e24), ROM_BIOS(1)) /* Label: "(C) 1990 NeXT, Inc. // All Rights Reserved. // Release 1.2 // 1142.02", underlabel exists but unknown */
	ROM_SYSTEM_BIOS( 1, "v10", "v1.0 v41" ) // version? word at 0xC: 3090
	ROMX_LOAD( "rev_1.0_v41.bin", 0x0000, 0x10000, CRC(54df32b9) SHA1(06e3ecf09ab67a571186efd870e6b44028612371), ROM_BIOS(2)) /* Label: "(C) 1989 NeXT, Inc. // All Rights Reserved. // Release 1.0 // 1142.00", underlabel: "MYF // 1.0.41 // 0D5C" */
	ROM_SYSTEM_BIOS( 2, "v10p", "v1.0 prototype?" ) // version? word at 0xC: 23D9
	ROMX_LOAD( "rev_1.0_proto.bin", 0x0000, 0x10000, CRC(F44974F9) SHA1(09EAF9F5D47E379CFA0E4DC377758A97D2869DDC), ROM_BIOS(3)) /* Label: "(C) 1989 NeXT, Inc. // All Rights Reserved. // Release 1.0 // 1142.00", no underlabel */

	ROM_REGION( 0x20, "rtc", 0 )
	ROM_LOAD( "nvram.bin", 0x00, 0x20, CRC(52162469) SHA1(f49888af63d2d2658db4f41b3345cae153f42884) )
ROM_END

ROM_START( nextnt ) // 68040 non-turbo
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v25", "v2.5 v66" ) // version? word at 0xC: F302
	ROMX_LOAD( "rev_2.5_v66.bin", 0x0000, 0x20000, CRC(f47e0bfe) SHA1(b3534796abae238a0111299fc406a9349f7fee24), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v24", "v2.4 v65" ) // version? word at 0xC: A634
	ROMX_LOAD( "rev_2.4_v65.bin", 0x0000, 0x20000, CRC(74e9e541) SHA1(67d195351288e90818336c3a84d55e6a070960d2), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v21a", "v2.1a? v60?" ) // version? word at 0xC: 894C; NOT SURE about correct rom major and v versions of this one!
	ROMX_LOAD( "rev_2.1a_v60.bin", 0x0000, 0x20000, CRC(739D7C07) SHA1(48FFE54CF2038782A92A0850337C5C6213C98571), ROM_BIOS(3)) /* Label: "(C) 1990 NeXT Computer, Inc. // All Rights Reserved. // Release 2.1 // 2918.AB" */
	ROM_SYSTEM_BIOS( 3, "v21", "v2.1 v59" ) // version? word at 0xC: 72FE
	ROMX_LOAD( "rev_2.1_v59.bin", 0x0000, 0x20000, CRC(f20ef956) SHA1(09586c6de1ca73995f8c9b99870ee3cc9990933a), ROM_BIOS(4))
	ROM_SYSTEM_BIOS( 4, "v12", "v1.2 v58" ) // version? word at 0xC: 6372
	ROMX_LOAD( "rev_1.2_v58.bin", 0x0000, 0x20000, CRC(B815B6A4) SHA1(97D8B09D03616E1487E69D26609487486DB28090), ROM_BIOS(5)) /* Label: "V58 // (C) 1990 NeXT, Inc. // All Rights Reserved // Release 1.2 // 1142.02" */

	ROM_REGION( 0x20, "rtc", 0 )
	ROM_LOAD( "nvram-nt.bin", 0x00, 0x20, BAD_DUMP CRC(7cb74196) SHA1(faa2177c2a08a6b76a6242761a9c0a8b64c4da6e) )
ROM_END

ROM_START( nexttrb ) // 68040 turbo
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v33", "v3.3 v74" ) // version? word at 0xC: (12)3456
	ROMX_LOAD( "rev_3.3_v74.bin", 0x0000, 0x20000, CRC(fbc3a2cd) SHA1(a9bef655f26f97562de366e4a33bb462e764c929), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v32", "v3.2 v72" ) // version? word at 0xC: (01)2f31
	ROMX_LOAD( "rev_3.2_v72.bin", 0x0000, 0x20000, CRC(e750184f) SHA1(ccebf03ed090a79c36f761265ead6cd66fb04329), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v30", "v3.0 v70" ) // version? word at 0xC: (01)06e8
	ROMX_LOAD( "rev_3.0_v70.bin", 0x0000, 0x20000, CRC(37250453) SHA1(a7e42bd6a25c61903c8ca113d0b9a624325ee6cf), ROM_BIOS(3))

	ROM_REGION( 0x20, "rtc", 0 )
	ROM_LOAD( "nvram-trb.bin", 0x00, 0x20, CRC(2516dd08) SHA1(b20afe82efee5b625fb8afda14a1d14aabc8e07e) )
ROM_END

static DRIVER_INIT(nexttrb)
{
	UINT32 *rom = (UINT32 *)(machine.region("user1")->base());
	rom[0x329c/4] = 0x70004e71; // memory test funcall
	rom[0x32a0/4] = 0x4e712400;
	rom[0x03f8/4] = 0x001a4e71; // rom checksum
	rom[0x03fc/4] = 0x4e714e71;
}

static DRIVER_INIT(next)
{
	UINT32 *rom = (UINT32 *)(machine.region("user1")->base());
	rom[0x3f48/4] = 0x2f017000; // memory test funcall
	rom[0x3f4c/4] = 0x4e712400;
	rom[0x00b8/4] = 0x001a4e71; // rom checksum
	rom[0x00bc/4] = 0x4e714e71;
}

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                 FULLNAME               FLAGS */
COMP( 1987, next,   0,          0,   next,      next,    next,      "Next Software Inc",   "NeXT Computer",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1990, nextnt, next,       0,   next040,   next,    0,      "Next Software Inc",   "NeXT (Non Turbo)",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1992, nexttrb,next,       0,   next040,   next,    nexttrb, "Next Software Inc",   "NeXT (Turbo)",			GAME_NOT_WORKING | GAME_NO_SOUND)
