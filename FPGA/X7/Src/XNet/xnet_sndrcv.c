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
* @file XNet_SndRcv.c
*
* This file contains functions for sending and recieving network data
*
* MODIFICATION HISTORY:
*
* Ver     Date   Changes
* ----- -------- -------------------------------------------------------
* 1.0   11/18/12 First release 
*
******************************************************************************/
/***************************** Include Files *********************************/

#include "Precomp.h"

#if TRACE_ENABLED
#include "XNet_SndRcv.tmh"
#endif // TRACE_ENABLED

VOID SendSgListComplete(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PSCATTER_GATHER_LIST pScatterGatherList,
    IN PVOID                Context);

BOOLEAN
IsFrameAcceptedByPacketFilter(
    __in  PMP_ADAPTER  Adapter,
    __in_bcount(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    __in  ULONG        FrameType);

NDIS_STATUS
SetTxStatsFromNetBuffer(
	PMP_ADAPTER			Adapter,
    PNET_BUFFER			NetBuffer,
	ULONG				bytesTransferred);


//**************************************************************************************************
//**************************************************************************************************
//   SEND FUNTIONS
//**************************************************************************************************
//**************************************************************************************************

/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our miniport sends one or more packets.

    The input packet descriptor pointers have been ordered according
    to the order in which the packets should be sent over the network
    by the protocol driver that set up the packet array. The NDIS
    library preserves the protocol-determined ordering when it submits
    each packet array to MiniportSendPackets

    As a deserialized driver, we are responsible for holding incoming send
    packets in our internal queue until they can be transmitted over the
    network and for preserving the protocol-determined ordering of packet
    descriptors incoming to its MiniportSendPackets function.
    A deserialized miniport driver must complete each incoming send packet
    with NdisMSendComplete, and it cannot call NdisMSendResourcesAvailable.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              Head of a list of NBLs to send
    PortNumber                  A miniport adapter port.  Default is 0.
    SendFlags                   Additional flags for the send operation

Return Value:

    None.  Write status directly into each NBL with the NET_BUFFER_LIST_STATUS
    macro.

--*/
VOID
MPSendNetBufferLists(
    __in  NDIS_HANDLE             MiniportAdapterContext,
    __in  PNET_BUFFER_LIST        NetBufferLists,
    __in  NDIS_PORT_NUMBER        PortNumber,
    __in  ULONG                   SendFlags)
{
    PMP_ADAPTER				Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    PNET_BUFFER_LIST		Nbl;
    PNET_BUFFER_LIST		NextNbl = NULL;
	PNET_BUFFER				NetBuffer;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDMATRANSACTION       dmaTransaction;
    PDATA_TRANSFER_PARAMS   pDMATrans;
	PFS_NETWORK_SEND		pFSSend;
	PMDL					CurrentMdl;
	ULONG					MdlOffset;
	PUCHAR					VirtualAddress;
	ULONG					Length;
    BOOLEAN					fAtDispatch = (SendFlags & NDIS_SEND_FLAGS_DISPATCH_LEVEL) ? TRUE:FALSE;
    NDIS_STATUS				Status;
    ULONG					NumNbls = 0;
	ULONG					PacketLength;
	LONG					i;

    //DEBUGP(DEBUG_TRACE, "---> MPSendNetBufferLists");

    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(SendFlags);
    ASSERT(PortNumber == 0); // Only the default port is supported

    // Each NET_BUFFER_LIST has a list of NET_BUFFERs.
    // Loop over all the NET_BUFFER_LISTs, sending each NET_BUFFER.
    for (Nbl = NetBufferLists; Nbl != NULL; Nbl = NextNbl, ++NumNbls)
    {
		dmaTransaction = NULL;
		pDMATrans = NULL;
        NextNbl = NET_BUFFER_LIST_NEXT_NBL(Nbl);

        // We use a reference count to make sure that we don't send complete
        // the NBL until we're done reading each NB on the NBL.
        NET_BUFFER_LIST_NEXT_NBL(Nbl) = NULL;
        SEND_REF_FROM_NBL(Nbl) = 0;

        Status = TXNblReference(Adapter, Nbl);
        if (Status == NDIS_STATUS_SUCCESS)
        {
			// Assume we will be successful sending the packet
            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_SUCCESS;

			// Create the DMA transaction object on the fly.
			WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DATA_TRANSFER_PARAMS);
			Status = WdfObjectCreate(&attributes, &dmaTransaction);
		    if (!NT_SUCCESS(Status)) 
			{
				DEBUGP(DEBUG_ERROR, "MPSendNetBufferLists: WdfObjectCreate failed %X", Status);
	            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESOURCES;
	            TXNblRelease(Adapter, Nbl, fAtDispatch);
		        continue;
	        }
		    pDMATrans = DMATransCtx(dmaTransaction);
			// Save a back pointer to the Transaction object,
	        //  we'll use it on the completion callback
		    pDMATrans->TRANSACTION_PTR = dmaTransaction;
			pDMATrans->CardAddress = 0;

			pDMATrans->FS_PTR = NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
				(UINT)sizeof(FS_NETWORK_SEND), NIC_TAG_SG, NormalPoolPriority);
			if (pDMATrans->FS_PTR == NULL)
			{
				DEBUGP(DEBUG_ERROR, "NdisAllocateMemoryWithTagPriority (FS_Send) failed");
	            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESOURCES;
			    WdfObjectDelete(dmaTransaction);
	            TXNblRelease(Adapter, Nbl, fAtDispatch);
				continue;
			}
			NdisZeroMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_SEND));
			pFSSend = (PFS_NETWORK_SEND)pDMATrans->FS_PTR;
			pFSSend->NET_BUFFER_LIST_PTR = Nbl;
			pFSSend->SystemSGArrayIndex = 0;

			pDMATrans->SG_LIST_PTR = (PSCATTER_GATHER_LIST) NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
				(UINT)sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_TX_SG_ELEMENTS, 
				NIC_TAG_SG, NormalPoolPriority);
			if (pDMATrans->SG_LIST_PTR == NULL)
			{
				DEBUGP(DEBUG_ERROR, "NdisAllocateMemoryWithTagPriority (SG List) failed");
	            NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESOURCES;
				NdisFreeMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_SEND), 0);
			    WdfObjectDelete(dmaTransaction);
	            TXNblRelease(Adapter, Nbl, fAtDispatch);
				continue;
			}
			NdisZeroMemory(pDMATrans->SG_LIST_PTR, (UINT)sizeof(SCATTER_GATHER_LIST) + 
				(sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_TX_SG_ELEMENTS));
			pDMATrans->SG_LIST_PTR->NumberOfElements = 0;

			PacketLength = 0;
            //
            // Queue each NB for transmission.
            //
            for (NetBuffer = NET_BUFFER_LIST_FIRST_NB(Nbl);
                NetBuffer != NULL;
                NetBuffer = NET_BUFFER_NEXT_NB(NetBuffer))
            {
                NBL_FROM_SEND_NB(NetBuffer) = Nbl;

			    //
				// Start from current MDL
				//
				CurrentMdl = NET_BUFFER_CURRENT_MDL(NetBuffer);
				MdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
				while (CurrentMdl)
				{
					VirtualAddress = MmGetMdlVirtualAddress(CurrentMdl);
					Length = MmGetMdlByteCount(CurrentMdl);
					if (!VirtualAddress)
					{
						Status = NDIS_STATUS_RESOURCES;
						DEBUGP(DEBUG_ERROR, "ERROR: MmGetMdlVirtualAddress returned a null address");
						break;
					}

					// The first MDL segment should be accessed from the current MDL offset
					VirtualAddress += MdlOffset;
					Length -= MdlOffset;
					MdlOffset = 0;

					if (pDMATrans->SG_LIST_PTR->NumberOfElements >= (MAX_NUMBER_TX_SG_ELEMENTS -1))
					{
						Status = NDIS_STATUS_INVALID_LENGTH;
						DEBUGP(DEBUG_WARN, "Warning: No more S/G elements available");
						break;
					}
					if (pFSSend->SystemSGArrayIndex >= (SG_ARRAY_SIZE -1))
					{
						Status = NDIS_STATUS_INVALID_LENGTH;
						DEBUGP(DEBUG_WARN, "Warning: No more S/G Array elements available");
						break;
					}

		 			Status = Adapter->pSendDmaAdapter->DmaOperations->GetScatterGatherList(
								Adapter->pSendDmaAdapter,	
								Adapter->Fdo,
								CurrentMdl,
								VirtualAddress, 
								Length,
								SendSgListComplete,
								pDMATrans,			// Context
								TRUE);				// This is a write

					// Get next MDL (if any available) 
					CurrentMdl = NDIS_MDL_LINKAGE(CurrentMdl);
					PacketLength += Length;
					if (PacketLength > Adapter->FrameSize)
					{
						DEBUGP(DEBUG_WARN, "Warning: Send Packet too large (%d bytes), Max is %d", 
						PacketLength, HW_MAX_FRAME_SIZE);
						Status = NDIS_STATUS_INVALID_LENGTH;
						break;
					}
				} // while (CurrentMDL)
				if (!NT_SUCCESS(Status)) 
				{
					break;
				}
			} // for...
			
			// Make sure we were successful in creating the scatter gather list.
			//    If not, bypass the send of this packet.
			if (NT_SUCCESS(Status)) 
			{
				Status = NDIS_STATUS_RESOURCES;
				if (pDMATrans->SG_LIST_PTR->NumberOfElements  > 0)
				{
					DEBUGP(DEBUG_INFO, "Sending %d bytes, PacketLength");
					//DEBUGP(DEBUG_VERBOSE, "Sending %d bytes, NBL 0x%p, DMA Trans 0x%p", PacketLength, Nbl, pDMATrans);
				    NdisAcquireSpinLock(&Adapter->SendSpinLock);
					// We pass the offset to start, the SG List, callback info, etc.
					Status = Adapter->SendDMALink.XDMADataTransfer(Adapter->DMASendHandle, pDMATrans);
					if (!NT_SUCCESS(Status)) 
					{
						DEBUGP(DEBUG_ERROR, "<-- XNet call to XDMA failed: error 0x%x", Status);
						Status = NDIS_STATUS_ADAPTER_NOT_READY;
				    }
					NdisReleaseSpinLock(&Adapter->SendSpinLock);
				}
			}

			// This could be a fall through from the scatter/gather process or a failed
			//  return from the XDMA driver.
			if (!NT_SUCCESS(Status)) 
			{
				// The transaction failed, the DmaTransaction must be deleted.
				if (pDMATrans->SG_LIST_PTR != NULL)
				{
					// Free our DMA Link Scatter/Gather list
					NdisFreeMemory(pDMATrans->SG_LIST_PTR, 
						(UINT)sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_TX_SG_ELEMENTS, 0);
					pDMATrans->SG_LIST_PTR = NULL;
				}
				if (pDMATrans->FS_PTR != NULL)
				{
					pFSSend = (PFS_NETWORK_SEND)pDMATrans->FS_PTR;
					for (i = 0; i < pFSSend->SystemSGArrayIndex; i++)
					{
						// Tell the OS we are done with this system allocated Scatter/Gather list.
						Adapter->pSendDmaAdapter->DmaOperations->PutScatterGatherList(Adapter->pSendDmaAdapter,	
							pFSSend->SystemSGArray[i], TRUE);				// This was a write
					}
					// Finally release the FS Send structure
					NdisFreeMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_SEND), 0);
				}
				// Release the DMA Transaction
		        WdfObjectDelete(dmaTransaction);
	            NET_BUFFER_LIST_STATUS(Nbl) = Status;
	            TXNblRelease(Adapter, Nbl, fAtDispatch);
				//DEBUGP(DEBUG_ERROR, "<-- XNet call to XDMA or SG failed: error 0x%x", Status);
			}
        }
        else  // (Status == NDIS_STATUS_SUCCESS)
        {
			DEBUGP(DEBUG_WARN, "MPSendNetBufferLists unable to send, status 0x%x", Status);
            // We can't send this NBL now.  Indicate failure.
            if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_RESET_IN_PROGRESS;
            }
            else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS|fMP_ADAPTER_PAUSED))
            {
                NET_BUFFER_LIST_STATUS(Nbl) = NDIS_STATUS_PAUSED;
            }
            else
            {
                NET_BUFFER_LIST_STATUS(Nbl) = Status;
            }
            TXNblRelease(Adapter, Nbl, fAtDispatch);
            continue;
        }
    }  // for (Nbl = NetBufferLists; Nbl != NULL; Nbl = NextNbl, ++NumNbls)

    DEBUGP(DEBUG_TRACE, "<--- MPSendNetBufferLists, %i NBLs processed.", NumNbls);
}


