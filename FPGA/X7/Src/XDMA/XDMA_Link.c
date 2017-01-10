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

    XDMA_Link.c

Abstract:

    Contains the Driver to Driver Linkage functions

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_Link.tmh"
#endif // TRACE_ENABLED

NTSTATUS Bus_DoStaticEnumeration(PDEVICE_EXTENSION pDevExt);
NTSTATUS Bus_PlugInDevice(WDFDEVICE Device, ULONG HardwareIds, ULONG SerialNo);
NTSTATUS Bus_CreatePdo(WDFDEVICE Device, ULONG HardwareIds, ULONG SerialNo);

#define	XBLOCK_DRIVER_ENTRY			0
#define XBLOCK1_DRIVER_ENTRY		1
#define	XRAWETH0_DRIVER_ENTRY		2
#define	XRAWETH1_DRIVER_ENTRY		3
#define	XNET_DRIVER_ENTRY			4



WCHAR	ChildDeviceIds[MAX_CHILD_DRIVER_DEVICES][MAX_PATH] = {
	{L"XDMA_V7\\XBLOCK"}, 
	{L"XDMA_V7\\XBLOCK1"},
	{L"XDMA_V7\\XRAWETH0"},
	{L"XDMA_V7\\XRAWETH1"},
	{L"XDMA_V7\\XNET"},
};


/*++
Routine Description:
 
        This function allows a Function Specific Driver (FSD) to request
        exclusive use of a DMA Engine. The function will verify the choice of
        engine, initializes the DMA Descriptors and enables interrupts. 
        Only one user is supported per engine at any given time. Incase the
        engine has already been registered with another user driver, an error
        will be returned.
 
Arguments:

    IN PREGISTER_DMA_ENGINE_REQUEST LinkReq, 
    OUT PREGISTER_DMA_ENGINE_RETURN  LinkRet

Return Value:
 
    XDMA_HANDLE - pointer to the DMA Engine context 
    
--*/
XDMA_HANDLE
XDMARegister(
    IN PREGISTER_DMA_ENGINE_REQUEST LinkReq, 
    OUT PREGISTER_DMA_ENGINE_RETURN  LinkRet
    )
{
    PDEVICE_EXTENSION       pDevExt;
    PDMA_ENGINE_EXTENSION   pDMAExt;
    XDMA_HANDLE             ReturnHandle = INVALID_XDMA_HANDLE;

    // This is our one and only sanity check to make sure we do not have a
    // bogus DMA Register request. This could be changed to a signature for more 
    // security.
    if (LinkReq->FSD_VERSION == XDMA_VERSION)
    {
        pDevExt = (PDEVICE_EXTENSION)LinkReq->XDMA_DEVICE_CONTEXT;

        // Make sure the DMA Engine requesed is in range
        if (LinkReq->DMA_ENGINE < MAX_SUPPORTED_DMA_ENGINES)
        {
            // See if we have a DMA Engine context for the requested engine
            pDMAExt = pDevExt->pDMAExt[LinkReq->DMA_ENGINE];
            if (pDMAExt != NULL)
            {
		        // In the unlikely event that two drivers are trying to bind at the same time...
				WdfSpinLockAcquire(pDMAExt->RequestLock);

                // Make sure the DMA Engine is going in the correct direction
                if (LinkReq->DIRECTION == pDMAExt->DMADirection)
                {
                    // Verify that no one else has claimed this engine.
                    if (pDMAExt->EngineState == AVAILABLE)
                    {
                        // Save the FSD context for later callbacks.
                        pDMAExt->FSDContext = LinkReq->FSD_DEVICE_CONTEXT;
                        pDMAExt->FSDCompletionFunc = LinkReq->FSDCompletion;
                        // Initialize the DMA Descriptors
                        if (InitializeDMADescriptors(pDMAExt) == STATUS_SUCCESS)
						{
							LinkRet->MAX_NUMBER_DMA_DESCRIPTORS = pDMAExt->TotalNumberDescriptors;
							LinkRet->XDMA_DMA_VERSION = XDMA_VERSION;
							LinkRet->MAXIMUM_TRANSFER_LENGTH = MAXIMUM_DMA_TRANSFER_LENGTH;
							LinkRet->DMAEnabler = pDMAExt->FSDDMAEnabler;
							LinkRet->XDMADataTransfer = &XlxDataTransfer;
							LinkRet->XDMACancelTransfer = &XlxCancelTransfers;
							LinkRet->PUSER_SPACE = (PUSER_SPACE_MAP)&pDevExt->pDMARegisters->USER_SPACE;
							LinkRet->DMA_ENGINE = LinkReq->DMA_ENGINE;
							pDMAExt->EngineState = REGISTERED;
							// The return value is the pointer to the DMA Engine context
							ReturnHandle = pDMAExt;
						}
                    }
                }
		        WdfSpinLockRelease(pDMAExt->RequestLock);
            }
        }
    }
    return ReturnHandle;
}

