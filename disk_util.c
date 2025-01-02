/**
 * Copyright (c) 2025, tan2pow16 (https://github.com/tan2pow16)
 *
 * This software library is licensed under terms of GPLv3
 */

#include "disk_util.h"

#include <winuser.h>
#include <setupapi.h>
#include <winioctl.h>
#include <dbt.h>
#include <stdio.h>

#include "constants.h"
#include "spd_proc.h"
#include "custom_test.h"

const GUID GUID_DEVINTERFACE_DISK = {0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b};

// The MAX_PATH is 260 and this should be more than enough to hold even the WCHAR form.
//  Please correct me if I am wrong.
char singleton_device_path[0x400];

HANDLE try_setup_device_by_path(const char *dev_path)
{
  HANDLE ret;
  uint16_t s;
  
  printf("Device found: %s\n", dev_path);
  ret = spawn_target_disk_handle(dev_path);
  if(ret == INVALID_HANDLE_VALUE)
  {
    return INVALID_HANDLE_VALUE;
  }
    
  for(s = 0 ; dev_path[s] ; s++)
  {
    singleton_device_path[s] = dev_path[s];
    if(singleton_device_path[s] >= 0x61 && singleton_device_path[s] <= 0x7A)
    {
      singleton_device_path[s] -= 0x20; // To lower case
    }
  }
  
  return ret;
}

HANDLE get_target_disk_handle()
{
  HDEVINFO hDevInfoSet;
  SP_DEVICE_INTERFACE_DATA ifdata;
  BYTE buf[GENERIC_STACK_BUF_SIZE];
  SP_DEVICE_INTERFACE_DETAIL_DATA_A *pDetail;
  DWORD idx;
  BOOL result;

  HANDLE ret;
  uint16_t s;

  ret = INVALID_HANDLE_VALUE;

  //get a handle to a device information set
  hDevInfoSet = SetupDiGetClassDevs(
    &GUID_DEVINTERFACE_DISK,    // class GUID
    NULL,    // Enumerator
    NULL,    // hwndParent
    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE  // present devices
  );
 
  //fail...
  if (hDevInfoSet == INVALID_HANDLE_VALUE)
  {
    fprintf(stderr, "IOCTL_STORAGE_GET_DEVICE_NUMBER Error: %ld\n", GetLastError());
    return INVALID_HANDLE_VALUE;
  }
 
  pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)&buf[0];

  idx = 0;

  // device index = 0, 1, 2... test the device interface one by one
  while(TRUE)
  {
    ifdata.cbSize = sizeof(ifdata);
 
    //enumerates the device interfaces that are contained in a device information set
    result = SetupDiEnumDeviceInterfaces(
      hDevInfoSet,   // DeviceInfoSet
      NULL,      // DeviceInfoData
      &GUID_DEVINTERFACE_DISK,      // GUID
      idx,   // MemberIndex
      &ifdata    // DeviceInterfaceData
    );

    if (!result)
    {
      break;
    }

    memset(pDetail, 0, GENERIC_STACK_BUF_SIZE);
    pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    // get details about a device interface
    result = SetupDiGetDeviceInterfaceDetailA(
      hDevInfoSet,  // DeviceInfoSet
      &ifdata,    // DeviceInterfaceData
      pDetail,    // DeviceInterfaceDetailData
      GENERIC_STACK_BUF_SIZE,  // DeviceInterfaceDetailDataSize
      NULL,       // RequiredSize
      NULL      // DeviceInfoData
    );

    if (!result)
    {
      idx++;
      continue;
    }
    
    ret = try_setup_device_by_path(pDetail->DevicePath);
    if(ret == INVALID_HANDLE_VALUE)
    {
      idx++;
      continue;
    }
    
    break;
  }

  SetupDiDestroyDeviceInfoList(hDevInfoSet);

  return ret;
}

int32_t get_disk_id(HANDLE h_dev)
{
  STORAGE_DEVICE_NUMBER disk_idx;
  DWORD sz;
  
  if(!DeviceIoControl(
    h_dev,
    IOCTL_STORAGE_GET_DEVICE_NUMBER,
    NULL,
    0,
    &disk_idx,
    sizeof(STORAGE_DEVICE_NUMBER),
    &sz,
    NULL
  ))
  {
    printf("Failed to query disk ID (0x%lx)\n", GetLastError());
    return -1; // Valid disk index is always a small positive integer, so treating negative values as invalid should be fine.
  }

  return disk_idx.DeviceNumber;
}

