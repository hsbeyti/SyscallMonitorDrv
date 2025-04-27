// src/Driver.c
#include <ntddk.h>
#include <wdf.h>
#include "../inc/Communication.h"
#include "../inc/HypervisorSetup.h"
#include "../inc/ProcessNotification.h"
#include "../inc/SyscallFilter.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

NTSTATUS DriverEntry(PDRIVER_OBJECT drvObj, PUNICODE_STRING regPath) {
    WDF_DRIVER_CONFIG cfg;
    WDF_DRIVER_CONFIG_INIT(&cfg, EvtDeviceAdd);
    cfg.EvtDriverUnload = EvtDriverUnload;
    return WdfDriverCreate(drvObj, regPath, WDF_NO_OBJECT_ATTRIBUTES, &cfg, WDF_NO_HANDLE);
}

NTSTATUS EvtDeviceAdd(WDFDRIVER driver, PWDFDEVICE_INIT pInit) {
    UNREFERENCED_PARAMETER(driver);
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&attrs);
    status = WdfDeviceCreate(&pInit, &attrs, &device);
    if (!NT_SUCCESS(status)) return status;
    status = SetupCommunication(device);
    if (!NT_SUCCESS(status)) return status;
    status = InitializeHypervisor();
    if (!NT_SUCCESS(status)) return status;
    status = RegisterProcessNotifications();
    if (!NT_SUCCESS(status)) return status;
    status = InitializeSyscallFilter();
    if (!NT_SUCCESS(status)) return status;
    return STATUS_SUCCESS;
}

void EvtDriverUnload(WDFDRIVER driver) {
    UNREFERENCED_PARAMETER(driver);
    ShutdownHypervisor();
    UnregisterProcessNotifications();
    CleanupSyscallFilter();
    CleanupCommunication();
}
