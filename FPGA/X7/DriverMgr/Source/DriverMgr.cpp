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

    DriverMgr.cpp

Abstract:

Defines the entry points for this Library.
DriverMgr implements a subset of Device Manger functions such as:
		Driver Disable,
		Driver Enable,
		Driver Status,
		Rescan
    
	
Environment:

    User mode

--*/

#include "..\include\DevCon.h"
#include "../../Include/Public.h"


/*++

Routine Description:

    cmdStatus <BoardNumber, Device>

Arguments:
	BoardNumber  Selects the Board number. Starts at 0, -1 = special case - show all drivers status
	Device is the Driver Type, XDMA, XBlock or XNet

Return Value:

    EXIT_xxxx

--*/
int cmdStatus
(
	int				BoardNumber,
	int				Device,
	PDEV_STATUS_LIST pDevStatus
)
{
    CMD_CONTEXT		context;
    int				failcode;
	UINT			instance;

	context.Count = 0;
	context.DevType = Device;
	if (BoardNumber == -1)
	{
		if (pDevStatus->DevStatusSize < sizeof(DEV_STATUS_LIST))
		{
			failcode = EXIT_FAIL;
		}
		for (instance = 0; instance < MAX_NUMBER_DEVICES_SUPPORTED; instance++)
		{
			// Initialize all the slots in the table with no driver found
			pDevStatus->DevStats[instance].Instance = instance;
			pDevStatus->DevStats[instance].DriverStatus = DEVICE_STATUS_NOT_FOUND;

			context.Control = FIND_DEVICE | FIND_STATUS;
			context.pDevStats = pDevStatus;
			context.Instance = (int)instance;
			failcode = EnumerateDevices(FindCallback, &context);
		}
		pDevStatus->ReturnNumDevices = MAX_NUMBER_DEVICES_SUPPORTED;
		failcode = EXIT_OK;
	}
	else
	{
		// Initialize with no driver found
		pDevStatus->DevStats[context.Count].Instance = BoardNumber;
		pDevStatus->DevStats[context.Count].DriverStatus = -1;

		context.Count = 0;
		context.Control = FIND_DEVICE | FIND_STATUS;
		context.pDevStats = pDevStatus;
		context.Instance = (int)BoardNumber; 
		failcode = EnumerateDevices(FindCallback, &context);
	}
    return failcode;
}

/*++

Routine Description:

    cmdEnable <BoardNumber, Device>

Arguments:
	BoardNumber  Selects the Board number. Starts at 0.
	Device is the Driver Type, XDMA, XBlock or XNet

Return Value:

    EXIT_xxxx

--*/
int cmdEnable
(
	int				BoardNumber,
	int				Device
)
{
	CMD_CONTEXT		context;
    int				failcode = EXIT_FAIL;

    context.Control = DICS_ENABLE;
	context.Count = 0;
	context.Instance = BoardNumber;
	context.DevType = Device;
	context.pDevStats = NULL;
	failcode = EnumerateDevices(ControlCallback, &context);
    return failcode;
}

/*++

Routine Description:

    cmdDisable <BoardNumber , Device> ... starting at 0

Arguments:

    BoardNumber of Driver instance to Disable
	Device is the Driver Type, XDMA, XBlock or XNet

Return Value:

    EXIT_xxxx 

--*/
int cmdDisable
(
	int				BoardNumber,
	int				Device
)
{
	CMD_CONTEXT		context;
    int				failcode = EXIT_FAIL;

    context.Control = DICS_DISABLE;
	context.Count = 0;
	context.Instance = BoardNumber;
	context.DevType = Device;
	context.pDevStats = NULL;
	failcode = EnumerateDevices(ControlCallback, &context);
    return failcode;
}


/*++

Routine Description:

    cmdRescan
    
	rescan for new devices

Arguments:

Return Value:

    EXIT_xxxx

--*/
int cmdRescan
	(
	)
{
    //
    // reenumerate from the root of the devnode tree
    // totally CM based
    //
    int failcode = EXIT_FAIL;
    HMACHINE machineHandle = NULL;
    DEVINST devRoot;

    if (CM_Locate_DevNode_Ex(
			&devRoot,
			NULL,
			CM_LOCATE_DEVNODE_NORMAL,
			machineHandle) == CR_SUCCESS) 
	{
	    if(CM_Reenumerate_DevNode_Ex(devRoot, 0, machineHandle) == CR_SUCCESS) 
		{
			failcode = EXIT_OK;
		}
	}

    if(machineHandle) 
	{
        CM_Disconnect_Machine(machineHandle);
    }
    return failcode;
}


