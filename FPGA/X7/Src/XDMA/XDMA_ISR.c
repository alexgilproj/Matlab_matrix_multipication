/* $Id: */
/*******************************************************************************
** � Copyright 2012 Xilinx, Inc. All rights reserved.
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

    XDMA_ISR.c

Abstract:

    Contains routines related to interrupt handling and deferred proccessing.

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_ISR.tmh"
#endif // TRACE_ENABLED


//#ifdef USE_HW_INTERRUPTS
/**
    @doc
        This method is to configure and create the WDFINTERRUPT object.
        This routine is called by EvtDeviceAdd callback.

    @param
        pDevExt - is a pointer to our DEVICE_EXTENSION.
    
    @return
        NTSTATUS.
*/
NTSTATUS
XlxInterruptCreate(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    NTSTATUS                    status;
    WDF_INTERRUPT_CONFIG        InterruptConfig;

    WDF_INTERRUPT_CONFIG_INIT( &InterruptConfig, XlxEvtInterruptIsr, NULL);
    InterruptConfig.EvtInterruptEnable  = XlxEvtInterruptEnable;
    InterruptConfig.EvtInterruptDisable = XlxEvtInterruptDisable;
    InterruptConfig.AutomaticSerialization = FALSE;

    status = WdfInterruptCreate( pDevExt->Device,
                                 &InterruptConfig,
                                 WDF_NO_OBJECT_ATTRIBUTES,
                                 &pDevExt->Interrupt);
    if (!NT_SUCCESS(status))
    {
		DEBUGP(DEBUG_ERROR, "WdfInterruptCreate failed: %x", status);
    }
    return status;
}


/**
    @doc
        This method is to Interrupt handler for this driver. Called at 
        DIRQL level when the device or another device sharing the same interrupt
        line asserts the interrupt. The driver first checks the device to make
        sure whether this interrupt is generated by its device
        and if so ackknowledge the interrupt and queue a DMA
        Engine specific DPC to do other I/O work related to
        interrupt.

    @param
        Interrupt - is a handle to WDFINTERRUPT object for this device.
    
    @param
        MessageID - is a MSI message ID. (Not Used)
    
    @return
        TRUE  -  This device generated the interrupt.
        FALSE -  This device did not generate the interrupt.
*/
BOOLEAN
XlxEvtInterruptIsr(
    IN WDFINTERRUPT Interrupt,
    IN ULONG        MessageID
    )
{
    PDEVICE_EXTENSION       pDevExt;
    PDMA_ENGINE_EXTENSION   pDMAExt;
    BOOLEAN                 isRecognized = FALSE;
    unsigned long           Status;
    int                     dmaEngineIdx;

    UNREFERENCED_PARAMETER(MessageID);

//	DEBUGP(DEBUG_INFO, "XlxEvtInterruptIsr ---->");
    pDevExt = XlxGetDeviceContext(WdfInterruptGetDevice(Interrupt));

	/* We know we have a valid interrupt, iterate through all the DMA Engines */
    for (dmaEngineIdx = 0; dmaEngineIdx < pDevExt->NumberDMAEngines; dmaEngineIdx++)
    {
		pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        if (pDMAExt != NULL)
        {
			/* Check the individual DMA Engines status for interrupts
               It could have arrived after we read the Globals Status above */
			Status = pDMAExt->pDMAEngRegs->CONTROL.ULONG;
            if ((Status & DMA_ENG_INTERRUPT_ENABLE) &&
				(Status & DMA_ENG_INTERRUPT_ACTIVE))
            {
#if DBG
				if (pDMAExt->pDMAEngRegs->CONTROL.ULONG == (DMA_ENG_DESCRIPTOR_ALIGNMENT_ERROR |
                                                            DMA_ENG_DESCRIPTOR_FETCH_ERROR |
                                                            DMA_ENG_SW_ABORT_ERROR))
                {
					DEBUGP(DEBUG_ERROR,
						"DMA Channel reported error, ControlStatus Reg: 0x%x", pDMAExt->pDMAEngRegs->CONTROL.ULONG);
				}
#endif // DEBUG Version (i.e.Checked)
					
				// Queue up the DPC for this DMA Engine
				if (!(WdfDpcEnqueue(pDMAExt->CompletionDPC)))
				{
					DEBUGP(DEBUG_INFO, "DPC already queued");
				}
				_InterlockedIncrement(&pDMAExt->InterruptsPerSecond);
			    isRecognized = TRUE;
            }
        }
	}
    return isRecognized;
}


