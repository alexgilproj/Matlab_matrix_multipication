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

    XBlock_Main.c

Abstract:


Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XBlock_Main.tmh"
#endif // TRACE_ENABLED


// Windows XP component IDs for DbgPrintEx are different than later
//  Windows version.  We have to query the OS on start up and adjust the 
//   component ID if necessary.
ULONG	DbgComponentID	=	DPFLTR_IHVDRIVER_ID;


/*++

Routine Description:

    Driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    NTSTATUS    - if the status value is not STATUS_SUCCESS,
                        the driver will get unloaded immediately.

--*/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;
    WDF_OBJECT_ATTRIBUTES attributes;

#if TRACE_ENABLED
    // Initialize WDF WPP tracing.
    WPP_INIT_TRACING( DriverObject, RegistryPath );
#else
	RTL_OSVERSIONINFOEXW osver;
	
	osver.dwOSVersionInfoSize = sizeof(osver);
	RtlGetVersion((PRTL_OSVERSIONINFOW)&osver);
	if ((osver.dwMajorVersion == 5) && (osver.dwMinorVersion == 1))
	{
		// Adjust the ID for Windows XP.
		DbgComponentID = DPFLTR_IHVDRIVER_ID+2;	
	}
#endif // TRACE_ENABLED


    DEBUGP(DEBUG_ALWAYS, "Xilinx XBlock Function Specific Driver, Built %s %s", __DATE__, __TIME__);

    // Initialize the Driver Config structure.
    WDF_DRIVER_CONFIG_INIT( &config, XBlockEvtDeviceAdd );

	config.DriverPoolTag = 'xlXB';
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = XBlockEvtDriverContextCleanup;

    status = WdfDriverCreate( DriverObject,
                              RegistryPath,
                              &attributes,
                              &config,
                              WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "WdfDriverCreate failed with status 0x%x", status);
#if TRACE_ENABLED
		// Cleanup tracing here because DriverContextCleanup will not be called
        // as we have failed to create WDFDRIVER object itself.
        // Please note that if your return failure from DriverEntry after the
        // WDFDRIVER object is created successfully, you don't have to
        // call WPP cleanup because in those cases DriverContextCleanup
        // will be executed when the framework deletes the DriverObject.
        WPP_CLEANUP(DriverObject);
#endif // TRACE_ENABLED
	}
    return status;
}


