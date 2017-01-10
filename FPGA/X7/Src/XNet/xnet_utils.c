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
* @file XNet_Utils.c
*
* This file contains functions for allocating and freeing adpater data
* and for setting up and tearing down the DMA Engines..
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
#include "XNet_Utils.tmh"
#endif // TRACE_ENABLED

// Local function prototypes
NDIS_STATUS ReadJumboFramesConfig(NDIS_HANDLE ConfigurationHandle, PMP_ADAPTER Adapter);



NDIS_STATUS
NICAllocAdapter(
    __in NDIS_HANDLE MiniportAdapterHandle, 
    __deref_out PMP_ADAPTER *pAdapter)
/*++
Routine Description:

    The NICAllocAdapter function allocates and initializes the memory used to track a miniport instance. 

    NICAllocAdapter runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterHandle       NDIS handle for the adapter. 
    pAdapter                    Receives the allocated and initialized adapter memory.

    Return Value:

    NDIS_STATUS_xxx code

--*/    
{
    PMP_ADAPTER Adapter = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
//    LONG index;

    DEBUGP(DEBUG_TRACE, "---> NICAllocAdapter");

    PAGED_CODE();

    *pAdapter = NULL;

    do
    {
        //
        // Allocate extra space for the MP_ADAPTER memory (one cache line's worth) so that we can
        // reference the memory from a cache-aligned starting point. This way we guarantee that 
        // members with cache-aligned directives are actually cache aligned. 
        //
        PVOID UnalignedAdapterBuffer = NULL;
        ULONG UnalignedAdapterBufferSize = sizeof(MP_ADAPTER)+ NdisGetSharedDataAlignment();
        
        //
        // Allocate memory for adapter context (unaligned)
        //
        UnalignedAdapterBuffer = NdisAllocateMemoryWithTagPriority(
                NdisDriverHandle,
                UnalignedAdapterBufferSize,
                NIC_TAG,
                NormalPoolPriority);
        if (!UnalignedAdapterBuffer)
        {
            Status = NDIS_STATUS_RESOURCES;
            DEBUGP(DEBUG_ERROR, "Failed to allocate memory for adapter context");
            break;
        }

        //
        // Zero the memory block
        //
        NdisZeroMemory(UnalignedAdapterBuffer, UnalignedAdapterBufferSize);

        //
        // Start the Adapter pointer at a cache-aligned boundary
        //
        Adapter = ALIGN_UP_POINTER_BY(UnalignedAdapterBuffer, NdisGetSharedDataAlignment());

        //
        // Store the unaligned information so that we can free it later
        // 
        Adapter->UnalignedAdapterBuffer = UnalignedAdapterBuffer;
        Adapter->UnalignedAdapterBufferSize = UnalignedAdapterBufferSize;

        //
        // Set the adapter handle
        //
        Adapter->AdapterHandle = MiniportAdapterHandle;

		// Set the Default App number (this is the DMA Engines to use)
		Adapter->S2CDMAEngine		= DEFAULT_APP;
		Adapter->C2SDMAEngine		= DEFAULT_APP + CS2_DMA_ENGINE_OFFSET;
		Adapter->DMAAutoConfigure	= TRUE;
		Adapter->MACAddressOverride = FALSE;
		Adapter->DesignMode			= DEFAULT_DESIGN_MODE_SETTING;

        NdisInitializeListHead(&Adapter->List);

		// Set to default to the largest possible frame size
		Adapter->FrameSize		= HW_MAX_FRAME_SIZE; 
								// HW_MAX_JUMBO_VLAN_FRAME_SIZE;

        //
        // Set the default lookahead buffer size.
        //
        Adapter->ulLookahead		= NIC_MAX_LOOKAHEAD;

		// Setup the reserve amounts
		Adapter->ulMaxBusySends		= NIC_MAX_BUSY_SENDS;
		Adapter->ulMaxBusyRecvs		= NIC_MAX_BUSY_RECVS;

		NdisAllocateSpinLock(&Adapter->SendSpinLock);
		NdisAllocateSpinLock(&Adapter->RecvSpinLock);


		Adapter->ulLinkSendSpeed =	10000000000;
		Adapter->ulLinkRecvSpeed =	10000000000;

    } while(FALSE);

    *pAdapter = Adapter;

    //
    // In the failure case, the caller of this routine will end up
    // calling NICFreeAdapter to free all the successfully allocated
    // resources.
    //
    DEBUGP(DEBUG_TRACE, "<--- NICAllocAdapter");
    return Status;
}


