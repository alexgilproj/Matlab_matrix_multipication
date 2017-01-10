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

    XDMA_Init.c

Abstract:

    Contains most of initialization functions

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_Init.tmh"
#endif // TRACE_ENABLED


NTSTATUS XlxDMARelease(PDEVICE_EXTENSION pDevExt);

NTSTATUS XlxGetRegistrySettings(PDEVICE_EXTENSION pDevExt);


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
XlxInitializeDeviceExtension(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    NTSTATUS                status;
    WDF_IO_QUEUE_CONFIG     queueConfig;
#if 1
	PCI_CAPABILITIES_HEADER	CapabilityHdr;
	PPCI_EXPRESS_CAPABILITY	pPCICapabilities;
	unsigned char			CapabilityPtr = 0;
	ULONG                   GetBusData_status;
#endif
	int                     i;

    //PAGED_CODE();

    pDevExt->pDMARegisters          =   NULL;
    pDevExt->NumberDMAEngines       =   0;
    pDevExt->StaticEnumerationDone  =   FALSE; 

	pDevExt->tstatsWrite			= 0;
	pDevExt->tstatsRead				= 0;
	pDevExt->tstatsNum				= 0;

    for (i = 0; i < MAX_SUPPORTED_DMA_ENGINES; i++)
    {
        pDevExt->pDMAExt[i]         =   NULL;
    }

    // Set Maximum Transfer Length
    pDevExt->MaximumDMADescriptorPoolSize = (sizeof(DMA_DESCRIPTOR_ALIGNED) * DEFAULT_NUMBER_DMA_DESCRIPTORS) + PAGE_SIZE;
	DEBUGP(DEBUG_INFO, "Maximum DMA Descriptor Pool Size %Iu", pDevExt->MaximumDMADescriptorPoolSize);

    // Setup the IOCTL interface. This is the only user application interface 
    // we make avialable for this driver. The Function Specific drivers layered above,
    // contain the data transfer interfaces to user applications (if any).
//    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
//                              WdfIoQueueDispatchSequential);
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                              WdfIoQueueDispatchSequential);

    queueConfig.EvtIoDeviceControl = XlxEvtIoControl;
    status = WdfIoQueueCreate( pDevExt->Device,
                               &queueConfig,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &pDevExt->IoctlQueue);

    if(!NT_SUCCESS(status)) {
		DEBUGP(DEBUG_ERROR, "WdfIoQueueCreate failed: 0x%x", status);
        return status;
    }

    // Set the ioctl Queue forwarding for IRP_MJ_IOCTL requests.
    status = WdfDeviceConfigureRequestDispatching( pDevExt->Device,
                                                   pDevExt->IoctlQueue,
                                                   WdfRequestTypeDeviceControl);
    if(!NT_SUCCESS(status)) {
		DEBUGP(DEBUG_ERROR, "DeviceConfigureRequestDispatching failed: 0x%x", status);
        return status;
    }

	// Read the Registry entry for Raw Ethernet mode
	XlxGetRegistrySettings(pDevExt);
#if 1
	// Setup an interface with the underlying PCI Bus driver
	status = WdfFdoQueryForInterface(pDevExt->Device, &GUID_BUS_INTERFACE_STANDARD,
		(PINTERFACE) &pDevExt->PCIInterface, sizeof(BUS_INTERFACE_STANDARD), 1, NULL);
    if (!NT_SUCCESS(status)) {
		DEBUGP(DEBUG_ALWAYS, "Request WdfFdoQueryForInterface failed: 0x%x", status);
        return status;
    }
#if 1
	DEBUGP(DEBUG_ALWAYS,"About to call GetBusData in XlxInitializeDeviceExtension");
	// Now Read the PCI Config space and save it in our local context.
	GetBusData_status = pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
				&pDevExt->PciCfg, 0, sizeof(PCI_CONFIG_SPACE));
	DEBUGP(DEBUG_ALWAYS,"Came out of GetBusData call in XlxInitializeDeviceExtension");
	if(GetBusData_status == 0)
	{
	   DEBUGP(DEBUG_ALWAYS, "Request GetBusData failed: 0x%x", GetBusData_status);
	}

	// Check for extend capabilities, i.e. MSI, MSI-X support.
	if ((pDevExt->PciCfg.STATUS & PCI_STATUS_CAPABILITIES_LIST) == PCI_STATUS_CAPABILITIES_LIST)
	{
		if ((pDevExt->PciCfg.HEADER_TYPE == PCI_DEVICE_TYPE) || 
			(pDevExt->PciCfg.HEADER_TYPE == PCI_MULTIFUNCTION))
		{
			CapabilityPtr = pDevExt->PciCfg.CAPABILITIES_PTR;
			// Do we have an offset?
			while (CapabilityPtr)
			{
				// Get the next capability record
				pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
					&CapabilityHdr, CapabilityPtr, sizeof(PCI_CAPABILITIES_HEADER));
				if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_PCI_EXPRESS)
				{
					pPCICapabilities = ExAllocatePoolWithTag(PagedPool, sizeof(PCI_EXPRESS_CAPABILITY), 'XLXD');
					if (pPCICapabilities != NULL)
					{
						pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
								pPCICapabilities, CapabilityPtr, sizeof(PCI_EXPRESS_CAPABILITY));

						pDevExt->MPS = pPCICapabilities->DeviceControl.MaxPayloadSize;
						pDevExt->MRRS = pPCICapabilities->DeviceControl.MaxReadRequestSize;
						pDevExt->LinkSpeed = pPCICapabilities->LinkStatus.LinkSpeed;
						pDevExt->LinkWidth = pPCICapabilities->LinkStatus.LinkWidth;

						ExFreePoolWithTag(pPCICapabilities, 'XLXD');
					}
				}
				else if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_MSIX)
				{
					pDevExt->IntMode = INT_MSIX;
				}
				else if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_MSI)
				{
					pDevExt->IntMode = INT_MSI;
				}
				// next
				CapabilityPtr = CapabilityHdr.Next;
			}
		}
	}