/*++

Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. Here the driver should register all the
    PNP, power and Io callbacks, register interfaces and allocate other
    software resources required by the device. The driver can query
    any interfaces or get the config space information from the bus driver
    but cannot access hardware registers or initialize the device.

Arguments:

Return Value:

--*/
NTSTATUS
XBlockEvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
{
    NTSTATUS                   status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES       attributes;
    WDFDEVICE                   device;
    PDEVICE_EXTENSION           pDevExt = NULL;
    WDF_IO_TARGET_OPEN_PARAMS   openParams;
    PWSTR                       pSymlink = NULL;
    UNICODE_STRING              di;

    UNREFERENCED_PARAMETER( Driver );

    DEBUGP(DEBUG_TRACE,  "--> XBlockEvtDeviceAdd");

    //PAGED_CODE();

	// Setup our driver as a direct I/O device.
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    // Zero out the PnpPowerCallbacks structure.
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    // Set Callbacks for any of the functions we are interested in.
    // If no callback is set, Framework will take the default action
    // by itself.

    // These two callbacks set up and tear down hardware state that must be
    // done every time the device moves in and out of the D0-working state.
    pnpPowerCallbacks.EvtDeviceD0Entry         = XBlockEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit          = XBlockEvtDeviceD0Exit;

    // Register the PnP Callbacks..
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // Initialize Fdo Attributes.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);

    attributes.SynchronizationScope = WdfSynchronizationScopeQueue;
    // Create the device
    status = WdfDeviceCreate( &DeviceInit, &attributes, &device );

    if (!NT_SUCCESS(status)) {
        // Device Initialization failed.
        DEBUGP(DEBUG_ERROR, "DeviceCreate failed 0x%x", status);
        return status;
    }

    // Get the DeviceExtension and initialize it. XBlockGetDeviceContext is an inline function
    // defined by WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in the
    // private header file. This function will do the type checking and return
    // the device context. If you pass a wrong object a wrong object handle
    // it will return NULL and assert if run under framework verifier mode.
    pDevExt = XBlockGetDeviceContext(device);
    // Initialize the Device Extension Context
    RtlZeroMemory(pDevExt, sizeof(DEVICE_EXTENSION));
    pDevExt->Device = device;

    // Tell the Framework that this device will need an interface
    // NOTE: See the note in Public.h concerning this GUID value.
    status = WdfDeviceCreateDeviceInterface( device,
                                             (LPGUID) &GUID_V7_XBLOCK_INTERFACE,
                                             NULL );

    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "WdfDeviceCreateDeviceInterface failed 0x%x", status);
        return status;
    }

    // Initalize the Device Extension.
    status = XBlockInitializeDeviceExtension(pDevExt);
    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "XBlockInitializeDeviceExtension 0x%x", status);
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    status = WdfIoTargetCreate(device, &attributes, &pDevExt->IoTarget);
    if (!NT_SUCCESS(status)) {
        /* Device Initialization failed.*/
        DEBUGP(DEBUG_ERROR, "WdfIoTargetCreate failed 0x%x", status);
        return status;
    }

    // Look up the GUID for the XDMA driver
    status = IoGetDeviceInterfaces((LPGUID)&GUID_V7_XDMA_INTERFACE, NULL, 
                            0, &pSymlink);
    if(NT_SUCCESS(status) && (NULL != pSymlink) )
    {
        RtlInitUnicodeString(&di, pSymlink);
        DEBUGP(DEBUG_TRACE, "IoGetDeviceInterfaces success - %wZ ", &di);
    }
    else
    {
        DEBUGP(DEBUG_ERROR, "IoGetDeviceInterfaces failed 0x%x", status);
        return status;
    }

    // Open an interface to the XDMA Driver.
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams, &di, STANDARD_RIGHTS_ALL);
    status = WdfIoTargetOpen(pDevExt->IoTarget, &openParams);

    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "WdfIoTargetOpen Failed 0x%x", status);
        WdfObjectDelete(pDevExt->IoTarget);
        return status;
    }

    // Get the REGISTER_XDRIVER structure presented by the XDMA Driver.
    status = WdfIoTargetQueryForInterface(pDevExt->IoTarget , &GUID_V7_XDMA_INTERFACE, 
                        (PINTERFACE)&pDevExt->DriverLink, sizeof(REGISTER_XDRIVER), 1, NULL);
    if (!NT_SUCCESS(status)) {
        DEBUGP(DEBUG_ERROR, "WdfIoTargetQueryForInterface failed 0x%x", status);
        return status;
    }

    // One last step is to make sure the version numbers agree.
    if (pDevExt->DriverLink.XDMA_DRV_VERSION != XBLOCK_FSD_VERSION)
    {
        DEBUGP(DEBUG_ERROR, "XDMA Version does not match 0x%x", pDevExt->DriverLink.XDMA_DRV_VERSION);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

	// Link to the S2C and C2S DMA Engines
	status = XBlockInitWrite(pDevExt);
#if 1
    if (!NT_SUCCESS(status)) 
	{
	    while((pDevExt->S2CDMAEngine + 1) <  CS2_DMA_ENGINE_OFFSET)
		{
		    pDevExt->S2CDMAEngine++;
		    status = XBlockInitWrite(pDevExt);
			if(!NT_SUCCESS(status))
			{
			   continue;
			}
			else
			{
			   break;
			}
		}
	}
	if (!NT_SUCCESS(status)) 
	{
		/* Failed to initialize the Write DMA Engine */
		DEBUGP(DEBUG_ERROR, "XBlockEvtDeviceAdd: XBlockInitWrite failed 0x%x", status);
	    return STATUS_DEVICE_CONFIGURATION_ERROR;
	}
#endif

	status = XBlockInitRead(pDevExt);
#if 1
	if (!NT_SUCCESS(status)) 
	{
		while((pDevExt->C2SDMAEngine + 1) < (MAX_CS2_DMA_ENGINE + 1))
		{
	        pDevExt->C2SDMAEngine++;
		    status = XBlockInitRead(pDevExt);
			if(!NT_SUCCESS(status))
			{
			   continue;
			}
			else
			{
			   break;
			}
	    }
	}

	if (!NT_SUCCESS(status)) 
	{
	    /* Failed to initialize the Read DMA Engine */
		DEBUGP(DEBUG_ERROR, "XBlockEvtDeviceAdd: XBlockInitRead failed 0x%x", status);
	}
#endif
    DEBUGP(DEBUG_TRACE,  "<-- XBlockEvtDeviceAdd");
    return status;
}



