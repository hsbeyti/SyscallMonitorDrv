// src/Driver.c

#include <ntddk.h>
#include <wdf.h>
#include <sddl.h>      // For SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R
#include <intrin.h>    // For __debugbreak()

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

// Global device handle for cleanup if needed
static WDFDEVICE g_Device = NULL;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    DbgPrint(">>> SysMonDrv: Entered DriverEntry\n");
    // __debugbreak();  // Uncomment if attaching kernel debugger

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
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(DeviceInit);
    LOG_RAW("EvtDeviceAdd called");

    NTSTATUS status;

    // Allocate a CONTROL device for user-mode interface
    PWDFDEVICE_INIT controlInit =
        WdfControlDeviceInitAllocate(DeviceInit,
            L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GR;;;WD)");
    if (controlInit == NULL) {
        LOG_RAW("WdfControlDeviceInitAllocate failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WdfDeviceInitSetIoType(controlInit, WdfDeviceIoBuffered);

    static const UNICODE_STRING ntName = RTL_CONSTANT_STRING(L"\\Device\\SyscallMonitor");
    status = WdfDeviceInitAssignName(controlInit, &ntName);
    if (!NT_SUCCESS(status)) {
        LOG("AssignName failed: 0x%X", status);
        WdfDeviceInitFree(controlInit);
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);

    status = WdfDeviceCreate(&controlInit, &attrs, &g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceCreate failed: 0x%X", status);
        return status;
    }

    static const UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\SyscallMonitor");
    status = WdfDeviceCreateSymbolicLink(g_Device, &symLink);
    if (!NT_SUCCESS(status)) {
        LOG("SymbolicLink creation failed: 0x%X", status);
        return status;
    }

    LOG_RAW("Control device and symbolic link created");

    // OPTIONAL: Only enable IOCTLs, comm, hypervisor AFTER stable testing
    status = RegisterIoctlHandlers(g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("RegisterIoctlHandlers failed: 0x%X", status);
        return status;
    }

    status = SetupCommunication(g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("SetupCommunication failed: 0x%X", status);
        return status;
    }

    status = InitializeHypervisor();
    if (!NT_SUCCESS(status)) {
        LOG("InitializeHypervisor failed: 0x%X", status);
        return status;
    }

    status = RegisterProcessNotifications();
    if (!NT_SUCCESS(status)) {
        LOG("RegisterProcessNotifications failed: 0x%X", status);
        return status;
    }

    status = InitializeSyscallFilter();
    if (!NT_SUCCESS(status)) {
        LOG("InitializeSyscallFilter failed: 0x%X", status);
        return status;
    }

    WdfControlFinishInitializing(g_Device);
    LOG_RAW("Driver fully initialized");
    return STATUS_SUCCESS;
}

VOID
EvtDriverUnload(
    _In_ WDFDRIVER Driver
)
{
    UNREFERENCED_PARAMETER(Driver);
    LOG_RAW("EvtDriverUnload called");

    CleanupSyscallFilter();
    UnregisterProcessNotifications();
    ShutdownHypervisor();
    CleanupCommunication();

    LOG_RAW("Driver cleanup complete");
}