// GetDriverChildConfig
//
// Returns the value of the Child Driver Config registry key for 
//  the BoardNumber given.
//
long GetDriverChildConfig
	(
	int				BoardNumber
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegChildDriverConfig;
	DWORD			dwLen;
	long			lChildDriverResult;

	// Get the device info class for our GUID
	//printf("boardNumber = %d\n",BoardNumber);
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				dwLen = sizeof(RegChildDriverConfig);
				lChildDriverResult = RegQueryValueEx(hDrvSoftInfoKey, TEXT("ChildDriverConfig"),
					NULL, NULL, (LPBYTE)&RegChildDriverConfig, &dwLen );
				RegCloseKey( hDrvSoftInfoKey );
				if (lChildDriverResult == ERROR_SUCCESS)
				{
					return RegChildDriverConfig;
				}
				else
				{
					printf("GetDriverChildConfig: Failed to find Reg Key, %d\n",GetLastError());
				}
			}
			else
			{
				printf("GetDriverChildConfig: Failed to open reg key, %d\n", GetLastError());
			}
		}
		else
		{
			printf("GetDriverChildConfig: Failed to find board info. %d\n",GetLastError());
		}
	}
	else
	{
		printf("GetDriverChildConfig: Failed to find GUID %d\n", GetLastError());
	}
	return -1;
}



// SetDriverChildConfig
//
// Sets the value of the Child Driver Config registry key for 
//  the BoardNumber given.
//
long SetDriverChildConfig
	(
	int				BoardNumber,
	long			DriverChildConfig
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegChildDriverConfig = DriverChildConfig;
	long			lChildDriverResult;

	// Get the device info class for our GUID
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_SET_VALUE);
			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				lChildDriverResult = RegSetValueEx(hDrvSoftInfoKey, TEXT("ChildDriverConfig"),
					NULL, REG_DWORD, (LPBYTE)&RegChildDriverConfig, sizeof(RegChildDriverConfig));
				RegCloseKey( hDrvSoftInfoKey );
				if (lChildDriverResult == ERROR_SUCCESS)
				{
					return lChildDriverResult;
				}
				else
				{
					printf("SetDriverChildConfig: Failed to find Reg Key, error %d\n", lChildDriverResult);
				}
			}
			else
			{
				printf("SetDriverChildConfig: Failed to open reg key\n");
			}
		}
		else
		{
			printf("SetDriverChildConfig: Failed to find board info.\n");
		}
	}
	else
	{
		printf("SetDriverChildConfig: Failed to find GUID\n");
	}
	return -1;
}


// GetRawEthernet
//
// Returns the value of the Raw Ethernet registry key for 
//  the BoardNumber given.
//
long GetRawEthernet
	(
	int				BoardNumber
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegRawEthernet = RAW_ETHERNET_OFF;
	DWORD			dwLen;
	long			lRawEthernetResult;

	// Get the device info class for our GUID
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				dwLen = sizeof(RegRawEthernet);
				lRawEthernetResult = RegQueryValueEx(hDrvSoftInfoKey, TEXT("RawEthernet"),
					NULL, NULL, (LPBYTE)&RegRawEthernet, &dwLen );
				RegCloseKey( hDrvSoftInfoKey );
				if (lRawEthernetResult == ERROR_SUCCESS)
				{
					return RegRawEthernet;
				}
				else
				{
					printf("GetRawEthernet: Failed to find Reg Key, error %d\n", lRawEthernetResult);
				}
			}
			else
			{
				printf("GetRawEthernet: Failed to open reg key\n");
			}
		}
		else
		{
			printf("GetRawEthernet: Failed to find board info.\n");
		}
	}
	else
	{
		printf("GetRawEthernet: Failed to find GUID\n");
	}
	return RegRawEthernet;
}



