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
* @file XNet_Main.c
*
* This file contains the main NDIS MiniPort driver functions.
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
#include "XNet_Main.tmh"
#endif // TRACE_ENABLED

NDIS_STATUS
DriverEntry(
    __in  PVOID DriverObject,
    __in  PVOID RegistryPath);


#pragma NDIS_INIT_FUNCTION(DriverEntry)
#pragma NDIS_PAGEABLE_FUNCTION(DriverUnload)
#pragma NDIS_PAGEABLE_FUNCTION(MPInitializeEx)
#pragma NDIS_PAGEABLE_FUNCTION(MPPause)
#pragma NDIS_PAGEABLE_FUNCTION(MPRestart)
#pragma NDIS_PAGEABLE_FUNCTION(MPHaltEx)
#pragma NDIS_PAGEABLE_FUNCTION(MPDevicePnpEventNotify)


NDIS_HANDLE     NdisDriverHandle;
NDIS_HANDLE     GlobalDriverContext = NULL;

// Windows XP component IDs for DbgPrintEx are different than later
//  Windows version.  We have to query the OS on start up and adjust the 
//   component ID if necessary.
ULONG	DbgComponentID	=	DPFLTR_IHVNETWORK_ID;

/*++
Routine Description:

    In the context of its DriverEntry function, a miniport driver associates
    itself with NDIS, specifies the NDIS version that it is using, and
    registers its entry points.


Arguments:
    PVOID DriverObject - pointer to the driver object.
    PVOID RegistryPath - pointer to the driver registry path.

    Return Value:

    NTSTATUS code

--*/
NDIS_STATUS
DriverEntry(
    __in  PVOID DriverObject,
    __in  PVOID RegistryPath)
{
    NDIS_STATUS			Status;
    NDIS_MINIPORT_DRIVER_CHARACTERISTICS MPChar;
    WDF_DRIVER_CONFIG   config;
    WDFDRIVER			hDriver;

#if TRACE_ENABLED
    WPP_INIT_TRACING(DriverObject,RegistryPath);
#else
	RTL_OSVERSIONINFOEXW osver;
	
	osver.dwOSVersionInfoSize = sizeof(osver);
	RtlGetVersion((PRTL_OSVERSIONINFOW)&osver);
	if ((osver.dwMajorVersion == 5) && (osver.dwMinorVersion == 1))
	{
		// Adjust the ID for Windows XP.
		DbgComponentID = DPFLTR_IHVNETWORK_ID+2;	
	}
#endif // TRACE_ENABLED

	DEBUGP(DEBUG_ALWAYS, "Xilinx XNet Function Specific Driver: Built %s %s", __DATE__, __TIME__);
    PAGED_CODE();

	DEBUGP(DEBUG_ERROR, "Debug Error level is enabled");
	DEBUGP(DEBUG_WARN, "Debug Warning level is enabled");
	DEBUGP(DEBUG_TRACE, "Debug Trace level is enabled");
	DEBUGP(DEBUG_INFO, "Debug Info level is enabled");
	DEBUGP(DEBUG_VERBOSE, "Debug Verbose level is enabled");

    do
    {
		WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

	    /*
		 * Set WdfDriverInitNoDispatchOverride flag to tell the framework
	     * not to provide dispatch routines for the driver. In other words,
	     * the framework must not intercept IRPs that the I/O manager has
	     * directed to the driver. In this case, it will be handled by NDIS
	     * miniport driver.
	     */
	    config.DriverInitFlags |= WdfDriverInitNoDispatchOverride;
	    Status = WdfDriverCreate(DriverObject, RegistryPath,
			                       WDF_NO_OBJECT_ATTRIBUTES, &config, &hDriver);
		if (!NT_SUCCESS(Status))
		{
			DEBUGP(DEBUG_ERROR, "WdfDriverCreate Failed 0x%x ", Status);
			Status = NDIS_STATUS_FAILURE;
			break;
		}

        //
        // Fill in the Miniport characteristics structure with the version numbers
        // and the entry points for driver-supplied MiniportXxx
        //

        NdisZeroMemory(&MPChar, sizeof(MPChar));

#if defined(NDIS60_MINIPORT)
        {C_ASSERT(sizeof(MPChar) >= NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1);}
        MPChar.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
        MPChar.Header.Size = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1;
        MPChar.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_1;
#elif defined(NDIS620_MINIPORT)
        {C_ASSERT(sizeof(MPChar) >= NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2);}
        MPChar.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
        MPChar.Header.Size = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
        MPChar.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
#else
#   error Unsupported miniport version
#endif // NDIS MINIPORT VERSION

        MPChar.MajorNdisVersion = MP_NDIS_MAJOR_VERSION;
        MPChar.MinorNdisVersion = MP_NDIS_MINOR_VERSION;

        MPChar.MajorDriverVersion = NIC_MAJOR_DRIVER_VERSION;
        MPChar.MinorDriverVersion = NIC_MINOR_DRIVER_VERISON;

        MPChar.Flags = 0;

        MPChar.SetOptionsHandler = MPSetOptions; // Optional
        MPChar.InitializeHandlerEx = MPInitializeEx;
        MPChar.HaltHandlerEx = MPHaltEx;
        MPChar.UnloadHandler = DriverUnload;
        MPChar.PauseHandler = MPPause;
        MPChar.RestartHandler = MPRestart;
        MPChar.OidRequestHandler = MPOidRequest;
        MPChar.SendNetBufferListsHandler = MPSendNetBufferLists;
        MPChar.ReturnNetBufferListsHandler = MPReturnNetBufferLists;
        MPChar.CancelSendHandler = MPCancelSend;
        MPChar.CheckForHangHandlerEx = MPCheckForHangEx;
        MPChar.ResetHandlerEx = MPResetEx;
        MPChar.DevicePnPEventNotifyHandler = MPDevicePnpEventNotify;
        MPChar.ShutdownHandlerEx = MPShutdownEx;
        MPChar.CancelOidRequestHandler = MPCancelOidRequest;

        //
        // Associate the miniport driver with NDIS by calling the
        // NdisMRegisterMiniportDriver. This function returns an NdisDriverHandle.
        // The miniport driver must retain this handle but it should never attempt
        // to access or interpret this handle.
        //
        // By calling NdisMRegisterMiniportDriver, the driver indicates that it
        // is ready for NDIS to call the driver's MiniportSetOptions and
        // MiniportInitializeEx handlers.
        //
        DEBUGP(DEBUG_VERBOSE, "Calling NdisMRegisterMiniportDriver...");
        Status = NdisMRegisterMiniportDriver(
                DriverObject,
                RegistryPath,
                &GlobalDriverContext,
                &MPChar,
                &NdisDriverHandle);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(DEBUG_ERROR, "NdisMRegisterMiniportDriver failed: %d", Status);
            DriverUnload(DriverObject);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

    } while(FALSE);

    DEBUGP(DEBUG_TRACE, "<--- DriverEntry Status 0x%08x", Status);
    return Status;
}


