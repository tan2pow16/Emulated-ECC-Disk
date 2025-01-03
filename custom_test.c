/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include "custom_test.h"

#include <stdint.h>
#include <winioctl.h>
#include <stdio.h>

#include "constants.h"
#include "disk_util.h"
#include "blk_ecc.h"

// The config of those counterfeit SD cards I got.
//  The fake capacities have quite unique values.
const DISK_GEOMETRY target = {
  {
    16317,
    0
  },
  RemovableMedia,
  255,
  63,
  512
};

const uint8_t magic_bytes[] = {
  0x15, 0x61, 0xbf, 0x8f, 0xd3, 0x99, 0xd3, 0xc3, 0x06, 0xad, 0x4f, 0x57, 0x81, 0x87, 0x2d, 0x41,
  0xe1, 0xdf, 0xa5, 0xf7, 0xdb, 0xa1, 0xe3, 0x37, 0x96, 0xf3, 0x2e, 0x48, 0x2b, 0x11, 0x09, 0xb5,
  0x63, 0x5d, 0xe7, 0x55, 0x11, 0x15, 0xd9, 0x5b, 0xad, 0x4e, 0x4f, 0xe7, 0xe1, 0xab, 0x3b, 0x21,
  0xa9, 0x48, 0x69, 0xe5, 0xbd, 0xd7, 0x22, 0xa9, 0x6b, 0x39, 0x31, 0x2d, 0x01, 0xaa, 0xcb, 0xf9,
  0x6f, 0xc7, 0xe1, 0x9d, 0x7f, 0xc9, 0x9d, 0xc9, 0x00, 0xf1, 0xc1, 0xbf, 0xd3, 0x3c, 0xdf, 0x48,
  0xb7, 0x14, 0xc3, 0x15, 0x8d, 0xd7, 0x05, 0xd8, 0xbf, 0xf7, 0x4d, 0xf1, 0xe9, 0x29, 0xe7, 0xdb,
  0x88, 0x2d, 0x79, 0x30, 0x23, 0xf9, 0x83, 0xd7, 0x7d, 0x25, 0xde, 0x15, 0xd8, 0x6a, 0x50, 0x9d,
  0x89, 0xe9, 0x43, 0xf1, 0xad, 0x95, 0x2e, 0xd3, 0x2f, 0x15, 0x35, 0x0b, 0xd7, 0x4d, 0x5d, 0xf5,
  0xf3, 0xbf, 0x13, 0x17, 0x6f, 0xdd, 0x9b, 0x4d, 0x0b, 0x4f, 0x61, 0x1a, 0x39, 0xf7, 0xf4, 0x0b,
  0xe3, 0x8d, 0x91, 0x29, 0x6a, 0x69, 0xf7, 0x03, 0x3d, 0x19, 0x1f, 0x0f, 0x33, 0xc1, 0x8f, 0x13,
  0xf7, 0xb9, 0xc5, 0x91, 0x00, 0x3d, 0x2b, 0x72, 0x41, 0x53, 0x37, 0xd1, 0xe3, 0x2f, 0x78, 0x23,
  0xeb, 0xdf, 0x7f, 0x09, 0x45, 0x33, 0x03, 0xb3, 0x05, 0xb5, 0xf9, 0x27, 0x0b, 0x99, 0x8b, 0x36,
  0x2d, 0x13, 0x0b, 0x8d, 0x9d, 0x28, 0x33, 0x69, 0x69, 0x2f, 0x55, 0x87, 0xf9, 0xde, 0x51, 0x8d,
  0x13, 0x15, 0xe3, 0x06, 0xf1, 0xc3, 0x30, 0x3b, 0x29, 0x3d, 0x75, 0xcb, 0xa9, 0xf9, 0x4e, 0x8f,
  0x9b, 0x69, 0x89, 0x51, 0x0b, 0x95, 0x23, 0x43, 0xe3, 0x69, 0x4e, 0x37, 0x9b, 0x78, 0x03, 0x29,
  0xdd, 0x48, 0x73, 0xb2, 0x3b, 0x2f, 0xa1, 0xb3, 0xdd, 0xed, 0x8e, 0xaa, 0x85, 0x2f, 0x7b, 0x21,
  0x91, 0x1b, 0x85, 0x7d, 0xb1, 0xa7, 0xca, 0xf5, 0x4d, 0xd9, 0x89, 0xe9, 0x19, 0xeb, 0xb3, 0x6f,
  0x87, 0xf4, 0xe3, 0xa1, 0x79, 0x36, 0xc5, 0x13, 0x99, 0xdf, 0x82, 0x9b, 0x31, 0x19, 0x5d, 0x90,
  0x2b, 0x17, 0x43, 0x50, 0x4d, 0x48, 0x85, 0xbd, 0x91, 0x28, 0xb5, 0xf5, 0xeb, 0x8d, 0x0d, 0x56,
  0x79, 0x6f, 0xf9, 0xa5, 0xf5, 0x9b, 0x3d, 0x4d, 0xd7, 0x6b, 0xf9, 0x87, 0xf1, 0xd5, 0xe9, 0x51,
  0x91, 0xef, 0x36, 0x43, 0xf3, 0x93, 0x51, 0x03, 0x61, 0xe5, 0x3b, 0x22, 0x19, 0x65, 0x69, 0xd8,
  0x22, 0x63, 0x53, 0x13, 0xaf, 0xd5, 0x3d, 0x05, 0xa1, 0x91, 0x5d, 0x55, 0x15, 0x8e, 0xd9, 0x91,
  0x64, 0xb3, 0x4f, 0x51, 0xbf, 0x3d, 0xb3, 0x5d, 0x33, 0xb9, 0x53, 0x56, 0x29, 0x37, 0xa4, 0x8f,
  0x9d, 0x8f, 0x87, 0x36, 0x56, 0x5d, 0x03, 0xf9, 0x6d, 0x6b, 0xc1, 0xef, 0x67, 0x5d, 0x7e, 0xd3,
  0xd5, 0x91, 0x8f, 0xcf, 0xdb, 0x99, 0x5b, 0xff, 0xb9, 0x4e, 0x8f, 0xcb, 0x29, 0x0f, 0x85, 0xc3,
  0x2f, 0x57, 0x0d, 0x91, 0x8f, 0x41, 0x8e, 0x9d, 0x97, 0xdf, 0xc1, 0x59, 0xdd, 0xe9, 0xf7, 0xdf,
  0x5b, 0x6f, 0xf7, 0xb7, 0x50, 0x6b, 0x28, 0xdf, 0xe5, 0x2b, 0xec, 0x97, 0xf3, 0xe0, 0xd1, 0x90,
  0xc1, 0x03, 0x14, 0x8d, 0xd9, 0xe5, 0x19, 0xab, 0x83, 0xb3, 0x97, 0x03, 0x23, 0x8f, 0x2d, 0x69,
  0x8f, 0x22, 0x64, 0xa1, 0x69, 0x55, 0xf9, 0x8d, 0x31, 0x55, 0x06, 0x28, 0xfa, 0x0c, 0x55, 0x03,
  0xf7, 0x53, 0xdf, 0xbf, 0xcd, 0xa9, 0xd5, 0xb8, 0xa5, 0xe5, 0x8d, 0xcf, 0xb9, 0x05, 0x03, 0x37,
  0xef, 0xec, 0x47, 0xc5, 0xf5, 0x19, 0x4b, 0xe7, 0x7f, 0x91, 0x1f, 0x5b, 0xf7, 0x3d, 0xf5, 0x17,
  0x73, 0x41, 0x4b, 0x3d, 0x28, 0x29, 0x03, 0xf7, 0xbf, 0xd1, 0xdf, 0x56, 0x93, 0x91, 0xa1, 0xd5
};