// Modified from https://stackoverflow.com/questions/16214768/detecting-usb-insertion-removal-in-c-non-gui-application

#define LISTENER_CLS_NAME "ECC_DRIVE_CLASS"
#define LISTENER_WND_NAME "ECC_DRIVE_LISTENER"
#define HWND_MESSAGE ((HWND)-3)

LRESULT usb_event_handler(HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
  switch (uint)
  {
    case WM_NCCREATE:
    {
      // before window creation
      return TRUE;
    }
    case WM_CREATE:
    {
      // The actual creation of the window
      //  you can get your creation params here..like GUID..
      LPCREATESTRUCT params = (LPCREATESTRUCT) lparam;
      GUID InterfaceClassGuid;
      memcpy(&InterfaceClassGuid, params->lpCreateParams, sizeof(GUID));
      DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
      memset(&NotificationFilter, 0, sizeof(NotificationFilter));
      NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
      NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
      memcpy(&(NotificationFilter.dbcc_classguid), &GUID_DEVINTERFACE_DISK, sizeof(GUID));
      HDEVNOTIFY dev_notify = RegisterDeviceNotification(hwnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
      
      if(dev_notify == NULL)
      {
        printf("Cannot register USB listener!\n");
      }
      break;
    }
    case WM_DEVICECHANGE:
    {
      PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR) lparam;
      PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE) lpdb;

      if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
      {
        switch (wparam)
        {
          case DBT_DEVICEARRIVAL:
          {
            HANDLE h_dev;
            h_dev;
            
            if(p_disk_singleton)
            {
              // Custom disk already active.
              break;
            }
            
            h_dev = try_setup_device_by_path(lpdbv->dbcc_name);
            if(h_dev == INVALID_HANDLE_VALUE)
            {
              break;
            }

            // Only a try.
            if(create_disk(h_dev))
            {
              printf("Successfully captured the target device!\n");
            }

            break;
          }
          case DBT_DEVICEREMOVECOMPLETE:
          {
            uint16_t s;
            BOOL found;
            
            if(!p_disk_singleton)
            {
              // Custom disk in not active.
              break;
            }
            
            s = 0;
            found = FALSE;
            while(TRUE)
            {
              if(lpdbv->dbcc_name[s] != singleton_device_path[s])
              {
                if(lpdbv->dbcc_name[s] < 0x61 || lpdbv->dbcc_name[s] > 0x7A || (lpdbv->dbcc_name[s] - 0x20) != singleton_device_path[s])
                {
                  break;
                }
              }
              
              // Make sure the NULL character is also checked!
              if(!singleton_device_path[s])
              {
                found = TRUE;
                break;
              }
              
              s++;
            }
            
            if(found)
            {
              spd_force_shutdown();
            }
            
            break;
          }
        }
      }
      break;
    }
  }
  return FALSE;
}

BOOL setup_usb_event_listener(void)
{
  HWND hWnd = NULL;
  WNDCLASSEX wx;
  GUID dummy;
  MSG msg;

  memset(&wx, 0, sizeof(wx));
  wx.cbSize = sizeof(WNDCLASSEX);
  wx.lpfnWndProc = usb_event_handler;
  wx.style = CS_HREDRAW | CS_VREDRAW;
  wx.hInstance = GetModuleHandle(0);
  wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wx.lpszClassName = LISTENER_CLS_NAME;
  
  if (!RegisterClassEx(&wx))
  {
    printf("Cannot register USB listener class!\n");
    return FALSE;
  }
  
  memcpy(&dummy, &GUID_DEVINTERFACE_DISK, sizeof(GUID));
  hWnd = CreateWindow(
    LISTENER_CLS_NAME,
    LISTENER_WND_NAME,
    WS_ICONIC,
    0,
    0,
    CW_USEDEFAULT,
    0,
    HWND_MESSAGE,
    NULL,
    GetModuleHandle(0),
    &dummy
  );

  if(!hWnd)
  {
    printf("Cannot register USB listener window!\n");
    return FALSE;
  }

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return TRUE;
}
