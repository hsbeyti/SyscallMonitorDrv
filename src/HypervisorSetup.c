// src/HypervisorSetup.c

#include "../inc/HypervisorSetup.h"
#include "../inc/SyscallInterceptor.h"
#include <wdm.h>


// GPA of the MSR_LSTAR stub page (will be set in BuildEptIdentityMap)
UINT64 g_lstarGpa = 0;

// Pointers to the WHP APIs (loaded via MmGetSystemRoutineAddress)
PFN_WHvCreatePartition              g_pWHvCreatePartition = NULL;
PFN_WHvSetupPartition               g_pWHvSetupPartition = NULL;
PFN_WHvDeletePartition              g_pWHvDeletePartition = NULL;   // <— this one was missing
PFN_WHvSetPartitionProperty         g_pWHvSetPartitionProperty = NULL;
PFN_WHvCreateVirtualProcessor       g_pWHvCreateVirtualProcessor = NULL;
PFN_WHvRunVirtualProcessor          g_pWHvRunVirtualProcessor = NULL;
PFN_WHvDeleteVirtualProcessor       g_pWHvDeleteVirtualProcessor = NULL;
PFN_WHvGetVirtualProcessorRegisters g_pWHvGetVirtualProcessorRegisters = NULL;
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

    return status;
}
_IRQL_requires_max_(PASSIVE_LEVEL)
void ArmEptHook(void)
{
    // TODO: set the execute‐disable bit on the MSR_LSTAR page in EPT/NPT.
    KdPrint(("Hypervisor: ArmEptHook called\n"));
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void DisarmEptHook(void)
{
    // TODO: clear the execute‐disable bit so syscalls run normally again.
    KdPrint(("Hypervisor: DisarmEptHook called\n"));
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
//------------------------------------------------------------------------------
// Stub implementations so the driver will link.
// Replace these with your real EPT‐builder and interceptor logic later.
//------------------------------------------------------------------------------

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
BuildEptIdentityMap(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
)
{
    UNREFERENCED_PARAMETER(PartitionHandle);
    // TODO: Build your identity‐mapped EPT/NPT tables here
    KdPrint(("Hypervisor: BuildEptIdentityMap stub called\n"));
    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------
// Stub for SingleStepLstar (must match your header)
//------------------------------------------------------------------------------
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
SingleStepLstar(
    _In_ WHV_PARTITION_HANDLE               PartitionHandle,
    _In_ const WHV_RUN_VP_EXIT_CONTEXT* ExitCtx
)
{
    UNREFERENCED_PARAMETER(PartitionHandle);
    UNREFERENCED_PARAMETER(ExitCtx);
    // TODO: emulate or single-step the first instruction at LSTAR GPA
    KdPrint(("SingleStepLstar stub\n"));
}

