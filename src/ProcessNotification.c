// src/ProcessNotification.c
#include "../inc/ProcessNotification.h"
#include <wdm.h>

typedef struct _PROC_ENTRY { HANDLE Cr3; LARGE_INTEGER ExpireTime; } PROC_ENTRY;
static PROC_ENTRY g_procs[64]; // up to 64 concurrent
static ULONG      g_procCount;
static KDPC       g_timerDpc;
static KTIMER     g_timer;

VOID ProcessCreateNotify(PROCESS_NOTIFY_INFO* info) {
    if (info->CreateInfo) {
        if (g_procCount < ARRAYSIZE(g_procs)) {
            g_procs[g_procCount].Cr3 = info->CreateInfo->CreatingThreadId; // read CR3 separately
            KeQuerySystemTime(&g_procs[g_procCount].ExpireTime);
            g_procs[g_procCount].ExpireTime.QuadPart += 50LL * 10000000; // +50s
            g_procCount++;
            ArmEptHook(); // ensure EPT NX bit set
        }
    }
}

NTSTATUS RegisterProcessNotifications() {
    NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, FALSE);
    // init timer DPC
    KeInitializeTimer(&g_timer);
    KeInitializeDpc(&g_timerDpc, TimerDpcRoutine, NULL);
    return status;
}

void UnregisterProcessNotifications() {
    PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, TRUE);
    KeCancelTimer(&g_timer);
}

VOID TimerDpcRoutine(KDPC* dpc, PVOID ctx, PVOID sysArg1, PVOID sysArg2) {
    LARGE_INTEGER now;
    KeQuerySystemTime(&now);
    // remove expired and disarm when none remain
}
