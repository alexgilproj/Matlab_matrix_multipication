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

    DriverStatus.cpp

Abstract:

    Defines the exported functions for the DLL application.
    This dll is invoked by the LandingPage of the Java GUI.
Environment:

    User mode

--*/



#include "stdafx.h"
#include "../DriverMgr/Include/DriverMgr.h"
#include "com_xilinx_gui_DrvStatus.h"

int DrvStatus(
	int		BoardNumber,
	int		Device
	)
{
	DEV_STATUS_LIST		DevList;
	unsigned int		status = 0;
	int					i;
	int					MaxDevices = 8; 

	DevList.DevStatusSize = sizeof(DEV_STATUS_LIST);

	status = cmdStatus(BoardNumber, Device, &DevList);
	// Check the status
	if (status == 0)
	{
		if (DevList.ReturnNumDevices < MaxDevices)
			MaxDevices = DevList.ReturnNumDevices;

		if (MaxDevices < BoardNumber)
		{
			printf("Board Number is out of range %d, Max %d\n", BoardNumber, MaxDevices);
			return -1;
		}

		for (i = 0; i < MaxDevices; i++)
		{
			if (BoardNumber == i)
			{
				status = -DevList.DevStats[i].DriverStatus;

				switch (DevList.DevStats[i].DriverStatus)
				{
					case DEVICE_STATUS_NOT_FOUND:
						printf("No Driver found for instance %d\n", DevList.DevStats[i].Instance);
						status = 10;
						break;
					case DEVICE_STATUS_PHANTOM:
						printf("Phantom device found for instance %d\n", DevList.DevStats[i].Instance);
						break;
					case DEVICE_STATUS_DISABLED:
						printf("Driver for instance %d is disabled\n", DevList.DevStats[i].Instance);
						status = 0;
						break;
					case DEVICE_STATUS_ENABLED:
						//printf("Driver for instance %d is enabled\n", DevList.DevStats[i].Instance);
						status = 0;
						break;
					case DEVICE_STATUS_HAS_PROBLEM:
						printf("Driver for instance %d has a problem\n", DevList.DevStats[i].Instance);
						break;
					case DEVICE_STATUS_HAS_PRIVATE_PROBLEM:
						printf("Driver for instance %d has an internal problem\n", DevList.DevStats[i].Instance);
							break;
					case DEVICE_STATUS_STARTED:
						//printf("Driver for instance %d is Started\n", DevList.DevStats[i].Instance);
						break;
					case DEVICE_STATUS_NOT_STARTED:
						printf("Driver for instance %d is Not Started\n", DevList.DevStats[i].Instance);
						break;
					default:
						printf("Driver for instance %d is in an unknown state\n", DevList.DevStats[i].Instance);
				}
			}
		}
	}
	else
	{
		printf("DriverStatus: Error returned. status = 0x%x\n", status);
	}
	return status;
}


JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getBoardNumber(JNIEnv *env, jobject obj){
	int i = 0;
	int status = 0;
	for (i = 0; i < 4; i++)
	{
		status = DrvStatus(i, DEV_TYPE_XDMA);
		if (status == 0)
			break;
	}
	if (status != 0)
		return -1;
	return i;
}

JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_cmdDisable(JNIEnv *env, jobject jobj, jint bnum){
	cmdDisable(bnum, DEV_TYPE_XDMA);
}
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_cmdEnable(JNIEnv *env, jobject jobj, jint bnum){
	cmdEnable(bnum, DEV_TYPE_XDMA);
}
JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getMode(JNIEnv *env, jobject jobj, jint bnum){
	// 1 - Performance mode, 2 - App mode
	return GetDriverChildConfig(bnum);
}

JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getRawMode(JNIEnv *env, jobject jobj, jint bnum){
	// 0 - off 1 -on
	return GetRawEthernet(bnum);
}

JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getTestConfig (JNIEnv *env, jobject jobj, jint bnum){
	// 1 - Internal buffers, 0 - Application buffers
	return GetTestConfig(bnum);
}

JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setTestConfig (JNIEnv *env, jobject jobj, jint bnum, jint option){
	SetTestConfig(bnum, option);
}

JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setRawMode (JNIEnv *env, jobject jobj, jint bnum, jint option){
	SetRawEthernet(bnum, option);
}

JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setMode (JNIEnv *env, jobject jobj, jint bnum, jint option){
	SetDriverChildConfig(bnum, option);
}