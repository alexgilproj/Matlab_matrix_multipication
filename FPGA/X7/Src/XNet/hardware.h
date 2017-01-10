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
* @file hardware.h
*
*    This module defines constants that describe physical characteristics and
*    limits of the underlying hardware. 
*
* MODIFICATION HISTORY:
*
* Ver     Date   Changes
* ----- -------- -------------------------------------------------------
* 1.0   11/18/12 First release 
*
******************************************************************************/


#ifndef _HARDWARE_H
#define _HARDWARE_H

//
// Link layer addressing
// -----------------------------------------------------------------------------
//

// Number of bytes in a hardware address.  Ethernet uses 6 byte addresses.
#define NIC_MACADDR_SIZE                   ETH_LENGTH_OF_ADDRESS

// True iff the given address is a multicast address
#define NIC_ADDR_IS_MULTICAST(_addr)       ETH_IS_MULTICAST(_addr)

// True iff the given address is a broadcast address
#define NIC_ADDR_IS_BROADCAST(_addr)       ETH_IS_BROADCAST(_addr)

// Copies a hardware address from _src to _dest
#define NIC_COPY_ADDRESS(_dest,_src)       ETH_COPY_NETWORK_ADDRESS(_dest,_src)

// True iff the given address was assigned by the local administrator
#define NIC_ADDR_IS_LOCALLY_ADMINISTERED(_addr) \
        (BOOLEAN)(((PUCHAR)(_addr))[0] & ((UCHAR)0x02))

// True iff two addresses are equal
#define NIC_ADDR_EQUAL(_a,_b) \
        ((*(ULONG UNALIGNED *)&(_a)[2] == *(ULONG UNALIGNED *)&(_b)[2]) \
        && (*(USHORT UNALIGNED *)(_a) == *(USHORT UNALIGNED *)(_b)))



//
// Frames
// -----------------------------------------------------------------------------
//

#define HW_FRAME_HEADER_SIZE				14
#define HW_FRAME_HEADER_VLAN_SIZE			18	/* Size of an Ethernet header with VLAN */
#define HW_FRAME_MAX_DATA_SIZE				1500
#define HW_MIN_FRAME_SIZE					60

#define HW_JUMBO_FRAME_DATA_SIZE			8192	/* Max data size of a jumbo Ethernet frame */
#define HW_TRL_SIZE							4	/* Size of an Ethernet trailer (FCS) */

#define HW_MAX_FRAME_SIZE					(HW_FRAME_HEADER_SIZE + HW_FRAME_MAX_DATA_SIZE) // + HW_TRL_SIZE)
#define HW_MAX_VLAN_FRAME_SIZE				(HW_FRAME_HEADER_VLAN_SIZE + HW_FRAME_MAX_DATA_SIZE) // + HW_TRL_SIZE)
#define HW_MAX_JUMBO_FRAME_SIZE				(HW_FRAME_HEADER_SIZE + HW_JUMBO_FRAME_DATA_SIZE) // + HW_TRL_SIZE)
#define HW_MAX_JUMBO_VLAN_FRAME_SIZE		(HW_FRAME_HEADER_VLAN_SIZE + HW_JUMBO_FRAME_DATA_SIZE + HW_TRL_SIZE)


typedef struct tagNIC_FRAME_HEADER
{
    UCHAR  DestAddress[NIC_MACADDR_SIZE];
    UCHAR  SrcAddress[NIC_MACADDR_SIZE];
    UCHAR  EtherType[2];
} NIC_FRAME_HEADER, *PNIC_FRAME_HEADER;

C_ASSERT(sizeof(NIC_FRAME_HEADER) == HW_FRAME_HEADER_SIZE);

#define GET_DESTINATION_OF_FRAME(_dest, _frame) NdisMoveMemory(_dest, ((PNIC_FRAME_HEADER)(_frame))->DestAddress, NIC_MACADDR_SIZE)

//
// Medium properties
// -----------------------------------------------------------------------------
//

#define NIC_MEDIUM_TYPE                    NdisMedium802_3

// If you have physical hardware on 802.3, use NdisPhysicalMedium802_3.
// or NdisPhysicalMediumUnspecified
#define NIC_PHYSICAL_MEDIUM                NdisPhysicalMedium802_3 

// Set this value to TRUE if there is a physical adapter.
#define NIC_HAS_PHYSICAL_CONNECTOR         TRUE
#define NIC_ACCESS_TYPE                    NET_IF_ACCESS_BROADCAST
#define NIC_DIRECTION_TYPE                 NET_IF_DIRECTION_SENDRECEIVE
#define NIC_CONNECTION_TYPE                NET_IF_CONNECTION_DEDICATED

// This value must match the *IfType in the driver .inf file
#define NIC_IFTYPE                         IF_TYPE_ETHERNET_CSMACD