// SetRawEthernet
//
// Sets the value of the Raw Ethernet registry key for 
//  the BoardNumber given.
//
long SetRawEthernet
	(
	int				BoardNumber,
	long			RawEthernet
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegRawEthernet = RawEthernet;
	long			lRawEthernetResult;

	// Get the device info class for our GUID
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_SET_VALUE);
			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				lRawEthernetResult = RegSetValueEx(hDrvSoftInfoKey, TEXT("RawEthernet"),
					NULL, REG_DWORD, (LPBYTE)&RegRawEthernet, sizeof(RegRawEthernet));
				RegCloseKey( hDrvSoftInfoKey );
				if (lRawEthernetResult == ERROR_SUCCESS)
				{
					return lRawEthernetResult;
				}
				else
				{
					printf("SetRawEthernet: Failed to find Reg Key, error %d\n", lRawEthernetResult);
				}
			}
			else
			{
				printf("SetRawEthernet: Failed to open reg key\n");
			}
		}
		else
		{
			printf("SetRawEthernet: Failed to find board info.\n");
		}
	}
	else
	{
		printf("SetRawEthernet: Failed to find GUID\n");
	}
	return -1;
}


// GetTestConfig
//
// Returns the value of the Test Config registry key for 
//  the BoardNumber given.
//
long GetTestConfig
	(
	int				BoardNumber
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegTestConfig = TEST_CONFIG_INTERNAL_BUFFER;
	DWORD			dwLen;
	long			lTestConfigResult;

	// Get the device info class for our GUID
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				dwLen = sizeof(RegTestConfig);
				lTestConfigResult = RegQueryValueEx(hDrvSoftInfoKey, TEXT("TestConfig"),
					NULL, NULL, (LPBYTE)&RegTestConfig, &dwLen );
				RegCloseKey( hDrvSoftInfoKey );
				if (lTestConfigResult == ERROR_SUCCESS)
				{
					return RegTestConfig;
				}
				else
				{
					printf("GetTestConfig: Failed to find Reg Key, error %d\n", lTestConfigResult);
				}
			}
			else
			{
				printf("GetTestConfig: Failed to open reg key\n");
			}
		}
		else
		{
			printf("GetTestConfig: Failed to find board info.\n");
		}
	}
	else
	{
		printf("GetTestConfig: Failed to find GUID\n");
	}
	return RegTestConfig;
}


// SetTestConfig
//
// Sets the value of the Test Config registry key for 
//  the BoardNumber given.
//
long SetTestConfig
	(
	int				BoardNumber,
	long			modeConfig
	)
{
	SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	HDEVINFO		hDevInfo;
	HKEY			hDrvSoftInfoKey;
	DWORD			RegModeConfig = modeConfig;
	long			lModeConfigResult;

	// Get the device info class for our GUID
	hDevInfo = SetupDiGetClassDevs(&GUID_V7_XDMA_INTERFACE, NULL, NULL,
		 DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
		return GetLastError();
	}

	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_V7_XDMA_INTERFACE,
			 BoardNumber, &DeviceInterfaceData) == TRUE)
	{
		// Get the DeviceInfo
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		if (SetupDiEnumDeviceInfo(hDevInfo, BoardNumber, &DeviceInfoData))
		{
			// Open the registry for us to query
			hDrvSoftInfoKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, 
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_SET_VALUE);
			if( hDrvSoftInfoKey != INVALID_HANDLE_VALUE )
			{
				lModeConfigResult = RegSetValueEx(hDrvSoftInfoKey, TEXT("TestConfig"),
					NULL, REG_DWORD, (LPBYTE)&RegModeConfig, sizeof(RegModeConfig));
				RegCloseKey( hDrvSoftInfoKey );
				if (lModeConfigResult == ERROR_SUCCESS)
				{
					return lModeConfigResult;
				}
				else
				{
					printf("SetRawEthernet: Failed to find Reg Key, error %d\n", lModeConfigResult);
				}
			}
			else
			{
				printf("SetRawEthernet: Failed to open reg key\n");
			}
		}
		else
		{
			printf("SetRawEthernet: Failed to find board info.\n");
		}
	}
	else
	{
		printf("SetRawEthernet: Failed to find GUID\n");
	}
	return -1;
}
