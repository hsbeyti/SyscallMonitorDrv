// src/SyscallFilter.c
#include "-./inc/SyscallFilter.h"

static ULONG g_filterBitmap[32]; // supports up to 1024 IDs

NTSTATUS InitializeSyscallFilter() {
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));
    // set bits for your 50 syscall IDs
    for (int i = 0; i < 50; i++) SetBit(g_filterBitmap, g_interestedIds[i]);
    return STATUS_SUCCESS;
}

void CleanupSyscallFilter() {
    RtlZeroMemory(g_filterBitmap, sizeof(g_filterBitmap));
}

BOOLEAN IsSyscallTracked(ULONG id) {
    return TestBit(g_filterBitmap, id);
}