/*!***************************************************************************
*
* 	\brief SendSgListComplete - This routine converts a scatter
* 		gather entries into DMA Descriptors
* 
* 	\param pDeviceObject - Caller-supplied pointer to a DEVICE_OBJECT structure. 
*			This is the device object for the target device, previously created 
*			by the driver's AddDevice routine.
*   \param pIrp - Not used - Caller-supplied pointer to an IRP structure that 
*			describes the I/O operation, 
*			NOTE: Do not use unless CurrentIrp is set in the DeviceObject.
*	\param pScatterGatherList - Caller-supplied pointer to a 
*			SCATTER_GATHER_LIST structure describing scatter/gather regions.
* 	\param Context - Caller Supplied Context field
* 
*   \return nothing
* 
*****************************************************************************/
VOID SendSgListComplete(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PSCATTER_GATHER_LIST pScatterGatherList,
    IN PVOID                Context
    )
{
	PDATA_TRANSFER_PARAMS   pDMATrans = (PDATA_TRANSFER_PARAMS)Context;
	PSCATTER_GATHER_LIST    pTransSG = pDMATrans->SG_LIST_PTR;
	PFS_NETWORK_SEND		pFSSend = (PFS_NETWORK_SEND)pDMATrans->FS_PTR;
	ULONG 					i;

    UNREFERENCED_PARAMETER(pDeviceObject);
    UNREFERENCED_PARAMETER(pIrp);

//    DEBUGP(DEBUG_TRACE, "---> SendSgListComplete");

	// Validate the SG List and fill out our own SG list for XDMA to use.
	if (pScatterGatherList != NULL)
    {
		if ((pScatterGatherList->NumberOfElements > 0) &&
			(pScatterGatherList->NumberOfElements < MAX_NUMBER_TX_SG_ELEMENTS))
		{
			if (pFSSend->SystemSGArrayIndex < SG_ARRAY_SIZE)
			{
				// Save a pointer to the system allocated SG List
				pFSSend->SystemSGArray[pFSSend->SystemSGArrayIndex++] = pScatterGatherList;

				// Now fill out our DMA Link SG List.
				for (i = 0; i < pScatterGatherList->NumberOfElements; i++)
				{
					pTransSG->Elements[pTransSG->NumberOfElements].Length = pScatterGatherList->Elements[i].Length;
					pTransSG->Elements[pTransSG->NumberOfElements].Address.QuadPart = pScatterGatherList->Elements[i].Address.QuadPart;
					pTransSG->NumberOfElements++;
					if (pTransSG->NumberOfElements >= MAX_NUMBER_TX_SG_ELEMENTS)
					{
						DEBUGP(DEBUG_ERROR, "ERROR:SendSgListComplete hit maximum number of SGs");
						break;
					}
				}
			}
			else
			{
				// We need to do something but there is no return values for this routine
				// We cannot do a PutScatterGatherList here.
			}
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "ERROR:SendSgListComplete unable to use SG Entry, list has SGs %d, need %d more",
				pTransSG->NumberOfElements, pScatterGatherList->NumberOfElements);
		}
	}
    //DEBUGP(DEBUG_TRACE, "<--- SendSgListComplete");
	return;
}




