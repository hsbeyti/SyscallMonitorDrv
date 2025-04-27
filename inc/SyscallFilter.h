// inc/SyscallFilter.h
#pragma once

#include <ntddk.h>

// Maximum syscall ID you might track; adjust if needed
#define SYSCALL_FILTER_MAX_ID       1023

// How many IDs you’re tracking
#define SYSCALL_FILTER_ID_COUNT     50

// Size of the bitmap in bytes
#define SYSCALL_FILTER_BITMAP_BYTES ((SYSCALL_FILTER_MAX_ID + 8) / 8)

// Backing bitmap—one bit per possible syscall ID
extern UCHAR g_filterBitmap[SYSCALL_FILTER_BITMAP_BYTES];

/// Initialize the filter bitmap (sets bits for your 50 IDs)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
InitializeSyscallFilter(void);

/// Clear the filter bitmap (called on driver unload)
_IRQL_requires_max_(PASSIVE_LEVEL)
void
CleanupSyscallFilter(void);

/// Fast inline check if an ID is being tracked
static __inline
BOOLEAN
IsSyscallTracked(_In_ ULONG Id)
{
    return (BOOLEAN)((g_filterBitmap[Id >> 3] >> (Id & 7)) & 1U);
}
