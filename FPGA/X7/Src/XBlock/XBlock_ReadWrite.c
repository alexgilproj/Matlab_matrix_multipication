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

    XBlock_ReadWrite.c

Abstract:


Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XBlock_ReadWrite.tmh"
#endif // TRACE_ENABLED

//-----------------------------------------------------------------------------
//  WRITE Functions
//-----------------------------------------------------------------------------
/*++

Routine Description:

    Called by the framework as soon as it receives a write request.
    If the device is not ready, fail the request.
    Otherwise get scatter-gather list for this request and send the
    packet to the hardware for DMA.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.
    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

--*/
VOID
XBlockEvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
{
    PDEVICE_EXTENSION       pDevExt = XBlockGetDeviceContext(WdfIoQueueGetDevice(Queue));
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDMATRANSACTION       dmaTransaction = NULL;
#ifdef ADDRESSABLE_SUPPORT
    WDF_REQUEST_PARAMETERS  Parameters;
#endif // ADDRESSABLE_SUPPORT
    PDATA_TRANSFER_PARAMS   pDMATrans = NULL;
    BOOLEAN                 bCreated = FALSE;
    NTSTATUS                status = STATUS_UNSUCCESSFUL;

    DEBUGP(DEBUG_TRACE, "--> XBlockEvtIoWrite: Request %p", Request);

    do {
		if (pDevExt->WriteDMAStatus != AVAILABLE)
        {
			DEBUGP(DEBUG_WARN, "Write Warning: WriteDMAStatus != AVAILABLE");
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
        // Validate the Length parameter.
        if (Length > pDevExt->WriteDMALink.MAXIMUM_TRANSFER_LENGTH)  
        {
			DEBUGP(DEBUG_WARN, "Write Warning: Length > pDevExt->WriteDMALink.MAXIMUM_TRANSFER_LENGTH");
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

		// Grab the lock while we allocate and add the DMA transaction to our list.
		//   This protects us during the linking just in case a cancel request comes through.
		WdfSpinLockAcquire(pDevExt->S2CDMATransListLock);
		status = WdfRequestMarkCancelableEx(Request, XBlockWriteRequestCancel);
		if (!NT_SUCCESS(status)) 
		{
			DEBUGP(DEBUG_ERROR, "Write: WdfRequestMarkCancelableEx failed: 0x%x", status);
			WdfSpinLockRelease(pDevExt->S2CDMATransListLock);
            break;
		}

		// Create the DMA transaction object on the fly.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DATA_TRANSFER_PARAMS);
        status = WdfDmaTransactionCreate(pDevExt->WriteDMALink.DMAEnabler,
                                         &attributes,
                                         &dmaTransaction);
        if (!NT_SUCCESS(status)) 
		{
            DEBUGP(DEBUG_ERROR, "Write: WdfDmaTransactionCreate failed %X", status);
			WdfSpinLockRelease(pDevExt->S2CDMATransListLock);
            break;
        }

		bCreated = TRUE;
        pDMATrans = DMATransCtx(dmaTransaction);
        // Save a back pointer to the Transaction object,
        //  we'll use it on the completion callback
        pDMATrans->TRANSACTION_PTR = dmaTransaction;
		// Now put it on our managed list
		InsertTailList(&pDevExt->S2CDMATransList,  &pDMATrans->DT_LINK);
		// Free the List Lock
		WdfSpinLockRelease(pDevExt->S2CDMATransListLock);

		// Extract the Write offset and save it as the card address offset for the device
#ifdef ADDRESSABLE_SUPPORT
        WDF_REQUEST_PARAMETERS_INIT(&Parameters);
        WdfRequestGetParameters(Request, &Parameters);
        pDMATrans->CardAddress = Parameters.Parameters.Read.DeviceOffset;
#else
        pDMATrans->CardAddress = 0;
#endif // ADDRESSABLE_SUPPORT

        // Initialize this new DmaTransaction with the Write Request Info.
        status = WdfDmaTransactionInitializeUsingRequest(dmaTransaction,
                                                         Request,
                                                         XBlockEvtProgramWriteDma,
                                                         WdfDmaDirectionWriteToDevice);
        if (!NT_SUCCESS(status)) 
        {
            DEBUGP(DEBUG_ERROR, "Write: WdfDmaTransactionInitializeUsingRequest failed: 0x%x", status);
            break;
        }

        // Execute this DmaTransaction.
        status = WdfDmaTransactionExecute(dmaTransaction, pDevExt);
        if (!NT_SUCCESS(status)) 
        {
            // Couldn't execute this DmaTransaction, so fail Request.
            DEBUGP(DEBUG_ERROR, "Write: WdfDmaTransactionExecute failed: 0x%x", status);
            break;
        }

        // Indicate that Dma transaction has been started successfully.
        // The request will be complete by the callback routine when the DMA
        // transaction completes.
    } while (0);

    // If there are errors, then clean up and complete the Request.
    if (!NT_SUCCESS(status))
    {
        if (bCreated) 
        {
			// Lock and remove this DMA Transaction from our managed queue
			WdfSpinLockAcquire(pDevExt->S2CDMATransListLock);
			if (RemoveEntryList(&pDMATrans->DT_LINK))
			{
				DEBUGP(DEBUG_ERROR, "XBlockEvtIoWrite:  pDMATrans was not on the list");
			}
			WdfSpinLockRelease(pDevExt->S2CDMATransListLock);
            WdfObjectDelete(dmaTransaction);
        }
        WdfRequestCompleteWithInformation(Request, status, 0);
    }
    DEBUGP(DEBUG_TRACE, "<-- XBlockEvtIoWrite: status 0x%x", status);
    return;
}

/*++

Routine Description:
 
    Prepares the Transfer parameters structure and call down
    to the XDMA driver to do the actual DMA Setup and start
 
Arguments:

Return Value:

--*/
BOOLEAN
XBlockEvtProgramWriteDma(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  PVOID                   Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    SgList
    )
{
    PDEVICE_EXTENSION       pDevExt = (PDEVICE_EXTENSION)Context;
    PDATA_TRANSFER_PARAMS   pDMATrans;
	NTSTATUS				status  = STATUS_SUCCESS;
    BOOLEAN                 success = TRUE;

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER( Direction );

    DEBUGP(DEBUG_TRACE, "--> XBlockEvtProgramWriteDma");

    // Setup the DMA Transaction Context pointer.
    pDMATrans = DMATransCtx(Transaction);
    // Fill out the rest of the transfer structure for the call down to XDMA
    pDMATrans->SG_LIST_PTR      = SgList;

#ifdef ADDRESSABLE_SUPPORT
    // Get the number of bytes as the offset to the beginning of this
    // Dma operations transfer location in the buffer.
    pDMATrans->CardAddress += WdfDmaTransactionGetBytesTransferred(Transaction);
#endif // ADDRESSABLE_SUPPORT

	// We pass the offset to start, the SG List, callback info, etc.
    status = pDevExt->WriteDMALink.XDMADataTransfer(pDevExt->DMAWriteHandle, pDMATrans);

    if (!NT_SUCCESS(status)) 
    {
        // The transaction failed, the DmaTransaction must be deleted and
        //       the Request must be completed.
        WDFREQUEST      Request;

        // Must abort the transaction before deleting it.
        (VOID) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        // Get the associated request from the transaction.
        Request = WdfDmaTransactionGetRequest(Transaction);

		// Lock and remove this DMA Transaction from our managed queue
		WdfSpinLockAcquire(pDevExt->S2CDMATransListLock);
		if (RemoveEntryList(&pDMATrans->DT_LINK))
		{
			DEBUGP(DEBUG_ERROR, "XBlockEvtProgramWriteDma:  pDMATrans was not on the list");
		}
		WdfSpinLockRelease(pDevExt->S2CDMATransListLock);

        // Release the DMA Transaction
        WdfDmaTransactionRelease(Transaction);        
        WdfObjectDelete(Transaction);
		if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED) 
		{
			// Complete the request with status and bytes transferred.
			WdfRequestCompleteWithInformation( Request, STATUS_INVALID_DEVICE_STATE, 0);
		}
		success = FALSE;		
        DEBUGP(DEBUG_ERROR, "<-- XBlockEvtProgramWriteDma: error ****");
    }

    DEBUGP(DEBUG_TRACE, "<-- XBlockEvtProgramWriteDma");
    return success;
}


/*++

Routine Description:
 
    A callback to completes a DMA.  Releases the
    transaction and completes the Request
 
Arguments:
 
    FSD_HANDLE  - Handle which is our context pointer
    PDATA_TRANSFER_PARAMS - The parameters of the DMA transfer

Return Value:
 
    None
 
--*/
VOID
XBlockWriteRequestComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans
    )
{
	PDEVICE_EXTENSION       pDevExt = (PDEVICE_EXTENSION)FSDHandle;
    WDFREQUEST              Request;
    WDFDMATRANSACTION       Transaction;
    size_t                  bytesTransferred;
	BOOLEAN 				TransactionComplete;
    NTSTATUS                Status = STATUS_SUCCESS;

	if (pDMATrans != NULL)
	{
		if (pDMATrans->TRANSACTION_PTR != NULL)
		{
		    // Retrieve the DMA Transaction from the transfer structure
			Transaction = pDMATrans->TRANSACTION_PTR;
			// Get the associated request from the transaction.
			Request = WdfDmaTransactionGetRequest(Transaction);
			if (Request != NULL)
			{
				// Get the final bytes transferred count.
				bytesTransferred =  pDMATrans->BytesTransferred;
				// If any errors are reported, make it a hardware error to Windows.
				if (pDMATrans->Status)
				{
					TransactionComplete = WdfDmaTransactionDmaCompletedFinal(Transaction,
														pDMATrans->BytesTransferred,
														&Status);
					if (pDMATrans->Status == DMA_TRANS_STATUS_SHORT)
					{
						DEBUGP(DEBUG_ALWAYS, "XBlockWriteRequestComplete detected short DMA");
					}
					else if (pDMATrans->Status == (ULONG)STATUS_CANCELLED)
					{
						Status = STATUS_CANCELLED;
						DEBUGP(DEBUG_ALWAYS, "XBlockWriteRequestComplete detected a cancelled DMA");
					}
					else
					{
						Status = STATUS_ADAPTER_HARDWARE_ERROR;
						DEBUGP(DEBUG_ALWAYS, "XBlockWriteRequestComplete Hardware Error detected");
					}
				}
				else
				{
					// complete the transaction,  transaction completed successfully
					// tell the framework that this DMA set is complete
					TransactionComplete = WdfDmaTransactionDmaCompletedWithLength(Transaction,
													pDMATrans->BytesTransferred,
													&Status );
				}

				// Is the full transaction complete?
				if (TransactionComplete)
				{
					DEBUGP(DEBUG_TRACE, "XBlockWriteRequestComplete:  Request %p, Status 0x%x, bytes transferred %d",
								Request, Status, (int) bytesTransferred);

					// Lock and remove this DMA Transaction from our managed queue
					WdfSpinLockAcquire(pDevExt->S2CDMATransListLock);
					RemoveEntryList(&pDMATrans->DT_LINK);
					WdfSpinLockRelease(pDevExt->S2CDMATransListLock);

					// Make sure we do not try and re-issue this transaction
					pDMATrans->TRANSACTION_PTR = NULL;
					pDMATrans->SG_LIST_PTR = NULL;

					// Release the DMA Transaction
					WdfDmaTransactionRelease(Transaction);        
					WdfObjectDelete(Transaction);
					if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED ) 
					{
						// Complete the request with status and bytes transferred.
						WdfRequestCompleteWithInformation(Request, Status, bytesTransferred);
					}
					else
					{
						DEBUGP(DEBUG_ALWAYS, "XBlockWriteRequestComplete:  Request was cancelled");
					}
				}
				else
				{
					DEBUGP(DEBUG_ALWAYS, "XBlockWriteRequestComplete:  ERROR- Transaction not complete");
				}
			}
			else
			{
				DEBUGP(DEBUG_ERROR, "XBlockWriteRequestComplete:  Request == NULL");
			}
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "XBlockWriteRequestComplete:  pDMATrans->TRANSACTION_PTR == NULL");
		}
	}
	else
	{
		DEBUGP(DEBUG_ERROR, "XBlockWriteRequestComplete:  pDMATrans == NULL");
	}

}