void NICFreeAdapter(
    __in  PMP_ADAPTER Adapter)
/*++
Routine Description:

    The NICFreeAdapter function frees memory used to track a miniport instance. Should only be called from MPHaltEx.

    NICFreeAdapter runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Adapter                    Adapter memory to free.

    Return Value:

    NDIS_STATUS_xxx code

--*/     
{
    PLIST_ENTRY			pEntry;
	PNET_BUFFER			NB;
	WDFCOMMONBUFFER		CommonBuffer;
	PMDL				RxMDL;
	PFS_NETWORK_RECV	pFSRecv;

    DEBUGP(DEBUG_TRACE, "---> NICFreeAdapter");

    ASSERT(Adapter);

	// Shutdown the Enthernet controller
	XXgEthernet_Stop(Adapter);

    //
    // Free all the resources we allocated in NICAllocAdapter.
    //
	if (Adapter->RxDMATransList.Flink)
    {
		while (NULL != (pEntry = NdisInterlockedRemoveHeadList(
				&Adapter->RxDMATransList,
				&Adapter->RxDMATransListLock)))
		{
			PDATA_TRANSFER_PARAMS pDMATrans = CONTAINING_RECORD(pEntry, DATA_TRANSFER_PARAMS, DT_LINK);
			CommonBuffer = NULL;

			if (pDMATrans->FS_PTR != NULL)
			{
				pFSRecv = (PFS_NETWORK_RECV)pDMATrans->FS_PTR;

				if (pFSRecv->NET_BUFFER_LIST_PTR != NULL)
				{
					DEBUGP(DEBUG_VERBOSE, "Freeing Rx buffer at address:0x%p", pFSRecv->RecvFrameBufferVa);
					NB = NET_BUFFER_LIST_FIRST_NB((PNET_BUFFER_LIST)pFSRecv->NET_BUFFER_LIST_PTR);

					// Free the MDL
					RxMDL = NET_BUFFER_CURRENT_MDL(NB);
					if (RxMDL != NULL)
					{
						NdisFreeMdl(RxMDL);
					}
					CommonBuffer = COMMON_BUFFER_FROM_NBL((PNET_BUFFER_LIST)pFSRecv->NET_BUFFER_LIST_PTR);

					NdisFreeNetBufferList(pFSRecv->NET_BUFFER_LIST_PTR);
					pFSRecv->NET_BUFFER_LIST_PTR = NULL;
				}
				// Free the common memory frame buffer
				if (CommonBuffer != NULL)
				{
					WdfObjectDelete(CommonBuffer);
					CommonBuffer = NULL;
				}
				if (pDMATrans->SG_LIST_PTR != NULL)
				{
					// Free the scatter/Gather list memory
					NdisFreeMemory(pDMATrans->SG_LIST_PTR, 
									(UINT)sizeof(SCATTER_GATHER_LIST) + 
									(sizeof(SCATTER_GATHER_ELEMENT) * MAX_NUMBER_RX_SG_ELEMENTS), 0);
					pDMATrans->SG_LIST_PTR = NULL;
				}

				// Finally release the FS Send structure
				NdisFreeMemory(pDMATrans->FS_PTR, (UINT)sizeof(FS_NETWORK_RECV), 0);
			}

			// Free the DMA Transaction object
		    // Retrieve the DMA Transaction from the transfer strucutre
			if (pDMATrans->TRANSACTION_PTR != NULL)
			{
				// Release the DMA Transaction
				WdfObjectDelete(pDMATrans->TRANSACTION_PTR);
			}
		}   
	}

    if (Adapter->RecvNblPoolHandle)
    {
		NdisFreeNetBufferListPool(Adapter->RecvNblPoolHandle);
        Adapter->RecvNblPoolHandle = NULL;
	}

    NdisFreeSpinLock(&Adapter->RxDMATransListLock);
	NdisFreeSpinLock(&Adapter->SendSpinLock);
	NdisFreeSpinLock(&Adapter->RecvSpinLock);

    //
    // Finally free the memory for adapter context. 
    //
    NdisFreeMemory(Adapter->UnalignedAdapterBuffer, Adapter->UnalignedAdapterBufferSize, 0);

    DEBUGP(DEBUG_TRACE, "<--- NICFreeAdapter");
}


