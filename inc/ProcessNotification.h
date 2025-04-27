// inc/ProcessNotification.h
#pragma once

// This brings in PEPROCESS, KAPC_STATE, PsSetCreateProcessNotifyRoutineEx, timers, DPCs
#include <ntifs.h>

// This must declare ArmEptHook/DisarmEptHook – adjust path if your include dirs differ
#include "HypervisorSetup.h"   // extern VOID ArmEptHook(void); extern VOID DisarmEptHook(void);

#ifdef __cplusplus
extern "C" {
#endif

	/// How many processes we can track at once
#define MAX_MONITORED_PROCS 64

/// Install the CreateProcess callback & start the cleanup timer
	NTSTATUS RegisterProcessNotifications(void);

	/// Remove the callback & cancel the timer
	VOID     UnregisterProcessNotifications(void);

#ifdef __cplusplus
}
#endif