//-----------------------------------------------------------------------------
//  READ Functions
//-----------------------------------------------------------------------------

/*++

Routine Description:

    Called by the framework as soon as it receives a read request.
    If the device is not ready, fail the request.
    Otherwise get scatter-gather list for this request and send the
    packet to the hardware for DMA.

Arguments:

    Queue      - Default queue handle
    Request    - Handle to the write request
    Length - Length of the data buffer associated with the request.
                     The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:

--*/
VOID
XBlockEvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
{
    PDEVICE_EXTENSION       pDevExt = XBlockGetDeviceContext(WdfIoQueueGetDevice(Queue));
    WDFDMATRANSACTION       dmaTransaction = NULL;
    WDF_OBJECT_ATTRIBUTES   attributes;
#ifdef ADDRESSABLE_SUPPORT
    WDF_REQUEST_PARAMETERS  Parameters;
#endif // ADDRESSABLE_SUPPORT
    PDATA_TRANSFER_PARAMS   pDMATrans = NULL;
    BOOLEAN                 bCreated = FALSE;
    NTSTATUS                status = STATUS_UNSUCCESSFUL;

    DEBUGP(DEBUG_TRACE, "--> XBlockEvtIoRead: Request %p", Request);

    do {
        if (pDevExt->ReadDMAStatus != AVAILABLE)
        {
			DEBUGP(DEBUG_WARN, "Warning: ReadDMAStatus != AVAILABLE");
            status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
        // Validate the Length parameter.
        if (Length > pDevExt->ReadDMALink.MAXIMUM_TRANSFER_LENGTH)  
        {
			DEBUGP(DEBUG_WARN, "Read: Warning: Length > pDevExt->ReadDMALink.MAXIMUM_TRANSFER_LENGTH");
            status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

		// Grab the lock while we allocate and add the DMA transaction to our list.
		//   This protects us during the linking just in case a cancel request comes through.
		WdfSpinLockAcquire(pDevExt->C2SDMATransListLock);
		status = WdfRequestMarkCancelableEx(Request, XBlockReadRequestCancel);
		if (!NT_SUCCESS(status)) 
		{
			DEBUGP(DEBUG_ERROR, "Read: WdfRequestMarkCancelableEx failed: 0x%x", status);
			WdfSpinLockRelease(pDevExt->C2SDMATransListLock);
            break;
		}

        // Create the DMA transaction object on the fly.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DATA_TRANSFER_PARAMS);
        status = WdfDmaTransactionCreate(pDevExt->ReadDMALink.DMAEnabler,
                                         &attributes,
                                         &dmaTransaction);
        if (!NT_SUCCESS(status)) 
		{
            DEBUGP(DEBUG_ERROR, "Read: WdfDmaTransactionCreate failed %X", status);
			WdfSpinLockRelease(pDevExt->C2SDMATransListLock);
            break;
        }

        bCreated = TRUE;
        pDMATrans = DMATransCtx(dmaTransaction);
        // Save a back pointer to the Transaction object, we''ll need it
        // in the completion callback
        pDMATrans->TRANSACTION_PTR = dmaTransaction;
		// Now put it on our managed list
		InsertTailList(&pDevExt->C2SDMATransList,  &pDMATrans->DT_LINK);
		// Free the List Lock
		WdfSpinLockRelease(pDevExt->C2SDMATransListLock);

#ifdef ADDRESSABLE_SUPPORT
        // Extract the Write offset and save it as the card address offset for the device
        WDF_REQUEST_PARAMETERS_INIT(&Parameters);
        WdfRequestGetParameters(Request, &Parameters);
        pDMATrans->CardAddress = Parameters.Parameters.Read.DeviceOffset;
#else
		pDMATrans->CardAddress = 0;
#endif // ADDRESSABLE_SUPPORT

        // Initialize this new DmaTransaction with the Read Request Info.
        status = WdfDmaTransactionInitializeUsingRequest(dmaTransaction,
                                                         Request,
                                                         XBlockEvtProgramReadDma,
                                                         WdfDmaDirectionReadFromDevice);
        if (!NT_SUCCESS(status)) 
        {
            DEBUGP(DEBUG_ERROR, "Read: WdfDmaTransactionInitializeUsingRequest failed: 0x%x", status);
            break;
        }

        // Execute this DmaTransaction.
        status = WdfDmaTransactionExecute(dmaTransaction, pDevExt);
        if (!NT_SUCCESS(status)) 
        {
			// Couldn't execute this DmaTransaction, so fail Request.
            DEBUGP(DEBUG_ERROR, "Read: WdfDmaTransactionExecute failed: 0x%x", status);
            break;
        }

        // Indicate that Dma transaction has been started successfully.
        // The request will be complete by the callback routine when the DMA
        // transaction completes.
    } while (0);

    // If there are errors, then clean up and complete the Request.
    if (!NT_SUCCESS(status))
    {
        if (bCreated) 
        {
			// Lock and remove this DMA Transaction from our managed queue
			WdfSpinLockAcquire(pDevExt->C2SDMATransListLock);
			if (RemoveEntryList(&pDMATrans->DT_LINK))
			{
				DEBUGP(DEBUG_ERROR, "XBlockEvtIoRead:  pDMATrans was not on the list");
			}
			WdfSpinLockRelease(pDevExt->C2SDMATransListLock);

            WdfObjectDelete(dmaTransaction);
        }
        WdfRequestCompleteWithInformation(Request, status, 0);
    }
    DEBUGP(DEBUG_TRACE, "<-- XBlockEvtIoRead: status 0x%x", status);
    return;
}


/*++

Routine Description:

     Prepares the Transfer parameters structure and call down
    to the XDMA driver to do the actual DMA Setup and start

Arguments:

Return Value:

--*/
BOOLEAN
XBlockEvtProgramReadDma(
    IN  WDFDMATRANSACTION       Transaction,
    IN  WDFDEVICE               Device,
    IN  WDFCONTEXT              Context,
    IN  WDF_DMA_DIRECTION       Direction,
    IN  PSCATTER_GATHER_LIST    SgList
    )
{
    PDEVICE_EXTENSION       pDevExt = (PDEVICE_EXTENSION)Context;
    PDATA_TRANSFER_PARAMS   pDMATrans;
    NTSTATUS				status = STATUS_SUCCESS;
    BOOLEAN                 success = TRUE;

	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(Direction);

	DEBUGP(DEBUG_TRACE, "--> XBlockEvtProgramReadDma");

    // Setup the DMA Transaction Context pointer.
    pDMATrans = DMATransCtx(Transaction);
    // Fill out the rest of the transfer structure for the call down to XDMA
    pDMATrans->SG_LIST_PTR      = SgList;

#ifdef ADDRESSABLE_SUPPORT
    // Get the number of bytes as the offset to the beginning of this
    // Dma operations transfer location in the buffer.
    pDMATrans->CardAddress += WdfDmaTransactionGetBytesTransferred(Transaction);
#endif // ADDRESSABLE_SUPPORT

    // We pass the offset to start, the SG List, callback info, etc.
	status = pDevExt->ReadDMALink.XDMADataTransfer(pDevExt->DMAReadHandle, pDMATrans);

    if (!NT_SUCCESS(status)) 
    {
        // The tranaction failed, the DmaTransaction must be deleted and
        //       the Request must be completed.
        WDFREQUEST      Request;

		// Must abort the transaction before deleting it.
        (VOID) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &status);
        // Get the associated request from the transaction.
        Request = WdfDmaTransactionGetRequest(Transaction);

		// Lock and remove this DMA Transaction from our managed queue
		WdfSpinLockAcquire(pDevExt->C2SDMATransListLock);
		if (RemoveEntryList(&pDMATrans->DT_LINK))
		{
			DEBUGP(DEBUG_ERROR, "XBlockEvtProgramReadDma:  pDMATrans was not on the list");
		}
		WdfSpinLockRelease(pDevExt->C2SDMATransListLock);

        // Release the DMA Transaction
        WdfDmaTransactionRelease(Transaction);        
        WdfObjectDelete(Transaction);
		if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED) 
		{
			// Complete the request with status and bytes transferred.
			WdfRequestCompleteWithInformation( Request, STATUS_INVALID_DEVICE_STATE, 0);
		}
		success = FALSE;
        DEBUGP(DEBUG_ERROR, "<-- XBlockEvtProgramReadDma: error ****");
    }

    DEBUGP(DEBUG_TRACE, "<-- XBlockEvtProgramReadDma");
    return success;
}


