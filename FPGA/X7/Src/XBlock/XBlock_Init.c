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

    XBlock_Init.c

Abstract:

    Contains most of initialization functions

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XBlock_Init.tmh"
#endif // TRACE_ENABLED


/*++
Routine Description:

    This routine is called by EvtDeviceAdd. Here the device context is
    initialized and all the software resources required by the device is
    allocated.

Arguments:

    DevExt     Pointer to the Device Extension

Return Value:

     NTSTATUS

--*/
NTSTATUS
XBlockInitializeDeviceExtension(
    IN PDEVICE_EXTENSION    pDevExt
    )
{
    WDF_IO_QUEUE_CONFIG     queueConfig;
    NTSTATUS                status;

    //PAGED_CODE();

	// Setup the linkage for the Read and Write function calls.
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE( &queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoRead			= XBlockEvtIoRead;
    queueConfig.EvtIoWrite			= XBlockEvtIoWrite;
    status = WdfIoQueueCreate( pDevExt->Device,
                               &queueConfig,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &pDevExt->IOCtlQueue );

    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "WdfIoQueueCreate (Read & Write) failed: 0x%x", status);
        return status;
    }

	// Make sure we indicate the DMA Engines are not setup
	pDevExt->WriteDMAStatus = NOT_INITIALIZED;
	pDevExt->ReadDMAStatus  = NOT_INITIALIZED;

	// And they are not bound.
	pDevExt->DMAWriteHandle = INVALID_XDMA_HANDLE;
	pDevExt->DMAReadHandle  = INVALID_XDMA_HANDLE;

	pDevExt->S2CDMAEngine		= DEFAULT_APP;
	pDevExt->C2SDMAEngine		= DEFAULT_APP + CS2_DMA_ENGINE_OFFSET;


	// Initialize the DMA Transaction list for Writes
	InitializeListHead(&pDevExt->S2CDMATransList);
	// And the spinlock to protect the queue
	status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &pDevExt->S2CDMATransListLock);
	if (!NT_SUCCESS(status)) 
	{
		DEBUGP(DEBUG_ERROR, "S2C Transaction Lock WdfSpinLockCreate failed: 0x%x", status);
	    return status;
	}

	// Initialize the DMA Transaction list for Reads
	InitializeListHead(&pDevExt->C2SDMATransList);
	// And the spinlock to protect the queue
	status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &pDevExt->C2SDMATransListLock);
	if (!NT_SUCCESS(status)) 
	{
		DEBUGP(DEBUG_ERROR, "C2S Transaction Lock WdfSpinLockCreate failed: 0x%x", status);
	    return status;
	}

    return status;
}



/*++
Routine Description:

    Link to an XDMA S2C DMA Engine
 
Arguments:

    DevExt     Pointer to Device Extension

Return Value:

    None

--*/
NTSTATUS
XBlockInitWrite(
    IN PDEVICE_EXTENSION    pDevExt
    )
{
    REGISTER_DMA_ENGINE_REQUEST     Request;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    DEBUGP(DEBUG_TRACE, "--> XBlockInitWrite");

    pDevExt->WriteDMAStatus = INITIALIZING;

	Request.DMA_ENGINE = pDevExt->S2CDMAEngine;
	Request.DIRECTION = WdfDmaDirectionWriteToDevice;
	Request.XDMA_DEVICE_CONTEXT = pDevExt->DriverLink.XDMA_DEVICE_CONTEXT;
	Request.FSD_DEVICE_CONTEXT = pDevExt;
	Request.FSDCompletion = XBlockWriteRequestComplete;
	Request.FSD_VERSION = XBLOCK_FSD_VERSION;

	pDevExt->DMAWriteHandle = pDevExt->DriverLink.DmaRegister(&Request, &pDevExt->WriteDMALink);
	if (pDevExt->DMAWriteHandle != INVALID_XDMA_HANDLE)
	{
		if (pDevExt->WriteDMALink.XDMA_DMA_VERSION == XBLOCK_FSD_VERSION)
		{
			pDevExt->WriteDMAStatus = AVAILABLE;
			status = STATUS_SUCCESS;
		}
	}
	else
	{
//	    DEBUGP(DEBUG_TRACE, "Invalid XDMA handle in Xblock Init write");
		pDevExt->WriteDMAStatus = FAILED;
	}
//	DEBUGP(DEBUG_TRACE, "<-- Succefully registered with engine %d XBlockInitWrite",Request.DMA_ENGINE );
	DEBUGP(DEBUG_TRACE, "<-- XBlockInitWrite");
    return status;
}