NDIS_STATUS 
ReadAPPConfig(
    NDIS_HANDLE		ConfigurationHandle,
    PMP_ADAPTER		Adapter)
/*++
Routine Description:

    This routine will read the Xilinx APP configuration from the 
	NDIS registry, and set the results to the App field.

Arguments:

    ConfigurationHandle     - Adapter configuration handle
    Adapter                 - Pointer to our adapter
    
Return Value:

    NDIS_STATUS

--*/         
{
    NDIS_STATUS Status		= NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING APPKeyword	= NDIS_STRING_CONST("APP"); 

    DEBUGP(DEBUG_TRACE, "---> ReadAppConfig");

	NdisReadConfiguration(&Status, &Parameter, ConfigurationHandle, &APPKeyword, NdisParameterInteger);

    if(Status != NDIS_STATUS_SUCCESS)
    {
		DEBUGP(DEBUG_ERROR, "NdisReadConfiguration for APP failed Status 0x%x, defaulting to App0.", Status);
        Status = NDIS_STATUS_SUCCESS;
    }
	else
	{
		if (Parameter->ParameterData.IntegerData < MAX_NUMBER_APPS)
		{
			Adapter->DMAAutoConfigure = FALSE;
			if (Parameter->ParameterData.IntegerData < 2)
			{
				Adapter->S2CDMAEngine = Parameter->ParameterData.IntegerData;
				Adapter->C2SDMAEngine = Parameter->ParameterData.IntegerData + CS2_DMA_ENGINE_OFFSET;
			}
			else
			{
				// Application2	= "XOver-DMA Engines 0 & 33"
				// Application3	= "XOver-DMA Engines 1 & 32"
				if (Parameter->ParameterData.IntegerData == 2)
				{
					Adapter->S2CDMAEngine = 0;
					Adapter->C2SDMAEngine = 1 + CS2_DMA_ENGINE_OFFSET;
				}
				else
				{
					Adapter->S2CDMAEngine = 1;
					Adapter->C2SDMAEngine = 0 + CS2_DMA_ENGINE_OFFSET;
				}
			}
		    DEBUGP(DEBUG_ALWAYS, "Set S2C %d, C2S %d", Adapter->S2CDMAEngine, Adapter->C2SDMAEngine);
		}
		else
		{
		    DEBUGP(DEBUG_WARN, "Found invalid App number in registry (%d)", Parameter->ParameterData.IntegerData);
		}
    }
    DEBUGP(DEBUG_TRACE, "<--- ReadAppConfig Status 0x%x", Status);
    return Status;
}

NDIS_STATUS 
ReadJumboFramesConfig(
    NDIS_HANDLE		ConfigurationHandle,
    PMP_ADAPTER		Adapter)
