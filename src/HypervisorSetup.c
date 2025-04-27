// src/HypervisorSetup.c
#include "../inc/HypervisorSetup.h"
#include "../inc/SyscallInterceptor.h"
#include <wdm.h>

static WHV_PARTITION_HANDLE g_part;

NTSTATUS InitializeHypervisor() {
    NTSTATUS status;
    status = WhvCreatePartition(&g_part);
    if (!NT_SUCCESS(status)) return status;
    status = WhvSetupPartition(g_part);
    if (!NT_SUCCESS(status)) { WhvDeletePartition(g_part); return status; }
    // Setup EPT/NPT identity map and guest initial state...
    status = BuildEptIdentityMap(g_part);
    if (!NT_SUCCESS(status)) { ShutdownHypervisor(); return status; }
    // Create vCPU and start interception thread
    status = StartInterceptorThread(g_part);
    return status;
}

void ShutdownHypervisor() {
    StopInterceptorThread();
    if (g_part) { WhvDeletePartition(g_part); g_part = 0; }
}
