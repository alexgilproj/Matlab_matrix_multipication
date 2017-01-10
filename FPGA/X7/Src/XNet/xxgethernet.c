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
* @file xxgethernet.c
*
* The APIs in this file takes care of the primary functionalities of the driver.
* The APIs in this driver take care of the following:
*	- Starting or stopping the XGEMAC Ethernet device
*	- Initializing and resetting the XGEMAC Ethernet device
*	- Setting MAC address  
*	- Provide means for controlling the PHY and communicating with it.
*	- Turn on/off various features/options provided by the XGEMAC Ethernet
*	  device.
* See xxgethernet.h for a detailed description of the driver.
* 
* MODIFICATION HISTORY:
*
* Ver  Date     Changes
* ---- --------- -------------------------------------------------------
* 
*
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "Precomp.h"


/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

static void InitHw(PMP_ADAPTER Adapter);	/* HW reset */

/************************** Variable Definitions *****************************/


/*****************************************************************************/
/**
* XXgEthernet_Start starts the XGEMAC Ethernet device as follows:
*	- Enable transmitter if XXGE_TRANSMIT_ENABLE_OPTION is set
*	- Enable receiver if XXGE_RECEIVER_ENABLE_OPTION is set
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @return	None.
*
* @note		None.
*
*
******************************************************************************/
void 
XXgEthernet_Start(
	PMP_ADAPTER Adapter
	)
{
	ULONG Reg;

	DEBUGP(DEBUG_TRACE, "XXgEthernet_Start");

	/* Assert bad arguments and conditions */
    if (Adapter == NULL)
		return;
    if (!MP_IS_READY(Adapter))
		return;

	/* If already started, then there is nothing to do */
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_STARTED)) 
	{
		return;
	}
 
//    Reg_tmp = XXgEthernet_ReadReg(InstancePtr->Config.BaseAddress, XXGE_VER_OFFSET);
//    DEBUGP(DEBUG_INFO, "**Version Register = %x \n", Reg_tmp);
//    Reg_tmp = XXgEthernet_ReadReg(InstancePtr->Config.BaseAddress, 	XXGE_CAP_OFFSET);
//    DEBUGP(DEBUG_INFO, "the Value of Capability Register = %x \n", Reg_tmp);

	/* Enable transmitter if not already enabled */
	if (Adapter->Options & XXGE_TRANSMITTER_ENABLE_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "enabling transmitter");
		Reg = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);
		if (!(Reg & XXGE_TC_TX_MASK)) 
		{
			DEBUGP(DEBUG_WARN,	"transmitter not enabled, enabling now");
			Xil_Out32(Adapter->MACRegisters + XXGE_TC_OFFSET, Reg | XXGE_TC_TX_MASK);
		}
		DEBUGP(DEBUG_INFO, "transmitter enabled");
	}

	/* Enable receiver */
	if (Adapter->Options & XXGE_RECEIVER_ENABLE_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "enabling receiver");
		Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
		if (!(Reg & XXGE_RCW1_RX_MASK)) 
		{
			DEBUGP(DEBUG_WARN, "receiver not enabled, enabling now");
			Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET,	Reg | XXGE_RCW1_RX_MASK);
		}
		DEBUGP(DEBUG_INFO, "receiver enabled");
	}

	/* Mark as started */
    MP_SET_FLAG(Adapter, fMP_ADAPTER_STARTED);
	DEBUGP(DEBUG_TRACE, "XXgEthernet_Start: done");
}

/*****************************************************************************/
/**
* XXgEthernet_Stop gracefully stops the XGEMAC Ethernet device as follows:
*	- Disable all interrupts from this device
*	- Disable the receiver
*
* XXgEthernet_Stop does not modify any of the current device options.
*
* Since the transmitter is not disabled, frames currently in internal buffers
* or in process by a DMA engine are allowed to be transmitted.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @return	None
*
* @note		None.
*
*
******************************************************************************/
void 
XXgEthernet_Stop(
	PMP_ADAPTER Adapter
	)
{
	ULONG Reg;

	DEBUGP(DEBUG_TRACE, "XXgEthernet_Stop");

	if (Adapter == NULL)
		return;
    if (!MP_IS_READY(Adapter))
		return;
	/* If already stopped, then there is nothing to do */
    if (!MP_TEST_FLAG(Adapter, fMP_ADAPTER_STARTED)) 
	{
		return;
	}

	DEBUGP(DEBUG_INFO, "XXgEthernet_Stop: disabling interrupts");

    /* Disable the receiver */
    Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
    Reg &= ~XXGE_RCW1_RX_MASK;
    Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, Reg);

    /*
     * Stopping the receiver in mid-packet causes a dropped packet
     * indication from HW. Clear it.
     */

	/* Mark as stopped */
    MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_STARTED);
	DEBUGP(DEBUG_TRACE, "XXgEthernet_Stop: done");
}