/*++
Routine Description:

    This routine will read the Jumbo Frames parameter from the 
	NDIS registry, and set the results to the FrameSize field.

Arguments:

    ConfigurationHandle     - Adapter configuration handle
    Adapter                 - Pointer to our adapter
    
Return Value:

    NDIS_STATUS

--*/         
{
    NDIS_STATUS Status		= NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING JumboKeyword	= NDIS_STRING_CONST("*JumboPacket"); 

    DEBUGP(DEBUG_TRACE, "---> ReadJumboFramesConfig");

	NdisReadConfiguration(&Status, &Parameter, ConfigurationHandle, &JumboKeyword, NdisParameterInteger);

    if(Status != NDIS_STATUS_SUCCESS)
    {
		DEBUGP(DEBUG_ERROR, "NdisReadConfiguration for Jumbo Frames failed Status 0x%x.", Status);
        Status = NDIS_STATUS_SUCCESS;
    }
	else
	{
		if (Parameter->ParameterData.IntegerData < XXGE_MAX_JUMBO_FRAME_SIZE)
		{
			Adapter->FrameSize = Parameter->ParameterData.IntegerData;
		    DEBUGP(DEBUG_ALWAYS, "Jumbo Frame size set to %d", Adapter->FrameSize);
		}
		else
		{
		    DEBUGP(DEBUG_WARN, "Found invalid Jumbo Frame size in registry (%d)", Parameter->ParameterData.IntegerData);
		}
    }
    DEBUGP(DEBUG_TRACE, "<--- ReadJumboFramesConfig Status 0x%x", Status);
    return Status;
}

NDIS_STATUS 
ReadDesignModeConfig(
    NDIS_HANDLE		ConfigurationHandle,
    PMP_ADAPTER		Adapter)
/*++
Routine Description:

    This routine will read the "Design Mode parameter from the 
	NDIS registry, and set the results to the DesignMode field.

Arguments:

    ConfigurationHandle     - Adapter configuration handle
    Adapter                 - Pointer to our adapter
    
Return Value:

    NDIS_STATUS

--*/         
{
    NDIS_STATUS Status		= NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Parameter = NULL;
    NDIS_STRING DesignModeKeyword	= NDIS_STRING_CONST("DesignMode"); 

    DEBUGP(DEBUG_TRACE, "---> ReadDesignModeConfig");

	NdisReadConfiguration(&Status, &Parameter, ConfigurationHandle, &DesignModeKeyword, NdisParameterInteger);

    if(Status != NDIS_STATUS_SUCCESS)
    {
		DEBUGP(DEBUG_ERROR, "NdisReadConfiguration for DesignMode failed Status 0x%x.", Status);
        Status = NDIS_STATUS_SUCCESS;
    }
	else
	{
		if (Parameter->ParameterData.IntegerData < MAX_DESIGN_MODE_SETTING)
		{
			Adapter->DesignMode = Parameter->ParameterData.IntegerData;
		    DEBUGP(DEBUG_ALWAYS, "Design Mode set to %d", Adapter->DesignMode);
		}
		else
		{
		    DEBUGP(DEBUG_WARN, "Found invalid Design Mode setting in registry (%d)", Parameter->ParameterData.IntegerData);
		}
    }
    DEBUGP(DEBUG_TRACE, "<--- ReadDesignModeConfig Status 0x%x", Status);
    return Status;
}