/*++

Routine Description:

    Per DMA Engine DPC callback for ISR. Our architecture has one DPC
    for every DMA Engine on a board. 
 
    Please note that on a multiprocessor system,you could have more
    than one DPCs running simulataneously on multiple processors.
    So if you are accesing any global resources make sure to
    synchrnonize the accesses with a spinlock.

Arguments:

    Interupt  - Handle to WDFINTERRUPT Object for this device.
    Device    - WDFDEVICE object passed to InterruptCreate

Return Value:

--*/
VOID
XlxDMADPC(
    IN WDFDPC   Dpc
    )
{
	PDPC_CONTEXT            pDPCCtx;
	PDMA_ENGINE_EXTENSION   pDMAExt;
	PDMA_DESCRIPTOR         pDesc;
	ULONG			        CachedStatusCount = 0;
	PDATA_TRANSFER_PARAMS   pXferParams;
	ULONG                   i = 0;

	pDPCCtx = DPCContext(Dpc);
	pDMAExt = pDPCCtx->pDMAExt;

	/* Acknowledge the Interrupt(s) */
	pDMAExt->pDMAEngRegs->CONTROL.BIT.INTERRUPT_ACTIVE = 1;

	// Grab and hold the spinlock for the entire duration of the DPC. 
	// We do not want another DPC to run and mess up our DMA device queue.
	WdfSpinLockAcquire(pDMAExt->QueueTailLock);

	_InterlockedIncrement(&pDMAExt->DPCsPerSecond);

	pDesc = pDMAExt->pTailDMADescriptor;

	// Is this a Read/C2S?
	if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
	{
		// Look for completed DMA Descriptor(s), this could be completed or errored
		while (i < pDMAExt->PktCoalesceCount)
		{
			if (pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
			{
				// Update the tail pointer since we are counting the current descriptor as completed
				pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

				// Cache the status and byte count.
				CachedStatusCount = pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
				// Make sure we indicate we are done with this descriptor.
				pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;

				if (CachedStatusCount & DMA_DESC_ERROR)
				{
					DEBUGP(DEBUG_ERROR, "XlxDMADPC: C2S pDesc 0x%p, Error bit set 0x%x", 
							pDesc, CachedStatusCount);
				}

				if (pDesc->XFER_PARAMS != NULL)
				{
					_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
					// Read the Transaction Pointer.
					pXferParams = pDesc->XFER_PARAMS;
					// Clear the DMA Trans link.
					pDesc->XFER_PARAMS = NULL;

					if (CachedStatusCount & DMA_DESC_SOP)
					{
						// Since this is the first DMA Descriptor, reset the byte count.
						pXferParams->BytesTransferred = 0;
						// And the status...
						pXferParams->Status = 0;
					}
					// Accumulate the byte count
					pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
					// And status
					pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
							CachedStatusCount & DMA_DESC_SHORT);

					// See if this is the last descriptor for this packet
					if (CachedStatusCount & DMA_DESC_EOP)
					{
						// Keep a running tally for stats purposes
						pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
						WdfSpinLockRelease(pDMAExt->QueueTailLock);
						// Call the Function Specific Driver to complete the transaction
						pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						i++;
						WdfSpinLockAcquire(pDMAExt->QueueTailLock);
					}
				}
				else
				{  
				// This case is for Internal DMA transfers that do not have requests associated to them.
	            pDMAExt->SWrate += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;;
				}
				// Fetch the new tail and start the search again.
				pDesc = pDMAExt->pTailDMADescriptor;
			}
			else
			{
			    DEBUGP(DEBUG_INFO, "XlxDMADPC: C2S pDesc 0x%p,  Status byte 0x%x, Error in xdma_xfer completion interrupt logic. i = %d", 
							pDesc, pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG,i);
				break;
			}
		}
#if 0
		if(i == pDMAExt->PktCoalesceCount)
		{
		    DEBUGP(DEBUG_ALWAYS, "XlxDMADPC: Successfully processed %d C2S packets in DPC", i);
		}
#endif
	}
	else // Must be a S2C transfer
	{

		// Look for completed DMA Descriptor(s), this could be completed or errored
		while (i < pDMAExt->PktCoalesceCount)
		{
			if(pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
			{
				// Update the tail pointer since we are counting the current descriptor as completed
				pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

				CachedStatusCount = pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
				// Make sure we indicate we are done with this descriptor.
				pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;

				if (CachedStatusCount & DMA_DESC_ERROR)
				{
					DEBUGP(DEBUG_ERROR, "XlxDMADPC: S2C pDesc 0x%p, Error bit set 0x%x", 
							pDesc, CachedStatusCount);
				}

				if (pDesc->XFER_PARAMS != NULL)
				{
					_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
					// Read the Transaction Pointer.
					pXferParams = pDesc->XFER_PARAMS;
					// Clear the DMA Trans link.
					pDesc->XFER_PARAMS = NULL;

					// Is this the first DMA descriptor of a packet?
					if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.SOP)
					{
						// Since this is the first DMA Descriptor, reset the byte count.
						pXferParams->BytesTransferred = 0;
						// And the status...
						pXferParams->Status = 0;
					}
					// Accumulate the byte count
					pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
					// And status
					pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
							CachedStatusCount & DMA_DESC_SHORT);

					// See if this is the last descriptor for this packet
					if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.EOP)
					{
						// Keep a running tally for stats purposes
						pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
						WdfSpinLockRelease(pDMAExt->QueueTailLock);
						// Call the Function Specific Driver to complete the transaction
						pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						i++;
						WdfSpinLockAcquire(pDMAExt->QueueTailLock);

					}
				}
				else
				{
				    // This case is for Internal DMA transfers that do not have requests associated to them.
	                pDMAExt->SWrate += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;;
				}
				// Fetch the new tail and start the search again.
				pDesc = pDMAExt->pTailDMADescriptor;
			}
			else
			{
			    DEBUGP(DEBUG_INFO, "XlxDMADPC: S2C pDesc 0x%p, status byte 0x%x, Error in xdma_xfer completion interrupt logic. i = %d", 
							pDesc, CachedStatusCount,i);
				break;
			}
		}