/*****************************************************************************/
/**
* XXgEthernet_Reset performs a reset of the XGEMAC Ethernet device, specified by
* <i>Adapter</i>.
*
* XXgEthernet_Reset also resets the XGEMAC Ethernet's options to their
* default values.
*
* The calling software is responsible for re-configuring the XGEMAC Ethernet
* (if necessary) and restarting the MAC after the reset.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
*
* @note		None.
*
*
******************************************************************************/
void 
XXgEthernet_Reset(
	PMP_ADAPTER Adapter
	)
{
	ULONG Reg;
	ULONG TimeoutLoops;

	DEBUGP(DEBUG_TRACE, "XXgEthernet_Reset");

	if (Adapter == NULL)
		return;
    if (!MP_IS_READY(Adapter))
		return;

	/* Stop the device and reset HW */
	XXgEthernet_Stop(Adapter);
	Adapter->Options = XXGE_DEFAULT_OPTIONS;

	/* Reset the receiver */
	DEBUGP(DEBUG_INFO, "resetting the receiver");
	Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	Reg |= XXGE_RCW1_RST_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, Reg);

	/* Reset the transmitter */
	DEBUGP(DEBUG_INFO, "resetting the transmitter");
	Reg = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);
	Reg |= XXGE_TC_RST_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_TC_OFFSET, Reg);

	DEBUGP(DEBUG_INFO, "waiting until reset is done");

	TimeoutLoops  = XXGE_RST_DELAY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && (Reg & (XXGE_RCW1_RST_MASK | XXGE_TC_RST_MASK))) 
	{
		Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
		Reg |= Xil_In32(Adapter->MACRegisters +	XXGE_TC_OFFSET);
		TimeoutLoops --;
	}
	if (TimeoutLoops == 0) 
	{
		DEBUGP(DEBUG_ERROR, "Timed out waiting for reset");
		return;
	}

	/* Setup HW */
	InitHw(Adapter);
}


/******************************************************************************
*
* InitHw (internal use only) performs a one-time setup of a XGEMAC Ethernet device.
* The setup performed here only need to occur once after any reset.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @note		None.
*
*
******************************************************************************/
void 
InitHw(
	PMP_ADAPTER Adapter
	)
{
	ULONG Reg;

	DEBUGP(DEBUG_TRACE, "XXgEthernet InitHw");

	/* Disable the receiver */
	Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	Reg &= ~XXGE_RCW1_RX_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, Reg);

    /*
     * Sync default options with HW but leave receiver and transmitter
     * disabled. They get enabled with XXgEthernet_Start() if
     * XXGE_TRANSMITTER_ENABLE_OPTION and XXGE_RECEIVER_ENABLE_OPTION
     * are set
     */
    XXgEthernet_SetOptions(Adapter, Adapter->Options & ~(XXGE_TRANSMITTER_ENABLE_OPTION |
		XXGE_RECEIVER_ENABLE_OPTION));

	XXgEthernet_ClearOptions(Adapter, ~Adapter->Options);

	Reg = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	DEBUGP(DEBUG_INFO, "XXgEthernet InitHw: RCW1 = 0x%x", Reg);
	Reg = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);
	DEBUGP(DEBUG_INFO, "XXgEthernet InitHw: TC = 0x%x", Reg);

	/* Set default MDIO divisor */
	XXgEthernet_PhySetMdioDivisor(Adapter, XXGE_MDIO_DIV_DFT);

	DEBUGP(DEBUG_TRACE, "XXgEthernet InitHw: done");
}


/*****************************************************************************/
/**
 * XXgEthernet_SetMacAddress sets the MAC address for the XGEMAC Ethernet device,
 * specified by <i>Adapter</i> to the MAC address specified by
 * <i>AddressPtr</i>.
 * The XGEMAC Ethernet device must be stopped before calling this function.
 *
 * @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
 *		worked on.
 * @param	AddressPtr is a reference to the 6-byte MAC address to set.
 *
 * @return
 *		- XST_SUCCESS on successful completion.
 *		- XST_DEVICE_IS_STARTED if the XGEMAC Ethernet device has not
 *		  stopped,
 *
 * @note
 * This routine also supports the extended/new VLAN and multicast mode. The
 * XXGE_RAF_NEWFNCENBL_MASK bit dictates which offset will be configured.
 *
 ******************************************************************************/
NTSTATUS
XXgEthernet_SetMacAddress(
	PMP_ADAPTER Adapter, 
	void *AddressPtr
	)
{
	ULONG MacAddr;
	UCHAR *Aptr = (UCHAR *) AddressPtr;

	if (Adapter == NULL)
		return STATUS_UNSUCCESSFUL;
    if (!MP_IS_READY(Adapter))
		return STATUS_UNSUCCESSFUL;
	if (AddressPtr == NULL)
		return STATUS_UNSUCCESSFUL;

	/* Be sure device has been stopped */
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_STARTED)) 
	{
		DEBUGP(DEBUG_ERROR, "XXgEthernet_SetMacAddress Adapter Started!\n");
		return STATUS_UNSUCCESSFUL;
	}

	// REFERENCE
    // MAC address: 00:0a:35:01:fa:3a
    // XIo_Out32( (dmaData->barInfo[0].baseVAddr) + 0x9404, 0x01350a00 );
    // XIo_Out32( (dmaData->barInfo[0].baseVAddr) + 0x9408, 0x00003afa );

    MacAddr = Aptr[0];
    MacAddr |= Aptr[1] << 8;
    MacAddr |= Aptr[2] << 16;
    MacAddr |= Aptr[3] << 24;
    Xil_Out32(Adapter->XGERegisters + XXGE_MACL_OFFSET, MacAddr);

	MacAddr  = 0;
    MacAddr |= Aptr[4];
    MacAddr |= Aptr[5] << 8;
    Xil_Out32(Adapter->XGERegisters + XXGE_MACU_OFFSET, MacAddr);

	DEBUGP(DEBUG_INFO, "0x9400 is : 0x%x", 
		Xil_In32(Adapter->pUserSpace + 0x1400));
    DEBUGP(DEBUG_INFO, "0x9404 is : 0x%x",
		Xil_In32(Adapter->pUserSpace + 0x1404));
    DEBUGP(DEBUG_INFO, "0x9408 is : 0x%x",
		Xil_In32(Adapter->pUserSpace + 0x1408));
    DEBUGP(DEBUG_INFO, "0x940C is : 0x%x",
		Xil_In32(Adapter->pUserSpace + 0x140C));
    DEBUGP(DEBUG_INFO, "0x9410 is : 0x%x",
		Xil_In32(Adapter->pUserSpace + 0x1410));
    DEBUGP(DEBUG_INFO, "0x9414 is : 0x%x",
		Xil_In32(Adapter->pUserSpace + 0x1414));
       
    return STATUS_SUCCESS;
}