#endif
#endif
#if 0
	pDevExt->MPS = 0;
	pDevExt->MRRS = 3;
	pDevExt->LinkSpeed =  3;
	pDevExt->LinkWidth = 8;
#endif

    return status;
}

#if 0
/*++
Routine Description:

    This routine is called by D0Start. Here the device context is
    initialized after reading the PCI config space.
    allocated.

Arguments:

    DevExt     Pointer to the Device Extension

Return Value:

     NTSTATUS

--*/
NTSTATUS
XlxReadPciConfigSpace(
    IN PDEVICE_EXTENSION pDevExt
    )
	{
	NTSTATUS                status = STATUS_SUCCESS;
	PCI_CAPABILITIES_HEADER	CapabilityHdr;
	PPCI_EXPRESS_CAPABILITY	pPCICapabilities;
	unsigned char			CapabilityPtr = 0;
	ULONG                   GetBusData_status;

	// Setup an interface with the underlying PCI Bus driver
	status = WdfFdoQueryForInterface(pDevExt->Device, &GUID_BUS_INTERFACE_STANDARD,
		(PINTERFACE) &pDevExt->PCIInterface, sizeof(BUS_INTERFACE_STANDARD), 1, NULL);
    if (!NT_SUCCESS(status)) {
		DEBUGP(DEBUG_ALWAYS, "Request WdfFdoQueryForInterface failed: 0x%x", status);
        return status;
    }
	// Now Read the PCI Config space and save it in our local context.
	GetBusData_status = pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
				&pDevExt->PciCfg, 0, sizeof(PCI_CONFIG_SPACE));

	if(GetBusData_status == 0)
	{
	   status = 1;
	   DEBUGP(DEBUG_ALWAYS, "Request GetBusData failed: 0x%x", GetBusData_status);
	}

	// Check for extend capabilities, i.e. MSI, MSI-X support.
	if ((pDevExt->PciCfg.STATUS & PCI_STATUS_CAPABILITIES_LIST) == PCI_STATUS_CAPABILITIES_LIST)
	{
		if ((pDevExt->PciCfg.HEADER_TYPE == PCI_DEVICE_TYPE) || 
			(pDevExt->PciCfg.HEADER_TYPE == PCI_MULTIFUNCTION))
		{
			CapabilityPtr = pDevExt->PciCfg.CAPABILITIES_PTR;
			// Do we have an offset?
			while (CapabilityPtr)
			{
				// Get the next capability record
				pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
					&CapabilityHdr, CapabilityPtr, sizeof(PCI_CAPABILITIES_HEADER));
				if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_PCI_EXPRESS)
				{
					pPCICapabilities = ExAllocatePoolWithTag(PagedPool, sizeof(PCI_EXPRESS_CAPABILITY), 'XLXD');
					if (pPCICapabilities != NULL)
					{
						pDevExt->PCIInterface.GetBusData(pDevExt->PCIInterface.Context, PCI_WHICHSPACE_CONFIG,
								pPCICapabilities, CapabilityPtr, sizeof(PCI_EXPRESS_CAPABILITY));

						pDevExt->MPS = pPCICapabilities->DeviceControl.MaxPayloadSize;
						pDevExt->MRRS = pPCICapabilities->DeviceControl.MaxReadRequestSize;
						pDevExt->LinkSpeed = pPCICapabilities->LinkStatus.LinkSpeed;
						pDevExt->LinkWidth = pPCICapabilities->LinkStatus.LinkWidth;

						ExFreePoolWithTag(pPCICapabilities, 'XLXD');
					}
				}
				else if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_MSIX)
				{
					pDevExt->IntMode = INT_MSIX;
				}
				else if (CapabilityHdr.CapabilityID == PCI_CAPABILITY_ID_MSI)
				{
					pDevExt->IntMode = INT_MSI;
				}
				// next
				CapabilityPtr = CapabilityHdr.Next;
			}
		}
	  }
	  return status;
	}
