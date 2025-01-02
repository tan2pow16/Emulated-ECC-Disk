/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <stdint.h>

#include "winspd/winspd.h"

#include "custom_test.h"

#define BLOCK_SIZE RAW_CHUNK_SZ
#define BLOCK_WITH_ECC_SIZE (RAW_CHUNK_SZ + RS_CHUNK_SZ)

typedef struct _CustomDiskDesc
{
  SPD_STORAGE_UNIT *spd;
  
  uint32_t block_count; // Number of "visible" blocks. The block size is defined by the ECC module!
  
  HANDLE h_dev;

  CRITICAL_SECTION lock;
} CustomDiskDesc;

extern CustomDiskDesc *p_disk_singleton;

CustomDiskDesc *create_disk(HANDLE h_dev);
void spd_force_shutdown(void);