/*++

Routine Description:
 
    A callback to completes a DMA.  Releases the
    transaction and completes the packet
 
Arguments:
 
    FSD_HANDLE  - Handle which is our context pointer
    PDATA_TRANSFER_PARAMS - The parameters of the DMA transfer

Return Value:
 
    None
 
--*/
VOID
XNetSendComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans
    )
{
    PMP_ADAPTER				Adapter = (PMP_ADAPTER)FSDHandle;
    WDFDMATRANSACTION       Transaction;
	PFS_NETWORK_SEND		pFSSend;
	PNET_BUFFER				NetBuffer;
    PNET_BUFFER_LIST		Nbl;
	LONG					i;

    DEBUGP(DEBUG_TRACE, "---> XNetSendComplete");

    // Retrieve the DMA Transaction from the transfer strucutre
	Transaction = pDMATrans->TRANSACTION_PTR;
	// Make sure we do not try and re-issue this transaction
	pDMATrans->TRANSACTION_PTR = NULL;

//	DEBUGP(DEBUG_INFO, "XNetSendComplete: Sent %d bytes", pDMATrans->BytesTransferred);

	if (pDMATrans->SG_LIST_PTR != NULL)
	{
		// Free our DMA Link Scatter/Gather list
		NdisFreeMemory(pDMATrans->SG_LIST_PTR, 
			(UINT)sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_TX_SG_ELEMENTS, 0);
		pDMATrans->SG_LIST_PTR = NULL;
	}
	else
	{
		DEBUGP(DEBUG_ERROR, "ERROR: No DMA Link SG Entry for completed send.");
	}

	if (pDMATrans->FS_PTR != NULL)
	{
		pFSSend = (PFS_NETWORK_SEND)pDMATrans->FS_PTR;

		DEBUGP(DEBUG_INFO, "Sent %d bytes", pDMATrans->BytesTransferred);

		if (pDMATrans->BytesTransferred == 0)
	    {
			//
			// Failed to send the frame.
			//
			Adapter->TransmitFailuresOther++;
			NET_BUFFER_LIST_STATUS((PNET_BUFFER_LIST)pFSSend->NET_BUFFER_LIST_PTR) = NDIS_STATUS_RESOURCES;
			DEBUGP(DEBUG_ERROR, "ERROR: Failed to send any data (pDMATrans->BytesTransferred == 0)");
		}
		else
		{
			//
		    // We've finished sending this NB successfully; update the stats.
			//
			Nbl = pFSSend->NET_BUFFER_LIST_PTR;
			NetBuffer = NET_BUFFER_LIST_FIRST_NB(Nbl);
			SetTxStatsFromNetBuffer(Adapter, NetBuffer, pDMATrans->BytesTransferred);
		}

		for (i = 0; i < pFSSend->SystemSGArrayIndex; i++)
		{
			// Tell the OS we are done with this system allocated Scatter/Gather list.
			Adapter->pSendDmaAdapter->DmaOperations->PutScatterGatherList(Adapter->pSendDmaAdapter,	
				pFSSend->SystemSGArray[i], TRUE);				// This was a write
		}
	
		if (pFSSend->NET_BUFFER_LIST_PTR)
		{
//			DEBUGP(DEBUG_INFO, "Send %d bytes of NBL 0x%p, DMA Trans 0x%p", pDMATrans->BytesTransferred, pDMATrans->NET_BUFFER_LIST_PTR, pDMATrans);
			TXNblRelease(Adapter, pFSSend->NET_BUFFER_LIST_PTR, TRUE);
			pFSSend->NET_BUFFER_LIST_PTR = NULL;
		}
		else
		{
			DEBUGP(DEBUG_ERROR, "ERROR: No NBL for completed send.");
		}
		// Finally release the FS Send structure
		NdisFreeMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_SEND), 0);
	}

	if (Transaction != NULL)
	{
		// Release the DMA Transaction
		WdfObjectDelete(Transaction);
	}
	else
	{
		DEBUGP(DEBUG_ERROR, "ERROR: No DMA Transaction for completed send.");
	}
    DEBUGP(DEBUG_TRACE, "<--- XNetSendComplete");
}



