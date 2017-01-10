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
* @file adapter.h
*
* This file contains defines and data strucutres for the adapter.
*
* MODIFICATION HISTORY:
*
* Ver     Date   Changes
* ----- -------- -------------------------------------------------------
* 1.0   11/18/12 First release 
*
******************************************************************************/

#define XNET_FSD_VERSION		((VER_MAJOR_NUM << 16) | (VER_MINOR_NUM << 8) | VER_SUBMINOR_NUM)

#define	MAX_NUMBER_APPS				4
#define	DEFAULT_APP					0

#define MAX_CS2_DMA_ENGINE			3

#define	MAX_DESIGN_MODE_SETTING		2
#define	DEFAULT_DESIGN_MODE_SETTING	0	// Raw Mode

// The following is the offset to the MAC Registers from the start of the
//  User Space (offset 0x8000) in BAR0.
#define		XNET_CONTROL_REGS_MAC0	0x3000
#define		XNET_CONTROL_REGS_MAC1	0x4000

#define		XNET_CONTROL_STAT_REGS	0x0200

#define NIC_ADAPTER_NAME_SIZE       128

#define		XGEMAC_MAC1__			1
//
// Utility macros
// -----------------------------------------------------------------------------
//

#define MP_SET_FLAG(_M, _F)             ((_M)->Flags |= (_F))
#define MP_CLEAR_FLAG(_M, _F)           ((_M)->Flags &= ~(_F))
#define MP_TEST_FLAG(_M, _F)            (((_M)->Flags & (_F)) != 0)
#define MP_TEST_FLAGS(_M, _F)           (((_M)->Flags & (_F)) == (_F))

#define MP_IS_READY(_M)        (((_M)->Flags &                             \
                                 (fMP_DISCONNECTED                         \
                                    | fMP_RESET_IN_PROGRESS                \
                                    | fMP_ADAPTER_HALT_IN_PROGRESS         \
                                    | fMP_ADAPTER_PAUSE_IN_PROGRESS        \
                                    | fMP_ADAPTER_PAUSED                   \
                                    )) == 0)


//
// Status flags for Flags field in the Adapter data struct
//
#define fMP_RESET_IN_PROGRESS               0x00000001
#define fMP_DISCONNECTED                    0x00000002
#define fMP_ADAPTER_HALT_IN_PROGRESS        0x00000004
#define fMP_ADAPTER_PAUSE_IN_PROGRESS       0x00000010
#define fMP_ADAPTER_PAUSED                  0x00000020
#define fMP_ADAPTER_SURPRISE_REMOVED        0x00000100
#define fMP_ADAPTER_STARTED					0x80000000
#define fMP_ADAPTER_STOPPED					0x40000000


