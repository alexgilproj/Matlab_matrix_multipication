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
* @file XNet_Link.c
*
* This file contains the XDMA linkage functions.
*
*  Portions of this file are Copyright (c) Microsoft Corporation.  All rights reserved.
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
#include "XNet_Link.tmh"
#endif // TRACE_ENABLED


/***************************** Local Prototypes ******************************/

NDIS_STATUS XNetInitializeScatterGatherDma(IN PMP_ADAPTER Adapter);
VOID SetTestMode(IN PMP_ADAPTER	Adapter);


/***************************** Local Functions *******************************/

NDIS_STATUS
XNetLinkToXDMA(
    NDIS_HANDLE		MiniportAdapterHandle,
    PMP_ADAPTER		Adapter
	)
{
    NDIS_STATUS					Status = NDIS_STATUS_SUCCESS;

	WDFDRIVER					hDriver;
    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_IO_TARGET_OPEN_PARAMS   openParams;
    PWSTR                       pSymlink = NULL;
    UNICODE_STRING              di;  
    ULONG						nameLength;

	DEBUGP(DEBUG_TRACE, "---> XNetLinkToXDMA");

    do
    {
        //
        // Initialize list and lock of the Receive DMA Transactions
        //
        NdisInitializeListHead(&Adapter->RxDMATransList);
        NdisAllocateSpinLock(&Adapter->RxDMATransListLock);

        //
        // NdisMGetDeviceProperty function enables us to get the:
        // PDO - created by the bus driver to represent our device.
        // FDO - created by NDIS to represent our miniport as a function driver.
        // NextDeviceObject - deviceobject of another driver (filter)
        //                      attached to us at the bottom.
        // In a pure NDIS miniport driver, there is no use for this
        // information, but a NDISWDM driver would need to know this so that it
        // can transfer packets to the lower WDM stack using IRPs.
        //
        NdisMGetDeviceProperty(MiniportAdapterHandle, &Adapter->Pdo, &Adapter->Fdo,
                &Adapter->NextDeviceObject, NULL, NULL);

        Status = IoGetDeviceProperty (Adapter->Pdo, DevicePropertyDeviceDescription,
                                      NIC_ADAPTER_NAME_SIZE, Adapter->Name,
                                      &nameLength);
        if (!NT_SUCCESS (Status))
        {
            DEBUGP(DEBUG_ERROR, "IoGetDeviceProperty failed (0x%x)", Status);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WDF_DEVICE_INFO);
		hDriver = WdfGetDriver();
        Status = WdfDeviceMiniportCreate(hDriver, &attributes, Adapter->Fdo,
                                         Adapter->NextDeviceObject, Adapter->Pdo,
                                         &Adapter->WdfDevice);
        if (!NT_SUCCESS (Status))
        {
            DEBUGP(DEBUG_ERROR, "WdfDeviceMiniportCreate failed (0x%x)", Status);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = Adapter->WdfDevice;
        GetWdfDeviceInfo(Adapter->WdfDevice)->Adapter = Adapter;

		Status = WdfIoTargetCreate(Adapter->WdfDevice, &attributes, &Adapter->IoTarget);
        if (!NT_SUCCESS(Status)) {
            
            /* Device Initialization failed.*/          
            DEBUGP(DEBUG_ERROR, "WdfIoTargetCreate failed 0x%x", Status);
            Status = NDIS_STATUS_FAILURE;
			break;
        }

		// Look up the GUID for the XDMA driver and only the active one
        Status = IoGetDeviceInterfaces((LPGUID)&GUID_V7_XDMA_INTERFACE, NULL, 
                                        0, &pSymlink);
        if(NT_SUCCESS(Status) && (NULL != pSymlink) )
        {
            RtlInitUnicodeString(&di, pSymlink);
            DEBUGP(DEBUG_TRACE, "IoGetDeviceInterfaces success - %wZ ", &di);
        }
        else
        {
            Status = NDIS_STATUS_FAILURE;
			break;
        }

		// Open an interface to the XDMA Driver.
        WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams, &di, STANDARD_RIGHTS_ALL);
        Status = WdfIoTargetOpen(Adapter->IoTarget, &openParams);
        if (!NT_SUCCESS(Status)) 
		{
            DEBUGP(DEBUG_ERROR, "WdfIoTargetOpen Failed 0x%x", Status);
            WdfObjectDelete(Adapter->IoTarget);
            Status = NDIS_STATUS_FAILURE;
			break;
        }   
        
		// Get the REGISTER_XDRIVER structure presented by the XDMA Driver.
		Status = WdfIoTargetQueryForInterface(Adapter->IoTarget, &GUID_V7_XDMA_INTERFACE,
						(PINTERFACE)&Adapter->DriverLink, sizeof(REGISTER_XDRIVER), 1, NULL);
        if (!NT_SUCCESS(Status)) 
		{
            /* Device Initialization failed.WdfIoTargetQueryForInterface*/
            DEBUGP(DEBUG_ERROR, "WdfFdoQueryForInterface failed 0x%x", Status);
            Status = NDIS_STATUS_FAILURE;
			break;
        }
	    // One last step is to make sure the version numbers agree.
		if (Adapter->DriverLink.XDMA_DRV_VERSION != XNET_FSD_VERSION)
		{							
			DEBUGP(DEBUG_ERROR, "XDMA Version does not match 0x%x vs. 0x%x", Adapter->DriverLink.XDMA_DRV_VERSION, XNET_FSD_VERSION);
			Status = STATUS_DEVICE_CONFIGURATION_ERROR;
			break;
		}

		Status = XNetInitSend(Adapter);
        if (!NT_SUCCESS(Status)) 
		{
			if (Adapter->DMAAutoConfigure)
			{
				Adapter->S2CDMAEngine++;
				if (Adapter->S2CDMAEngine >=  CS2_DMA_ENGINE_OFFSET)
				{
					Adapter->S2CDMAEngine = 0;
				}
				Status = XNetInitSend(Adapter);
			}

			if (!NT_SUCCESS(Status)) 
			{
	            /* Failed to initialize the Send DMA Engine */
				DEBUGP(DEBUG_ERROR, "XNetInitSend failed 0x%x", Status);
				Status = NDIS_STATUS_FAILURE;
				break;
			}
		}

		Status = XNetInitRecv(Adapter);
		if (!NT_SUCCESS(Status)) 
		{
			if (Adapter->DMAAutoConfigure)
			{
				Adapter->C2SDMAEngine++;
				if (Adapter->C2SDMAEngine >  MAX_CS2_DMA_ENGINE)
				{
					Adapter->C2SDMAEngine =  CS2_DMA_ENGINE_OFFSET;
				}
				Status = XNetInitRecv(Adapter);
			}

			if (!NT_SUCCESS(Status)) 
			{
	            /* Failed to initialize the Recv DMA Engine */
				DEBUGP(DEBUG_ERROR, "XNetInitRecv failed 0x%x", Status);
				Status = NDIS_STATUS_FAILURE;
				break;
			}
		}

		// Setup the Design Mode, etc.
		SetTestMode(Adapter);
		Adapter->pUserSpace		= (PUCHAR)Adapter->SendDMALink.PUSER_SPACE;
		Adapter->pMACStats		= (PMAC_STATS)Adapter->SendDMALink.PUSER_SPACE + XNET_CONTROL_STAT_REGS;
		if (Adapter->C2SDMAEngine ==  CS2_DMA_ENGINE_OFFSET)
		{
			Adapter->MACRegisters	= (PUCHAR)Adapter->SendDMALink.PUSER_SPACE + XNET_CONTROL_REGS_MAC0;
			Adapter->XGERegisters	= (PUCHAR)Adapter->SendDMALink.PUSER_SPACE + XXGE0_REGISTER_OFFSET;
			Adapter->PhyAddress		= XXGE0_PHY_ADDRESS;
		}
		else
		{
			Adapter->MACRegisters	= (PUCHAR)Adapter->SendDMALink.PUSER_SPACE + XNET_CONTROL_REGS_MAC1;
			Adapter->XGERegisters	= (PUCHAR)Adapter->SendDMALink.PUSER_SPACE + XXGE1_REGISTER_OFFSET;
			Adapter->PhyAddress		= XXGE1_PHY_ADDRESS;
		}
		DEBUGP(DEBUG_ALWAYS, "USER_SPACE 0x%p, User Space 0x%p, MAC Regs 0x%p\n", 
			Adapter->SendDMALink.PUSER_SPACE, Adapter->pUserSpace, Adapter->MACRegisters);

	} while(FALSE);

	if (pSymlink != NULL)
	{
		ExFreePool(pSymlink);
	}

	// Check for any failures, if so cleanup and exit.
	if (!NT_SUCCESS(Status)) 
	{
		XNetShutdown(Adapter);
	}

    DEBUGP(DEBUG_TRACE, "---> XNetLinkToXDMA");
	return Status;
}