/*++

Routine Description:

    The unload handler is called during driver unload to free up resources
    acquired in DriverEntry. This handler is registered in DriverEntry through
    NdisMRegisterMiniportDriver. Note that an unload handler differs from
    a MiniportHalt function in that this unload handler releases resources that
    are global to the driver, while the halt handler releases resource for a
    particular adapter.

    Runs at IRQL = PASSIVE_LEVEL.

Arguments:

    DriverObject        Not used

Return Value:

    None.

--*/
VOID
DriverUnload(
    __in  PDRIVER_OBJECT  DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
    DEBUGP(DEBUG_TRACE, "---> DriverUnload");
    PAGED_CODE();

    //
    // Since DriverEntry has successfully called NdisMRegisterMiniportDriver,
    // NdisMDeregisterMiniportDriver must be called to release NDIS's per-driver
    // resources.
    //
    DEBUGP(DEBUG_VERBOSE, "Calling NdisMDeregisterMiniportDriver...");
    NdisMDeregisterMiniportDriver(NdisDriverHandle);

#if TRACE_ENABLED
    WPP_CLEANUP(DriverObject);
#endif // TRACE_ENABLED

    DEBUGP(DEBUG_TRACE, "<--- DriverUnload");
}


/*++
Routine Description:

    The MiniportSetOptions function registers optional handlers.  For each
    optional handler that should be registered, this function makes a call
    to NdisSetOptionalHandlers.

    MiniportSetOptions runs at IRQL = PASSIVE_LEVEL.

Arguments:

    DriverContext  The context handle

Return Value:

    NDIS_STATUS_xxx code

--*/
NDIS_STATUS
MPSetOptions(
    __in  NDIS_HANDLE  DriverHandle,
    __in  NDIS_HANDLE  DriverContext)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
//    PMP_GLOBAL Global = (PMP_GLOBAL)DriverContext;

    DEBUGP(DEBUG_TRACE, "---> MPSetOptions");
    UNREFERENCED_PARAMETER(DriverHandle);
    UNREFERENCED_PARAMETER(DriverContext);

    //
    // Set any optional handlers by filling out the appropriate struct and
    // calling NdisSetOptionalHandlers here.
    //
    DEBUGP(DEBUG_TRACE, "<--- MPSetOptions Status = 0x%08x", Status);
    return Status;
}