#endif

/*++
Routine Description:

    Gets the HW resources assigned by the bus driver from the start-irp
    and maps it to system address space.

Arguments:

    DevExt      Pointer to our DEVICE_EXTENSION

Return Value:

     None

--*/
NTSTATUS
XlxPrepareHardware(
    IN PDEVICE_EXTENSION pDevExt,
    IN WDFCMRESLIST     ResourcesTranslated
    )
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  desc;
    ULONG               i;
    NTSTATUS            status = STATUS_DEVICE_CONFIGURATION_ERROR;
    int                 nBAR;

    //PAGED_CODE();

    pDevExt->NumberBARs =   0;

    // Parse the resource list and save the resource information.
    // We are only interested in memory resources so we can memory map them.
    for (i=0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) 
    {
        desc = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        if(!desc) {
			DEBUGP(DEBUG_ERROR, "WdfResourceCmGetDescriptor failed");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        nBAR = pDevExt->NumberBARs;
        switch (desc->Type) 
        {
            case CmResourceTypeMemoryLarge:
            case CmResourceTypeMemory:
                if (nBAR < MAX_BARS)
                {
                    pDevExt->NumberBARs++;
                    if ((desc->Flags & CM_RESOURCE_MEMORY_LARGE_64) == CM_RESOURCE_MEMORY_LARGE_64)
                    {
                        pDevExt->BARInfo[nBAR].BAR_PHYS_ADDR = desc->u.Memory64.Start;
                        pDevExt->BARInfo[nBAR].BAR_LENGTH = desc->u.Memory64.Length64;
                    }
                    else if ((desc->Flags & CM_RESOURCE_MEMORY_LARGE_48) == CM_RESOURCE_MEMORY_LARGE_48)
                    {
                        pDevExt->BARInfo[nBAR].BAR_PHYS_ADDR = desc->u.Memory48.Start;
                        pDevExt->BARInfo[nBAR].BAR_LENGTH = desc->u.Memory48.Length48;
                    }
                    else if ((desc->Flags & CM_RESOURCE_MEMORY_LARGE_40) == CM_RESOURCE_MEMORY_LARGE_40)
                    {
                        pDevExt->BARInfo[nBAR].BAR_PHYS_ADDR = desc->u.Memory40.Start;
                        pDevExt->BARInfo[nBAR].BAR_LENGTH = desc->u.Memory40.Length40;
                    }
                    else
                    {
                        pDevExt->BARInfo[nBAR].BAR_PHYS_ADDR = desc->u.Memory.Start;
                        pDevExt->BARInfo[nBAR].BAR_LENGTH = desc->u.Memory.Length;
                    }

                    pDevExt->BARInfo[nBAR].BAR_VIRT_ADDR = MmMapIoSpace(pDevExt->BARInfo[nBAR].BAR_PHYS_ADDR,
                                                                        pDevExt->BARInfo[nBAR].BAR_LENGTH,
                                                                        MmNonCached);

                    if (pDevExt->BARInfo[nBAR].BAR_VIRT_ADDR == NULL) {
						DEBUGP(DEBUG_ERROR, "Unable to map BAR %d", nBAR);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

					DEBUGP(DEBUG_INFO, "BAR[%d] is Memory [Start:0x%llx End:0x%llx], Virt Addr 0x%p",
                                nBAR, desc->u.Memory.Start.QuadPart,
                                desc->u.Memory.Start.QuadPart + desc->u.Memory.Length,
								pDevExt->BARInfo[nBAR].BAR_VIRT_ADDR);
                    status = STATUS_SUCCESS;
                }
                break;


            case CmResourceTypePort:
				DEBUGP(DEBUG_ERROR, "Found PORT BAR[%d] - Not Supported", nBAR);
                break;

            default:
                //
                // Ignore all other descriptors
                //
                break;
        }
    }

    // Set a Pointer to BAR0 Registers
    pDevExt->pDMARegisters = (PDMA_REGISTER_MAP) pDevExt->BARInfo[DMA_ENGINE_BAR].BAR_VIRT_ADDR;
	pDevExt->pUserSpace = (PUSER_SPACE_MAP) &pDevExt->pDMARegisters->USER_SPACE;

    // Now that we have the BARs setup, setup the DMA Engines
    status = XlxInitializeDMAEngines(pDevExt);
    if (!NT_SUCCESS(status)) 
    {
        return status;
    }

    return status;
}


/*++
Routine Description:

    Initializes the all the DMA Engines into seperate contexts. We allocate a context
    area for each DMA engine along with queues, buffers, DPCs, etc. so each DMA Engine can
    operate autonomously from each other.

Arguments:

    pDevExt      Pointer to our DEVICE_EXTENSION

Return Value:

     Status

--*/
NTSTATUS
XlxInitializeDMAEngines(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt;
    NTSTATUS                status = STATUS_DEVICE_CONFIGURATION_ERROR;
    WDF_DMA_ENABLER_CONFIG  DMADescriptorConfig;
    WDF_COMMON_BUFFER_CONFIG DMADescriptorBufferConfig;
    WDF_COMMON_BUFFER_CONFIG InternalDMABufferConfig;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDF_DPC_CONFIG          DPCConfig;
    PDPC_CONTEXT            pDPCCtx;
    int                     dmaIdx;
	int						i;
   // PAGED_CODE();

	pDevExt->ScalingFactor = 1 << pDevExt->pUserSpace->DESIGN_REGISTERS.PERF_SCALING_FACTOR.BIT.SCALING_FACTOR;

    // Make sure we start of at array element 0 in the DMA Engine table.
    pDevExt->NumberDMAEngines = 0;
    // Make sure we found at least BAR0
    if (pDevExt->NumberBARs > 0)
    {
        // Make sure that we mapped BAR0
        if (pDevExt->pDMARegisters != NULL)
        {
            // Search all of the available DMA Engine slots to look for valid DMA Engines
            for (dmaIdx = 0; dmaIdx < MAX_NUMBER_DMA_ENGINES; dmaIdx++)
            {
				// Check if the DMA Engine is present by checking for a reset bit
                if (pDevExt->pDMARegisters->DMA_ENGINE[dmaIdx].DMA_ENGINE_REGS.CAPABILITIES.BIT.ENGINE_PRESET == 1)
                {
					// Double check to make sure it is a packet type DMA Engine.
                    if (pDevExt->pDMARegisters->DMA_ENGINE[dmaIdx].DMA_ENGINE_REGS.CAPABILITIES.BIT.FIFO_PACKET_ENGINE == 1)
                    {
                        // We found a DMA Eengine, allocate the DMAExt space (pDMAExt)
                        pDevExt->pDMAExt[pDevExt->NumberDMAEngines] =
                            (PDMA_ENGINE_EXTENSION) ExAllocatePoolWithTag(NonPagedPool,
                                                                          sizeof(DMA_ENGINE_EXTENSION),
                                                                          'XLXD');
                        // Make sure the allocation was successful
                        if (pDevExt->pDMAExt[pDevExt->NumberDMAEngines] != NULL)
                        {
                            // Load the pointer to the DMA Ext context
                            pDMAExt = pDevExt->pDMAExt[pDevExt->NumberDMAEngines];
                            // Initialize the DMA Engine Context
                            RtlZeroMemory(pDMAExt, sizeof(DMA_ENGINE_EXTENSION));

                            // Setup a pointer to the DMA Engine Registers and get the interesting attributes.
                            pDMAExt->pDMAEngRegs = (PDMA_ENGINE_REGISTERS)&pDevExt->pDMARegisters->DMA_ENGINE[dmaIdx];
                            pDMAExt->DMAEngineNumber = pDevExt->NumberDMAEngines;
                            pDMAExt->InternalDMAEngineNumber = pDMAExt->pDMAEngRegs->CAPABILITIES.BIT.ENGINE_NUMBER;
							pDMAExt->ScalingFactor = (1 << pDMAExt->pDMAEngRegs->CAPABILITIES.BIT.SCALING_FACTOR);
							DEBUGP(DEBUG_INFO, "DMA Eng %d: Scaling Factor = %d", dmaIdx, pDMAExt->ScalingFactor);


                            pDMAExt->pDeviceExtension = pDevExt;
                            pDMAExt->EngineState	= INITIALIZING;
							pDMAExt->TestMode		= 0;
							pDMAExt->BDerrs			= 0;

							// Clear status, counters, mode controls, etc.
							pDMAExt->MaxPktSize		= 0;
							pDMAExt->MinPktSize		= 0;
							pDMAExt->TestMode		= 0;
							pDMAExt->bInternalTestMode = FALSE;
							
                            if (pDMAExt->pDMAEngRegs->CAPABILITIES.BIT.CARD_TO_SYSTEM)
                            {
                                pDMAExt->DMADirection = WdfDmaDirectionReadFromDevice;
                            }
                            else
                            {
                                pDMAExt->DMADirection = WdfDmaDirectionWriteToDevice;
                            }

							// Clear all the stats (this is already done by the RtlZeroMemory)
							for (i = 0; i < MAX_STATS; i++)
							{
								pDMAExt->DStats[i].LAT = 0;
								pDMAExt->DStats[i].LWT = 0;
								pDMAExt->DStats[i].LBR = 0;
								pDMAExt->DStats[i].IPS = 0;
								pDMAExt->DStats[i].DPS = 0;
								pDMAExt->SStats[i].LBR = 0;
							}

							pDMAExt->dstatsRead = 0;
							pDMAExt->dstatsWrite = 0;
							pDMAExt->dstatsNum = 0;
							pDMAExt->sstatsRead = 0;
							pDMAExt->sstatsWrite = 0;
							pDMAExt->sstatsNum = 0;

							// Clear the bdcount stats which enables interrupt enabling based on coalesce count.
							pDMAExt->IntrBDCount = 0;
							pDMAExt->PktCoalesceCount = 1;
#if 0
							//Clear the transaction pointer which stores the last transaction handled by DPC
							pDMAExt->pXferParams->BytesTransferred = 0;
			                pDMAExt->pXferParams = NULL;
#endif

                            // Setup the Spinlocks, DMAEnabler object, DMA Descriptor buffers, DPC
                            //  This is only for the DMA Engine allocation. Actual Scatter/Gather for user
                            // data buffers will happen in the layered driver above if applicable.
                                
                            // Create a new DMA Enabler for the DMA Descriptor buffer pool.
                            WDF_DMA_ENABLER_CONFIG_INIT( &DMADescriptorConfig,
                                                         WdfDmaProfileScatterGather,
                                                         pDevExt->MaximumDMADescriptorPoolSize);

                            status = WdfDmaEnablerCreate( pDevExt->Device,
                                                          &DMADescriptorConfig,
                                                          WDF_NO_OBJECT_ATTRIBUTES,
                                                          &pDMAExt->DMAEnabler32Bit);
                            if (!NT_SUCCESS (status)) 
                            {
								DEBUGP(DEBUG_ERROR, "WdfDmaEnablerCreate failed: 0x%x", status);
                                return status;
                            }

                            pDMAExt->TotalNumberDescriptors = DEFAULT_NUMBER_DMA_DESCRIPTORS;
                            pDMAExt->AvailNumberDescriptors = DEFAULT_NUMBER_DMA_DESCRIPTORS;
                            pDMAExt->DMADescriptorBufferSize = sizeof(DMA_DESCRIPTOR_ALIGNED) * pDMAExt->TotalNumberDescriptors;

                            // NWL DMA Descriptors must be aligned on 32 bytes boundaries.
                            WDF_COMMON_BUFFER_CONFIG_INIT(&DMADescriptorBufferConfig, FILE_32_BYTE_ALIGNMENT);
                            status = WdfCommonBufferCreateWithConfig(pDMAExt->DMAEnabler32Bit,
                                                                     pDMAExt->DMADescriptorBufferSize,
                                                                     &DMADescriptorBufferConfig,
                                                                     WDF_NO_OBJECT_ATTRIBUTES,
                                                                     &pDMAExt->DMADescriptorBuffer);
                            if (!NT_SUCCESS(status)) {
								DEBUGP(DEBUG_ERROR, "WdfCommonBufferCreate for DMA Descriptors failed: 0x%x", status);
                                return status;
                            }

                            pDMAExt->pDMADescriptorBase = WdfCommonBufferGetAlignedVirtualAddress(pDMAExt->DMADescriptorBuffer);
                            pDMAExt->DMADescriptorBufferPhysAddr = WdfCommonBufferGetAlignedLogicalAddress(pDMAExt->DMADescriptorBuffer);
                            RtlZeroMemory(pDMAExt->pDMADescriptorBase, pDMAExt->DMADescriptorBufferSize);

							DEBUGP(DEBUG_INFO, "CommonBuffer 0x%p  (#0x%I64X), length %Id",
                                        pDMAExt->pDMADescriptorBase,
                                        pDMAExt->DMADescriptorBufferPhysAddr.QuadPart,
                                        WdfCommonBufferGetLength(pDMAExt->DMADescriptorBuffer) );

							// Allocate an internal DMA buffer just fo doing performance testing.
							pDMAExt->InternalDMABufferSize = DEFAULT_INTERNAL_BUFFER_SIZE;
							WDF_COMMON_BUFFER_CONFIG_INIT(&InternalDMABufferConfig, FILE_32_BYTE_ALIGNMENT);
							status = WdfCommonBufferCreateWithConfig(pDMAExt->DMAEnabler32Bit,
																	pDMAExt->InternalDMABufferSize,
																	&InternalDMABufferConfig,
																	WDF_NO_OBJECT_ATTRIBUTES,
																	&pDMAExt->InternalDMABuffer);
							if (!NT_SUCCESS(status)) 
							{
								DEBUGP(DEBUG_ERROR, "WdfCommonBufferCreate for Internal buffer failed: 0x%x", status);
								return status;
							}
							pDMAExt->pInternalDMABufferBase = WdfCommonBufferGetAlignedVirtualAddress(pDMAExt->InternalDMABuffer);
							pDMAExt->InternalDMABufferPhysAddr = WdfCommonBufferGetAlignedLogicalAddress(pDMAExt->InternalDMABuffer);

                            // Create a new DMA Enabler instance for the FSD to use.
                            // Use Scatter/Gather, 64-bit Addresses, Duplex-type profile.
                            WDF_DMA_ENABLER_CONFIG_INIT( &DMADescriptorConfig,
                                                         WdfDmaProfileScatterGather64,
                                                         MAXIMUM_DMA_TRANSFER_LENGTH );

                            status = WdfDmaEnablerCreate( pDevExt->Device,
                                                          &DMADescriptorConfig,
                                                          WDF_NO_OBJECT_ATTRIBUTES,
                                                          &pDMAExt->FSDDMAEnabler);
                            if (!NT_SUCCESS (status)) {
								DEBUGP(DEBUG_ERROR, "WdfDmaEnablerCreate failed: 0x%x", status);
                                return status;
                            }

                            // Create the spinlock for protecting the DMA Engine queues.
                            status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &pDMAExt->QueueHeadLock);
                            if (!NT_SUCCESS(status)) {
                        		DEBUGP(DEBUG_ERROR, "QueueHeadLock WdfSpinLockCreate failed: 0x%x", status);
                                return status;
                            }

                            status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &pDMAExt->QueueTailLock);
                            if (!NT_SUCCESS(status)) {
                           		DEBUGP(DEBUG_ERROR, "QueueTailLock WdfSpinLockCreate failed: 0x%x", status);
                                return status;
                            }

						    // Create the spinlock for use during DMA Register & Unregister calls.
							status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &pDMAExt->RequestLock);
							if (!NT_SUCCESS(status)) {
								DEBUGP(DEBUG_ERROR, "Request WdfSpinLockCreate failed: 0x%x", status);
								return status;
							}
#if 1
							// Create the DPC for this DMA Engine
                            WDF_DPC_CONFIG_INIT(&DPCConfig, XlxDMADPC);
                            DPCConfig.AutomaticSerialization = FALSE;
                            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DPC_CONTEXT);
                            attributes.ParentObject = pDevExt->Device;
                            status = WdfDpcCreate(&DPCConfig, &attributes, &pDMAExt->CompletionDPC);
                            if(!NT_SUCCESS(status)) {
								DEBUGP(DEBUG_ERROR, "WdfDpcCreate failed: 0x%x", status);
                                return status;
                            }

                            // Store a pointer to this DMA Engine information for when the DPC is called.
                            pDPCCtx = DPCContext(pDMAExt->CompletionDPC);
                            pDPCCtx->pDMAExt = pDMAExt;
#endif

	                        status = InitializeDMADescriptors(pDMAExt);
							if (!NT_SUCCESS(status)) 
							{
								DEBUGP(DEBUG_ERROR, "Initialize DMA Descriptors failed: 0x%x", status);
							}
                            // On to the next DMA Engine...
                            pDevExt->NumberDMAEngines++;
                            if (pDevExt->NumberDMAEngines > MAX_SUPPORTED_DMA_ENGINES)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return status;
}


/*++

Routine Description:

    Sets the device and DMA Engines to be ready for requests.
    This is called from D0Entry when the device should be ready.
    This is where most Power Management functions reside.

Arguments:

    DevExt -  Pointer to our adapter

Return Value:

    None

--*/
NTSTATUS
XlxD0Startup(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt;
	WDF_TIMER_CONFIG		timerConfig;
	WDF_OBJECT_ATTRIBUTES	timerAttributes;
    int                     dmaEngineIdx;
    NTSTATUS                status = STATUS_SUCCESS;

	DEBUGP(DEBUG_TRACE, "---> XlxD0Startup");
#if 0
	XlxReadPciConfigSpace(pDevExt);
#endif

    for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
        // Setup a pointer to the context (if any).
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        // Verify we have allocated space for the DMA Engine
        if (pDMAExt != NULL)
        {
            pDMAExt->EngineState = AVAILABLE;
			DEBUGP(DEBUG_INFO,	"Dma Engine %d with channel %d is available", pDMAExt->DMAEngineNumber,pDMAExt->InternalDMAEngineNumber );
        }
    }

	pDevExt->PollTimerHandle = NULL;
	if (pDevExt->TestConfig == TEST_CONFIG_INTERNAL_BUFFER)
	{
		// Create a timer for polling stats every one millisecond
		WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, XDMAPollTimer, 1);
		timerConfig.AutomaticSerialization = TRUE;
		WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
		timerAttributes.ParentObject = pDevExt->Device;
		status= WdfTimerCreate(&timerConfig, &timerAttributes, &pDevExt->PollTimerHandle);
		if (!NT_SUCCESS (status)) 
		{
			DEBUGP(DEBUG_ERROR,	"WdfTimerCreate failed: 0x%x", status);
			return status;
		}
		WdfTimerStart(pDevExt->PollTimerHandle, WDF_REL_TIMEOUT_IN_MS(1));
	}
#if 0
	for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
		pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        if (pDMAExt != NULL)
		{
			if(pDMAExt->EngineState == AVAILABLE)
			{
				pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle = NULL;
				WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, XDMAPollEngineTimer, 1);
				timerConfig.AutomaticSerialization = TRUE;
				WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
				timerAttributes.ParentObject = pDevExt->Device;
				status= WdfTimerCreate(&timerConfig, &timerAttributes, &(pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle));
				if (!NT_SUCCESS (status)) 
				{
					DEBUGP(DEBUG_ERROR,	"WdfTimerCreate failed: 0x%x", status);
					return status;
				}
				pDevExt->PollEngineHandle[dmaEngineIdx].engineIdxToPoll = dmaEngineIdx;
				WdfTimerStart(pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle, WDF_REL_TIMEOUT_IN_MS(1));
			}
		}
	}