HANDLE spawn_target_disk_handle(const char *device_path)
{
  HANDLE hDevice, ret;
  DISK_GEOMETRY desc;
  DWORD idx, x;
  BOOL result;
  BYTE buf[GENERIC_STACK_BUF_SIZE];
  int32_t disk_id;
  
  uint8_t test_data[RAW_CHUNK_SZ + RS_CHUNK_SZ];
  
  ret = INVALID_HANDLE_VALUE;
  
  hDevice = CreateFileA(
    device_path,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );

  if (hDevice == INVALID_HANDLE_VALUE)
  {
    printf("Failed to open device for Disk ID query\n");

    return INVALID_HANDLE_VALUE;
  }

  memset(&desc, 0, sizeof(DISK_GEOMETRY));
  // Query the disk properties (specifically the Disk ID)
  result = DeviceIoControl(
    hDevice,
    IOCTL_DISK_GET_DRIVE_GEOMETRY,
    NULL,
    0,
    &desc,
    sizeof(DISK_GEOMETRY),
    &x,
    NULL
  );

  if(!result)
  {
    printf("Failed to query disk deometry (0x%lx)\n", GetLastError());
      
    CloseHandle(hDevice);
    return INVALID_HANDLE_VALUE;
  }

  printf("IsUSB           = %d\n", desc.MediaType == RemovableMedia);
  printf("Cylinders       = %lld\n", desc.Cylinders.QuadPart); // Bad SDCard => 16317
  printf("Tracks/cylinder = %ld\n", desc.TracksPerCylinder); // 255
  printf("Sectors/track   = %ld\n", desc.SectorsPerTrack); // 63
  printf("Bytes/sector    = %ld\n", desc.BytesPerSector); // 512
    
  if(memcmp(&desc, &target, sizeof(DISK_GEOMETRY)))
  {
    // Not the target!
      
    for(BYTE i = 0 ; i < x ; i++)
    {
      printf("%02X ", ((BYTE *)&desc)[i]);
    }
    printf("\n");

    for(BYTE i = 0 ; i < x ; i++)
    {
      printf("%02X ", ((BYTE *)&target)[i]);
    }
    printf("\n");
      
    CloseHandle(hDevice);

    return INVALID_HANDLE_VALUE;
  }

  disk_id = get_disk_id(hDevice);
  CloseHandle(hDevice);

  if(disk_id < 0)
  {
    return INVALID_HANDLE_VALUE;
  }
  printf("Candidate disk ID = %d\n", disk_id);

  // 0x400 in size should be way over the length limit of such a string.
  sprintf(buf, "\\\\.\\PhysicalDrive%d", disk_id);

  // Read-only first. The program will later check for the first "ECC block" to make sure its what we want to read+write.
  hDevice = CreateFileA(
    buf,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );
  if (hDevice == INVALID_HANDLE_VALUE)
  {
    printf("Failed to open device for head chunk test!\n");

    return INVALID_HANDLE_VALUE;
  }
  
  result = ReadFile(hDevice, test_data, RAW_CHUNK_SZ + RS_CHUNK_SZ, &x, NULL);
  if (!result)
  {
    printf("Failed to read the head chunk for testing!\n");

    CloseHandle(hDevice);
    return INVALID_HANDLE_VALUE;
  }
  
  CloseHandle(hDevice);
  
  ecc_read_chunk(test_data, &test_data[RAW_CHUNK_SZ]);
  if(memcmp(test_data, magic_bytes, sizeof(magic_bytes)))
  {
    printf("Magic bytes mismatch!\n");
    return INVALID_HANDLE_VALUE;
  }
  
  return CreateFileA(
    buf,
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );
}