/*++
Routine Description:

    The MiniportInitialize function is a required function that sets up a
    NIC (or virtual NIC) for network I/O operations, claims all hardware
    resources necessary to the NIC in the registry, and allocates resources
    the driver needs to carry out network I/O operations.

    MiniportInitialize runs at IRQL = PASSIVE_LEVEL.

Arguments:

    Return Value:

    NDIS_STATUS_xxx code

--*/
NDIS_STATUS
MPInitializeEx(
    __in  NDIS_HANDLE MiniportAdapterHandle,
    __in  NDIS_HANDLE MiniportDriverContext,
    __in  PNDIS_MINIPORT_INIT_PARAMETERS MiniportInitParameters)
{
    PMP_ADAPTER			Adapter = NULL;
    USHORT				usInfo;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;

    DEBUGP(DEBUG_TRACE, "---> MPInitializeEx");
    UNREFERENCED_PARAMETER(MiniportDriverContext);
    UNREFERENCED_PARAMETER(MiniportInitParameters);
    PAGED_CODE();

    do
    {
        NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES AdapterRegistration;
        NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES AdapterGeneral;
    
#if defined(NDIS60_MINIPORT)
        NDIS_PNP_CAPABILITIES PnpCapabilities;
#elif defined(NDIS620_MINIPORT)
        NDIS_PM_CAPABILITIES PmCapabilities;
#else
#   error Unsupported miniport version
#endif // NDIS MINIPORT VERSION

        //
        // Allocate adapter context structure and initialize all the
        // memory resources for sending and receiving packets.
        //
        Status = NICAllocAdapter(MiniportAdapterHandle, &Adapter);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

		DEBUGP(DEBUG_TRACE, "MPInitializeEx Adapter allocated. Adapter Addr: 0x%p", Adapter);

        //
        // First, set the registration attributes.
        //

        NdisZeroMemory(&AdapterRegistration, sizeof(AdapterRegistration));

        {C_ASSERT(sizeof(AdapterRegistration) >= NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1);}
        AdapterRegistration.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        AdapterRegistration.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
        AdapterRegistration.Header.Revision = NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;

        AdapterRegistration.MiniportAdapterContext = Adapter;
        AdapterRegistration.AttributeFlags = NIC_ADAPTER_ATTRIBUTES_FLAGS;
        AdapterRegistration.CheckForHangTimeInSeconds = NIC_ADAPTER_CHECK_FOR_HANG_TIME_IN_SECONDS;
        AdapterRegistration.InterfaceType = NIC_INTERFACE_TYPE;

        Status = NdisMSetMiniportAttributes(
                MiniportAdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterRegistration);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(DEBUG_ERROR, "NdisSetMiniportAttributes Status 0x%08x", Status);
            break;
        }

        //
        // Read Advanced configuration information from the registry
		// Before linking to the DMA Engines.
        //
		NICReadRegParameters(Adapter);

		Status = XNetLinkToXDMA(MiniportAdapterHandle, Adapter);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DEBUG_ERROR, "XNetLinkToXDMA Status 0x%08x", Status);
            break;
        }

        //
        // Initialize the default receive block
        //
		Status = AllocRxDMA(Adapter, DEFAULT_NUMBER_RX_RECEIVES);
        if(Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DEBUG_ERROR, "AllocRxDMA Status 0x%08x", Status);
            break;
        }

		// Now that we have established a link to the DMA Driver, start up the MAC.
		XXgEthernet_Reset(Adapter);

		// Get the MAC Address from the hardware
		XXgEthernet_GetMacAddress(Adapter, &Adapter->PermanentAddress);
	    DEBUGP(DEBUG_ALWAYS, "Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x", 
		    Adapter->PermanentAddress[0],
	        Adapter->PermanentAddress[1],
			Adapter->PermanentAddress[2],
			Adapter->PermanentAddress[3],
			Adapter->PermanentAddress[4],
			Adapter->PermanentAddress[5]);

		if (Adapter->MACAddressOverride)
		{
			// Write the override MAC address back to the card.
			XXgEthernet_SetMacAddress(Adapter,  &Adapter->CurrentAddress);
		}
		else
		{
			XXgEthernet_GetMacAddress(Adapter, &Adapter->CurrentAddress);
		}
	    DEBUGP(DEBUG_ALWAYS, "Current Address = %02x-%02x-%02x-%02x-%02x-%02x", 
		    Adapter->CurrentAddress[0],
	        Adapter->CurrentAddress[1],
		    Adapter->CurrentAddress[2],
	        Adapter->CurrentAddress[3],
			Adapter->CurrentAddress[4],
			Adapter->CurrentAddress[5]);

        //
        // Next, set the general attributes.
        //
        NdisZeroMemory(&AdapterGeneral, sizeof(AdapterGeneral));

