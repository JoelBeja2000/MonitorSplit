/*++

Module Name:
    Edid.h

Abstract:
    EDID (Extended Display Identification Data) generation for
    MonitorSplit virtual monitors. Each virtual monitor reports
    a custom EDID that describes its resolution and capabilities.

    EDID 1.4 structure (128 bytes) is constructed manually.

License: MIT
Project: MonitorSplit

--*/

#pragma once
#include <windows.h>

//
// EDID block is exactly 128 bytes for EDID 1.x
//
#define EDID_BLOCK_SIZE 128

//
// Manufacturer ID encoded as 3 characters ('MSP' for MonitorSplitProject)
// EDID encodes manufacturer as two bytes using 5-bit characters (A=1 ... Z=26)
// 'M'=13, 'S'=19, 'P'=16
// Byte0 = (0 << 7) | ((M-1) << 2) | ((S-1) >> 3) = 0b0_01100_10 = 0x62
// Byte1 = (((S-1) & 7) << 5) | (P-1)              = 0b110_01111 = 0xCF
//
#define EDID_MANUFACTURER_BYTE0  0x62
#define EDID_MANUFACTURER_BYTE1  0xCF

//
// Product code (unique ID per monitor index)
//
#define EDID_PRODUCT_CODE_MONITOR1  0x0001
#define EDID_PRODUCT_CODE_MONITOR2  0x0002

//
// Generate a Detailed Timing Descriptor (DTD) for a given resolution.
// The DTD encodes horizontal and vertical timing parameters.
//
// This uses common CVT/DMT timings. We use safe values for blank/sync.
//
// Parameters:
//   width, height  — active pixels
//   refreshHz      — target refresh rate
//   pDtd           — output buffer (18 bytes)
//
inline void BuildDetailedTimingDescriptor(
    UINT16 width, UINT16 height, UINT8 refreshHz,
    BYTE* pDtd /*[18]*/)
{
    // Simplified timing calculation using CVT-RB (Reduced Blanking) approximations
    // For a proper implementation, calculate actual pixel clock and blanking
    // Here we approximate with common values

    // Horizontal timing
    UINT16 hBlank    = 160;   // typical reduced blanking
    UINT16 hActive   = width;
    UINT16 hTotal    = hActive + hBlank;
    UINT16 hSyncOff  = 48;    // sync offset
    UINT16 hSyncWidth = 32;

    // Vertical timing  
    UINT16 vBlank    = 45;    // lines of blanking
    UINT16 vActive   = height;
    UINT16 vSyncOff  = 3;
    UINT16 vSyncWidth = 5;

    // Pixel clock = total pixels * refresh / 10000 (units: 10kHz)
    UINT32 pixelClockKHz = (UINT32)hTotal * (vActive + vBlank) * refreshHz;
    UINT16 pixelClock10kHz = (UINT16)(pixelClockKHz / 10000);

    ZeroMemory(pDtd, 18);

    // Bytes 0-1: Pixel clock (little endian, units of 10kHz)
    pDtd[0] = (BYTE)(pixelClock10kHz & 0xFF);
    pDtd[1] = (BYTE)(pixelClock10kHz >> 8);

    // Byte 2: H active pixels (lower 8 bits)
    pDtd[2] = (BYTE)(hActive & 0xFF);
    // Byte 3: H blanking (lower 8 bits)
    pDtd[3] = (BYTE)(hBlank & 0xFF);
    // Byte 4: H active (upper 4) | H blank (upper 4)
    pDtd[4] = (BYTE)(((hActive >> 8) << 4) | (hBlank >> 8));

    // Byte 5: V active (lower 8 bits)
    pDtd[5] = (BYTE)(vActive & 0xFF);
    // Byte 6: V blanking (lower 8 bits)
    pDtd[6] = (BYTE)(vBlank & 0xFF);
    // Byte 7: V active (upper 4) | V blank (upper 4)
    pDtd[7] = (BYTE)(((vActive >> 8) << 4) | (vBlank >> 8));

    // Bytes 8-9: H sync offset and width
    pDtd[8] = (BYTE)(hSyncOff & 0xFF);
    pDtd[9] = (BYTE)(hSyncWidth & 0xFF);

    // Byte 10: V sync offset (upper 4) | V sync width (lower 4) — lower bits
    pDtd[10] = (BYTE)(((vSyncOff & 0xF) << 4) | (vSyncWidth & 0xF));

    // Byte 11: H sync off (upper 2) | H sync width (upper 2) | V sync off (upper 2) | V sync width (upper 2)
    pDtd[11] = (BYTE)(((hSyncOff >> 8) << 6) | ((hSyncWidth >> 8) << 4) |
                      ((vSyncOff >> 4) << 2) | (vSyncWidth >> 4));

    // Bytes 12-15: Physical size (leave as 0 = undefined)
    pDtd[12] = 0;
    pDtd[13] = 0;
    pDtd[14] = 0;
    pDtd[15] = 0;

    // Byte 16: H image border (0)
    pDtd[16] = 0;
    // Byte 17: V image border (0) | interlaced (0) | sync type (digital separate, 0b11000)
    pDtd[17] = 0x18;  // Digital Separate Sync, positive H and V polarity
}