/*++
Routine Description:

    Link to an XDMA S2C DMA Engine
 
Arguments:

    Adapter		Pointer to the Adapter data space

Return Value:

    Status

--*/
NTSTATUS
XNetInitSend(
    IN PMP_ADAPTER			Adapter
    )
{
    REGISTER_DMA_ENGINE_REQUEST     Request;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;

    DEBUGP(DEBUG_TRACE, "--> XNetInitWrite");

	Adapter->SendDMAStatus = INITIALIZING;

	Request.DMA_ENGINE = Adapter->S2CDMAEngine;
	Request.DIRECTION = WdfDmaDirectionWriteToDevice;
	Request.XDMA_DEVICE_CONTEXT = Adapter->DriverLink.XDMA_DEVICE_CONTEXT;
	Request.FSD_DEVICE_CONTEXT = Adapter;
	Request.FSDCompletion = XNetSendComplete;
	Request.FSD_VERSION = XNET_FSD_VERSION;

	Adapter->DMASendHandle = Adapter->DriverLink.DmaRegister(&Request, &Adapter->SendDMALink);
	if (Adapter->DMASendHandle != INVALID_XDMA_HANDLE)
	{
		if (Adapter->SendDMALink.XDMA_DMA_VERSION == XNET_FSD_VERSION)
		{
			DEBUGP(DEBUG_ALWAYS, "DMASendHandle valid for DMA Engine %d.", Request.DMA_ENGINE);
			Adapter->SendDMAStatus = AVAILABLE;
			// Get the DMA Adapter handle
			Adapter->pSendDmaAdapter = WdfDmaEnablerWdmGetDmaAdapter(Adapter->SendDMALink.DMAEnabler,
															WdfDmaDirectionWriteToDevice);
			status = STATUS_SUCCESS;
		}
		else
		{
			Adapter->SendDMAStatus = FAILED;
			DEBUGP(DEBUG_ERROR, "ERROR: XDMA (0x%x) and XNET (0x%x) versions are incompatible.", 
				Adapter->SendDMALink.XDMA_DMA_VERSION, XNET_FSD_VERSION);
		}
	}
	else
	{
		Adapter->SendDMAStatus = FAILED;
		DEBUGP(DEBUG_ERROR, "ERROR: DMASendHandle is invalid for DMA Engine %d.", Request.DMA_ENGINE);
	}
	DEBUGP(DEBUG_TRACE, "<-- XNetInitWrite");
    return status;
}