NDIS_STATUS
TXNblReference(
    __in  PMP_ADAPTER       Adapter,
    __in  PNET_BUFFER_LIST  NetBufferList)
/*++

Routine Description:

    Adds a reference on a NBL that is being transmitted.
    The NBL won't be returned to the protocol until the last reference is
    released.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to reference

Return Value:

    NDIS_STATUS_SUCCESS if reference was acquired succesfully. 
    NDIS_STATUS_ADAPTER_NOT_READY if the adapter state is such that we should not acquire new references to resources

--*/
{
    NdisInterlockedIncrement(&Adapter->nBusySend);

    //
    // Make sure the increment happens before ready state check
    //
    KeMemoryBarrier();

    //
    // If the adapter is not ready, undo the reference and fail the call
    //
    if (!MP_IS_READY(Adapter))
    {
        InterlockedDecrement(&Adapter->nBusySend);
        DEBUGP(DEBUG_WARN, "Could not acquire transmit reference, the adapter is not ready.");
        return NDIS_STATUS_ADAPTER_NOT_READY;
    }
    
    NdisInterlockedIncrement(&SEND_REF_FROM_NBL(NetBufferList));
    return NDIS_STATUS_SUCCESS;

}


VOID
TXNblRelease(
    __in  PMP_ADAPTER       Adapter,
    __in  PNET_BUFFER_LIST  NetBufferList,
    __in  BOOLEAN           fAtDispatch)
/*++

Routine Description:

    Releases a reference on a NBL that is being transmitted.
    If the last reference is released, the NBL is returned to the protocol.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    Adapter                     Pointer to our adapter
    NetBufferList               The NBL to release
    fAtDispatch                 TRUE if the current IRQL is DISPATCH_LEVEL

Return Value:

    None.

--*/
{

    if (0 == NdisInterlockedDecrement(&SEND_REF_FROM_NBL(NetBufferList)))
    {
        DEBUGP(DEBUG_TRACE, "Send NBL 0x%p complete.", NetBufferList);

        NET_BUFFER_LIST_NEXT_NBL(NetBufferList) = NULL;

        NdisMSendNetBufferListsComplete(
                Adapter->AdapterHandle,
                NetBufferList,
                fAtDispatch ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL:0);
    }
    else
    {
		DEBUGP(DEBUG_WARN, "Warning: Send NBL 0x%p not complete. RefCount: %i.", NetBufferList, SEND_REF_FROM_NBL(NetBufferList));
    }

    NdisInterlockedDecrement(&Adapter->nBusySend);
//	DEBUGP(DEBUG_WARN, "nBusySend %i.", Adapter->nBusySend);
}

VOID
MPCancelSend(
    __in  NDIS_HANDLE     MiniportAdapterContext,
    __in  PVOID           CancelId)
