// src/HypervisorSetup.c

#include "../inc/HypervisorSetup.h"
#include "../inc/SyscallInterceptor.h"
#include <wdm.h>

static WHV_PARTITION_HANDLE g_PartitionHandle;

NTSTATUS
InitializeHypervisor(
    void
)
{
    NTSTATUS status;

    // 1) Create the partition
    status = g_pWHvCreatePartition(&g_PartitionHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Hypervisor: WHvCreatePartition failed: 0x%X\n", status));
        return status;
    }

    // 2) Configure + setup the partition
    status = g_pWHvSetupPartition(g_PartitionHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Hypervisor: WHvSetupPartition failed: 0x%X\n", status));
        g_pWHvDeletePartition(g_PartitionHandle);
        g_PartitionHandle = 0;
        return status;
    }

    // 3) Build the EPT/NPT identity map and initial guest state
    status = BuildEptIdentityMap(g_PartitionHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Hypervisor: BuildEptIdentityMap failed: 0x%X\n", status));
        ShutdownHypervisor();
        return status;
    }

    // 4) Start the interceptor (vCPU) thread
    status = StartInterceptorThread(g_PartitionHandle);
    if (!NT_SUCCESS(status)) {
        KdPrint(("Hypervisor: StartInterceptorThread failed: 0x%X\n", status));
        ShutdownHypervisor();
    }

    return status;
}

void
ShutdownHypervisor(
    void
)
{
    // Stop the vCPU/interceptor thread
    StopInterceptorThread();

    // Tear down the partition if it exists
    if (g_PartitionHandle) {
        g_pWHvDeletePartition(g_PartitionHandle);
        g_PartitionHandle = 0;
    }
}
