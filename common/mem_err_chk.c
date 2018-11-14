// tary, 16:44 2012-02-06
#define DBG	1
#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

#define MEM_WR_WORD(adr, word)		((*(volatile ulong*)adr) = (word))
#define MEM_RD_WORD(adr)		(*(volatile ulong*)adr)
#define MAGIC_W_0			0x55555555
#define MAGIC_W_1			0xAAAAAAAA
#define MAGIC_W_2			0x55AA55AA
#define MAGIC_W_3			0xAA55AA55

#define MEM_WR_BYTE(adr, byte)		((*(volatile char*)adr) = (byte))
#define MEM_RD_BYTE(adr)		(*(volatile char*)adr)
#define MAGIC_B_0			0x55
#define	MAGIC_B_1			0xAA
#define MAGIC_B_2			0x5A
#define MAGIC_B_3			0xA5

static int mem_adr_01_chk(ulong base) {
	ulong adr[4];

	adr[0] = base + 0;
	adr[1] = base + (1UL << 0);
	adr[2] = base + (1UL << 1);
	adr[3] = base + (1UL << 0) + (1UL << 1);

	MEM_WR_BYTE(adr[0], MAGIC_B_0);
	MEM_WR_BYTE(adr[1], MAGIC_B_1);
	MEM_WR_BYTE(adr[2], MAGIC_B_2);
	MEM_WR_BYTE(adr[3], MAGIC_B_3);

	if (MEM_RD_BYTE(adr[0]) != MAGIC_B_0) {
		return -1;
	}
	if (MEM_RD_BYTE(adr[1]) != MAGIC_B_1) {
		return -1;
	}
	if (MEM_RD_BYTE(adr[2]) != MAGIC_B_2) {
		return -1;
	}
	if (MEM_RD_BYTE(adr[3]) != MAGIC_B_3) {
		return -1;
	}
	return 0;
}

// check for address line short with ground or vcc
static int mem_adr_1bit_chk(ulong base, int nr) {
	int r;
	int i;
	ulong adr[2];

	r = 0;
	adr[0] = base + 0;
	for (i = 2; i < nr - 1; i++) {
		adr[1] = base + (1UL << i);
		MEM_WR_WORD(adr[0], MAGIC_W_0);
		MEM_WR_WORD(adr[1], MAGIC_W_1);
		if (MEM_RD_WORD(adr[0]) != MAGIC_W_0) {
#if DBG
			printf("  \tA%d *** ERROR *** (0x%08X != 0x%08X)\n",
				i, MEM_RD_WORD(adr[0]), MAGIC_W_0);
#else
			printf("  \tA%d *** ERROR ***\n", i);
#endif
			r = -1;
		}
		if (MEM_RD_WORD(adr[1]) != MAGIC_W_1) {
#if DBG
			printf("  \tA%d *** ERROR *** (0x%08X != 0x%08X)\n",
				i, MEM_RD_WORD(adr[1]), MAGIC_W_1);
#else
			printf("  \tA%d *** ERROR ***\n", i);
#endif
			r = -1;
		}
	}
	return r;
}

// check for two address lines short with each other
static int mem_adr_2bit_chk(ulong base, int bx, int by) {
	int r;
	ulong adr[4];

	adr[0] = base + 0;
	adr[1] = base + (1UL << bx);
	adr[2] = base + (1UL << by);
	adr[3] = base + (1UL << bx) + (1UL << by);

	MEM_WR_WORD(adr[0], MAGIC_W_0);
	MEM_WR_WORD(adr[1], MAGIC_W_1);
	MEM_WR_WORD(adr[2], MAGIC_W_2);
	MEM_WR_WORD(adr[3], MAGIC_W_3);

	r = 0;

	if (MEM_RD_WORD(adr[0]) != MAGIC_W_0) {
#if DBG
		printf("  \tA%2dA%2d=00 (0x%08X != 0x%08X)\n", bx, by, MEM_RD_WORD(adr[0]), MAGIC_W_0);
#endif
		r = -1;
	}
	if (MEM_RD_WORD(adr[1]) != MAGIC_W_1) {
#if DBG
		printf("  \tA%2dA%2d=10 (0x%08X != 0x%08X)\n", bx, by, MEM_RD_WORD(adr[1]), MAGIC_W_1);
#endif
		r = -1;
	}
	if (MEM_RD_WORD(adr[2]) != MAGIC_W_2) {
#if DBG
		printf("  \tA%2dA%2d=01 (0x%08X != 0x%08X)\n", bx, by, MEM_RD_WORD(adr[2]), MAGIC_W_2);
#endif
		r = -1;
	}
	if (MEM_RD_WORD(adr[3]) != MAGIC_W_3) {
#if DBG
		printf("  \tA%2dA%2d=11 (0x%08X != 0x%08X)\n", bx, by, MEM_RD_WORD(adr[3]), MAGIC_W_3);
#endif
		r = -1;
	}
	return r;
}

// check for data line error
static int mem_dat_bus_chk(ulong base) {
	int r;
	int i;
	ulong adr, word;

	adr = base;
	r = 0;
	for (i = 0; i < 32; i++) {
		word = 1UL << i;
		MEM_WR_WORD(adr, word);
		if (MEM_RD_WORD(adr) != word) {
			r = -1;
			printf("  \tD%d *** ERROR ***\n");
		}
	}
	return r;
}

static int mem_bank_chk(ulong start, ulong size) {
	int adr_nr;
	int i, j;
	int r = 0;

	for (i = 0; i < 32; i++) {
		if ((1UL << i) >= size) break;
	}
	adr_nr = i;

	printf("  ADDRESS LINE # = %d A0-A%d\n", adr_nr, adr_nr - 1);

	if (mem_dat_bus_chk(start) != 0) {
		return -1;
	}

	if (mem_adr_1bit_chk(start, adr_nr) != 0) {
		return -1;
	}

	for (i = 2; i < adr_nr - 1; i++)
	for (j = i + 1; j < adr_nr - 1; j++) {
		if (mem_adr_2bit_chk(start, i, j) != 0) {
			printf("  \tA%d A%d short\n", i, j);
			r = -1;
		}
	}

	if (mem_adr_01_chk(start) != 0) {
		printf("  \tA0 A1 short\n");
		r = -1;
	}

	return r;
}

int mem_err_chk(void) {
	const char * ddr_names[] = {"DDR0", "DDR1"};
	int i;
	int r;

	printf("%s()\n", __func__);


	gd->bd->bi_dram[0].start = 0x80000000;
	gd->bd->bi_dram[0].size  = 0x20000000;


	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		printf("DDR%d : NAME = %s ADDR = 0x%d (SIZE = 0x%d)\n",
			i,
			ddr_names[i],
			gd->bd->bi_dram[i].start,
			gd->bd->bi_dram[i].size);
		r = mem_bank_chk(gd->bd->bi_dram[i].start, gd->bd->bi_dram[i].size);
		if (r != 0) {
			printf("DDR%d ****** ERROR ******\n", i);
		}
	}
	
	return 0;
}

