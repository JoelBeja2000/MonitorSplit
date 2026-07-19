/*++

Module Name:
    Trace.h

Abstract:
    WPP Tracing macros for MonitorSplitDriver.
    Enables ETW-based debug tracing via WPP.

License: MIT
Project: MonitorSplit

--*/

#pragma once

//
// Define the tracing GUID for MonitorSplit
// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        MonitorSplitTraceGuid, \
        (A1B2C3D4, E5F6, 7890, AB, CD, EF, 12, 34, 56, 78, 90), \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO) \
    )

//
// This comment block is required by WPP preprocessor.
// begin_wpp config
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// FUNC TraceEntry();
// FUNC TraceExit();
// end_wpp
//

#define TraceEntry() TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "-->%!FUNC!")
#define TraceExit()  TraceEvents(TRACE_LEVEL_INFORMATION, MYDRIVER_ALL_INFO, "<--%!FUNC!")

// Convenience macros
#define TraceError(fmt, ...)   TraceEvents(TRACE_LEVEL_ERROR,       MYDRIVER_ALL_INFO, fmt, __VA_ARGS__)
#define TraceWarning(fmt, ...) TraceEvents(TRACE_LEVEL_WARNING,     MYDRIVER_ALL_INFO, fmt, __VA_ARGS__)
#define TraceInfo(fmt, ...)    TraceEvents(TRACE_LEVEL_INFORMATION,  MYDRIVER_ALL_INFO, fmt, __VA_ARGS__)
#define TraceVerbose(fmt, ...) TraceEvents(TRACE_LEVEL_VERBOSE,      MYDRIVER_ALL_INFO, fmt, __VA_ARGS__)
