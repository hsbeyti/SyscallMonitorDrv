// inc/Communication.h
#pragma once

#include <ntddk.h>
#include <wdf.h>

// IOCTL definitions
#define IOCTL_START_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_STOP_MONITOR  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_GET_DATA      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

// Ring buffer size and helper
#define COMM_RING_SIZE 1024
#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))

// This must match your kernel’s SyscallRecord
typedef struct _SyscallRecord {
    ULONG     ProcessId;
    ULONG     SyscallId;
    ULONGLONG Timestamp;
} SyscallRecord;

/// Initialize the communication layer (ring buffer + named event)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
SetupCommunication(
    _In_ WDFDEVICE Device
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS StartProcessMonitor(_In_ HANDLE pid);

/// Tear down the communication layer
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
CleanupCommunication(
    VOID
);

/// Create the default IOCTL queue for Start/Stop/GetData
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RegisterIoctlHandlers(
    _In_ WDFDEVICE Device
);

/// Signal the user‐mode event (called from your interceptor or Enqueue)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
SignalUserEvent(
    VOID
);

/// Enqueue a SyscallRecord into the ring (called at DISPATCH_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
EnqueueSyscallRecord(
    _In_ const SyscallRecord* Rec
);
