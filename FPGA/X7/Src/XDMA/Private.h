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

    Private.h

Abstract: 
 
    This file defines all the driver specific structures, defines, typedefs,
    function prototypes and WDF declarations. 

Environment:

    Kernel mode

--*/


#ifndef _XDMA_PRIVATE_H_
#define _XDMA_PRIVATE_H_

#define	TRACE_ENABLED		0

//#define	USE_HW_INTERRUPTS	1

#define XDMA_VERSION        ((VER_MAJOR_NUM << 16) | (VER_MINOR_NUM << 8) | VER_SUBMINOR_NUM)

/**************************** Type Definitions *******************************/
// Tunable Parameters:
#define MAX_SUPPORTED_DMA_ENGINES   16  /**< Maximum number of DMA Engines the driver supports */
#define DEFAULT_NUMBER_DMA_DESCRIPTORS  2999   /**< Default number of DMA Descriptors to allocate for each DMA Engine */

/** Buffer size for our internal test buffer used only for performance testing */
#define	DEFAULT_INTERNAL_BUFFER_SIZE	65536

/* Structures to store statistics - the latest 100 */
#define	MAX_STATS					10
#define	STAT_TIMER_PERIOD			990		// MS between each timer stat collection

// Define the maximum single DMA Transfer size to be 67,108,864 bytes
#define	MAXIMUM_DMA_TRANSFER_LENGTH 16384 * PAGE_SIZE

// Driver Linkage parameters:
#define MAX_ID_LEN								80      // Max size of the driver strings
#define LOCAL_ID                                0x409   // US English

// "Child Driver Configuration"
// 0	= "2 xRawEth"
// 1	= "2 XBlock"
// 2	= "2 XNet"
#define	CHILD_DRIVER_CONFIG_XRAWETH			0
#define	CHILD_DRIVER_CONFIG_XBLOCK			1
#define	CHILD_DRIVER_CONFIG_2_XNET			2
#define	CHILD_DRIVER_CONFIG_4_XNET			3
#define CHILD_DRIVER_CONFIG_4_XBLOCK        4
#define CHILD_DRIVER_CONFIG_1_XBLOCK        5
#define	CHILD_DRIVER_CONFIG_DEFAULT			CHILD_DRIVER_CONFIG_1_XBLOCK
#define	CHILD_DRIVER_CONFIG_MAX				6
#define	MAX_CHILD_DRIVER_DEVICES			6		// Maximum number of different child driver types

#define	RAW_ETHERNET_DEFAULT				0
#define	RAW_ETHERNET_MAX					1

#define	TEST_CONFIG_DEFAULT					1
#define	TEST_CONFIG_NORMAL_OPERATION		0
#define	TEST_CONFIG_INTERNAL_BUFFER			1
#define	TEST_CONFIG_MAX						1

#define MAXRAWPKTSIZE		(4 * PAGE_SIZE - 1)
#define MAXPKTSIZE			(8 * PAGE_SIZE)

#define MINPKTSIZE			(64)

#define     APP0_WRITE_DMA_ENGINE	0
#define     APP0_READ_DMA_ENGINE	4

#define     APP1_WRITE_DMA_ENGINE	1
#define     APP1_READ_DMA_ENGINE	5

#define     APP2_WRITE_DMA_ENGINE   2
#define     APP2_READ_DMA_ENGINE    6

#define     APP3_WRITE_DMA_ENGINE   3
#define     APP3_READ_DMA_ENGINE    7

#define	MAX_NUMBER_APPS				4
#define	DEFAULT_APP					1

#define	NUMBER_CONCURRENT_READS		10

#define BD_COALESC_COUNT   50

#define XILINX_GEN3_SPEED    4
#define STD_GEN3_SPEED_REPRESENTATION 3

typedef struct _APP_TABLE_ENTRY
{
    unsigned char	WRITE_ENGINE;
    unsigned char	READ_ENGINE;
}APP_TABLE_ENTRY, *PAPP_TABLE_ENTRY;