// Claim to be 100mbps duplex
#define MEGABITS_PER_SECOND                1000000ULL
#define NIC_XMIT_SPEED                     (100ULL*MEGABITS_PER_SECOND)
#define NIC_RECV_SPEED                     (100ULL*MEGABITS_PER_SECOND)

// Number of Receive packets to allocate for data reception
#define	DEFAULT_NUMBER_RX_RECEIVES			128

// Set the maximum number of Scatter/Gather elements allocted for each transfer
#define	MAX_NUMBER_TX_SG_ELEMENTS			30
#define	MAX_NUMBER_RX_SG_ELEMENTS			4

#define	SG_ARRAY_SIZE						MAX_NUMBER_TX_SG_ELEMENTS
//
// Hardware limits
// -----------------------------------------------------------------------------
//

// Max number of multicast addresses supported in hardware
#define NIC_MAX_MCAST_LIST                 32

// Maximum number of uncompleted sends that a single adapter will permit
#define NIC_MAX_BUSY_SENDS                 256

// Maximum number of unreturned receives that a single adapter will permit
#define NIC_MAX_BUSY_RECVS                 256



// Maximum number of send completes that will be processed per DPC.
#define NIC_MAX_SENDS_PER_DPC              64

//
// Maximum number of receives that will be processed per DPC.
// This constraints the amount of time spent for a single receive DPC. 
//
#define NIC_MAX_RECVS_PER_DPC              64

#define NIC_MAX_LOOKAHEAD                  HW_FRAME_MAX_DATA_SIZE
#define NIC_BUFFER_SIZE                    HW_MAX_FRAME_SIZE


// Simulated latency across the link.  If this is set to zero, the driver
// will saturate the link.  Unfortunately, when the "link" is simulated in CPU,
// that means that the CPU is saturated, and you hit DPC timeouts.
// (This throttling isn't needed with a physical NIC)
//#define NIC_SIMULATED_LATENCY              0 // in 100ns units

//
// Physical adapter properties
// -----------------------------------------------------------------------------
//

// The bus that connects the adapter to the PC.
// (Example: PCI adapters should use NdisInterfacePci).
#define NIC_INTERFACE_TYPE                 NdisInterfaceInternal

// Change to your company name instead of using Microsoft
#define NIC_VENDOR_DESC                    "Xilinx"

// Highest byte is the NIC byte plus three vendor bytes. This is normally
// obtained from the NIC.
#define NIC_VENDOR_ID                      0x0000AA00


// Wakeup capabilities, as in OID_PNP_CAPABILITIES
#define NIC_MAGIC_PACKET_WAKEUP            NdisDeviceStateUnspecified
#define NIC_PATTERN_WAKEUP                 NdisDeviceStateUnspecified
#define NIC_LINK_CHANGE_WAKEUP             NdisDeviceStateUnspecified


#define NIC_SUPPORTED_FILTERS ( \
                NDIS_PACKET_TYPE_DIRECTED   | \
                NDIS_PACKET_TYPE_MULTICAST  | \
                NDIS_PACKET_TYPE_BROADCAST  | \
                NDIS_PACKET_TYPE_PROMISCUOUS | \
                NDIS_PACKET_TYPE_ALL_MULTICAST)

//
// This sample is a virtual device, so it can tolerate surprise removal
// and suspend.  Ensure the correct flags are set for your hardware.
//
// If your hardware supports busmaster DMA, you must specify
// NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER. Our virtual miniport will 
// not be allocating hardware resources such as interrupts, so we set the 
// WDM attribute.  
//
#define NIC_ADAPTER_ATTRIBUTES_FLAGS (NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM)

//
// Specify a bitmask that defines optional properties of the NIC.
// This miniport indicates receive with NdisMIndicateReceiveNetBufferLists
// function.  Such a driver should set this NDIS_MAC_OPTION_TRANSFERS_NOT_PEND
// flag.
//
// NDIS_MAC_OPTION_NO_LOOPBACK tells NDIS that NIC has no internal
// loopback support so NDIS will manage loopbacks on behalf of
// this driver.
//
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA tells the protocol that
// our receive buffer is not on a device-specific card. If
// NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA is not set, multi-buffer
// indications are copied to a single flat buffer.
//
#define NIC_MAC_OPTIONS (\
                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | \
                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  | \
                NDIS_MAC_OPTION_NO_LOOPBACK)

// NDIS 6.x miniports must support all counters in OID_GEN_STATISTICS.
#define NIC_SUPPORTED_STATISTICS (\
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV   | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV              | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS           | \
                NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR              | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT  | \
                NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR             | \
                NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS          | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV     | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_RCV    | \
                NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT    | \
                NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_XMIT   | \
                NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_XMIT)


#endif // _HARDWARE_H


