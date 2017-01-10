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
* @file XNet_OIDs.c
*
* This file contains NDIS OID functions
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
#include "XNet_OIDs.tmh"
#endif // TRACE_ENABLED


NDIS_OID NICSupportedOids[] =
{
		OID_GEN_SUPPORTED_LIST,
        OID_GEN_HARDWARE_STATUS,
        OID_GEN_MEDIA_SUPPORTED,
        OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
	OID_GEN_LINK_STATE,
		OID_GEN_TRANSMIT_BUFFER_SPACE,
        OID_GEN_RECEIVE_BUFFER_SPACE,
        OID_GEN_TRANSMIT_BLOCK_SIZE,
        OID_GEN_RECEIVE_BLOCK_SIZE,
        OID_GEN_VENDOR_ID,
        OID_GEN_VENDOR_DESCRIPTION,
        OID_GEN_VENDOR_DRIVER_VERSION,
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_CURRENT_LOOKAHEAD,
        OID_GEN_DRIVER_VERSION,
        OID_GEN_MAXIMUM_TOTAL_SIZE,
	OID_GEN_MAC_OPTIONS,
		 OID_GEN_MEDIA_CONNECT_STATUS,
        OID_GEN_MAXIMUM_SEND_PACKETS,
        OID_GEN_XMIT_OK,
        OID_GEN_RCV_OK,
        OID_GEN_XMIT_ERROR,
        OID_GEN_RCV_ERROR,
        OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_RCV_DISCARDS,
        OID_GEN_TRANSMIT_QUEUE_LENGTH,       // Optional
        OID_802_3_XMIT_ONE_COLLISION,
        OID_802_3_XMIT_MORE_COLLISIONS,
        OID_802_3_XMIT_MAX_COLLISIONS,       // Optional
        OID_802_3_RCV_ERROR_ALIGNMENT,
		OID_GEN_STATISTICS,
		OID_GEN_PHYSICAL_MEDIUM_EX,
			OID_GEN_LINK_PARAMETERS,
        OID_802_3_PERMANENT_ADDRESS,
        OID_802_3_CURRENT_ADDRESS,
        OID_802_3_MULTICAST_LIST,
        OID_802_3_MAXIMUM_LIST_SIZE,
        OID_802_3_XMIT_DEFERRED,             // Optional
        OID_802_3_RCV_OVERRUN,               // Optional
        OID_802_3_XMIT_UNDERRUN,             // Optional
        OID_802_3_XMIT_HEARTBEAT_FAILURE,    // Optional
        OID_802_3_XMIT_TIMES_CRS_LOST,       // Optional
        OID_802_3_XMIT_LATE_COLLISIONS,      // Optional
        OID_PNP_CAPABILITIES,                // Optional
};

static
NDIS_STATUS
MPMethodRequest(
    __in  PMP_ADAPTER             Adapter,
    __in  PNDIS_OID_REQUEST       NdisRequest);

static
NDIS_STATUS
MPSetInformation(
    __in  PMP_ADAPTER         Adapter,
    __in  PNDIS_OID_REQUEST   NdisSetRequest);

static
NDIS_STATUS
MPQueryInformation(
    __in  PMP_ADAPTER        Adapter,
    __inout PNDIS_OID_REQUEST  NdisQueryRequest);

static
NDIS_STATUS
NICSetMulticastList(
    __in  PMP_ADAPTER        Adapter,
    __in  PNDIS_OID_REQUEST  NdisSetRequest);

static
NDIS_STATUS
NICSetPacketFilter(
    __in PMP_ADAPTER Adapter,
    __in ULONG PacketFilter);


/*++

Routine Description:

    Entry point called by NDIS to get or set the value of a specified OID.

Arguments:

    MiniportAdapterContext  - Our adapter handle
    NdisRequest             - The OID request to handle

Return Value:

    Return code from the NdisRequest below.

--*/
NDIS_STATUS
MPOidRequest(
    __in  NDIS_HANDLE        MiniportAdapterContext,
    __in  PNDIS_OID_REQUEST  NdisRequest)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPOidRequest");

    switch (NdisRequest->RequestType)
    {
        case NdisRequestMethod:
            Status = MPMethodRequest(Adapter, NdisRequest);
            break;

        case NdisRequestSetInformation:
            Status = MPSetInformation(Adapter, NdisRequest);
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            Status = MPQueryInformation(Adapter, NdisRequest);
            break;

        default:
            //
            // The entry point may by used by other requests
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    DEBUGP(DEBUG_TRACE, "<--- MPOidRequest Status = 0x%08x", Status);
    return Status;
}


/*++

Routine Description:

    Entry point called by NDIS to abort an asynchronous OID request.

Arguments:

    MiniportAdapterContext  - Our adapter handle
    RequestId               - An identifier that corresponds to the RequestId
                              field of the NDIS_OID_REQUEST

Return Value:

    None.

--*/
VOID
MPCancelOidRequest(
    __in  NDIS_HANDLE  MiniportAdapterContext,
    __in  PVOID        RequestId)
{
    PMP_ADAPTER Adapter = MP_ADAPTER_FROM_CONTEXT(MiniportAdapterContext);

    DEBUGP(DEBUG_TRACE, "---> MPCancelOidRequest");
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(RequestId);


    //
    // This miniport sample does not pend any OID requests, so we don't have
    // to worry about cancelling them.
    //

    DEBUGP(DEBUG_TRACE, "<--- MPCancelOidRequest");
}



