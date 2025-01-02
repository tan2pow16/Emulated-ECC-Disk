/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <stdint.h>

#include "rscode/ecc.h"

// Do NOT change these.
#define RAWBLK_SZ NPAR
#define RSBLK_SZ (RAWBLK_SZ + NPAR)
#define INTERLACE_WIDTH 0x200
#define RAW_CHUNK_SZ (RAWBLK_SZ * INTERLACE_WIDTH)
#define RS_CHUNK_SZ (NPAR * INTERLACE_WIDTH)

// The pseudo device BLOCK_LENGTH must be the same as RAW_CHUNK_SZ and RS_CHUNK_SZ!!

// "cold_" means directly retrieved from the device (and thus interlaced)!
// The output will be in the "cold_data_chunk" (overwrite the input).
void ecc_read_chunk(uint8_t *cold_data_chunk, uint8_t *cold_ecc_chunk);

// "hot_" means raw input (and thus not interlaced)!
// The output will be in the "hot_data_chunk" AND "dest_ecc_chunk" (overwrite both inputs).
void ecc_write_chunk(uint8_t *hot_data_chunk, uint8_t *dest_ecc_chunk);
