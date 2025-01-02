// https://stackoverflow.com/questions/16214768/detecting-usb-insertion-removal-in-c-non-gui-application

#define ANSI
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT   0x0501

#include <windows.h>
#include <winuser.h>
#include <initguid.h>
#include <usbiodef.h>
#include <Dbt.h>

#include <string>
#include <iostream>
#include <stdexcept>

// {0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}}
#define HID_CLASSGUID {0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}
#define CLS_NAME "DUMMY_CLASS"
#define HWND_MESSAGE ((HWND)-3)

LRESULT message_handler(HWND__* hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
    GUID guid_disk = HID_CLASSGUID;

    switch (uint)
    {
    case WM_NCCREATE: // before window creation
        return true;
        break;

    case WM_CREATE: // the actual creation of the window
    {
        // you can get your creation params here..like GUID..
        LPCREATESTRUCT params = (LPCREATESTRUCT) lparam;
        GUID InterfaceClassGuid = *((GUID*)params->lpCreateParams);
        DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
        ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
        NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        memcpy(&(NotificationFilter.dbcc_classguid),&(guid_disk), sizeof(struct _GUID));
        HDEVNOTIFY dev_notify = RegisterDeviceNotification(hwnd, &NotificationFilter,
                                                           DEVICE_NOTIFY_WINDOW_HANDLE);
        if(dev_notify == NULL)
        {
            throw std::runtime_error("Could not register for devicenotifications!");
        }
        break;
    }

    case WM_DEVICECHANGE:
    {
        PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR) lparam;
        PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE) lpdb;
        std::string path;
        if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
            path = std::string(lpdbv->dbcc_name);
            switch (wparam)
            {
            case DBT_DEVICEARRIVAL:
                //std::cout << "new device connected: " << path << "\n";
                printf("%s\n", lpdbv->dbcc_name);
                break;

            case DBT_DEVICEREMOVECOMPLETE:
                // std::cout << "device disconnected: " << path << "\n";
                printf("%s\n", lpdbv->dbcc_name);
                break;
            }
        }
        break;
    }

    }
    return 0L;
}

int main(int argc, char* argv[])
{
    HWND hWnd = NULL;
    WNDCLASSEX wx;
    ZeroMemory(&wx, sizeof(wx));

    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = reinterpret_cast<WNDPROC>(message_handler);
    wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
    wx.style = CS_HREDRAW | CS_VREDRAW;
    wx.hInstance = GetModuleHandle(0);
    wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wx.lpszClassName = CLS_NAME;

    GUID guid = HID_CLASSGUID;

    if (RegisterClassEx(&wx))
    {
        hWnd = CreateWindow(CLS_NAME, "DevNotifWnd", WS_ICONIC,
                            0, 0, CW_USEDEFAULT, 0, HWND_MESSAGE,
                            NULL, GetModuleHandle(0), (void*)&guid);
    }

    if(hWnd == NULL)
    {
        throw std::runtime_error("Could not create message window!");
    }

    std::cout << "waiting for new devices..\n";

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}