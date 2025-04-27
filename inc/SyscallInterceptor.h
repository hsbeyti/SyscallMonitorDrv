// inc/SyscallInterceptor.h
#pragma once
#include <ntddk.h>
#include <winhvplatform.h> // Include the WHP API header for WHV_PARTITION_HANDLE

typedef struct { ULONG ProcessId; ULONG SyscallId; ULONGLONG Timestamp; } SyscallRecord;

NTSTATUS StartInterceptorThread(WHV_PARTITION_HANDLE part);
void     StopInterceptorThread();