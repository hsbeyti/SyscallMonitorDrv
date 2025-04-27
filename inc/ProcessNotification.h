// inc/ProcessNotification.h
#pragma once
#include <ntddk.h>

NTSTATUS RegisterProcessNotifications();
void     UnregisterProcessNotifications();
