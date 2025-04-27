// inc/SyscallFilter.h
#pragma once
#include <ntddk.h>

BOOLEAN IsSyscallTracked(ULONG id);
NTSTATUS InitializeSyscallFilter();
void     CleanupSyscallFilter();