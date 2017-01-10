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

    XDMA_Ioctl.c

Abstract:
 
    Contains the Device I/O Control (ioctl) handler for the driver. 

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_Ioctl.tmh"
#endif // TRACE_ENABLED

// The following is used for Raw Ethernet mode
#define Xil_Out32(addr, data)   (WRITE_REGISTER_ULONG((PULONG)(addr), (data)))

#define MAC1_OFFSET 0x3000
#define MAC2_OFFSET 0x4000

#define XXGE_RCW1_OFFSET   0x00000404
#define XXGE_TC_OFFSET     0x00000408

#define RCV_ENABLE   0x50000000
#define TSMT_ENABLE  0x50000000

// Local function prototypes
//
void GetPCIStats(PDEVICE_EXTENSION pDevExt, PCIState * pPCIState, size_t Length);
void GetEngineStats(PDEVICE_EXTENSION pDevExt, EngState * pEngState, size_t Length);
void GetDMAStats(PDEVICE_EXTENSION pDevExt, EngStatsArray *	pStatsArray, size_t Length);
void GetTransStats(PDEVICE_EXTENSION pDevExt, TRNStatsArray * pTransArray, size_t Length);
void GetSWStats(PDEVICE_EXTENSION pDevExt, SWStatsArray * pSWArray, size_t Length);
NTSTATUS SetTestMode(IN PDEVICE_EXTENSION pDevExt, IN TestCmd * TC);
NTSTATUS XlxCancelRxTransfers(IN PDEVICE_EXTENSION pDevExt, IN INT *Engine);