#endif
	// Create a timer for polling stats every second
	WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, XDMAStatsTimer, STAT_TIMER_PERIOD);
	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
	timerAttributes.ParentObject = pDevExt->Device;
	status= WdfTimerCreate(&timerConfig, &timerAttributes, &pDevExt->TimerHandle);
    if (!NT_SUCCESS (status)) 
	{
			DEBUGP(DEBUG_ERROR,	"WdfTimerCreate failed: 0x%x", status);
		return status;
    }
	WdfTimerStart(pDevExt->TimerHandle, WDF_REL_TIMEOUT_IN_MS(STAT_TIMER_PERIOD));

	DEBUGP(DEBUG_TRACE, "<--- XlxD0Startup");
    return status;
}

/************************************************************************** 
  * 
  *     SHUTDOWN FUNCTIONS
  *  
  *************************************************************************/

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
NTSTATUS
XlxD0Shutdown(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt;
    int                     dmaEngineIdx;

	DEBUGP(DEBUG_TRACE, "---> XlxD0Shutdown");

	// Shut down the poll timer
	if (pDevExt->PollTimerHandle)
	{
		WdfTimerStop(pDevExt->PollTimerHandle, TRUE);
	}

	// Shut down the stats timer
	if (pDevExt->TimerHandle)
	{
		WdfTimerStop(pDevExt->TimerHandle, TRUE);
	}
#if 0	
	for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
		// Setup a pointer to the context (if any).
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        // Verify we have allocated space for the DMA Engine
        if (pDMAExt != NULL)
        {
			if(pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle)
			{
				WdfTimerStop(pDevExt->PollEngineHandle[dmaEngineIdx].TimerHandle, TRUE);
			}
		}
	}
