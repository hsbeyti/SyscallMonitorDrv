// inc/SyscallInterceptor.h
#pragma once

#include <ntddk.h>
#include <winhvplatform.h>    // WHV_PARTITION_HANDLE, WHV_RUN_VP_EXIT_CONTEXT

#include "SyscallFilter.h"    // IsSyscallTracked
#include "ProcessNotification.h" // IsProcessMonitored
#include "Communication.h"    // SyscallRecord

#ifdef __cplusplus
extern "C" {
#endif

    /// GPA of the LSTAR syscall‐entry page (set in your EPT builder)
    extern UINT64 g_lstarGpa;

    /// Pointer to the WinHvRunVirtualProcessor API (loaded at init)
    extern PFN_WHvRunVirtualProcessor g_pWHvRunVirtualProcessor;

    /// Starts the interceptor system‐thread that runs the vCPU and traps syscalls
    _IRQL_requires_max_(PASSIVE_LEVEL)
        NTSTATUS
        StartInterceptorThread(
            _In_ WHV_PARTITION_HANDLE PartitionHandle
        );

    /// Signals the interceptor thread to exit and waits for it
    _IRQL_requires_max_(PASSIVE_LEVEL)
        void
        StopInterceptorThread(
            void
        );

    /// Performs a single‐step or emulation of the first instruction of the syscall stub
    /// immediately after an EPT execute‐disable exit
    _IRQL_requires_max_(DISPATCH_LEVEL)
        VOID
        SingleStepLstar(
            _In_ WHV_PARTITION_HANDLE               PartitionHandle,
            _In_ const WHV_RUN_VP_EXIT_CONTEXT* ExitContext
        );

#ifdef __cplusplus
}
#endif