void SetupInternalDMATransfers(PDMA_ENGINE_EXTENSION pDMAExt, unsigned int MaxPktSize);

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
/*++

Routine Description:

    Called by the framework as soon as it receives a DeviceIOControl request.

Arguments:

    Queue      - Default queue handle
    Request    - Handle to the write request (Irp)
    OutputBufferLength - Length of the OutputBuffer passed in the DeviceIOControl
                function (if any).
    InputBufferLength - Length of the InputBuffer passed in the DeviceIOControl
                function (if any).
    IoControlCode - The IOCTL Code passed in the DeviceIOControl function.
 
Return Value:

--*/
VOID 
XlxEvtIoControl(
    IN  WDFQUEUE    Queue,
    IN  WDFREQUEST  Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN  ULONG       IoControlCode
    )
{
    PDEVICE_EXTENSION	pDevExt;
    NTSTATUS			status = STATUS_UNSUCCESSFUL;
	size_t				completeByteCount = 0;

	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	//DEBUGP(DEBUG_TRACE, "--> XlxEvtIoControl: Request %p", Request);

    //
    // Get the DevExt from the Queue handle
    //
    pDevExt = XlxGetDeviceContext(WdfIoQueueGetDevice(Queue));
	if (pDevExt == NULL)
	{
        WdfRequestCompleteWithInformation(Request, STATUS_NO_SUCH_DEVICE, 0);
	}

    switch (IoControlCode)
    {
		case IGET_PCI_STATE:
		{
			PCIState * pPCIState;

			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PCIState), (PVOID *) &pPCIState, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				GetPCIStats(pDevExt, pPCIState, completeByteCount);
			}
		}
        break;
	
		case IGET_ENG_STATE:
		{
			EngState * pEngState;
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(EngState), (PVOID *) &pEngState, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				GetEngineStats(pDevExt, pEngState, completeByteCount);
			}
		}
        break;

		case IGET_DMA_STATISTICS:
		{
			EngStatsArray *	pStatsArray;
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(EngStatsArray), (PVOID *) &pStatsArray, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				GetDMAStats(pDevExt, pStatsArray, completeByteCount);
			}
		}
        break;

		case IGET_TRN_STATISTICS:
		{
			TRNStatsArray * pTransArray;
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(TRNStatsArray), (PVOID *) &pTransArray, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				GetTransStats(pDevExt, pTransArray, completeByteCount);
			}
		}
        break;

		case IGET_SW_STATISTICS:
		{
			SWStatsArray * pSWStatsArray;
			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SWStatsArray), (PVOID *) &pSWStatsArray, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				GetSWStats(pDevExt, pSWStatsArray, completeByteCount);
			}
		}
        break;

		case IGET_LED_STATISTICS:
		{
			LedStats *	pLstats; 

			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(LedStats), (PVOID *) &pLstats, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				if (completeByteCount >= sizeof(LedStats))
				{
					/* 1st bit 'on' of Status Register indicated DDR3 Calibration done*/
					pLstats->DdrCalib1 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.DDR3_SODIMMA;
					pLstats->DdrCalib2 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.DDR3_SODIMMB;
					pLstats->Phy0 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.XPHY0;
					pLstats->Phy1 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.XPHY1;
					pLstats->Phy2 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.XPHY2;
					pLstats->Phy3 = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_STATUS.BIT.XPHY3;
				}
			}
		}
        break;

		case ISET_PCI_LINKSPEED:
		{

		}

		case IGET_POWER_STATISTICS:
		{
			PowerMonitor *	pPowerStats; 

			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PowerMonitor ), (PVOID *) & pPowerStats  , &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				if (completeByteCount >= sizeof(PowerMonitor))
				{
					pPowerStats->VCCINT_PWR_CONS =	pDevExt->pUserSpace->POWER_MONITOR.VCCINT_PWR_CONS;
					pPowerStats->VCCAUX_PWR_CONS =	pDevExt->pUserSpace->POWER_MONITOR.VCCAUX_PWR_CONS ;
					pPowerStats->VCC3V3_PWR_CONS =	pDevExt->pUserSpace->POWER_MONITOR.VCC3V3_PWR_CONS;
					pPowerStats->VCC2V5_PWR_CONS =	pDevExt->pUserSpace->POWER_MONITOR.VCC2V5_PWR_CONS;
					pPowerStats->VCC1V5_PWR_CONS =	pDevExt->pUserSpace->POWER_MONITOR.VCC1V5_PWR_CONS;
					pPowerStats->MGT_AVCC_PWR_CONS = pDevExt->pUserSpace->POWER_MONITOR.MGT_AVCC_PWR_CONS;
					pPowerStats->MGT_AVTT_PWR_CONS = pDevExt->pUserSpace->POWER_MONITOR.MGT_AVTT_PWR_CONS;
				}
			}
		}
        break;

		case ISTART_TEST:
		case ISTOP_TEST:
		{
			TestCmd * pTC;
			if (InputBufferLength >= sizeof(TestCmd))
			{
				status = WdfRequestRetrieveInputBuffer(Request, sizeof(TestCmd),
														&pTC, NULL);
				if (status == STATUS_SUCCESS)
				{
					status = SetTestMode(pDevExt, pTC);
				}
			}
		}
		break;

		case IGET_TEST_STATE:
		{
			TestCmd * pTC;
			PDMA_ENGINE_EXTENSION	pDMAExt;

			status = WdfRequestRetrieveOutputBuffer(Request, sizeof(TestCmd),
													&pTC, &completeByteCount);
			if (status == STATUS_SUCCESS)
			{
				if (completeByteCount >= sizeof(TestCmd))
				{
					/* First, check if requested engine is valid */
					if (pTC->Engine < pDevExt->NumberDMAEngines)
					{
						pDMAExt = pDevExt->pDMAExt[pTC->Engine];
						// Return the test specifics
						pTC->MaxPktSize	= pDMAExt->MaxPktSize;
						pTC->MinPktSize	= pDMAExt->MinPktSize;
						pTC->TestMode	= pDMAExt->TestMode;
					}
				}
			}
		}
        break;

		case ISET_CANCEL_REQUESTS:
		{
		    INT *Engine;
			if (InputBufferLength >= sizeof(INT))
			{
			    status = WdfRequestRetrieveInputBuffer(Request, sizeof(INT),
														(PVOID *)&Engine, NULL);
				if (status == STATUS_SUCCESS)
				{
					status = XlxCancelRxTransfers(pDevExt, Engine);
				}
			}
		}
		break;

		default:
			DEBUGP(DEBUG_WARN, "--> XDMAEvtIoDevCtrl: Invalid command %d, Request %p", 
					IoControlCode, Request);
	        status = STATUS_INVALID_DEVICE_REQUEST;
		    break;
	}


    WdfRequestCompleteWithInformation(Request, status, completeByteCount);
	/*DEBUGP(DEBUG_TRACE,
                "<-- XDMAEvtIoDevCtrl: status 0x%x", status);*/
	return;
}


