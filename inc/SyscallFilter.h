// inc/SyscallFilter.h
#pragma once

#include <ntddk.h>

// Maximum syscall ID to track; adjust if needed
#define SYSCALL_FILTER_MAX_ID       1023

// Size of bitmap in bytes
#define SYSCALL_FILTER_BITMAP_BYTES ((SYSCALL_FILTER_MAX_ID + 8) / 8)

// Bitmap storing which syscalls are tracked
extern UCHAR g_filterBitmap[SYSCALL_FILTER_BITMAP_BYTES];

/// Initialize the syscall filter bitmap (populate bits for tracked IDs)
NTSTATUS InitializeSyscallFilter(void);

/// Returns TRUE if the given syscall ID is in the tracked set
static __inline
BOOLEAN IsSyscallTracked(_In_ ULONG Id)
{
    return (BOOLEAN)((g_filterBitmap[Id >> 3] >> (Id & 7)) & 1U);
}

/// Clear all bits and free any resources (if needed)
void CleanupSyscallFilter(void);
