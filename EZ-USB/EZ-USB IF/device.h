//
// Define below GUIDs
//
#pragma once

#include <stdint.h>
#include "ErrorCodes.h"

enum class eEndPoint : unsigned char {
    EP1 = 1,
    EP2 = 2,
    EP3 = 3,
    EP4 = 4,
    EP5 = 5,
    EP6 = 6,
    EP7 = 7,
    EP8 = 8,
    EP_READ = 0x80,
};
inline eEndPoint operator|(const eEndPoint &lhs, const eEndPoint &rhs) {
    unsigned char l = (unsigned char)lhs;
    unsigned char r = (unsigned char)rhs;
    return (eEndPoint)(l | r);
}

class Device
{
public:

public:
    eErrorCode Open(PBOOL FailureDeviceNotFound);
    eErrorCode Close();

    eErrorCode GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR &deviceDesc);
    eErrorCode VendorRequestWrite(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* Buffer, size_t BufferSize);
    eErrorCode VendorRequestRead(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* Buffer, size_t BufferSize);
    eErrorCode BulkRead(eEndPoint ep, uint8_t* buffer, uint32_t size);
    eErrorCode BulkWrite(eEndPoint ep, uint8_t* buffer, uint32_t size);

    int GetLastError() { return LastError; }
private:
    eErrorCode RetrieveDevicePath(PBOOL FailureDeviceNotFound);

private:
    int LastError;
private:
    BOOL                    HandlesOpen;
    WINUSB_INTERFACE_HANDLE WinusbHandle;
    HANDLE                  DeviceHandle;
    TCHAR                   DevicePath[MAX_PATH];
};