static const APP_TABLE_ENTRY APP_TABLE[MAX_NUMBER_APPS] =
{
	{APP0_WRITE_DMA_ENGINE,	APP0_READ_DMA_ENGINE},
	{APP1_WRITE_DMA_ENGINE,	APP1_READ_DMA_ENGINE},
	{APP2_WRITE_DMA_ENGINE, APP2_READ_DMA_ENGINE},
	{APP3_WRITE_DMA_ENGINE, APP3_READ_DMA_ENGINE}
};


/** @name DMA_ENGINE_EXTENSION: Each DMA Engine is allocated one of these 
 * structures to maintain state of the DMA Engine.
 * @{
 */
typedef struct _DMA_ENGINE_EXTENSION {
    PVOID                   pDeviceExtension;       /**< Back pointer to the Device Extension */
    unsigned long           DMAEngineNumber;
    unsigned long           InternalDMAEngineNumber;
    WDF_DMA_DIRECTION       DMADirection;

   volatile PDMA_ENGINE_REGISTERS  pDMAEngRegs;    /**< Virtual base address of DMA engine */

    ULONG                   EngineState;            /**< State of the DMA engine */

    WDFDMAENABLER           DMAEnabler32Bit;        // DMA Descriptors must live in the first 4GB

    WDFDMAENABLER           FSDDMAEnabler;          // Used by the FSD to create SG Lists.

    WDFQUEUE                DMATransactionQueue;

    LONG                    TotalNumberDescriptors;
    LONG                    AvailNumberDescriptors;

    WDFCOMMONBUFFER         DMADescriptorBuffer;
    size_t                  DMADescriptorBufferSize;
    PHYSICAL_ADDRESS        DMADescriptorBufferPhysAddr;

    PDMA_DESCRIPTOR         pDMADescriptorBase;
    PDMA_DESCRIPTOR         pNextDMADescriptor;
    PHYSICAL_ADDRESS        pNextDMADescriptorPhysAddr;
    PDMA_DESCRIPTOR         pTailDMADescriptor;

    WDFDPC                  CompletionDPC;          /**< Individual Completion DPC for this DMA Engine */
    WDFSPINLOCK             QueueHeadLock;
    WDFSPINLOCK             QueueTailLock;
    WDFSPINLOCK             RequestLock;			// Lock for DMA Register & Unregister

    PVOID  	                FSDContext;
    VOID                (* FSDCompletionFunc)(FSD_HANDLE, PDATA_TRANSFER_PARAMS);

	int						ScalingFactor; 

	DMAStatistics			DStats[MAX_STATS];
	SWStatistics			SStats[MAX_STATS];

	LONG					InterruptsPerSecond;
	LONG					DPCsPerSecond;

	LONG					dstatsRead;
	LONG					dstatsWrite;
	LONG					dstatsNum;
	LONG					sstatsRead;
	LONG					sstatsWrite;
	LONG					sstatsNum;
	LONG					SWrate;
	ULONG                   IntrBDCount;
	ULONG                   PktCoalesceCount;
	int						BDerrs;

	// Stats, counters, mode controls and other information
	unsigned int 			MaxPktSize;
	unsigned int 			MinPktSize;
	unsigned int 			TestMode;			/**< Current Test Mode settings */

    WDFCOMMONBUFFER         InternalDMABuffer;
    size_t                  InternalDMABufferSize;
    PHYSICAL_ADDRESS        InternalDMABufferPhysAddr;
    PBYTE			        pInternalDMABufferBase;
	BOOLEAN					bInternalTestMode;

	PDATA_TRANSFER_PARAMS   pXferParams; /* To handle the case where for a DMA engine, SOP is handled for a packet but EOP has not yet completed.*/
} DMA_ENGINE_EXTENSION, *PDMA_ENGINE_EXTENSION;


/** @name BAR_PARAMS: Each Base Address Register presented by
 * the PCI configuration space is represented in one of the 
 * entries in this structure. 
 * @{ 
 */
typedef struct _BAR_PARAMS{
    PHYSICAL_ADDRESS BAR_PHYS_ADDR; /**< Base physical address of BAR memory window */
    unsigned long BAR_LENGTH;       /**< Length of BAR memory window */
    PVOID   BAR_VIRT_ADDR;          /**< Virtual Address of mapped BAR memory window */
} BAR_PARAMS, *PBAR_PARAMS;

