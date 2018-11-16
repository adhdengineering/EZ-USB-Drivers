#include "pch.h"

#include <stdio.h>
#include <iostream>

void doBulkReadTest(Device &device);

LONG __cdecl _tmain( LONG Argc, LPTSTR* Argv)
{
    Device device;
    USB_DEVICE_DESCRIPTOR deviceDesc;
    BOOL                  noDevice;
    eErrorCode e;

    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

    //
    // Find a device connected to the system that has WinUSB installed using our
    // INF
    //
    if (eErrorCode::eOk != (e = device.Open(&noDevice)))
    {
        if (noDevice) 
        {
            printf(_T("Device not connected or driver not installed\n"));

        } 
        else
        {
            printf(_T("Failed looking for device, error code is 0x%x\n"), e);
        }
        std::cin.get();
        return 0;
    }

    //
    // Get device descriptor
    //
    if (eErrorCode::eOk != (e = device.GetDeviceDescriptor(deviceDesc)))
    {
        printf(_T("GetDeviceDescriptor LastError %d"), device.GetLastError());
        device.Close();
        std::cin.get();
        return 0;
    }

    uint8_t state = 0;

    if (eErrorCode::eOk == (e = device.VendorRequestRead(0x1, 0, 0, &state, 1)))
    {
        state ^= 0x1;
        if (eErrorCode::eOk != (e = device.VendorRequestWrite(0x1, 0, 0, &state, 1)))
        {
            printf(_T("VendorRequestWrite LastError %d"), device.GetLastError());
        }
    }
    else
    {
        printf(_T("VendorRequestRead LastError %d"), device.GetLastError());
    }

    //
    // Print a few parts of the device descriptor
    //
    printf(_T("Device found: VID_%04X&PID_%04X; bcdUsb %04X\n"),
           deviceDesc.idVendor,
           deviceDesc.idProduct,
           deviceDesc.bcdUSB);

    doBulkReadTest(device);

    device.Close();

    std::cin.get();
    return 0;
}

void doBulkReadTest(Device &device)
{
    eErrorCode e;
    uint32_t size = 4096;
    if (eErrorCode::eOk != (e = device.VendorRequestWrite(0x1, 0, 0, (uint8_t*)&size, sizeof(uint32_t))))
    {
        printf(_T("doBulkReadTest VendorRequestWrite LastError %d"), device.GetLastError());
        return;
    }
    uint8_t *data = new uint8_t[size];

    if (eErrorCode::eOk != (e = device.BulkRead(eEndPoint::EP6, data, size)))
    {
        printf(_T("doBulkReadTest BulkRead LastError %d"), device.GetLastError());
    }

    delete[] data;
}