/*++
Routine Description:

    Link to a XDMA Driver C2S DMA Engine

Arguments:

    DevExt     Pointer to Device Extension

Return Value:

--*/
NTSTATUS
XBlockInitRead(
    IN PDEVICE_EXTENSION    pDevExt
    )
{
    REGISTER_DMA_ENGINE_REQUEST     Request;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;

    DEBUGP(DEBUG_TRACE, "--> XBlockInitRead");

    pDevExt->ReadDMAStatus = INITIALIZING;

	Request.DMA_ENGINE = pDevExt->C2SDMAEngine;
	Request.DIRECTION = WdfDmaDirectionReadFromDevice;
	Request.XDMA_DEVICE_CONTEXT = pDevExt->DriverLink.XDMA_DEVICE_CONTEXT;
	Request.FSD_DEVICE_CONTEXT = pDevExt;
	Request.FSDCompletion = XBlockReadRequestComplete;
	Request.FSD_VERSION = XBLOCK_FSD_VERSION;

	pDevExt->DMAReadHandle = pDevExt->DriverLink.DmaRegister(&Request, &pDevExt->ReadDMALink);
    if (pDevExt->DMAReadHandle != INVALID_XDMA_HANDLE)
	{
		if (pDevExt->ReadDMALink.XDMA_DMA_VERSION == XBLOCK_FSD_VERSION)
		{
			pDevExt->ReadDMAStatus = AVAILABLE;
			status = STATUS_SUCCESS;
		}
	}
	else
	{
//	    DEBUGP(DEBUG_TRACE, "Invalid XDMA handle in Xblock Init Read");
		pDevExt->ReadDMAStatus = FAILED;
	}
//	DEBUGP(DEBUG_TRACE, "<-- Succefully registered with engine %d XBlockInitRead",Request.DMA_ENGINE );
	DEBUGP(DEBUG_TRACE, "<-- XBlockInitRead");
    return status;
}


/*++

Routine Description:

    Reset the device to put the device in a known initial state.
    This is called from D0Exit when the device is torn down or
    when the system is shutdown. Note that Wdf has already
    called out EvtDisable callback to disable the interrupt.

Arguments:

    DevExt -  Pointer to our adapter

Return Value:

    None

--*/
VOID
XBlockShutdown(
    IN PDEVICE_EXTENSION	pDevExt
    )
{
	DEBUGP(DEBUG_TRACE, "---> XBlockShutdown");

	pDevExt->WriteDMAStatus = SHUTDOWN;
	pDevExt->ReadDMAStatus = SHUTDOWN;

	if (pDevExt->DMAWriteHandle != INVALID_XDMA_HANDLE)
	{
		pDevExt->DriverLink.DmaUnregister(pDevExt->DMAWriteHandle);
		pDevExt->DMAWriteHandle = INVALID_XDMA_HANDLE;
	}

	if (pDevExt->DMAReadHandle != INVALID_XDMA_HANDLE)
	{
		pDevExt->DriverLink.DmaUnregister(pDevExt->DMAReadHandle);
		pDevExt->DMAReadHandle = INVALID_XDMA_HANDLE;
	}

	if (pDevExt->IoTarget != NULL)
	{
		WdfObjectDelete(pDevExt->IoTarget);
		pDevExt->IoTarget = NULL;
	}

    DEBUGP(DEBUG_TRACE, "<--- XBlockShutdown");
}





