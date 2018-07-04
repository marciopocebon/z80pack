/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2016-2017 by Udo Munk
 *
 * This module implements memory management for an IMSAI 8080 system
 *
 * History:
 * 22-NOV-2016 stuff moved to here and implemented as inline functions
 * 30-DEC-2016 implemented 1 KB page table and setup for that
 * 26-JAN-2017 initialise ROM with 0xff
 */
#include "stdbool.h"

extern void init_memory(void), init_rom(void);
extern void wait_step(void), wait_int_step(void);
extern void imsai_vio_ctrl(BYTE);
extern BYTE memory[];
extern int p_tab[];

#define MEM_RW		0	/* memory is readable and writeable */
#define MEM_RO		1	/* memory is read-only */
#define MEM_WPROT	2	/* memory is write protected */
#define MEM_NONE	3	/* no memory available */

extern void ctrl_port_out(BYTE);
extern BYTE ctrl_port_in(void);

/* Memory access read and write vector tables */
extern BYTE *rdrvec[64];
extern BYTE *wrtvec[64];

extern int cyclecount;
extern bool mempage;
extern void groupswap();

static inline BYTE *loadaddr(unsigned int addr) {
	return (BYTE *)(rdrvec[addr>>10] + (addr&0x03ff));
}

static inline BYTE *wrtaddr(unsigned int addr) {
	return (BYTE *)(wrtvec[addr>>10] + (addr&0x03ff));
}
static inline BYTE *rdraddr(unsigned int addr) {
	if(mempage && cyclecount-- == 0) groupswap();
	return (BYTE *)(rdrvec[addr>>10] + (addr&0x03ff));
}

/*
 * memory access for the CPU cores
 */
static inline void memwrt(WORD addr, BYTE data)
{
	cpu_bus &= ~(CPU_WO | CPU_MEMR);

	fp_clock++;
	fp_led_address = addr;
	fp_led_data = data;
	fp_sampleData();
	wait_step();

	if (p_tab[addr >> 10] == MEM_RW)
		*wrtaddr(addr) = data;
	if (addr == 0xf7ff)
		imsai_vio_ctrl(data);
}

static inline BYTE memrdr(WORD addr)
{
	register BYTE data = *rdraddr(addr);
	cpu_bus |= CPU_WO | CPU_MEMR;

	fp_clock++;
	fp_led_address = addr;
	if (p_tab[addr >> 10] != MEM_NONE)
		fp_led_data = data;
	else
		fp_led_data = 0xff;
	fp_sampleData();
	wait_step();

	if (p_tab[addr >> 10] != MEM_NONE)
		return(data);
	else
		return(0xff);
}

/*
 * memory access for DMA devices
 */
static inline BYTE dma_read(WORD addr)
{
	if (p_tab[addr >> 10] != MEM_NONE)
		return(memory[addr]);
	else
		return(0xff);
}

static inline void dma_write(WORD addr, BYTE data)
{
	if (p_tab[addr >> 10] == MEM_RW)
		memory[addr] = data;
}

/*
 * return memory base pointer for the simulation frame
 */
static inline BYTE *mem_base(void)
{
	return(&memory[0]);
}