/*****************************************************************************/
/**
 * XXgEthernet_GetMacAddress gets the MAC address for the XGEMAC Ethernet,
 * specified by <i>Adapter</i> into the memory buffer specified by
 * <i>AddressPtr</i>.
 *
 * @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
 *		worked on.
 * @param	AddressPtr references the memory buffer to store the retrieved
 *		MAC address. This memory buffer must be at least 6 bytes in
 *		length.
 *
 * @return	None.
 *
 * @note
 *
 * This routine also supports the extended/new VLAN and multicast mode. The
 * XXGE_RAF_NEWFNCENBL_MASK bit dictates which offset will be configured.
 *
 ******************************************************************************/
void 
XXgEthernet_GetMacAddress(
	PMP_ADAPTER Adapter, 
	void *AddressPtr
	)
{
	ULONG MacAddr;
	UCHAR *Aptr = (UCHAR *) AddressPtr;

	if (Adapter == NULL)
		return;
	if (AddressPtr == NULL)
		return;
    if (!MP_IS_READY(Adapter))
		return;

    /* Read MAC bits [31:0] in UAW0 */
	MacAddr = Xil_In32(Adapter->XGERegisters + XXGE_MACL_OFFSET);
    Aptr[0] = (UCHAR) MacAddr;
    Aptr[1] = (UCHAR) (MacAddr >> 8);
    Aptr[2] = (UCHAR) (MacAddr >> 16);
    Aptr[3] = (UCHAR) (MacAddr >> 24);

    /* Read MAC bits [47:32] in UAW1 */
	MacAddr = Xil_In32(Adapter->XGERegisters + XXGE_MACU_OFFSET);
    Aptr[4] = (UCHAR) MacAddr;
    Aptr[5] = (UCHAR) (MacAddr >> 8);

	DEBUGP(DEBUG_INFO, "MAC Address: 0x%x %x %x %x %x %x", 
		Aptr[0], Aptr[1], Aptr[2], Aptr[3], Aptr[4], Aptr[5]);
}



/*****************************************************************************/
/**
 * XXgEthernet_UpdateDepOption check and update dependent options for
 * new/extended features. This is a helper function that is meant to be called
 * by XXgEthernet_SetOptions() and XXgEthernet_ClearOptions().
 *
 * @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
 *		worked on.
 *
 * @return	Dependent options that are required to set/clear per
 *		hardware requirement.
 *
 * @note
 *
 * This helper function collects the dependent OPTION(s) per hardware design.
 * When conflicts arises, extended features have precedence over legacy ones.
 * Two operations to be considered,
 * 1. Adding extended options. If XATE_VLAN_OPTION is enabled and enable one of
 *	extended VLAN options, XATE_VLAN_OPTION should be off and configure to
 *	hardware.
 *	However, XGEMAC-ethernet instance Options variable still holds
 *	XATE_VLAN_OPTION so when all of the extended feature are removed,
 *	XATE_VLAN_OPTION can be effective and configured to hardware.
 * 2. Removing extended options. Remove extended option can not just remove
 *	the selected extended option and dependent options. All extended
 *	options need to be verified and remained when one or more extended
 *	options are enabled.
 *
 * Dependent options are :
 *	- XXGE_VLAN_OPTION,
 *	- XXGE_JUMBO_OPTION
 *	- XXGE_FCS_INSERT_OPTION,
 *	- XXGE_FCS_STRIP_OPTION
 *	- XXGE_PROMISC_OPTION.
 *
 ******************************************************************************/
static ULONG 
XXgEthernet_UpdateDepOptions(
	PMP_ADAPTER Adapter
	)
{
	/*
	 * This is a helper function for XXgEthernet_[Set|Clear]Options()
	 */
	ULONG DepOptions = Adapter->Options;

	/*
	 * The extended/new features require some OPTIONS to be on/off per
	 * hardware design. We determine these extended/new functions here
	 * first and also on/off other OPTIONS later. So that dependent
	 * OPTIONS are in sync and _[Set|Clear]Options() can be performed
	 * seamlessly.
	 */

    /*
     * enable Promiscuous option
     * XXGE_PROMISC_OPTION is required to be enabled.
     */
	DepOptions |= XXGE_PROMISC_OPTION;
	DEBUGP(DEBUG_VERBOSE, "CheckDepOptions: enabling ext promiscous\n");

    return(DepOptions);
}