#if 0
		if(i == pDMAExt->PktCoalesceCount)
		{
		    DEBUGP(DEBUG_ALWAYS, "XlxDMADPC: Successfully processed %d S2C packets in DPC", i);
		}
#endif
	}

	WdfSpinLockRelease(pDMAExt->QueueTailLock);
	return;
}


/**
    @doc
        This method is called by the framework at DIRQL immediately 
        after registering the ISR with the kernel by calling IoConnectInterrupt.

    @param
        Interrupt - is a handle to WDFINTERRUPT Object for this device.
        
    @param
        Device    - is a WDFDEVICE object passed to InterruptCreate.
    
    @return
        NTSTATUS.
*/
NTSTATUS
XlxEvtInterruptEnable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
    )
{
    PDEVICE_EXTENSION  pDevExt;
	UNREFERENCED_PARAMETER(Device);

    pDevExt  = XlxGetDeviceContext(WdfInterruptGetDevice(Interrupt));
    // Enable the Global Adapter Interrupts
    pDevExt->pDMARegisters->COMMON_CONTROL.COMMON_CONTROL.BIT.GLOBAL_INTERRUPT_ENABLE = 1;
    return STATUS_SUCCESS;
}

/**
    @doc
        This method is called by the framework at DIRQL before Deregistering
        the ISR with the kernel by calling IoDisconnectInterrupt.

    @param
        Interrupt - is a handle to WDFINTERRUPT Object for this device.
        
    @param
        Device    - is a WDFDEVICE object passed to InterruptCreate.
    
    @return
        NTSTATUS.
*/
NTSTATUS
XlxEvtInterruptDisable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
    )
{
    PDEVICE_EXTENSION  pDevExt; 
	UNREFERENCED_PARAMETER(Device);

    pDevExt  = XlxGetDeviceContext(WdfInterruptGetDevice(Interrupt));
    // Disable the Global Interrupts by writing 0 to the control register
    pDevExt->pDMARegisters->COMMON_CONTROL.COMMON_CONTROL.BIT.GLOBAL_INTERRUPT_ENABLE = 0;
    return STATUS_SUCCESS;
}


