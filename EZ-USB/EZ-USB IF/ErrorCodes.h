#pragma once

enum class eErrorCode
{
    eOk = 0,
    eFailed,
    eMemoryAllocationFailed,
    eCreateFileFailed,
    eInitializeFailed,
    eNotOpen,
    eGetDescriptorFailed,
    eControlTransferFailed,
    eBufferSizeMismatch,
    eSetupDiEnumDeviceInterfacesFailed,
    eSetupDiGetDeviceInterfaceDetailFailed,
    eSetupDiGetClassDevsFailed,
};