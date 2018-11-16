#include "pch.h"

#include <SetupAPI.h>
#include <initguid.h>
//
// Device Interface GUID.
// Used by all WinUsb devices that this application talks to.
// Must match "DeviceInterfaceGUIDs" registry value specified in the INF file.
// a7adc3fd-2928-4846-952b-b527ab035296
//
DEFINE_GUID(GUID_DEVINTERFACE_EZUSBIF,
    0xa7adc3fd, 0x2928, 0x4846, 0x95, 0x2b, 0xb5, 0x27, 0xab, 0x03, 0x52, 0x96);

eErrorCode Device::Open(PBOOL FailureDeviceNotFound)
{
    BOOL    bResult;

    HandlesOpen = FALSE;

    eErrorCode e = RetrieveDevicePath(FailureDeviceNotFound);
    if (e != eErrorCode::eOk)
    {
        return e;
    }

    DeviceHandle = CreateFile(DevicePath,
                            GENERIC_WRITE | GENERIC_READ,
                            FILE_SHARE_WRITE | FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                            NULL);

    if (INVALID_HANDLE_VALUE == DeviceHandle)
    {
        LastError = GetLastError();
        return eErrorCode::eCreateFileFailed;
    }

    bResult = WinUsb_Initialize(DeviceHandle, &WinusbHandle);

    if (FALSE == bResult)
    {
        LastError = GetLastError();
        CloseHandle(DeviceHandle);
        return eErrorCode::eInitializeFailed;
    }

    HandlesOpen = TRUE;
    return eErrorCode::eOk;
}

eErrorCode Device::Close()
{
    if (FALSE == HandlesOpen)
    {
        return eErrorCode::eNotOpen;
    }

    WinUsb_Free(WinusbHandle);
    CloseHandle(DeviceHandle);
    HandlesOpen = FALSE;

    return eErrorCode::eOk;
}

eErrorCode Device::GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR &deviceDesc)
{
    ULONG lengthReceived;

    BOOL bResult = WinUsb_GetDescriptor(WinusbHandle,
        USB_DEVICE_DESCRIPTOR_TYPE,
        0,
        0,
        (PBYTE)&deviceDesc,
        sizeof(deviceDesc),
        &lengthReceived);

    if (FALSE == bResult || lengthReceived != sizeof(deviceDesc))
    {
        LastError = GetLastError();
        return eErrorCode::eGetDescriptorFailed;
    }
    return eErrorCode::eOk;
}

eErrorCode Device::VendorRequestWrite(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* Buffer, size_t BufferSize)
{
    unsigned long BytesReceived = 0;

    WINUSB_SETUP_PACKET wsp = {
        0x40,
        bRequest,
        wValue,
        wIndex,
        static_cast<uint16_t>(BufferSize)
    };

    if (!WinUsb_ControlTransfer(WinusbHandle, wsp, Buffer, static_cast<ULONG>(BufferSize), &BytesReceived, NULL))
    {
        LastError = GetLastError();
        return eErrorCode::eControlTransferFailed;
    }

    if (BytesReceived != BufferSize)
    {
        LastError = GetLastError();
        return eErrorCode::eBufferSizeMismatch;
    }
    
    return eErrorCode::eOk;
}

eErrorCode Device::VendorRequestRead(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint8_t* Buffer, size_t BufferSize)
{
    unsigned long BytesReceived = 0;

    WINUSB_SETUP_PACKET wsp = {
        0x40 | 0x80,
        bRequest,
        wValue,
        wIndex,
        static_cast<uint16_t>(BufferSize)
    };

    if (!WinUsb_ControlTransfer(WinusbHandle, wsp, Buffer, static_cast<ULONG>(BufferSize), &BytesReceived, NULL))
    {
        LastError = GetLastError();
        return eErrorCode::eControlTransferFailed;
    }

    if (BytesReceived != BufferSize)
    {
        LastError = GetLastError();
        return eErrorCode::eBufferSizeMismatch;
    }

    return eErrorCode::eOk;
}