//
// Each adapter managed by this driver has a MP_ADAPTER struct.
//
typedef struct _MP_ADAPTER
{
    LIST_ENTRY              List;
    //
    // Keep track of various device objects.
    //
    PDEVICE_OBJECT          Pdo;
    PDEVICE_OBJECT          Fdo;
    PDEVICE_OBJECT          NextDeviceObject;
    WDFIOTARGET				IoTarget;

    NDIS_HANDLE             AdapterHandle;

    ULONG                   Flags;

    UCHAR                   PermanentAddress[NIC_MACADDR_SIZE];
    UCHAR                   CurrentAddress[NIC_MACADDR_SIZE];

	// XDMA Driver information and Linkage
    REGISTER_XDRIVER        DriverLink;
	int						S2CDMAEngine;
	int						C2SDMAEngine;
	BOOLEAN					DMAAutoConfigure;

	BOOLEAN					MACAddressOverride;
	int						DesignMode;

    // Write
	NDIS_SPIN_LOCK			SendSpinLock;
    int                     SendDMAStatus;
    XDMA_HANDLE             DMASendHandle;
    REGISTER_DMA_ENGINE_RETURN  SendDMALink;
	PDMA_ADAPTER			pSendDmaAdapter;

    // Read
    NDIS_SPIN_LOCK			RecvSpinLock;
    int                     RecvDMAStatus;
    XDMA_HANDLE             DMARecvHandle;
    REGISTER_DMA_ENGINE_RETURN  RecvDMALink;
	PDMA_ADAPTER			pRecvDmaAdapter;

    /* Added for WdfDevice */
	WDFDEVICE				WdfDevice;
    WCHAR                   Name[NIC_ADAPTER_NAME_SIZE];

    // Number of transmit NBLs from the protocol that we still have
    volatile LONG           nBusySend;

	// Maximum Frame size the driver can handle
	ULONG					FrameSize;

	PUCHAR					pUserSpace;
	PUCHAR					MACRegisters;
	PUCHAR					XGERegisters;
	ULONG					PhyAddress;
	PMAC_STATS				pMACStats;

	ULONG					Options;

    //
    // Receive tracking
    // -------------------------------------------------------------------------
    //

    // List of allocated DMA Transaction dedicated to Receives
    LIST_ENTRY              RxDMATransList;
    NDIS_SPIN_LOCK          RxDMATransListLock;

    NDIS_HANDLE             RecvNblPoolHandle;

	
    //
    // NIC configuration
    // -------------------------------------------------------------------------
    //
    ULONG                   PacketFilter;
    ULONG                   ulLookahead;
    ULONG64                 ulLinkSendSpeed;
    ULONG64                 ulLinkRecvSpeed;
    ULONG                   ulMaxBusySends;
    ULONG                   ulMaxBusyRecvs;
	ULONG					LinkState;

    // multicast list
    ULONG                   ulMCListSize;
    UCHAR                   MCList[NIC_MAX_MCAST_LIST][NIC_MACADDR_SIZE];

    //
    // Statistics
    // -------------------------------------------------------------------------
    //

    // Packet counts
    ULONG64                 FramesRxDirected;
    ULONG64                 FramesRxMulticast;
    ULONG64                 FramesRxBroadcast;
    ULONG64                 FramesTxDirected;
    ULONG64                 FramesTxMulticast;
    ULONG64                 FramesTxBroadcast;

    // Byte counts
    ULONG64                 BytesRxDirected;
    ULONG64                 BytesRxMulticast;
    ULONG64                 BytesRxBroadcast;
    ULONG64                 BytesTxDirected;
    ULONG64                 BytesTxMulticast;
    ULONG64                 BytesTxBroadcast;

    // Count of transmit errors
    ULONG                   TxAbortExcessCollisions;
    ULONG                   TxLateCollisions;
    ULONG                   TxDmaUnderrun;
    ULONG                   TxLostCRS;
    ULONG                   TxOKButDeferred;
    ULONG                   OneRetry;
    ULONG                   MoreThanOneRetry;
    ULONG                   TotalRetries;
    ULONG                   TransmitFailuresOther;

    // Count of receive errors
    ULONG                   RxCrcErrors;
    ULONG                   RxAlignmentErrors;
    ULONG                   RxResourceErrors;
    ULONG                   RxDmaOverrunErrors;
    ULONG                   RxCdtFrames;
    ULONG                   RxRuntErrors;

    //
    // Reference to the allocated root of MP_ADAPTER memory, which may not be cache aligned. 
    // When allocating, the pointer returned will be UnalignedBuffer + an offset that will make
    // the base pointer cache aligned.
    //
    PVOID                   UnalignedAdapterBuffer;
    ULONG                   UnalignedAdapterBufferSize;

    //
    // An OID request that could not be fulfulled at the time of the call. These OIDs are serialized
    // so we will not receive new queue management OID's until this one is complete. 
    // Currently this is used only for freeing a Queue (which may still have outstanding references)
    //
    PNDIS_OID_REQUEST PendingRequest;

} MP_ADAPTER, *PMP_ADAPTER;

#define MP_ADAPTER_FROM_CONTEXT(_ctx_) ((PMP_ADAPTER)(_ctx_))


typedef struct _WDF_DEVICE_INFO {
    PMP_ADAPTER		Adapter;
} WDF_DEVICE_INFO, *PWDF_DEVICE_INFO;    

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_DEVICE_INFO, GetWdfDeviceInfo)


// Get a pointer to an DMA Transaction Object from an NBL
#define DMATRANS_FROM_NBL(_NBL) (*((PDATA_TRANSFER_PARAMS *)&(_NBL)->MiniportReserved[0]))
#define COMMON_BUFFER_FROM_NBL(_NBL) (*((WDFCOMMONBUFFER *)&(_NBL)->MiniportReserved[1]))

//
// Functions specific - Network send structure required for each Tx DMA Transaction
//
typedef struct _FS_NETWORK_SEND {
    PVOID					NET_BUFFER_LIST_PTR;	// Network Only- Pointer to the Net Buffer List
	LONG					SystemSGArrayIndex;		// System allocated array index
	PSCATTER_GATHER_LIST	SystemSGArray[MAX_NUMBER_TX_SG_ELEMENTS]; // System allocated S/G pointers array
} FS_NETWORK_SEND, *PFS_NETWORK_SEND;

