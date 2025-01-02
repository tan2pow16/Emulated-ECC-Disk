#include <stdint.h>

#include "winspd/winspd.h"

#include "custom_test.h"

#define BLOCK_SIZE RAW_CHUNK_SZ
#define BLOCK_WITH_ECC_SIZE (RAW_CHUNK_SZ + RS_CHUNK_SZ)

// The number of bytes since last flush allowed before a flush is enforced without signal from the OS.
//  Default is 16MiB.
#define FORCE_FLUSH_SIZE 0x1000000

typedef struct _CustomDiskDesc
{
  SPD_STORAGE_UNIT *spd;
  
  uint32_t block_count; // Number of "visible" blocks. The block size is defined by the ECC module!
  
  HANDLE h_dev;
  HANDLE h_map_view;
  
  uint8_t *map_view_base;
  uint64_t unflushed_size;
} CustomDiskDesc;

extern CustomDiskDesc *p_disk_singleton;

CustomDiskDesc *create_disk(HANDLE h_dev);
void spd_force_shutdown(void);
