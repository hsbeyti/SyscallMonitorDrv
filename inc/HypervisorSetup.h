#pragma once

#include <ntddk.h>
#include <winhvplatform.h>   // Windows Hypervisor Platform definitions

//
// PFN_* typedefs (for older SDKs that may not define them)
//
#ifndef PFN_WHvCreatePartition
typedef HRESULT(WINAPI* PFN_WHvCreatePartition)(
    _Out_ WHV_PARTITION_HANDLE* Partition
    );
typedef HRESULT(WINAPI* PFN_WHvSetupPartition)(
    _In_ WHV_PARTITION_HANDLE Partition
    );
typedef HRESULT(WINAPI* PFN_WHvDeletePartition)(
    _In_ WHV_PARTITION_HANDLE Partition
    );
typedef HRESULT(WINAPI* PFN_WHvSetPartitionProperty)(
    _In_ WHV_PARTITION_HANDLE            Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE     PropertyCode,
    _In_reads_bytes_(PropertyBufferSize) const VOID* PropertyBuffer,
    _In_ UINT32                          PropertyBufferSize
    );
typedef HRESULT(WINAPI* PFN_WHvCreateVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex,
    _In_ UINT32               Flags
    );
typedef HRESULT(WINAPI* PFN_WHvRunVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex,
    _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext,
    _In_ UINT32               ExitContextSizeInBytes
    );
typedef HRESULT(WINAPI* PFN_WHvDeleteVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex
    );
#endif

//
// Dynamically‐loaded WHP entry points
//
extern PFN_WHvCreatePartition        g_pWHvCreatePartition;
extern PFN_WHvSetupPartition         g_pWHvSetupPartition;
extern PFN_WHvDeletePartition        g_pWHvDeletePartition;
extern PFN_WHvSetPartitionProperty   g_pWHvSetPartitionProperty;
extern PFN_WHvCreateVirtualProcessor g_pWHvCreateVirtualProcessor;
extern PFN_WHvRunVirtualProcessor    g_pWHvRunVirtualProcessor;
extern PFN_WHvDeleteVirtualProcessor g_pWHvDeleteVirtualProcessor;

//
// Core hypervisor lifecycle
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS InitializeHypervisor(void);

_IRQL_requires_max_(PASSIVE_LEVEL)
void     ShutdownHypervisor(void);

//
// EPT & vCPU management helpers
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS BuildEptIdentityMap(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS StartInterceptorThread(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
void     StopInterceptorThread(void);

//
// **NEW**: hooks to arm/disarm the syscall‐stub execute‐disable EPT trap
//
_IRQL_requires_max_(PASSIVE_LEVEL)
void     ArmEptHook(void);

_IRQL_requires_max_(PASSIVE_LEVEL)
void     DisarmEptHook(void);
#pragma once

#include <ntddk.h>
#include <winhvplatform.h>   // Windows Hypervisor Platform definitions

//
// PFN_* typedefs (for older SDKs that may not define them)
//
#ifndef PFN_WHvCreatePartition
typedef HRESULT(WINAPI* PFN_WHvCreatePartition)(
    _Out_ WHV_PARTITION_HANDLE* Partition
    );
typedef HRESULT(WINAPI* PFN_WHvSetupPartition)(
    _In_ WHV_PARTITION_HANDLE Partition
    );
typedef HRESULT(WINAPI* PFN_WHvDeletePartition)(
    _In_ WHV_PARTITION_HANDLE Partition
    );
typedef HRESULT(WINAPI* PFN_WHvSetPartitionProperty)(
    _In_ WHV_PARTITION_HANDLE            Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE     PropertyCode,
    _In_reads_bytes_(PropertyBufferSize) const VOID* PropertyBuffer,
    _In_ UINT32                          PropertyBufferSize
    );
typedef HRESULT(WINAPI* PFN_WHvCreateVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex,
    _In_ UINT32               Flags
    );
typedef HRESULT(WINAPI* PFN_WHvRunVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex,
    _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext,
    _In_ UINT32               ExitContextSizeInBytes
    );
typedef HRESULT(WINAPI* PFN_WHvDeleteVirtualProcessor)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32               VpIndex
    );
#endif

//
// Dynamically‐loaded WHP entry points
//
extern PFN_WHvCreatePartition        g_pWHvCreatePartition;
extern PFN_WHvSetupPartition         g_pWHvSetupPartition;
extern PFN_WHvDeletePartition        g_pWHvDeletePartition;
extern PFN_WHvSetPartitionProperty   g_pWHvSetPartitionProperty;
extern PFN_WHvCreateVirtualProcessor g_pWHvCreateVirtualProcessor;
extern PFN_WHvRunVirtualProcessor    g_pWHvRunVirtualProcessor;
extern PFN_WHvDeleteVirtualProcessor g_pWHvDeleteVirtualProcessor;

//
// Core hypervisor lifecycle
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS InitializeHypervisor(void);

_IRQL_requires_max_(PASSIVE_LEVEL)
void     ShutdownHypervisor(void);

//
// EPT & vCPU management helpers
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS BuildEptIdentityMap(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS StartInterceptorThread(
    _In_ WHV_PARTITION_HANDLE PartitionHandle
);



//
// **NEW**: hooks to arm/disarm the syscall‐stub execute‐disable EPT trap
//
_IRQL_requires_max_(PASSIVE_LEVEL)
void     ArmEptHook(void);

_IRQL_requires_max_(PASSIVE_LEVEL)
void     DisarmEptHook(void);
// After your other PFN_* externs…

// Fetch general‐purpose registers at a VM-exit
typedef HRESULT(WINAPI* PFN_WHvGetVirtualProcessorRegisters)(
    _In_  WHV_PARTITION_HANDLE Partition,
    _In_  UINT32               VpIndex,
    _In_reads_(RegisterCount)  const WHV_REGISTER_NAME* RegisterNames,
    _In_  UINT32               RegisterCount,
    _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
    );
extern PFN_WHvGetVirtualProcessorRegisters g_pWHvGetVirtualProcessorRegisters;