/*++
Routine Description:

    Link to a XDMA Driver C2S DMA Engine

Arguments:

    Adapter		Pointer to the Adapter data space

Return Value:

	Status

--*/
NTSTATUS
XNetInitRecv(
    IN PMP_ADAPTER			Adapter
    )
{
    REGISTER_DMA_ENGINE_REQUEST     Request;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;

    DEBUGP(DEBUG_TRACE, "--> XNetInitRecv");

	Adapter->RecvDMAStatus = INITIALIZING;

	Request.DMA_ENGINE = Adapter->C2SDMAEngine;
	Request.DIRECTION = WdfDmaDirectionReadFromDevice;
	Request.XDMA_DEVICE_CONTEXT = Adapter->DriverLink.XDMA_DEVICE_CONTEXT;
	Request.FSD_DEVICE_CONTEXT = Adapter;
	Request.FSDCompletion = XNetRecvComplete;
	Request.FSD_VERSION = XNET_FSD_VERSION;

	Adapter->DMARecvHandle = Adapter->DriverLink.DmaRegister(&Request, &Adapter->RecvDMALink);
	if (Adapter->DMARecvHandle != INVALID_XDMA_HANDLE)
	{
		if (Adapter->RecvDMALink.XDMA_DMA_VERSION == XNET_FSD_VERSION)
		{
			DEBUGP(DEBUG_ALWAYS, "DMARecvHandle valid for DMA Engine %d.", Request.DMA_ENGINE);
			Adapter->RecvDMAStatus = AVAILABLE;
			// Get the DMA Adapter handle
			Adapter->pRecvDmaAdapter = WdfDmaEnablerWdmGetDmaAdapter(Adapter->RecvDMALink.DMAEnabler,
															WdfDmaDirectionReadFromDevice);
			status = STATUS_SUCCESS;
		}
		else
		{
			Adapter->SendDMAStatus = FAILED;
			DEBUGP(DEBUG_ERROR, "ERROR: XDMA (0x%x) and XNET (0x%x) versions are incompatible.", 
				Adapter->RecvDMALink.XDMA_DMA_VERSION, XNET_FSD_VERSION);
		}
	}
	else
	{
		Adapter->RecvDMAStatus = FAILED;
		DEBUGP(DEBUG_ERROR, "ERROR: DMARecvHandle is invalid for DMA Engine %d.", Request.DMA_ENGINE);
	}
	DEBUGP(DEBUG_TRACE, "<-- XNetInitRecv");
    return status;
}


