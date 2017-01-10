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

    DevCon.cpp

Abstract:

    Helper functions to functions defined in DriverMgr.cpp.
	
Environment:

    User mode

--*/

#include "..\include\DevCon.h"


/*++

Routine Description:

    Generic enumerator for devices that will be passed the following arguments:
    <id> [<id>...]
    =<class> [<id>...]
    where <id> can either be @instance-id, or hardware-id and may contain wildcards
    <class> is a class name

Arguments:

    Flags    - extra enumeration flags (eg DIGCF_PRESENT)
    argc/argv - remaining arguments on command line
    Callback - function to call for each hit
    Context  - data to pass function for each hit

Return Value:

    EXIT_xxxx

--*/
int 
EnumerateDevices
(
	__in CallbackFunc Callback, 
	__in PCMD_CONTEXT pContext
)
{
    HDEVINFO		devs = INVALID_HANDLE_VALUE;
    int				Status = EXIT_FAIL;
    DWORD			devIndex;
    SP_DEVINFO_DATA devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
	int				FoundInstance = 0;

	if (pContext->DevType == DEV_TYPE_XDMA)
	{
		//
		// add all id's to list
		// if there's a class, filter on specified class
		//
		devs = SetupDiGetClassDevsEx(NULL, (PCWSTR)&PCI_CLASS_STR, NULL, DIGCF_ALLCLASSES, // | DIGCF_PRESENT,
						            NULL, NULL, NULL);
		if (devs != INVALID_HANDLE_VALUE) 
		{
			devInfoListDetail.cbSize = sizeof(devInfoListDetail);
			if (SetupDiGetDeviceInfoListDetail(devs, &devInfoListDetail)) 
			{
			    devInfo.cbSize = sizeof(devInfo);
				for(devIndex=0; SetupDiEnumDeviceInfo(devs, devIndex, &devInfo); devIndex++)
				{
					TCHAR devID[MAX_DEVICE_ID_LEN];
					LPTSTR *hwIds = NULL;
					LPTSTR *compatIds = NULL;
					//
					// determine instance ID
					//
					if (CM_Get_Device_ID_Ex(
									devInfo.DevInst,
									devID,
									MAX_DEVICE_ID_LEN,
									0,
									NULL)!= CR_SUCCESS) 
					{
						devID[0] = TEXT('\0');
					}
					// determine hardware ID's and search for matches
					hwIds = GetDevMultiSz(devs, &devInfo, SPDRP_HARDWAREID);
					compatIds = GetDevMultiSz(devs,&devInfo,SPDRP_COMPATIBLEIDS);
					if (WildCompareHwIds(hwIds, (LPCTSTR)&PCI_VEN_STR) ||
						WildCompareHwIds(compatIds, (LPCTSTR)&PCI_VEN_STR)) 
					{
						if (FoundInstance == pContext->Instance)
						{
						    //_tprintf(L"%d DeviceID: %s, ", 	FoundInstance, devID);
							Status = Callback(devs, &devInfo, pContext);
						}
						FoundInstance++;
					}
					DelMultiSz(hwIds);
					DelMultiSz(compatIds);
				}
			}
		}
	}
	else 
	{
		//
		// add all id's to list
		// if there's a class, filter on specified class
		//
		devs = SetupDiGetClassDevsEx(NULL, (PCWSTR)&XDMA_CLASS_STR, NULL, DIGCF_ALLCLASSES, // | DIGCF_PRESENT,
						            NULL, NULL, NULL);
		if (devs != INVALID_HANDLE_VALUE) 
		{
			devInfoListDetail.cbSize = sizeof(devInfoListDetail);
			if (SetupDiGetDeviceInfoListDetail(devs, &devInfoListDetail)) 
			{
			    devInfo.cbSize = sizeof(devInfo);
				for (devIndex=0; SetupDiEnumDeviceInfo(devs, devIndex, &devInfo); devIndex++)
				{
					TCHAR devID[MAX_DEVICE_ID_LEN];
					LPTSTR *hwIds = NULL;
					LPTSTR *compatIds = NULL;
					//
					// determine instance ID
					//
					if (CM_Get_Device_ID_Ex(devInfo.DevInst, devID, MAX_DEVICE_ID_LEN,	0, NULL) != CR_SUCCESS) 
					{
						devID[0] = TEXT('\0');
					}

					// determine hardware ID's and search for matches
					hwIds = GetDevMultiSz(devs, &devInfo, SPDRP_HARDWAREID);
					compatIds = GetDevMultiSz(devs, &devInfo, SPDRP_COMPATIBLEIDS);

					if (pContext->DevType == DEV_TYPE_XBLOCK)
					{

						if (WildCompareHwIds(hwIds, (LPCTSTR)&XBLOCK_VEN_STR) ||
							WildCompareHwIds(compatIds, (LPCTSTR)&XBLOCK_VEN_STR)) 
						{
							if (FoundInstance == pContext->Instance)
							{
							    //_tprintf(L"%d DeviceID: %s, ", 	FoundInstance, devID);
								Status = Callback(devs, &devInfo, pContext);
							}
							FoundInstance++;
						}
					}
					else
					{
						if (WildCompareHwIds(hwIds, (LPCTSTR)&XNET_VEN_STR) ||
							WildCompareHwIds(compatIds, (LPCTSTR)&XNET_VEN_STR)) 
						{
							if (FoundInstance == pContext->Instance)
							{
							    //_tprintf(L"%d DeviceID: %s, ", 	FoundInstance, devID);
								Status = Callback(devs, &devInfo, pContext);
							}
							FoundInstance++;
						}
					}
					DelMultiSz(hwIds);
					DelMultiSz(compatIds);
				}
			}
		}
	}
	if (devs != INVALID_HANDLE_VALUE) 
	{
        SetupDiDestroyDeviceInfoList(devs);
	}
    return Status;
}