/**
    @doc
        This function must be called by the user driver to unregister itself from
        the base DMA driver. After doing required checks to verify the handle
        and engine state, the DMA engine is reset, interrupts are disabled if
        required, and the BD ring is freed, while returning all the packet buffers
        to the user driver.

    @param
        handle  - is the handle which was assigned during the registration
                  process.

    @return
        - 0 incase of success.
        - XST_FAILURE incase of any error.
*/
int 
XDMAUnregister(
    IN XDMA_HANDLE  UnregisterHandle
    )
{
    PDMA_ENGINE_EXTENSION   pDMAExt = (PDMA_ENGINE_EXTENSION)UnregisterHandle;
    PDEVICE_EXTENSION       pDevExt;
    LARGE_INTEGER           delay;

	pDevExt = pDMAExt->pDeviceExtension;

    // In the unlikely event that two drivers are trying to unbind at the same time...
    WdfSpinLockAcquire(pDMAExt->RequestLock);

    // Make sure that it was Bound.
    if (pDMAExt->EngineState == REGISTERED)
    {
        // Show that is is not available.
        pDMAExt->EngineState = INITIALIZING;
        // Start the Reset process, issue reset to the User Logic
        pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET_REQUEST = 1;
        // Wait 100 msec.
        delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(100);
		WdfSpinLockRelease(pDMAExt->RequestLock);
        KeDelayExecutionThread( KernelMode, TRUE, &delay );
		WdfSpinLockAcquire(pDMAExt->RequestLock);
        // Now reset the DMA Engine and wait a few mils
        pDMAExt->pDMAEngRegs->CONTROL.BIT.DMA_RESET = 1;
        delay.QuadPart =  WDF_REL_TIMEOUT_IN_MS(10);
		WdfSpinLockRelease(pDMAExt->RequestLock);
        KeDelayExecutionThread( KernelMode, TRUE, &delay );
		WdfSpinLockAcquire(pDMAExt->RequestLock);
        // Cleanup the DMA Descriptors and set to a known state.
        InitializeDMADescriptors(pDMAExt);
        // Indicate we are now available to bind.
        pDMAExt->EngineState = AVAILABLE;
    }

    WdfSpinLockRelease(pDMAExt->RequestLock);
    return 0;
}



