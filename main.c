/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include <windows.h>
#include <stdio.h>

#include "disk_util.h"
#include "spd_proc.h"
#include "rscode/ecc.h"

int main()
{
  HANDLE r;

  // MUST be the first!
  initialize_ecc();

  r = get_target_disk_handle();
  if(r != INVALID_HANDLE_VALUE)
  {
    if(!create_disk(r))
    {
      printf("Create disk failed. Starting a USB listener instead.\n");
      CloseHandle(r);
    }
  }
  else
  {
    printf("Bad target handle. Starting a USB listener instead.\n");
  }

  if(!setup_usb_event_listener())
  {
    printf("Cannot setup USB listener.\n");
    return 1;
  }

  return 0;
}