/*++

Routine Description:

    This routine prepares the device for use.  It is called whenever the device
    enters the D0 state, which happens when the device is started, when it is
    restarted, and when it has been powered off.

    Note that interrupts will not be enabled at the time that this is called.
    They will be enabled after this callback completes.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device  - The handle to the WDF device object

    PreviousState - The state the device was in before this callback was invoked.

Return Value:

    NTSTATUS

    Success implies that the device can be used.

    Failure will result in the    device stack being torn down.

--*/
NTSTATUS
XBlockEvtDeviceD0Entry(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE PreviousState
    )
{
    PDEVICE_EXTENSION   pDevExt;
    NTSTATUS            status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(PreviousState);

    pDevExt = XBlockGetDeviceContext(Device);

	DEBUGP(DEBUG_ALWAYS, "XBlockEvtDeviceD0Entry, PreviousState %d", PreviousState);

	if (pDevExt->WriteDMAStatus != AVAILABLE)
	{
		pDevExt->S2CDMAEngine		= DEFAULT_APP;
		// Link to the S2C DMA Engines
		status = XBlockInitWrite(pDevExt);
		if (!NT_SUCCESS(status)) 
		{
			pDevExt->S2CDMAEngine++;
			if (pDevExt->S2CDMAEngine >=  CS2_DMA_ENGINE_OFFSET)
			{
				pDevExt->S2CDMAEngine = 0;
			}
			status = XBlockInitWrite(pDevExt);
		}
		if (!NT_SUCCESS(status)) 
		{
			/* Failed to initialize the Write DMA Engine */
			DEBUGP(DEBUG_ERROR, "XBlockEvtDeviceAdd: XBlockInitWrite failed 0x%x", status);
			return STATUS_DEVICE_CONFIGURATION_ERROR;
		}
	}

	if (pDevExt->ReadDMAStatus != AVAILABLE)
	{
		pDevExt->C2SDMAEngine		= DEFAULT_APP + CS2_DMA_ENGINE_OFFSET;
		// Link to the C2S DMA Engines
		status = XBlockInitRead(pDevExt);
		if (!NT_SUCCESS(status)) 
		{
			pDevExt->C2SDMAEngine++;
			if (pDevExt->C2SDMAEngine >  MAX_CS2_DMA_ENGINE)
			{
				pDevExt->C2SDMAEngine =  CS2_DMA_ENGINE_OFFSET;
			}
			status = XBlockInitRead(pDevExt);
		}
	}

	return status;
}

/*++

Routine Description:

    This routine undoes anything done in XBlockEvtDeviceD0Entry.  It is called
    whenever the device leaves the D0 state, which happens when the device
    is stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.

    Note that interrupts have already been disabled by the time that this
    callback is invoked.

Arguments:

    Device  - The handle to the WDF device object

    TargetState - The state the device will go to when this callback completes.

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
NTSTATUS
XBlockEvtDeviceD0Exit(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE TargetState
    )
{
    PDEVICE_EXTENSION   pDevExt;

    //PAGED_CODE();

    UNREFERENCED_PARAMETER(TargetState);

    pDevExt = XBlockGetDeviceContext(Device);

	DEBUGP(DEBUG_ALWAYS, "XBlockEvtDeviceD0Exit, TargetState %d", TargetState);

	if (pDevExt->WriteDMAStatus != SHUTDOWN)
	{
		if (pDevExt->DMAWriteHandle != INVALID_XDMA_HANDLE)
		{
			DEBUGP(DEBUG_ALWAYS, "XBlockEvtDeviceD0Exit, Shutting down Write DMA");
			pDevExt->WriteDMAStatus = SHUTDOWN;
			pDevExt->DriverLink.DmaUnregister(pDevExt->DMAWriteHandle);
			pDevExt->DMAWriteHandle = INVALID_XDMA_HANDLE;
		}
	}

	if (pDevExt->ReadDMAStatus != SHUTDOWN)
	{
		if (pDevExt->DMAReadHandle != INVALID_XDMA_HANDLE)
		{
			DEBUGP(DEBUG_ALWAYS, "XBlockEvtDeviceD0Exit, Shutting down Read DMA");
			pDevExt->ReadDMAStatus = SHUTDOWN;
			pDevExt->DriverLink.DmaUnregister(pDevExt->DMAReadHandle);
			pDevExt->DMAReadHandle = INVALID_XDMA_HANDLE;
		}
	}


    return STATUS_SUCCESS;
}



/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
VOID
XBlockEvtDriverContextCleanup(
    IN WDFDRIVER Driver
    )
{
    //PAGED_CODE ();

    DEBUGP(DEBUG_TRACE, "XlxEvtDriverContextCleanup: enter");
	
	UNREFERENCED_PARAMETER(Driver);

#if TRACE_ENABLED
	WPP_CLEANUP( WdfDriverWdmGetDriverObject( Driver ) );
#endif // TRACE_ENABLED
}




