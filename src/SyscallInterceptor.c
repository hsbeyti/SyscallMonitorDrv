// src/SyscallInterceptor.c
#include "../inc/SyscallInterceptor.h"
#include "../inc/Communication.h"
#include <wdm.h>

static HANDLE g_threadHandle;
bool g_stopThread;

VOID InterceptorThread(PVOID ctx) {
    WHV_RUN_VP_EXIT_CONTEXT exitCtx;
    while (!g_stopThread) {
        NTSTATUS st = WhvRunVirtualProcessor(ctx, 0, &exitCtx, sizeof(exitCtx));
        if (st == STATUS_SUCCESS && exitCtx.ExitReason == WHvRunVpExitReasonEptViolation) {
            ULONG64 faultGpa = exitCtx.ExitQualification;
            if (faultGpa == g_lstarGpa) {
                ULONG id = (ULONG)exitCtx.VpRegisters.Rax;
                ULONG cr3 = (ULONG)exitCtx.VpRegisters.Cr3;
                if (IsProcessMonitored(cr3) && IsSyscallTracked(id)) {
                    SyscallRecord rec = { cr3, id, KeQueryInterruptTime() };
                    EnqueueRecord(rec);
                    SignalUserEvent();
                }
                SingleStepLstar();
            }
        }
    }
}

NTSTATUS StartInterceptorThread(WHV_PARTITION_HANDLE part) {
    g_stopThread = false;
    return PsCreateSystemThread(&g_threadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, InterceptorThread, part);
}

void StopInterceptorThread() {
    g_stopThread = true;
    if (g_threadHandle) { ZwClose(g_threadHandle); g_threadHandle = NULL; }
}