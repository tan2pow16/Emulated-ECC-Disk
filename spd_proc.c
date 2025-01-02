/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "spd_proc.h"
#include "blk_ecc.h"

CustomDiskDesc disk_singleton;
CustomDiskDesc *p_disk_singleton = NULL;

const char custom_product_id[0x10] = "Counterfeit"; // MUST be <= 15 bytes
const char custom_product_rev[4] = "0.1"; // MUST be <= 3 bytes

/*
void xprintf(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  FILE *fp = fopen("test000.log", "ab");
  vfprintf(fp, fmt, argptr);
  fclose(fp);
  va_end(argptr);
}
*/

BOOL flush_dev(CustomDiskDesc *custom_disk)
{
  return FlushFileBuffers(custom_disk->h_dev);
}

static BOOLEAN spd_ecc_read(SPD_STORAGE_UNIT *spd, PVOID v_buf, UINT64 block_idx, UINT32 blocks_count, BOOLEAN flush_first, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;
  uint8_t *ptr_w;
  uint64_t pos_r;
  DWORD l, h;
  uint8_t cache[BLOCK_WITH_ECC_SIZE];
  uint32_t i;

  // xprintf("spd_ecc_read(-, %lld, %ld, %d, -)\n", block_idx, blocks_count, flush_first);

  custom_disk = spd->UserContext;
  
  if(flush_first && !flush_dev(custom_disk))
  {
    // xprintf("spd_ecc_read() flush failed.\n");
    
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
    return TRUE;
  }

  pos_r = SKIP_HEADER_SIZE + block_idx * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  l = pos_r & 0xFFFFFFFF;
  h = pos_r >> 0x20;
  EnterCriticalSection(&(custom_disk->lock));
  if(SetFilePointer(custom_disk->h_dev, l, &h, FILE_BEGIN) != l)
  {
    // xprintf("SetFilePointer() failed in spd_ecc_read() <%d>\n", GetLastError());
    
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
    LeaveCriticalSection(&(custom_disk->lock));
    return TRUE;
  }

  ptr_w = v_buf;
  for(i = 0 ; i < blocks_count ; i++)
  {
    if(!ReadFile(custom_disk->h_dev, cache, BLOCK_WITH_ECC_SIZE, &l, NULL) || l != BLOCK_WITH_ECC_SIZE)
    {
      // xprintf("ReadFile() failed in spd_ecc_read() <%d>\n", GetLastError());
      
      SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
      break;
    }

    ecc_read_chunk(cache, &cache[BLOCK_SIZE]);
    memcpy(ptr_w, cache, BLOCK_SIZE);
    
    pos_r += BLOCK_WITH_ECC_SIZE;
    ptr_w += BLOCK_SIZE;
  }
  LeaveCriticalSection(&(custom_disk->lock));
  
  // xprintf("spd_ecc_read() succeeded.\n");
  
  return TRUE;
}

static BOOLEAN spd_ecc_write(SPD_STORAGE_UNIT *spd, PVOID v_buf, UINT64 block_idx, UINT32 blocks_count, BOOLEAN force_flush, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;
  uint8_t *ptr_r;
  uint64_t pos_w;
  DWORD l, h;
  uint8_t cache[BLOCK_WITH_ECC_SIZE];
  uint32_t i;

  // xprintf("spd_ecc_write(-, %lld, %ld, %d, -)\n", block_idx, blocks_count, force_flush);

  custom_disk = spd->UserContext;

  pos_w = SKIP_HEADER_SIZE + block_idx * ((uint64_t)BLOCK_WITH_ECC_SIZE);
  l = pos_w & 0xFFFFFFFF;
  h = pos_w >> 0x20;
  EnterCriticalSection(&(custom_disk->lock));
  if(SetFilePointer(custom_disk->h_dev, l, &h, FILE_BEGIN) != l)
  {
    // xprintf("SetFilePointer() failed in spd_ecc_write() <%d>\n", GetLastError());
    
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
    LeaveCriticalSection(&(custom_disk->lock));
    return TRUE;
  }

  ptr_r = v_buf;
  for(i = 0 ; i < blocks_count ; i++)
  {
    memcpy(cache, ptr_r, BLOCK_SIZE);
    ecc_write_chunk(cache, &cache[BLOCK_SIZE]);

    if(!WriteFile(custom_disk->h_dev, cache, BLOCK_WITH_ECC_SIZE, &l, NULL) || l != BLOCK_WITH_ECC_SIZE)
    {
      // xprintf("WriteFile() failed in spd_ecc_read() <%d>\n", GetLastError());
      
      SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, NULL);
      break;
    }

    ptr_r += BLOCK_SIZE;
    pos_w += BLOCK_WITH_ECC_SIZE;
  }
  LeaveCriticalSection(&(custom_disk->lock));
  
  if(force_flush && !flush_dev(custom_disk))
  {
    // xprintf("spd_ecc_write() flush failed.\n");
    
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, 0);
  }

  // xprintf("spd_ecc_write() succeeded.\n");

  return TRUE;
}

