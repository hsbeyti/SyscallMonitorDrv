// src/SyscallInterceptor.c

#include <ntddk.h>
#include <winhvplatform.h>    // WHV_RUN_VP_EXIT_CONTEXT, WHvRunVpExitReasonMemoryAccess, WHV_MEMORY_ACCESS_CONTEXT
#include <intrin.h>           // __readcr3

#include "../inc/SyscallInterceptor.h"
#include "../inc/ProcessNotification.h"
#include "../inc/SyscallFilter.h"
#include "../inc/Communication.h"

// Dynamically–loaded WHP APIs (set in InitializeHypervisor)
extern PFN_WHvRunVirtualProcessor            g_pWHvRunVirtualProcessor;
extern PFN_WHvGetVirtualProcessorRegisters   g_pWHvGetVirtualProcessorRegisters;

// GPA of the MSR_LSTAR syscall stub page (set during EPT setup)
extern UINT64 g_lstarGpa;

// From ProcessNotification.c
extern BOOLEAN IsProcessMonitored(ULONG_PTR cr3);

// From HypervisorSetup.c via header
// VOID SingleStepLstar(WHV_PARTITION_HANDLE, const WHV_RUN_VP_EXIT_CONTEXT*);

static HANDLE                g_ThreadHandle = NULL;
static volatile BOOLEAN      g_StopThread = FALSE;
static WHV_PARTITION_HANDLE  g_Partition = NULL;

//-----------------------------------------------------------------------------
// Interceptor thread: run the vCPU, trap EPT violations on MSR_LSTAR,
// extract RAX+CR3 via WHvGetVirtualProcessorRegisters, and log them.
//-----------------------------------------------------------------------------
static VOID
InterceptorThread(
    _In_ PVOID Context
)
{
    g_Partition = (WHV_PARTITION_HANDLE)Context;
    WHV_RUN_VP_EXIT_CONTEXT exitCtx;

    while (!g_StopThread) {
        RtlZeroMemory(&exitCtx, sizeof(exitCtx));

        // Execute the vCPU until it exits (e.g. a MemoryAccess/EPT violation)
        HRESULT hr = g_pWHvRunVirtualProcessor(
            g_Partition,
            0,          // vCPU index
            &exitCtx,
            sizeof(exitCtx)
        );
        if (hr < 0) {
            KdPrint(("Interceptor: WHvRunVirtualProcessor failed 0x%X\n", hr));
            break;
        }

        // WHP treats EPT-execute-disable as a memory-access exit
        if (exitCtx.ExitReason == WHvRunVpExitReasonMemoryAccess) {
            // Faulting GPA is in exitCtx.MemoryAccess.Gpa
            UINT64 faultGpa = exitCtx.MemoryAccess.Gpa;

            if (faultGpa == g_lstarGpa) {
                // Fetch RAX and CR3 from the guest
                WHV_REGISTER_NAME regs[] = {
                    WHvX64RegisterRax,
                    WHvX64RegisterCr3
                };
                WHV_REGISTER_VALUE vals[2] = { 0 };

                hr = g_pWHvGetVirtualProcessorRegisters(
                    g_Partition,
                    0,
                    regs, ARRAYSIZE(regs),
                    vals
                );
                if (hr >= 0) {
                    ULONG     syscallId = (ULONG)vals[0].Reg64;
                    ULONG_PTR cr3 = (ULONG_PTR)vals[1].Reg64;

                    if (IsProcessMonitored(cr3) && IsSyscallTracked(syscallId)) {
                        SyscallRecord rec = {
                            (ULONG)cr3,
                            syscallId,
                            KeQueryInterruptTime()
                        };
                        EnqueueSyscallRecord(&rec);
                        // EnqueueSyscallRecord also signals the user‐mode event
                    }
                }

                // Single‐step or emulate the syscall stub instruction, then resume
                SingleStepLstar(g_Partition, &exitCtx);
            }
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

//-----------------------------------------------------------------------------
// Kick off the interceptor system thread
//-----------------------------------------------------------------------------
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
StartInterceptorThread(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
)
{
    g_StopThread = FALSE;
    return PsCreateSystemThread(
        &g_ThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        InterceptorThread,
        PartitionHandle
    );
}

//-----------------------------------------------------------------------------
// Signal the thread to exit and wait for it to terminate
//-----------------------------------------------------------------------------
_IRQL_requires_max_(PASSIVE_LEVEL)
void
StopInterceptorThread(
    void
)
{
    if (g_ThreadHandle) {
        g_StopThread = TRUE;
        KeWaitForSingleObject(
            g_ThreadHandle,
            Executive,
            KernelMode,
            FALSE,
            NULL
        );
        ZwClose(g_ThreadHandle);
        g_ThreadHandle = NULL;
    }
}
