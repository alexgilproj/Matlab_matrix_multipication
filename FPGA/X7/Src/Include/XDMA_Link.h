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

    XDMA_Link.h

Abstract:

    This file contains the defines, structures for linking drivers.

Environment:

    Kernel mode
 
--*/

#ifndef __XDMA_LINK_H_
#define __XDMA_LINK_H_


/**************************** Type Definitions *******************************/

typedef PVOID       XDMA_HANDLE;
typedef PVOID       FSD_HANDLE;

#define INVALID_XDMA_HANDLE     NULL

#define LOOPBACK            0x00000002	/* Enable TX data loopback onto RX */
#define RAW_DESIGN_MODE		0x00000000
#define PERF_DESIGN_MODE	0x00000003
/* Test start / stop conditions */
#define PKTCHKR             0x00000001	/* Enable TX packet checker */
#define PKTGENR             0x00000001	/* Enable RX packet generator */

#define BURST_SIZE			256 

#define MAX_SUPPORTED_DMA_ENGINES   16  /**< Maximum number of DMA Engines the driver supports */

#define FIRST_S2C_DMA_ENGINE		0
#define CS2_DMA_ENGINE_OFFSET		4

#define	DESIGN_AND_STATUS_REGISTER_BLOCK_SIZE 0x100 /**< Size of User Register Design and Status area */
#define USER_REGISTER_RESERVED0_SIZE     0x1000   /**< Size of the User Application Reserved0 space */

#define DESIGN_AND_STATUS_REGISTERS_RESERVED_SIZE 0x0008 /**<Size of Design and Status Registers Reserved Space */

#define	APPLICATION_REGISTER_BLOCK_SIZE	0x100	/**< Size of the Application specific register space */

enum {
    NOT_INITIALIZED = 0,
    SHUTDOWN,
    INITIALIZING,
    FAILED,
    AVAILABLE,
	INTERNAL,
    REGISTERED
} DMA_ENGINE_STATE;

#define PCI_BAR_ADDRESSES			6

/** @name PCI Configuration Space
  */
typedef struct _PCI_CONFIG_SPACE {
    unsigned short  VENDOR_ID;
    unsigned short  DEVICE_ID;
    unsigned short  COMMAND;
    unsigned short  STATUS;
    unsigned char   REVISION_ID;
    unsigned char   PROG_IF;
    unsigned char   SUBCLASS;       
    unsigned char   BASECLASS;
    unsigned char   CACHE_LINE_SIZE;
    unsigned char   LATENCY_TIMER;
    unsigned char   HEADER_TYPE;
    unsigned char   BIST;
	unsigned long   BASEADDRESSES[PCI_BAR_ADDRESSES];
    unsigned long   CIS;
    unsigned short  SUBVENDOR_ID;
    unsigned short  SUBSYSTEM_ID;
    unsigned long   ROM_BASE_ADDRESS;
    unsigned char   CAPABILITIES_PTR;
    unsigned char   Reserved1[3];
    unsigned long   Reserved2;
    unsigned char   INTERRUPT_LINE;
    unsigned char   INTERRUPT_PIN;
    unsigned char   MINIMUM_GRANT;
    unsigned char   MAXIMUM_LATENCY;
} PCI_CONFIG_SPACE, *PPCI_CONFIG_SPACE;

#ifndef _NDIS_
typedef struct _NET_BUFFER
{
	unsigned int	NO_USED;
} NET_BUFFER, *PNET_BUFFER;
#endif // Not defined NET_BUFFER

#pragma warning(disable:4214)  // Bit fields other than int warning

/** @name User Application Registers structure (starts at 0x8000)
  */
