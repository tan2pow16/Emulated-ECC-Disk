#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "spd_proc.h"
#include "blk_ecc.h"

CustomDiskDesc disk_singleton;
CustomDiskDesc *p_disk_singleton = NULL;

const char custom_product_id[0x10] = "Counterfeit"; // MUST be <= 15 bytes
const char custom_product_rev[4] = "0.1"; // MUST be <= 3 bytes

#pragma pack(1)
typedef struct _IO_STATUS_TRACKER {
  uint8_t read_status;
  uint8_t write_status;
  uint8_t read_error;
  uint8_t write_error;
} IO_STATUS_TRACKER;
#pragma pack()

IO_STATUS_TRACKER *p_singleton_io_tracker;

DWORD io_exception_handler(EXCEPTION_POINTERS *exceps)
{
  uint8_t caught = 0;
  
  if(!p_singleton_io_tracker)
  {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  if(p_singleton_io_tracker->read_status)
  {
    p_singleton_io_tracker->read_error = caught = 1;
  }
  
  if(p_singleton_io_tracker->write_status)
  {
    p_singleton_io_tracker->write_error = caught = 1;
  }
  
  return (caught ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH);
}

void setup_io_exception_handler(void)
{
  p_singleton_io_tracker = NULL;
  AddVectoredExceptionHandler(0, (PVECTORED_EXCEPTION_HANDLER)io_exception_handler);
}

BOOL flush_dev(CustomDiskDesc *custom_disk, void *raw_file_addr, uint64_t byte_size)
{
  BOOL ret;
  
  ret = FlushViewOfFile(raw_file_addr, byte_size);
  ret &= FlushFileBuffers(custom_disk->h_dev);
  
  if(ret)
  {
    custom_disk->unflushed_size = 0;
  }
  
  return ret;
}

static BOOLEAN spd_ecc_read(SPD_STORAGE_UNIT *spd, PVOID v_buf, UINT64 block_idx, UINT32 blocks_count, BOOLEAN flush_first, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;
  uint8_t *addr_from;
  uint64_t byte_size;
  uint8_t *ptr_r, *ptr_w;
  uint8_t cache[BLOCK_WITH_ECC_SIZE];
  uint32_t i;
  
  IO_STATUS_TRACKER io_tracker = {0, 0, 0, 0};
  
  custom_disk = spd->UserContext;
  
  addr_from = custom_disk->map_view_base + block_idx * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  byte_size = ((uint64_t)blocks_count) * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  if(flush_first && !flush_dev(custom_disk, addr_from, byte_size))
  {
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
    return TRUE;
  }
  
  ptr_r = addr_from;
  ptr_w = v_buf;
  p_singleton_io_tracker = &io_tracker;
  for(i = 0 ; i < blocks_count ; i++)
  {
    io_tracker.read_status = 1;
    memcpy(cache, ptr_r, BLOCK_WITH_ECC_SIZE);
    io_tracker.read_status = 0;
    
    ecc_read_chunk(cache, &cache[BLOCK_SIZE]);
    memcpy(ptr_w, cache, BLOCK_SIZE);
    
    ptr_r += BLOCK_WITH_ECC_SIZE;
    ptr_w += BLOCK_SIZE;
  }
  p_singleton_io_tracker = NULL;
  
  return TRUE;
}

static BOOLEAN spd_ecc_write(SPD_STORAGE_UNIT *spd, PVOID v_buf, UINT64 block_idx, UINT32 blocks_count, BOOLEAN force_flush, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;
  uint8_t *addr_from;
  uint64_t byte_size;
  uint8_t *ptr_r, *ptr_w;
  uint8_t cache[BLOCK_WITH_ECC_SIZE];
  uint32_t i;
  
  IO_STATUS_TRACKER io_tracker = {0, 0, 0, 0};
  
  custom_disk = spd->UserContext;
  
  addr_from = custom_disk->map_view_base + block_idx * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  byte_size = ((uint64_t)blocks_count) * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  
  ptr_r = v_buf;
  ptr_w = addr_from;
  p_singleton_io_tracker = &io_tracker;
  for(i = 0 ; i < blocks_count ; i++)
  {
    memcpy(cache, ptr_r, BLOCK_SIZE);
    ecc_write_chunk(cache, &cache[BLOCK_SIZE]);
    
    io_tracker.write_status = 1;
    memcpy(ptr_w, cache, BLOCK_WITH_ECC_SIZE);
    io_tracker.write_status = 0;
    
    custom_disk->unflushed_size += BLOCK_WITH_ECC_SIZE;
    if(custom_disk->unflushed_size >= FORCE_FLUSH_SIZE && !flush_dev(custom_disk, addr_from, 0))
    {
      p_singleton_io_tracker = NULL;
      SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, 0);
      return TRUE;
    }
    
    ptr_r += BLOCK_SIZE;
    ptr_w += BLOCK_WITH_ECC_SIZE;
  }
  p_singleton_io_tracker = NULL;
  
  if(force_flush && !flush_dev(custom_disk, addr_from, byte_size))
  {
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, 0);
  }
  
  return TRUE;
}