/*++

Routine Description:

    MiniportCancelSend cancels the transmission of all NET_BUFFER_LISTs that
    are marked with a specified cancellation identifier. Miniport drivers
    that queue send packets for more than one second should export this
    handler. When a protocol driver or intermediate driver calls the
    NdisCancelSendNetBufferLists function, NDIS calls the MiniportCancelSend
    function of the appropriate lower-level driver (miniport driver or
    intermediate driver) on the binding.

    Runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    CancelId                    All the packets with this Id should be cancelled

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(CancelId);

    DEBUGP(DEBUG_WARN, "---> MPCancelSend");

    //
    // This miniport completes its sends quickly, so it isn't strictly
    // neccessary to implement MiniportCancelSend.
    //
    // If we did implement it, we'd have to walk the Adapter->SendWaitList
    // and look for any NB that points to a NBL where the CancelId matches
    // NDIS_GET_NET_BUFFER_LIST_CANCEL_ID(Nbl).  For any NB that so matches,
    // we'd remove the NB from the SendWaitList and set the NBL's status to
    // NDIS_STATUS_SEND_ABORTED, then complete the NBL.
    //

    DEBUGP(DEBUG_TRACE, "<--- MPCancelSend");
}

//**************************************************************************************************
//**************************************************************************************************
//   RECEIVE FUNTIONS
//**************************************************************************************************
//**************************************************************************************************


VOID
MPReturnNetBufferLists(
    __in  NDIS_HANDLE       MiniportAdapterContext,
    __in  PNET_BUFFER_LIST  NetBufferLists,
    __in  ULONG             ReturnFlags)
/*++

Routine Description:

    NDIS Miniport entry point called whenever protocols are done with one or
    NBLs that we indicated up with NdisMIndicateReceiveNetBufferLists.

    Note that the list of NBLs may be chained together from multiple separate
    lists that were indicated up individually.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetBufferLists              NBLs being returned
    ReturnFlags                 May contain the NDIS_RETURN_FLAGS_DISPATCH_LEVEL
                                flag, which if is set, indicates we can get a
                                small perf win by not checking or raising the
                                IRQL

Return Value:

    None.

--*/
{
    PMP_ADAPTER				Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    PDATA_TRANSFER_PARAMS	pDMATrans;
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(ReturnFlags);

	//DEBUGP(DEBUG_TRACE, "---> MPReturnNetBufferLists: FirstNBL: 0x%x", NetBufferLists);

    while (NetBufferLists)
    {
		// Get the link to the DMA Transaction from the NBL.
		pDMATrans = DMATRANS_FROM_NBL(NetBufferLists);
		pDMATrans->BytesTransferred = 0;
		pDMATrans->Status = 0;

		// Feed the DMA Transaction, buffer, etc to the Receive DMA Engine
		if (pDMATrans->SG_LIST_PTR->NumberOfElements  > 0)
		{
		    NdisAcquireSpinLock(&Adapter->RecvSpinLock);
			// We pass the offset to start, the SG List, callback info, etc.
			Status = Adapter->RecvDMALink.XDMADataTransfer(Adapter->DMARecvHandle, pDMATrans);
			if (Status != 0)
			{
			    DEBUGP(DEBUG_ERROR, "MPReturnNetBufferLists, Error XDMA call, Status = 0x%x", Status);
			}
			NdisReleaseSpinLock(&Adapter->RecvSpinLock);
		}
		else
		{
		    DEBUGP(DEBUG_ERROR, "MPReturnNetBufferLists, NO SG Elements in DMATrans.");
		}
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);
    }

    //DEBUGP(DEBUG_TRACE, "<--- MPReturnNetBufferLists");
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
XNetRecvComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans
    )
{
    PMP_ADAPTER				Adapter = (PMP_ADAPTER)FSDHandle;
	PFS_NETWORK_RECV		pFSRecv;
	ULONG					NumNblsReceived = 1;
	PNET_BUFFER_LIST		FirstNBL;
	PNIC_FRAME_HEADER		RecvFrameBufferVa = NULL;
    ULONG					FrameType = 0;
    size_t                  bytesTransferred;
    NTSTATUS                Status = STATUS_SUCCESS;

//	DEBUGP(DEBUG_TRACE, "---> RXReceiveIndicate.");

    // Get the final bytes transferred count.
    bytesTransferred =  pDMATrans->BytesTransferred;

	// If any errors are reported, make it a hardware error to Windows.
    if (pDMATrans->Status & 0xFFFFFFFE)
    {
        Status = STATUS_ADAPTER_HARDWARE_ERROR;
	}

	if (pDMATrans->FS_PTR != NULL)
	{
		pFSRecv = (PFS_NETWORK_RECV)pDMATrans->FS_PTR;
		FirstNBL = (PNET_BUFFER_LIST)pFSRecv->NET_BUFFER_LIST_PTR;
		RecvFrameBufferVa = (PNIC_FRAME_HEADER)pFSRecv->RecvFrameBufferVa;

		//DEBUGP(DEBUG_INFO, "XNetRecvComplete:  NetBuffer %p, Status 0x%x, "
		//	"bytes transferred %d", FirstNBL, Status, (int) bytesTransferred);
		DEBUGP(DEBUG_INFO, "XNetRecvComplete: bytes transferred %d", (int) bytesTransferred);

		if (NIC_ADDR_IS_BROADCAST(RecvFrameBufferVa->DestAddress))
		{
	        FrameType = NDIS_PACKET_TYPE_BROADCAST;
			Adapter->FramesRxBroadcast++;
			Adapter->BytesRxBroadcast += bytesTransferred;
		    DEBUGP(DEBUG_INFO, "Recv %ld Broadcast frames", Adapter->FramesRxBroadcast);
		}
		else if(NIC_ADDR_IS_MULTICAST(RecvFrameBufferVa->DestAddress))
		{
	        FrameType = NDIS_PACKET_TYPE_MULTICAST;
			Adapter->FramesRxMulticast++;
			Adapter->BytesRxMulticast += bytesTransferred;
		    DEBUGP(DEBUG_INFO, "Recv %ld Multicast frames", Adapter->FramesRxMulticast);
		}
		else
		{
	        FrameType = NDIS_PACKET_TYPE_DIRECTED;
			Adapter->FramesRxDirected++;
		    Adapter->BytesRxDirected += bytesTransferred;
		    DEBUGP(DEBUG_INFO, "Recv %ld Directed frames", Adapter->FramesRxDirected);
		}

		if (IsFrameAcceptedByPacketFilter(Adapter, RecvFrameBufferVa->DestAddress, FrameType))
		{
			//
			// Indicate up the NBL.
			//
			// The NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL allows a perf optimization:
			// NDIS doesn't have to check and raise the current IRQL, since we
			// promise that the current IRQL is exactly DISPATCH_LEVEL already.
			//
			NdisMIndicateReceiveNetBufferLists(Adapter->AdapterHandle,
								FirstNBL, 0, NumNblsReceived,
								(NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL)
								| NDIS_RECEIVE_FLAGS_PERFECT_FILTERED
#if defined(NDIS620_MINIPORT)
		                        | NDIS_RECEIVE_FLAGS_SINGLE_QUEUE
			                    | (0)
#endif                            
				                );
		}
		else
		{
			// Recycle this buffer since we are not going to indicate it.
			pDMATrans->BytesTransferred = 0;
			pDMATrans->Status = 0;

			// Feed the DMA Transaction, buffer, etc to the Receive DMA Engine
			if (pDMATrans->SG_LIST_PTR->NumberOfElements  > 0)
			{
				NdisAcquireSpinLock(&Adapter->RecvSpinLock);
				// We pass the offset to start, the SG List, callback info, etc.
				Status = Adapter->RecvDMALink.XDMADataTransfer(Adapter->DMARecvHandle, pDMATrans);
				if (Status != 0)
				{
				    DEBUGP(DEBUG_ERROR, "RXReceiveIndicate, Error XDMA call, Status = 0x%x", Status);
				}
				NdisReleaseSpinLock(&Adapter->RecvSpinLock);
			}
			else
			{
			    DEBUGP(DEBUG_ERROR, "RXReceiveIndicate, NO SG Elements in DMATrans.");
			}
		}
	}
	else
	{
	    DEBUGP(DEBUG_ERROR, "RXReceiveIndicate, ERROR, DMA Transaction did not have an FS RECV.");
	}
//	DEBUGP(DEBUG_TRACE, "<--- RXReceiveIndicate.");
}