/*++

Routine Description:

    GetPCIStats - returns the PCI Configuration information

Arguments:

    pDevExt - XDMA Driver context
    pPCIState - Pointer to PCI Statistics structure
    Length - Size of the PCIState structure
 
Return Value:
	None
--*/
void
GetPCIStats(
	IN PDEVICE_EXTENSION	pDevExt,
	IN PCIState *			pPCIState,
	IN size_t				Length
	)
{
	// Make sure there is enough room in the structure
	if (Length >= sizeof(PCIState))
	{
		/* Indicates that link is up. */
		pPCIState->LinkState = LINK_UP;
		pPCIState->VendorId = pDevExt->PciCfg.VENDOR_ID;
		pPCIState->DeviceId = pDevExt->PciCfg.DEVICE_ID;

		if (pDevExt->PciCfg.INTERRUPT_PIN == 0)
		{
			pPCIState->IntMode = pDevExt->IntMode;
		}
		else if ((pDevExt->PciCfg.INTERRUPT_PIN > 0) &&
				(pDevExt->PciCfg.INTERRUPT_PIN < 5))
		{
			pPCIState->IntMode = INT_LEGACY;
		}
		else
		{
			pPCIState->IntMode = INT_NONE;
		}

		if(pDevExt->LinkSpeed == STD_GEN3_SPEED_REPRESENTATION)
			pPCIState->LinkSpeed = XILINX_GEN3_SPEED;
		else
			pPCIState->LinkSpeed	= pDevExt->LinkSpeed;

		pPCIState->LinkWidth	= pDevExt->LinkWidth;
		pPCIState->MPS			= 128 << pDevExt->MPS;
		pPCIState->MRRS			= 128 << pDevExt->MRRS;

		/* Read Initial Flow Control Credits information */
		pPCIState->InitFCCplD = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_COMPLETION_DATA_CREDITS.BIT.INIT_FC_CD;
		pPCIState->InitFCCplH = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_COMPLETION_HEADER_CREDITS.BIT.INIT_FC_CH;
		pPCIState->InitFCNPD  = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_NPD_CREDITS.BIT.INIT_FC_NPD;
		pPCIState->InitFCNPH  = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_NPH_CREDITS.BIT.INIT_FC_NPH;
		pPCIState->InitFCPD   = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_PD_CREDITS.BIT.INIT_FC_PD;
		pPCIState->InitFCPH   = pDevExt->pUserSpace->DESIGN_REGISTERS.INITIAL_PH_CREDITS.BIT.INIT_FC_PH;
		pPCIState->Version    = pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_VERSION.ULONG;
	}
}

/*++

Routine Description:

    GetEngineStats - returns the DMA Engine information

Arguments:

    pDevExt - XDMA Driver context
    pEngState - Pointer to Engine Statistics structure
    Length - Size of the Engine State structure
 
Return Value:
	None
--*/
void
GetEngineStats(
	IN PDEVICE_EXTENSION	pDevExt,
	IN EngState *			pEngState,
	IN size_t				Length
	)
{
	PDMA_ENGINE_EXTENSION	pDMAExt;

	// Make sure there is enough room to store the stats.
	if (Length >= sizeof(EngState))
	{
		/* First, check if requested engine is valid */
		if (pEngState->Engine < pDevExt->NumberDMAEngines)
		{
			pDMAExt = pDevExt->pDMAExt[pEngState->Engine];

			pEngState->BDs			= pDMAExt->TotalNumberDescriptors;
			pEngState->BDerrs		= pDMAExt->BDerrs;
			pEngState->BDSerrs		= 0;
			pEngState->DataMismatch = 0;
			if (pDevExt->TestConfig == TEST_CONFIG_INTERNAL_BUFFER)
			{
				pEngState->IntEnab		= 0;
			}
			else
			{
				pEngState->IntEnab		= 1;
			}
			pEngState->Buffers		= pDMAExt->AvailNumberDescriptors;
			pEngState->Engine		= pDMAExt->DMAEngineNumber;

			if (pDevExt->RawEthernet)
			{
				pEngState->MaxPktSize	= MAXRAWPKTSIZE;
			}
			else
			{
				pEngState->MaxPktSize	= MAXPKTSIZE;
			}
			pEngState->MinPktSize	= MINPKTSIZE;
			pEngState->TestMode		= pDMAExt->TestMode;
		}
	}
}

