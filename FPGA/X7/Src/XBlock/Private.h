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

    Private.h

Abstract:

Environment:

    Kernel mode

--*/


#ifndef _XBLOCK_PRIVATE_H_
#define _XBLOCK_PRIVATE_H_

#define	TRACE_ENABLED		0

#define	CANCEL_SUPPORT		1

#define XBLOCK_FSD_VERSION      ((VER_MAJOR_NUM << 16) | (VER_MINOR_NUM << 8) | VER_SUBMINOR_NUM)

#define	DEFAULT_APP					0

#define CS2_DMA_ENGINE_OFFSET		4

#define MAX_CS2_DMA_ENGINE			7

#define	MAX_NUMBER_APPS				4


//
// The device extension for the device object
//
typedef struct _DEVICE_EXTENSION {
    WDFDEVICE               Device;

	// Device IO Control
    WDFQUEUE                IOCtlQueue;

    REGISTER_XDRIVER        DriverLink;
	WDFIOTARGET             IoTarget;

	int						S2CDMAEngine;
	int						C2SDMAEngine;

	// Write
    int                     WriteDMAStatus;
    XDMA_HANDLE             DMAWriteHandle;
    REGISTER_DMA_ENGINE_RETURN  WriteDMALink;
    LIST_ENTRY              S2CDMATransList;
    WDFSPINLOCK		        S2CDMATransListLock;

    // Read
    int                     ReadDMAStatus;
    XDMA_HANDLE             DMAReadHandle;
    REGISTER_DMA_ENGINE_RETURN  ReadDMALink;
    LIST_ENTRY              C2SDMATransList;
    WDFSPINLOCK		        C2SDMATransListLock;

}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// This will generate the function named XBlockGetDeviceContext to be use for
// retreiving the DEVICE_EXTENSION pointer.
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, XBlockGetDeviceContext)

// Function prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD XBlockEvtDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP XBlockEvtDriverContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY XBlockEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT XBlockEvtDeviceD0Exit;

EVT_WDF_IO_QUEUE_IO_READ XBlockEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE XBlockEvtIoWrite;

NTSTATUS
XBlockInitializeDeviceExtension(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
XBlockInitRead(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
XBlockInitWrite(
    IN PDEVICE_EXTENSION DevExt
    );

VOID
XBlockReadRequestComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pXferParams
    );

VOID
XBlockWriteRequestComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pXferParams
    );

EVT_WDF_REQUEST_CANCEL XBlockReadRequestCancel;
EVT_WDF_REQUEST_CANCEL XBlockWriteRequestCancel;

VOID XBlockReadRequestCancel(
    IN WDFREQUEST	Request
	);

VOID XBlockWriteRequestCancel(
    IN WDFREQUEST	Request
	);

VOID
XBlockShutdown(
    IN PDEVICE_EXTENSION DevExt
    );

EVT_WDF_PROGRAM_DMA XBlockEvtProgramReadDma;
EVT_WDF_PROGRAM_DMA XBlockEvtProgramWriteDma;


#pragma warning(disable:4127) // avoid conditional expression is constant error with W4

#endif  // _XBLOCK_PRIVATE_H_