typedef struct _DESIGN_AND_STATUS_REGISTERS
{
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     MINOR_VERSION:4;      // Bits 0 - 3
            unsigned long     MAJOR_VERSION:4;      // Bits 4 - 7
            unsigned long     NWL_VERSION:8;        // Bits 8 - 15
            unsigned long     DEVICE:4;             // Bits 16 - 19
        } BIT;
    } DESIGN_VERSION;								// Offset 0x9000
	unsigned long     DESIGN_MODE_ADDRESS;			// Offset 0x9004
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     DDR3_MEMORY_INIT_COMPLETE:1; // Bit 0
            unsigned long     :1;			               // Bit 1
            unsigned long     DDR3_FIFO_EMPTY:8;           // Bits 2 - 9
			unsigned long	  :16;				           // Bits 10 - 25
			unsigned long	  XPHY0:1;				       // Bit 26
			unsigned long	  XPHY1:1;				       // Bit 27
			unsigned long	  XPHY2:1;				       // Bit 28
			unsigned long	  XPHY3:1;				       // Bit 29
			unsigned long     DDR3_SODIMMA:1;              // Bit 30
            unsigned long     DDR3_SODIMMB:1;              // Bit 31
			} BIT;
    }DESIGN_STATUS;									// Offset 0x9008
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SAMPLE_COUNT:2;		// Bits 0 - 1
            unsigned long     BYTE_COUNT:30;		// Bits 4 - 31
        } BIT;
    }TRANSIMT_UTILIZATION;							// Offset 0x900C
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SAMPLE_COUNT:2;		// Bits 0 - 1
            unsigned long     BYTE_COUNT:30;		// Bits 4 - 31
        } BIT;
    }RECEIVE_UTILIZATION;							// Offset 0x9010
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SAMPLE_COUNT:2;		// Bits 0 - 1
            unsigned long     BYTE_COUNT:30;		// Bits 4 - 31
        } BIT;
    }UPSTREAM_MEMORY_WRITE_BYTE_COUNT;				// Offset 0x9014
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SAMPLE_COUNT:2;		// Bits 0 - 1
            unsigned long     BYTE_COUNT:30;		// Bits 4 - 31
        } BIT;
    }DOWNSTREAM_COMPLETION_BYTE_COUNT;				// Offset 0x9018
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_CD:12;		// Bits 0 - 11
        } BIT;
    }INITIAL_COMPLETION_DATA_CREDITS;				// Offset 0x901C
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_CH:8;			// Bits 0 - 7
        } BIT;
    }INITIAL_COMPLETION_HEADER_CREDITS;				// Offset 0x9020
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_NPD:12;		// Bits 0 - 11
        } BIT;
    }INITIAL_NPD_CREDITS;							// Offset 0x9024
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_NPH:8;		// Bits 0 - 7
        } BIT;
    }INITIAL_NPH_CREDITS;							// Offset 0x9028
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_PD:12;		// Bits 0 - 11
        } BIT;
    }INITIAL_PD_CREDITS;							// Offset 0x902C
	union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INIT_FC_PH:8;			// Bits 0 - 7
        } BIT;
    }INITIAL_PH_CREDITS;							// Offset 0x9030
	union
	{
	    unsigned long ULONG;
		struct
		{
		    unsigned long SCALING_FACTOR:2;         //Bits 0-1
		} BIT;
	}PERF_SCALING_FACTOR;                           // Offset 0x9034
} DESIGN_AND_STATUS_REGISTERS, *PDESIGN_AND_STATUS_REGISTERS;

/** @name Power Monitoring Registers
 *        structure  Offset 0x9040
  */
typedef struct _POWER_MONITOR_REGISTERS
{
    unsigned long	VCCINT_PWR_CONS;     /* VCC */
	unsigned long	VCC2V5_PWR_CONS;
	unsigned long	VCCAUX_PWR_CONS;
	unsigned long   Reserved;
	unsigned long	MGT_AVCC_PWR_CONS;
	unsigned long	MGT_AVTT_PWR_CONS;
	unsigned long	VCC1V5_PWR_CONS;
	unsigned long	VCC3V3_PWR_CONS;
} POWER_MONITOR_REGISTERS, *PPOWER_MONITOR_REGISTERS;


/** @name Packed Design and Status Structure with reserved
 *        space.
  */
