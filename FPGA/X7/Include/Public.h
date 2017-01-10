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

    Public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications. This GUID is used by applications to 'Open'
    the driver by using the CreateFile function.

Environment:

    user and kernel

--*/
#include <initguid.h>
//  The GUID for Opening the interface to the XDMA Driver
// 
// {C286CF1E-A04D-4ED6-A67E-E5DCE6B658C1}
DEFINE_GUID(GUID_V7_XDMA_INTERFACE, 
0xc286cf1e, 0xa04d, 0x4ed6, 0xa6, 0x7e, 0xe5, 0xdc, 0xe6, 0xb6, 0x58, 0xc1);

// The GUID for Opening the interface to the XBlock0 Driver
//
// {11BEC5B8-F13E-4A17-BD66-44F30FD7F537}
DEFINE_GUID(GUID_V7_XBLOCK_INTERFACE, 
0x11bec5b8, 0xf13e, 0x4a17, 0xbd, 0x66, 0x44, 0xf3, 0xf, 0xd7, 0xf5, 0x37);

#define BUSENUM_COMPATIBLE_IDS L"{99FB84E5-3F33-4A8C-A8C8-2C7E20B27527}\0"
#define BUSENUM_COMPATIBLE_IDS_LENGTH sizeof(BUSENUM_COMPATIBLE_IDS)