static BOOLEAN spd_ecc_flush(SPD_STORAGE_UNIT *spd, UINT64 block_idx, UINT32 blocks_count, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;
  
  custom_disk = spd->UserContext;
  if(!flush_dev(custom_disk, custom_disk->map_view_base + block_idx * ((uint64_t)BLOCK_WITH_ECC_SIZE), ((uint64_t)blocks_count) * ((uint64_t)BLOCK_WITH_ECC_SIZE)))
  {
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, 0);
  }

  return TRUE;
}

static BOOLEAN spd_ecc_unmap(SPD_STORAGE_UNIT *spd, SPD_UNMAP_DESCRIPTOR *unmap_desc, UINT32 unmap_desc_count, SPD_STORAGE_UNIT_STATUS *status_out)
{
  // Do nothing per this impl (leave unmapped data there without clearing them to 0)
  return TRUE;
}

static BOOL WINAPI ctrl_signal_handler(DWORD ctrl_signal_type)
{
  spd_force_shutdown();
  return TRUE;
}

static SPD_STORAGE_UNIT_INTERFACE spd_io_functions;
static SPD_GUARD ctrl_signal_guard;

// If failed, the caller MUST close the unused handle!
CustomDiskDesc *create_disk(HANDLE h_dev)
{
  SPD_STORAGE_UNIT_PARAMS spd_params;
  uint64_t map_size;
  DWORD err;

  if(p_disk_singleton)
  {
    // Singleton already initialized
    return p_disk_singleton;
  }

  // Sanitize the input handle
  SetFilePointer(h_dev, 0, NULL, FILE_BEGIN);

  map_size = ((uint64_t)BLOCK_WITH_ECC_SIZE) * ((uint64_t)MAX_BLOCK_COUNT);

  memset(&disk_singleton, 0, sizeof(disk_singleton));
  disk_singleton.h_dev = h_dev;
  disk_singleton.h_map_view = CreateFileMappingW(h_dev, 0, PAGE_READWRITE, map_size >> 0x20, map_size & 0xFFFFFFFF, 0);
  if(!(disk_singleton.h_map_view))
  {
    printf("CreateFileMappingW() failed. (%d)\n", GetLastError());
    
    return NULL;
  }
  
  disk_singleton.map_view_base = MapViewOfFile(disk_singleton.h_map_view, FILE_MAP_ALL_ACCESS, 0, SKIP_HEADER_SIZE, 0);
  if(!(disk_singleton.map_view_base))
  {
    printf("MapViewOfFile() failed. (%d)\n", GetLastError());
    
    CloseHandle(disk_singleton.h_map_view);
    return NULL;
  }

  memset(&spd_params, 0, sizeof(SPD_STORAGE_UNIT_PARAMS));
  UuidCreate(&spd_params.Guid);
  spd_params.BlockCount = MAX_BLOCK_COUNT;
  spd_params.BlockLength = BLOCK_SIZE;
  spd_params.MaxTransferLength = 0x40000; // 256 KiB
  memcpy(spd_params.ProductId, custom_product_id, 0x10);
  memcpy(spd_params.ProductRevisionLevel, custom_product_rev, 4);
  spd_params.WriteProtected = 0;
  spd_params.CacheSupported = 1;
  spd_params.UnmapSupported = 1;

  memset(&spd_io_functions, 0, sizeof(SPD_STORAGE_UNIT_INTERFACE));
  spd_io_functions.Read = spd_ecc_read;
  spd_io_functions.Write = spd_ecc_write;
  spd_io_functions.Flush = spd_ecc_flush;
  spd_io_functions.Unmap = spd_ecc_unmap;

  err = SpdStorageUnitCreate(NULL, &spd_params, &spd_io_functions, &(disk_singleton.spd));
  if(err != ERROR_SUCCESS)
  {
    printf("Create SPD storage failed.\n");
    
    UnmapViewOfFile(disk_singleton.map_view_base);
    CloseHandle(disk_singleton.h_map_view);
    return NULL;
  }
  
  InitializeSRWLock(&(ctrl_signal_guard.Lock));
  ctrl_signal_guard.Pointer = NULL;
  
  err = SpdStorageUnitStartDispatcher(disk_singleton.spd, 1); // This impl is single-threaded
  if(err != ERROR_SUCCESS)
  {
    printf("Setup SPD storage persistence failed.\n");
    
    UnmapViewOfFile(disk_singleton.map_view_base);
    CloseHandle(disk_singleton.h_map_view);
    return NULL;
  }

  SpdGuardSet(&ctrl_signal_guard, p_disk_singleton->spd);
  SetConsoleCtrlHandler(ctrl_signal_handler, TRUE);
  SpdStorageUnitWaitDispatcher(p_disk_singleton->spd);
  SpdGuardSet(&ctrl_signal_guard, 0);

  (disk_singleton.spd)->UserContext = p_disk_singleton = &disk_singleton;
  setup_io_exception_handler();

  return p_disk_singleton;
}

void spd_force_shutdown(void)
{
  SpdGuardExecute(&ctrl_signal_guard, SpdStorageUnitShutdown);

  if(!p_disk_singleton)
  {
    return;
  }
  p_disk_singleton = NULL;

  FlushViewOfFile(disk_singleton.map_view_base, 0);
  FlushFileBuffers(disk_singleton.h_dev);
  
  UnmapViewOfFile(disk_singleton.map_view_base);
  CloseHandle(disk_singleton.h_map_view);
  CloseHandle(disk_singleton.h_dev);
  
  memset(&disk_singleton, 0, sizeof(disk_singleton));
}