#if defined(NDIS60_MINIPORT)
        {C_ASSERT(sizeof(AdapterGeneral) >= NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1);}
        AdapterGeneral.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        AdapterGeneral.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
        AdapterGeneral.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
#elif defined(NDIS620_MINIPORT)
        {C_ASSERT(sizeof(AdapterGeneral) >= NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2);}
        AdapterGeneral.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        AdapterGeneral.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
        AdapterGeneral.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_2;
#else
#   error Unsupported miniport version
#endif // NDIS MINIPORT VERSION

        //
        // Specify the medium type that the NIC can support but not
        // necessarily the medium type that the NIC currently uses.
        //
        AdapterGeneral.MediaType = NIC_MEDIUM_TYPE;

        //
        // Specifiy medium type that the NIC currently uses.
        //
        AdapterGeneral.PhysicalMediumType = NIC_PHYSICAL_MEDIUM;

        //
        // Specifiy the maximum network frame size, in bytes, that the NIC
        // supports excluding the header. A NIC driver that emulates another
        // medium type for binding to a transport must ensure that the maximum
        // frame size for a protocol-supplied net buffer does not exceed the
        // size limitations for the true network medium.
        //
        AdapterGeneral.MtuSize = HW_FRAME_MAX_DATA_SIZE;
        AdapterGeneral.MaxXmitLinkSpeed = Adapter->ulLinkSendSpeed;
        AdapterGeneral.XmitLinkSpeed = Adapter->ulLinkSendSpeed;
        AdapterGeneral.MaxRcvLinkSpeed = Adapter->ulLinkRecvSpeed;
        AdapterGeneral.RcvLinkSpeed = Adapter->ulLinkRecvSpeed;

		XXgEthernet_PhyRead(Adapter, Adapter->PhyAddress, XXGE_MDIO_REGISTER_ADDRESS, &usInfo);
		if (usInfo & XXGE_MDIO_PHY_LINK_UP_MASK)
		{
	        AdapterGeneral.MediaConnectState = MediaConnectStateConnected;
			Adapter->LinkState = MediaConnectStateConnected;
			DEBUGP(DEBUG_INFO, "Connected\n");
		}
		else
		{
	        AdapterGeneral.MediaConnectState = MediaConnectStateDisconnected;
			Adapter->LinkState = MediaConnectStateDisconnected;
			DEBUGP(DEBUG_WARN, "Disconnected\n");
		}

        AdapterGeneral.MediaDuplexState = MediaDuplexStateFull;

        //
        // The maximum number of bytes the NIC can provide as lookahead data.
        // If that value is different from the size of the lookahead buffer
        // supported by bound protocols, NDIS will call MiniportOidRequest to
        // set the size of the lookahead buffer provided by the miniport driver
        // to the minimum of the miniport driver and protocol(s) values. If the
        // driver always indicates up full packets with
        // NdisMIndicateReceiveNetBufferLists, it should set this value to the
        // maximum total frame size, which excludes the header.
        //
        // Upper-layer drivers examine lookahead data to determine whether a
        // packet that is associated with the lookahead data is intended for
        // one or more of their clients. If the underlying driver supports
        // multipacket receive indications, bound protocols are given full net
        // packets on every indication. Consequently, this value is identical
        // to that returned for OID_GEN_RECEIVE_BLOCK_SIZE.
        //
        AdapterGeneral.LookaheadSize = Adapter->ulLookahead;
        AdapterGeneral.MacOptions = NIC_MAC_OPTIONS;
        AdapterGeneral.SupportedPacketFilters = NIC_SUPPORTED_FILTERS;

        //
        // The maximum number of multicast addresses the NIC driver can manage.
        // This list is global for all protocols bound to (or above) the NIC.
        // Consequently, a protocol can receive NDIS_STATUS_MULTICAST_FULL from
        // the NIC driver when attempting to set the multicast address list,
        // even if the number of elements in the given list is less than the
        // number originally returned for this query.
        //
        AdapterGeneral.MaxMulticastListSize = NIC_MAX_MCAST_LIST;
        AdapterGeneral.MacAddressLength = NIC_MACADDR_SIZE;

        //
        // Return the MAC address of the NIC burnt in the hardware.
        //
        NIC_COPY_ADDRESS(AdapterGeneral.PermanentMacAddress, Adapter->PermanentAddress);

        //
        // Return the MAC address the NIC is currently programmed to use. Note
        // that this address could be different from the permananent address as
        // the user can override using registry. Read NdisReadNetworkAddress
        // doc for more info.
        //
        NIC_COPY_ADDRESS(AdapterGeneral.CurrentMacAddress, Adapter->CurrentAddress);
        AdapterGeneral.RecvScaleCapabilities = NULL;
        AdapterGeneral.AccessType = NIC_ACCESS_TYPE;
        AdapterGeneral.DirectionType = NIC_DIRECTION_TYPE;
        AdapterGeneral.ConnectionType = NIC_CONNECTION_TYPE;
        AdapterGeneral.IfType = NIC_IFTYPE;
        AdapterGeneral.IfConnectorPresent = NIC_HAS_PHYSICAL_CONNECTOR;
        AdapterGeneral.SupportedStatistics = NIC_SUPPORTED_STATISTICS;
        AdapterGeneral.SupportedPauseFunctions = NdisPauseFunctionsUnsupported;
        AdapterGeneral.DataBackFillSize = 0;
        AdapterGeneral.ContextBackFillSize = 0;

        //
        // The SupportedOidList is an array of OIDs for objects that the
        // underlying driver or its NIC supports.  Objects include general,
        // media-specific, and implementation-specific objects. NDIS forwards a
        // subset of the returned list to protocols that make this query. That
        // is, NDIS filters any supported statistics OIDs out of the list
        // because protocols never make statistics queries.
        //
        AdapterGeneral.SupportedOidList = NICSupportedOids;