NDIS_STATUS 
NICReadRegParameters(
    PMP_ADAPTER Adapter
)
/*++
Routine Description:

    Read device configuration parameters from the registry
 
Arguments:

    Adapter                         Pointer to our adapter

    Should be called at IRQL = PASSIVE_LEVEL.
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_FAILURE
    NDIS_STATUS_RESOURCES                                       

--*/    
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    NDIS_CONFIGURATION_OBJECT     ConfigurationParameters;
    NDIS_HANDLE     ConfigurationHandle;
    PUCHAR          NetworkAddress;
    UINT            Length;
    
    DEBUGP(DEBUG_TRACE, "--> NICReadRegParameters\n");

    PAGED_CODE();

    //
    // Open the registry for this adapter to read advanced
    // configuration parameters stored by the INF file.
    //

    NdisZeroMemory(&ConfigurationParameters, sizeof(ConfigurationParameters));
    ConfigurationParameters.Header.Type		= NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigurationParameters.Header.Size		= NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
    ConfigurationParameters.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
    ConfigurationParameters.NdisHandle		= Adapter->AdapterHandle;
    ConfigurationParameters.Flags			= 0;

    Status = NdisOpenConfigurationEx(&ConfigurationParameters, &ConfigurationHandle);
    if(Status != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(DEBUG_ERROR, "NdisOpenConfigurationEx Status = 0x%x", Status);
        return NDIS_STATUS_FAILURE;
    }

	Status = ReadAPPConfig(ConfigurationHandle, Adapter);
	Status = ReadJumboFramesConfig(ConfigurationHandle, Adapter);
	Status = ReadDesignModeConfig(ConfigurationHandle, Adapter);
   
    //
    // Read NetworkAddress registry value and use it as the current address 
    // if there is a software configurable NetworkAddress specified in 
    // the registry.
    //
    NdisReadNetworkAddress(&Status, &NetworkAddress, &Length, ConfigurationHandle);

	if ((Status == NDIS_STATUS_SUCCESS) && (Length == ETH_LENGTH_OF_ADDRESS))
    {
		if (ETH_IS_MULTICAST(NetworkAddress) || ETH_IS_BROADCAST(NetworkAddress))
		{
			DEBUGP(DEBUG_ERROR, "Overriding NetworkAddress is invalid - %02x-%02x-%02x-%02x-%02x-%02x", 
				    NetworkAddress[0], NetworkAddress[1], NetworkAddress[2],
                    NetworkAddress[3], NetworkAddress[4], NetworkAddress[5]);
		}
        else
        {
			ETH_COPY_NETWORK_ADDRESS(Adapter->CurrentAddress, NetworkAddress);
			Adapter->MACAddressOverride = TRUE;
		    DEBUGP(DEBUG_ALWAYS, "Hardware MAC Address overridden"); 
        }    
	}

    //Adapter->ulLinkSpeed = NIC_LINK_SPEED;

    //
    // Close the configuration registry
    //
    NdisCloseConfiguration(ConfigurationHandle);
    
    DEBUGP(DEBUG_TRACE, "<-- NICReadRegParameters");

    return NDIS_STATUS_SUCCESS;
}


VOID
NICIndicateNewLinkSpeed(
	PMP_ADAPTER	pAdapter,
	ULONG64		linkSpeed,
	ULONG		phyLinkState
    )    
{
    NDIS_STATUS_INDICATION  statusIndication;
    NDIS_LINK_STATE         linkState;

    NdisZeroMemory(&statusIndication, sizeof(NDIS_STATUS_INDICATION));
    NdisZeroMemory(&linkState, sizeof(NDIS_LINK_STATE));

    //
    // Fill in object headers
    //
    statusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    statusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
    statusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);

    linkState.Header.Revision = NDIS_LINK_STATE_REVISION_1;
    linkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
    linkState.Header.Size = sizeof(NDIS_LINK_STATE);

    //
    // Link state buffer
    //
    linkState.MediaConnectState = phyLinkState;
    linkState.MediaDuplexState = MediaDuplexStateFull;
    linkState.RcvLinkSpeed = linkSpeed;
    linkState.XmitLinkSpeed = linkSpeed;

    //
    // Fill in the status buffer
    // 
    statusIndication.StatusCode = NDIS_STATUS_LINK_STATE;
    statusIndication.SourceHandle = pAdapter->AdapterHandle;
    statusIndication.DestinationHandle = NULL;
    statusIndication.RequestId = 0;

    statusIndication.StatusBuffer = &linkState;
    statusIndication.StatusBufferSize = sizeof(NDIS_LINK_STATE);

    //
    // Indicate the status to NDIS
    //
    NdisMIndicateStatusEx(pAdapter->AdapterHandle, &statusIndication);
}