#endif

    for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
        // Setup a pointer to the context (if any).
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        // Verify we have allocated space for the DMA Engine
        if (pDMAExt != NULL)
        {
			pDMAExt->bInternalTestMode = FALSE;
            pDMAExt->EngineState = SHUTDOWN;
			DEBUGP(DEBUG_ALWAYS, "XlxD0Shutdown: DMA Engine %d shutdown", dmaEngineIdx);
			XlxCancelTransfers((XDMA_HANDLE)pDMAExt);

        }
    }

	DEBUGP(DEBUG_TRACE, "<--- XlxD0Shutdown");
    return STATUS_SUCCESS;
}


/*++

Routine Description:

    Releases all hardware and driver related object, interrupts,
    memory, etc. in preparation for unloading the driver.

Arguments:

    DevExt -  Pointer to our adapter

Return Value:

    None

--*/
NTSTATUS
XlxReleaseHardware(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    int                 i;

    // Disable interupts and release.

    // WdfInterrupt is already disabled so issue a full reset
    // 
    // Reset all DMA Engines to get to a quiescent state.
    if (pDevExt->pDMARegisters != NULL) {
        XlxHardwareReset(pDevExt);
	    // Release DMA Descriptor buffer, queues, DPCs
        XlxDMARelease(pDevExt);
    }

	if (pDevExt->TimerHandle)
	{
		WdfObjectDelete(pDevExt->TimerHandle);
		pDevExt->TimerHandle = NULL;
	}

    // Unmap and release the BAR memory windows.
    for (i = 0; i < MAX_BARS; i++)
    {
        if (pDevExt->BARInfo[i].BAR_VIRT_ADDR != NULL) 
        {
            MmUnmapIoSpace(pDevExt->BARInfo[i].BAR_VIRT_ADDR, pDevExt->BARInfo[i].BAR_LENGTH);
            pDevExt->BARInfo[i].BAR_VIRT_ADDR = NULL;
        }
    }
    return STATUS_SUCCESS;
}


