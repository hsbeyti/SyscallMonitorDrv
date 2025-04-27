// inc/ProcessNotification.h
#pragma once

#include <ntddk.h>
#include "HypervisorSetup.h"  // for ArmEptHook/DisarmEptHook prototypes

#ifdef __cplusplus
extern "C" {
#endif

    /// How many processes we can track at once
#define MAX_MONITORED_PROCS 64

/// Install the CreateProcess notify and start the cleanup timer/DPC
    _IRQL_requires_max_(PASSIVE_LEVEL)
        NTSTATUS
        RegisterProcessNotifications(void);

    /// Remove the notify callback and cancel the timer/DPC
    _IRQL_requires_max_(PASSIVE_LEVEL)
        void
        UnregisterProcessNotifications(void);

    /// Returns TRUE if the given CR3 (page table) belongs to a monitored process
    _IRQL_requires_max_(DISPATCH_LEVEL)
        BOOLEAN
        IsProcessMonitored(
            _In_ ULONG_PTR Cr3
        );

#ifdef __cplusplus
}
#endif