/*++
Routine Description:
 
    This functions create the drriver to driver linkage capability
 
    This linkage is global from driver to driver. When the link
    is establish the layered driver can then request individual DMA
    Engines.
 
    Most of this code is taken from the WDK Toaster example.
 
Arguments:

    DevExt     Pointer to the Device Extension

Return Value:

     NTSTATUS

--*/
NTSTATUS
XlxCreateDriverToDriverInterface(
    IN PDEVICE_EXTENSION        pDevExt
    )
{
   	WDF_QUERY_INTERFACE_CONFIG  qiConfig;
    WDF_OBJECT_ATTRIBUTES       attributes;
    PNP_BUS_INFORMATION         busInfo;
    NTSTATUS                    status = STATUS_SUCCESS;

	RtlZeroMemory(&pDevExt->DriverLink, sizeof(REGISTER_XDRIVER));

	pDevExt->DriverLink.InterfaceHeader.Size = sizeof(REGISTER_XDRIVER);
	pDevExt->DriverLink.InterfaceHeader.Version = VER_MAJOR_NUM;
	pDevExt->DriverLink.InterfaceHeader.Context = pDevExt->Device;

	/* Let the framework handle reference counting */
	pDevExt->DriverLink.InterfaceHeader.InterfaceReference =
		WdfDeviceInterfaceReferenceNoOp;
	pDevExt->DriverLink.InterfaceHeader.InterfaceDereference =
		WdfDeviceInterfaceDereferenceNoOp;

    pDevExt->DriverLink.XDMA_DEVICE_CONTEXT = pDevExt;
    pDevExt->DriverLink.XDMA_DRV_VERSION = XDMA_VERSION;
	pDevExt->DriverLink.DmaRegister = XDMARegister;
	pDevExt->DriverLink.DmaUnregister = XDMAUnregister;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
		(PINTERFACE) &pDevExt->DriverLink,
		&GUID_V7_XDMA_INTERFACE,
		NULL);

    // Call WdfDeviceAddQueryInterface to register the XDMA Driver
	status = WdfDeviceAddQueryInterface(pDevExt->Device, &qiConfig);
	if (!NT_SUCCESS(status)) {
		DEBUGP(DEBUG_ERROR, "WdfDeviceAddQueryInterface failed 0x%x", status);
		return status;
	}

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = pDevExt->Device;
		
	/* Purpose of this lock is documented in Bus_PlugInDevice routine below.*/
    status = WdfWaitLockCreate(&attributes, &pDevExt->ChildLock);
	if (!NT_SUCCESS(status)) 
	{
		DEBUGP(DEBUG_ERROR, "WdfWaitLockCreate failed 0x%x", status);
		return status;
	}

	/* This value is used in responding to the IRP_MN_QUERY_BUS_INFORMATION
	 * for the child devices. This is an optional information provided to
	 * uniquely idenitfy the bus the device is connected.
	 */
	busInfo.BusTypeGuid = GUID_V7_XDMA_INTERFACE;
	busInfo.LegacyBusType = PNPBus;
	busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(pDevExt->Device, &busInfo);
	status = Bus_DoStaticEnumeration(pDevExt);
    return status;
}