//        AdapterGeneral.SupportedOidListLength = sizeof(NICSupportedOids);
        AdapterGeneral.AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;

        //
        // Set the power management capabilities.  The format used is NDIS
        // version-specific.
        //
		//  This MAC does not support any of these features
		//
#if defined(NDIS60_MINIPORT)
        NdisZeroMemory(&PnpCapabilities, sizeof(PnpCapabilities));
        PnpCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp = NIC_MAGIC_PACKET_WAKEUP;
        PnpCapabilities.WakeUpCapabilities.MinPatternWakeUp = NIC_PATTERN_WAKEUP;
        AdapterGeneral.PowerManagementCapabilities = &PnpCapabilities; // Optional

#elif defined(NDIS620_MINIPORT)

        NdisZeroMemory(&PmCapabilities, sizeof(PmCapabilities));

        {C_ASSERT(sizeof(PmCapabilities) >= NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1);}
        PmCapabilities.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PmCapabilities.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1;
        PmCapabilities.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_1;

        PmCapabilities.MinMagicPacketWakeUp = NIC_MAGIC_PACKET_WAKEUP;
        PmCapabilities.MinPatternWakeUp = NIC_PATTERN_WAKEUP;
        PmCapabilities.MinLinkChangeWakeUp = NIC_LINK_CHANGE_WAKEUP;

        AdapterGeneral.PowerManagementCapabilitiesEx = &PmCapabilities;
#else
#   error Unsupported miniport version
#endif // NDIS MINIPORT VERSION

        Status = NdisMSetMiniportAttributes(
                MiniportAdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&AdapterGeneral);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DEBUGP(DEBUG_ERROR, "NdisSetOptionalHandlers Status 0x%08x", Status);
            break;
        }
    } while(FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        if (Adapter)
        {
            NICFreeAdapter(Adapter);
        }
        Adapter = NULL;
    }

    DEBUGP(DEBUG_TRACE, "<--- MPInitializeEx Status = 0x%08x%", Status);
    return Status;
}


NDIS_STATUS
MPPause(
    __in  NDIS_HANDLE  MiniportAdapterContext,
    __in  PNDIS_MINIPORT_PAUSE_PARAMETERS MiniportPauseParameters)
/*++

Routine Description:

    When a miniport receives a pause request, it enters into a Pausing state.
    The miniport should not indicate up any more network data.  Any pending
    send requests must be completed, and new requests must be rejected with
    NDIS_STATUS_PAUSED.

    Once all sends have been completed and all recieve NBLs have returned to
    the miniport, the miniport enters the Paused state.

    While paused, the miniport can still service interrupts from the hardware
    (to, for example, continue to indicate NDIS_STATUS_MEDIA_CONNECT
    notifications).

    The miniport must continue to be able to handle status indications and OID
    requests.  MiniportPause is different from MiniportHalt because, in
    general, the MiniportPause operation won't release any resources.
    MiniportPause must not attempt to acquire any resources where allocation
    can fail, since MiniportPause itself must not fail.


    MiniportPause runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    MiniportPauseParameters  Additional information about the pause operation

Return Value:

    If the miniport is able to immediately enter the Paused state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport must wait for send completions or pending receive NBLs, it
    should return NDIS_STATUS_PENDING now, and call NDISMPauseComplete when the
    miniport has entered the Paused state.

    No other return value is permitted.  The pause operation must not fail.

--*/
{
    NDIS_STATUS Status;

    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPPause");
    UNREFERENCED_PARAMETER(MiniportPauseParameters);
    PAGED_CODE();

    MP_SET_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS);

    if (Adapter->nBusySend)
    {
        //
        // The adapter is busy sending or receiving data, so the pause must
        // be completed asynchronously later.
        //
        Status = NDIS_STATUS_PENDING;
    }
    else // !NICIsBusy
    {
        //
        // The pause operation has completed synchronously.
        //
        MP_SET_FLAG(Adapter, fMP_ADAPTER_PAUSED);
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_PAUSE_IN_PROGRESS);
        Status = NDIS_STATUS_SUCCESS;
    }

    DEBUGP(DEBUG_TRACE, "<--- MPPause Status = 0x%08x", Status);
    return Status;
}


