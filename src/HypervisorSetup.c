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

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
StartInterceptorThread(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
)
{
    UNREFERENCED_PARAMETER(PartitionHandle);
    // TODO: Launch your PsCreateSystemThread for RunVirtualProcessor loop
    KdPrint(("Hypervisor: StartInterceptorThread stub called\n"));
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
StopInterceptorThread(
    void
)
{
    // TODO: Signal your interceptor thread to exit and wait on its handle
    KdPrint(("Hypervisor: StopInterceptorThread stub called\n"));
}
