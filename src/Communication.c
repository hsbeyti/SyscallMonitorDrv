// src/Communication.c

#include "../inc/Communication.h"
#include <ntintsafe.h>
#include <wdf.h>
#include <wdfrequest.h>
#include <ntddk.h>

// Forward‐declare your EvtIoDeviceControl callback *before* RegisterIoctlHandlers
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL IoctlDispatch;

extern NTSTATUS StartProcessMonitor(HANDLE pid);
extern NTSTATUS StopProcessMonitor(HANDLE pid);

// Globals (same as before)
static WDFDEVICE      g_device;
static PKEVENT        g_userEvent;
static KSPIN_LOCK     g_ringSpin;
static USHORT         g_ringHead, g_ringTail;
static SyscallRecord  g_ringBuffer[1024];

// … SetupCommunication and CleanupCommunication unchanged …

NTSTATUS
RegisterIoctlHandlers(
    _In_ WDFDEVICE device,
    _In_ WDFQUEUE  queue
)
{
    UNREFERENCED_PARAMETER(device);

    WDF_IO_QUEUE_CONFIG  qcfg;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&qcfg, WdfIoQueueDispatchSequential);

    // Now the compiler knows IoctlDispatch
    qcfg.EvtIoDeviceControl = IoctlDispatch;

    return WdfIoQueueCreate(device, &qcfg, WDF_NO_OBJECT_ATTRIBUTES, NULL);
}

// Now define the callback itself, matching the typedef exactly:

VOID
IoctlDispatch(
    _In_ WDFQUEUE   queue,
    _In_ WDFREQUEST req,
    _In_ size_t     outputBufferLength,
    _In_ size_t     inputBufferLength,
    _In_ ULONG      ioControlCode
)
{
    UNREFERENCED_PARAMETER(queue);
    UNREFERENCED_PARAMETER(outputBufferLength);
    UNREFERENCED_PARAMETER(inputBufferLength);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    switch (ioControlCode) {
    case IOCTL_START_MONITOR:
    {
        PVOID  buf;
        size_t bufLen;
        status = WdfRequestRetrieveInputBuffer(
            req,
            sizeof(ULONG),
            &buf,
            &bufLen);
        if (NT_SUCCESS(status) && bufLen >= sizeof(ULONG)) {
            ULONG pid = *(ULONG*)buf;
            status = StartProcessMonitor((HANDLE)(ULONG_PTR)pid);
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    break;

    case IOCTL_STOP_MONITOR:
    {
        PVOID  buf;
        size_t bufLen;
        status = WdfRequestRetrieveInputBuffer(
            req,
            sizeof(ULONG),
            &buf,
            &bufLen);
        if (NT_SUCCESS(status) && bufLen >= sizeof(ULONG)) {
            ULONG pid = *(ULONG*)buf;
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
        status = WdfRequestRetrieveOutputBuffer(
            req,
            sizeof(SyscallRecord),
            (PVOID*)&outBuf,
            &outSize);
        if (NT_SUCCESS(status)) {
            KIRQL oldIrql;
            KeAcquireSpinLock(&g_ringSpin, &oldIrql);

            if (g_ringHead != g_ringTail) {
                *outBuf = g_ringBuffer[g_ringTail];
                g_ringTail = (USHORT)((g_ringTail + 1) % ARRAYSIZE(g_ringBuffer));
                WdfRequestSetInformation(req, sizeof(SyscallRecord));
                status = STATUS_SUCCESS;
            }
            else {
                WdfRequestSetInformation(req, 0);
                status = STATUS_NO_MORE_ENTRIES;
            }

            KeReleaseSpinLock(&g_ringSpin, oldIrql);
        }
    }
    break;
    }

    WdfRequestComplete(req, status);
}

// … plus your SignalUserEvent and EnqueueSyscallRecord helpers as before …