/*++
Routine Description:

    The AllocRxDMA function ...

    IRQL = PASSIVE_LEVEL
    
Arguments:

    Adapter                     Pointer to our adapter
    NumberOfRcbs                Number of RCB structures to allocate (and NBLs as a result)
    
    Return Value:

    NDIS_STATUS_xxx code
    
--*/         
NDIS_STATUS
AllocRxDMA(
	PMP_ADAPTER Adapter,
    ULONG NumberRxPackets
    )
{
	WDF_COMMON_BUFFER_CONFIG	commonBufferConfig;
	WDFCOMMONBUFFER				CommonBuffer;
	WDF_OBJECT_ATTRIBUTES		attributes;
	PFS_NETWORK_RECV			pFSRecv;
	PVOID						RecvFrameBufferVa = NULL;
    WDFDMATRANSACTION			dmaTransaction;
    PDATA_TRANSFER_PARAMS		pDMATrans;
	PMDL						pDataMDL;

	NDIS_STATUS					Status = NDIS_STATUS_SUCCESS;
    NET_BUFFER_LIST_POOL_PARAMETERS NblParameters;
    ULONG						index = 0;

    // Allocate an NBL pool for receive indications.
    {C_ASSERT(sizeof(NblParameters) >= NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1);}
    NblParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    NblParameters.Header.Size = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
    NblParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;

    NblParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT; // always use DEFAULT for miniport drivers
    NblParameters.fAllocateNetBuffer = TRUE;
    NblParameters.ContextSize = 0;
    NblParameters.PoolTag = NIC_TAG_RECV_NBL;
    NblParameters.DataSize = 0;

    Adapter->RecvNblPoolHandle = NdisAllocateNetBufferListPool(NdisDriverHandle,  &NblParameters);
	if (Adapter->RecvNblPoolHandle == NULL)
    {
		Status = NDIS_STATUS_RESOURCES;
        DEBUGP(DEBUG_ERROR, "NdisAllocateNetBufferListPool failed");
		return Status;
    }

    do
    {
		// Create the DMA transaction object.
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DATA_TRANSFER_PARAMS);
		Status = WdfObjectCreate(&attributes, &dmaTransaction);
	    if (!NT_SUCCESS(Status)) 
		{
		    DEBUGP(DEBUG_ERROR, "WdfObjectCreate failed %X", Status);
	        break;
        }
		pDMATrans = DMATransCtx(dmaTransaction);
		// Save a back pointer to the Transaction object, we'll use it on the completion callback
		pDMATrans->TRANSACTION_PTR = dmaTransaction;
		pDMATrans->CardAddress = 0;
		pDMATrans->BytesTransferred = 0;
		pDMATrans->Status = 0;

		// Put all the DMA Transaction objects on a list for later freeing.
	    NdisInterlockedInsertTailList(&Adapter->RxDMATransList, &pDMATrans->DT_LINK, &Adapter->RxDMATransListLock);

		pDMATrans->FS_PTR = NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
				(UINT)sizeof(FS_NETWORK_RECV), NIC_TAG_SG, NormalPoolPriority);
		if (!pDMATrans->FS_PTR)
		{
			DEBUGP(DEBUG_ERROR, "NdisAllocateMemoryWithTagPriority (FS_Recv) failed");
		    WdfObjectDelete(dmaTransaction);
			break;
		}
		NdisZeroMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_RECV));
		pFSRecv = (PFS_NETWORK_RECV)pDMATrans->FS_PTR;
		pFSRecv->NET_BUFFER_LIST_PTR = NULL;

		// Allocate space for the associated Scatter/Gather list to be passed to XDMA
		pDMATrans->SG_LIST_PTR = (PSCATTER_GATHER_LIST) NdisAllocateMemoryWithTagPriority(NdisDriverHandle,
			(UINT)sizeof(SCATTER_GATHER_LIST) + sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_RX_SG_ELEMENTS, 
			NIC_TAG_SG, NormalPoolPriority);
		if (pDMATrans->SG_LIST_PTR == NULL)
		{
			DEBUGP(DEBUG_ERROR, "NdisAllocateMemoryWithTagPriority (SG List) failed");
		    WdfObjectDelete(dmaTransaction);
			break;
		}
		NdisZeroMemory(pDMATrans->SG_LIST_PTR, (UINT)sizeof(SCATTER_GATHER_LIST) + 
				(sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_RX_SG_ELEMENTS));
		pDMATrans->SG_LIST_PTR->NumberOfElements = 0;

		// Create the Recieve Buffer - this is where the ethernet frames land.
		WDF_COMMON_BUFFER_CONFIG_INIT(&commonBufferConfig, FILE_64_BYTE_ALIGNMENT);
		Status = WdfCommonBufferCreateWithConfig(Adapter->RecvDMALink.DMAEnabler,
											 Adapter->FrameSize,
											 &commonBufferConfig,
											 WDF_NO_OBJECT_ATTRIBUTES,
											 &CommonBuffer);
		if (NT_SUCCESS(Status))
		{
			// Setup the status pool pointers
			RecvFrameBufferVa = WdfCommonBufferGetAlignedVirtualAddress(CommonBuffer);
			pDMATrans->SG_LIST_PTR->Elements[pDMATrans->SG_LIST_PTR->NumberOfElements].Address = 
					WdfCommonBufferGetAlignedLogicalAddress(CommonBuffer);
			pDMATrans->SG_LIST_PTR->Elements[pDMATrans->SG_LIST_PTR->NumberOfElements].Length = Adapter->FrameSize;
			pDMATrans->SG_LIST_PTR->NumberOfElements++;
			pFSRecv->RecvFrameBufferVa = RecvFrameBufferVa;
			DEBUGP(DEBUG_VERBOSE, "Rx buffer Created starting at address:0x%p", RecvFrameBufferVa);
		}

		// Allocate an MDL to associate to the buffer for passing up the stack
		pDataMDL = NdisAllocateMdl(Adapter, RecvFrameBufferVa, Adapter->FrameSize);
		if (pDataMDL == NULL)
		{
			DEBUGP(DEBUG_ERROR, "NdisAllocateMdl:Data MDL failed");
			break;
		}
		NDIS_MDL_LINKAGE(pDataMDL) = NULL;

		//
	    // Allocate an NBL with its single NET_BUFFER from the preallocated pool that cooresponds to the
		//  data buffer we allocated above.
	    //
		pFSRecv->NET_BUFFER_LIST_PTR = NdisAllocateNetBufferAndNetBufferList(Adapter->RecvNblPoolHandle,
                                                 0,		// No Context
                                                 0,		// No Context backfill */
                                                 pDataMDL,
                                                 0,		// Data Offset is 0
                                                 Adapter->FrameSize);
		if (pFSRecv->NET_BUFFER_LIST_PTR == NULL)
		{
			DEBUGP(DEBUG_ERROR, "NdisAllocateNetBufferAndNetBufferList failed");
            break;
        }
	
		// Save a link to the DMA Transaction from the NBL.
		DMATRANS_FROM_NBL((PNET_BUFFER_LIST)pFSRecv->NET_BUFFER_LIST_PTR) = pDMATrans;
		// Save a link to the common buffer for later shutdown and freeing.
		COMMON_BUFFER_FROM_NBL((PNET_BUFFER_LIST)pFSRecv->NET_BUFFER_LIST_PTR) = CommonBuffer;

		// Feed the DMA Transaction, buffer, etc to the Receive DMA Engine
		Status = NDIS_STATUS_ADAPTER_NOT_READY;
		if (pDMATrans->SG_LIST_PTR->NumberOfElements  > 0)
		{
		    NdisAcquireSpinLock(&Adapter->RecvSpinLock);
			// We pass the offset to start, the SG List, callback info, etc.
			Status = Adapter->RecvDMALink.XDMADataTransfer(Adapter->DMARecvHandle, pDMATrans);
			NdisReleaseSpinLock(&Adapter->RecvSpinLock);
		}

		if (!NT_SUCCESS(Status)) 
		{
			// The transaction failed, the DmaTransaction must be deleted.
			if (pDMATrans->SG_LIST_PTR != NULL)
			{
				// Free the Scatter/Gather list
				NdisFreeMemory(pDMATrans->SG_LIST_PTR, 
					(UINT)sizeof(SCATTER_GATHER_LIST) + 
						(sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_RX_SG_ELEMENTS), 0);
				pDMATrans->SG_LIST_PTR = NULL;
			}
			if (pDMATrans->FS_PTR != NULL)
			{
				// Finally release the FS Send structure
				NdisFreeMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_RECV), 0);
			}

			WdfObjectDelete(dmaTransaction);
			DEBUGP(DEBUG_ERROR, "<-- XNet call to XDMA failed: error 0x%x", Status);
			break;
		}
		index++;
    } while(index < NumberRxPackets);

    return Status;
}