//
// Build a complete 128-byte EDID block for a virtual monitor.
//
// Parameters:
//   monitorIndex  — 0 for left monitor, 1 for right monitor
//   width         — active pixel width
//   height        — active pixel height
//   refreshHz     — refresh rate
//   pEdid         — output buffer (EDID_BLOCK_SIZE bytes)
//
inline void BuildEdidBlock(
    UINT32 monitorIndex,
    UINT16 width, UINT16 height, UINT8 refreshHz,
    BYTE* pEdid /*[EDID_BLOCK_SIZE]*/)
{
    ZeroMemory(pEdid, EDID_BLOCK_SIZE);

    // ─── Header ───────────────────────────────────────────────────
    // EDID header: 0x00 FF FF FF FF FF FF 0x00
    pEdid[0] = 0x00;
    pEdid[1] = 0xFF; pEdid[2] = 0xFF; pEdid[3] = 0xFF;
    pEdid[4] = 0xFF; pEdid[5] = 0xFF; pEdid[6] = 0xFF;
    pEdid[7] = 0x00;

    // ─── Vendor & Product ID (bytes 8-17) ─────────────────────────
    pEdid[8]  = EDID_MANUFACTURER_BYTE0;
    pEdid[9]  = EDID_MANUFACTURER_BYTE1;
    // Product code (little endian)
    UINT16 productCode = (monitorIndex == 0) ? EDID_PRODUCT_CODE_MONITOR1 : EDID_PRODUCT_CODE_MONITOR2;
    pEdid[10] = (BYTE)(productCode & 0xFF);
    pEdid[11] = (BYTE)(productCode >> 8);
    // Serial number (32-bit, 0 = not used)
    pEdid[12] = 0x00; pEdid[13] = 0x00; pEdid[14] = 0x00; pEdid[15] = 0x00;
    // Week / Year of manufacture
    pEdid[16] = 0x01;  // week 1
    pEdid[17] = 0x24;  // 2006+36 = year... use 2024: 2024-1990=34, 0x22=34
    pEdid[17] = 0x22;

    // ─── EDID version (bytes 18-19) ───────────────────────────────
    pEdid[18] = 0x01;  // Version 1
    pEdid[19] = 0x04;  // Revision 4

    // ─── Basic Display Parameters (bytes 20-24) ───────────────────
    // Input: Digital, 8 bits per primary, DisplayPort interface
    pEdid[20] = 0xA5;  // 1010_0101: digital, 8bpc, DisplayPort
    // H size (cm) — leave as 0 for undefined
    pEdid[21] = 0x00;
    // V size (cm) — leave as 0 for undefined
    pEdid[22] = 0x00;
    // Gamma = 2.2 → encoded as (gamma*100)-100 = 120 = 0x78
    pEdid[23] = 0x78;
    // Feature support: standby/suspend/active-off = 0, RGB color, preferred timing, sRGB
    pEdid[24] = 0xCE;

    // ─── Chromaticity (bytes 25-34) ───────────────────────────────
    // sRGB chromaticity values (standard values)
    pEdid[25] = 0x00; pEdid[26] = 0xA0;
    pEdid[27] = 0x57; pEdid[28] = 0x4A;
    pEdid[29] = 0x99; pEdid[30] = 0x26;
    pEdid[31] = 0x12; pEdid[32] = 0x48;
    pEdid[33] = 0x4C; pEdid[34] = 0x00;

    // ─── Established Timings (bytes 35-37) ───────────────────────
    // Skip standard legacy timings, we use detailed timing descriptor
    pEdid[35] = 0x00;
    pEdid[36] = 0x00;
    pEdid[37] = 0x00;

    // ─── Standard Timing Information (bytes 38-53) ───────────────
    // Fill with "not used" (0x01 0x01)
    for (int i = 38; i <= 53; i += 2) {
        pEdid[i]   = 0x01;
        pEdid[i+1] = 0x01;
    }

    // ─── Detailed Timing Descriptors (bytes 54-125) ──────────────
    // Descriptor 1: Primary timing (our custom resolution)
    BuildDetailedTimingDescriptor(width, height, refreshHz, &pEdid[54]);

    // Descriptor 2: Monitor name (ASCII descriptor, tag 0xFC)
    {
        BYTE* pDesc = &pEdid[72];
        pDesc[0] = 0x00; pDesc[1] = 0x00; pDesc[2] = 0x00;
        pDesc[3] = 0xFC;  // Monitor name tag
        pDesc[4] = 0x00;
        // "MSP-0\n" or "MSP-1\n" (padded with spaces)
        const char* name = (monitorIndex == 0) ? "MSplit-Left " : "MSplit-Right";
        for (int i = 0; i < 13; i++) {
            pDesc[5 + i] = (i < (int)strlen(name)) ? (BYTE)name[i] : 0x20;
        }
        pDesc[17] = '\n';
    }

    // Descriptor 3: Range Limits (tag 0xFD)
    {
        BYTE* pDesc = &pEdid[90];
        pDesc[0] = 0x00; pDesc[1] = 0x00; pDesc[2] = 0x00;
        pDesc[3] = 0xFD;  // Range limits tag
        pDesc[4] = 0x00;
        pDesc[5] = 48;    // Min vertical field rate (Hz)
        pDesc[6] = 75;    // Max vertical field rate (Hz)
        pDesc[7] = 30;    // Min horizontal line rate (kHz)
        pDesc[8] = 135;   // Max horizontal line rate (kHz)
        pDesc[9] = 28;    // Max pixel clock rate (10MHz units = 280MHz)
        pDesc[10] = 0x00; // Extended timing info (none)
        // Pad with 0x20 0x20...
        for (int i = 11; i < 18; i++) pDesc[i] = 0x20;
        pDesc[11] = '\n';
    }

    // Descriptor 4: Additional serial / leave unused (0x10 tag)
    {
        BYTE* pDesc = &pEdid[108];
        pDesc[0] = 0x00; pDesc[1] = 0x00; pDesc[2] = 0x00;
        pDesc[3] = 0x10;  // Dummy descriptor
        for (int i = 4; i < 18; i++) pDesc[i] = 0x00;
    }

    // ─── Extensions (byte 126) ────────────────────────────────────
    pEdid[126] = 0x00;  // No extension blocks

    // ─── Checksum (byte 127) ──────────────────────────────────────
    // Sum of all bytes 0-126 mod 256 must make byte 127 such that total sum = 0 mod 256
    BYTE sum = 0;
    for (int i = 0; i < 127; i++) {
        sum += pEdid[i];
    }
    pEdid[127] = (BYTE)(256 - sum);
}