/*++

Routine Description:
 
    Releases all DMA engine information and frees each DMA Engine context space
 
    NOTE: Assumes that the interrupts are shutdown and the DMAExt is no longer needed.
 
Arguments:

    DevExt -  Pointer to our adapter

Return Value:

    None
 
    NOTE: 

--*/
NTSTATUS
XlxDMARelease(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt;
    int                     dmaEngineIdx;
    NTSTATUS                status = STATUS_SUCCESS;

    for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
        // Setup a pointer to the context (if any).
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        // Verify we have allocated space for a DMA Engine
        if (pDMAExt != NULL)
        {

			if (pDMAExt->InternalDMABuffer != NULL)
			{
                WdfObjectDelete(pDMAExt->InternalDMABuffer);
                pDMAExt->InternalDMABuffer = NULL;
				pDMAExt->pInternalDMABufferBase = NULL;
				pDMAExt->InternalDMABufferPhysAddr.QuadPart = 0;
			}

            if (pDMAExt->DMADescriptorBuffer != NULL)
            {
                WdfObjectDelete(pDMAExt->DMADescriptorBuffer);
                pDMAExt->DMADescriptorBuffer = NULL;
                pDMAExt->pDMADescriptorBase = NULL;
                pDMAExt->DMADescriptorBufferPhysAddr.QuadPart = 0;

            }
            // Free the DMA Extension context itself.
            ExFreePoolWithTag(pDMAExt, 'XLXD');
            pDevExt->pDMAExt[dmaEngineIdx] = NULL;
			DEBUGP(DEBUG_ALWAYS, "XlxDMARelease: DMA Engine %d freed", dmaEngineIdx);
        }
    }
    return status;
}


