// inc/HypervisorSetup.h
#pragma once
#include <ntddk.h>
#include <wdf.h>

NTSTATUS InitializeHypervisor();
void     ShutdownHypervisor();