/*****************************************************************************/
/**
* XXgEthernet_SetOptions enables the options, <i>Options</i> for the
* XGEMAC Ethernet, specified by <i>Adapter</i>. XGEMAC Ethernet should be
* stopped with XXgEthernet_Stop() before changing options.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @param	Options is a bitmask of OR'd XXGE_*_OPTION values for options to
*		set. Options not specified are not affected.
*
* @return
*		- XST_SUCCESS on successful completion.
*		- XST_DEVICE_IS_STARTED if the device has not been stopped.
*
*
* @note
* See xxgethernet.h for a description of the available options.
*
*
******************************************************************************/
NTSTATUS 
XXgEthernet_SetOptions(
	PMP_ADAPTER Adapter, 
	ULONG Options
	)
{
	ULONG Reg;	/* Generic register contents */
	ULONG RegRcw1;	/* Reflects original contents of RCW1 */
	ULONG RegTc;	/* Reflects original contents of TC  */
	ULONG RegNewRcw1;	/* Reflects new contents of RCW1 */
	ULONG RegNewTc;	/* Reflects new contents of TC  */
	ULONG DepOptions;	/* Required dependent options for new features */

    ULONG TempRegRcw1;    /* Reflects original contents of RCW1 */
    ULONG TempRegTc;      /* Reflects original contents of TC  */

	DEBUGP(DEBUG_TRACE, "XXgEthernet_SetOptions\n");

	if (Adapter == NULL)
		return STATUS_UNSUCCESSFUL;
    if (!MP_IS_READY(Adapter))
		return STATUS_UNSUCCESSFUL;
	/* Be sure device has been stopped */
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_STARTED)) 
	{
		DEBUGP(DEBUG_WARN, "XXgEthernet_SetOptions Adapter Started!\n");
		return STATUS_UNSUCCESSFUL;
	}


	/*
	 * Set options word to its new value.
	 * The step is required before calling _UpdateDepOptions() since
	 * we are operating on updated options.
	 */
	Adapter->Options |= Options;

	/*
	 * There are options required to be on/off per hardware requirement.
	 * Invoke _UpdateDepOptions to check hardware availability and update
	 * options accordingly.
	 */
	DepOptions = XXgEthernet_UpdateDepOptions(Adapter);

    /*
     * New/extended function bit should be on if any new/extended features
     * are on and hardware is built with them.
     */

    /*
     * Many of these options will change the RCW1 or TC registers.
     * To reduce the amount of IO to the device, group these options here
     * and change them all at once.
     */
    /* Get current register contents */
    RegRcw1 = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
    RegTc = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);
    RegNewRcw1 = RegRcw1;
    RegNewTc = RegTc;

	DEBUGP(DEBUG_INFO, "current control regs: RCW1: 0x%0x; TC: 0x%0x\n", RegRcw1, RegTc);
	DEBUGP(DEBUG_INFO, "Options: 0x%0x; default options: 0x%0x\n", Options, XXGE_DEFAULT_OPTIONS);

	/* Turn on jumbo packet support for both Rx and Tx */
	if (DepOptions & XXGE_JUMBO_OPTION) 
	{
		RegNewTc |= XXGE_TC_JUM_MASK;
		RegNewRcw1 |= XXGE_RCW1_JUM_MASK;
	}
        
    /* Turn on FCS Stripping support */
    if (DepOptions & XXGE_FCS_STRIP_OPTION) 
	{
		RegNewTc &= ~XXGE_TC_FCS_MASK;
        RegNewRcw1 &= ~XXGE_RCW1_FCS_MASK;
    }    

    /* Turn on length/type field checking on receive packets */
    if (DepOptions & XXGE_LENTYPE_ERR_OPTION) 
	{
		RegNewRcw1 &= ~XXGE_RCW1_LT_DIS_MASK;
    }

	/* Enable transmitter */
	if (DepOptions & XXGE_TRANSMITTER_ENABLE_OPTION) 
	{
		RegNewTc |= XXGE_TC_TX_MASK;
	}

	/* Enable receiver */
	if (DepOptions & XXGE_RECEIVER_ENABLE_OPTION) 
	{
		RegNewRcw1 |= XXGE_RCW1_RX_MASK;
	}

	/* Change the TC or RCW1 registers if they need to be modified */
	if (RegTc != RegNewTc) 
	{
		DEBUGP(DEBUG_INFO, "setOptions: writing tc: 0x%0x", RegNewTc);
		Xil_Out32(Adapter->MACRegisters + XXGE_TC_OFFSET, RegNewTc);
	}

	if (RegRcw1 != RegNewRcw1) 
	{
		DEBUGP(DEBUG_INFO, "setOptions: writing rcw1: 0x%0x", RegNewRcw1);
		Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, RegNewRcw1);
	}

    TempRegRcw1 = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
    TempRegTc = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);

    /*
     * Rest of options twiddle bits of other registers. Handle them one at
     * a time
     */

	/* Turn on flow control */
	if (DepOptions & XXGE_FLOW_CONTROL_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "setOptions: enabling flow control");
		Reg = Xil_In32(Adapter->MACRegisters + XXGE_FCC_OFFSET);
		Reg |= XXGE_FCC_FCRX_MASK;
		Xil_Out32(Adapter->MACRegisters + XXGE_FCC_OFFSET, Reg);
	}

	DEBUGP(DEBUG_INFO, "setOptions: rcw1 is now (fcc):0x%0x",
		Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET));

    /* Turn on promiscuous frame filtering (all frames are received ) */
    if (DepOptions & XXGE_PROMISC_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "setOptions: enabling promiscuous mode");
		Reg = Xil_In32(Adapter->XGERegisters + XXGE_AFC_OFFSET);
		Reg |= XXGE_AFC_PM_MASK;
		Xil_Out32(Adapter->XGERegisters + XXGE_AFC_OFFSET, Reg);	
    }
    DEBUGP(DEBUG_WARN, "setOptions: rcw1 is now (afm):0x%0x",
	    Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET));

    /*
     * The remaining options not handled here are managed elsewhere in the
     * driver. No register modifications are needed at this time.
     * Reflecting the option in Adapter->Options is good enough for
     * now.
     */
    DEBUGP(DEBUG_TRACE, "setOptions: returning SUCCESS\n");
    return STATUS_SUCCESS;
}

