/*******************************************************************************
** © Copyright 2012 - 2013 Xilinx, Inc. All rights reserved.
** This file contains confidential and proprietary information of Xilinx, Inc. and 
** is protected under U.S. and international copyright and other intellectual property laws.
*******************************************************************************
**   ____  ____ 
**  /   /\/   / 
** /___/  \  /   Vendor: Xilinx 
** \   \   \/    
**  \   \
**  /   /          
** /___/    \
** \   \  /  \   Kintex-7 PCIe-DMA-DDR3-10GMAC-10GBASER Targeted Reference Design
**  \___\/\___\
** 
**  Device: xc7k325t
**  Version: 1.0
**  Reference: UG927
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


**  Critical Applications:
**
**    Xilinx products are not designed or intended to be fail-safe, or for use in any application 
**    requiring fail-safe performance, such as life-support or safety devices or systems, 
**    Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
**    or any other applications that could lead to death, personal injury, or severe property or 
**    environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
**    the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
**    to applicable laws and regulations governing limitations on product liability.

**  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.

*******************************************************************************/
/*****************************************************************************/
/**
*
* @file XNet_Main.h
*
* This file contains defines and version number info for the NDIS Driver.
*
* MODIFICATION HISTORY:
*
* Ver     Date   Changes
* ----- -------- -------------------------------------------------------
* 1.0   11/18/12 First release 
*
******************************************************************************/
/***************************** Include Files *********************************/


#ifndef _XNET_Main_H
#define _XNET_Main_H

#define	TRACE_ENABLED		0

//
// Update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version.
// Also make sure that VER_FILEVERSION specified in the .RC file also
// matches with the driver version because NDISTESTER checks for that.
//
// Currently version 1.0.
//
#define NIC_MAJOR_DRIVER_VERSION           0x01
#define NIC_MINOR_DRIVER_VERISON           0x00
#define NIC_VENDOR_DRIVER_VERSION          ((NIC_MAJOR_DRIVER_VERSION << 16) | NIC_MINOR_DRIVER_VERISON)



//
// Define the NDIS miniport interface version that this driver targets.
//
#if defined(NDIS60_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION             0
#elif defined(NDIS620_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION            20
#else
#  error Unsupported NDIS version
#endif


//
// Memory allocation tags to help track and debug memory usage.  Change these
// for your miniport.  You can add or remove tags as needed.
//
#define NIC_TAG								((ULONG)'_XLX')  // XLX_
#define NIC_TAG_TCB							((ULONG)'TXLX')  // XLXT
#define NIC_TAG_RCB							((ULONG)'RXLX')  // XLXR
#define NIC_TAG_RECV_NBL					((ULONG)'rXLX')  // XLXr
#define NIC_TAG_SG							((ULONG)'sXLX')  // XLXF

#define NIC_ADAPTER_CHECK_FOR_HANG_TIME_IN_SECONDS 4


//
// Buffer size passed in NdisMQueryAdapterResources
// We should only need three adapter resources (IO, interrupt and memory),
// Some devices get extra resources, so have room for 10 resources
//
#define NIC_RESOURCE_BUF_SIZE \
        (sizeof(NDIS_RESOURCE_LIST) + \
         (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))


//
// Utility macros
// -----------------------------------------------------------------------------
//

#ifndef min
#define    min(_a, _b)      (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define    max(_a, _b)      (((_a) > (_b)) ? (_a) : (_b))
#endif



#define LIST_ENTRY_FROM_NBL(_NBL) ((PLIST_ENTRY)&(_NBL)->MiniportReserved[0])
#define NBL_FROM_LIST_ENTRY(_ENTRY) (CONTAINING_RECORD(_ENTRY, NET_BUFFER_LIST, MiniportReserved[0]))

// Get a pointer to a LIST_ENTRY for the receive free list, from an NBL pointer
#define RECV_FREE_LIST_FROM_NBL(_NBL) LIST_ENTRY_FROM_NBL(_NBL)

