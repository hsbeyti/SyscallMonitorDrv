/*
 * src/SyscallFilter.c
 * Implements the syscall filter bitmap and initialization.
 */

#include "../inc/SyscallFilter.h"
#include <ntddk.h>

 // External list of syscall IDs of interest
 // Define this in SyscallInterceptor.c or a dedicated header
extern ULONG g_interestedIds[];
extern const ULONG g_interestedCount;

// Filter bitmap storage
UCHAR g_filterBitmap[SYSCALL_FILTER_BITMAP_BYTES] = { 0 };

/// Initialize the syscall filter by setting bits for each ID in g_interestedIds
NTSTATUS
InitializeSyscallFilter(
    void
)
{
    if (g_interestedCount == 0 || g_interestedCount > SYSCALL_FILTER_MAX_ID)
        return STATUS_INVALID_PARAMETER;

    // Clear existing bitmap
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));

    // Populate bitmap
    for (ULONG i = 0; i < g_interestedCount; ++i) {
        ULONG id = g_interestedIds[i];
        if (id <= SYSCALL_FILTER_MAX_ID) {
            g_filterBitmap[id >> 3] |= (UCHAR)(1U << (id & 7));
        }
    }

    return STATUS_SUCCESS;
}

/// Cleanup the filter (zero-out bitmap)
void
CleanupSyscallFilter(
    void
)
{
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));
}