/*****************************************************************************/
/**
* XXgEthernet_ClearOptions clears the options, <i>Options</i> for the
* XGEMAC Ethernet, specified by <i>Adapter</i>. XGEMAC Ethernet should be stopped
* with XXgEthernet_Stop() before changing options.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @param	Options is a bitmask of OR'd XXGE_*_OPTION values for options to
*		clear. Options not specified are not affected.
*
* @return
*		- XST_SUCCESS on successful completion.
*		- XST_DEVICE_IS_STARTED if the device has not been stopped.
*
* @note
* See xxgethernet.h for a description of the available options.
*
******************************************************************************/
NTSTATUS
XXgEthernet_ClearOptions(
	PMP_ADAPTER Adapter,
	ULONG Options
	)
{
	ULONG Reg;	/* Generic */
	ULONG RegRcw1;	/* Reflects original contents of RCW1 */
	ULONG RegTc;	/* Reflects original contents of TC  */
	ULONG RegNewRcw1;	/* Reflects new contents of RCW1 */
	ULONG RegNewTc;	/* Reflects new contents of TC  */
	ULONG DepOptions;	/* Required dependent options for new features */

	DEBUGP(DEBUG_TRACE, "XXgEthernet_ClearOptions: 0x%08x", Options);

	if (Adapter == NULL)
		return STATUS_UNSUCCESSFUL;
    if (!MP_IS_READY(Adapter))
		return STATUS_UNSUCCESSFUL;

	/* Be sure device has been stopped */
    if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_STARTED)) 
	{
		DEBUGP(DEBUG_WARN, "XXgEthernet_ClearOptions Adapter Started!\n");
		return STATUS_UNSUCCESSFUL;
	}

	/*
	 * Set options word to its new value.
	 * The step is required before calling _UpdateDepOptions() since
	 * we are operating on updated options.
	 */
	Adapter->Options &= ~Options;

    DepOptions = 0xFFFFFFFF;

    /*
     * Many of these options will change the RCW1 or TC registers.
     * Group these options here and change them all at once. What we are
     * trying to accomplish is to reduce the amount of IO to the device
     */

	/* Grab current register contents */
	RegRcw1 = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	RegTc = Xil_In32(Adapter->MACRegisters + XXGE_TC_OFFSET);
	RegNewRcw1 = RegRcw1;
	RegNewTc = RegTc;

	/* Turn off jumbo packet support for both Rx and Tx */
	if (DepOptions & XXGE_JUMBO_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling jumbo");
		RegNewTc &= ~XXGE_TC_JUM_MASK;
		RegNewRcw1 &= ~XXGE_RCW1_JUM_MASK;
	}

    /* Turn off FCS stripping on receive packets */
    if (DepOptions & XXGE_FCS_STRIP_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling fcs strip");
		RegNewRcw1 |= XXGE_RCW1_FCS_MASK;
	}

	/* Turn off FCS insertion on transmit packets */
	if (DepOptions & XXGE_FCS_INSERT_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling fcs insert");
		RegNewTc |= XXGE_TC_FCS_MASK;
	}

	/* Turn off length/type field checking on receive packets */
	if (DepOptions & XXGE_LENTYPE_ERR_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling lentype err");
		RegNewRcw1 |= XXGE_RCW1_LT_DIS_MASK;
	}

	/* Disable transmitter */
	if (DepOptions & XXGE_TRANSMITTER_ENABLE_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling transmitter");
		RegNewTc &= ~XXGE_TC_TX_MASK;
	}

	/* Disable receiver */
	if (DepOptions & XXGE_RECEIVER_ENABLE_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling receiver");
		RegNewRcw1 &= ~XXGE_RCW1_RX_MASK;
	}

	/* Change the TC and RCW1 registers if they need to be
	 * modified
	 */
	if (RegTc != RegNewTc) 
	{
		DEBUGP(DEBUG_VERBOSE, "XXgEthernet_ClearOptions: setting TC: 0x%0x", RegNewTc);
		Xil_Out32(Adapter->MACRegisters + XXGE_TC_OFFSET, RegNewTc);
	}

	if (RegRcw1 != RegNewRcw1) 
	{
		DEBUGP(DEBUG_VERBOSE, "XXgEthernet_ClearOptions: setting RCW1: 0x%0x", RegNewRcw1);
		Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, RegNewRcw1);
	}

	/*
	 * Rest of options twiddle bits of other registers. Handle them one at
	 * a time
	 */

	/* Turn off flow control */
	if (DepOptions & XXGE_FLOW_CONTROL_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling flow control");
		Reg = Xil_In32(Adapter->MACRegisters + XXGE_FCC_OFFSET);
		Reg &= ~XXGE_FCC_FCRX_MASK;
		Xil_Out32(Adapter->MACRegisters + XXGE_FCC_OFFSET, Reg);
	}

	/* Turn off promiscuous frame filtering */
	if (DepOptions & XXGE_PROMISC_OPTION) 
	{
		DEBUGP(DEBUG_INFO, "XXgEthernet_ClearOptions: disabling promiscuous mode");
		Reg = Xil_In32(Adapter->XGERegisters + XXGE_AFC_OFFSET);
		Reg &= ~XXGE_AFC_PM_MASK;
		Xil_Out32(Adapter->XGERegisters + XXGE_AFC_OFFSET, Reg);	
	}

    /*
     * The remaining options not handled here are managed elsewhere in the
     * driver. No register modifications are needed at this time.
     * Reflecting the option in Adapter->Options is good enough for
     * now.
     */
    DEBUGP(DEBUG_TRACE, "ClearOptions: returning SUCCESS");
    return STATUS_SUCCESS;
}