// Get a pointer to a NBL, from a pointer to a LIST_ENTRY on the receive free list
#define NBL_FROM_RECV_FREE_LIST(_ENTRY) NBL_FROM_LIST_ENTRY(_ENTRY)


// Get a pointer to a LIST_ENTRY for the cancel list, from an NBL pointer
#define CANCEL_LIST_FROM_NBL(_NBL) ((PSINGLE_LIST_ENTRY)&(_NBL)->MiniportReserved[0])

// Get a pointer to a NBL, from a pointer to a LIST_ENTRY on the cancel list
#define NBL_FROM_CANCEL_LIST(_ENTRY) NBL_FROM_LIST_ENTRY(_ENTRY)

// Get a pointer to a LIST_ENTRY for the send wait list, from an NB pointer
#define SEND_WAIT_LIST_FROM_NB(_NB) ((PLIST_ENTRY)&(_NB)->MiniportReserved[0])

// Get a pointer to a NB, from a pointer to a LIST_ENTRY on the send wait list
#define NB_FROM_SEND_WAIT_LIST(_ENTRY) (CONTAINING_RECORD(_ENTRY, NET_BUFFER, MiniportReserved[0]))

// Get a pointer to an RCB from an NBL
#define RCB_FROM_NBL(_NBL) (*((PRCB*)&(_NBL)->MiniportReserved[0]))

// Gets the number of outstanding TCBs/NET_BUFFERs associated with
// this NET_BUFFER_LIST
#define SEND_REF_FROM_NBL(_NBL) (*((PLONG)&(_NBL)->MiniportReserved[1]))


#define NBL_FROM_SEND_NB(_NB) (*((PNET_BUFFER_LIST*)&(_NB)->MiniportReserved[2]))

#define FRAME_TYPE_FROM_SEND_NB(_NB) (*((PULONG)&(_NB)->MiniportReserved[3]))


#if 0 // Commented out

#define ACQUIRE_NDIS_SPINLOCK(_AtDpc, _SpinLock)\
    if(_AtDpc)\
    {\
        NdisDprAcquireSpinLock(_SpinLock);\
    }\
    else\
    {\
        NdisAcquireSpinLock(_SpinLock);\
    }

#define RELEASE_NDIS_SPINLOCK(_AtDpc, _SpinLock)\
    if(_AtDpc)\
    {\
        NdisDprReleaseSpinLock(_SpinLock);\
    }\
    else\
    {\
        NdisReleaseSpinLock(_SpinLock);\
    }
#endif  // Commented out


struct _MP_ADAPTER;


// Global data
extern NDIS_HANDLE     NdisDriverHandle;
extern NDIS_HANDLE     GlobalDriverContext;

extern NDIS_OID NICSupportedOids[];


// Miniport routines
SET_OPTIONS  MPSetOptions;

// Prototypes for standard NDIS miniport entry points
MINIPORT_INITIALIZE                 MPInitializeEx;
MINIPORT_HALT                       MPHaltEx;
MINIPORT_UNLOAD                     DriverUnload;
MINIPORT_PAUSE                      MPPause;
MINIPORT_RESTART                    MPRestart;
MINIPORT_SEND_NET_BUFFER_LISTS      MPSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS    MPReturnNetBufferLists;
MINIPORT_CANCEL_SEND                MPCancelSend;
MINIPORT_CHECK_FOR_HANG             MPCheckForHangEx;
MINIPORT_RESET                      MPResetEx;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY    MPDevicePnpEventNotify;
MINIPORT_SHUTDOWN                   MPShutdownEx;
MINIPORT_CANCEL_OID_REQUEST         MPCancelOidRequest;


void
MPAttachAdapter(
    __in  struct _MP_ADAPTER *Adapter);

void
MPDetachAdapter(
    __in  struct _MP_ADAPTER *Adapter);

BOOLEAN
MPIsAdapterAttached(
    __in struct _MP_ADAPTER *Adapter);


VOID
DbgPrintOidName(
    __in  NDIS_OID  OidReqQuery);

VOID
DbgPrintAddress(
    __in_bcount(NIC_MACADDR_SIZE) PUCHAR Address);


#endif    // _XNET_Main_H



