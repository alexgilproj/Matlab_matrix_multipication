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

    XDMA_Timer.c

Abstract:
 
    Contains the all the timer functions for the driver. 

Environment:

    Kernel mode

--*/

#include "precomp.h"

#if TRACE_ENABLED
#include "XDMA_Timer.tmh"
#endif // TRACE_ENABLED

// One second interval polling timer for collecting performance statistics
//
VOID
XDMAStatsTimer(
   IN WDFTIMER	Timer
   )
{
	PDEVICE_EXTENSION	pDevExt = XlxGetDeviceContext(WdfTimerGetParentObject(Timer));
    PDMA_ENGINE_EXTENSION   pDMAExt = NULL;
    int i;

    /* First, get DMA payload statistics */
	for(i = 0; i < pDevExt->NumberDMAEngines; i++) 
    {
		pDMAExt = pDevExt->pDMAExt[i];
		if (pDMAExt != NULL)
		{
			if ((pDMAExt->EngineState > AVAILABLE) || (pDMAExt->bInternalTestMode))
			{
				/* 
				* We want to store the latest set of statistics. If an app is not
				* fetching the data, the statistics will build up. In this case the read 
				* pointer is moved forward along with the write pointer. In this case 
				* the oldest data is discarded.
				*/
				pDMAExt->DStats[pDMAExt->dstatsWrite].LAT = pDMAExt->pDMAEngRegs->ENGINE_ACTIVE_TIME & ENGINE_STATS_MASK;
				pDMAExt->DStats[pDMAExt->dstatsWrite].LWT = pDMAExt->pDMAEngRegs->ENGINE_WAIT_TIME & ENGINE_STATS_MASK;
				pDMAExt->DStats[pDMAExt->dstatsWrite].LBR = pDMAExt->pDMAEngRegs->COMPLETED_BYTE_COUNT & ENGINE_STATS_MASK;
#if 0
				DEBUGP(DEBUG_VERBOSE, "#####LBR %lu, LAT %lu, LWR %lu####", 
					pDMAExt->DStats[pDMAExt->dstatsWrite].LBR,
					pDMAExt->DStats[pDMAExt->dstatsWrite].LAT,
					pDMAExt->DStats[pDMAExt->dstatsWrite].LWT);
#endif
				pDMAExt->DStats[pDMAExt->dstatsWrite].IPS = _InterlockedExchange(&pDMAExt->InterruptsPerSecond, 0);
				pDMAExt->DStats[pDMAExt->dstatsWrite].DPS = _InterlockedExchange(&pDMAExt->DPCsPerSecond, 0);

				if (_InterlockedIncrement(&pDMAExt->dstatsWrite) >= MAX_STATS) 
					pDMAExt->dstatsWrite = 0;

				if (pDMAExt->dstatsNum < MAX_STATS) 
				{
			        _InterlockedIncrement(&pDMAExt->dstatsNum);
				}
				else
				{
					/* else move the read pointer forward, drop older data */
					if (_InterlockedIncrement(&pDMAExt->dstatsRead) >= MAX_STATS) 
						pDMAExt->dstatsRead = 0;
				}

				/* Next, read the SW statistics counters */
				
				pDMAExt->SStats[pDMAExt->sstatsWrite].LBR = _InterlockedExchange(&pDMAExt->SWrate, 0);
				if (_InterlockedIncrement(&pDMAExt->sstatsWrite) >= MAX_STATS) 
					pDMAExt->sstatsWrite = 0;

				if (pDMAExt->sstatsNum < MAX_STATS)
				{
			        _InterlockedIncrement(&pDMAExt->sstatsNum);
				}
			    else
				{
			        /* else move the read pointer forward, drop older data */
				    if (_InterlockedIncrement(&pDMAExt->sstatsRead) >= MAX_STATS) 
						pDMAExt->sstatsRead = 0;
				}
			}
		}
	}

    /* Now, get the TRN statistics */
    /* Registers to be read for TRN stats */
    /* This counts all TLPs including header */
	pDevExt->TStats[pDevExt->tstatsWrite].LTX = 
		pDevExt->pDMARegisters->USER_SPACE.DESIGN_REGISTERS.TRANSIMT_UTILIZATION.ULONG & ENGINE_STATS_MASK;
	pDevExt->TStats[pDevExt->tstatsWrite].LRX = 
		pDevExt->pDMARegisters->USER_SPACE.DESIGN_REGISTERS.RECEIVE_UTILIZATION.ULONG & ENGINE_STATS_MASK;

	DEBUGP(DEBUG_VERBOSE, "LTX %lu, LRX %lu ",
		pDevExt->TStats[pDevExt->tstatsWrite].LTX ,
		pDevExt->TStats[pDevExt->tstatsWrite].LRX);

    if (_InterlockedIncrement(&pDevExt->tstatsWrite) >= MAX_STATS) 
		pDevExt->tstatsWrite = 0;

    if (pDevExt->tstatsNum < MAX_STATS)
        _InterlockedIncrement(&pDevExt->tstatsNum);
    else
    {
	    /* else move the read pointer forward and drop older data */
        if (_InterlockedIncrement(&pDevExt->tstatsRead) >= MAX_STATS) 
			pDevExt->tstatsRead = 0;
    }
}