/*****************************************************************************/
/**
* XXgEthernet_GetOptions returns the current option settings.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
*
* @return	Returns a bitmask of XXGE_*_OPTION constants,
*		each bit specifying an option that is currently active.
*
* @note
* See xxgethernet.h for a description of the available options.
*
******************************************************************************/
ULONG 
XXgEthernet_GetOptions(
	PMP_ADAPTER Adapter
	)
{
	if (Adapter == NULL)
		return 0;
    if (!MP_IS_READY(Adapter))
		return 0;
	return (Adapter->Options);
}


/*****************************************************************************/
/**
 * XAxiEthernet_GetOperatingSpeed gets the current operating link speed. This
 * may be the value set by XAxiEthernet_SetOperatingSpeed() or a hardware
 * default.
 *
 * @param	Adapter is a pointer to the Axi Ethernet instance to be
 *		worked on.
 *
 * @return	Returns the link speed in units of megabits per second (10 /
 *		100 / 1000).
 *		Can return a value of 0, in case it does not get a valid
 *		speed from EMMC.
 *
 * @note	None.
 *
 * @note
 *
 *
 ******************************************************************************/
void 
XXgEthernet_DisableControlFrameLenCheck(
	PMP_ADAPTER Adapter
	)
{
	ULONG RegRcw1;

	/* Grab current register contents */
	RegRcw1 = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	RegRcw1 |= XXGE_RCW1_CL_DIS_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, RegRcw1);
}

/*****************************************************************************/
/**
* XXgEthernet_EnableControlFrameLenCheck is used to enable the length check
* for control frames (pause frames). After calling the API, all control frames
* received will be checked for proper length (less than minimum frame length).
* By default, upon normal start up, control frame length check is enabled.
* Hence this API needs to be called only if previously the control frame length
* check has been disabled by calling the API
* XXgEthernet_DisableControlFrameLenCheck.
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
*
* @return	None.
*
* @note
*
******************************************************************************/
void 
XXgEthernet_EnableControlFrameLenCheck(
	PMP_ADAPTER Adapter
	)
{
	ULONG RegRcw1;

	/* Grab current register contents */
	RegRcw1 = Xil_In32(Adapter->MACRegisters + XXGE_RCW1_OFFSET);
	RegRcw1 &= ~XXGE_RCW1_CL_DIS_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_RCW1_OFFSET, RegRcw1);
}

/*****************************************************************************/
/**
* XXgEthernet_PhySetMdioDivisor sets the MDIO clock divisor in the
* XGEMAC Ethernet,specified by <i>Adapter</i> to the value, <i>Divisor</i>.
* This function must be called once after each reset prior to accessing
* MII PHY registers.
*
* From the XGEMAC User Guide, the following equation governs the MDIO clock to the PHY:
*
* <pre>
* 			f[HOSTCLK]
*	f[MDC] = -----------------------
*			(1 + Divisor) * 2
* </pre>
*
* where f[HOSTCLK] is the bus clock frequency in MHz, and f[MDC] is the
* MDIO clock frequency in MHz to the PHY. Typically, f[MDC] should not
* exceed 2.5 MHz. Some PHYs can tolerate faster speeds which means faster
* access.
*
* @param	Adapter references the XGEMAC Ethernet instance on which to
*		operate.
* @param	Divisor is the divisor value to set within the range of 0 to
*		XXGE_MDIO_CFG0_CLOCK_DIVIDE_MAX.
*
* @note	None.
*
******************************************************************************/
void 
XXgEthernet_PhySetMdioDivisor(
	PMP_ADAPTER Adapter,
	UCHAR Divisor
	)
{
	DEBUGP(DEBUG_TRACE, "XXgEthernet_PhySetMdioDivisor");

	if (Adapter == NULL)
		return;
    if (!MP_IS_READY(Adapter))
		return;
	if (Divisor > XXGE_MDIO_CFG0_CLOCK_DIVIDE_MAX)
		return;

	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_CFG0_OFFSET,
				(ULONG) Divisor | XXGE_MDIO_CFG0_MDIOEN_MASK);
}