/*++

Routine Description:

    Helper function to perform a query OID request

Arguments:

    Adapter           -
    NdisQueryRequest  - The OID that is being queried

Return Value:

    NDIS_STATUS

--*/
NDIS_STATUS
MPQueryInformation(
    __in  PMP_ADAPTER        Adapter,
    __inout PNDIS_OID_REQUEST  NdisQueryRequest)
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    struct _QUERY          *Query = &NdisQueryRequest->DATA.QUERY_INFORMATION;

    NDIS_HARDWARE_STATUS    HardwareStatus = NdisHardwareStatusReady;
    UCHAR                   VendorDesc[] = NIC_VENDOR_DESC;
    NDIS_MEDIUM             Medium = NIC_MEDIUM_TYPE;
    NDIS_MEDIUM             PhysMedium = NdisPhysicalMedium802_3;
    ULONG                   ulInfo;
    USHORT                  usInfo;
    ULONG64                 ulInfo64;

    // Default to returning the ULONG value
    PVOID                   pInfo=NULL;
    ULONG                   ulInfoLen = sizeof(ulInfo);

    DEBUGP(DEBUG_TRACE, "---> MPQueryInformation");
    DbgPrintOidName(Query->Oid);

    switch(Query->Oid)
    {
		  case OID_GEN_SUPPORTED_LIST:
		    pInfo = (PVOID) NICSupportedOids;
			ulInfoLen = sizeof(NICSupportedOids);
			break;

        case OID_GEN_HARDWARE_STATUS:
            //
            // Specify the current hardware status of the underlying NIC as
            // one of the following NDIS_HARDWARE_STATUS-type values.
            //
            pInfo = (PVOID) &HardwareStatus;
            ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;

		case OID_GEN_LINK_STATE:
			break;

		case OID_GEN_LINK_SPEED:
			break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            //
            // Specify the maximum total packet length, in bytes, the NIC
            // supports including the header. A protocol driver might use
            // this returned length as a gauge to determine the maximum
            // size packet that a NIC driver could forward to the
            // protocol driver. The miniport driver must never indicate
            // up to the bound protocol driver packets received over the
            // network that are longer than the packet size specified by
            // OID_GEN_MAXIMUM_TOTAL_SIZE.
            //

            __fallthrough;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            //
            // The OID_GEN_TRANSMIT_BLOCK_SIZE OID specifies the minimum
            // number of bytes that a single net packet occupies in the
            // transmit buffer space of the NIC. For example, a NIC that
            // has a transmit space divided into 256-byte pieces would have
            // a transmit block size of 256 bytes. To calculate the total
            // transmit buffer space on such a NIC, its driver multiplies
            // the number of transmit buffers on the NIC by its transmit
            // block size. In our case, the transmit block size is
            // identical to its maximum packet size.

            __fallthrough;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            //
            // The OID_GEN_RECEIVE_BLOCK_SIZE OID specifies the amount of
            // storage, in bytes, that a single packet occupies in the receive
            // buffer space of the NIC.
            //
			ulInfo = (ULONG) Adapter->FrameSize;
            pInfo = &ulInfo;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            //
            // Specify the amount of memory, in bytes, on the NIC that
            // is available for buffering transmit data. A protocol can
            // use this OID as a guide for sizing the amount of transmit
            // data per send.
            //

			ulInfo = Adapter->FrameSize * Adapter->ulMaxBusySends;
            pInfo = &ulInfo;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            //
            // Specify the amount of memory on the NIC that is available
            // for buffering receive data. A protocol driver can use this
            // OID as a guide for advertising its receive window after it
            // establishes sessions with remote nodes.
            //

			ulInfo = Adapter->FrameSize * Adapter->ulMaxBusyRecvs;
            pInfo = &ulInfo;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
            //
            // Return an array of media that are supported by the miniport.
            // This miniport only supports one medium (Ethernet), so the OID
            // returns identical results to OID_GEN_MEDIA_IN_USE.
            //

            __fallthrough;

        case OID_GEN_MEDIA_IN_USE:
            //
            // Return an array of media that are currently in use by the
            // miniport.  This array should be a subset of the array returned
            // by OID_GEN_MEDIA_SUPPORTED.
            //
            pInfo = &Medium;
            ulInfoLen = sizeof(Medium);
            break;

		case OID_GEN_PHYSICAL_MEDIUM_EX:
            //
            // Return the actual underlying media 
            //
            pInfo = &PhysMedium;
            ulInfoLen = sizeof(PhysMedium);
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = NIC_MAX_BUSY_SENDS;
            pInfo = &ulInfo;
            break;

        case OID_GEN_XMIT_ERROR:
            ulInfo = (ULONG)
                    (Adapter->TxAbortExcessCollisions +
                    Adapter->TxDmaUnderrun +
                    Adapter->TxLostCRS +
                    Adapter->TxLateCollisions+
                    Adapter->TransmitFailuresOther);
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_ERROR:
            ulInfo = (ULONG)
                    (Adapter->RxCrcErrors +
                    Adapter->RxAlignmentErrors +
                    Adapter->RxDmaOverrunErrors +
                    Adapter->RxRuntErrors);
            pInfo = &ulInfo;
            break;

		case OID_GEN_RCV_CRC_ERROR:
            ulInfo = (ULONG)Adapter->RxCrcErrors;
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_DISCARDS:
            ulInfo = (ULONG)Adapter->RxResourceErrors;
            pInfo = &ulInfo;
            break;

        case OID_GEN_RCV_NO_BUFFER:
            ulInfo = (ULONG)
                    Adapter->RxResourceErrors;
            pInfo = &ulInfo;
            break;

        case OID_GEN_VENDOR_ID:
            //
            // Specify a three-byte IEEE-registered vendor code, followed
            // by a single byte that the vendor assigns to identify a
            // particular NIC. The IEEE code uniquely identifies the vendor
            // and is the same as the three bytes appearing at the beginning
            // of the NIC hardware address. Vendors without an IEEE-registered
            // code should use the value 0xFFFFFF.
            //

            ulInfo = NIC_VENDOR_ID;
            pInfo = &ulInfo;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            //
            // Specify a zero-terminated string describing the NIC vendor.
            //
            pInfo = VendorDesc;
            ulInfoLen = sizeof(VendorDesc);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            //
            // Specify the vendor-assigned version number of the NIC driver.
            // The low-order half of the return value specifies the minor
            // version; the high-order half specifies the major version.
            //

            ulInfo = NIC_VENDOR_DRIVER_VERSION;
            pInfo = &ulInfo;
            break;

        case OID_GEN_DRIVER_VERSION:
            //
            // Specify the NDIS version in use by the NIC driver. The high
            // byte is the major version number; the low byte is the minor
            // version number.
            //
            usInfo = (USHORT) (MP_NDIS_MAJOR_VERSION<<8) + MP_NDIS_MINOR_VERSION;
            pInfo = (PVOID) &usInfo;
            ulInfoLen = sizeof(USHORT);
            break;

        case OID_PNP_CAPABILITIES:
            //
            // Return the wake-up capabilities of its NIC. If you return
            // NDIS_STATUS_NOT_SUPPORTED, NDIS considers the miniport driver
            // to be not Power management aware and doesn't send any power
            // or wake-up related queries such as
            // OID_PNP_SET_POWER, OID_PNP_QUERY_POWER,
            // OID_PNP_ADD_WAKE_UP_PATTERN, OID_PNP_REMOVE_WAKE_UP_PATTERN,
            // OID_PNP_ENABLE_WAKE_UP.
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;

            break;
            //
            // Following 4 OIDs are for querying Ethernet Operational
            // Characteristics.
            //
        case OID_802_3_PERMANENT_ADDRESS:
            //
            // Return the MAC address of the NIC burnt in the hardware.
            //
            pInfo = Adapter->PermanentAddress;
            ulInfoLen = NIC_MACADDR_SIZE;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            //
            // Return the MAC address the NIC is currently programmed to
            // use. Note that this address could be different from the
            // permananent address as the user can override using
            // registry. Read NdisReadNetworkAddress doc for more info.
            //
            pInfo = Adapter->CurrentAddress;
            ulInfoLen = NIC_MACADDR_SIZE;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            //
            // The maximum number of multicast addresses the NIC driver
            // can manage. This list is global for all protocols bound
            // to (or above) the NIC. Consequently, a protocol can receive
            // NDIS_STATUS_MULTICAST_FULL from the NIC driver when
            // attempting to set the multicast address list, even if
            // the number of elements in the given list is less than
            // the number originally returned for this query.
            //

            ulInfo = NIC_MAX_MCAST_LIST;
            pInfo = &ulInfo;
            break;

            //
            // Following list  consists of both general and Ethernet
            // specific statistical OIDs.
            //

        case OID_GEN_XMIT_OK:
			ulInfo64 = 0;
#if 0
			/*
			WARNING:  Using the code below will cause bus hangs.
			*/
			ulInfo64 = Xil_In64(Adapter->MACRegisters + XXGE_TXFL_OFFSET);
			DEBUGP(DEBUG_WARN, "Xmit MAC Regs 0x%p, returned 0x%lx\n", Adapter->MACRegisters  + XXGE_TXFL_OFFSET, ulInfo64);
#endif // Commented Out
			ulInfo64 = Adapter->FramesTxDirected + Adapter->FramesTxMulticast + Adapter->FramesTxBroadcast;
			DEBUGP(DEBUG_WARN, "Xmit_OK returned %ld\n", ulInfo64);
            pInfo = &ulInfo64;
            if (Query->InformationBufferLength >= sizeof(ULONG64) ||
                Query->InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            // We should always report that only 8 bytes are required to keep ndistest happy
            Query->BytesNeeded =  sizeof(ULONG64);
            break;

        case OID_GEN_RCV_OK:
			ulInfo64 = 0;
#if 0
			/*
			WARNING:  Using the code below will cause bus hangs.
			*/
			ulInfo64 = Xil_In64(Adapter->MACRegisters + XXGE_RXFL_OFFSET);
			DEBUGP(DEBUG_WARN, "Rcv MAC Regs 0x%p, returned 0x%lx\n", Adapter->MACRegisters  + XXGE_RXFL_OFFSET, ulInfo64);
#endif // Commented Out
			ulInfo64 = Adapter->FramesRxDirected + Adapter->FramesRxMulticast + Adapter->FramesRxBroadcast;
			DEBUGP(DEBUG_WARN, "Rcv_Ok returned %ld\n", ulInfo64);
            pInfo = &ulInfo64;
            if (Query->InformationBufferLength >= sizeof(ULONG64) ||
                Query->InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            // We should always report that only 8 bytes are required to keep ndistest happy
            Query->BytesNeeded =  sizeof(ULONG64);
            break;

		case OID_GEN_MEDIA_CONNECT_STATUS:
			XXgEthernet_PhyRead(Adapter, Adapter->PhyAddress, XXGE_MDIO_REGISTER_ADDRESS, &usInfo);
			if (usInfo & XXGE_MDIO_PHY_LINK_UP_MASK)
			{
				ulInfo =  NdisMediaStateConnected;
				Adapter->LinkState = MediaConnectStateConnected;
				DEBUGP(DEBUG_WARN, "Connect OID: Phys Regs 0x%u - Connected\n", Adapter->PhyAddress);
			}
			else
			{
				ulInfo =  NdisMediaStateDisconnected;
				Adapter->LinkState = MediaConnectStateDisconnected;
				DEBUGP(DEBUG_WARN, "Connect OID: Phys Regs 0x%u - Disconnected\n", Adapter->PhyAddress);
			}
            pInfo = &ulInfo;
		    break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            ulInfo = Adapter->ulLookahead;
            pInfo = &ulInfo;
            break;


        case OID_GEN_STATISTICS:

            if (Query->InformationBufferLength < sizeof(NDIS_STATISTICS_INFO))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                Query->BytesNeeded = sizeof(NDIS_STATISTICS_INFO);
                break;
            }
            else
            {
                PNDIS_STATISTICS_INFO Statistics = (PNDIS_STATISTICS_INFO)Query->InformationBuffer;

		        NdisZeroMemory(Statistics, sizeof(NDIS_STATISTICS_INFO));

                {C_ASSERT(sizeof(NDIS_STATISTICS_INFO) >= NDIS_SIZEOF_STATISTICS_INFO_REVISION_1);}
                Statistics->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                Statistics->Header.Size = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;
                Statistics->Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;

                Statistics->SupportedStatistics = NIC_SUPPORTED_STATISTICS;

#if 0
				/*
				WARNING:  Using the code below will cause bus hangs.
				*/
                /* Bytes in */
                Statistics->ifHCInOctets = Adapter->pMACStats->XXGE_RXB.STAT.ULONG64;
                /* Packets in */
				Statistics->ifHCInUcastPkts = Adapter->pMACStats->XXGE_RXF.STAT.ULONG64;
				Statistics->ifHCInMulticastPkts = Adapter->pMACStats->XXGE_RXMCSTF.STAT.ULONG64;
                Statistics->ifHCInBroadcastPkts = Adapter->pMACStats->XXGE_RXBCSTF.STAT.ULONG64;

                /* Errors in */
                Statistics->ifInErrors = 
                        Adapter->RxCrcErrors +
                        Adapter->RxAlignmentErrors +
                        Adapter->RxDmaOverrunErrors +
                        Adapter->RxRuntErrors;

                Statistics->ifInDiscards =
                        Adapter->RxResourceErrors;

                /* Bytes out */
				Statistics->ifHCOutOctets = Adapter->pMACStats->XXGE_TXB.STAT.ULONG64;

                /* Packets out */
				Statistics->ifHCOutUcastPkts = Adapter->pMACStats->XXGE_TXF.STAT.ULONG64;
				Statistics->ifHCOutMulticastPkts = Adapter->pMACStats->XXGE_TXMCSTF.STAT.ULONG64;
				Statistics->ifHCOutBroadcastPkts = Adapter->pMACStats->XXGE_TXBCSTF.STAT.ULONG64;

                /* Errors out */
                Statistics->ifOutErrors =
                        Adapter->TxAbortExcessCollisions +
                        Adapter->TxDmaUnderrun +
                        Adapter->TxLostCRS +
                        Adapter->TxLateCollisions+
                        Adapter->TransmitFailuresOther;
#else
                /* Bytes in */
                Statistics->ifHCInOctets = Adapter->BytesRxDirected +
						Adapter->BytesRxMulticast + Adapter->BytesRxBroadcast;

                /* Packets in */
				Statistics->ifHCInUcastPkts = Adapter->FramesRxDirected;
				Statistics->ifHCInMulticastPkts = Adapter->FramesRxMulticast;
                Statistics->ifHCInBroadcastPkts = Adapter->FramesRxBroadcast;

                /* Errors in */
                Statistics->ifInErrors = 
                        Adapter->RxCrcErrors +
                        Adapter->RxAlignmentErrors +
                        Adapter->RxDmaOverrunErrors +
                        Adapter->RxRuntErrors;

                Statistics->ifInDiscards = Adapter->RxResourceErrors;

                /* Bytes out */
				Statistics->ifHCOutOctets = Adapter->BytesTxDirected +
						Adapter->BytesTxMulticast + Adapter->BytesTxBroadcast;

                /* Packets out */
				Statistics->ifHCOutUcastPkts = Adapter->FramesTxDirected;
				Statistics->ifHCOutMulticastPkts = Adapter->FramesTxMulticast;
				Statistics->ifHCOutBroadcastPkts = Adapter->FramesTxBroadcast;

                /* Errors out */
                Statistics->ifOutErrors =
                        Adapter->TxAbortExcessCollisions +
                        Adapter->TxDmaUnderrun +
                        Adapter->TxLostCRS +
                        Adapter->TxLateCollisions+
                        Adapter->TransmitFailuresOther;
#endif // Commented Out
				DEBUGP(DEBUG_WARN, "Tx bytes %ld, Rx bytes returned %ld\n", Statistics->ifHCOutOctets, Statistics->ifHCInOctets);

                ulInfoLen = NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;
            }

            break;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:

            ulInfo = Adapter->nBusySend;
            pInfo = &ulInfo;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:

            ulInfo = Adapter->RxAlignmentErrors;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:

            ulInfo = Adapter->OneRetry;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:

            ulInfo = Adapter->MoreThanOneRetry;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_DEFERRED:

            ulInfo = Adapter->TxOKButDeferred;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_MAX_COLLISIONS:

            ulInfo = Adapter->TxAbortExcessCollisions;
            pInfo = &ulInfo;
            break;

        case OID_802_3_RCV_OVERRUN:

            ulInfo = Adapter->RxDmaOverrunErrors;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_UNDERRUN:

            ulInfo = Adapter->TxDmaUnderrun;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_HEARTBEAT_FAILURE:

            ulInfo = Adapter->TxLostCRS;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_TIMES_CRS_LOST:

            ulInfo = Adapter->TxLostCRS;
            pInfo = &ulInfo;
            break;

        case OID_802_3_XMIT_LATE_COLLISIONS:

            ulInfo = Adapter->TxLateCollisions;
            pInfo = &ulInfo;
            break;

        case OID_GEN_INTERRUPT_MODERATION:
        {
            PNDIS_INTERRUPT_MODERATION_PARAMETERS Moderation = (PNDIS_INTERRUPT_MODERATION_PARAMETERS)Query->InformationBuffer;
            Moderation->Header.Type = NDIS_OBJECT_TYPE_DEFAULT; 
            Moderation->Header.Revision = NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            Moderation->Header.Size = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
            Moderation->Flags = 0;
            Moderation->InterruptModeration = NdisInterruptModerationNotSupported;
            ulInfoLen = NDIS_SIZEOF_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
        }
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        ASSERT(ulInfoLen > 0);

        if (ulInfoLen <= Query->InformationBufferLength)
        {
            if(pInfo)
            {
                // Copy result into InformationBuffer
                NdisMoveMemory(Query->InformationBuffer, pInfo, ulInfoLen);
            }
            Query->BytesWritten = ulInfoLen;
        }
        else
        {
            // too short
            Query->BytesNeeded = ulInfoLen;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
    }

    DEBUGP(DEBUG_TRACE, "<--- MPQueryInformation Status = 0x%08x", Status);
    return Status;
}


/*++

Routine Description:

    Helper function to perform a set OID request

Arguments:

    Adapter         -
    NdisSetRequest  - The OID to set

Return Value:

    NDIS_STATUS

--*/
NDIS_STATUS
MPSetInformation(
    __in  PMP_ADAPTER         Adapter,
    __in  PNDIS_OID_REQUEST   NdisSetRequest)
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    struct _SET            *Set = &NdisSetRequest->DATA.SET_INFORMATION;

    DEBUGP(DEBUG_TRACE, "---> MPSetInformation");
    DbgPrintOidName(Set->Oid);

    switch(Set->Oid)
    {
        case OID_802_3_MULTICAST_LIST:
            //
            // Set the multicast address list on the NIC for packet reception.
            // The NIC driver can set a limit on the number of multicast
            // addresses bound protocol drivers can enable simultaneously.
            // NDIS returns NDIS_STATUS_MULTICAST_FULL if a protocol driver
            // exceeds this limit or if it specifies an invalid multicast
            // address.
            //
            Status = NICSetMulticastList(Adapter, NdisSetRequest);

            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            //
            // Program the hardware to indicate the packets
            // of certain filter types.
            //
            if(Set->InformationBufferLength != sizeof(ULONG))
            {
                Set->BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            Set->BytesRead = Set->InformationBufferLength;

            Status = NICSetPacketFilter(
                            Adapter,
                            *((PULONG)Set->InformationBuffer));

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            //
            // A protocol driver can set a suggested value for the number
            // of bytes to be used in its binding; however, the underlying
            // NIC driver is never required to limit its indications to
            // the value set.
            //
            if (Set->InformationBufferLength != sizeof(ULONG))
            {
                Set->BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
            Adapter->ulLookahead = *(PULONG)Set->InformationBuffer;

            Set->BytesRead = sizeof(ULONG);
            Status = NDIS_STATUS_SUCCESS;
            break;

        case OID_PNP_SET_POWER:
        case OID_PNP_QUERY_POWER:
        case OID_PNP_ADD_WAKE_UP_PATTERN:
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        case OID_PNP_ENABLE_WAKE_UP:
            ASSERT(!"NIC does not support PNP POWER OIDs");
        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;

    }

    if(Status == NDIS_STATUS_SUCCESS)
    {
        Set->BytesRead = Set->InformationBufferLength;
    }

    DEBUGP(DEBUG_TRACE, "<--- MPSetInformation Status = 0x%08x", Status);
    return Status;
}


/*++
Routine Description:

    Helper function to perform a WMI OID request

Arguments:

    Adapter      -
    NdisRequest  - THe WMI OID request

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED

--*/
NDIS_STATUS
MPMethodRequest(
    __in  PMP_ADAPTER             Adapter,
    __in  PNDIS_OID_REQUEST       NdisRequest)
{
    NDIS_STATUS      Status = NDIS_STATUS_SUCCESS;
    NDIS_OID         Oid = NdisRequest->DATA.METHOD_INFORMATION.Oid;

    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(NdisRequest);

    DEBUGP(DEBUG_TRACE, "---> MPMethodRequest");
    DbgPrintOidName(Oid);

    switch (Oid)
    {
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DEBUGP(DEBUG_TRACE, "<--- MPMethodRequest Status = 0x%08x", Status);
    return Status;
}




/*++
Routine Description:

    This routine will set up the adapter so that it accepts packets
    that match the specified packet filter.

Arguments:

    Adapter      - pointer to adapter block
    PacketFilter - the new packet filter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED

--*/
NDIS_STATUS
NICSetPacketFilter(
    __in  PMP_ADAPTER Adapter,
    __in  ULONG PacketFilter)
{
	ULONG		Options = 0;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGP(DEBUG_TRACE, "---> NICSetPacketFilter");

    // any bits not supported?
    if (PacketFilter & ~(NIC_SUPPORTED_FILTERS))
    {
        DEBUGP(DEBUG_WARN, "Unsupported packet filter: 0x%08x", PacketFilter);
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    // any filtering changes?
    if (PacketFilter != Adapter->PacketFilter)
    {
        //
        // Change the filtering modes on hardware
        //
		if (PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
		{
			Options |= XXGE_BROADCAST_OPTION;
		}
		if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
		{
			Options |= XXGE_PROMISC_OPTION;
		}
		
//			NDIS_PACKET_TYPE_MULTICAST
//			NDIS_PACKET_TYPE_DIRECTED
//			NDIS_PACKET_TYPE_ALL_MULTICAST

//		Status = XXgEthernet_SetOptions(Adapter, Options);

        // Save the new packet filter value
        Adapter->PacketFilter = PacketFilter;
    }


    DEBUGP(DEBUG_TRACE, "<--- NICSetPacketFilter Status = 0x%08x", Status);

    return Status;
}


/*++
Routine Description:

    This routine will set up the adapter for a specified multicast
    address list.

Arguments:

    Adapter         - Pointer to adapter block
    NdisSetRequest  - The OID request with the new multicast list

Return Value:

    NDIS_STATUS

--*/
NDIS_STATUS
NICSetMulticastList(
    __in  PMP_ADAPTER        Adapter,
    __in  PNDIS_OID_REQUEST  NdisSetRequest)
{
    NDIS_STATUS   Status = NDIS_STATUS_SUCCESS;
    struct _SET  *Set = &NdisSetRequest->DATA.SET_INFORMATION;

#if DBG
    ULONG                  index;
#endif

    DEBUGP(DEBUG_TRACE, "---> NICSetMulticastList");


    //
    // Initialize.
    //
    Set->BytesNeeded = NIC_MACADDR_SIZE;
    Set->BytesRead = Set->InformationBufferLength;

    do
    {
        if (Set->InformationBufferLength % NIC_MACADDR_SIZE)
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (Set->InformationBufferLength > (NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE))
        {
            Status = NDIS_STATUS_MULTICAST_FULL;
            Set->BytesNeeded = NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE;
            break;
        }

        //
        // Protect the list update with a lock if it can be updated by
        // another thread simultaneously.
        //

        NdisZeroMemory(Adapter->MCList,
                       NIC_MAX_MCAST_LIST * NIC_MACADDR_SIZE);

        NdisMoveMemory(Adapter->MCList,
                       Set->InformationBuffer,
                       Set->InformationBufferLength);

        Adapter->ulMCListSize = Set->InformationBufferLength / NIC_MACADDR_SIZE;

#if DBG
        // display the multicast list
        for(index = 0; index < Adapter->ulMCListSize; index++)
        {
            DEBUGP(DEBUG_VERBOSE, "MC(%d) = ", index);
            DbgPrintAddress(Adapter->MCList[index]);
        }
#endif
    }
    while (FALSE);


    //
    // Program the hardware to add suport for these muticast addresses
    //


    DEBUGP(DEBUG_TRACE, "<--- NICSetMulticastList Status 0x%08x", Status);
    return Status;
}



#ifndef DBG

VOID
DbgPrintOidName(
    __in  NDIS_OID  Oid)
{
    UNREFERENCED_PARAMETER(Oid);
}

VOID
DbgPrintAddress(
    __in_bcount(NIC_MACADDR_SIZE) PUCHAR Address)
{
    UNREFERENCED_PARAMETER(Address);
}


#else // DBG

VOID
DbgPrintOidName(
    __in  NDIS_OID  Oid)
{
    PCHAR oidName = NULL;

    switch (Oid){

        #undef MAKECASE
        #define MAKECASE(oidx) case oidx: oidName = #oidx ""; break;

        /* Operational OIDs */
        MAKECASE(OID_GEN_SUPPORTED_LIST)
        MAKECASE(OID_GEN_HARDWARE_STATUS)
        MAKECASE(OID_GEN_MEDIA_SUPPORTED)
        MAKECASE(OID_GEN_MEDIA_IN_USE)
        MAKECASE(OID_GEN_MAXIMUM_LOOKAHEAD)
        MAKECASE(OID_GEN_MAXIMUM_FRAME_SIZE)
        MAKECASE(OID_GEN_LINK_SPEED)
        MAKECASE(OID_GEN_TRANSMIT_BUFFER_SPACE)
        MAKECASE(OID_GEN_RECEIVE_BUFFER_SPACE)
        MAKECASE(OID_GEN_TRANSMIT_BLOCK_SIZE)
        MAKECASE(OID_GEN_RECEIVE_BLOCK_SIZE)
        MAKECASE(OID_GEN_VENDOR_ID)
        MAKECASE(OID_GEN_VENDOR_DESCRIPTION)
        MAKECASE(OID_GEN_VENDOR_DRIVER_VERSION)
        MAKECASE(OID_GEN_CURRENT_PACKET_FILTER)
        MAKECASE(OID_GEN_CURRENT_LOOKAHEAD)
        MAKECASE(OID_GEN_DRIVER_VERSION)
        MAKECASE(OID_GEN_MAXIMUM_TOTAL_SIZE)
        MAKECASE(OID_GEN_PROTOCOL_OPTIONS)
        MAKECASE(OID_GEN_MAC_OPTIONS)
        MAKECASE(OID_GEN_MEDIA_CONNECT_STATUS)
        MAKECASE(OID_GEN_MAXIMUM_SEND_PACKETS)
        MAKECASE(OID_GEN_SUPPORTED_GUIDS)
        MAKECASE(OID_GEN_NETWORK_LAYER_ADDRESSES)
        MAKECASE(OID_GEN_TRANSPORT_HEADER_OFFSET)
        MAKECASE(OID_GEN_MEDIA_CAPABILITIES)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM)
        MAKECASE(OID_GEN_MACHINE_NAME)
        MAKECASE(OID_GEN_VLAN_ID)
        MAKECASE(OID_GEN_RNDIS_CONFIG_PARAMETER)

        /* Operational OIDs for NDIS 6.0 */
        MAKECASE(OID_GEN_MAX_LINK_SPEED)
        MAKECASE(OID_GEN_LINK_STATE)
        MAKECASE(OID_GEN_LINK_PARAMETERS)
        MAKECASE(OID_GEN_MINIPORT_RESTART_ATTRIBUTES)
        MAKECASE(OID_GEN_ENUMERATE_PORTS)
        MAKECASE(OID_GEN_PORT_STATE)
        MAKECASE(OID_GEN_PORT_AUTHENTICATION_PARAMETERS)
        MAKECASE(OID_GEN_INTERRUPT_MODERATION)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM_EX)

        /* Statistical OIDs */
        MAKECASE(OID_GEN_XMIT_OK)
        MAKECASE(OID_GEN_RCV_OK)
        MAKECASE(OID_GEN_XMIT_ERROR)
        MAKECASE(OID_GEN_RCV_ERROR)
        MAKECASE(OID_GEN_RCV_NO_BUFFER)
        MAKECASE(OID_GEN_DIRECTED_BYTES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_BYTES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_BYTES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_BYTES_RCV)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_RCV)
        MAKECASE(OID_GEN_MULTICAST_BYTES_RCV)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_RCV)
        MAKECASE(OID_GEN_BROADCAST_BYTES_RCV)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_RCV)
        MAKECASE(OID_GEN_RCV_CRC_ERROR)
        MAKECASE(OID_GEN_TRANSMIT_QUEUE_LENGTH)

        /* Statistical OIDs for NDIS 6.0 */
        MAKECASE(OID_GEN_STATISTICS)
        MAKECASE(OID_GEN_BYTES_RCV)
        MAKECASE(OID_GEN_BYTES_XMIT)
        MAKECASE(OID_GEN_RCV_DISCARDS)
        MAKECASE(OID_GEN_XMIT_DISCARDS)

        /* Misc OIDs */
        MAKECASE(OID_GEN_GET_TIME_CAPS)
        MAKECASE(OID_GEN_GET_NETCARD_TIME)
        MAKECASE(OID_GEN_NETCARD_LOAD)
        MAKECASE(OID_GEN_DEVICE_PROFILE)
        MAKECASE(OID_GEN_INIT_TIME_MS)
        MAKECASE(OID_GEN_RESET_COUNTS)
        MAKECASE(OID_GEN_MEDIA_SENSE_COUNTS)

        /* PnP power management operational OIDs */
        MAKECASE(OID_PNP_CAPABILITIES)
        MAKECASE(OID_PNP_SET_POWER)
        MAKECASE(OID_PNP_QUERY_POWER)
        MAKECASE(OID_PNP_ADD_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_REMOVE_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_ENABLE_WAKE_UP)
        MAKECASE(OID_PNP_WAKE_UP_PATTERN_LIST)

        /* PnP power management statistical OIDs */
        MAKECASE(OID_PNP_WAKE_UP_ERROR)
        MAKECASE(OID_PNP_WAKE_UP_OK)

        /* Ethernet operational OIDs */
        MAKECASE(OID_802_3_PERMANENT_ADDRESS)
        MAKECASE(OID_802_3_CURRENT_ADDRESS)
        MAKECASE(OID_802_3_MULTICAST_LIST)
        MAKECASE(OID_802_3_MAXIMUM_LIST_SIZE)
        MAKECASE(OID_802_3_MAC_OPTIONS)

        /* Ethernet operational OIDs for NDIS 6.0 */
        MAKECASE(OID_802_3_ADD_MULTICAST_ADDRESS)
        MAKECASE(OID_802_3_DELETE_MULTICAST_ADDRESS)

        /* Ethernet statistical OIDs */
        MAKECASE(OID_802_3_RCV_ERROR_ALIGNMENT)
        MAKECASE(OID_802_3_XMIT_ONE_COLLISION)
        MAKECASE(OID_802_3_XMIT_MORE_COLLISIONS)
        MAKECASE(OID_802_3_XMIT_DEFERRED)
        MAKECASE(OID_802_3_XMIT_MAX_COLLISIONS)
        MAKECASE(OID_802_3_RCV_OVERRUN)
        MAKECASE(OID_802_3_XMIT_UNDERRUN)
        MAKECASE(OID_802_3_XMIT_HEARTBEAT_FAILURE)
        MAKECASE(OID_802_3_XMIT_TIMES_CRS_LOST)
        MAKECASE(OID_802_3_XMIT_LATE_COLLISIONS)

        /*  TCP/IP OIDs */
        MAKECASE(OID_TCP_TASK_OFFLOAD)
        MAKECASE(OID_TCP_TASK_IPSEC_ADD_SA)
        MAKECASE(OID_TCP_TASK_IPSEC_DELETE_SA)
        MAKECASE(OID_TCP_SAN_SUPPORT)
        MAKECASE(OID_TCP_TASK_IPSEC_ADD_UDPESP_SA)
        MAKECASE(OID_TCP_TASK_IPSEC_DELETE_UDPESP_SA)
        MAKECASE(OID_TCP4_OFFLOAD_STATS)
        MAKECASE(OID_TCP6_OFFLOAD_STATS)
        MAKECASE(OID_IP4_OFFLOAD_STATS)
        MAKECASE(OID_IP6_OFFLOAD_STATS)

        /* TCP offload OIDs for NDIS 6 */
        MAKECASE(OID_TCP_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(OID_TCP_OFFLOAD_PARAMETERS)
        MAKECASE(OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(OID_TCP_CONNECTION_OFFLOAD_CURRENT_CONFIG)
        MAKECASE(OID_TCP_CONNECTION_OFFLOAD_HARDWARE_CAPABILITIES)
        MAKECASE(OID_OFFLOAD_ENCAPSULATION)

#if defined(NDIS620_MINIPORT)
        /* VMQ OIDs for NDIS 6.20 */
        MAKECASE(OID_RECEIVE_FILTER_FREE_QUEUE)
        MAKECASE(OID_RECEIVE_FILTER_CLEAR_FILTER)
        MAKECASE(OID_RECEIVE_FILTER_ALLOCATE_QUEUE)
        MAKECASE(OID_RECEIVE_FILTER_QUEUE_ALLOCATION_COMPLETE)
        MAKECASE(OID_RECEIVE_FILTER_SET_FILTER)
#endif
    }

    if (oidName)
    {
        DEBUGP(DEBUG_INFO, " -- %s", oidName);
    }
    else
    {
        DEBUGP(DEBUG_WARN, "<** Unknown OID 0x%08x **>", Oid);
    }
}

VOID
DbgPrintAddress(
    __in_bcount(NIC_MACADDR_SIZE) PUCHAR Address)
{
    // If your MAC address has a different size, adjust the printf accordingly.
    {C_ASSERT(NIC_MACADDR_SIZE == 6);}

    DEBUGP(DEBUG_VERBOSE, "\t%02x-%02x-%02x-%02x-%02x-%02x",
            Address[0], Address[1], Address[2],
            Address[3], Address[4], Address[5]);
}



#endif //DBG