/*++

Routine Description:

    GetDMAStats - returns the DMA Engine information

Arguments:

    pDevExt - XDMA Driver context
    pStatsArray - Pointer to DMA Engine Statistics array structure
    Length - Size of the DMA Engine structure array
 
Return Value:
	None
--*/
void
GetDMAStats(
	IN PDEVICE_EXTENSION	pDevExt,
	IN EngStatsArray *		pStatsArray,
	IN size_t				Length
	)
{
	PDMA_ENGINE_EXTENSION	pDMAExt;
	DMAStatistics *			pDMAStats;
    int						j;
	int						element;
	BOOLEAN					noStats;

	element = 0;
	// Make sure there is enough room in relation to the count field.
	if (Length >= ((pStatsArray->Count * sizeof(DMAStatistics)) + sizeof(int)))
	{
		while (element < pStatsArray->Count)
		{
			noStats = TRUE;
			/* Must copy in a round-robin manner so that reporting is fair */
			for (j = 0; j < pDevExt->NumberDMAEngines; j++)
			{
				pDMAExt = pDevExt->pDMAExt[j];

				if (pDMAExt->dstatsNum) 
				{
					pDMAStats = &pStatsArray->eng[element++];
					if (element > pStatsArray->Count) 
						break;

					pDMAStats->Engine = j;
					pDMAStats->LAT = pDMAExt->DStats[pDMAExt->dstatsRead].LAT;
					pDMAStats->LBR = pDMAExt->DStats[pDMAExt->dstatsRead].LBR;
					pDMAStats->LWT = pDMAExt->DStats[pDMAExt->dstatsRead].LWT;
					pDMAStats->scaling_factor = pDMAExt->ScalingFactor;
					pDMAStats->IPS = pDMAExt->DStats[pDMAExt->dstatsRead].IPS;
					pDMAStats->DPS = pDMAExt->DStats[pDMAExt->dstatsRead].DPS;

					pDMAExt->DStats[pDMAExt->dstatsRead].LAT = 0;
					pDMAExt->DStats[pDMAExt->dstatsRead].LBR = 0;
					pDMAExt->DStats[pDMAExt->dstatsRead].LWT = 0;
					pDMAExt->DStats[pDMAExt->dstatsRead].IPS = 0;
					pDMAExt->DStats[pDMAExt->dstatsRead].DPS = 0;

					/*DEBUGP(DEBUG_VERBOSE, "#####LBR %lu, LAT %lu, LWT %lu####", 
						pDMAStats->LBR, pDMAStats->LAT, pDMAStats->LWT); */
					_InterlockedDecrement(&pDMAExt->dstatsNum);
					_InterlockedIncrement(&pDMAExt->dstatsRead);
					if (pDMAExt->dstatsRead == MAX_STATS)
						pDMAExt->dstatsRead = 0;
					noStats = FALSE;
				}
			}
			if (noStats)
			{
				// If no stats found in any of the DMA Engines, exit the while loop.
				break;
			}
		}
	}
	pStatsArray->Count = element;
}

/*++

Routine Description:

    GetTransStats - returns the PCI Transaction information

Arguments:

    pDevExt - XDMA Driver context
    pTransArray - Pointer to Transaction array statistics structure
    Length - Size of the Transaction array.
 
Return Value:
	None
--*/
void
GetTransStats(
	IN PDEVICE_EXTENSION	pDevExt,
	IN TRNStatsArray *		pTransArray,
	IN size_t				Length
	)
{
	TRNStatistics *			pTransStats;
	int						i = 0;
	BOOL					foundStats;

	// Make sure there is enough room in relation to the count field.
	if (Length >= ((pTransArray->Count * sizeof(TRNStatistics)) + sizeof(int)))
	{
		while (i < pTransArray->Count)
		{
			foundStats = FALSE;
			while (pDevExt->tstatsNum) 
			{
				foundStats = TRUE;
				pTransStats = &pTransArray->trn[i];
				pTransStats->LRX = 0;
				pTransStats->LTX = 0;
				pTransStats->LRX += pDevExt->TStats[pDevExt->tstatsRead].LRX;
				pTransStats->LTX += pDevExt->TStats[pDevExt->tstatsRead].LTX;
				pDevExt->TStats[pDevExt->tstatsRead].LRX = 0;
				pDevExt->TStats[pDevExt->tstatsRead].LTX = 0;
				pTransStats->scaling_factor = pDevExt->ScalingFactor;
				_InterlockedDecrement(&pDevExt->tstatsNum);
				_InterlockedIncrement(&pDevExt->tstatsRead);
				if (pDevExt->tstatsRead == MAX_STATS)
					pDevExt->tstatsRead = 0;
			    i++;
			}
			if (!foundStats) break;
		}
	}
    pTransArray->Count = i;
}

