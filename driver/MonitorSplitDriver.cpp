/*++

Module Name:
    MonitorSplitDriver.cpp

Abstract:
    MonitorSplit - Windows Indirect Display Driver (IDD/IddCx)
    
    This driver creates two virtual monitors that Windows detects as
    independent display devices, effectively "splitting" a physical monitor.
    
    Based on Microsoft's IddSampleDriver sample from Windows-driver-samples.
    Uses UMDF2 (User-Mode Driver Framework 2.x) + IddCx.

    Architecture:
    - One virtual adapter is created
    - Two virtual monitors are reported via IddCxMonitorArrival
    - Each monitor has its own resolution (configurable via IOCTL)
    - A background thread per monitor processes swap chain frames
    
    To build: Requires Visual Studio 2022 + WDK 10.0.26100 or later.
    See docs/BUILD.md for full instructions.

License: MIT
Project: https://github.com/your-org/MonitorSplit

--*/

#include "MonitorSplitDriver.h"
#include "Edid.h"
#include "Trace.h"
#include "MonitorSplitDriver.tmh"  // WPP generated header

//=============================================================================
// DriverEntry — required entry point for all WDF drivers
//=============================================================================
extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceInfo("MonitorSplit DriverEntry v1.0");

    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, MonitorSplit_EvtDriverDeviceAdd);

    NTSTATUS status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        TraceError("WdfDriverCreate failed: 0x%08X", status);
        WPP_CLEANUP(DriverObject);
    }

    return status;
}