/**
    @doc
        Use a One Millisecond interval polling timer for instead of using hardware interrupts.

    @param
        Timer - is a handle to WDFTIMER Object for this device.
        
    @return
        None
*/
VOID
XDMAPollTimer(
   IN WDFTIMER	Timer
   )
{
	PDEVICE_EXTENSION	pDevExt = XlxGetDeviceContext(WdfTimerGetParentObject(Timer));
    PDMA_ENGINE_EXTENSION   pDMAExt = NULL;
    PDMA_DESCRIPTOR         pDesc;
    PDATA_TRANSFER_PARAMS   pXferParams;
	unsigned long			CachedStatusCount = 0;
    int                     dmaEngineIdx;

	for (dmaEngineIdx = 0; dmaEngineIdx < pDevExt->NumberDMAEngines; dmaEngineIdx++)
	{
		pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
	    if (pDMAExt != NULL)
		{
			_InterlockedIncrement(&pDMAExt->DPCsPerSecond);

		    pDesc = pDMAExt->pTailDMADescriptor;
			if (pDesc == NULL)
			{
				continue;
			}
			// Acknoledge the packets are complete.
			pDMAExt->pDMAEngRegs->CONTROL.BIT.DESCRIPTOR_COMPLETE = 1;

			// Is this a Read/C2S?
			if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
			{
				// Look for completed DMA Descriptor(s), this could be completed or errored
				while (pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
				{
					// Update the tail pointer since we are counting the current descriptor as completed
					pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

					// Cache the status and byte count.
					CachedStatusCount = pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
					// Make sure we indicate we are done with this descriptor.
					pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;

					if (CachedStatusCount & DMA_DESC_ERROR)
					{
						DEBUGP(DEBUG_ERROR, "XlxDMADPC: C2S pDesc 0x%p, Error bit set 0x%x", 
							pDesc, pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG);
					}

					if (pDesc->XFER_PARAMS != NULL)
					{
						_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
						// Read the Transaction Pointer.
						pXferParams = pDesc->XFER_PARAMS;
						// Clear the DMA Trans link.
						pDesc->XFER_PARAMS = NULL;
							
						if (CachedStatusCount & DMA_DESC_SOP)
						{
							// Since this is the first DMA Descriptor, reset the byte count.
							pXferParams->BytesTransferred = 0;
							// And the status...
							pXferParams->Status = 0;
						}
						// Accumulate the byte count
						pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
						// And status
						pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
												CachedStatusCount & DMA_DESC_SHORT);

						// See if this is the last descriptor for this packet
						if (CachedStatusCount & DMA_DESC_EOP)
						{
							// Keep a running tally for stats purposes
							pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
							// Call the Function Specific Driver to complete the transaction
							pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						}
					}
					else
					{
						// This case is for Internal DMA transfers that do not have requests associated to them.
					    pDMAExt->SWrate += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
					}
					// Fetch the new tail and start the search again.
					pDesc = pDMAExt->pTailDMADescriptor;
				}
			}
			else // Must be a S2C transfer
			{
				// Look for completed DMA Descriptor(s), this could be completed or errored
				while (pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
				{
					// Update the tail pointer since we are counting the current descriptor as completed
					pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

					CachedStatusCount = pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
					// Make sure we indicate we are done with this descriptor.
					pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG &= DMA_DESC_BYTE_COUNT_MASK;

					if (CachedStatusCount & DMA_DESC_ERROR)
					{
						DEBUGP(DEBUG_ERROR, "XlxDMADPC: S2C pDesc 0x%p, Error bit set 0x%x", 
							pDesc, pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG);
					}
	
					if (pDesc->XFER_PARAMS != NULL)
					{
						_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
						// Read the Transaction Pointer.
					    pXferParams = pDesc->XFER_PARAMS;
						// Clear the DMA Trans link.
						pDesc->XFER_PARAMS = NULL;

						// Is this the first DMA descriptor for this packet?
						if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.SOP)
						{
							// Since this is the first DMA Descriptor, reset the byte count.
							pXferParams->BytesTransferred = 0;
							// And the status...
							pXferParams->Status = 0;
						}
						// Accumulate the byte count
						pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
						// And status
						pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
												CachedStatusCount & DMA_DESC_SHORT);

						// See if this is the last descriptor for this packet
						if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.EOP)
						{
							// Keep a running tally for stats purposes
				            pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
							// Call the Function Specific Driver to complete the transaction
							pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						}
					}
					else
					{
						// This case is for Internal DMA transfers that do not have requests associated to them.
					    pDMAExt->SWrate += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
					}
					// Fetch the new tail and start the search again.
					pDesc = pDMAExt->pTailDMADescriptor;
				}
			}
		}
	}
}

