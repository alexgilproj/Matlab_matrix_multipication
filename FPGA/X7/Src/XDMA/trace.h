/* $Id: */
/*******************************************************************************
** © Copyright 2012 Xilinx, Inc. All rights reserved.
** This file contains confidential and proprietary information of Xilinx, Inc. and 
** is protected under U.S. and international copyright and other intellectual property laws.
*******************************************************************************
**   ____  ____ 
**  /   /\/   / 
** /___/  \  /   Vendor: Xilinx 
** \   \   \/    
**  \   \        
**  /   /          
** /___/   /\     
** \   \  /  \
**  \___\/\___\ 
** 
*******************************************************************************
**
**  Disclaimer: 
**
**    This disclaimer is not a license and does not grant any rights to the materials 
**    distributed herewith. Except as otherwise provided in a valid license issued to you 
**    by Xilinx, and to the maximum extent permitted by applicable law: 
**    (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
**    AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
**    INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
**    FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
**    or tort, including negligence, or under any other theory of liability) for any loss or damage 
**    of any kind or nature related to, arising under or in connection with these materials, 
**    including for any direct, or any indirect, special, incidental, or consequential loss 
**    or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
**    as a result of any action brought by a third party) even if such damage or loss was 
**    reasonably foreseeable or Xilinx had been advised of the possibility of the same.
**
**  Critical Applications:
**
**    Xilinx products are not designed or intended to be fail-safe, or for use in any application 
**    requiring fail-safe performance, such as life-support or safety devices or systems, 
**    Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
**    or any other applications that could lead to death, personal injury, or severe property or 
**    environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
**    the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
**    to applicable laws and regulations governing limitations on product liability.
**
**  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.
**
*******************************************************************************/
/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

	Portions copyright Pro Code Works, LLC.

--*/


#if TRACE_ENABLED

#include <evntrace.h> // For TRACE_LEVEL definitions


//
// If software tracing is defined in the sources file..
// WPP_DEFINE_CONTROL_GUID specifies the GUID used for this driver.
// 
// *** IF YOU MODIFY THIS DRIVER, REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
// 
// WPP_DEFINE_BIT allows setting debug bit masks to selectively print.
// The names defined in the WPP_DEFINE_BIT call define the actual names
// that are used to control the level of tracing for the control guid
// specified.
//
// Name of the logger is XDMA and the guid is
//  {068ED462-FDF5-4D2D-ABDB-B98309D66408}
//  (0x68ed462, 0xfdf5, 0x4d2d, 0xab, 0xdb, 0xb9, 0x83, 0x9, 0xd6, 0x64, 0x8);

#define WPP_CHECK_FOR_NULL_STRING  //to prevent exceptions due to NULL strings

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(XDMATraceGuid, (068ED462, FDF5, 4D2D, ABDB, B98309D66408),\
        WPP_DEFINE_BIT(DBG_INIT)             /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(DBG_PNP)              /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(DBG_POWER)            /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(DBG_WMI)              /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(DBG_CREATE_CLOSE)     /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(DBG_IOCTLS)           /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(DBG_WRITE)            /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(DBG_READ)             /* bit  7 = 0x00000080 */ \
        WPP_DEFINE_BIT(DBG_DPC)              /* bit  8 = 0x00000100 */ \
        WPP_DEFINE_BIT(DBG_INTERRUPT)        /* bit  9 = 0x00000200 */ \
        WPP_DEFINE_BIT(DBG_LOCKS)            /* bit 10 = 0x00000400 */ \
        WPP_DEFINE_BIT(DBG_QUEUEING)         /* bit 11 = 0x00000800 */ \
        WPP_DEFINE_BIT(DBG_HW_ACCESS)        /* bit 12 = 0x00001000 */ \
        /* You can have up to 32 defines. If you want more than that,\
            you have to provide another trace control GUID */\
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)

#define	DEBUG_VERBOSE	TRACE_LEVEL_VERBOSE
#define	DEBUG_INFO		TRACE_LEVEL_INFORMATION
#define	DEBUG_TRACE		TRACE_LEVEL_INFORMATION
#define	DEBUG_WARN		TRACE_LEVEL_WARNING
#define	DEBUG_ERROR		TRACE_LEVEL_ERROR


// If you want to use Microsoft's tracing facility you will have to
// serach and replace all DEBUGP macros with TraceEvents.  This is due to
// the way the tracing has been implemented. It cannot be used in a macro
// for easy substitution.  We have found that tracing is difficult to setup
// and use. Clients have found it much easier to use the debug print filter 
// method over tracing.
//
//  For example replace:
//		DEBUGP(DEBUG_INFO
//  with:
//		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT
//

// Make DEBUGP macros null 
#define	DEBUGP(level, ...)

#else

extern ULONG DbgComponentID;

#if DBG

// To use Debug Print open Regedit on the machine running the drivers.
//  Goto HKLM\SYSTEM\CurrentControlSet\Control\Session Manager
//    if the "Debug Print Filter" key does not exist create it.
//    In the HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Debug Print Filter 
//       section add a 32 bit DWORD value called "IHVDRIVER" or "IHVNETWORK" for NDIS
//       drivers.  Set the value to:
//			1 for ERRORs only,
//			2 for ERRORs and WARNINGs,
//			3 for ERRORs, WARNINGs and Info,
//			4 to recieve all messages
//
#define	DEBUG_VERBOSE	DPFLTR_INFO_LEVEL + 1
#define	DEBUG_INFO		DPFLTR_INFO_LEVEL
#define	DEBUG_TRACE		DPFLTR_TRACE_LEVEL
#define	DEBUG_WARN		DPFLTR_WARNING_LEVEL
#define	DEBUG_ERROR		DPFLTR_ERROR_LEVEL
#define	DEBUG_ALWAYS	DPFLTR_ERROR_LEVEL

#define DEBUGP(level, ...) \
{\
	KdPrintEx((DbgComponentID, level, "XDMA.SYS:")); \
    KdPrintEx((DbgComponentID, level, __VA_ARGS__)); \
	KdPrintEx((DbgComponentID, level, "\n")); \
} 

#else // Not DEBUG

// Make DEBUGP macros null 
#define	DEBUGP(level, ...)

#endif // DEBUG

#endif // Tracing versus Debug Print.