/*++

Routine Description:
 
    A callback to completes a DMA.  Releases the
    transaction and completes the Request
 
Arguments:
 
    FSD_HANDLE  - Handle which is our context pointer
    PDATA_TRANSFER_PARAMS - The parameters of the DMA transfer
 
Return Value:
 
    None 
--*/
VOID
XBlockReadRequestComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans
    )
{
	PDEVICE_EXTENSION       pDevExt = (PDEVICE_EXTENSION)FSDHandle;
    WDFREQUEST              Request;
    WDFDMATRANSACTION       Transaction;
    size_t                  bytesTransferred;
	BOOLEAN 				TransactionComplete;
    NTSTATUS                Status = STATUS_SUCCESS;

	if (pDMATrans != NULL)
	{
		if (pDMATrans->TRANSACTION_PTR != NULL)
		{
			// Retrieve the DMA Transaction from the transfer strucutre
			Transaction = pDMATrans->TRANSACTION_PTR;
			// Get the associated request from the transaction.
			Request = WdfDmaTransactionGetRequest(Transaction);
			if (Request != NULL)
			{
				// Get the final bytes transferred count.
				bytesTransferred =  pDMATrans->BytesTransferred;
				// If any errors are reported, make it a hardware error to Windows.
				if (pDMATrans->Status)
				{
					TransactionComplete = WdfDmaTransactionDmaCompletedFinal(Transaction,
														pDMATrans->BytesTransferred,
														&Status);
					if (pDMATrans->Status == DMA_TRANS_STATUS_SHORT)
					{
						DEBUGP(DEBUG_ALWAYS, "XBlockReadRequestComplete detected short DMA");
					}
					else if (pDMATrans->Status == (ULONG)STATUS_CANCELLED)
					{
						Status = STATUS_CANCELLED;
						DEBUGP(DEBUG_ALWAYS, "XBlockReadRequestComplete detected a cancelled DMA");
					}
					else
					{
						Status = STATUS_ADAPTER_HARDWARE_ERROR;
						DEBUGP(DEBUG_ALWAYS, "XBlockReadRequestComplete Hardware Error detected");
					}
				}
				else
				{
					// complete the transaction,  transaction completed successfully
					// tell the framework that this DMA set is complete
					TransactionComplete = WdfDmaTransactionDmaCompletedWithLength(Transaction,
										pDMATrans->BytesTransferred,
										&Status );
				}

				// Is the full transaction complete?
				if (TransactionComplete)
				{
					DEBUGP(DEBUG_INFO, "XBlockReadRequestComplete:  Request %p, Status 0x%x, bytes transferred %d",
						        Request, Status, (int) bytesTransferred);

					// Lock and remove this DMA Transaction from our managed queue
					WdfSpinLockAcquire(pDevExt->C2SDMATransListLock);
					RemoveEntryList(&pDMATrans->DT_LINK);
					WdfSpinLockRelease(pDevExt->C2SDMATransListLock);

					// Make sure we do not try and re-issue this transaction
					pDMATrans->TRANSACTION_PTR = NULL;
					pDMATrans->SG_LIST_PTR = NULL;

					// Release the DMA Transaction
					WdfDmaTransactionRelease(Transaction);        
					WdfObjectDelete(Transaction);
					if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED ) 
					{
						// Complete the request with status and bytes transferred.
						WdfRequestCompleteWithInformation( Request, Status, bytesTransferred);
					}
					else
					{
						DEBUGP(DEBUG_ALWAYS, "XBlockReadRequestComplete:  Request was already cancelled.");
					}
				}
				else
				{
					DEBUGP(DEBUG_ALWAYS, "XBlockReadRequestComplete:  ERROR- Transaction not complete");
				}
			}
			else
			{
				DEBUGP(DEBUG_ERROR, "XBlockReadRequestComplete:  Request == NULL");
			}
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "XBlockReadRequestComplete:  pDMATrans->TRANSACTION_PTR == NULL");
		}
	}
	else
	{
		DEBUGP(DEBUG_ERROR, "XBlockReadRequestComplete:  pDMATrans == NULL");
	}
}