#if 1
typedef struct _POLL_PARAMS{
	WDFTIMER TimerHandle;
	int      engineIdxToPoll;
}POLL_PARAMS;
#endif

/** @name DEVICE_EXTENSION: Each PCI Express adapter
 * installed in the system is allocated one of these structures
 * to maintain state of the Adapter. 
 * @{ 
 */
typedef struct _DEVICE_EXTENSION {
    WDFDEVICE               Device;
    PDEVICE_OBJECT          PDO;        /* Physical Device Object */
    PDEVICE_OBJECT          FDO;        /* Functional Device Object */
	BUS_INTERFACE_STANDARD	PCIInterface;	/* Required for reading PCI Config */

    int                     NumberBARs; /* Number of Memory BARs found */
    BAR_PARAMS              BARInfo[MAX_BARS]; /* Information on each BAR for an adapter */

    volatile PDMA_REGISTER_MAP   pDMARegisters; /* Pointer to the first DMA Engine Registers */
	PUSER_SPACE_MAP			pUserSpace;			/* Pointer to the BAR0 User Space */

    int                     NumberDMAEngines;   /* The number of DMA Engines found on this adapter */
    PDMA_ENGINE_EXTENSION   pDMAExt[MAX_SUPPORTED_DMA_ENGINES]; /* Array of pointers to each DMA_ENGINE_EXTENSION */
                                    
    WDFINTERRUPT            Interrupt;      /* Returned by InterruptCreate */
    size_t					MaximumDMADescriptorPoolSize;

    WDFQUEUE                IoctlQueue;

    BOOLEAN                 StaticEnumerationDone; /* Avoid duplicate calls for rebalance test */
    WDFWAITLOCK             ChildLock;
    REGISTER_XDRIVER        DriverLink;

	int						RawEthernet;		/* Indicates Raw Therrnet mode if set. */
	int						TestConfig;			// Test Mode configuration, i.e. Internal, Application, etc.

#if 1 // ndef USE_HW_INTERRUPTS
	WDFTIMER				PollTimerHandle;
#endif // USE_HW_INTERRUPTS

	WDFTIMER				TimerHandle;		/** To keep track of the stats polling timer */

#if 1
	POLL_PARAMS             PollEngineHandle[MAX_SUPPORTED_DMA_ENGINES]; /* Separate poll timer for each engine */
#endif
	
	// Cache the config and capabilities for ioctl queries.
	PCI_CONFIG_SPACE		PciCfg;				/* Copy of the devices PCI config space */
    int						LinkSpeed;			/**< Link Speed */
    int						LinkWidth;			/**< Link Width */
	int						LinkUpCap;          /**< Link up configurable capability */
    int						IntMode;            /**< Legacy or MSI interrupts */
    int						MPS;                /**< Max Payload Size */
    int						MRRS;               /**< Max Read Request Size */

	int						ScalingFactor; 

	TRNStatistics			TStats[MAX_STATS];
	LONG					tstatsRead; 
	LONG					tstatsWrite;
	LONG					tstatsNum;
}  DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// This will generate the function named XlxGetDeviceContext to be use for
// retreiving the DEVICE_EXTENSION pointer.
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, XlxGetDeviceContext)


// The context structure used with the DMA Engine DPC
typedef struct _DPC_CONTEXT{
    PDMA_ENGINE_EXTENSION   pDMAExt;
} DPC_CONTEXT, *PDPC_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DPC_CONTEXT, DPCContext)


/*
 * The device extension for the PDOs. The xilinx device 
 *   which this bus driver enumerates.
 */
typedef struct _PDO_DEVICE_DATA
{
    /* Unique serail number of the device on the bus */
    ULONG           SerialNo;
} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, PdoGetData)