static BOOLEAN spd_ecc_flush(SPD_STORAGE_UNIT *spd, UINT64 block_idx, UINT32 blocks_count, SPD_STORAGE_UNIT_STATUS *status_out)
{
  CustomDiskDesc *custom_disk;

  // xprintf("spd_ecc_flush(-, %lld, %ld, -)\n", block_idx, blocks_count);

  custom_disk = spd->UserContext;
  if(!flush_dev(custom_disk))
  {
    SpdStorageUnitStatusSetSense(status_out, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_WRITE_ERROR, 0);
  }

  // xprintf("spd_ecc_flush() succeeded.\n");

  return TRUE;
}

static BOOLEAN spd_ecc_unmap(SPD_STORAGE_UNIT *spd, SPD_UNMAP_DESCRIPTOR *unmap_desc, UINT32 unmap_desc_count, SPD_STORAGE_UNIT_STATUS *status_out)
{
  // Do nothing per this impl (leave unmapped data there without clearing them to 0)
  UINT32 I;

  // xprintf("spd_ecc_unmap(-, -, %ld, -)\n", unmap_desc_count);
  
  for (I = 0; unmap_desc_count > I; I++)
  {
    // xprintf("logged unmap: (%d, %d)\n", unmap_desc[I].BlockAddress, unmap_desc[I].BlockCount);
  }
  
  return TRUE;
}

static BOOL WINAPI ctrl_signal_handler(DWORD ctrl_signal_type)
{
  spd_force_shutdown();
  return TRUE;
}

static SPD_STORAGE_UNIT_INTERFACE spd_io_functions;
static SPD_GUARD ctrl_signal_guard;

DWORD custom_disk_guard_thread(void *param)
{
  SpdStorageUnitWaitDispatcher(p_disk_singleton->spd);
  SpdGuardSet(&ctrl_signal_guard, 0);
  
  return 0;
}

// If failed, the caller MUST close the unused handle!
CustomDiskDesc *create_disk(HANDLE h_dev)
{
  SPD_STORAGE_UNIT_PARAMS spd_params;
  DWORD err;

  if(p_disk_singleton)
  {
    // Singleton already initialized
    return p_disk_singleton;
  }

  // Sanitize the input handle
  SetFilePointer(h_dev, SKIP_HEADER_SIZE, NULL, FILE_BEGIN);

  memset(&disk_singleton, 0, sizeof(disk_singleton));
  disk_singleton.h_dev = h_dev;

  memset(&spd_params, 0, sizeof(SPD_STORAGE_UNIT_PARAMS));
  UuidCreate(&spd_params.Guid);
  spd_params.BlockCount = MAX_BLOCK_COUNT;
  spd_params.BlockLength = BLOCK_SIZE;
  spd_params.MaxTransferLength = 0x40000; // 256 KiB
  memcpy(spd_params.ProductId, custom_product_id, 0x10);
  memcpy(spd_params.ProductRevisionLevel, custom_product_rev, 4);
  // spd_params.WriteProtected = 0;
  // spd_params.CacheSupported = 0;
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

    return NULL;
  }
  
  InitializeSRWLock(&(ctrl_signal_guard.Lock));
  ctrl_signal_guard.Pointer = NULL;
  
  InitializeCriticalSection(&(disk_singleton.lock));
  
  err = SpdStorageUnitStartDispatcher(disk_singleton.spd, 1); // This impl is single-threaded
  if(err != ERROR_SUCCESS)
  {
    printf("Setup SPD storage persistence failed.\n");

    return NULL;
  }

  (disk_singleton.spd)->UserContext = p_disk_singleton = &disk_singleton;
  SpdGuardSet(&ctrl_signal_guard, p_disk_singleton->spd);
  SetConsoleCtrlHandler(ctrl_signal_handler, TRUE);
  
  CreateThread(NULL, 0, custom_disk_guard_thread, NULL, 0, NULL);

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

  CloseHandle(disk_singleton.h_dev);
  DeleteCriticalSection(&(disk_singleton.lock));
  
  memset(&disk_singleton, 0, sizeof(disk_singleton));
}
