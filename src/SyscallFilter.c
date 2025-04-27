// src/SyscallFilter.c

#include "../inc/SyscallFilter.h"

//------------------------------------------------------------------------------
// Your 50 syscall IDs to watch—private to this .c file
// Replace these example slots with the actual IDs you care about.
//------------------------------------------------------------------------------
static const ULONG g_interestedIds[SYSCALL_FILTER_ID_COUNT] = {
    0x3B, 0x3C, 0x26, 0x27, 0x2A, 0x5A, 0x5B, 0x60, 0x61, 0x62,
    0x75, 0x76, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80,
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
    0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94,
    0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E
};

//------------------------------------------------------------------------------
// The bitmap itself—one bit per syscall ID
//------------------------------------------------------------------------------
UCHAR g_filterBitmap[SYSCALL_FILTER_BITMAP_BYTES] = { 0 };

//------------------------------------------------------------------------------
// Initialize: zero the bitmap then set bits for each ID in g_interestedIds
//------------------------------------------------------------------------------
NTSTATUS
InitializeSyscallFilter(void)
{
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));

    for (ULONG i = 0; i < SYSCALL_FILTER_ID_COUNT; i++) {
        ULONG id = g_interestedIds[i];
        if (id <= SYSCALL_FILTER_MAX_ID) {
            g_filterBitmap[id >> 3] |= (UCHAR)(1U << (id & 7));
        }
    }

    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// Cleanup: clear the bitmap (optional, but good hygiene)
//------------------------------------------------------------------------------
void
CleanupSyscallFilter(void)
{
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));
}