/*****************************************************************************/
/*
* XXgEthernet_PhyRead reads the specified PHY register, <i>RegisterNum</i> on 
* the PHY specified by <i>PhyAddress</i> into <i>PhyDataPtr</i>. This Ethernet 
* driver does not require the device to be stopped before reading from the PHY.
* It is the responsibility of the calling code to stop the device if it is 
* deemed necessary.
*
* Note that the XGEMAC Ethernet hardware provides the ability to talk to a PHY
* that adheres to the Management Data Input/Output (MDIO) Interface. 
*
* <b>It is important that calling code set up the MDIO clock with
* XXgEthernet_PhySetMdioDivisor() prior to accessing the PHY with this
* function.
* </b>
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @param	PhyAddress is the address of the PHY to be written. This is a 
*       bitmask comprising a Port Address and a Device Address.
* @param	RegisterNum is the register number, 0-31, of the specific PHY
*		register to write.
* @param	PhyDataPtr is a reference to the location where the 16-bit
*		result value is stored.
*
* @return	None.
*
*
* @note
*
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the write is
* done). If this is of concern, the calling code should provide a mechanism
* suitable for recovery.
*
******************************************************************************/
void 
XXgEthernet_PhyRead(
	PMP_ADAPTER Adapter,
	ULONG PhyAddress,
	ULONG RegisterNum, 
	USHORT *PhyDataPtr
	)
{

#ifdef	MDIO_CHANGES
	ULONG MdioCtrlReg = 0;
	ULONG TimeoutLoops;

	/*
	 * Verify that each of the inputs are valid.
	 */
	if (Adapter == NULL)
		return;
	if (RegisterNum > XXGE_PHY_REG_NUM_LIMIT)
		return;

	DEBUGP(DEBUG_INFO, "PhyRead: BaseAddress %x Offset %x PhyAddress %x RegisterNum %d\n", 
            Adapter->MACRegisters, XXGE_MDIO_CFG1_OFFSET, PhyAddress, RegisterNum);

    /* Sequence of steps is:
     * - Set Address opcode (CFG1) and actual address (TX Data)
     * - RX Data opcode (CFG1) and actual data read (RX Data)
     * - Check for MDIO ready at every step
     */

	/*
	 * Wait till MDIO interface is ready to accept a new transaction.
	 */
	TimeoutLoops  = XXGE_LOOPS_FOR_MDIO_READY;
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		if (TimeoutLoops-- == 0) 
		{
			DEBUGP(DEBUG_ERROR, "Timed out waiting for MDIO Ready");
			return;
		}
	}
    DEBUGP(DEBUG_INFO, "MDIO CFG1 %x = %x\n", XXGE_MDIO_CFG1_OFFSET, 
            Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET));

    /* Now initiate the set PHY register address operation */
	MdioCtrlReg = PhyAddress | XXGE_MDIO_CFG1_INITIATE_MASK | XXGE_MDIO_CFG1_OP_SETADDR_MASK;
	DEBUGP(DEBUG_INFO, "Writing Base %x Offset %x = %x", 
        	Adapter->MACRegisters, XXGE_MDIO_CFG1_OFFSET, MdioCtrlReg);
	DEBUGP(DEBUG_INFO, "Writing Base %x Offset %x = %x", 
		Adapter->MACRegisters, XXGE_MDIO_TX_DATA_OFFSET, (RegisterNum & XXGE_MDIO_TX_DATA_MASK));
	
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_TX_DATA_OFFSET, (RegisterNum & XXGE_MDIO_TX_DATA_MASK));
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET, MdioCtrlReg);

	/*
	 * Wait till MDIO transaction is completed.
	 */
	TimeoutLoops  = XXGE_LOOPS_FOR_MDIO_READY;
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		if (TimeoutLoops-- == 0) 
		{
			DEBUGP(DEBUG_ERROR, "Timed out waiting for MDIO Ready");
			return;
		}
	}
    DEBUGP(DEBUG_INFO, "MDIO CFG1 %x = %x\n", XXGE_MDIO_CFG1_OFFSET, 
            Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET));

	/* Now initiate the read PHY register operation */
	MdioCtrlReg = PhyAddress | XXGE_MDIO_CFG1_INITIATE_MASK |
                XXGE_MDIO_CFG1_OP_READ_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET, MdioCtrlReg);
	/*
	 * Wait till MDIO transaction is completed.
	 */
	TimeoutLoops  = XXGE_LOOPS_FOR_MDIO_READY;
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		if (TimeoutLoops-- == 0) 
		{
			DEBUGP(DEBUG_ERROR, "Timed out waiting for MDIO Ready");
			return;
		}
	}
	*PhyDataPtr = (USHORT) Xil_In32(Adapter->MACRegisters + XXGE_MDIO_RX_DATA_OFFSET);
	DEBUGP(DEBUG_INFO, "XXgEthernet_PhyRead: Value retrieved: 0x%0x\n", *PhyDataPtr);