/*++

Routine Description:
 
    Cancels a waiting request.
 
Arguments:
 
    Request - The request to cancel
 
Return Value:
 
    None 
--*/
VOID 
XBlockWriteRequestCancel(
    IN WDFREQUEST	cancelRequest
	)
{
	WDFDEVICE 				device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(cancelRequest));
	PDEVICE_EXTENSION		pDevExt = XBlockGetDeviceContext(device);
	WDFREQUEST				Request;
    WDFDMATRANSACTION       Transaction;
	PDATA_TRANSFER_PARAMS	pDMATrans;
    PLIST_ENTRY				pEntry;
    NTSTATUS                Status = STATUS_CANCELLED;
	
	DEBUGP(DEBUG_ALWAYS, "XBlock cancelling all write requests");

	pDevExt->WriteDMAStatus = INITIALIZING;

	// The following resets the DMA Engine.
	pDevExt->WriteDMALink.XDMACancelTransfer(pDevExt->DMAWriteHandle);

	WdfSpinLockAcquire(pDevExt->S2CDMATransListLock);

	while (!(IsListEmpty(&pDevExt->S2CDMATransList)))
    {
		pEntry = RemoveHeadList(&pDevExt->S2CDMATransList);
		pDMATrans = CONTAINING_RECORD(pEntry, DATA_TRANSFER_PARAMS, DT_LINK);
		if (pDMATrans != NULL)
		{
			if (pDMATrans->TRANSACTION_PTR != NULL)
			{
				// Retrieve the DMA Transaction from the transfer strucutre
				Transaction = pDMATrans->TRANSACTION_PTR;
				// Get the associated request from the transaction.
				Request = WdfDmaTransactionGetRequest(Transaction);
				if (Request != NULL)
				{
					WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &Status);
					Status = STATUS_CANCELLED;

					// Make sure we do not try and re-issue this transaction
					pDMATrans->TRANSACTION_PTR = NULL;
					pDMATrans->SG_LIST_PTR = NULL;

					// Release the DMA Transaction
					WdfDmaTransactionRelease(Transaction);        
					WdfObjectDelete(Transaction);

					if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED ) 
					{
						if (Request != cancelRequest)
						{
							// Complete the request with status and bytes transferred.
							WdfRequestCompleteWithInformation( Request, Status, 0);
						}
						else
						{
							DEBUGP(DEBUG_ERROR, "XBlockWriteRequestCancel:  Request == cancelRequest");
						}
					}
				}
				else
				{
					DEBUGP(DEBUG_ERROR, "XBlockWriteRequestCancel:  Request == NULL");
				}
			}
			else
			{
				DEBUGP(DEBUG_ERROR, "XBlockWriteRequestCancel:  pDMATrans->TRANSACTION_PTR == NULL");
			}
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "XBlockWriteRequestCancel:  pDMATrans == NULL");
		}
	}
	WdfSpinLockRelease(pDevExt->S2CDMATransListLock);

	DEBUGP(DEBUG_ALWAYS, "XBlockWrite cancelling request 0x%p", cancelRequest);
	// Complete the request with status and bytes transferred.
	WdfRequestCompleteWithInformation(cancelRequest, STATUS_CANCELLED, 0);

	pDevExt->WriteDMAStatus = AVAILABLE;
	return;
}

