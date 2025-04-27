#pragma once
#include <ntddk.h>
#include <wdf.h>
#include "SyscallInterceptor.h"
#include "ProcessNotification.h"
#include "ProcessNotification.h"
#include "WHPDefinitions.h"

#define IOCTL_START_MONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_STOP_MONITOR  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_GET_DATA      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

NTSTATUS SetupCommunication(WDFDEVICE device);
void CleanupCommunication();
NTSTATUS RegisterIoctlHandlers(WDFDEVICE device, WDFQUEUE queue);
VOID SignalUserEvent();