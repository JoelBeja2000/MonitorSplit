/*++

Module Name:
    MonitorSplitDriver.h

Abstract:
    MonitorSplit - Windows Indirect Display Driver (IDD/IddCx)
    Splits a physical monitor into two virtual monitors that
    Windows detects as independent display devices.

    Based on Microsoft's IddSampleDriver sample:
    https://github.com/microsoft/Windows-driver-samples/tree/main/video/IndirectDisplay

License: MIT
Project: https://github.com/your-org/MonitorSplit

--*/

#pragma once

// WDF + IddCx headers
#define IDDCX_VERSION_MAJOR 1
#define IDDCX_VERSION_MINOR 4
#define IDDCX_VERSION 0x1400

#include <windows.h>
#include <wdf.h>
#include <iddcx/1.4/IddCx.h>
#include <avrt.h>

// Standard C++ headers
#include <vector>
#include <memory>
#include <string>

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DEVINTERFACE_MONITORSPLIT =
    { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };

//
// IOCTL Definitions — communication between driver and usermode app
//
#define MONITORSPLIT_IOCTL_BASE         0x8000
#define IOCTL_MONITORSPLIT_SET_CONFIG   CTL_CODE(FILE_DEVICE_VIDEO, MONITORSPLIT_IOCTL_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITORSPLIT_GET_CONFIG   CTL_CODE(FILE_DEVICE_VIDEO, MONITORSPLIT_IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITORSPLIT_ENABLE       CTL_CODE(FILE_DEVICE_VIDEO, MONITORSPLIT_IOCTL_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MONITORSPLIT_DISABLE      CTL_CODE(FILE_DEVICE_VIDEO, MONITORSPLIT_IOCTL_BASE + 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Configuration structure shared between driver and app
//
#pragma pack(push, 1)
typedef struct _MONITORSPLIT_CONFIG {
    UINT32  Version;            // Config version (currently 1)
    UINT32  Monitor1Width;      // Width of left virtual monitor (pixels)
    UINT32  Monitor1Height;     // Height of left virtual monitor (pixels)
    UINT32  Monitor1RefreshHz; // Refresh rate of left monitor
    UINT32  Monitor2Width;      // Width of right virtual monitor (pixels)
    UINT32  Monitor2Height;     // Height of right virtual monitor (pixels)
    UINT32  Monitor2RefreshHz; // Refresh rate of right monitor
    BOOL    IsEnabled;          // Whether the split is active
} MONITORSPLIT_CONFIG, *PMONITORSPLIT_CONFIG;
#pragma pack(pop)

#define MONITORSPLIT_CONFIG_VERSION 1

//
// Default split configuration (1920x1080 physical → 960x1080 each)
//
#define DEFAULT_MONITOR1_WIDTH    960
#define DEFAULT_MONITOR1_HEIGHT   1080
#define DEFAULT_MONITOR1_REFRESH  60
#define DEFAULT_MONITOR2_WIDTH    960
#define DEFAULT_MONITOR2_HEIGHT   1080
#define DEFAULT_MONITOR2_REFRESH  60

// Maximum number of virtual monitors (we create exactly 2)
#define MONITORSPLIT_MONITOR_COUNT 2

//
// Device context — per-device state
//
typedef struct _DEVICE_CONTEXT {
    IDDCX_ADAPTER       Adapter;
    WDFDEVICE           WdfDevice;
    MONITORSPLIT_CONFIG Config;
    BOOLEAN             MonitorsCreated;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(DEVICE_CONTEXT);

//
// Monitor context — per-monitor state
//
typedef struct _MONITOR_CONTEXT {
    IDDCX_MONITOR       Monitor;
    UINT32              MonitorIndex;   // 0 = left, 1 = right
    UINT32              Width;
    UINT32              Height;
    UINT32              RefreshHz;
} MONITOR_CONTEXT, *PMONITOR_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(MONITOR_CONTEXT);

//
// Swap chain context — per swapchain rendering state
//
typedef struct _SWAPCHAIN_CONTEXT {
    HANDLE              hThread;
    HANDLE              hTerminateEvent;
    IDDCX_SWAPCHAIN     SwapChain;
    IDDCX_MONITOR       Monitor;
} SWAPCHAIN_CONTEXT, *PSWAPCHAIN_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(SWAPCHAIN_CONTEXT);


//
// Function prototypes
//

// Driver entry and device callbacks
extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD           MonitorSplit_EvtDriverDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY             MonitorSplit_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT              MonitorSplit_EvtDeviceD0Exit;

// IddCx adapter callbacks
EVT_IDD_CX_ADAPTER_INIT_FINISHED    MonitorSplit_EvtAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES     MonitorSplit_EvtAdapterCommitModes;
EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION MonitorSplit_EvtParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES MonitorSplit_EvtMonitorGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES MonitorSplit_EvtMonitorQueryTargetModes;

// IddCx swapchain callbacks
EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN  MonitorSplit_EvtMonitorAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN MonitorSplit_EvtMonitorUnassignSwapChain;

// IOCTL handler
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  MonitorSplit_EvtIoDeviceControl;

// Internal helpers
NTSTATUS MonitorSplit_CreateMonitors(PDEVICE_CONTEXT pDevCtx);
NTSTATUS MonitorSplit_CreateSingleMonitor(PDEVICE_CONTEXT pDevCtx, UINT32 index);
void     MonitorSplit_FillMonitorModes(UINT32 width, UINT32 height, UINT32 refreshHz,
                                       IDDCX_MONITOR_MODE* pModes, UINT32* pModeCount);
DWORD WINAPI MonitorSplit_SwapChainThread(LPVOID pContext);