/*++

Routine Description:

    Callback for use by Enable/Disable/Restart
    Invokes DIF_PROPERTYCHANGE with correct parameters
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

    In Enable case, we try global first, and if still disabled, enable local

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
int ControlCallback
	(
		__in HDEVINFO Devs, 
		__in PSP_DEVINFO_DATA DevInfo,
		__in PCMD_CONTEXT pContext
	)
{
    SP_PROPCHANGE_PARAMS pcp;
    SP_DEVINSTALL_PARAMS devParams;
    int Status = EXIT_FAIL;

	switch(pContext->Control) 
	{
        case DICS_ENABLE:
            //
            // enable both on global and config-specific profile
            // do global first and see if that succeeded in enabling the device
            // (global enable doesn't mark reboot required if device is still
            // disabled on current config whereas vice-versa isn't true)
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pContext->Control;
            pcp.Scope = DICS_FLAG_GLOBAL;
            pcp.HwProfile = 0;
            //
            // don't worry if this fails, we'll get an error when we try config-
            // specific.
            if (SetupDiSetClassInstallParams(Devs, DevInfo, &pcp.ClassInstallHeader, sizeof(pcp))) 
			{
				//printf("ControlCallback: Global Enabled failed 0x%x\n", GetLastError());
				SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo);
            }
            //
            // now enable on config-specific
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pContext->Control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

        default:
            //
            // operate on config-specific profile
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pContext->Control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;
    }

    if (!SetupDiSetClassInstallParams(Devs, DevInfo, &pcp.ClassInstallHeader, sizeof(pcp))) 
	{
		printf("ControlCallback: SetupDiSetClassInstallParams Failed 0x%x\n", GetLastError());
	}
//    if (!SetupDiSetClassInstallParams(Devs, DevInfo, &pcp.ClassInstallHeader, sizeof(pcp)) ||
//       !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, Devs, DevInfo)) 
	if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, Devs, DevInfo))
	{
		printf("ControlCallback: DIF_PROPERTYCHANGE Failed 0x%x\n", GetLastError());
        //
        // failed to invoke DIF_PROPERTYCHANGE
        //
		Status = EXIT_FAIL;
    } 
	else 
	{
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if (SetupDiGetDeviceInstallParams(Devs, DevInfo, &devParams) && 
			(devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) 
		{
			printf("ControlCallback: DIF_PROPERTYCHANGE Will need to reboot system\n");
			Status = EXIT_REBOOT;
        } 
		else 
		{
            //
            // appears to have succeeded
            //
			//printf("ControlCallback: Success\n");
			Status = EXIT_OK;
        }
        pContext->Count++;
    }
    return Status;
}

/*++

Routine Description:

    Callback for use by Find/FindAll
    just simply display the device

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
int FindCallback
	(
		__in HDEVINFO Devs, 
		__in PSP_DEVINFO_DATA DevInfo,
		__in PCMD_CONTEXT pContext
	)
{
    //if (!DumpDeviceWithInfo(Devs, DevInfo, NULL)) 
	//{
    //    return EXIT_OK;
   // }
    if (pContext->Control & FIND_DEVICE) 
	{
		DeviceDescr(Devs, DevInfo);
    }
    if (pContext->Control & FIND_STATUS) 
	{
		pContext->pDevStats->DevStats[pContext->Count].Instance = pContext->Instance;
		pContext->pDevStats->DevStats[pContext->Count].DriverStatus = DeviceStatus(Devs, DevInfo);
    }
    pContext->Count++;
	pContext->pDevStats->ReturnNumDevices = pContext->Count;
    return EXIT_OK;
}


LPTSTR GetDeviceStringProperty(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop)
/*++

Routine Description:

    Return a string property for a device, otherwise NULL

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )
    Prop     - string property to obtain

Return Value:

    string containing description

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    DWORD szChars;

    size = 1024; // initial guess
    buffer = new TCHAR[(size/sizeof(TCHAR))+1];
    if(!buffer) 
	{
        return NULL;
    }
    while (!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) 
	{
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		{
            goto failed;
        }
        if(dataType != REG_SZ) 
		{
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+1];
        if(!buffer) 
		{
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    return buffer;

failed:
    if (buffer) 
	{
        delete [] buffer;
    }
    return NULL;
}


/*++

Routine Description:

    Write device description to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    TRUE if success

--*/
BOOL DeviceDescr
	(
		__in HDEVINFO Devs, 
		__in PSP_DEVINFO_DATA DevInfo
	)
{
    LPTSTR desc;

    desc = GetDeviceStringProperty(Devs, DevInfo, SPDRP_FRIENDLYNAME);
    if (desc != NULL) 
	{
	    //_tprintf(L"Device Friendly Name: %s\n", desc);
	}
	else
	{
        desc = GetDeviceStringProperty(Devs, DevInfo, SPDRP_DEVICEDESC);
	    if (desc != NULL) 
		{
		   //_tprintf(L"Device Desc: %s\n", desc);
		}
		else
		{
			return FALSE;
		}
    }
    delete [] desc;
    return TRUE;
}