/*++

Routine Description:

    GetSWStats - returns the software statistics

Arguments:

    pDevExt - XDMA Driver context
    pSWArray - Pointer to Software statistics array structure
    Length - Size of the Software statistics array
 
Return Value:
	None
--*/
void
GetSWStats(
	IN PDEVICE_EXTENSION	pDevExt,
	IN SWStatsArray *		pSWArray,
	IN size_t				Length
	)
{
	PDMA_ENGINE_EXTENSION	pDMAExt;
	SWStatistics *			pSWStats;
	int						i = 0;
    int						j;
	BOOL					foundStats;

	// Make sure there is enough room in relation to the count field.
	if (Length >= ((pSWArray->Count * sizeof(SWStatistics)) + sizeof(int)))
	{
		while (i < pSWArray->Count)
		{
			foundStats = FALSE;
			/* Must copy in a round-robin manner so that reporting is fair */
			for (j = 0; j < pDevExt->NumberDMAEngines; j++)
			{
				pSWStats = &pSWArray->sw[i];
				pSWStats->LBR = 0;
				pDMAExt = pDevExt->pDMAExt[j];
				while (pDMAExt->sstatsNum) 
				{
					foundStats = TRUE;
					pSWStats->LBR += pDMAExt->SStats[pDMAExt->sstatsRead].LBR;
					pDMAExt->SStats[pDMAExt->sstatsRead].LBR = 0;
					pSWStats->Engine = j;
					_InterlockedDecrement(&pDMAExt->sstatsNum);
					if (_InterlockedIncrement(&pDMAExt->sstatsRead) == MAX_STATS)
						pDMAExt->sstatsRead = 0;
			        i++;
					if (i >= pSWArray->Count) break;
				}
			}
			if (!foundStats) break;
		}
	}
	pSWArray->Count = i;
}



/*++

Routine Description:

    Control the Packet Generator, Checker and Loopback functions.

Arguments:

    pDevExt - The adapter data space
            
    TC - Pointer to the TestCmd structure.

Return Value:
	NTSTATUS
--*/

NTSTATUS
SetTestMode(
	IN PDEVICE_EXTENSION	pDevExt,
	IN TestCmd * pTC
	)
{
	PDMA_ENGINE_EXTENSION	pDMAExt;
	unsigned int 			MaxPktSize;
	unsigned int 			MinPktSize;
	int						seqno;
	int						AppNo;
    NTSTATUS				status = STATUS_UNSUCCESSFUL;

	/* First, check if requested engine is valid */
	if (pTC->Engine < pDevExt->NumberDMAEngines)
	{
		MinPktSize = pTC->MinPktSize;
		MaxPktSize = pTC->MaxPktSize;

		AppNo = pTC->Engine;
		pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].WRITE_ENGINE];

		pDMAExt->TestMode = pTC->TestMode;

		/* Now write the registers */
		if (pTC->TestMode & TEST_START)
		{
			if (!(pTC->TestMode & (ENABLE_PKTCHK | ENABLE_PKTGEN | ENABLE_LOOPBACK)))
			{
				return STATUS_INVALID_PARAMETER;
			}
#if 0	
			/* Make sure Last DMA Transaction pointers donot contain stale data */
			pDMAExt->pXferParams->BytesTransferred = 0;
			pDMAExt->pXferParams = NULL;
#endif
			/* Initializing Packet Coalesce Count */
			        AppNo = pTC->Engine;
			        pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].WRITE_ENGINE];
					pDMAExt->IntrBDCount = 0;
			        pDMAExt->PktCoalesceCount = BD_COALESC_COUNT;

				    if (pDevExt->RawEthernet)
				    {
				        if(AppNo == 0)
					        AppNo = 1;
					    else
					        AppNo = 0;
				    }
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].READ_ENGINE];
#if 0
					XlxCancelTransfers((XDMA_HANDLE)pDMAExt);
#endif
					pDMAExt->IntrBDCount = 0;
			        pDMAExt->PktCoalesceCount = BD_COALESC_COUNT;

			/* Now ensure the sizes remain within bounds */
			if (pDevExt->RawEthernet)
			{
				if (pTC->MaxPktSize > MAXRAWPKTSIZE)
					MaxPktSize = MAXRAWPKTSIZE;
			}
			else
			{
				if (pTC->MaxPktSize > MAXPKTSIZE)
					MaxPktSize = MAXPKTSIZE;
			}
			if (pTC->MinPktSize < MINPKTSIZE)
				MinPktSize = MINPKTSIZE;
			if (pTC->MinPktSize > pTC->MaxPktSize)
				MinPktSize = MaxPktSize;

			// Before we do anything, make sure the DMA Engine is in a known state.