/*++
Routine Description:

    Performs a reset of all the DMA Engines on the adapter.
 
Arguments:

    DevExt     Pointer to Device Extension

Return Value:

--*/
VOID
XlxHardwareReset(
    IN PDEVICE_EXTENSION pDevExt
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt;
    LARGE_INTEGER           delay;
    int                     dmaEngineIdx;

	DEBUGP(DEBUG_TRACE, "--> XlxHardwareReset");

    for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        if (pDMAExt != NULL)
        {
            pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET = 1;
			DEBUGP(DEBUG_ALWAYS, "XlxHardwareReset: DMA Engine %d Reset", dmaEngineIdx);
        }
    }

    //
    // Wait 100 msec.
    //
    delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(100);
    KeDelayExecutionThread( KernelMode, TRUE, &delay );

    for (dmaEngineIdx = 0; dmaEngineIdx < MAX_SUPPORTED_DMA_ENGINES; dmaEngineIdx++)
    {
        pDMAExt = pDevExt->pDMAExt[dmaEngineIdx];
        if (pDMAExt != NULL)
        {
            if (pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET_REQUEST)
            {
				DEBUGP(DEBUG_ERROR, "DMA Engine %d not completed reset", dmaEngineIdx);
                pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET = 1;
            }
        }
    }
	DEBUGP(DEBUG_TRACE, "<-- XlxHardwareReset");
}