NDIS_STATUS
MPRestart(
    __in  NDIS_HANDLE                             MiniportAdapterContext,
    __in  PNDIS_MINIPORT_RESTART_PARAMETERS       RestartParameters)
/*++

Routine Description:

    When a miniport receives a restart request, it enters into a Restarting
    state.  The miniport may begin indicating received data (e.g., using
    NdisMIndicateReceiveNetBufferLists), handling status indications, and
    processing OID requests in the Restarting state.  However, no sends will be
    requested while the miniport is in the Restarting state.

    Once the miniport is ready to send data, it has entered the Running state.
    The miniport informs NDIS that it is in the Running state by returning
    NDIS_STATUS_SUCCESS from this MiniportRestart function; or if this function
    has already returned NDIS_STATUS_PENDING, by calling NdisMRestartComplete.


    MiniportRestart runs at IRQL = PASSIVE_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    RestartParameters  Additional information about the restart operation

Return Value:

    If the miniport is able to immediately enter the Running state, it should
    return NDIS_STATUS_SUCCESS.

    If the miniport is still in the Restarting state, it should return
    NDIS_STATUS_PENDING now, and call NdisMRestartComplete when the miniport
    has entered the Running state.

    Other NDIS_STATUS codes indicate errors.  If an error is encountered, the
    miniport must return to the Paused state (i.e., stop indicating receives).

--*/

{
    NDIS_STATUS Status = NDIS_STATUS_PENDING;

    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPRestart");
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(RestartParameters);
    PAGED_CODE();

	// Write the override MAC address back to the card.
	XXgEthernet_SetMacAddress(Adapter,  &Adapter->CurrentAddress);
	// Set the options
	XXgEthernet_SetOptions(Adapter, Adapter->Options);

	// Finally start the ethernet MAC
	XXgEthernet_Start(Adapter);

    MP_CLEAR_FLAG(Adapter, (fMP_ADAPTER_PAUSE_IN_PROGRESS|fMP_ADAPTER_PAUSED));

    //
    // The simulated hardware is immediately ready to send data again, so in
    // this sample code, MiniportRestart always returns success.  If we had to
    // wait for hardware to reinitialize, we'd return NDIS_STATUS_PENDING now,
    // and call NdisMRestartComplete later.
    //
    Status = NDIS_STATUS_SUCCESS;

    DEBUGP(DEBUG_TRACE, "<--- MPRestart Status = 0x%08x", Status);
    return Status;
}


VOID
MPHaltEx(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_HALT_ACTION HaltAction
    )
/*++

Routine Description:

    Halt handler is called when NDIS receives IRP_MN_STOP_DEVICE,
    IRP_MN_SUPRISE_REMOVE or IRP_MN_REMOVE_DEVICE requests from the PNP
    manager. Here, the driver should free all the resources acquired in
    MiniportInitialize and stop access to the hardware. NDIS will not submit
    any further request once this handler is invoked.

    1) Free and unmap all I/O resources.
    2) Disable interrupt and deregister interrupt handler.
    3) Deregister shutdown handler regsitered by
        NdisMRegisterAdapterShutdownHandler .
    4) Cancel all queued up timer callbacks.
    5) Finally wait indefinitely for all the outstanding receive
        packets indicated to the protocol to return.

    MiniportHalt runs at IRQL = PASSIVE_LEVEL.


Arguments:

    MiniportAdapterContext  Pointer to the Adapter
    HaltAction  The reason for halting the adapter

Return Value:

    None.

--*/
{
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    LONG              nHaltCount = 0;


    PAGED_CODE();
    DEBUGP(DEBUG_TRACE, "---> MPHaltEx");
    UNREFERENCED_PARAMETER(HaltAction);

    MP_SET_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS);

    //
    // Call Shutdown handler to disable interrupt and turn the hardware off by
    // issuing a full reset
    //
    // On XP and later, NDIS notifies our PNP event handler the reason for
    // calling Halt. So before accessing the device, check to see if the device
    // is surprise removed, if so don't bother calling the shutdown handler to
    // stop the hardware because it doesn't exist.
    //
	XXgEthernet_Stop(Adapter);