/*++

Routine Description:
 
    Cancels a waiting request.
 
Arguments:
 
    Request - The request to cancel
 
Return Value:
 
    None 
--*/
VOID 
XBlockReadRequestCancel(
    IN WDFREQUEST	cancelRequest
	)
{
	WDFDEVICE 				device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(cancelRequest));
	PDEVICE_EXTENSION		pDevExt = XBlockGetDeviceContext(device);
	WDFREQUEST				Request;
    WDFDMATRANSACTION       Transaction;
	PDATA_TRANSFER_PARAMS	pDMATrans;
    PLIST_ENTRY				pEntry;
    NTSTATUS                Status = STATUS_CANCELLED;

	DEBUGP(DEBUG_ALWAYS, "XBlock cancelling all read requests");

	pDevExt->ReadDMAStatus = INITIALIZING;

	// The following resets the DMA Engine.
	pDevExt->ReadDMALink.XDMACancelTransfer(pDevExt->DMAReadHandle);

	WdfSpinLockAcquire(pDevExt->C2SDMATransListLock);

	while (!(IsListEmpty(&pDevExt->C2SDMATransList)))
    {
		pEntry = RemoveHeadList(&pDevExt->C2SDMATransList);
		pDMATrans = CONTAINING_RECORD(pEntry, DATA_TRANSFER_PARAMS, DT_LINK);
		if (pDMATrans != NULL)
		{
			if (pDMATrans->TRANSACTION_PTR != NULL)
			{
				// Retrieve the DMA Transaction from the transfer strucutre
				Transaction = pDMATrans->TRANSACTION_PTR;
				// Get the associated request from the transaction.
				Request = WdfDmaTransactionGetRequest(Transaction);
				if (Request != NULL)
				{
					WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &Status);
					Status = STATUS_CANCELLED;

					// Make sure we do not try and re-issue this transaction
					pDMATrans->TRANSACTION_PTR = NULL;
					pDMATrans->SG_LIST_PTR = NULL;

					// Release the DMA Transaction
					WdfDmaTransactionRelease(Transaction);        
					WdfObjectDelete(Transaction);

					if (WdfRequestUnmarkCancelable(Request) != STATUS_CANCELLED ) 
					{
						if (Request != cancelRequest)
						{
							// Complete the request with status and bytes transferred.
							WdfRequestCompleteWithInformation( Request, Status, 0);
						}
					}
				}
				else
				{
					DEBUGP(DEBUG_ERROR, "XBlockReadRequestCancel:  Request == NULL");
				}
			}
			else
			{
				DEBUGP(DEBUG_ERROR, "XBlockReadRequestCancel:  pDMATrans->TRANSACTION_PTR == NULL");
			}
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "XBlockReadRequestCancel:  pDMATrans == NULL");
		}
	}
	WdfSpinLockRelease(pDevExt->C2SDMATransListLock);

	DEBUGP(DEBUG_ALWAYS, "XBlockRead cancelling request 0x%p", cancelRequest);
	// Complete the request with status and bytes transferred.
	WdfRequestCompleteWithInformation(cancelRequest, STATUS_CANCELLED, 0);

	pDevExt->ReadDMAStatus = AVAILABLE;
	return;
}




