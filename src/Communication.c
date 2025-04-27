// src/Communication.c

#include "../inc/Communication.h"
#include <ntintsafe.h>
#include <wdf.h>
#include <wdfrequest.h>
#include <ntddk.h>    // for KeAcquireSpinLock, KeSetEvent, etc.

// Forward‐declare the IOCTL dispatch (match the KMDF typedef)
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL IoctlDispatch;

// Globals
static WDFDEVICE      g_device;
static PKEVENT        g_userEvent;
static KSPIN_LOCK     g_ringSpin;
static ULONG          g_ringHead, g_ringTail;
static SyscallRecord  g_ringBuffer[COMM_RING_SIZE];

//--------------------------------------------------------------------------------
// SetupCommunication: create a named event and init ring buffer
//--------------------------------------------------------------------------------
NTSTATUS
SetupCommunication(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS       status;
    HANDLE         evtHandle;
    UNICODE_STRING evtName;

    g_device = Device;
    KeInitializeSpinLock(&g_ringSpin);
    g_ringHead = g_ringTail = 0;

    // Create a notification event the user‐mode console opens by name
    RtlInitUnicodeString(&evtName, L"\\BaseNamedObjects\\SyscallTrapEvent");
    status = IoCreateNotificationEvent(&evtName, &evtHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Comm: IoCreateNotificationEvent failed 0x%X\n", status));
        return status;
    }

    // Convert handle to kernel event pointer
    status = ObReferenceObjectByHandle(
        evtHandle,
        EVENT_MODIFY_STATE,
        *ExEventObjectType,
        KernelMode,
        (PVOID*)&g_userEvent,
        NULL);
    ZwClose(evtHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Comm: ObReferenceObjectByHandle failed 0x%X\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

//--------------------------------------------------------------------------------
// CleanupCommunication: dereference the event
//--------------------------------------------------------------------------------
VOID
CleanupCommunication(
    VOID
)
{
    if (g_userEvent) {
        ObDereferenceObject(g_userEvent);
        g_userEvent = NULL;
    }
}

//--------------------------------------------------------------------------------
// RegisterIoctlHandlers: set up the default sequential queue for IOCTLs
//--------------------------------------------------------------------------------
NTSTATUS
RegisterIoctlHandlers(
    _In_ WDFDEVICE Device
)
{
    WDF_IO_QUEUE_CONFIG qcfg;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&qcfg, WdfIoQueueDispatchSequential);
    qcfg.EvtIoDeviceControl = IoctlDispatch;

    return WdfIoQueueCreate(
        Device,
        &qcfg,
        WDF_NO_OBJECT_ATTRIBUTES,
        NULL  // we don’t need to keep the WDFQUEUE handle
    );
}

//--------------------------------------------------------------------------------
// IoctlDispatch: handle Start, Stop, and GetData
//--------------------------------------------------------------------------------
VOID
IoctlDispatch(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    switch (IoControlCode) {

    case IOCTL_START_MONITOR:
    {
        PVOID buf;
        size_t bufLen;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buf, &bufLen);
        if (NT_SUCCESS(status) && bufLen >= sizeof(ULONG)) {
            ULONG pid = *(ULONG*)buf;
            extern NTSTATUS StartProcessMonitor(HANDLE);
            status = StartProcessMonitor((HANDLE)(ULONG_PTR)pid);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    break;

    case IOCTL_STOP_MONITOR:
    {
        PVOID buf;
        size_t bufLen;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(ULONG), &buf, &bufLen);
        if (NT_SUCCESS(status) && bufLen >= sizeof(ULONG)) {
            ULONG pid = *(ULONG*)buf;
            extern NTSTATUS StopProcessMonitor(HANDLE);
            status = StopProcessMonitor((HANDLE)(ULONG_PTR)pid);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    break;

    case IOCTL_GET_DATA:
    {
        SyscallRecord* outBuf;
        size_t         outSize;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SyscallRecord), (PVOID*)&outBuf, &outSize);
        if (NT_SUCCESS(status)) {
            KIRQL oldIrql;
            KeAcquireSpinLock(&g_ringSpin, &oldIrql);

            if (g_ringHead != g_ringTail) {
                *outBuf = g_ringBuffer[g_ringTail];
                g_ringTail = (g_ringTail + 1) % COMM_RING_SIZE;
                WdfRequestSetInformation(Request, sizeof(SyscallRecord));
                status = STATUS_SUCCESS;
            }
            else {
                WdfRequestSetInformation(Request, 0);
                status = STATUS_NO_MORE_ENTRIES;
            }

            KeReleaseSpinLock(&g_ringSpin, oldIrql);
        }
    }
    break;
    }

    WdfRequestComplete(Request, status);
}

//--------------------------------------------------------------------------------
// SignalUserEvent: wake user‐mode console that data is ready
//--------------------------------------------------------------------------------
VOID
SignalUserEvent(
    VOID
)
{
    if (g_userEvent) {
        KeSetEvent(g_userEvent, IO_NO_INCREMENT, FALSE);
    }
}

//--------------------------------------------------------------------------------
// EnqueueSyscallRecord: push a new record into the ring buffer
//   Must be callable at DISPATCH_LEVEL.
//--------------------------------------------------------------------------------
BOOLEAN
EnqueueSyscallRecord(
    _In_ const SyscallRecord* Rec
)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_ringSpin, &oldIrql);

    ULONG next = (g_ringHead + 1) % COMM_RING_SIZE;
    if (next == g_ringTail) {
        // Ring is full: drop oldest entry
        g_ringTail = (g_ringTail + 1) % COMM_RING_SIZE;
    }
    g_ringBuffer[g_ringHead] = *Rec;
    g_ringHead = next;

    KeReleaseSpinLock(&g_ringSpin, oldIrql);
    SignalUserEvent();
    return TRUE;
}
//------------------------------------------------------------------------------
// Stubs for the IoctlDispatch helpers
//------------------------------------------------------------------------------
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
StartProcessMonitor(
    _In_ HANDLE pid
)
{
    UNREFERENCED_PARAMETER(pid);
    // TODO: start EPT trapping for this PID (or simply return success for now)
    KdPrint(("StartProcessMonitor stub: PID=%u\n", (ULONG)(ULONG_PTR)pid));
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
StopProcessMonitor(
    _In_ HANDLE pid
)
{
    UNREFERENCED_PARAMETER(pid);
    // TODO: stop EPT trapping for this PID
    KdPrint(("StopProcessMonitor stub: PID=%u\n", (ULONG)(ULONG_PTR)pid));
    return STATUS_SUCCESS;
}