//			XlxResetDMAEngine(pDMAExt);

			if (pDevExt->RawEthernet)
			{
				pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_MODE_ADDRESS = RAW_DESIGN_MODE;
			}
			else
			{
				pDevExt->pUserSpace->DESIGN_REGISTERS.DESIGN_MODE_ADDRESS = PERF_DESIGN_MODE;
			}

			if ((pTC->Engine == CS2_DMA_ENGINE_OFFSET) || 
				(pTC->Engine == FIRST_S2C_DMA_ENGINE))
			{
				DEBUGP(DEBUG_TRACE,"Setting up App 0");

				AppNo = 0;
				pDevExt->pUserSpace->APP0_REGISTERS.PACKET_LENGTH.ULONG = MaxPktSize;
				DEBUGP(DEBUG_ALWAYS, "Packet Length set for App0 is %d",MaxPktSize);

				if (pDevExt->RawEthernet == 0)
				{
					seqno = TX_CONFIG_SEQNO;
					pDevExt->pUserSpace->APP0_REGISTERS.SEQNO_WRAP_REG = seqno;
				}
          
				/* Incase the last test was a loopback test, that bit may not be cleared. */
				pDevExt->pUserSpace->APP0_REGISTERS.LOOPBACK_CHECKER.BIT.LOOPBACK_ENABLE = 0;

				if (pTC->TestMode & ENABLE_PKTCHK)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Packet Checker for APP0");
					pDevExt->pUserSpace->APP0_REGISTERS.LOOPBACK_CHECKER.BIT.CHECKER_ENABLE = 1;
				}
				if (pTC->TestMode & ENABLE_LOOPBACK)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Loopback for APP0");
					pDevExt->pUserSpace->APP0_REGISTERS.LOOPBACK_CHECKER.BIT.LOOPBACK_ENABLE = 1;
				}
				if (pTC->TestMode & ENABLE_PKTGEN)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Packet Generator for APP0");
					pDevExt->pUserSpace->APP0_REGISTERS.TRAFFIC_GENERATOR.BIT.ENABLE = 1;
				}
#if 0 // Commented out (as in the Linux driver)
				pDevExt->pUserSpace->MEM_CTRL_CHNL_0.WRITE_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_0.READ_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_1.WRITE_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_1.READ_BURST_SIZE.BIT.SIZE = BURST_SIZE;
#endif // Commented out (as in the Linux driver)

				if (pDevExt->RawEthernet)
				{
#if 0
					if (pTC->TestMode & ENABLE_CRISCROSS)
#endif
					{
						DEBUGP(DEBUG_ALWAYS,"Setting up Crisscross with MAC2 RX");
						Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC2_OFFSET+XXGE_RCW1_OFFSET, RCV_ENABLE);
					}
#if 0
					else
					{
						DEBUGP(DEBUG_ALWAYS, "Setting up MAC1 RX");
						Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC1_OFFSET+XXGE_RCW1_OFFSET, RCV_ENABLE);
					}
#endif
					Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC1_OFFSET+XXGE_TC_OFFSET, TSMT_ENABLE);
				}
			}
			else
			{
				DEBUGP(DEBUG_TRACE,"Setting up App 1");

				AppNo = 1;
#if 0
				pDevExt->pUserSpace->APP1_REGISTERS.PACKET_LENGTH.ULONG = MaxPktSize;
				DEBUGP(DEBUG_ALWAYS, "Packet Length set for App1 is %d",MaxPktSize);

				if (pDevExt->RawEthernet == 0)
				{
					seqno = TX_CONFIG_SEQNO;
					pDevExt->pUserSpace->APP1_REGISTERS.SEQNO_WRAP_REG = seqno;
				}
          
				/* Incase the last test was a loopback test, that bit may not be cleared. */
				pDevExt->pUserSpace->APP1_REGISTERS.LOOPBACK_CHECKER.BIT.LOOPBACK_ENABLE = 0;
					
				if (pTC->TestMode & ENABLE_PKTCHK)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Packet Checker for APP1");
					pDevExt->pUserSpace->APP1_REGISTERS.LOOPBACK_CHECKER.BIT.CHECKER_ENABLE = 1;
				}
				if (pTC->TestMode & ENABLE_LOOPBACK)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Loopback for APP1");
					pDevExt->pUserSpace->APP1_REGISTERS.LOOPBACK_CHECKER.BIT.LOOPBACK_ENABLE = 1;
				}
				if (pTC->TestMode & ENABLE_PKTGEN)
				{
					DEBUGP(DEBUG_ALWAYS, "Enabling Packet Generator for APP1");
					pDevExt->pUserSpace->APP1_REGISTERS.TRAFFIC_GENERATOR.BIT.ENABLE = 1;
				}
#if 0 // Commented out (as in the Linux driver)
				pDevExt->pUserSpace->MEM_CTRL_CHNL_2.WRITE_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_2.READ_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_3.WRITE_BURST_SIZE.BIT.SIZE = BURST_SIZE;
				pDevExt->pUserSpace->MEM_CTRL_CHNL_3.READ_BURST_SIZE.BIT.SIZE = BURST_SIZE;