/*++

Routine Description:

    This routines checks to see whether the packet can be accepted
    for transmission based on the currently programmed filter type
    of the NIC and the mac address of the packet.

    With real adapter, this routine would be implemented in hardware.  However,
    since we don't have any hardware to do the matching for us, we'll do it in
    the driver.

Arguments:

    Adapter                     Our adapter that is receiving a frame
    FrameData                   The raw frame, starting at the frame header
    cbFrameData                 Number of bytes in the FrameData buffer

Return Value:

    TRUE if the frame is accepted by the packet filter, and should be indicated
    up to the higher levels of the stack.

    FALSE if the frame doesn't match the filter, and should just be dropped.

--*/
BOOLEAN
IsFrameAcceptedByPacketFilter(
    __in  PMP_ADAPTER  Adapter,
    __in_bcount(NIC_MACADDR_SIZE) PUCHAR  DestAddress,
    __in  ULONG        FrameType)
{
    BOOLEAN     result = FALSE;

    //DEBUGP(DEBUG_TRACE, "---> IsFrameAcceptedByPacketFilter PacketFilter = 0x%08x, FrameType = 0x%08x",
    //        Adapter->PacketFilter, FrameType);

    if (!MP_IS_READY(Adapter))
	{
		return FALSE;
	}

    do
    {
        //
        // If the NIC is in promiscuous mode, we will accept anything
        // and everything.
        //
        if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
        {
            result = TRUE;
            break;
        }

        switch (FrameType)
        {
            case NDIS_PACKET_TYPE_BROADCAST:
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
                {
                    //
                    // If it's a broadcast packet and broadcast is enabled,
                    // we can accept that.
                    //
                    result = TRUE;
                }
                break;


            case NDIS_PACKET_TYPE_MULTICAST:
                //
                // If it's a multicast packet and multicast is enabled,
                // we can accept that.
                //
                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
                {
                    result = TRUE;
                    break;
                }
                else if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
                {
                    ULONG index;

                    //
                    // Check to see if the multicast address is in our list
                    //
                    ASSERT(Adapter->ulMCListSize <= NIC_MAX_MCAST_LIST);
                    for (index=0; index < Adapter->ulMCListSize && index < NIC_MAX_MCAST_LIST; index++)
                    {
                        if (NIC_ADDR_EQUAL(DestAddress, Adapter->MCList[index]))
                        {
                            result = TRUE;
                            break;
                        }
                    }
                }
                break;


            case NDIS_PACKET_TYPE_DIRECTED:

                if (Adapter->PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
                {
                    //
                    // This has to be a directed packet. If so, does packet dest
                    // address match with the mac address of the NIC.
                    //
                    if (NIC_ADDR_EQUAL(DestAddress, Adapter->CurrentAddress))
                    {
                        result = TRUE;
                        break;
                    }
                }

                break;
        }

    } while (FALSE);

    DEBUGP(DEBUG_TRACE, "<--- IsFrameAcceptedByPacketFilter Result = %u", result);
    return result;
}


/*++

Routine Description:

    This routines checks the destination address and sets the appropriate counters

Arguments:

    Adapter                     Our adapter that is receiving a frame

Return Value:

--*/
NDIS_STATUS
SetTxStatsFromNetBuffer(
	PMP_ADAPTER			Adapter,
    PNET_BUFFER			NetBuffer,
	ULONG				bytesTransferred
	)
{
	PMDL	CurrentMdl;
	PUCHAR	SrcMemory;
	ULONG	MdlOffset;
	ULONG	Length;

	//
    // Start from current MDL
    //
    CurrentMdl = NET_BUFFER_CURRENT_MDL(NetBuffer);

	//
    // Map MDL memory to System Address Space. LowPagePriority means mapping may fail if 
    // system is low on memory resources. 
    //
    SrcMemory = MmGetSystemAddressForMdlSafe(CurrentMdl, LowPagePriority);
    Length = MmGetMdlByteCount(CurrentMdl);
    if (SrcMemory == NULL)
    {
	    DEBUGP(DEBUG_ERROR, "SetTxStatsFromNetBuffer SrcMemory == NULL");
		return NDIS_STATUS_RESOURCES;
	}

	//
    // The first MDL segment should be accessed from the current MDL offset
    //
    MdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(NetBuffer);
    SrcMemory += MdlOffset;
    Length -= MdlOffset;

	if (Length < XXGE_MAC_ADDR_SIZE)
	{
	    DEBUGP(DEBUG_ERROR, "SetTxStatsFromNetBuffer Length < MAC Address");
		return NDIS_STATUS_RESOURCES;
	}

	if (NIC_ADDR_IS_BROADCAST(SrcMemory))
	{
		Adapter->FramesTxBroadcast++;
		Adapter->BytesTxBroadcast += bytesTransferred;
	    DEBUGP(DEBUG_VERBOSE, "Sent %ld Broadcast frames", Adapter->FramesTxBroadcast);
	}
	else if(NIC_ADDR_IS_MULTICAST(SrcMemory))
	{
		Adapter->FramesTxMulticast++;
		Adapter->BytesTxMulticast += bytesTransferred;
	    DEBUGP(DEBUG_VERBOSE, "Sent %ld Multicast frames", Adapter->FramesTxMulticast);
	}
	else
	{
		Adapter->FramesTxDirected++;
	    Adapter->BytesTxDirected += bytesTransferred;
	    DEBUGP(DEBUG_VERBOSE, "Sent %ld Directed frames", Adapter->FramesTxDirected);
	}

    return NDIS_STATUS_SUCCESS;
}