/*++

Routine Description:

    Reset the device to put the device in a known initial state.
    This is called from D0Exit when the device is torn down or
    when the system is shutdown. Note that Wdf has already
    called out EvtDisable callback to disable the interrupt.

Arguments:

    Adapter -  Pointer to our adapter

Return Value:

    None

--*/
VOID
XNetShutdown(
    IN PMP_ADAPTER			Adapter
    )
{

	DEBUGP(DEBUG_TRACE, "---> XNetShutdown");

	Adapter->SendDMAStatus = SHUTDOWN;
	Adapter->RecvDMAStatus = SHUTDOWN;

	if (Adapter->DMASendHandle != INVALID_XDMA_HANDLE)
	{
		Adapter->DriverLink.DmaUnregister(Adapter->DMASendHandle);
	}

	if (Adapter->DMARecvHandle != INVALID_XDMA_HANDLE)
	{
		Adapter->DriverLink.DmaUnregister(Adapter->DMARecvHandle);
	}

	if (Adapter->WdfDevice != NULL)
	{
		WdfObjectDelete(Adapter->WdfDevice);
	}

	NdisFreeSpinLock(&Adapter->RxDMATransListLock);

    DEBUGP(DEBUG_TRACE, "<--- XNetShutdown");
}

/*++

Routine Description:

    Control the Packet Generator, Checker and Loopback functions.

Arguments:

    Adapter - The adapter data space
            
    TC - Pointer to the TestCmd structure.

Return Value:
	NTSTATUS
--*/

VOID
SetTestMode(
	IN PMP_ADAPTER			Adapter
	)
{
	/* Now write the registers */
	if (Adapter->DesignMode == 0)
	{
	    Adapter->SendDMALink.PUSER_SPACE->DESIGN_REGISTERS.DESIGN_MODE_ADDRESS = RAW_DESIGN_MODE;
	}
	else
	{
		Adapter->SendDMALink.PUSER_SPACE->DESIGN_REGISTERS.DESIGN_MODE_ADDRESS = PERF_DESIGN_MODE;
	}
//	Adapter->SendDMALink.PUSER_SPACE->APP1_REGISTERS.LOOPBACK_CHECKER.BIT.LOOPBACK_ENABLE = 1;
}