#else
	UNREFERENCED_PARAMETER(Adapter);
	UNREFERENCED_PARAMETER(PhyAddress);
	UNREFERENCED_PARAMETER(RegisterNum);

	*PhyDataPtr = 0x55AA;
#endif
}

/*****************************************************************************/
/*
* XXgEthernet_PhyWrite writes <i>PhyData</i> to the specified PHY register,
* <i>RegiseterNum</i> on the PHY specified by <i>PhyAddress</i>. This Ethernet
* driver does not require the device to be stopped before writing to the PHY.
* It is the responsibility of the calling code to stop the device if it is
* deemed necessary.
*
* <b>It is important that calling code set up the MDIO clock with
* XXgEthernet_PhySetMdioDivisor() prior to accessing the PHY with this
* function.</b>
*
* @param	Adapter is a pointer to the XGEMAC Ethernet instance to be
*		worked on.
* @param	PhyAddress is the address of the PHY to be written (multiple
*		PHYs supported).
* @param	RegisterNum is the register number, 0-31, of the specific PHY
*		register to write.
* @param	PhyData is the 16-bit value that will be written to the
*		register.
*
* @return	None.
*
* @note
*
* There is the possibility that this function will not return if the hardware
* is broken (i.e., it never sets the status bit indicating that the write is
* done). If this is of concern, the calling code should provide a mechanism
* suitable for recovery.
*
******************************************************************************/
void 
XXgEthernet_PhyWrite(
	PMP_ADAPTER Adapter, 
	ULONG PhyAddress,
	ULONG RegisterNum, 
	USHORT PhyData
	)
{
#ifdef  MDIO_CHANGES
	ULONG MdioCtrlReg = 0;

	/*
	 * Verify that each of the inputs are valid.
	 */
	if (Adapter == NULL)
		return;
	if (PhyAddress > XXGE_PHY_ADDR_LIMIT)
		return;
	if (RegisterNum > XXGE_PHY_REG_NUM_LIMIT)
		return;

	DEBUGP(DEBUG_INFO, "XXgEthernet_PhyWrite: BaseAddress: 0x%08x",
		Adapter->MACRegisters);

    /* Sequence of steps is:
     * - Set Address opcode (CFG1) and actual address (TX Data)
     * - TX Data opcode (CFG1) and actual data to be written (TX Data)
     * - Check for MDIO ready at every step
     */

#if 1
	/*
	 * Wait till the MDIO interface is ready to accept a new transaction.
	 */
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		;
	}
#endif
    DEBUGP(DEBUG_INFO, "MDIO CFG1 %x = %x", XXGE_MDIO_CFG1_OFFSET, 
            Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET));

    /* Now initiate the set PHY register address operation */
	MdioCtrlReg = PhyAddress | XXGE_MDIO_CFG1_INITIATE_MASK |
                XXGE_MDIO_CFG1_OP_SETADDR_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET, MdioCtrlReg);
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_TX_DATA_OFFSET, (RegisterNum & XXGE_MDIO_TX_DATA_MASK));

#if 1
	/*
	 * Wait till MDIO transaction is completed.
	 */
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		;
	}
#endif
    DEBUGP(DEBUG_INFO, "MDIO CFG1 %x = %x\n", XXGE_MDIO_CFG1_OFFSET, 
            Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET));

	/* Now initiate the write PHY register operation */
	MdioCtrlReg = PhyAddress | XXGE_MDIO_CFG1_INITIATE_MASK |
                XXGE_MDIO_CFG1_OP_WRITE_MASK;
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET, MdioCtrlReg);
	Xil_Out32(Adapter->MACRegisters + XXGE_MDIO_TX_DATA_OFFSET, (PhyData & XXGE_MDIO_TX_DATA_MASK));

#if 1
	/*
	 * Wait till the MDIO interface is ready to accept a new transaction.
	 */
	while (!(Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET) & XXGE_MDIO_CFG1_READY_MASK)) 
	{
		;
	}
#endif
    DEBUGP(DEBUG_INFO, "MDIO CFG1 %x = %x\n", XXGE_MDIO_CFG1_OFFSET, 
            Xil_In32(Adapter->MACRegisters + XXGE_MDIO_CFG1_OFFSET));
#else
	UNREFERENCED_PARAMETER(Adapter);
	UNREFERENCED_PARAMETER(PhyAddress);
	UNREFERENCED_PARAMETER(RegisterNum);
	UNREFERENCED_PARAMETER(PhyData);

#endif
}


#ifdef	MDIO_CHANGES
/*
 * The PHY registers read here should be standard registers in all PHY chips
 */
int 
GetPhyStatus(
	PMP_ADAPTER Adapter,
	int *linkup)
{
	USHORT reg;

	XXgEthernet_PhyRead(Adapter, Adapter->PhyAddress, XXGE_MDIO_REGISTER_ADDRESS, &reg);
#ifdef MDIO_CHANGES
	*linkup = reg & XXGE_MDIO_PHY_LINK_UP_MASK;
#else
//	 Forced this to 1 when there is no PHY
	*linkup = 1;
#endif
	return 0;
}
#endif