//    if(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_SURPRISE_REMOVED)) 
//	{
        MPShutdownEx(MiniportAdapterContext, NdisShutdownPowerOff);
//	}  

    while (Adapter->nBusySend)
    {
        if (++nHaltCount % 100)
        {
            DEBUGP(DEBUG_ERROR, "Halt timed out!");
            ASSERT(FALSE);
        }

        DEBUGP(DEBUG_INFO, "MPHaltEx - waiting ...");
        NdisMSleep(1000);
    }

    NICFreeAdapter(Adapter);

    DEBUGP(DEBUG_TRACE, "<--- MPHaltEx");
}


NDIS_STATUS
MPResetEx(
    __in  NDIS_HANDLE MiniportAdapterContext,
    __out PBOOLEAN AddressingReset)
/*++

Routine Description:

    MiniportResetEx is a required to issue a hardware reset to the NIC
    and/or to reset the driver's software state.

    1) The miniport driver can optionally complete any pending
        OID requests. NDIS will submit no further OID requests
        to the miniport driver for the NIC being reset until
        the reset operation has finished. After the reset,
        NDIS will resubmit to the miniport driver any OID requests
        that were pending but not completed by the miniport driver
        before the reset.
        
    2) A deserialized miniport driver must complete any pending send
        operations. NDIS will not requeue pending send packets for
        a deserialized driver since NDIS does not maintain the send
        queue for such a driver.

    3) If MiniportReset returns NDIS_STATUS_PENDING, the driver must
        complete the original request subsequently with a call to
        NdisMResetComplete.

    MiniportReset runs at IRQL <= DISPATCH_LEVEL.

Arguments:

AddressingReset - If multicast or functional addressing information
                  or the lookahead size, is changed by a reset,
                  MiniportReset must set the variable at AddressingReset
                  to TRUE before it returns control. This causes NDIS to
                  call the MiniportSetInformation function to restore
                  the information.

MiniportAdapterContext - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS       Status;
    PMP_ADAPTER       Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPResetEx");

    do
    {
        ASSERT(!MP_TEST_FLAG(Adapter, fMP_ADAPTER_HALT_IN_PROGRESS));

        if (MP_TEST_FLAG(Adapter, fMP_RESET_IN_PROGRESS))
        {
            Status = NDIS_STATUS_RESET_IN_PROGRESS;
            break;
        }

        MP_SET_FLAG(Adapter, fMP_RESET_IN_PROGRESS);

        //
        // Complete all the queued up send packets
        //
//        TXFlushSendQueue(Adapter, NDIS_STATUS_RESET_IN_PROGRESS);
//        if (Adapter->nBusySend)
//        {
            //
            // By returning NDIS_STATUS_PENDING, we are promising NDIS that
            // we will complete the reset request by calling NdisMResetComplete.
            //
//            Status = NDIS_STATUS_PENDING;
//            break;
//        }

		XXgEthernet_Stop(Adapter);

        *AddressingReset = FALSE;
        MP_CLEAR_FLAG(Adapter, fMP_RESET_IN_PROGRESS);

        Status = NDIS_STATUS_SUCCESS;

    } while(FALSE);


    DEBUGP(DEBUG_TRACE, "<--- MPResetEx Status = 0x%08x", Status);
    return Status;
}

VOID
MPShutdownEx(
    __in  NDIS_HANDLE           MiniportAdapterContext,
    __in  NDIS_SHUTDOWN_ACTION  ShutdownAction)
/*++

Routine Description:

    The MiniportShutdownEx handler restores hardware to its initial state when
    the system is shut down, whether by the user or because an unrecoverable
    system error occurred. This is to ensure that the NIC is in a known
    state and ready to be reinitialized when the machine is rebooted after
    a system shutdown occurs for any reason, including a crash dump.

    Here just disable the interrupt and stop the DMA engine.  Do not free
    memory resources or wait for any packet transfers to complete.  Do not call
    into NDIS at this time.

    This can be called at aribitrary IRQL, including in the context of a
    bugcheck.

Arguments:

    MiniportAdapterContext  Pointer to our adapter
    ShutdownAction  The reason why NDIS called the shutdown function

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(ShutdownAction);
    UNREFERENCED_PARAMETER(Adapter);

    DEBUGP(DEBUG_TRACE, "---> MPShutdownEx");

	// Shut down the XDMA DMA engines we are using.
	XNetShutdown(Adapter);

    DEBUGP(DEBUG_TRACE, "<--- MPShutdownEx");
}


BOOLEAN
MPCheckForHangEx(
    __in  NDIS_HANDLE MiniportAdapterContext)
/*++

Routine Description:

    The MiniportCheckForHangEx handler is called to report the state of the
    NIC, or to monitor the responsiveness of an underlying device driver.
    This is an optional function. If this handler is not specified, NDIS
    judges the driver unresponsive when the driver holds
    MiniportQueryInformation or MiniportSetInformation requests for a
    time-out interval (deafult 4 sec), and then calls the driver's
    MiniportReset function. A NIC driver's MiniportInitialize function can
    extend NDIS's time-out interval by calling NdisMSetAttributesEx to
    avoid unnecessary resets.

    MiniportCheckForHangEx runs at IRQL <= DISPATCH_LEVEL.

Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    TRUE    NDIS calls the driver's MiniportReset function.
    FALSE   Everything is fine

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);
	USHORT		usInfo;

    DEBUGP(DEBUG_TRACE, "---> MPCheckForHangEx");

	// Check the Link State while we are here
	XXgEthernet_PhyRead(Adapter, Adapter->PhyAddress, XXGE_MDIO_REGISTER_ADDRESS, &usInfo);
	if (usInfo & XXGE_MDIO_PHY_LINK_UP_MASK)
	{
		if (Adapter->LinkState != MediaConnectStateConnected)
		{
			NICIndicateNewLinkSpeed(Adapter, Adapter->ulLinkSendSpeed, MediaConnectStateConnected);
			Adapter->LinkState = MediaConnectStateConnected;
			DEBUGP(DEBUG_INFO, "Link Connected\n");
		}
	}
	else
	{
		if (Adapter->LinkState != MediaConnectStateDisconnected)
		{
			NICIndicateNewLinkSpeed(Adapter, Adapter->ulLinkSendSpeed, MediaConnectStateDisconnected);
	        Adapter->LinkState = MediaConnectStateDisconnected;
			DEBUGP(DEBUG_WARN, "Link Disconnected\n");
		}
	}

    return FALSE;
}


VOID
MPDevicePnpEventNotify(
    __in  NDIS_HANDLE             MiniportAdapterContext,
    __in  PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent)
/*++

Routine Description:

    Runs at IRQL = PASSIVE_LEVEL in the context of system thread.

Arguments:

    MiniportAdapterContext      Pointer to our adapter
    NetDevicePnPEvent           Self-explanatory

Return Value:

    None.

--*/
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPDevicePnpEventNotify");

    PAGED_CODE();

    switch (NetDevicePnPEvent->DevicePnPEvent)
    {
        case NdisDevicePnPEventSurpriseRemoved:
            //
            // Called when NDIS receives IRP_MN_SUPRISE_REMOVAL.
            // NDIS calls MiniportHalt function after this call returns.
            //
            MP_SET_FLAG(Adapter, fMP_ADAPTER_SURPRISE_REMOVED);
            DEBUGP(DEBUG_INFO, "MPDevicePnpEventNotify: NdisDevicePnPEventSurpriseRemoved");
            break;

        case NdisDevicePnPEventPowerProfileChanged:
            //
            // After initializing a miniport driver and after miniport driver
            // receives an OID_PNP_SET_POWER notification that specifies
            // a device power state of NdisDeviceStateD0 (the powered-on state),
            // NDIS calls the miniport's MiniportPnPEventNotify function with
            // PnPEvent set to NdisDevicePnPEventPowerProfileChanged.
            //
            DEBUGP(DEBUG_INFO, "MPDevicePnpEventNotify: NdisDevicePnPEventPowerProfileChanged");

            if (NetDevicePnPEvent->InformationBufferLength == sizeof(ULONG))
            {
                ULONG NdisPowerProfile = *((PULONG)NetDevicePnPEvent->InformationBuffer);

                if (NdisPowerProfile == NdisPowerProfileBattery)
                {
                    DEBUGP(DEBUG_INFO, "The host system is running on battery power");
                }
                if (NdisPowerProfile == NdisPowerProfileAcOnLine)
                {
                    DEBUGP(DEBUG_INFO, "The host system is running on AC power");
                }
            }
            break;

        default:
            DEBUGP(DEBUG_ERROR, "MPDevicePnpEventNotify: unknown PnP event 0x%x", NetDevicePnPEvent->DevicePnPEvent);
    }

    DEBUGP(DEBUG_TRACE, "<--- MPDevicePnpEventNotify");
}