/**
    @doc
        The routine enables you to statically enumerate child devices
		during start instead of running the enum.exe/notify.exe to
		enumerate Xilinx Function Specific drivers.

        In order to statically enumerate, user may override the
        number of Drivers in the Xilinx Bus driver's device
        registry.
    
        Most of this code is taken from the WDK Toaster example.

	@param
        Device - is our device.

	@return
        NTSTATUS.
*/
NTSTATUS
Bus_DoStaticEnumeration(
    IN PDEVICE_EXTENSION        pDevExt
    )
{
    WDFKEY      hKey = NULL;
    NTSTATUS    status;
    ULONG       value;
    DECLARE_CONST_UNICODE_STRING(valueName, L"ChildDriverConfig");

	DEBUGP(DEBUG_TRACE, "Entered Bus_DoStaticEnumeration");

	value = CHILD_DRIVER_CONFIG_DEFAULT;
    /* Open the device registry and read the "ChildDriverConfig" value.*/
    status = WdfDeviceOpenRegistryKey(pDevExt->Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      NULL, 
                                      &hKey);
    if (NT_SUCCESS (status)) 
    {
        status = WdfRegistryQueryULong(hKey, &valueName, &value);
        WdfRegistryClose(hKey);
        hKey = NULL; // Set hKey to NULL to catch any accidental subsequent use.

        if (NT_SUCCESS (status)) 
        {
            // Make sure it doesn't exceed the max. This is required to prevent
            // denial of service by enumerating large number of child devices.
            value = min(value, CHILD_DRIVER_CONFIG_DEFAULT);
        } else {
			value = CHILD_DRIVER_CONFIG_DEFAULT;
        }
    }

	DEBUGP(DEBUG_INFO, "Enumerating %d xilinx devices", value);

#if 0
    /* If we the basic TRD just set the config to two XBlocks  */
	if (pDevExt->PciCfg.DEVICE_ID == BASIC_TRD_DEVICE_ID)
	{
		value = CHILD_DRIVER_CONFIG_XBLOCK;
	}
	if (pDevExt->PciCfg.DEVICE_ID == VC709_TRD_DEVICE_ID)
	{
		value = CHILD_DRIVER_CONFIG_4_XNET;
	}
#endif

	if (value == CHILD_DRIVER_CONFIG_2_XNET)
	{
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 1);
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 2);
	}
	else if (value == CHILD_DRIVER_CONFIG_4_XNET)
	{
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 1);
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 2);
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 3);
		status = Bus_PlugInDevice(pDevExt->Device, XNET_DRIVER_ENTRY, 4);
	}
	else if (value == CHILD_DRIVER_CONFIG_XBLOCK)
	{
		status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 1);
		status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 2);
	}
	else if (value ==  CHILD_DRIVER_CONFIG_1_XBLOCK)
	{
	    status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 1);
	}
	else if (value ==  CHILD_DRIVER_CONFIG_4_XBLOCK)
	{
	    status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 1);
		status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 2);
		status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 3);
		status = Bus_PlugInDevice(pDevExt->Device, XBLOCK_DRIVER_ENTRY, 4);
	}
	else // Default config (2 xRaw)
	{
		status = Bus_PlugInDevice(pDevExt->Device, XRAWETH0_DRIVER_ENTRY, 1);
		status = Bus_PlugInDevice(pDevExt->Device, XRAWETH1_DRIVER_ENTRY, 2);
	}
    return status;
}

/**
    @doc
        The user application has told us that a new device on the bus has arrived.

		We therefore need to create a new PDO, initialize it, add it to the list
		of PDOs for this FDO bus, and then tell Plug and Play that all of this
		happened so that it will start sending prodding IRPs.
    
        Most of this code is taken from the WDK Toaster example.

	@param
        Device		- is our device.

	@param
        HardwareIds - is hardware ids of child devices.

	@param
        SerialNo	- is serial number.

	@return
        NTSTATUS.
*/

