/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <windows.h>

// Modify these based on your need!

// 4 KiB * 0x300000 = 12 GiB visible size
#define MAX_BLOCK_COUNT 0x300000

// Skips the first 16MiB of the device
#define SKIP_HEADER_SIZE 0x1000000

HANDLE spawn_target_disk_handle(const char *device_path);
