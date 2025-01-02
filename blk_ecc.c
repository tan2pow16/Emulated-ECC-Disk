/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include "blk_ecc.h"

#include <string.h>

// "cold_" means directly retrieved from the device (and thus interlaced)!
// The output will be in the "cold_data_chunk" (overwrite the input).
void ecc_read_chunk(uint8_t *cold_data_chunk, uint8_t *cold_ecc_chunk)
{
  size_t i, j;
  uint8_t rs_block[RSBLK_SZ];
  uint8_t raw_chunk[RAW_CHUNK_SZ];
  uint8_t rs_chunk[RS_CHUNK_SZ];
  
  // Un-interlace both input
  for(i = 0 ; i < INTERLACE_WIDTH ; i++)
  {
    for(j = 0 ; j < RAWBLK_SZ ; j++)
    {
      raw_chunk[i * RAWBLK_SZ + j] = cold_data_chunk[i + INTERLACE_WIDTH * j];
    }
  }
  // Use 2 loops to reduce the cache miss chance.
  for(i = 0 ; i < INTERLACE_WIDTH ; i++)
  {
    for(j = 0 ; j < NPAR ; j++)
    {
      rs_chunk[i * NPAR + j] = cold_ecc_chunk[i + INTERLACE_WIDTH * j];
    }
  }
  
  for(i = 0, j = 0 ; i < RAW_CHUNK_SZ ; i += RAWBLK_SZ, j += NPAR)
  {
    memcpy(rs_block, &raw_chunk[i], RAWBLK_SZ);
    memcpy(&rs_block[RAWBLK_SZ], &rs_chunk[j], NPAR);

    decode_data(rs_block, RSBLK_SZ);
    if(check_syndrome()) // Found error(s)!
    {
      // In our case, erasures will be at unknown locations even if they exist.
      correct_errors_erasures(rs_block, RSBLK_SZ, 0, NULL);
    }
    
    // Output to data chunk copied here!
    memcpy(&cold_data_chunk[i], rs_block, RAWBLK_SZ);
  }
}

// "hot_" means raw input (and thus not interlaced)!
// The output will be in the "hot_data_chunk" AND "dest_ecc_chunk" (overwrite both inputs).
void ecc_write_chunk(uint8_t *hot_data_chunk, uint8_t *dest_ecc_chunk)
{
  size_t i, j;
  uint8_t rs_block[RSBLK_SZ];
  uint8_t raw_chunk[RAW_CHUNK_SZ];
  uint8_t rs_chunk[RS_CHUNK_SZ];

  memcpy(raw_chunk, hot_data_chunk, RAW_CHUNK_SZ);

  // Generate ECC output
  for(i = 0, j = 0 ; i < RAW_CHUNK_SZ ; i += RAWBLK_SZ, j += NPAR)
  {
    encode_data(&raw_chunk[i], RAWBLK_SZ, rs_block);
    
    // Raw data unchanged in the raw_chunk, so only ECC data has to be cloned!
    memcpy(&rs_chunk[j], &rs_block[RAWBLK_SZ], NPAR);
  }

  // Interlace both outputs
  for(i = 0 ; i < INTERLACE_WIDTH ; i++)
  {
    for(j = 0 ; j < RAWBLK_SZ ; j++)
    {
      hot_data_chunk[i + INTERLACE_WIDTH * j] = raw_chunk[i * RAWBLK_SZ + j];
    }
  }
  // Use 2 loops to reduce the cache miss chance.
  for(i = 0 ; i < INTERLACE_WIDTH ; i++)
  {
    for(j = 0 ; j < NPAR ; j++)
    {
      dest_ecc_chunk[i + INTERLACE_WIDTH * j] = rs_chunk[i * NPAR + j];
    }
  }
}
