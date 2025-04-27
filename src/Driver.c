// src/Driver.c

#include <ntddk.h>
#include <wdf.h>
#include "../inc/Communication.h"
#include "../inc/HypervisorSetup.h"
#include "../inc/ProcessNotification.h"
#include "../inc/SyscallFilter.h"

DRIVER_INITIALIZE        DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD     EvtDriverUnload;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    config.EvtDriverUnload = EvtDriverUnload;

    return WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );
}

NTSTATUS
EvtDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);
    NTSTATUS status;

    //
    // 1) Give the device an NT name: "\Device\SyscallMonitor"
    //
    static const UNICODE_STRING ntName = RTL_CONSTANT_STRING(L"\\Device\\SyscallMonitor");
    status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceInitAssignName failed 0x%X\n", status));
        return status;
    }

    //
    // 2) Create the WDFDEVICE
    //
    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);

    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &attrs, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceCreate failed 0x%X\n", status));
        return status;
    }

    //
    // 3) Create the symbolic link "\DosDevices\SyscallMonitor"
    //
    static const UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\SyscallMonitor");
    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceCreateSymbolicLink failed 0x%X\n", status));
        return status;
    }

    //
    // 4) Now you can set up IOCTL queue, hypervisor, etc.
    //
    status = RegisterIoctlHandlers(device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: RegisterIoctlHandlers failed 0x%X\n", status));
        return status;
    }

    status = SetupCommunication(device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: SetupCommunication failed 0x%X\n", status));
        return status;
    }

    status = InitializeHypervisor();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: InitializeHypervisor failed 0x%X\n", status));
        return status;
    }

    status = RegisterProcessNotifications();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: RegisterProcessNotifications failed 0x%X\n", status));
        return status;
    }

    status = InitializeSyscallFilter();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: InitializeSyscallFilter failed 0x%X\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
EvtDriverUnload(
    _In_ WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);

    CleanupSyscallFilter();
    UnregisterProcessNotifications();
    ShutdownHypervisor();
    CleanupCommunication();
}
// src/Driver.c

#include <ntddk.h>
#include <wdf.h>
#include "../inc/Communication.h"
#include "../inc/HypervisorSetup.h"
#include "../inc/ProcessNotification.h"
#include "../inc/SyscallFilter.h"

DRIVER_INITIALIZE        DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD     EvtDriverUnload;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    config.EvtDriverUnload = EvtDriverUnload;

    return WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );
}

NTSTATUS
EvtDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);
    NTSTATUS status;

    //
    // 1) Give the device an NT name: "\Device\SyscallMonitor"
    //
    static const UNICODE_STRING ntName = RTL_CONSTANT_STRING(L"\\Device\\SyscallMonitor");
    status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceInitAssignName failed 0x%X\n", status));
        return status;
    }

    //
    // 2) Create the WDFDEVICE
    //
    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);

    WDFDEVICE device;
    status = WdfDeviceCreate(&DeviceInit, &attrs, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceCreate failed 0x%X\n", status));
        return status;
    }

    //
    // 3) Create the symbolic link "\DosDevices\SyscallMonitor"
    //
    static const UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\SyscallMonitor");
    status = WdfDeviceCreateSymbolicLink(device, &symLink);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: WdfDeviceCreateSymbolicLink failed 0x%X\n", status));
        return status;
    }

    //
    // 4) Now you can set up IOCTL queue, hypervisor, etc.
    //
    status = RegisterIoctlHandlers(device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: RegisterIoctlHandlers failed 0x%X\n", status));
        return status;
    }

    status = SetupCommunication(device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: SetupCommunication failed 0x%X\n", status));
        return status;
    }

    status = InitializeHypervisor();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: InitializeHypervisor failed 0x%X\n", status));
        return status;
    }

    status = RegisterProcessNotifications();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: RegisterProcessNotifications failed 0x%X\n", status));
        return status;
    }

    status = InitializeSyscallFilter();
    if (!NT_SUCCESS(status)) {
        KdPrint(("Driver: InitializeSyscallFilter failed 0x%X\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
EvtDriverUnload(
    _In_ WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);

    CleanupSyscallFilter();
    UnregisterProcessNotifications();
    ShutdownHypervisor();
    CleanupCommunication();
}
