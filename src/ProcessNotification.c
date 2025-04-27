// src/ProcessNotification.c

#include <ntifs.h>                  // PEPROCESS, KAPC_STATE, PsSetCreateProcessNotifyRoutineEx, timers, DPCs
#include <intrin.h>                 // __readcr3
#include "../inc/ProcessNotification.h"

// Module‐private process entry
typedef struct _PROC_ENTRY {
    ULONG_PTR     Cr3;
    LARGE_INTEGER ExpireTime;
} PROC_ENTRY;

// Storage for up to MAX_MONITORED_PROCS concurrent processes
static PROC_ENTRY  g_procs[MAX_MONITORED_PROCS];
static ULONG       g_procCount = 0;
static KSPIN_LOCK  g_procLock;
static KDPC        g_timerDpc;
static KTIMER      g_timer;

// Forward declarations
static VOID
ProcessCreateNotify(
    _Inout_ PEPROCESS Process,
    _In_    HANDLE    ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);

static VOID
TimerDpcRoutine(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID     DeferredContext,
    _In_opt_ PVOID     SystemArgument1,
    _In_opt_ PVOID     SystemArgument2
);

//------------------------------------------------------------------------------
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
IsProcessMonitored(
    _In_ ULONG_PTR Cr3
)
{
    BOOLEAN found = FALSE;
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_procLock, &oldIrql);
    for (ULONG i = 0; i < g_procCount; i++) {
        if (g_procs[i].Cr3 == Cr3) {
            found = TRUE;
            break;
        }
    }
    KeReleaseSpinLock(&g_procLock, oldIrql);
    return found;
}

//------------------------------------------------------------------------------
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RegisterProcessNotifications(
    void
)
{
    // Initialize lock, timer, and DPC
    KeInitializeSpinLock(&g_procLock);
    KeInitializeTimerEx(&g_timer, NotificationTimer);
    KeInitializeDpc(&g_timerDpc, TimerDpcRoutine, NULL);

    // Register the callback
    return PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, FALSE);
}

//------------------------------------------------------------------------------
_IRQL_requires_max_(PASSIVE_LEVEL)
void
UnregisterProcessNotifications(
    void
)
{
    PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, TRUE);
    KeCancelTimer(&g_timer);
}

//------------------------------------------------------------------------------
static VOID
ProcessCreateNotify(
    _Inout_ PEPROCESS Process,
    _In_    HANDLE    ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(ProcessId);

    if (CreateInfo) {
        // Grab CR3 from the newly created process
        KAPC_STATE apcState;
        KeStackAttachProcess(Process, &apcState);
        ULONG_PTR cr3 = __readcr3();
        KeUnstackDetachProcess(&apcState);

        // Under lock, add to list and arm EPT hook
        KIRQL oldIrql;
        KeAcquireSpinLock(&g_procLock, &oldIrql);

        if (g_procCount < MAX_MONITORED_PROCS) {
            // Record CR3 and expiration time (+50s)
            g_procs[g_procCount].Cr3 = cr3;
            KeQuerySystemTime(&g_procs[g_procCount].ExpireTime);
            g_procs[g_procCount].ExpireTime.QuadPart += 50LL * 10'000'000LL;
            g_procCount++;

            // Ensure our syscall-stub EPT hook is armed
            ArmEptHook();

            // (Re)start a 1s periodic DPC to clean up old entries
            LARGE_INTEGER dueTime = { .QuadPart = -1LL * 1'000'0000LL }; // –1s relative
            KeSetTimerEx(&g_timer, dueTime, 1000, &g_timerDpc);
        }

        KeReleaseSpinLock(&g_procLock, oldIrql);
    }
}

//------------------------------------------------------------------------------
static VOID
TimerDpcRoutine(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID     DeferredContext,
    _In_opt_ PVOID     SystemArgument1,
    _In_opt_ PVOID     SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    LARGE_INTEGER now;
    KeQuerySystemTime(&now);

    // Remove expired entries under lock
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_procLock, &oldIrql);

    ULONG   writeIdx = 0;
    for (ULONG i = 0; i < g_procCount; i++) {
        if (g_procs[i].ExpireTime.QuadPart > now.QuadPart) {
            g_procs[writeIdx++] = g_procs[i];
        }
    }
    g_procCount = writeIdx;

    // If nothing left, disarm hook & cancel timer
    if (g_procCount == 0) {
        DisarmEptHook();
        KeCancelTimer(&g_timer);
    }

    KeReleaseSpinLock(&g_procLock, oldIrql);
}
