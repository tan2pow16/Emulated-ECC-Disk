/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <stdint.h>
#include <windows.h>

HANDLE get_target_disk_handle();
int32_t get_disk_id(HANDLE h_dev);
BOOL setup_usb_event_listener(void);
