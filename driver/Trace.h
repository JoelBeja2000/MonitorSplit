/*++

Module Name:
    Trace.h

Abstract:
    Custom Debug Tracing macros for MonitorSplitDriver.
    Uses standard OutputDebugStringA to write trace logs to debugger consoles
    (e.g., Sysinternals DebugView) without needing WPP preprocessor tools.

License: MIT
Project: MonitorSplit

--*/

#pragma once

#include <windows.h>
#include <strsafe.h>

// Helper trace printer function
inline void TraceMessage(const char* level, const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    
    // Prefix logs for easy filtering in DebugView
    StringCchCopyA(buffer, 512, "[MonitorSplit] ");
    StringCchCatA(buffer, 512, level);
    StringCchCatA(buffer, 512, ": ");
    
    char msg[256];
    StringCchVPrintfA(msg, 256, format, args);
    StringCchCatA(buffer, 512, msg);
    StringCchCatA(buffer, 512, "\n");
    
    OutputDebugStringA(buffer);
    va_end(args);
}

// Tracing macro overrides
#define TraceEntry()           TraceMessage("INFO", "--> Entering %s", __FUNCTION__)
#define TraceExit()            TraceMessage("INFO", "<-- Exiting %s", __FUNCTION__)
#define TraceError(fmt, ...)   TraceMessage("ERROR", fmt, __VA_ARGS__)
#define TraceWarning(fmt, ...) TraceMessage("WARNING", fmt, __VA_ARGS__)
#define TraceInfo(fmt, ...)    TraceMessage("INFO", fmt, __VA_ARGS__)
#define TraceVerbose(fmt, ...) TraceMessage("VERBOSE", fmt, __VA_ARGS__)

// WPP compatibility macros (disabled since we use OutputDebugString)
#define WPP_INIT_TRACING(a, b)
#define WPP_CLEANUP(a)