//=============================================================================
// MonitorSplit_EvtDriverDeviceAdd
// Called when PnP manager finds a device matching our INF.
// We set up the WDF device and IddCx adapter here.
//=============================================================================
NTSTATUS MonitorSplit_EvtDriverDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);

    TraceEntry();

    //
    // Configure device as an IDD device
    //
    NTSTATUS status = IddCxDeviceInitConfig(DeviceInit, nullptr);
    if (!NT_SUCCESS(status)) {
        TraceError("IddCxDeviceInitConfig failed: 0x%08X", status);
        return status;
    }

    //
    // Setup PnP/Power callbacks
    //
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDeviceD0Entry = MonitorSplit_EvtDeviceD0Entry;
    pnpCallbacks.EvtDeviceD0Exit  = MonitorSplit_EvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);

    //
    // Allocate a context for the device
    //
    WDF_OBJECT_ATTRIBUTES deviceAttribs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttribs, DEVICE_CONTEXT);

    WDFDEVICE wdfDevice;
    status = WdfDeviceCreate(&DeviceInit, &deviceAttribs, &wdfDevice);
    if (!NT_SUCCESS(status)) {
        TraceError("WdfDeviceCreate failed: 0x%08X", status);
        return status;
    }

    //
    // Initialize device context with default config
    //
    PDEVICE_CONTEXT pDevCtx = WdfObjectGet_DEVICE_CONTEXT(wdfDevice);
    RtlZeroMemory(pDevCtx, sizeof(DEVICE_CONTEXT));
    pDevCtx->WdfDevice = wdfDevice;

    pDevCtx->Config.Version          = MONITORSPLIT_CONFIG_VERSION;
    pDevCtx->Config.Monitor1Width    = DEFAULT_MONITOR1_WIDTH;
    pDevCtx->Config.Monitor1Height   = DEFAULT_MONITOR1_HEIGHT;
    pDevCtx->Config.Monitor1RefreshHz = DEFAULT_MONITOR1_REFRESH;
    pDevCtx->Config.Monitor2Width    = DEFAULT_MONITOR2_WIDTH;
    pDevCtx->Config.Monitor2Height   = DEFAULT_MONITOR2_HEIGHT;
    pDevCtx->Config.Monitor2RefreshHz = DEFAULT_MONITOR2_REFRESH;
    pDevCtx->Config.IsEnabled        = TRUE;

    //
    // Create an IOCTL queue for communication with usermode app
    //
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = MonitorSplit_EvtIoDeviceControl;

    WDFQUEUE queue;
    status = WdfIoQueueCreate(wdfDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (!NT_SUCCESS(status)) {
        TraceError("WdfIoQueueCreate failed: 0x%08X", status);
        return status;
    }

    //
    // Initialize the IddCx adapter
    //
    IDD_CX_CLIENT_CONFIG iddConfig;
    IDD_CX_CLIENT_CONFIG_INIT(&iddConfig);
    iddConfig.EvtIddCxAdapterInitFinished         = MonitorSplit_EvtAdapterInitFinished;
    iddConfig.EvtIddCxAdapterCommitModes          = MonitorSplit_EvtAdapterCommitModes;
    iddConfig.EvtIddCxParseMonitorDescription     = MonitorSplit_EvtParseMonitorDescription;
    iddConfig.EvtIddCxMonitorGetDefaultDescriptionModes = MonitorSplit_EvtMonitorGetDefaultModes;
    iddConfig.EvtIddCxMonitorQueryTargetModes     = MonitorSplit_EvtMonitorQueryTargetModes;
    iddConfig.EvtIddCxMonitorAssignSwapChain      = MonitorSplit_EvtMonitorAssignSwapChain;
    iddConfig.EvtIddCxMonitorUnassignSwapChain    = MonitorSplit_EvtMonitorUnassignSwapChain;

    status = IddCxDeviceInitialize(wdfDevice, &iddConfig);
    if (!NT_SUCCESS(status)) {
        TraceError("IddCxDeviceInitialize failed: 0x%08X", status);
        return status;
    }

    TraceInfo("Device created successfully");
    TraceExit();
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtDeviceD0Entry
// Called when device enters D0 (fully operational) power state.
//=============================================================================
NTSTATUS MonitorSplit_EvtDeviceD0Entry(
    _In_ WDFDEVICE              Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);
    TraceInfo("D0Entry - device powered on");
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtDeviceD0Exit
// Called when device leaves D0 power state.
//=============================================================================
NTSTATUS MonitorSplit_EvtDeviceD0Exit(
    _In_ WDFDEVICE              Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);
    TraceInfo("D0Exit - device powering down");
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtAdapterInitFinished
// Called when the IddCx adapter is fully initialized.
// This is where we announce our virtual monitors to Windows.
//=============================================================================
VOID MonitorSplit_EvtAdapterInitFinished(
    _In_ IDDCX_ADAPTER AdapterObject,
    _In_ const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs)
{
    UNREFERENCED_PARAMETER(pInArgs);

    TraceEntry();

    // Retrieve device context from adapter
    WDFDEVICE wdfDevice = IddCxAdapterWdfDevice(AdapterObject);
    PDEVICE_CONTEXT pDevCtx = WdfObjectGet_DEVICE_CONTEXT(wdfDevice);
    pDevCtx->Adapter = AdapterObject;

    // Create both virtual monitors
    if (NT_SUCCESS(MonitorSplit_CreateMonitors(pDevCtx))) {
        TraceInfo("Both virtual monitors created successfully");
    }
    else {
        TraceError("Failed to create virtual monitors");
    }

    TraceExit();
}

//=============================================================================
// MonitorSplit_CreateMonitors
// Creates both virtual monitors (left and right halves).
//=============================================================================
NTSTATUS MonitorSplit_CreateMonitors(PDEVICE_CONTEXT pDevCtx)
{
    TraceEntry();

    for (UINT32 i = 0; i < MONITORSPLIT_MONITOR_COUNT; i++)
    {
        NTSTATUS status = MonitorSplit_CreateSingleMonitor(pDevCtx, i);
        if (!NT_SUCCESS(status)) {
            TraceError("Failed to create monitor %u: 0x%08X", i, status);
            return status;
        }
    }

    pDevCtx->MonitorsCreated = TRUE;
    TraceExit();
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_CreateSingleMonitor
// Creates one virtual monitor and announces its arrival to IddCx.
//=============================================================================
NTSTATUS MonitorSplit_CreateSingleMonitor(PDEVICE_CONTEXT pDevCtx, UINT32 index)
{
    TraceInfo("Creating monitor %u", index);

    // Determine this monitor's resolution
    UINT32 width    = (index == 0) ? pDevCtx->Config.Monitor1Width  : pDevCtx->Config.Monitor2Width;
    UINT32 height   = (index == 0) ? pDevCtx->Config.Monitor1Height : pDevCtx->Config.Monitor2Height;
    UINT32 refresh  = (index == 0) ? pDevCtx->Config.Monitor1RefreshHz : pDevCtx->Config.Monitor2RefreshHz;

    // Build EDID for this monitor
    BYTE edid[EDID_BLOCK_SIZE];
    BuildEdidBlock(index, (UINT16)width, (UINT16)height, (UINT8)refresh, edid);

    // Create the IddCx monitor object
    IDARG_IN_CREATE_MONITOR monitorArgs = {};
    monitorArgs.MonitorInfo.MonitorDescription.Type        = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
    monitorArgs.MonitorInfo.MonitorDescription.DataSize    = EDID_BLOCK_SIZE;
    monitorArgs.MonitorInfo.MonitorDescription.pData      = edid;
    monitorArgs.MonitorInfo.MonitorDescription.EdidVersion = IDDCX_EDID_OPTION_YCBCR_SUPPORT;

    // Allocate monitor context
    WDF_OBJECT_ATTRIBUTES monitorAttribs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&monitorAttribs, MONITOR_CONTEXT);

    IDARG_OUT_CREATE_MONITOR monitorOut = {};
    NTSTATUS status = IddCxMonitorCreate(pDevCtx->Adapter, &monitorArgs, &monitorAttribs, &monitorOut);
    if (!NT_SUCCESS(status)) {
        TraceError("IddCxMonitorCreate failed for monitor %u: 0x%08X", index, status);
        return status;
    }

    // Initialize monitor context
    PMONITOR_CONTEXT pMonCtx = WdfObjectGet_MONITOR_CONTEXT(monitorOut.MonitorObject);
    pMonCtx->Monitor      = monitorOut.MonitorObject;
    pMonCtx->MonitorIndex = index;
    pMonCtx->Width        = width;
    pMonCtx->Height       = height;
    pMonCtx->RefreshHz    = refresh;

    // Announce monitor arrival to Windows
    IDARG_IN_MONITORCREATE_ARRIVED arrivedArgs = {};
    arrivedArgs.MonitorObject = monitorOut.MonitorObject;
    status = IddCxMonitorArrival(pDevCtx->Adapter, &arrivedArgs);
    if (!NT_SUCCESS(status)) {
        TraceError("IddCxMonitorArrival failed for monitor %u: 0x%08X", index, status);
        return status;
    }

    TraceInfo("Monitor %u created: %ux%u @ %uHz", index, width, height, refresh);
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtAdapterCommitModes
// Called when Windows decides which display mode to use for each monitor.
//=============================================================================
NTSTATUS MonitorSplit_EvtAdapterCommitModes(
    _In_ IDDCX_ADAPTER                        AdapterObject,
    _In_ const IDARG_IN_COMMITMODES*          pInArgs)
{
    UNREFERENCED_PARAMETER(AdapterObject);
    UNREFERENCED_PARAMETER(pInArgs);

    // For a virtual monitor, we always accept the committed modes.
    // A more sophisticated implementation would validate the mode here.
    TraceInfo("CommitModes called — accepting all modes");
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtParseMonitorDescription
// Called to parse the monitor's EDID and extract supported modes.
//=============================================================================
NTSTATUS MonitorSplit_EvtParseMonitorDescription(
    _In_  const IDARG_IN_PARSEMONITORDESCRIPTION*   pInArgs,
    _Out_ IDARG_OUT_PARSEMONITORDESCRIPTION*         pOutArgs)
{
    // Report that we support 1 mode (the primary resolution from our EDID).
    // Windows will call EvtMonitorQueryTargetModes for the actual list.
    pOutArgs->MonitorModeBufferOutputCount = 1;

    if (pInArgs->MonitorModeBufferInputCount < 1)
    {
        // Just reporting the count
        return STATUS_BUFFER_TOO_SMALL;
    }

    // The caller has a buffer — fill in the primary mode
    // We need the monitor context to know this monitor's resolution.
    // IddCx passes the monitor object via pInArgs->MonitorObject.
    PMONITOR_CONTEXT pMonCtx = WdfObjectGet_MONITOR_CONTEXT(pInArgs->MonitorObject);

    IDDCX_MONITOR_MODE mode = {};
    mode.Size               = sizeof(mode);
    mode.Origin             = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
    mode.MonitorVideoSignalInfo.AdditionalSignalInfo.VideoStandard       = AdditionalSignalInfoFlagsNone;
    mode.MonitorVideoSignalInfo.TotalSize.cx                              = pMonCtx->Width + 160;
    mode.MonitorVideoSignalInfo.TotalSize.cy                              = pMonCtx->Height + 45;
    mode.MonitorVideoSignalInfo.ActiveSize.cx                             = pMonCtx->Width;
    mode.MonitorVideoSignalInfo.ActiveSize.cy                             = pMonCtx->Height;
    mode.MonitorVideoSignalInfo.VSyncFreq.Numerator                       = pMonCtx->RefreshHz;
    mode.MonitorVideoSignalInfo.VSyncFreq.Denominator                     = 1;
    mode.MonitorVideoSignalInfo.HSyncFreq.Numerator                       = pMonCtx->RefreshHz * (pMonCtx->Height + 45);
    mode.MonitorVideoSignalInfo.HSyncFreq.Denominator                     = 1;
    mode.MonitorVideoSignalInfo.PixelRate                                  =
        (UINT64)(pMonCtx->Width + 160) * (pMonCtx->Height + 45) * pMonCtx->RefreshHz;
    mode.MonitorVideoSignalInfo.ScanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;

    pInArgs->pMonitorModes[0] = mode;
    pOutArgs->PreferredMonitorModeIdx = 0;

    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtMonitorGetDefaultModes
// Returns the default mode list (called when no EDID is present — not our case,
// but the callback must be implemented).
//=============================================================================
NTSTATUS MonitorSplit_EvtMonitorGetDefaultModes(
    _In_  IDDCX_MONITOR                             MonitorObject,
    _In_  const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs,
    _Out_ IDARG_OUT_GETDEFAULTDESCRIPTIONMODES*      pOutArgs)
{
    UNREFERENCED_PARAMETER(MonitorObject);
    UNREFERENCED_PARAMETER(pInArgs);
    pOutArgs->DefaultMonitorModeBufferOutputCount = 0;
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtMonitorQueryTargetModes
// Returns the list of target modes (output pixel formats + sizes).
//=============================================================================
NTSTATUS MonitorSplit_EvtMonitorQueryTargetModes(
    _In_  IDDCX_MONITOR                          MonitorObject,
    _In_  const IDARG_IN_QUERYTARGETMODES*       pInArgs,
    _Out_ IDARG_OUT_QUERYTARGETMODES*            pOutArgs)
{
    PMONITOR_CONTEXT pMonCtx = WdfObjectGet_MONITOR_CONTEXT(MonitorObject);
    pOutArgs->TargetModeBufferOutputCount = 1;

    if (pInArgs->TargetModeBufferInputCount < 1) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    IDDCX_TARGET_MODE targetMode = {};
    targetMode.Size               = sizeof(targetMode);
    targetMode.TargetVideoSignalInfo.AdditionalSignalInfo.VideoStandard = AdditionalSignalInfoFlagsNone;
    targetMode.TargetVideoSignalInfo.TotalSize.cx      = pMonCtx->Width + 160;
    targetMode.TargetVideoSignalInfo.TotalSize.cy      = pMonCtx->Height + 45;
    targetMode.TargetVideoSignalInfo.ActiveSize.cx     = pMonCtx->Width;
    targetMode.TargetVideoSignalInfo.ActiveSize.cy     = pMonCtx->Height;
    targetMode.TargetVideoSignalInfo.VSyncFreq.Numerator   = pMonCtx->RefreshHz;
    targetMode.TargetVideoSignalInfo.VSyncFreq.Denominator = 1;
    targetMode.TargetVideoSignalInfo.HSyncFreq.Numerator   = pMonCtx->RefreshHz * (pMonCtx->Height + 45);
    targetMode.TargetVideoSignalInfo.HSyncFreq.Denominator = 1;
    targetMode.TargetVideoSignalInfo.PixelRate             =
        (UINT64)(pMonCtx->Width + 160) * (pMonCtx->Height + 45) * pMonCtx->RefreshHz;
    targetMode.TargetVideoSignalInfo.ScanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;

    pInArgs->pTargetModes[0] = targetMode;
    return STATUS_SUCCESS;
}

//=============================================================================
// MonitorSplit_EvtMonitorAssignSwapChain
// Called when Windows is ready to start rendering to this virtual monitor.
// We launch a background thread to consume swap chain frames.
//=============================================================================
VOID MonitorSplit_EvtMonitorAssignSwapChain(
    _In_ IDDCX_MONITOR              MonitorObject,
    _In_ const IDARG_IN_SETSWAPCHAIN* pInArgs)
{
    TraceEntry();

    // Allocate swap chain context
    WDF_OBJECT_ATTRIBUTES scAttribs;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&scAttribs, SWAPCHAIN_CONTEXT);
    scAttribs.ParentObject = MonitorObject;

    WDFOBJECT scObject;
    if (!NT_SUCCESS(WdfObjectCreate(&scAttribs, &scObject))) {
        TraceError("Failed to create swap chain context object");
        return;
    }

    PSWAPCHAIN_CONTEXT pScCtx = WdfObjectGet_SWAPCHAIN_CONTEXT(scObject);
    pScCtx->SwapChain         = pInArgs->hSwapChain;
    pScCtx->Monitor           = MonitorObject;
    pScCtx->hTerminateEvent   = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (!pScCtx->hTerminateEvent) {
        TraceError("Failed to create terminate event");
        return;
    }

    // Launch background thread that processes frames
    pScCtx->hThread = CreateThread(
        nullptr,
        0,
        MonitorSplit_SwapChainThread,
        pScCtx,
        0,
        nullptr);

    if (!pScCtx->hThread) {
        TraceError("Failed to create swap chain thread");
        CloseHandle(pScCtx->hTerminateEvent);
    }

    TraceExit();
}

//=============================================================================
// MonitorSplit_EvtMonitorUnassignSwapChain
// Called when Windows stops rendering to this virtual monitor.
//=============================================================================
VOID MonitorSplit_EvtMonitorUnassignSwapChain(
    _In_ IDDCX_MONITOR MonitorObject)
{
    TraceEntry();

    // Find and signal termination of the swap chain thread
    // (In a robust implementation, store the context and signal it here)
    UNREFERENCED_PARAMETER(MonitorObject);

    TraceExit();
}

//=============================================================================
// MonitorSplit_SwapChainThread
// Background thread that processes rendered frames from the swap chain.
// For a "display splitter" that just creates virtual monitors, we simply
// acknowledge each frame to keep Windows happy.
//=============================================================================
DWORD WINAPI MonitorSplit_SwapChainThread(LPVOID pContext)
{
    PSWAPCHAIN_CONTEXT pScCtx = (PSWAPCHAIN_CONTEXT)pContext;
    
    // Set thread priority for smoother frame timing
    HANDLE hAvrtTask = nullptr;
    DWORD taskIndex = 0;
    hAvrtTask = AvSetMmThreadCharacteristicsW(L"Distribution", &taskIndex);

    TraceInfo("SwapChain thread started");

    while (WaitForSingleObject(pScCtx->hTerminateEvent, 0) == WAIT_TIMEOUT)
    {
        // Acquire the next frame from the swap chain
        IDARG_IN_ACQUIRESIGNALFRAME  acquireIn  = {};
        IDARG_OUT_ACQUIRESIGNALFRAME acquireOut = {};
        
        acquireIn.FrameStatisticsSequenceNumber = 0;

        NTSTATUS status = IddCxSwapChainReleaseAndAcquireBuffer(
            pScCtx->SwapChain,
            &acquireIn,
            &acquireOut);

        if (!NT_SUCCESS(status)) {
            // STATUS_PENDING means no frame yet — spin
            if (status == STATUS_PENDING) {
                Sleep(1);
                continue;
            }
            // Any other error means swap chain was torn down
            TraceError("IddCxSwapChainReleaseAndAcquireBuffer: 0x%08X", status);
            break;
        }

        //
        // At this point, acquireOut.MetaData contains frame metadata and
        // acquireOut.FrameStatistics has timing info. The frame surface is
        // a DirectX texture accessible via the swap chain.
        //
        // For MonitorSplit, we don't need to do anything with the frame content
        // (we're not encoding or redirecting it). The frame is just "consumed"
        // here which is correct behavior for a virtual monitor that doesn't
        // physically display anything — Windows uses its own composition engine
        // to composite windows onto the virtual display.
        //
        // If you want to implement screen capture / streaming, you would
        // access the DirectX surface here using IddCxSwapChainGetMsaaSurface
        // or similar APIs.
        //

        // Release the frame back to the system
        IDARG_IN_RELEASEANDACQUIREBUFFER releaseIn = {};
        releaseIn.FrameStatistics = acquireOut.FrameStatistics;
        IddCxSwapChainReleaseAndAcquireBuffer(pScCtx->SwapChain, (PIDARG_IN_ACQUIRESIGNALFRAME)&releaseIn, &acquireOut);
    }

    if (hAvrtTask) {
        AvRevertMmThreadCharacteristics(hAvrtTask);
    }

    TraceInfo("SwapChain thread exiting");
    return 0;
}

//=============================================================================
// MonitorSplit_EvtIoDeviceControl
// Handles IOCTL requests from the usermode control application.
//=============================================================================
VOID MonitorSplit_EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT pDevCtx = WdfObjectGet_DEVICE_CONTEXT(device);
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    ULONG_PTR bytesWritten = 0;

    switch (IoControlCode)
    {
    case IOCTL_MONITORSPLIT_SET_CONFIG:
    {
        //
        // Receive new configuration from the app and apply it.
        // Note: Changing resolution requires re-creating the monitors.
        //
        PMONITORSPLIT_CONFIG pNewConfig = nullptr;
        size_t configSize = 0;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(MONITORSPLIT_CONFIG),
                                               (PVOID*)&pNewConfig, &configSize);
        if (NT_SUCCESS(status) && pNewConfig->Version == MONITORSPLIT_CONFIG_VERSION)
        {
            TraceInfo("SET_CONFIG: Mon1=%ux%u@%u Mon2=%ux%u@%u",
                pNewConfig->Monitor1Width, pNewConfig->Monitor1Height, pNewConfig->Monitor1RefreshHz,
                pNewConfig->Monitor2Width, pNewConfig->Monitor2Height, pNewConfig->Monitor2RefreshHz);
            RtlCopyMemory(&pDevCtx->Config, pNewConfig, sizeof(MONITORSPLIT_CONFIG));
            status = STATUS_SUCCESS;
        }
        break;
    }

    case IOCTL_MONITORSPLIT_GET_CONFIG:
    {
        //
        // Return the current configuration to the app.
        //
        PMONITORSPLIT_CONFIG pOutConfig = nullptr;
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(MONITORSPLIT_CONFIG),
                                                (PVOID*)&pOutConfig, nullptr);
        if (NT_SUCCESS(status))
        {
            RtlCopyMemory(pOutConfig, &pDevCtx->Config, sizeof(MONITORSPLIT_CONFIG));
            bytesWritten = sizeof(MONITORSPLIT_CONFIG);
            status = STATUS_SUCCESS;
        }
        break;
    }

    case IOCTL_MONITORSPLIT_ENABLE:
        pDevCtx->Config.IsEnabled = TRUE;
        TraceInfo("IOCTL: MonitorSplit ENABLED");
        status = STATUS_SUCCESS;
        break;

    case IOCTL_MONITORSPLIT_DISABLE:
        pDevCtx->Config.IsEnabled = FALSE;
        TraceInfo("IOCTL: MonitorSplit DISABLED");
        status = STATUS_SUCCESS;
        break;

    default:
        TraceWarning("Unknown IOCTL: 0x%08X", IoControlCode);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesWritten);
}
