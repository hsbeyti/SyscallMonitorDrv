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

#define DRIVER_TAG "SysMonDrv"
#define LOG_RAW(msg) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, DRIVER_TAG ": %s\n", msg)
#define LOG(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, DRIVER_TAG ": " fmt "\n", __VA_ARGS__)

// Keep the WDFDEVICE around in case unload needs it
static WDFDEVICE g_Device = NULL;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT   DriverObject,
    _In_ PUNICODE_STRING  RegistryPath
)
{
    LOG_RAW("DriverEntry called");

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    config.EvtDriverUnload = EvtDriverUnload;

    NTSTATUS status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        LOG("WdfDriverCreate failed: 0x%X", status);
    }
    else {
        LOG_RAW("WdfDriverCreate succeeded");
    }

    return status;
}

NTSTATUS
EvtDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);
    LOG_RAW("EvtDeviceAdd called");

    NTSTATUS status;

    //
    // 1) Allocate a CONTROL device init so user-mode can open \\.\SyscallMonitor
    //

    PWDFDEVICE_INIT controlInit =
        
        WdfControlDeviceInitAllocate(DeviceInit,
            L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GR;;;WD)");

    if (controlInit == NULL) {
        LOG_RAW("WdfControlDeviceInitAllocate failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 2) Use buffered I/O (matches your METHOD_BUFFERED IOCTLs)
    WdfDeviceInitSetIoType(controlInit, WdfDeviceIoBuffered);

    // 3) Name the device \Device\SyscallMonitor
    static CONST UNICODE_STRING ntName =
        RTL_CONSTANT_STRING(L"\\Device\\SyscallMonitor");
    status = WdfDeviceInitAssignName(controlInit, &ntName);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceInitAssignName failed: 0x%X", status);
        WdfDeviceInitFree(controlInit);
        return status;
    }

    // 4) Create the device object
    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);

    status = WdfDeviceCreate(&controlInit, &attrs, &g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceCreate (control) failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Control device created");

    // 5) Create the DOS symbolic link \\DosDevices\\SyscallMonitor
    static CONST UNICODE_STRING symLink =
        RTL_CONSTANT_STRING(L"\\DosDevices\\SyscallMonitor");
    status = WdfDeviceCreateSymbolicLink(g_Device, &symLink);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceCreateSymbolicLink failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Symbolic link created");

    //
    // 6) Set up the IOCTL queue, hypervisor, process notifications, and filter
    //
    status = RegisterIoctlHandlers(g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("RegisterIoctlHandlers failed: 0x%X", status);
        return status;
    }
    LOG_RAW("IOCTL handlers registered");

    status = SetupCommunication(g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("SetupCommunication failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Communication setup complete");

    status = InitializeHypervisor();
    if (!NT_SUCCESS(status)) {
        LOG("InitializeHypervisor failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Hypervisor initialized");

    status = RegisterProcessNotifications();
    if (!NT_SUCCESS(status)) {
        LOG("RegisterProcessNotifications failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Process notifications registered");

    status = InitializeSyscallFilter();
    if (!NT_SUCCESS(status)) {
        LOG("InitializeSyscallFilter failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Syscall filter initialized");

    //
    // 7) Finish initialization so IRP_MJ_CREATE and IOCTLs start flowing
    //
    WdfControlFinishInitializing(g_Device);
    LOG_RAW("Control device initialized and ready");

    return STATUS_SUCCESS;
}

VOID
EvtDriverUnload(
    _In_ WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);
    LOG_RAW("EvtDriverUnload called");

    // Reverse order cleanup
    CleanupSyscallFilter();
    LOG_RAW("Syscall filter cleaned");

    UnregisterProcessNotifications();
    LOG_RAW("Process notifications unregistered");

    ShutdownHypervisor();
    LOG_RAW("Hypervisor shut down");

    CleanupCommunication();
    LOG_RAW("Communication cleaned up");

    // KMDF will delete the control device object and link automatically
    LOG_RAW("Driver unload completed successfully");
}