typedef struct _PACKED_DESIGN_AND_STATUS_REGISTERS
{
    DESIGN_AND_STATUS_REGISTERS		DESIGN_REGISTERS;
    unsigned char   Reserved[DESIGN_AND_STATUS_REGISTER_BLOCK_SIZE - (sizeof(DESIGN_AND_STATUS_REGISTERS))];
}PACKED_DESIGN_AND_STATUS_REGISTERS, *PPACKED_DESIGN_AND_STATUS_REGISTERS;


/** @name Application Generator/Checker/Loopback Registers
 *        structure.  Offsets 0x9100 (APP0) and 0x9200 (APP1)
  */
typedef struct _APPLICATION_REGISTERS
{
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     ENABLE:1;				// Bit 0
        } BIT;
    } TRAFFIC_GENERATOR;		// Offset 0x9x00
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     LENGTH:16;			// Bits 0, 15
        } BIT;
    } PACKET_LENGTH;			// Offset 0x9x04
    union
    {
		unsigned long   ULONG;
        struct 
        {
            unsigned long     CHECKER_ENABLE:1;		// Bit 0
            unsigned long     LOOPBACK_ENABLE:1;	// Bit 1
        } BIT;
    } LOOPBACK_CHECKER;			// Offset 0x9x08
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     ERROR:1;				// Bit 0      
        } BIT;
    } CHECKER_STATUS;			// Offset 0x9x0C
    unsigned long		SEQNO_WRAP_REG;		// Offset 0x9x10
} APPLICATION_REGISTERS, *PAPPLICATION_REGISTERS;

/** @name MAC Registers structure
 *     Offsets start from 0x9400
  */
typedef struct _XGEMAC_REGISTERS
{
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     PROMISCUOUS_MODE:1;				// Bit 0
			unsigned long     :30;                               //Bits 1- 30
			unsigned long     RECEIVE_FIFO_OVERFLOW_STATUS:1;    // Bit 31
        } BIT;
    } ADDRESS_FILTERING_CONTROL;
	unsigned long     MAC_LOWER_ADDRESS;
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     ADDRESS:16;			// Bits 0-15
        } BIT;
    } MAC_UPPER_ADDRESS;			
} XGEMAC_REGISTERS, *PXGEMAC_REGISTERS;

/** @name Memory Control (DDR3/AXI) Registers
 *        structure  Offset 0x9300
  */
typedef struct _MEMORY_CONTROL_REGISTERS
{
    unsigned long	START_ADDRESS;
	unsigned long	END_ADDRESS;
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SIZE:9;				// Bits 0, 8
        } BIT;
    } WRITE_BURST_SIZE;
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     SIZE:9;				// Bits 0, 9
        } BIT;
    } READ_BURST_SIZE;
} MEMORY_CONTROL_REGISTERS, *PMEMORY_CONTROL_REGISTERS;

#pragma warning(default:4214)

/** @name User Space Register memory map (usually located in BAR 0).
  */
typedef struct _USER_SPACE_MAP
{
	unsigned char Reserved[USER_REGISTER_RESERVED0_SIZE];				     // Offset 0x8000 - 0x9000

	DESIGN_AND_STATUS_REGISTERS				DESIGN_REGISTERS;                // Offset 0x9000
	unsigned char   Reserved2[DESIGN_AND_STATUS_REGISTERS_RESERVED_SIZE]; 
	POWER_MONITOR_REGISTERS					POWER_MONITOR;				     // Offset 0x9040
    unsigned char   Reserved3[DESIGN_AND_STATUS_REGISTER_BLOCK_SIZE - 
		(sizeof(DESIGN_AND_STATUS_REGISTERS) + sizeof(POWER_MONITOR_REGISTERS) + DESIGN_AND_STATUS_REGISTERS_RESERVED_SIZE)];
	APPLICATION_REGISTERS					APP0_REGISTERS;				     // Offset 0x9100
    unsigned char   Reserved4[APPLICATION_REGISTER_BLOCK_SIZE - (sizeof(APPLICATION_REGISTERS))];
	unsigned char   Reserved5[APPLICATION_REGISTER_BLOCK_SIZE * 0x02];		 // Offset 0x9200 - 0x9400
	XGEMAC_REGISTERS                       MAC0_REGISTERS;                   // Offset 0x9400 - 0x9408
	XGEMAC_REGISTERS                       MAC1_REGISTERS;                   // Offset 0x940C - 0x9414
	XGEMAC_REGISTERS                       MAC2_REGISTERS;                   // Offset 0x9418 - 0x9420
	XGEMAC_REGISTERS                       MAC3_REGISTERS;                   // Offset 0x9424 - 0x942C
}USER_SPACE_MAP, *PUSER_SPACE_MAP;


