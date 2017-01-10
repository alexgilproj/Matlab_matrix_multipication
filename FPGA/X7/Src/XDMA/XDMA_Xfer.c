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

    XDMA_Xfer.c

Abstract:

    Contains most of the DMA Transfer functions

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_Xfer.tmh"
#endif // TRACE_ENABLED


/*++
Routine Description:
 
    XlxDataTransfer setups up the DMA Descriptor(s) using the parameters
    passed in the structure, starts the DMA and return immediately.

Arguments:

    XDMAHandle  Handle for the DMA Engine which is the pointer to the DMA Engine Context
    PDATA_TRANSFER_PARAMS Contains all of the information necessary to so a DMA transfer.
 
Return Value:

     NTSTATUS - STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES if not enoguh
                DMA Descriptors are available.

--*/
NTSTATUS
XlxDataTransfer(
    IN XDMA_HANDLE              XDMAHandle, 
    IN PDATA_TRANSFER_PARAMS    pXferParams
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt = (PDMA_ENGINE_EXTENSION)XDMAHandle;
    PSCATTER_GATHER_LIST    SgList = pXferParams->SG_LIST_PTR;
    PDMA_DESCRIPTOR         pDesc;
    PDMA_DESCRIPTOR         pLastDesc = NULL;
    ULONG32                 CtrlFlags = 0;
    ULONG                   DescIdx;
    NTSTATUS                status = STATUS_SUCCESS;

    if (pDMAExt->EngineState != REGISTERED)
	{
		DEBUGP(DEBUG_ALWAYS, "XlxDataTransfer, DMA Engine %d not register state", pDMAExt->DMAEngineNumber);
	    return STATUS_INVALID_DEVICE_STATE;
	}

    // Acquire the lock for the head of the queue, we need to serialize the
    //  DMA Engine queue updates
    WdfSpinLockAcquire(pDMAExt->QueueHeadLock);
    if (pDMAExt->AvailNumberDescriptors >= (LONG)SgList->NumberOfElements)
    {
        // Is this a Read/C2S or a Write/S2C transfer?
        if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
        {
            // Get the address of the next available DMA Descriptor
            pDesc = pDMAExt->pNextDMADescriptor;
#if 0
            // Set the Start of Packet bit
            CtrlFlags = (DMA_DESC_IRQ_COMPLETE | DMA_DESC_IRQ_ERROR);
#endif

            // Now go build the DMA Descriptor list
            for (DescIdx = 0; DescIdx < SgList->NumberOfElements; DescIdx++)
            {
				if (pDesc->XFER_PARAMS != NULL)
				{
					DEBUGP(DEBUG_ERROR, "XlxDataTransfer, DMA Descriptor 0x%p already in use state", pDesc);
				}
			    //Set Interrupt on Error for all Buffer Descriptors.
			    //Set Interrupt on Completion based on Coaelse count.
			    CtrlFlags |= DMA_DESC_IRQ_ERROR;

                // Is this the last DMA Descriptor?, if so set EOP and IRQ flags
                if (DescIdx == (SgList->NumberOfElements - 1))
                {
				    pDMAExt->IntrBDCount += 1;
					if(pDMAExt->IntrBDCount == pDMAExt->PktCoalesceCount)
					{
					    pDMAExt->IntrBDCount = 0;
                        CtrlFlags |= (DMA_DESC_IRQ_COMPLETE | DMA_DESC_IRQ_ERROR);
						//DEBUGP(DEBUG_ALWAYS, "XlxDataTransfer, IRQ Completion set for C2S DMA Descriptor 0x%p", pDesc);
					}
                }

				pDesc->C2S.STATUS_FLAG_BYTE_COUNT.BIT.BYTE_COUNT = 0;
                pDesc->C2S.USER_STATUS.ULONG64 = 0;
#ifdef ADDRESSABLE_SUPPORT
                pDesc->C2S.CARD_ADDRESS = (ULONG32)(pXferParams->CardAddress & 0xFFFFFFFF);
                pDesc->C2S.CONTROL_BYTE_COUNT.ULONG_REG = ((ULONG32)((pXferParams->CardAddress & 0xF00000000) >> 12)) |
                                                           (SgList->Elements[DescIdx].Length & DMA_DESC_BYTE_COUNT_MASK) |
                                                           CtrlFlags;
                pXferParams->CardAddress += SgList->Elements[DescIdx].Length;
#else
                pDesc->C2S.CARD_ADDRESS = 0;
				pDesc->C2S.CONTROL_BYTE_COUNT.ULONG_REG = SgList->Elements[DescIdx].Length & DMA_DESC_BYTE_COUNT_MASK | CtrlFlags;
#endif // ADDRESSABLE_SUPPORT

                pDesc->C2S.SYSTEM_ADDRESS_PHYS.ULONG64 = SgList->Elements[DescIdx].Address.QuadPart;
                pDesc->XFER_PARAMS = pXferParams;

                pLastDesc = pDesc;
                _InterlockedDecrement(&pDMAExt->AvailNumberDescriptors);
                // Link to the next DMA Descriptor
                pDesc = pDesc->NEXT_DESC_VIRT_PTR;
            }
            // Save the link to the next available
            pDMAExt->pNextDMADescriptor = pDesc;
            // Start the DMA by moving the pointer forward, the Engine is already running.
            pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pLastDesc->C2S.NEXT_DESC_PHYS_PTR;
        }
        else // Must be S2C.
        {
            // Get the address of the next available DMA Descriptor
            pDesc = pDMAExt->pNextDMADescriptor;

            // Now go build the DMA Descriptor list
            for (DescIdx = 0; DescIdx < SgList->NumberOfElements; DescIdx++)
            {
				if (pDesc->XFER_PARAMS != NULL)
				{
					DEBUGP(DEBUG_ERROR, "XlxDataTransfer, DMA Descriptor 0x%p already in use state", pDesc);
				}
			    //Set Interrupt on Error for all Buffer Descriptors.
			    //Set Interrupt on Completion based on Coaelse count.
                CtrlFlags |= DMA_DESC_IRQ_ERROR;

				// Set the Start of Packet bit
                if (DescIdx == 0)
                    CtrlFlags |= (ULONG)(DMA_DESC_SOP);

				// Is this the last DMA Descriptor?, if so set EOP and IRQ flags
                if (DescIdx == (SgList->NumberOfElements - 1))
                {
                    CtrlFlags |= (ULONG)(DMA_DESC_EOP);
					pDMAExt->IntrBDCount += 1;
					if(pDMAExt->IntrBDCount == pDMAExt->PktCoalesceCount)
					{
					    pDMAExt->IntrBDCount = 0;
                        CtrlFlags |= (DMA_DESC_IRQ_COMPLETE | DMA_DESC_IRQ_ERROR);
						DEBUGP(DEBUG_TRACE, "XlxDataTransfer, IRQ Completion set for S2C DMA Descriptor 0x%p", pDesc);
					}
                }
                pDesc->S2C.STATUS_FLAG_BYTE_COUNT.BIT.BYTE_COUNT = (SgList->Elements[DescIdx].Length & DMA_DESC_BYTE_COUNT_MASK);
                pDesc->S2C.USER_CONTROL.ULONG64 = 0;
#ifdef ADDRESSABLE_SUPPORT
                pDesc->S2C.CARD_ADDRESS = (ULONG32)(pXferParams->CardAddress & 0xFFFFFFFF);
                pDesc->S2C.CONTROL_BYTE_COUNT.ULONG_REG = ((ULONG32)((pXferParams->CardAddress & 0xF00000000) >> 12)) |
                                                           (SgList->Elements[DescIdx].Length & DMA_DESC_BYTE_COUNT_MASK) |
                                                           CtrlFlags;
                pXferParams->CardAddress += SgList->Elements[DescIdx].Length;
#else
                pDesc->S2C.CARD_ADDRESS = 0;
				pDesc->S2C.CONTROL_BYTE_COUNT.ULONG_REG = (SgList->Elements[DescIdx].Length & DMA_DESC_BYTE_COUNT_MASK) | CtrlFlags;
#endif // ADDRESSABLE_SUPPORT
                pDesc->S2C.SYSTEM_ADDRESS_PHYS.ULONG64 = SgList->Elements[DescIdx].Address.QuadPart;
                pDesc->XFER_PARAMS = pXferParams;

				// Since we may have just filled out the SOP remove it.
				CtrlFlags &= ~(DMA_DESC_SOP);

                pLastDesc = pDesc;
                _InterlockedDecrement(&pDMAExt->AvailNumberDescriptors);
                // Link to the next DMA Descriptor
                pDesc = pDesc->NEXT_DESC_VIRT_PTR;
            }
            // Save the link to the next available
            pDMAExt->pNextDMADescriptor = pDesc;
            // Start the DMA by moving the pointer forward, the Engine is already running.
            pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pLastDesc->S2C.NEXT_DESC_PHYS_PTR;
        }
    }
    else
    {
		DEBUGP(DEBUG_ERROR, "XlxDataTransfer, Not enough descriptors available (%d), need %d",
			pDMAExt->AvailNumberDescriptors, SgList->NumberOfElements);
		pDMAExt->BDerrs++;
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    WdfSpinLockRelease(pDMAExt->QueueHeadLock);
    return status;
}

/*++
Routine Description:
	SetupInternalDMAXfers

Arguments:

    pDMAExt     Pointer to the DMA Engine Context

Return Value:

     NTSTATUS

--*/
NTSTATUS
SetupInternalDMAXfers(
    IN PDMA_ENGINE_EXTENSION    pDMAExt,
	unsigned int				packetSize
    )
{
    PDMA_DESCRIPTOR         pDesc;
    LONG                    DescIdx;
    NTSTATUS                status = STATUS_SUCCESS;

    // Acquire the lock for the head of the queue, we need to serialize the
    //  DMA Engine queue updates
    WdfSpinLockAcquire(pDMAExt->QueueHeadLock);
    if (pDMAExt->AvailNumberDescriptors == (LONG)pDMAExt->TotalNumberDescriptors)
    {
        // Is this a Read/C2S or a Write/S2C transfer?
        if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
        {
            // Get the address of the next available DMA Descriptor
            pDesc = pDMAExt->pNextDMADescriptor;

            // Now go build the DMA Descriptor list
            // Run through all the Read/C2S DMA descriptors and set them accordingly
            for (DescIdx = 0; DescIdx < pDMAExt->TotalNumberDescriptors; DescIdx++)
            {
                pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;
                pDesc->C2S.USER_STATUS.ULONG64 = 0;
                pDesc->C2S.CARD_ADDRESS = 0;
				// We do not use interrupts on internal transfer.
				pDesc->C2S.CONTROL_BYTE_COUNT.ULONG_REG = (ULONG)(packetSize | (DMA_DESC_SOP | DMA_DESC_EOP)); 
				pDesc->C2S.SYSTEM_ADDRESS_PHYS.ULONG64 = pDMAExt->InternalDMABufferPhysAddr.QuadPart;
				pDesc->XFER_PARAMS = NULL;
                _InterlockedDecrement(&pDMAExt->AvailNumberDescriptors);
	            // Save the link to the next available
		        pDesc = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;
				pDMAExt->pNextDMADescriptor = pDesc;
            }
		}
        else // Must be S2C.
        {
            // Get the address of the next available DMA Descriptor
            pDesc = pDMAExt->pNextDMADescriptor;

            // Run through all the Write/S2C DMA descriptors and set them accordingly
            for (DescIdx = 0; DescIdx < pDMAExt->TotalNumberDescriptors; DescIdx++)
            {
                pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;
                pDesc->S2C.USER_CONTROL.ULONG64 = 0;
                pDesc->S2C.CARD_ADDRESS = 0;
				pDesc->S2C.STATUS_FLAG_BYTE_COUNT.BIT.BYTE_COUNT = packetSize;
				// We do not use interrupts on internal transfer.
				pDesc->S2C.CONTROL_BYTE_COUNT.ULONG_REG = (ULONG)(packetSize | (DMA_DESC_SOP | DMA_DESC_EOP));
				pDesc->S2C.SYSTEM_ADDRESS_PHYS.ULONG64 = pDMAExt->InternalDMABufferPhysAddr.QuadPart;
                pDesc->XFER_PARAMS = NULL;
                _InterlockedDecrement(&pDMAExt->AvailNumberDescriptors);
				// Link to the next DMA Descriptor
		        pDesc = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;
				pDMAExt->pNextDMADescriptor = pDesc;
            }
        }
		// Make the DMA Run forever.
		pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = 0;
    }
    else
    {
		DEBUGP(DEBUG_ERROR, "SetupInternalDMAXfers, Not enough descriptors available (%d), Total %d",
			pDMAExt->AvailNumberDescriptors, pDMAExt->TotalNumberDescriptors);
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    WdfSpinLockRelease(pDMAExt->QueueHeadLock);
    return status;
}


/*++
Routine Description:


Arguments:

    pDMAExt     Pointer to the DMA Engine Context

Return Value:

     NTSTATUS

--*/
NTSTATUS
InitializeDMADescriptors(
    IN PDMA_ENGINE_EXTENSION    pDMAExt
    )
{
    PDMA_DESCRIPTOR         pDesc;
    PHYSICAL_ADDRESS        pDescPhys;
    LONG                    DescIdx;
    NTSTATUS                status = STATUS_SUCCESS;

    // Set the max descriptors available
    pDMAExt->AvailNumberDescriptors = pDMAExt->TotalNumberDescriptors;

    // Make sure there is a DMA Descriptor pool already allocated
    if (pDMAExt->pDMADescriptorBase != NULL)
    {
        pDMAExt->pNextDMADescriptor = pDMAExt->pDMADescriptorBase;
        pDesc = pDMAExt->pNextDMADescriptor;
        pDMAExt->pTailDMADescriptor = pDesc;
        pDescPhys = pDMAExt->DMADescriptorBufferPhysAddr;

        // Is this a Read/C2S or a Write/S2C transfer?
        if (pDMAExt->DMADirection == WdfDmaDirectionReadFromDevice)
        {
            // Run through all the Read/C2S DMA descriptors and set them accordingly
            for (DescIdx = 0; DescIdx < pDMAExt->TotalNumberDescriptors; DescIdx++)
            {
                pDesc->C2S.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;
                pDesc->C2S.USER_STATUS.ULONG64 = 0;
                pDesc->C2S.CARD_ADDRESS = 0;
				// Set the flags just in case we get a run away DMA Engine.
				pDesc->C2S.CONTROL_BYTE_COUNT.ULONG_REG = (ULONG)(DMA_DESC_SOP | DMA_DESC_EOP | 
													DMA_DESC_IRQ_ERROR | DMA_DESC_IRQ_COMPLETE);
				pDesc->C2S.SYSTEM_ADDRESS_PHYS.ULONG64 = (ULONG64) -1;
				pDesc->XFER_PARAMS = NULL;

                if (DescIdx == (pDMAExt->TotalNumberDescriptors - 1))
                {
                    // Link the list back to the start
                    pDesc->C2S.NEXT_DESC_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
                    pDesc->NEXT_DESC_VIRT_PTR = (PVOID)pDMAExt->pDMADescriptorBase;
                }
                else
                {
                    // Link to the next descriptor
                    pDescPhys.QuadPart += sizeof(DMA_DESCRIPTOR_ALIGNED);
                    pDesc->C2S.NEXT_DESC_PHYS_PTR = pDescPhys.LowPart;
                    pDesc->NEXT_DESC_VIRT_PTR = (PVOID)((UCHAR *)pDesc + sizeof(DMA_DESCRIPTOR_ALIGNED));
                    pDesc = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;
                }
            }
        }
        else
        {
            // Run through all the Write/S2C DMA descriptors and set them accordingly
            for (DescIdx = 0; DescIdx < pDMAExt->TotalNumberDescriptors; DescIdx++)
            {
                pDesc->S2C.STATUS_FLAG_BYTE_COUNT.ULONG_REG = 0;
                pDesc->S2C.USER_CONTROL.ULONG64 = 0;
                pDesc->S2C.CARD_ADDRESS = 0;
                // Set the flags just in case we get a run away DMA Engine.
	            pDesc->S2C.CONTROL_BYTE_COUNT.ULONG_REG = (ULONG)(DMA_DESC_SOP | DMA_DESC_EOP | 
		                                         DMA_DESC_IRQ_ERROR | DMA_DESC_IRQ_COMPLETE);
			    pDesc->S2C.SYSTEM_ADDRESS_PHYS.ULONG64 = (ULONG64) -1;
                pDesc->XFER_PARAMS = NULL;

                if (DescIdx == (pDMAExt->TotalNumberDescriptors - 1))
                {
                    // Link the list back to the start
                    pDesc->S2C.NEXT_DESC_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
                    pDesc->NEXT_DESC_VIRT_PTR = (PVOID)pDMAExt->pDMADescriptorBase;
                }
                else
                {
                    // Link to the next descriptor
                    pDescPhys.QuadPart += sizeof(DMA_DESCRIPTOR_ALIGNED);
                    pDesc->S2C.NEXT_DESC_PHYS_PTR = pDescPhys.LowPart;
                    pDesc->NEXT_DESC_VIRT_PTR = (PVOID)((UCHAR *)pDesc + sizeof(DMA_DESCRIPTOR_ALIGNED));
                    pDesc = (PDMA_DESCRIPTOR)pDesc->NEXT_DESC_VIRT_PTR;
                }
            }
        }
        // Initialize the DMA Descriptor fetch pointers to the start of the pool
        pDMAExt->pDMAEngRegs->NEXT_DESCRIPTOR_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
        pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
        pDMAExt->pDMAEngRegs->COMPLETED_DESCRIPTOR_PHYS_PTR = 0;
   		// We enable the engine but it will idle until the Software poinnter above is moved unless it
		//   is an internal transfer.
		pDMAExt->pDMAEngRegs->CONTROL.ULONG = (DMA_ENG_INTERRUPT_ENABLE | DMA_ENG_DMA_ENABLE);
    }
    else
    {
		DEBUGP(DEBUG_ERROR, "InitializeDMADescriptors, No descriptors allocated");
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}


/*++
Routine Description:
 
    XlxCancelTransfers cancels all DMA transactions for the given DMA Engine

Arguments:

    XDMAHandle  Handle for the DMA Engine which is the pointer to the DMA Engine Context
 
Return Value:

     NTSTATUS - STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES if not enoguh
                DMA Descriptors are available.

--*/
NTSTATUS
XlxCancelTransfers(
    IN XDMA_HANDLE              XDMAHandle
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt = (PDMA_ENGINE_EXTENSION)XDMAHandle;
	ULONG					currentState;
    NTSTATUS                status = STATUS_SUCCESS;

	DEBUGP(DEBUG_ALWAYS, "Entered XlxCancelTransfers for DMA Engine %d", pDMAExt->DMAEngineNumber);

	// Show that is is not available.
	currentState = pDMAExt->EngineState;
    pDMAExt->EngineState = INITIALIZING;

	// Reset the DMA Engine
	XlxResetDMAEngine(pDMAExt);

	// Go re-initialize the DMA Descriptors and DMA Engine.
	if (InitializeDMADescriptors(pDMAExt) == STATUS_SUCCESS)
	{
		pDMAExt->EngineState = currentState;
	}
    return status;
}

/*++
Routine Description:
 
    XlxResetDMAEngine resets the DMA Engine hardware

Arguments:

    XDMAExt		DMA Engine data
 
Return Value:

--*/
void
XlxResetDMAEngine(
    IN PDMA_ENGINE_EXTENSION    pDMAExt
	)
{
    LARGE_INTEGER           delay;
	int						loopCount;

	// Disable the DMA Engine before we get started.
	pDMAExt->pDMAEngRegs->CONTROL.ULONG = 0;
	// Reset the Engine pointers
	pDMAExt->pDMAEngRegs->NEXT_DESCRIPTOR_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
    pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pDMAExt->DMADescriptorBufferPhysAddr.LowPart;
    pDMAExt->pDMAEngRegs->COMPLETED_DESCRIPTOR_PHYS_PTR = 0;

	pDMAExt->pNextDMADescriptor = pDMAExt->pDMADescriptorBase;
    pDMAExt->pTailDMADescriptor = pDMAExt->pNextDMADescriptor;

	loopCount = 50;
	while (pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RUNNING)
	{
		pDMAExt->pDMAEngRegs->CONTROL.ULONG = 0;
	    //
	    // Wait 10 msec.
		//
		delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(10);
		KeDelayExecutionThread( KernelMode, TRUE, &delay );
		if (--loopCount == 0)
		{
			DEBUGP(DEBUG_ERROR, "XlxResetDMAEngine: DMA Engine %d did not stop", 
				pDMAExt->DMAEngineNumber);
			break;
		}
	}
	pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET_REQUEST = 1;

	loopCount = 50;
	while (pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RUNNING ||
			pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET_REQUEST)
	{
	    //
	    // Wait 10 msec.
		//
		delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(10);
		KeDelayExecutionThread( KernelMode, TRUE, &delay );
		if (--loopCount == 0)
		{
			DEBUGP(DEBUG_ERROR, "XlxResetDMAEngine: DMA Engine %d timed out waiting for request ack", 
				pDMAExt->DMAEngineNumber);
			break;
		}
	}

	// Clear the persistent bits in the control register.
	pDMAExt->pDMAEngRegs->CONTROL.BIT.INTERRUPT_ACTIVE = 1;
	pDMAExt->pDMAEngRegs->CONTROL.BIT.DESCRIPTOR_COMPLETE = 1;
	pDMAExt->pDMAEngRegs->CONTROL.BIT.FETCH_ERROR = 1;
	pDMAExt->pDMAEngRegs->CONTROL.BIT.ALIGNMENT_ERROR = 1;

	// Clear the stats.
	pDMAExt->InterruptsPerSecond = 0;
	pDMAExt->DPCsPerSecond = 0;
	pDMAExt->SWrate = 0;

	pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET = 1;

	loopCount = 50;
	while (pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET)
	{
	    //
		// Wait 10 msec.
		//
		delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(10);
		KeDelayExecutionThread( KernelMode, TRUE, &delay );
		if (--loopCount == 0)
		{
			DEBUGP(DEBUG_ERROR, "XlxResetDMAEngine: DMA Engine %d not respondng to reset", 
				pDMAExt->DMAEngineNumber);
			break;
		}
	}
}