//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD XlxEvtDeviceAdd;
//#pragma alloc_text("PAGED_CODE", XlxEvtDeviceAdd);
EVT_WDF_OBJECT_CONTEXT_CLEANUP XlxEvtDriverContextCleanup;
//#pragma alloc_text("PAGED_CODE", XlxEvtDriverContextCleanup);
EVT_WDF_DEVICE_D0_ENTRY XlxEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT XlxEvtDeviceD0Exit;
//#pragma alloc_text("PAGED_CODE", XlxEvtDeviceD0Exit);
EVT_WDF_DEVICE_PREPARE_HARDWARE XlxEvtDevicePrepareHardware;
//#pragma alloc_text("PAGED_CODE", XlxEvtDevicePrepareHardware);
EVT_WDF_DEVICE_RELEASE_HARDWARE XlxEvtDeviceReleaseHardware;
//#pragma alloc_text("PAGED_CODE", XlxEvtDeviceReleaseHardware);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  XlxEvtIoControl;

//#ifdef USE_HW_INTERRUPTS
EVT_WDF_INTERRUPT_ISR XlxEvtInterruptIsr;
EVT_WDF_INTERRUPT_ENABLE XlxEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE XlxEvtInterruptDisable;
EVT_WDF_DPC XlxDMADPC;
//#endif // USE_HW_INTERRUPTS


NTSTATUS
XlxInitializeDeviceExtension(
    IN PDEVICE_EXTENSION DevExt
    );
//#pragma alloc_text("PAGED_CODE", XlxInitializeDeviceExtension);

#if 0
NTSTATUS
XlxReadPciConfigSpace(
    IN PDEVICE_EXTENSION DevExt
    );
#endif

NTSTATUS
XlxPrepareHardware(
    IN PDEVICE_EXTENSION DevExt,
    IN WDFCMRESLIST     ResourcesTranslated
    );
//#pragma alloc_text("PAGED_CODE", XlxPrepareHardware);

// Functions in XDMA_Ioctl.c

// Functions in XDMA_Link.c
NTSTATUS
XlxCreateDriverToDriverInterface(
    IN PDEVICE_EXTENSION DevExt
    );

//
// WDFINTERRUPT Support
//
// Functions in XDMA_ISR.c
//#ifdef USE_HW_INTERRUPTS
NTSTATUS
XlxInterruptCreate(
    IN PDEVICE_EXTENSION DevExt
    );
//#else
EVT_WDF_TIMER	XDMAPollTimer;	

VOID
XDMAPollTimer(
   IN WDFTIMER	Timer
   );
#if 1
EVT_WDF_TIMER XDMAPollEngineTimer;

VOID
XDMAPollEngineTimer(
   IN WDFTIMER	Timer
   );
#endif
//#endif // USE_HW_INTERRUPTS

// Functions in XDMA_Init.c
NTSTATUS
XlxInitializeHardware(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
XlxInitializeDMAEngines(
    IN PDEVICE_EXTENSION DevExt
    );
//#pragma alloc_text("PAGED_CODE", XlxInitializeDMAEngines);

NTSTATUS
XlxD0Startup(
    IN PDEVICE_EXTENSION DevExt
    );

NTSTATUS
XlxReleaseHardware(
    IN PDEVICE_EXTENSION pDevExt
    );

NTSTATUS
XlxD0Shutdown(
    IN PDEVICE_EXTENSION DevExt
    );

VOID
XlxHardwareReset(
    IN PDEVICE_EXTENSION    DevExt
    );

// Functions in XDMA_Xfer.c

NTSTATUS
XlxDataTransfer(
    IN XDMA_HANDLE              XDMAHandle, 
    IN PDATA_TRANSFER_PARAMS    XferParams
    );

NTSTATUS
InitializeDMADescriptors(
    IN PDMA_ENGINE_EXTENSION    pDMAExt
    );

NTSTATUS
SetupInternalDMAXfers(
    IN PDMA_ENGINE_EXTENSION    pDMAExt,
	unsigned int				packetSize
    );

NTSTATUS
XlxCancelTransfers(
    IN XDMA_HANDLE              XDMAHandle
    );

void
XlxResetDMAEngine(
    IN PDMA_ENGINE_EXTENSION    pDMAExt
	);

// Functions in XDMA_Timer.c
EVT_WDF_TIMER	XDMAStatsTimer;	

VOID 
XDMAStatsTimer(
   IN WDFTIMER	Timer
   );

#pragma warning(disable:4127) // avoid conditional expression is constant error with W4

#endif  // _XDMA_PRIVATE_H_


