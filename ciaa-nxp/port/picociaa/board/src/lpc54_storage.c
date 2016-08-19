/*
 * This file is part of the Micro Python project, http:// micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <board.h>

#include "storage.h"
#include "py/obj.h"
#include "py/mphal.h"

#define FLASH_ADDR_BASE        (0x00000000)
#define FLASH_PAGE_SIZE        (256)
#define FLASH_SECTOR_SIZE      (32*1024)
#define FLASH_PAGES_PER_BLOCK  (FLASH_BLOCK_SIZE / FLASH_PAGE_SIZE)
#define FLASH_PAGES_PER_SECTOR (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)

#define FLASH_DISK_BASE      (0x00070000)
#define FLASH_DISK_SIZE      (64 * 1024)
#define FLASH_DISK_PAGE_BASE ((FLASH_DISK_BASE - FLASH_ADDR_BASE) / FLASH_PAGE_SIZE)
#define FLASH_DISK_SECTORS   (FLASH_DISK_SIZE / FLASH_BLOCK_SIZE)

static const char *flash_msg[] = {
		"CMD Sucess",
		"Invalid command",
		"SRC address error",
		"DST address error",
		"SRC addr not map",
		"DST addr not map",
		"COUNT error",
		"Invalid sector",
		"Sector not blank",
		"Sector not prepared",
		"Compare error",
		"BUSY"
};

static void flash_abort(const char *msg) {
	(void) msg;
	__enable_irq();
	mp_hal_stdout_tx_str(msg);
	while(1) { }
}

void storage_init(void) {
#if 0
	static volatile unsigned int flash_inited = 0;
	if (flash_inited == 0) {
		unsigned int command[5] = { 49, 0, 0, 0, 0 };
		unsigned int result[5];
		iap_entry(command, result);
		flash_inited = 1;
	}
#endif
}

void storage_flush(void) {
}

static void build_partition(uint8_t *buf, int boot, int type, uint32_t start_block, uint32_t num_blocks) {
    buf[0] = boot;

    if (num_blocks == 0) {
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 0;
    } else {
        buf[1] = 0xff;
        buf[2] = 0xff;
        buf[3] = 0xff;
    }

    buf[4] = type;

    if (num_blocks == 0) {
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = 0;
    } else {
        buf[5] = 0xff;
        buf[6] = 0xff;
        buf[7] = 0xff;
    }

    buf[8] = start_block;
    buf[9] = start_block >> 8;
    buf[10] = start_block >> 16;
    buf[11] = start_block >> 24;

    buf[12] = num_blocks;
    buf[13] = num_blocks >> 8;
    buf[14] = num_blocks >> 16;
    buf[15] = num_blocks >> 24;
}

static void my_memcpy(uint32_t *dst, const uint32_t *src, size_t len) {
	while(len--) {
		*dst++ = *src++;
	}
}

bool storage_read_block(uint8_t *dest, uint32_t block) {
    /* printf("RD %u\n", block); */
    if (block == 0) {
        /*  fake the MBR so we can decide on our own partition table */

        for (int i = 0; i < 446; i++) {
            dest[i] = 0;
        }

        build_partition(dest + 446, 0, 0x01 /* FAT12 */, 0x1, FLASH_DISK_SECTORS);
        build_partition(dest + 462, 0, 0, 0, 0);
        build_partition(dest + 478, 0, 0, 0, 0);
        build_partition(dest + 494, 0, 0, 0, 0);

        dest[510] = 0x55;
        dest[511] = 0xaa;

        return true;

    } else {
        const uint32_t *src = (const uint32_t*) (FLASH_DISK_BASE + FLASH_BLOCK_SIZE * block);
        my_memcpy((uint32_t*) dest, src, FLASH_BLOCK_SIZE / sizeof(uint32_t));
        return true;
    }
}

static uint32_t aliged_buffer[FLASH_BLOCK_SIZE / sizeof(uint32_t)];

static int check_blank(uint32_t address, uint32_t bytes)
{
	uint32_t *ptr = ((uint32_t *) address);
	uint32_t count = bytes / 4;
	uint32_t i;
	for (i=0; i<count; i++) {
		if (*ptr++ != 0xFFFFFFFF)
			return IAP_SECTOR_NOT_BLANK;
	}
	return IAP_CMD_SUCCESS;
}

bool storage_write_block(const uint8_t *src, uint32_t block) {
    /* printf("WR %u\n", block); */
    if (block == 0) {
        /*  can't write MBR, but pretend we did */
        return true;
    } else {
    	uint32_t page_start = FLASH_DISK_PAGE_BASE + ((block * FLASH_PAGE_SIZE) / FLASH_SECTOR_SIZE);
    	uint32_t page_end = page_start + FLASH_PAGES_PER_BLOCK - 1;
    	uint32_t flash_dst = FLASH_DISK_BASE + FLASH_BLOCK_SIZE * block;
    	uint32_t sector = page_start / FLASH_PAGES_PER_SECTOR;

#define GUARD(cmd) do { \
	uint8_t e = cmd; \
	if (e != IAP_CMD_SUCCESS) { \
		flash_abort(flash_msg[e]); \
		return false; \
	} \
} while(0)
    	my_memcpy(aliged_buffer, (const uint32_t*) src, FLASH_BLOCK_SIZE / sizeof(uint32_t));
        __disable_irq();
    	if (1) {
    		int retry = 3;
			do {
				uint32_t e;
		    	GUARD(Chip_IAP_PreSectorForReadWrite(sector, sector));
				GUARD(Chip_IAP_ErasePage(page_start, page_end));
				e = check_blank(flash_dst, FLASH_BLOCK_SIZE);
				if (e == IAP_CMD_SUCCESS) {
					break;
				} else {
					mp_hal_stdout_tx_str(flash_msg[e]);
					mp_hal_stdout_tx_str("\r\n");
				}
			} while(retry--);
    	}
    	GUARD(Chip_IAP_PreSectorForReadWrite(sector, sector));
    	GUARD(Chip_IAP_CopyRamToFlash(flash_dst, aliged_buffer, FLASH_BLOCK_SIZE));
    	GUARD(Chip_IAP_Compare(flash_dst, (uint32_t) aliged_buffer, FLASH_BLOCK_SIZE));
        __enable_irq();

        return true;
    }
}