#if 1
/**
    @doc
        Use a One Millisecond interval polling timer for instead of using hardware interrupts.

    @param
        Timer - is a handle to WDFTIMER Object for this device.
        
    @return
        None
*/
VOID
XDMAPollEngineTimer(
   IN WDFTIMER	Timer
   )
{
	PDEVICE_EXTENSION	pDevExt = XlxGetDeviceContext(WdfTimerGetParentObject(Timer));
    PDMA_ENGINE_EXTENSION   pDMAExt = NULL;
    PDMA_DESCRIPTOR         pDesc;
    PDATA_TRANSFER_PARAMS   pXferParams;
	unsigned long			CachedStatusCount = 0;
    int                     dmaEngineIdx;

	for (dmaEngineIdx = 0; dmaEngineIdx < pDevExt->NumberDMAEngines; dmaEngineIdx++)
	{
		pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
		if (pDMAExt != NULL)
		{
		    if(Timer == pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle)
			    break;
		}
	}

	if(dmaEngineIdx != pDevExt->PollEngineHandle[dmaEngineIdx].engineIdxToPoll)
	    DEBUGP(DEBUG_ERROR, "XDMAPollEngineTimer: EngineIndex not equal to the one obtained from for loop.\
							Some error in logic");

	    if (pDMAExt != NULL)
		{
			_InterlockedIncrement(&pDMAExt->DPCsPerSecond);

		    pDesc = pDMAExt->pTailDMADescriptor;
			if (pDesc == NULL)
			{
				return;
			}
			// Acknoledge the packets are complete.
			//pDMAExt->pDMAEngRegs->CONTROL.BIT.DESCRIPTOR_COMPLETE = 1;

			// Is this a Read/C2S?
			if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
			{
				// Look for completed DMA Descriptor(s), this could be completed or errored
				while (pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
				{
					// Update the tail pointer since we are counting the current descriptor as completed
					pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

					// Cache the status and byte count.
					CachedStatusCount = pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
					// Make sure we indicate we are done with this descriptor.
					pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;

					if (CachedStatusCount & DMA_DESC_ERROR)
					{
						DEBUGP(DEBUG_ERROR, "XlxDMADPC: C2S pDesc 0x%p, Error bit set 0x%x", 
							pDesc, pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG);
					}

					if (pDesc->XFER_PARAMS != NULL)
					{
						_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
						// Read the Transaction Pointer.
						pXferParams = pDesc->XFER_PARAMS;
						// Clear the DMA Trans link.
						pDesc->XFER_PARAMS = NULL;
							
						if (CachedStatusCount & DMA_DESC_SOP)
						{
							// Since this is the first DMA Descriptor, reset the byte count.
							pXferParams->BytesTransferred = 0;
							// And the status...
							pXferParams->Status = 0;
						}
						// Accumulate the byte count
						pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
						// And status
						pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
												CachedStatusCount & DMA_DESC_SHORT);

						// See if this is the last descriptor for this packet
						if (CachedStatusCount & DMA_DESC_EOP)
						{
							// Keep a running tally for stats purposes
							pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
							// Call the Function Specific Driver to complete the transaction
							pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						}
					}
					// Fetch the new tail and start the search again.
					pDesc = pDMAExt->pTailDMADescriptor;
				}
			}
			else // Must be a S2C transfer
			{
				// Look for completed DMA Descriptor(s), this could be completed or errored
				while (pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG & (DMA_DESC_COMPLETE | DMA_DESC_ERROR))
				{
					// Update the tail pointer since we are counting the current descriptor as completed
					pDMAExt->pTailDMADescriptor = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;

					CachedStatusCount = pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG;
					// Make sure we indicate we are done with this descriptor.
					pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG &= DMA_DESC_BYTE_COUNT_MASK;

					if (CachedStatusCount & DMA_DESC_ERROR)
					{
						DEBUGP(DEBUG_ERROR, "XlxDMADPC: S2C pDesc 0x%p, Error bit set 0x%x", 
							pDesc, pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG);
					}
	
					if (pDesc->XFER_PARAMS != NULL)
					{
						_InterlockedIncrement(&pDMAExt->AvailNumberDescriptors);
						// Read the Transaction Pointer.
					    pXferParams = pDesc->XFER_PARAMS;
						// Clear the DMA Trans link.
						pDesc->XFER_PARAMS = NULL;

						// Is this the first DMA descriptor for this packet?
						if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.SOP)
						{
							// Since this is the first DMA Descriptor, reset the byte count.
							pXferParams->BytesTransferred = 0;
							// And the status...
							pXferParams->Status = 0;
						}
						// Accumulate the byte count
						pXferParams->BytesTransferred += CachedStatusCount & DMA_DESC_BYTE_COUNT_MASK;
						// And status
						pXferParams->Status |= ((CachedStatusCount & DMA_DESC_ERROR) | 
												CachedStatusCount & DMA_DESC_SHORT);

						// See if this is the last descriptor for this packet
						if (pDesc->S2C.CONTROL_BYTE_COUNT.BIT.EOP)
						{
							// Keep a running tally for stats purposes
				            pDMAExt->SWrate += (LONG)pXferParams->BytesTransferred;
							// Call the Function Specific Driver to complete the transaction
							pDMAExt->FSDCompletionFunc(pDMAExt->FSDContext, pXferParams);
						}
					}
					// Fetch the new tail and start the search again.
					pDesc = pDMAExt->pTailDMADescriptor;
				}
			}
		}
}
#endif