//
// Functions specific - Network receive structure required for each Rx DMA Transaction
//
typedef struct _FS_NETWORK_RECV {
    PVOID					NET_BUFFER_LIST_PTR;	// Network Only- Pointer to the Net Buffer List
	PVOID					RecvFrameBufferVa;		// Start of buffer address
} FS_NETWORK_RECV, *PFS_NETWORK_RECV;


// NDIS Declarations


// Function prototypes

// Functions located in XNet_Utils.c

//static
NDIS_STATUS
NICAllocAdapter(
    __in NDIS_HANDLE MiniportAdapterHandle, 
    __deref_out PMP_ADAPTER *Adapter);

void
NICFreeAdapter(
    __in  PMP_ADAPTER Adapter);

NDIS_STATUS 
NICReadRegParameters(
    PMP_ADAPTER Adapter);

VOID
NICIndicateNewLinkSpeed(
	PMP_ADAPTER	pAdapter,
	ULONG64		linkSpeed,
	ULONG		linkState);

// Functions located in XNet_Link.c

NDIS_STATUS
XNetLinkToXDMA(
    NDIS_HANDLE		MiniportAdapterHandle,
    PMP_ADAPTER		Adapter);

NTSTATUS
XNetInitSend(
    IN PMP_ADAPTER			Adapter);

NTSTATUS
XNetInitRecv(
    IN PMP_ADAPTER			Adapter);

VOID
XNetShutdown(
    IN PMP_ADAPTER			Adapter);

NDIS_STATUS
AllocRxDMA(
	PMP_ADAPTER Adapter,
    ULONG NumberRxPackets);


// Functions located in XNet_SndRec.c
VOID
XNetSendComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans);

VOID
XNetRecvComplete(
    IN FSD_HANDLE               FSDHandle,
    IN PDATA_TRANSFER_PARAMS    pDMATrans);


NDIS_STATUS
TXNblReference(
    __in  PMP_ADAPTER       Adapter,
    __in  PNET_BUFFER_LIST  NetBufferList);

VOID
TXNblRelease(
    __in  PMP_ADAPTER       Adapter,
    __in  PNET_BUFFER_LIST  NetBufferList,
    __in  BOOLEAN           fAtDispatch);

/*
 * Initialization functions in xxgeethernet.c
 */
void XXgEthernet_Start(PMP_ADAPTER Adapter);
void XXgEthernet_Stop(PMP_ADAPTER Adapter);
void XXgEthernet_Reset(PMP_ADAPTER Adapter);


NTSTATUS XXgEthernet_SetOptions(PMP_ADAPTER Adapter, ULONG Options);
NTSTATUS XXgEthernet_ClearOptions(PMP_ADAPTER Adapter, ULONG Options);
ULONG XXgEthernet_GetOptions(PMP_ADAPTER Adapter);

NTSTATUS XXgEthernet_SetMacAddress(PMP_ADAPTER Adapter, void *AddressPtr);
void XXgEthernet_GetMacAddress(PMP_ADAPTER Adapter, void *AddressPtr);

NTSTATUS XXgEthernet_SetMacPauseAddress(PMP_ADAPTER Adapter,
							void *AddressPtr);
void XXgEthernet_GetMacPauseAddress(PMP_ADAPTER Adapter,
							void *AddressPtr);
int XXgEthernet_SendPausePacket(PMP_ADAPTER Adapter, USHORT PauseValue);


void XXgEthernet_SetBadFrmRcvOption(PMP_ADAPTER Adapter);
void XXgEthernet_ClearBadFrmRcvOption(PMP_ADAPTER Adapter);

void XXgEthernet_DisableControlFrameLenCheck(PMP_ADAPTER Adapter);
void XXgEthernet_EnableControlFrameLenCheck(PMP_ADAPTER Adapter);

void XXgEthernet_PhySetMdioDivisor(PMP_ADAPTER Adapter, UCHAR Divisor);
void XXgEthernet_PhyRead(PMP_ADAPTER Adapter, ULONG PhyAddress,
					ULONG RegisterNum, USHORT *PhyDataPtr);
void XXgEthernet_PhyWrite(PMP_ADAPTER Adapter, ULONG PhyAddress,
			 		ULONG RegisterNum, USHORT PhyData);