/*++
Routine Description:
	XlxGetRegistrySettings
    
	Reads the registry for setup specific entries
 
Arguments:

    DevExt     Pointer to Device Extension

Return Value:

--*/
NTSTATUS
XlxGetRegistrySettings(
    IN PDEVICE_EXTENSION        pDevExt
    )
{
    WDFKEY      hKey = NULL;
    NTSTATUS    status;
    ULONG       value = RAW_ETHERNET_DEFAULT;
    DECLARE_CONST_UNICODE_STRING(valueRawName, L"RawEthernet");
    DECLARE_CONST_UNICODE_STRING(valueInternalName, L"TestConfig");

	DEBUGP(DEBUG_TRACE, "XlxGetRegistrySettings");

	pDevExt->RawEthernet = RAW_ETHERNET_DEFAULT;
    /* Open the device registry and read the "ChildDriverConfig" value.*/
    status = WdfDeviceOpenRegistryKey(pDevExt->Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      NULL, 
                                      &hKey);
    if (NT_SUCCESS (status)) 
    {
        status = WdfRegistryQueryULong(hKey, &valueRawName, &value);
        if (NT_SUCCESS (status)) 
        {
            // Make sure it doesn't exceed the max.
            pDevExt->RawEthernet = min(value, RAW_ETHERNET_MAX);
			DEBUGP(DEBUG_ALWAYS, "Raw Ethernet registry set to %d", value);
        } 
		else 
		{
			pDevExt->RawEthernet = RAW_ETHERNET_DEFAULT;
			DEBUGP(DEBUG_ALWAYS, "Using Raw Ethernet default");
        }
		DEBUGP(DEBUG_ALWAYS, "Raw Ethernet mode set to %d", pDevExt->RawEthernet);

		status = WdfRegistryQueryULong(hKey, &valueInternalName, &value);
        if (NT_SUCCESS (status)) 
        {
            // Make sure it doesn't exceed the max.
            pDevExt->TestConfig = min(value, TEST_CONFIG_MAX);
			DEBUGP(DEBUG_ALWAYS, "Test Config registry set to %d", value);
        } 
		else 
		{
			pDevExt->TestConfig = TEST_CONFIG_DEFAULT;
			DEBUGP(DEBUG_ALWAYS, "Using Test Config default");
        }
		DEBUGP(DEBUG_ALWAYS, "Test Config set to %d", pDevExt->TestConfig);
        WdfRegistryClose(hKey);
        hKey = NULL; // Set hKey to NULL to catch any accidental subsequent use.
    }
	return status;
}