/*++

Routine Description:

    Write device status to stdout

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    none

--*/
int DeviceStatus
	(
		__in HDEVINFO Devs, 
		__in PSP_DEVINFO_DATA DevInfo
	)
{
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    ULONG status = 0;
    ULONG problem = 0;
    BOOL hasInfo = FALSE;
    BOOL isPhantom = FALSE;
    CONFIGRET cr = CR_SUCCESS;
	int		RetStatus;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(
		Devs, 
		&devInfoListDetail)) ||
      ((cr = CM_Get_DevNode_Status_Ex(
			&status,
			&problem,
			DevInfo->DevInst,
			0,
			devInfoListDetail.RemoteMachineHandle))!=CR_SUCCESS)) 
	{
        if ((cr == CR_NO_SUCH_DEVINST) || (cr == CR_NO_SUCH_VALUE)) 
		{
            isPhantom = TRUE;
        } 
		else 
		{
			return	DEVICE_STATUS_NOT_FOUND;
        }
    }
    //
    // handle off the status/problem codes
    //
    if (isPhantom) {
		RetStatus = DEVICE_STATUS_PHANTOM;
    }
	else 
	{
		if ((status & DN_HAS_PROBLEM) && problem == CM_PROB_DISABLED) 
		{
	        hasInfo = TRUE;
			RetStatus =	DEVICE_STATUS_DISABLED;
		}
		else if (status & DN_HAS_PROBLEM) 
		{
	        hasInfo = TRUE;
			RetStatus =	DEVICE_STATUS_HAS_PROBLEM;
	    }
		if (status & DN_PRIVATE_PROBLEM) 
		{
			hasInfo = TRUE;
			RetStatus = DEVICE_STATUS_HAS_PRIVATE_PROBLEM;
	    }
		if(status & DN_STARTED) 
		{
			RetStatus = DEVICE_STATUS_ENABLED;
		} 
		else if (!hasInfo) 
		{
			RetStatus = DEVICE_STATUS_NOT_STARTED;
		}
    }
    return RetStatus;
}