/************************* Structure Definitions *****************************/

#define	DMA_TRANS_STATUS_SHORT		1 << 25
#define	DMA_TRANS_STATUS_ERROR		1 << 28


typedef struct _DATA_TRANSFER_PARAMS {
    LIST_ENTRY              DT_LINK;				// General purpose Doubly Link List entry
	WDFDMATRANSACTION       TRANSACTION_PTR;		// Pointer back to the DMA Transaction object
    ULONG64                 CardAddress;			// Address of Card memory if applicable
    ULONG                   BytesTransferred;		// Number of Bytes transfered
    ULONG                   Status;					// Status of transfer
	PVOID					FS_PTR;					// Function specific pointer
	PSCATTER_GATHER_LIST    SG_LIST_PTR;			// Pointer to the Scatter/Gather table
} DATA_TRANSFER_PARAMS, *PDATA_TRANSFER_PARAMS;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DATA_TRANSFER_PARAMS, DMATransCtx)

typedef struct _REGISTER_DMA_ENGINE_REQUEST{
    ULONG           FSD_VERSION;					// Function Specific Driver version
    int      	    DMA_ENGINE;						// Requested DMA Engine number
    WDF_DMA_DIRECTION   DIRECTION;					// Requested DMA Direction, S2C or C2S
    PVOID	        XDMA_DEVICE_CONTEXT;			// The XDMA Driver context
    FSD_HANDLE      FSD_DEVICE_CONTEXT;				// The Function Specific Driver context
    VOID        (* FSDCompletion)(FSD_HANDLE, PDATA_TRANSFER_PARAMS);// Pointer to DMA Completion routine
    // More callback functions to be added as needed
} REGISTER_DMA_ENGINE_REQUEST, *PREGISTER_DMA_ENGINE_REQUEST;

typedef struct _REGISTER_DMA_ENGINE_RETURN{
    ULONG           XDMA_DMA_VERSION;				// XDMA Version
    int      	    DMA_ENGINE;						// DMA Engine
    ULONG      	    MAX_NUMBER_DMA_DESCRIPTORS;		// Number of total DMA Descriptors available
    ULONG           MAXIMUM_TRANSFER_LENGTH;		// Maximum single DAM Transfer size
    WDFDMAENABLER   DMAEnabler;						// DMA Enabler Object (used for Scatter/Gather operations)
    NTSTATUS    (* XDMADataTransfer)(XDMA_HANDLE, DATA_TRANSFER_PARAMS *); // Pointer to DMA Transfer function
    NTSTATUS    (* XDMACancelTransfer)(XDMA_HANDLE); // Pointer to DMA Cancel Transfer function
	PUSER_SPACE_MAP PUSER_SPACE;					// Pointer to User Space in BAR 0
    // More functions to be added as needed
} REGISTER_DMA_ENGINE_RETURN, *PREGISTER_DMA_ENGINE_RETURN;


typedef struct _REGISTER_XDRIVER{
    INTERFACE	    InterfaceHeader;				// Required by the OS- Must be first! 
    ULONG           XDMA_DRV_VERSION;				// Driver version info
    PVOID	        XDMA_DEVICE_CONTEXT;			// XDMA Reserved, used in RegisterDMA
    XDMA_HANDLE (* DmaRegister)(REGISTER_DMA_ENGINE_REQUEST *, REGISTER_DMA_ENGINE_RETURN *);
    int         (* DmaUnregister)(XDMA_HANDLE);
} REGISTER_XDRIVER, *PREGISTER_XDRIVER;

/************************** Function Prototypes ******************************/

#endif  // __XDMA_LINK_H_