eErrorCode Device::BulkRead(eEndPoint ep, uint8_t* buffer, uint32_t size)
{
    ULONG Yes = 1;
    WinUsb_SetPipePolicy(WinusbHandle, static_cast<UCHAR>(ep | eEndPoint::EP_READ), ALLOW_PARTIAL_READS, sizeof(ULONG), &Yes);
    WinUsb_SetPipePolicy(WinusbHandle, static_cast<UCHAR>(ep | eEndPoint::EP_READ), AUTO_FLUSH, sizeof(ULONG), &Yes);
    WinUsb_SetPipePolicy(WinusbHandle, static_cast<UCHAR>(ep | eEndPoint::EP_READ), AUTO_CLEAR_STALL, sizeof(ULONG), &Yes);

    uint32_t readCount = 0;
    if (FALSE == WinUsb_ReadPipe(WinusbHandle, static_cast<UCHAR>(ep | eEndPoint::EP_READ), buffer, size, (PULONG)&readCount, NULL)) {
        return eErrorCode::eFailed;
    }

    return readCount == size ? eErrorCode::eOk : eErrorCode::eBufferSizeMismatch;
}

eErrorCode Device::BulkWrite(eEndPoint ep, uint8_t* buffer, uint32_t size)
{
    ULONG Yes = 1;
    WinUsb_SetPipePolicy(WinusbHandle, static_cast<UCHAR>(ep), AUTO_CLEAR_STALL, sizeof(ULONG), &Yes);

    uint32_t writeCount = 0;
    if (FALSE == WinUsb_WritePipe(WinusbHandle, static_cast<UCHAR>(ep), buffer, size, (PULONG)&writeCount, NULL)) {
        return eErrorCode::eFailed;
    }

    return writeCount == size ? eErrorCode::eOk : eErrorCode::eBufferSizeMismatch;
}


eErrorCode Device::RetrieveDevicePath(PBOOL FailureDeviceNotFound)
{
    BOOL                             bResult = FALSE;
    HDEVINFO                         deviceInfo;
    SP_DEVICE_INTERFACE_DATA         interfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
    ULONG                            length;
    ULONG                            requiredLength=0;
    HRESULT                          hr;

    if (NULL != FailureDeviceNotFound) {

        *FailureDeviceNotFound = FALSE;
    }

    //
    // Enumerate all devices exposing the interface
    //
    deviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_EZUSBIF, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        LastError = GetLastError();
        return eErrorCode::eSetupDiGetClassDevsFailed;
    }

    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    //
    // Get the first interface (index 0) in the result set
    //
    if (FALSE == SetupDiEnumDeviceInterfaces(deviceInfo, NULL, &GUID_DEVINTERFACE_EZUSBIF, 0, &interfaceData))
    {
        //
        // We would see this error if no devices were found
        //
        if (ERROR_NO_MORE_ITEMS == GetLastError() && NULL != FailureDeviceNotFound)
        {

            *FailureDeviceNotFound = TRUE;
        }

        LastError = GetLastError();
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return eErrorCode::eSetupDiEnumDeviceInterfacesFailed;
    }

    //
    // Get the size of the path string
    // We expect to get a failure with insufficient buffer
    //
    bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, NULL, 0, &requiredLength, NULL);
    if (FALSE == bResult && ERROR_INSUFFICIENT_BUFFER != ::GetLastError())
    {
        LastError = ::GetLastError();
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return eErrorCode::eSetupDiGetDeviceInterfaceDetailFailed;
    }

    //
    // Allocate temporary space for SetupDi structure
    //
    detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, requiredLength);
    if (NULL == detailData)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return eErrorCode::eMemoryAllocationFailed;
    }

    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    length = requiredLength;

    //
    // Get the interface's path string
    //
    bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, length, &requiredLength, NULL);

    if(FALSE == bResult)
    {
        LastError = GetLastError();
        LocalFree(detailData);
        SetupDiDestroyDeviceInfoList(deviceInfo);
        return eErrorCode::eSetupDiGetDeviceInterfaceDetailFailed;
    }

    //
    // Give path to the caller. SetupDiGetDeviceInterfaceDetail ensured
    // DevicePath is NULL-terminated.
    //
    hr = StringCbCopy(DevicePath,
                      sizeof(DevicePath),
                      detailData->DevicePath);

    LocalFree(detailData);
    SetupDiDestroyDeviceInfoList(deviceInfo);

    return eErrorCode::eOk;
}