/*++

Routine Description:

    Compares all strings in Array against Id
    Use WildCardMatch to do real compare

Arguments:

    Array - pointer returned by GetDevMultiSz
    MatchEntry - string to compare against

Return Value:

    TRUE if any match, otherwise FALSE

--*/
BOOL WildCompareHwIds
	(
		__in PZPWSTR Array, 
		__in LPCTSTR MatchString
	)
{
    if (Array) 
	{
        while (Array[0]) 
		{
	        if (_tcsicmp(Array[0], MatchString) == 0)
			{
                return TRUE;
            }
            Array++;
        }
    }
    return FALSE;
}


/*++

Routine Description:

    Get an index array pointing to the MultiSz passed in

Arguments:

    MultiSz - well formed multi-sz string

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
LPTSTR * GetMultiSzIndexArray
	(
		__in LPTSTR MultiSz
	)
{
    LPTSTR scan;
    LPTSTR * array;
    int elements;

    for(scan = MultiSz, elements = 0; scan[0] ;elements++) 
	{
        scan += lstrlen(scan)+1;
    }
    array = new LPTSTR[elements+2];
    if(!array) 
	{
        return NULL;
    }
    array[0] = MultiSz;
    array++;
    if(elements) 
	{
        for(scan = MultiSz, elements = 0; scan[0]; elements++) 
		{
            array[elements] = scan;
            scan += lstrlen(scan)+1;
        }
    }
    array[elements] = NULL;
    return array;
}

/*++

Routine Description:

    Get a multi-sz device property
    and return as an array of strings

Arguments:

    Devs    - HDEVINFO containing DevInfo
    DevInfo - Specific device
    Prop    - SPDRP_HARDWAREID or SPDRP_COMPATIBLEIDS

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
LPTSTR * GetDevMultiSz
	(
		__in HDEVINFO Devs, 
		__in PSP_DEVINFO_DATA DevInfo, 
		__in DWORD Prop
	)
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer) 
	{
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) 
	{
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		{
            goto failed;
        }
        if(dataType != REG_MULTI_SZ) 
		{
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer) 
		{
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');
    array = GetMultiSzIndexArray(buffer);
    if(array) 
	{
        return array;
    }

failed:
    if(buffer) 
	{
        delete [] buffer;
    }
    return NULL;
}


/*++

Routine Description:

    Deletes the string array allocated by GetDevMultiSz/GetRegMultiSz/GetMultiSzIndexArray

Arguments:

    Array - pointer returned by GetMultiSzIndexArray

Return Value:

    None

--*/
void DelMultiSz
	(
		__in PZPWSTR Array
	)
{
    if(Array) 
	{
        Array--;
        if(Array[0]) 
		{
            delete [] Array[0];
        }
        delete [] Array;
    }
}