#endif // Commented out (as in the Linux driver)
				if (pDevExt->RawEthernet)
				{
#if 0
					if (pTC->TestMode & ENABLE_CRISCROSS)
#endif
					{

						DEBUGP(DEBUG_ALWAYS, "Setting up Crisscross with MAC1 RX");
						Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC1_OFFSET+XXGE_RCW1_OFFSET, RCV_ENABLE);
					}
#if 0
					else
					{
						DEBUGP(DEBUG_ALWAYS, "Setting up MAC2 RX");
						Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC2_OFFSET+XXGE_RCW1_OFFSET, RCV_ENABLE);
					}
#endif
					Xil_Out32((PUCHAR)(pDevExt->pUserSpace)+MAC2_OFFSET+XXGE_TC_OFFSET, TSMT_ENABLE);
				}
#endif
			}
			if (pDevExt->TestConfig == TEST_CONFIG_INTERNAL_BUFFER)
			{
				if (pTC->TestMode & (ENABLE_PKTCHK | ENABLE_LOOPBACK))
				{
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].WRITE_ENGINE];
					if (pDevExt->RawEthernet)
					{
						DEBUGP(DEBUG_ALWAYS, "SetTestMode: Setting up RAW Ethernet buffer for DMA Engine %d.", 
							pDMAExt->DMAEngineNumber);
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 0) =  0xFFFF;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 2) =  0xFFFF;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 4) =  0xFFFF;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 6) =  0xAABB;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 8) =  0xCCDD;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 10) = 0xEEFF;
						*(unsigned short *)(pDMAExt->pInternalDMABufferBase + 12) = 0x8870;
					}
					DEBUGP(DEBUG_ALWAYS, "SetTestMode: Setting up Internal Transfers for S2C DMA Engine %d.", 
							pDMAExt->DMAEngineNumber);
					if (pDMAExt->InternalDMABufferSize < MaxPktSize)
					{
						MaxPktSize = (unsigned int)pDMAExt->InternalDMABufferSize;
					}
					pDMAExt->MinPktSize = MinPktSize;
					pDMAExt->MaxPktSize = MaxPktSize;
					// Setup the descriptors to use an internal kernel allocated buffer.
					pDMAExt->bInternalTestMode = TRUE;
					pDMAExt->AvailNumberDescriptors = pDMAExt->TotalNumberDescriptors;
					status = SetupInternalDMAXfers(pDMAExt, MaxPktSize);
				}
				if (pTC->TestMode & (ENABLE_PKTGEN | ENABLE_LOOPBACK))
				{
					if (pDevExt->RawEthernet)
					{
						if(AppNo == 0)
							AppNo = 1;
						else
							AppNo = 0;
					}
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].READ_ENGINE];
					DEBUGP(DEBUG_ALWAYS, "SetTestMode: Setting up Internal Transfers for C2S DMA Engine %d.", 
							pDMAExt->DMAEngineNumber);
					if (pDMAExt->InternalDMABufferSize < MaxPktSize)
					{
						MaxPktSize = (unsigned int)pDMAExt->InternalDMABufferSize;
					}
					pDMAExt->MinPktSize = MinPktSize;
					pDMAExt->MaxPktSize = MaxPktSize;
					// Setup the descriptors to use an internal kernel allocated buffer.
					pDMAExt->bInternalTestMode = TRUE;
					pDMAExt->AvailNumberDescriptors = pDMAExt->TotalNumberDescriptors;
					status = SetupInternalDMAXfers(pDMAExt, MaxPktSize);
				}
			}
			else
			{
				// Set packet sizes.
				pDMAExt->MinPktSize = MinPktSize;
				pDMAExt->MaxPktSize = MaxPktSize;
				status = STATUS_SUCCESS;
			}
		}
		else
		{
			/* Else, stop the test. Do not remove any loopback here because
			*	the DMA queues and hardware FIFOs must drain first.
			*/
			if ((pTC->Engine == CS2_DMA_ENGINE_OFFSET) || 
				(pTC->Engine == FIRST_S2C_DMA_ENGINE))
			{
				AppNo = 0;
				pDevExt->pUserSpace->APP0_REGISTERS.LOOPBACK_CHECKER.BIT.CHECKER_ENABLE = 0;
				pDevExt->pUserSpace->APP0_REGISTERS.TRAFFIC_GENERATOR.BIT.ENABLE = 0;
			//	pDevExt->pUserSpace->APP0_REGISTERS.PACKET_LENGTH.ULONG = 0;
			}
			else
			{
				AppNo = 1;
#if 0
				pDevExt->pUserSpace->APP1_REGISTERS.LOOPBACK_CHECKER.BIT.CHECKER_ENABLE = 0;
				pDevExt->pUserSpace->APP1_REGISTERS.TRAFFIC_GENERATOR.BIT.ENABLE = 0;
			//	pDevExt->pUserSpace->APP1_REGISTERS.PACKET_LENGTH.ULONG = 0;
#endif
			}
			// If we were doing internal buffer transfer stop the DMA engine by setting the Stop DMA Pointer.
			if (pDevExt->TestConfig == TEST_CONFIG_INTERNAL_BUFFER)
			{
				if (pTC->TestMode & (ENABLE_PKTCHK | ENABLE_LOOPBACK))
				{
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].WRITE_ENGINE];
					DEBUGP(DEBUG_ALWAYS, "SetTestMode: Shutting down Internal Transfers for S2C DMA Engine %d.", 
							pDMAExt->DMAEngineNumber);
					pDMAExt->MinPktSize = MinPktSize;
					pDMAExt->MaxPktSize = MaxPktSize;
		            pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pDMAExt->pTailDMADescriptor->S2C.NEXT_DESC_PHYS_PTR;
					pDMAExt->AvailNumberDescriptors = pDMAExt->TotalNumberDescriptors;
					pDMAExt->bInternalTestMode = FALSE;
				}
				if (pTC->TestMode & (ENABLE_PKTGEN | ENABLE_LOOPBACK))
				{
					if (pDevExt->RawEthernet)
					{
						if(AppNo == 0)
							AppNo = 1;
						else
							AppNo = 0;
					}
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].READ_ENGINE];
					DEBUGP(DEBUG_ALWAYS, "SetTestMode: Shutting down Internal Transfers for C2S DMA Engine %d.", 
							pDMAExt->DMAEngineNumber);
					pDMAExt->MinPktSize = MinPktSize;
					pDMAExt->MaxPktSize = MaxPktSize;
		            pDMAExt->pDMAEngRegs->SOFTWARE_DESCRIPTOR_PHYS_PTR = pDMAExt->pTailDMADescriptor->S2C.NEXT_DESC_PHYS_PTR;
					pDMAExt->AvailNumberDescriptors = pDMAExt->TotalNumberDescriptors;
					pDMAExt->bInternalTestMode = FALSE;
				}
			}
			else
			{
				// Set packet sizes.
				pDMAExt->MinPktSize = MinPktSize;
				pDMAExt->MaxPktSize = MaxPktSize;
				/* Marking Packet Coalesce Count  as 1 to drain FIFOs in the Rx side. */

				    if ((pTC->Engine == CS2_DMA_ENGINE_OFFSET) || 
				                (pTC->Engine == FIRST_S2C_DMA_ENGINE))
			        {
				        AppNo = 0;
				    }
				    else
				    {
				        AppNo = 1;
				    }
				    if (pDevExt->RawEthernet)
				    {
				        if(AppNo == 0)
					        AppNo = 1;
					    else
					        AppNo = 0;
				    }
					pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].READ_ENGINE];
				    pDMAExt->IntrBDCount = 0;
			        pDMAExt->PktCoalesceCount = 1;

            }
			status = STATUS_SUCCESS;
		}          
		return status;
	}
	return STATUS_INVALID_PARAMETER;
}