NTSTATUS
Bus_PlugInDevice(
    IN WDFDEVICE    Device,
    IN  ULONG		HardwareIndex,
    IN  ULONG       SerialNo
    )
{
    NTSTATUS         status = STATUS_SUCCESS;
    BOOLEAN          unique = TRUE;
    WDFDEVICE        hChild;
    PPDO_DEVICE_DATA pdoData;
    PDEVICE_EXTENSION pDevExt;

	DEBUGP(DEBUG_TRACE, "Entered Bus_PlugInDevice");

    /*
     * First make sure that we don't already have another device with the
     * same serial number.
     * Framework creates a collection of all the child devices we have
     * created so far. So acquire the handle to the collection and lock
     * it before walking the item.
     */
    pDevExt = XlxGetDeviceContext(Device);
    hChild = NULL;

    /*
     * We need an additional lock to synchronize addition because
     * WdfFdoLockStaticChildListForIteration locks against anyone immediately
     * updating the static child list (the changes are put on a queue until the
     * list has been unlocked).  This type of lock does not enforce our concept
     * of unique IDs on the bus (ie SerialNo).
     *
     * Without our additional lock, 2 threads could execute this function, both
     * find that the requested SerialNo is not in the list and attempt to add
     * it.  If that were to occur, 2 PDOs would have the same unique SerialNo,
     * which is incorrect.
     *
     * We must use a passive level lock because you can only call WdfDeviceCreate
     * at PASSIVE_LEVEL.
     */
    WdfWaitLockAcquire(pDevExt->ChildLock, NULL);
    WdfFdoLockStaticChildListForIteration(Device);

    while ((hChild = WdfFdoRetrieveNextStaticChild(Device,
                            hChild, WdfRetrieveAddedChildren)) != NULL) 
	{
        /*
         * WdfFdoRetrieveNextStaticChild returns reported and to be reported
         * children (ie children who have been added but not yet reported to PNP).
         */
        /* A surprise removed child will not be returned in this list.*/
         
        pdoData = PdoGetData(hChild);

        /*
         * It's okay to plug in another device with the same serial number
         * as long as the previous one is in a surprise-removed state. The
         * previous one would be in that state after the device has been
         * physically removed, if somebody has an handle open to it.
         */
        if (SerialNo == pdoData->SerialNo) 
		{
		    DEBUGP(DEBUG_TRACE, "Bus_PlugInDevice serial number matched.");
            unique = FALSE;
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (unique) 
	{
        /*
         * Create a new child device.  It is OK to create and add a child while
         * the list locked for enumeration.  The enumeration lock applies only
         * to enumeration, not addition or removal.
         */
	    DEBUGP(DEBUG_TRACE, "Bus_PlugInDevice unique is true.");
        status = Bus_CreatePdo(Device, HardwareIndex, SerialNo);
    }

    WdfFdoUnlockStaticChildListFromIteration(Device);
    WdfWaitLockRelease(pDevExt->ChildLock);
	DEBUGP(DEBUG_TRACE, "Bus_PlugInDevice unique is true.");
    return status;
}


/**
    @doc
        This routine creates and initialize a PDO.
    
        Most of this code is taken from the WDK Toaster example.

	@param
        Device		- is our device.

	@param
        HardwareIds - is hardware ids of child devices.

	@param
        SerialNo	- is serial number.

	@return
        NTSTATUS.
*/

NTSTATUS
Bus_CreatePdo(
    IN WDFDEVICE    Device,
    IN ULONG		HardwareIndex,
    IN ULONG        SerialNo
)
{
    NTSTATUS                    status;
    PWDFDEVICE_INIT             pDeviceInit = NULL;
    PPDO_DEVICE_DATA            pdoData = NULL;
    WDFDEVICE                   hChild = NULL;
    WDF_OBJECT_ATTRIBUTES       pdoAttributes;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES powerCaps;
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"XDMA V7 Bus");
    DECLARE_UNICODE_STRING_SIZE(buffer, MAX_ID_LEN);
    UNICODE_STRING deviceId;

	DEBUGP(DEBUG_TRACE, "Entered Bus_CreatePdo");

	do
	{
		/*
		* Allocate a WDFDEVICE_INIT structure and set the properties
		* so that we can create a device object for the child.
		*/
		pDeviceInit = WdfPdoInitAllocate(Device);
		if (pDeviceInit == NULL) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
    
	    /* Set DeviceType */
		WdfDeviceInitSetDeviceType(pDeviceInit, FILE_DEVICE_BUS_EXTENDER);
   
		/* Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId */
		RtlInitUnicodeString(&deviceId, ChildDeviceIds[HardwareIndex]);

		status = WdfPdoInitAssignDeviceID(pDeviceInit, &deviceId);
		if (!NT_SUCCESS(status)) 
		{
			break;
	    }
    
	    /* Note same string  is used to initialize hardware id too */
	    status = WdfPdoInitAddHardwareID(pDeviceInit, &deviceId);
	    if (!NT_SUCCESS(status)) 
		{
			break;
		}

		// Serial Number 2 is reserved for an NDIS device driver which uses
		// a different ID.
		status = WdfPdoInitAddCompatibleID(pDeviceInit, &deviceId);

		if (!NT_SUCCESS(status)) 
		{
			break;
	    }

		status =  RtlUnicodeStringPrintf(&buffer, L"%02d", SerialNo);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

	    status = WdfPdoInitAssignInstanceID(pDeviceInit, &buffer);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

	    /*
		 * Provide a description about the device. This text is usually read from
		 * the device. This text is displayed momentarily by the PnP manager while
		 * it's looking for a matching INF. If it finds one, it uses the Device
		 * Description from the INF file or the friendly name created by
		 * coinstallers to display in the device manager. FriendlyName takes
		 * precedence over the DeviceDesc from the INF file.
		 */
		status = RtlUnicodeStringPrintf(&buffer, L"XILINX_CHILD_DEVICE_%02d", SerialNo );
		if (!NT_SUCCESS(status)) 
		{
			break;
	    }

		/*
		 * You can call WdfPdoInitAddDeviceText multiple times, adding device
		 * text for multiple locales. When the system displays the text, it
		 * chooses the text that matches the current locale, if available.
		 * Otherwise it will use the string for the default locale.
		 * The driver can specify the driver's default locale by calling
		 * WdfPdoInitSetDefaultLocale.
		 */
		status = WdfPdoInitAddDeviceText(pDeviceInit, &buffer, &deviceLocation, LOCAL_ID);
		if (!NT_SUCCESS(status)) 
		{
			break;
	    }

		WdfPdoInitSetDefaultLocale(pDeviceInit, LOCAL_ID);

		/*
		 * Initialize the attributes to specify the size of PDO device extension.
		 * All the state information private to the PDO will be tracked here.
		 */
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdoAttributes, PDO_DEVICE_DATA);

	    status = WdfDeviceCreate(&pDeviceInit, &pdoAttributes, &hChild);
	    if (!NT_SUCCESS(status)) 
		{
			break;
		}

	    /*
		 * Once the device is created successfully, framework frees the
	 	 * DeviceInit memory and sets the pDeviceInit to NULL. So don't
		 * call any WdfDeviceInit functions after that.
		 */
	    /* Get the device context.*/
        pdoData = PdoGetData(hChild);
	    pdoData->SerialNo = SerialNo;
    
		/* Set some properties for the child device.*/
		WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
		pnpCaps.Removable         = WdfTrue;
		pnpCaps.EjectSupported    = WdfTrue;
		pnpCaps.SurpriseRemovalOK = WdfTrue;

		pnpCaps.Address  = SerialNo;
		pnpCaps.UINumber = SerialNo;

		WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);

		WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);
		powerCaps.DeviceD1 = WdfTrue;
		powerCaps.WakeFromD1 = WdfTrue;
		powerCaps.DeviceWake = PowerDeviceD1;

		powerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD0;
		powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
		powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
		powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
		powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
		powerCaps.DeviceState[PowerSystemShutdown] = PowerDeviceD3;

		WdfDeviceSetPowerCapabilities(hChild, &powerCaps);

	    /*
		 * Add this device to the FDO's collection of children.
	 	 * After the child device is added to the static collection successfully,
	     * driver must call WdfPdoMarkMissing to get the device deleted. It
	     * shouldn't delete the child device directly by calling WdfObjectDelete.
	     */
		status = WdfFdoAddStaticChild(Device, hChild);
	} while (FALSE);

    if (!NT_SUCCESS(status)) 
	{
        /*
		 * Call WdfDeviceInitFree if you encounter an error before the
		 * device is created. Once the device is created, framework
		 * NULLs the pDeviceInit value.
		 */
		if (pDeviceInit != NULL) 
		{
			WdfDeviceInitFree(pDeviceInit);
		}
		if (hChild) 
		{
			WdfObjectDelete(hChild);
		}
    }
    return status;
}

