#include "../inc/ProcessNotification.h"
#include <intrin.h>  // for __readcr3

// Per‐process tracking entry
typedef struct _PROC_ENTRY {
    ULONG_PTR     Cr3;
    LARGE_INTEGER ExpireTime;
} PROC_ENTRY;

// Globals
static PROC_ENTRY g_Procs[MAX_MONITORED_PROCS];
static ULONG      g_ProcCount = 0;
static KSPIN_LOCK g_ProcLock;
static KDPC       g_TimerDpc;
static KTIMER     g_Timer;

// Forward‐declare the DPC routine
static VOID
TimerDpcRoutine(
    _In_ struct _KDPC* Dpc,
    _In_opt_ PVOID     DeferredContext,
    _In_opt_ PVOID     SystemArgument1,
    _In_opt_ PVOID     SystemArgument2
);

// This is the CreateProcessEx callback
static VOID
ProcessCreateNotify(
    _Inout_ PEPROCESS                Process,
    _In_    HANDLE                   ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO  CreateInfo
)
{
    UNREFERENCED_PARAMETER(ProcessId);

    if (CreateInfo) {
        KAPC_STATE apcState;

        // Grab CR3 from the new process
        KeStackAttachProcess(Process, &apcState);
        ULONG_PTR cr3 = __readcr3();
        KeUnstackDetachProcess(&apcState);

        // Add it to our list under lock
        KIRQL oldIrql;
        KeAcquireSpinLock(&g_ProcLock, &oldIrql);
        if (g_ProcCount < MAX_MONITORED_PROCS) {
            g_Procs[g_ProcCount].Cr3 = cr3;
            KeQuerySystemTime(&g_Procs[g_ProcCount].ExpireTime);
            g_Procs[g_ProcCount].ExpireTime.QuadPart += 50LL * 10'000'000LL; // +50s
            g_ProcCount++;

            // Arm the EPT execute-disable hook
            ArmEptHook();

            // (Re)start a 1s periodic timer to expire old entries
            LARGE_INTEGER dueTime;
            dueTime.QuadPart = -1LL * 1'000'0000LL; // relative –1s
            KeSetTimerEx(&g_Timer, dueTime, 1000, &g_TimerDpc);
        }
        KeReleaseSpinLock(&g_ProcLock, oldIrql);
    }
}

// DPC that runs every second to expire old processes
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

    KIRQL oldIrql;
    KeAcquireSpinLock(&g_ProcLock, &oldIrql);

    // Compact out expired entries
    ULONG writeIdx = 0;
    for (ULONG i = 0; i < g_ProcCount; i++) {
        if (g_Procs[i].ExpireTime.QuadPart > now.QuadPart) {
            g_Procs[writeIdx++] = g_Procs[i];
        }
    }
    g_ProcCount = writeIdx;

    // If none left, disarm hook+timer
    if (g_ProcCount == 0) {
        DisarmEptHook();
        KeCancelTimer(&g_Timer);
    }

    KeReleaseSpinLock(&g_ProcLock, oldIrql);
}

NTSTATUS
RegisterProcessNotifications(void)
{
    // Init lock, timer, and DPC
    KeInitializeSpinLock(&g_ProcLock);
    KeInitializeTimerEx(&g_Timer, NotificationTimer);
    KeInitializeDpc(&g_TimerDpc, TimerDpcRoutine, NULL);

    // Register for process‐create notifications
    return PsSetCreateProcessNotifyRoutineEx(
        ProcessCreateNotify,
        FALSE
    );
}

void
UnregisterProcessNotifications(void)
{
    PsSetCreateProcessNotifyRoutineEx(
        ProcessCreateNotify,
        TRUE
    );
    KeCancelTimer(&g_Timer);
}