NTSTATUS 
XlxCancelRxTransfers(
	IN PDEVICE_EXTENSION pDevExt,
	IN INT* Engine
	)
{
    PDMA_ENGINE_EXTENSION	pDMAExt;
	INT AppNo = 0;

	if (pDevExt->RawEthernet)
	{
	    if(*Engine == 0)
			AppNo = 1;
		else if(*Engine == 1)
			AppNo = 0;
	}
	else
	{
	    AppNo = *Engine;
	}
	
	pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].READ_ENGINE];
    
	DEBUGP(DEBUG_ALWAYS,"XlxCancelRxTransfers: Resetting C2S DMA internal Engine %d and engine number %d", 
							pDMAExt->InternalDMAEngineNumber,pDMAExt->DMAEngineNumber);
	XlxCancelTransfers((XDMA_HANDLE)pDMAExt);


	AppNo = *Engine;
	pDMAExt = pDevExt->pDMAExt[APP_TABLE[AppNo].WRITE_ENGINE];

	DEBUGP(DEBUG_ALWAYS,"XlxCancelRxTransfers: Resetting S2C DMA internal Engine %d and engine number %d", 
							pDMAExt->InternalDMAEngineNumber,pDMAExt->DMAEngineNumber);

	XlxCancelTransfers((XDMA_HANDLE)pDMAExt);

    return STATUS_SUCCESS;
}