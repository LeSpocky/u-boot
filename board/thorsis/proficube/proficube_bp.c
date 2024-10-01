/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#include <command.h>
#include <console.h>
#include <rand.h>
#include <stdio.h>
#include <time.h>
#include <vsprintf.h>
#include <asm/io.h>

/* TODO	Get through DT?	*/
#define BP_BASE_MOD	0x60000000
#define BP_BASE_CTRL	0x70000000

struct bp_cnt {
	unsigned long rd;
	unsigned long rd_m;
	unsigned long wr;
	unsigned long wr_m;
};

static void bp_upd_pr_cnt(struct bp_cnt *cnt);

static u16 g_wDprSize = 0x5000;

void bp_upd_pr_cnt(struct bp_cnt *cnt)
{
	if (!cnt)
		return;

	cnt->rd_m += cnt->rd >> 20;
	cnt->wr_m += cnt->wr >> 20;

	cnt->rd %= 0xfffff;
	cnt->wr %= 0xfffff;

	printf("\rinfo: ");
	printf("%ld MB and %ld byte written ", cnt->wr_m, cnt->wr);
	printf("%ld MB and %ld byte read  \r", cnt->rd_m, cnt->rd);
}

int do_bp_dpr(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	switch (argc) {
	case 2:
		g_wDprSize = simple_strtoul(argv[1], NULL, 16);
		fallthrough;
	case 1:
		printf("g_wDprSize = 0x%X\n", g_wDprSize);
		return CMD_RET_SUCCESS;
	default:
		return CMD_RET_USAGE;
	}

	return CMD_RET_USAGE;
}

int do_bp_test(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u8 by_cmp_buf[0x8000], data;
	struct bp_cnt cnt = {};
	u16 i;

	if (g_wDprSize > 0x8000) {
		pr_err("invalid DPR size: 0x%x\n", g_wDprSize);
		return CMD_RET_FAILURE;
	}

	srand(get_ticks() + rand());

	printf("Starting random backplane test ...\n");

	printf("Init ... ");
	for (i = 0; i < g_wDprSize; i++)
		by_cmp_buf[i] = 0xFF & rand();
	memcpy_toio(BP_BASE_MOD, by_cmp_buf, g_wDprSize);
	printf("done.\n");

	printf("Check init ... ");
	for (i = 0; i < g_wDprSize; i++) {
		data = readb(BP_BASE_MOD + i);
		if (by_cmp_buf[i] != data) {
			printf("Difference found!\n\r");
			printf("0x%02X read, 0x%02X expected at offset 0x%04X",
			       data, by_cmp_buf[i], i);
			printf("\n\r");
			return CMD_RET_FAILURE;
		}
	}
	printf("done.\n");

	do {
		u16 loop_cnt = 0;

		do {
			u16 byte_cnt, start_offs;

			start_offs = rand() % g_wDprSize;
			byte_cnt = (rand() % 0x100) + 1;

			if (start_offs + byte_cnt > g_wDprSize)
				byte_cnt = g_wDprSize - start_offs;

			if (rand() & 1) {
				/* write */

				for (i = 0; i < byte_cnt; i++)
					by_cmp_buf[start_offs + i] = 0xFF & rand();

				memcpy_toio(BP_BASE_MOD + start_offs,
					    &by_cmp_buf[start_offs],
					    byte_cnt);

				cnt.wr += byte_cnt;
			} else {
				/* read */

				for (i = 0; i < byte_cnt; i++) {
					data = readb(BP_BASE_MOD + start_offs + i);
					cnt.rd++;

					if (by_cmp_buf[start_offs + i] != data) {
						printf("\n\r");
						printf("Difference found!\n\r");

						bp_upd_pr_cnt(&cnt);

						printf("\n\r");
						printf("i=%u, 0x%02X read, 0x%02X expected at offset 0x%04X",
						       i, data,
						       by_cmp_buf[start_offs + i],
						       start_offs + i);
						printf("\n\r");

						return CMD_RET_FAILURE;
					}
				}
			}

			loop_cnt++;
			if (loop_cnt >= 1000) {
				bp_upd_pr_cnt(&cnt);
				loop_cnt = 0;
			}
		} while (loop_cnt);
	} while (!ctrlc());

	printf("\n");
	printf("Stopped random backplane test!\n");

	return CMD_RET_SUCCESS;
}
