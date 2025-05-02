#include <ntddk.h>
#include <wdf.h>

#include "../inc/Communication.h"
#include "../inc/HypervisorSetup.h"
#include "../inc/ProcessNotification.h"
#include "../inc/SyscallFilter.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

#define LOG_RAW(msg) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[SyscallMonitorDrv] %s\n", msg)

#define LOG(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[SyscallMonitorDrv] " fmt "\n", __VA_ARGS__)

// Global device handle for cleanup if needed
static WDFDEVICE g_Device = NULL;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    config.EvtDriverUnload = EvtDriverUnload;

    LOG_RAW("DriverEntry called");

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
    UNREFERENCED_PARAMETER(Driver);
    LOG_RAW("EvtDeviceAdd called");

    NTSTATUS status;

    static const UNICODE_STRING ntName = RTL_CONSTANT_STRING(L"\\Device\\SyscallMonitor");
    static const UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\SyscallMonitor");

    status = WdfDeviceInitAssignName(DeviceInit, &ntName);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceInitAssignName failed: 0x%X", status);
        return status;
    }

    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);

    status = WdfDeviceCreate(&DeviceInit, &attrs, &g_Device);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceCreate failed: 0x%X", status);
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(g_Device, &symLink);
    if (!NT_SUCCESS(status)) {
        LOG("WdfDeviceCreateSymbolicLink failed: 0x%X", status);
        return status;
    }

    // Step-by-step initialization
    if (!NT_SUCCESS(status = RegisterIoctlHandlers(g_Device))) {
        LOG("RegisterIoctlHandlers failed: 0x%X", status);
        return status;
    }
    LOG_RAW("IOCTL handlers registered");

    if (!NT_SUCCESS(status = SetupCommunication(g_Device))) {
        LOG("SetupCommunication failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Communication setup complete");

    if (!NT_SUCCESS(status = InitializeHypervisor())) {
        LOG("InitializeHypervisor failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Hypervisor initialized");

    if (!NT_SUCCESS(status = RegisterProcessNotifications())) {
        LOG("RegisterProcessNotifications failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Process notification registered");

    if (!NT_SUCCESS(status = InitializeSyscallFilter())) {
        LOG("InitializeSyscallFilter failed: 0x%X", status);
        return status;
    }
    LOG_RAW("Syscall filter initialized");

    LOG_RAW("Device successfully added");
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
    LOG_RAW("Syscall filter cleaned");

    UnregisterProcessNotifications();
    LOG_RAW("Process notifications unregistered");

    ShutdownHypervisor();
    LOG_RAW("Hypervisor shutdown");

    CleanupCommunication();
    LOG_RAW("Communication cleaned");

    LOG_RAW("Driver unload completed successfully");
}
