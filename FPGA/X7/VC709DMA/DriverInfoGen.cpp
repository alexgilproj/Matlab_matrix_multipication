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

DriverInfoGen.cpp

Abstract:

Defines the exported functions for the DLL application.
This dll is invoked by the MainScreen of the Java GUI.
Environment:

User mode

--*/


#include "stdafx.h"
#include "../DriverMgr/Include/DriverMgr.h"
#include "windriver.h"
#include "stdio.h"

#include "DriverInfoGen.h"

#define SPIN_COUNT_FOR_CS       0x4000

// Driver Instances (Corresponds to Application)
#define	XBLOCK_DRIVER0			0
#define	XBLOCK_DRIVER1			1

// Entries in the GUID Table
#define	XBLOCK_DRIVER			0
#define	XDMA_DRIVER				1
#define DRIVER_INDEX_MAX		2

GUID    GuidTable[] = {
	GUID_V7_XBLOCK_INTERFACE,
	GUID_V7_XDMA_INTERFACE,
};

#define MAX_STATS 350
#define MULTIPLIER  8
#define DIVISOR     (1024*1024*1024)    /* Graph is in Gbits/s */

struct
{
	int Engine;         /* Engine number - for communicating with driver */
	//char *name;         /* Name to be used in Setup screen */
	//int mode;           /* TX/RX - incase of specific screens */
} DMAConfig[MAX_ENGS] =
{
	{ 0/*, LABEL1, TX_MODE*/ },
	{ 4/*, LABEL1, RX_MODE*/ },
	{ 1/*, LABEL2, TX_MODE*/ },
	{ 5/*, LABEL2, RX_MODE*/ },
	{ 2/*, LABEL3, TX_MODE*/ },
	{ 6/*, LABEL3, RX_MODE*/ },
	{ 3/*, LABEL4, TX_MODE*/ },
	{ 7/*, LABEL4, RX_MODE*/ }
};

int getErrorCount0();
int getErrorCount1();
FILE *f;
// DrvStatus
//
// Function to handle the "status" command.
// 
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
			//printf("Board Number is out of range %d, Max %d\n", BoardNumber, MaxDevices);
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
					//printf("No Driver found for instance %d\n", DevList.DevStats[i].Instance);
					status = 10;
					break;
				case DEVICE_STATUS_PHANTOM:
					//printf("Phantom device found for instance %d\n", DevList.DevStats[i].Instance);
					break;
				case DEVICE_STATUS_DISABLED:
					//printf("Driver for instance %d is disabled\n", DevList.DevStats[i].Instance);
					status = 0;
					break;
				case DEVICE_STATUS_ENABLED:
					//printf("Driver for instance %d is enabled\n", DevList.DevStats[i].Instance);
					status = 0;
					break;
				case DEVICE_STATUS_HAS_PROBLEM:
					//printf("Driver for instance %d has a problem\n", DevList.DevStats[i].Instance);
					break;
				case DEVICE_STATUS_HAS_PRIVATE_PROBLEM:
					//printf("Driver for instance %d has an internal problem\n", DevList.DevStats[i].Instance);
					break;
				case DEVICE_STATUS_STARTED:
					//printf("Driver for instance %d is Started\n", DevList.DevStats[i].Instance);
					break;
				case DEVICE_STATUS_NOT_STARTED:
					//printf("Driver for instance %d is Not Started\n", DevList.DevStats[i].Instance);
					break;
				default:
					//printf("Driver for instance %d is in an unknown state\n", DevList.DevStats[i].Instance);
					break;
				}
			}
		}
	}
	else
	{
		//printf("DriverStatus: Error returned. status = 0x%x\n", status);
	}
	return status;
}

int getBoardNumber(){
	int i = 0;
	int status = 0;
	for (i = 0; i < 10; i++)
	{
		status = DrvStatus(i, DEV_TYPE_XDMA);
		if (status == 0)
			break;
	}
	if (status != 0)
		return -1;
	return i;
}

BOOL
startTest(int eng, int mode, int size)
{
	//printf("## mode = %x \n",mode);

	if ((mode & (ENABLE_PKTGEN)) && (mode & ENABLE_PKTCHK))
	{
		if (SetTestParams((TEST_START | ENABLE_PKTGEN | ENABLE_PKTCHK), size, eng) != 0)
		{
			return FALSE;
		}
		if (!bInternalXferMode)
		{
			//Sleep(500);
			ThreadedReadWriteTest(eng, size, mode);
		}
	}
	else if (mode & ENABLE_PKTCHK)
	{
		if (SetTestParams((TEST_START | ENABLE_PKTCHK), size, eng) != 0)
		{
			return FALSE;
		}
		if (!bInternalXferMode)
		{
			//Sleep(500);
			ThreadedWriteTest(eng, size, mode);
		}
	}
	else if (mode & ENABLE_PKTGEN)
	{
		if (SetTestParams((TEST_START | ENABLE_PKTGEN), size, eng) != 0)
		{
			return FALSE;
		}
		if (!bInternalXferMode)
		{
			//Sleep(500);
			ThreadedReadTest(eng, size, mode);
		}
	}
	else if (mode & ENABLE_LOOPBACK)
	{
		if (SetTestParams((TEST_START | ENABLE_LOOPBACK), size, eng) != 0)
		{
			return FALSE;
		}
		if (!bInternalXferMode)
		{
			//Sleep(500);
			ThreadedReadWriteTest(eng, size, mode);
		}
	}
	return true;
}

BOOL
stopTest(int eng, int mode, int size)
{
	PUCHAR drainFIFObuffer;

	if (!bInternalXferMode)
	{
		waitThreadsStop(eng);
	}
	if ((mode & ENABLE_PKTCHK) && (mode & ENABLE_PKTGEN))
		SetTestParams((TEST_STOP | ENABLE_PKTCHK | ENABLE_PKTGEN), 0, eng);
	else if (mode & ENABLE_PKTGEN)
		SetTestParams((TEST_STOP | ENABLE_PKTGEN), 0, eng);
	else if (mode & ENABLE_PKTCHK)
		SetTestParams((TEST_STOP | ENABLE_PKTCHK), 0, eng);
	else if (mode & ENABLE_LOOPBACK)
		SetTestParams((TEST_STOP | ENABLE_LOOPBACK), 0, eng);
	//Sleep(2000);
	if ((mode & ENABLE_PKTGEN) || bRawEthernetMode)
	{
		drainFIFObuffer = (PUCHAR)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
		if (drainFIFObuffer != NULL)
		{
			DrainHardwareFifo(eng, drainFIFObuffer, size);
			//Sleep(2000);
#if 0
			XlxCancelTransfers(eng);
#endif
			//Sleep(2000);
			VirtualFree(drainFIFObuffer, 0, MEM_RELEASE);
		}
		else
		{
			//printf("Memory Allocation failed for drain buffer\n");
		}
	}
	XlxCancelTransfers(eng);
	return true;
}

DWORD SetTestParams(DWORD TestMode, DWORD	PacketSize, DWORD Engine)
{
	TestCmd			TC;
	OVERLAPPED		os;			// OVERLAPPED structure for the operation
	DWORD			IOCode = ISTOP_TEST;
	DWORD			bytes;
	DWORD			LastErrorStatus = 0;
	DWORD			Status = 0;

	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	os.Offset = 0;
	os.OffsetHigh = 0;

	TC.Engine = Engine;
	TC.MaxPktSize = PacketSize;
	TC.MinPktSize = PacketSize;
	TC.TestMode = TestMode;

	//printf("Set Params: Engine: %d MaxPktSize: %d MinPktSize: %d TestMode: 0x%x\n",TC.Engine,TC.MaxPktSize,TC.MinPktSize,TC.TestMode);
	if (TestMode & TEST_START)
	{
		IOCode = ISTART_TEST;
	}

	if (!DeviceIoControl(hDMADevice, IOCode, &TC, sizeof(TestCmd), NULL, 0, &bytes, &os))
	{
		LastErrorStatus = GetLastError();
		if (LastErrorStatus == ERROR_IO_PENDING)
		{
			// Wait here (forever) for the Overlapped I/O to complete
			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
			{
				// Overlapped failed, fnd out why and exit
				LastErrorStatus = GetLastError();
				//printf("SetTestParams failed, Error code 0x%x.\n", LastErrorStatus);
				Status = LastErrorStatus;
			}
		}
		else
		{
			//printf("SetTestParams failed, Error code 0x%x.\n", LastErrorStatus);
			Status = LastErrorStatus;
		}
	}
	else
	{
		LastErrorStatus = GetLastError();
		//printf("SetTestParams ioctl failed, Error code 0x%x.\n", LastErrorStatus);
		Status = LastErrorStatus;
	}
	CloseHandle(os.hEvent);
	return Status;
}

DWORD DrainHardwareFifo(
	INT Engine,
	PUCHAR buffer,
	INT size
	)
{
	OVERLAPPED		os;			// OVERLAPPED structure for the operation
	DWORD			LastErrorStatus = 0;
	DWORD           bytes = 0;
	int				retry = 0;
	HANDLE          hBlockDevice = NULL;
	DWORD           Status = 0;

	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	os.Offset = 0;
	os.OffsetHigh = 0;

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	if (Engine == 0)
	{
		if (bRawEthernetMode)
			hBlockDevice = hBlockDevice1;
		else
			hBlockDevice = hBlockDevice0;
	}
	else if (Engine == 1)
	{
		if (bRawEthernetMode)
			hBlockDevice = hBlockDevice0;
		else
			hBlockDevice = hBlockDevice1;
	}
	else
	{
		//printf("Invalid Engine %d\n",Engine);
	}

	while (retry < 10)
	{
		while (1)
		{
			if (!ReadFile(hBlockDevice, buffer, size, &bytes, &os))
			{
				LastErrorStatus = GetLastError();
				if (LastErrorStatus == ERROR_IO_PENDING)
				{
					// Wait up to 1 second for the transfer to complete
					LastErrorStatus = WaitForSingleObjectEx(os.hEvent, 1000, TRUE);
					if (LastErrorStatus != WAIT_OBJECT_0)
					{
						// Overlapped failed, fnd out why and exit
						//printf("ReadFile failed (Overlapped) on DrainFifoThread for engine %d, Error code 0x%x.\n", Engine, LastErrorStatus);
						Status = LastErrorStatus;
						break;
					}
					else
					{
						GetOverlappedResult(hBlockDevice, &os, &bytes, FALSE);
						//printf("DrainFifoThread drained %d bytes from FIFOs\n",bytes);
					}
				}
				else
				{
					//printf("ReadFile failed in DrainFifoThread, Error code 0x%x.\n", LastErrorStatus);
					Status = LastErrorStatus;
					break;
				}
			}
			else
			{
				LastErrorStatus = GetLastError();
				//printf("ReadFile on DrainFifoThread for engine %d failed, Error code 0x%x.\n", Engine,LastErrorStatus);
				Status = LastErrorStatus;
				break;
			}
			ResetEvent(os.hEvent);
#if 0
			Sleep(1000);
#endif
		}
		retry++;
	}
	//	ResetEvent(os.hEvent);
#if 0
	Sleep(500);
#endif
	CloseHandle(os.hEvent);
	return Status;
}


BOOL XlxCancelTransfers(
	INT Engine
	)
{
	OVERLAPPED		os;			// OVERLAPPED structure for the operation
	DWORD			IOCode = ISET_CANCEL_REQUESTS;
	DWORD			bytes;
	DWORD			LastErrorStatus = 0;
	DWORD			Status = 0;

	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	os.Offset = 0;
	os.OffsetHigh = 0;


	if (!DeviceIoControl(hDMADevice, IOCode, &Engine, sizeof(Engine), NULL, 0, NULL, &os))
	{
		LastErrorStatus = GetLastError();
		if (LastErrorStatus == ERROR_IO_PENDING)
		{
			// Wait here (forever) for the Overlapped I/O to complete
			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
			{
				// Overlapped failed, fnd out why and exit
				LastErrorStatus = GetLastError();
				//printf("XlxCancelTransfers, Error code 0x%x.\n", LastErrorStatus);
				Status = LastErrorStatus;
			}
		}
		else
		{
			//printf("XlxCancelTransfers, Error code 0x%x.\n", LastErrorStatus);
			Status = LastErrorStatus;
		}
	}
	else
	{
		LastErrorStatus = GetLastError();
		//printf("XlxCancelTransfers, Error code 0x%x.\n", LastErrorStatus);
		Status = LastErrorStatus;
	}
	CloseHandle(os.hEvent);
	return Status;
}

/*+F*************************************************************************
* Function:
* OpenDriverInterface
*
* Description:
*	Returns the Handle to the requested Driver Instance number
*-F*************************************************************************/
HANDLE
OpenDriverInterface(
INT	DriverIndex,
INT	DriverNumber
)
{
	HDEVINFO		hDevInfo;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
	HANDLE			hDevice = INVALID_HANDLE_VALUE;
	INT				count = 0;
	DWORD			RequiredSize = 0;
	BOOL			BoolStatus = TRUE;

	// Lookup the GUID and see if it is present, store the info in hDevInfo
	hDevInfo = SetupDiGetClassDevs(&GuidTable[DriverIndex], NULL, NULL,
		DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		//printf("SetupDiGetClassDevs returned INVALID_HANDLE_VALUE, error = 0x%x\n", GetLastError());
	}

	// Count the number of devices present
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GuidTable[DriverIndex], count++,
		&DeviceInterfaceData) == TRUE);

	// last one failed, find out why
	if (GetLastError() != ERROR_NO_MORE_ITEMS)
	{
		//printf("SetupDiEnumDeviceInterfaces returned FALSE, index= %d, error = %d\n\tShould be ERROR_NO_MORE_ITEMS (%d)\n", count-1, GetLastError(), ERROR_NO_MORE_ITEMS);
	}
	// Always counts one too many
	count--;

	// Check to see if there are any boards present
	if (count == 0)
	{
		//printf ("Error: driver is not present.\n");
		return INVALID_HANDLE_VALUE;
	}
	// check to see if the DriverNumber if valid
	else if (count <= DriverNumber)
	{
		//printf ("Error: Invalid Driver Number.\n");
		return INVALID_HANDLE_VALUE;
	}

	// Get information for device
	BoolStatus = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GuidTable[DriverIndex],
		DriverNumber, &DeviceInterfaceData);
	if (BoolStatus == false)
	{
		//printf("SetupDiEnumDeviceInterfaces failed for board, DriverNumber = %d\n  Error: %d\n", DriverNumber, GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	// Get the Device Interface Detailed Data
	BoolStatus = SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, NULL, 0, &RequiredSize, NULL);
	// this should fail (returning false) and setting error to ERROR_INSUFFICIENT_BUFFER
	if ((BoolStatus == TRUE) || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
	{
		//printf("SetupDiGetDeviceInterfaceDetail failed for board, DriverNumber = %d\n  Error: %d\n", DriverNumber, GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	// allocate the correct size
	pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize);
	if (pDeviceInterfaceDetailData == NULL)
	{
		//printf("Insufficient memory, pDeviceInterfaceDetailData\n");
		return INVALID_HANDLE_VALUE;
	}
	// set the size to the fixed data size (not the full size)
	pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	// get the data
	BoolStatus = SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, pDeviceInterfaceDetailData,
		RequiredSize, NULL, NULL);  // Do not need DeviceInfoData at this time
	if (BoolStatus == false)
	{
		//printf("SetupDiGetDeviceInterfaceDetail failed for board, DriverNumber = %d\n  Error: %d\n", DriverNumber, GetLastError());
		free(pDeviceInterfaceDetailData);
		return INVALID_HANDLE_VALUE;
	}

	// Now connect to the card
	hDevice = CreateFile(pDeviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
		INVALID_HANDLE_VALUE);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		//printf("CreateFile failed for board, DriverNumber = %d\n  Error: %d\n", DriverNumber, GetLastError());
		free(pDeviceInterfaceDetailData);
		return INVALID_HANDLE_VALUE;
	}
	// Free up device detail
	if (pDeviceInterfaceDetailData != NULL)
	{
		free(pDeviceInterfaceDetailData);
	}
	return hDevice;
}


/*+F*************************************************************************
* Function:
* waitThreadsStop
*
* Description:
*	Waits for test threads to stop up to approx 2 seconds.
*-F*************************************************************************/
void
waitThreadsStop(
int engine
)
{
	DWORD	error;
	int		i = 0;
	int     j = 0;


	if (engine == 0)
	{
		//g_TimeUp0 = 0;
		while (g_TimeUp0) {
			Sleep(10);
		}

		if (bRawEthernetMode)
			g_TimeUp0_1 = 0;

		// wait till either all quit or time is due
		error = WaitForMultipleObjects(NumThreads0, Threads0, TRUE, 10000);
		if (error == WAIT_TIMEOUT)
		{
			// Stop the threads if time is up
			error = WaitForMultipleObjects(i, Threads0, TRUE, 1000);
			if (error)
			{
				//printf("WaitForMultipleObjects(2)[%d] error %d\n", i, error);
			}
		}
		else
		{
			if (error)
			{
				//printf("WaitForMultipleObjects(1)[%d] error %d\n", i, error);
			}
		}
		//		Sleep(2000);
		error = WaitForMultipleObjects(NumThreads0, WaitThreads0, TRUE, 10000);
		if (error == WAIT_TIMEOUT)
		{
			// Stop the threads if time is up
			error = WaitForMultipleObjects(i, WaitThreads0, TRUE, 1000);
			if (error)
			{
				//printf("WaitForMultipleObjects wait threads (2)[%d] error %d\n", i, error);
			}
		}
		else
		{
			if (error)
			{
				//printf("WaitForMultipleObjects wait threads (1)[%d] error %d\n", i, error);
			}
		}
		//Sleep(2000);
		if (Contexts0)
		{
			for (i = 0; i < ThreadCount0; i++)
			{
				if (Contexts0[i].ContextUsed == TRUE)
				{
					if (Contexts0[i].Buffer)
					{

					}
					for (j = 0; j<MAX_EVENTS_PUSHED; j++)
					{
						if (Contexts0[i].os[j].hEvent != NULL)
						{
							CloseHandle(Contexts0[i].os[j].hEvent);
						}
					}
					DeleteCriticalSection(&(Contexts0[i].cs));
				}
			}

			delete Contexts0;
			Contexts0 = NULL;
			ContextCount0 = 0;
		}

		if (Threads0)
		{
			delete Threads0;
			Threads0 = NULL;
		}
		if (WaitThreads0)
		{
			delete WaitThreads0;
			WaitThreads0 = NULL;
		}
	}
	else if (engine == 1)
	{
		g_TimeUp1 = 0;

		if (bRawEthernetMode)
			g_TimeUp1_0 = 0;

		// wait till either all quit or time is due
		error = WaitForMultipleObjects(NumThreads1, Threads1, TRUE, 10000);
		if (error == WAIT_TIMEOUT)
		{
			// Stop the threads if time is up
			error = WaitForMultipleObjects(i, Threads1, TRUE, 1000);
			if (error)
			{
				//printf("WaitForMultipleObjects(2)[%d] error %d\n", i, error);
			}
		}
		else
		{
			if (error)
			{
				//printf("WaitForMultipleObjects(1)[%d] error %d\n", i, error);
			}
		}
		//		Sleep(2000);
		error = WaitForMultipleObjects(NumThreads1, WaitThreads1, TRUE, 10000);
		if (error == WAIT_TIMEOUT)
		{
			// Stop the threads if time is up
			error = WaitForMultipleObjects(i, WaitThreads1, TRUE, 1000);
			if (error)
			{
				//printf("WaitForMultipleObjects(2)[%d] error %d\n", i, error);
			}
		}
		else
		{
			if (error)
			{
				//printf("WaitForMultipleObjects(1)[%d] error %d\n", i, error);
			}
		}
		if (Contexts1)
		{
			for (i = 0; i < ThreadCount1; i++)
			{
				if (Contexts1[i].ContextUsed == TRUE)
				{
					if (Contexts1[i].Buffer)
					{

					}
					for (j = 0; j<MAX_EVENTS_PUSHED; j++)
					{
						if (Contexts1[i].os[j].hEvent != NULL)
						{
							CloseHandle(Contexts1[i].os[j].hEvent);
						}
					}
					DeleteCriticalSection(&(Contexts1[i].cs));
				}
			}
			delete Contexts1;
			Contexts1 = NULL;
			ContextCount1 = 0;
		}
		if (Threads1)
		{
			delete Threads1;
			Threads1 = NULL;
		}
		if (WaitThreads1)
		{
			delete WaitThreads1;
			WaitThreads1 = NULL;
		}
	}
	else
	{
		//printf("wrong Engine \n");
	}
}

DWORD WINAPI WriteWaitThreadProc(LPVOID lpParameter)
{
	PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;
	DWORD			LastErrorStatus = 0;
	DWORD           Status;
	int             retry_count = 0;
	HANDLE         *hTempEvents;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	hTempEvents = Context->hEvent;

	while (1)
	{
		if ((MAX_EVENTS_PUSHED - Context->osIndexFree) >= STACKED_EVENTS_TO_CLEAR)
		{
			/*
			for(i=0;i<STACKED_EVENTS_TO_CLEAR;i++)
			printf(" Event handle values being checked in WaitforMultipleObjects are %p\n",*(hTempEvents + i));
			*/
#if 1
			Status = WaitForMultipleObjectsEx(STACKED_EVENTS_TO_CLEAR, hTempEvents, TRUE, INFINITE, TRUE);
			LastErrorStatus = GetLastError();
#endif
#if 1

			switch (Status)
			{
			case WAIT_IO_COMPLETION:
				//printf("WaitforMultipleObjectsEx failed with WAIT_IO_COMPLETION error in writewaitthread. LastError Status = %x\n", LastErrorStatus);
				break;
			case WAIT_TIMEOUT:
				//printf("WaitforMultipleObjectsEx failed with WAIT_TIMEOUT error in writewaitthread. LastError Status = %x\n", LastErrorStatus);
				break;
			case WAIT_FAILED:
				//printf("WaitforMultipleObjectsEx failed with WAIT_FAILED error in writewaitthread. LastError Status = %x\n", LastErrorStatus);
				break;
				/* default:
				printf("WaitforMultipleObjectsEx returned number of objects signaled or abandoned.\
				Status = %d LastErrorStatus = %x\n",Status,LastErrorStatus); */

			}
			if ((Status != WAIT_TIMEOUT) && (Status != WAIT_FAILED))
			{
				Context->osIndexTail += STACKED_EVENTS_TO_CLEAR;
				if (Context->osIndexTail >= MAX_EVENTS_PUSHED)
				{
					Context->osIndexTail = 0;
					hTempEvents = Context->hEvent;
				}
				hTempEvents += Context->osIndexTail;
				/*
				for(i=0;i<STACKED_EVENTS_TO_CLEAR;i++)
				printf(" Event handle values that will be checked next time in WaitforMultipleObjects are %p\n",*(hTempEvents + i));
				*/
				EnterCriticalSection(&(Context->cs));
				Context->osIndexFree += STACKED_EVENTS_TO_CLEAR;
				Context->BufferAvailable += ((Context->ChunkSize) * STACKED_EVENTS_TO_CLEAR);
				if (Context->osIndexFree > MAX_EVENTS_PUSHED)
				{
					Context->osIndexFree = MAX_EVENTS_PUSHED;
					//printf("ERROR osIndexFree crossed boundary, writewaitthreadproc recheck logic for engine %d\n", Context->engine);
				}
				if (Context->BufferAvailable > Context->MaxBufferAvailable)
				{
					Context->BufferAvailable = Context->MaxBufferAvailable;
					//printf("ERROR BufferAvailable crossed boundary, writewaitthreadproc recheck logic for engine %d\n", Context->engine);
				}
				LeaveCriticalSection(&(Context->cs));
			}
			else
			{
				//printf("WaitForMultipleObjectsEx failed in WriteWaitThreadProc with error code %x\n", LastErrorStatus);
				break;
			}
#endif
		}
		else
		{
			//		printf("Not pushing at the speed at which the events are cleared for write thread number %d.\n",Context->ThreadNumber);
			//		Sleep(10);
		}
		if (Context->ThreadDone == 0)
		{
			if ((Context->BufferAvailable < Context->MaxBufferAvailable) && (retry_count < 5))
			{
				retry_count++;
				continue;
			}
			break;
		}

	}
	//printf("In writeWaitThread BufferAvailable is %d and MaxBufferAvailable is %d\n", Context->BufferAvailable,Context->MaxBufferAvailable);
	Context->WaitThreadDone = 0;
	if (Context->debug)
	{
		//printf("WaitWriteThread %d shutting down.\n", Context->ThreadNumber);
	}
	ExitThread(0);
}

DWORD WINAPI WriteThreadProc(LPVOID lpParameter)
{
	DWORD			bytes = 0;
	PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;
	DWORD			LastErrorStatus = 0;
	int MaxBufferSize = 0;
	int PacketSize = Context->BufferSize;
	int ChunkSize = 0;
	int BufferUtilized = 0;
	int PacketSent = 0;
	int EvtCount = 0;

	//printf("Started WriteThreadProc\r\n");

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (PacketSize % 4)
	{
		ChunkSize = PacketSize + (4 - (PacketSize % 4));
	}
	else
	{
		ChunkSize = PacketSize;
	}
	//MaxBufferSize = DEFAULT_BUFFER_SIZE * MIN_PACKETS_IN_BUFFER;
	MaxBufferSize = myWriteBufferSize;
	//BufferUtilized = MaxBufferSize - (MaxBufferSize % (ChunkSize * TX_CONFIG_SEQNO));
	BufferUtilized = MaxBufferSize;

	//FormatBuffer(Context->Buffer, BufferUtilized, ChunkSize, PacketSize, bRawEthernetMode);

	EnterCriticalSection(&(Context->cs));
	Context->BufferAvailable = BufferUtilized;
	Context->MaxBufferAvailable = BufferUtilized;
	Context->ChunkSize = ChunkSize;
	LeaveCriticalSection(&(Context->cs));
	while (1)
	{
		if ((Context->osIndexFree > 0) && (Context->BufferAvailable > 0))
		{
			if (PacketSent + ChunkSize <= BufferUtilized)
			{
				//printf("Writefile invoked with event handle %p\n",Context->os[Context->osIndexHead].hEvent);
				if (!WriteFile(Context->hBlockDevice, Context->Buffer + PacketSent, PacketSize, &bytes, &(Context->os[Context->osIndexHead])))
				{
					(Context->osIndexHead)++;
					if (Context->osIndexHead == MAX_EVENTS_PUSHED)
						Context->osIndexHead = 0;
					LastErrorStatus = GetLastError();
					if (LastErrorStatus == ERROR_IO_PENDING)
					{
#if 0                   //Moving wait to another thread.
						// Wait up to one second for the transfer to complete
						LastErrorStatus = WaitForSingleObjectEx(os.hEvent, 1000, TRUE);

						if (LastErrorStatus != WAIT_OBJECT_0)
						{
							// Overlapped failed, fnd out why and exit
							//LastErrorStatus = GetLastError();
							printf("Write failed (Overlapped), Error code 0x%x.\n", LastErrorStatus);
							if (Context->ExitOnError)
							{
								break;
							}
						}
						else
						{
							GetOverlappedResult(Context->hBlockDevice, &os, &bytes, FALSE);
						}
#endif
					}
					else
					{
						//printf("WriteFile failed, Error code 0x%x.\n", LastErrorStatus);
						if (Context->ExitOnError)
						{
							//printf("WriteFile failed, ExitonError specified. Error code 0x%x.\n", LastErrorStatus);
							break;
						}
					}

					PacketSent = PacketSent + ChunkSize;
					EnterCriticalSection(&(Context->cs));
					(Context->osIndexFree)--;
					(Context->BufferAvailable) -= ChunkSize;
					LeaveCriticalSection(&(Context->cs));
					EvtCount++;
					if (EvtCount == STACKED_EVENTS_TO_CLEAR)
						EvtCount = 0;
				}
				else
				{
					//printf("WriteFile on Thread %d failed, Error code 0x%x.\n", Context->ThreadNumber, GetLastError());
					break;
				}
			}
			else
			{
				//PacketSent = 0;
				if (g_TimeUp0 != 0)	{
					//printf("PacketSent is: %d\r\n", PacketSent);
				}
				g_TimeUp0 = 0;
			}
		}
		else
		{
			//printf("Not clearing at the speed at which the events are pushed for write thread number %d.\n",Context->ThreadNumber);
			//	Sleep(100);

		}
		if (Context->hBlockDevice == hBlockDevice0)
		{
			//			if (g_TimeUp0 == 0 && EvtCount == 0)
			if (g_TimeUp0 == 0)
			{
				Context->ThreadDone = 0;
				break;
			}
		}
		else if (Context->hBlockDevice == hBlockDevice1)
		{
			if (g_TimeUp1 == 0 && EvtCount == 0)
			{
				Context->ThreadDone = 0;
				break;
			}
		}
#if 0
		Sleep(1000);
#endif
	}
	Context->ThreadDone = 0;
	if (Context->debug)
	{
		//printf("WriteThread %d shutting down for engine %d.\n", Context->ThreadNumber, Context->engine);
	}

	//printf("Finished WriteThreadProc\r\n");

	ExitThread(0);
}


BOOL ThreadedWriteTest(int Engine, int PacketSize, int mode)
{
	BOOL		status = TRUE;
	DWORD_PTR	pAffinity0;
	DWORD_PTR	pAffinity1;
	ULONG		TestTimeSeconds = TestTime;
	float		GbpsTransferred = 0;
	int			i;
	int         j;
	int         k;
	DWORD       last_error_status;

	HANDLE hThread;
	HANDLE hWaitThread;
	HANDLE hBlockDevice;

	if (Engine == 0)
	{
		hBlockDevice = hBlockDevice0;
		g_TimeUp0 = 1;

		Contexts0 = new THREAD_CONTEXT[ThreadCount0];
		ContextCount0 = ThreadCount0;
		Threads0 = new HANDLE[ThreadCount0];
		WaitThreads0 = new HANDLE[ThreadCount0];
		if (Contexts0 == NULL || Threads0 == NULL || WaitThreads0 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedWriteTest with error code %x\n", last_error_status);
			return FALSE;
		}

		/*printf("\nStarting %d Write threads running for %d seconds, please wait...\n",
		ThreadCount0, TestTimeSeconds);*/

		pAffinity0 = 1 << NUM_CORES_TO_LEAVE;
		for (i = 0, j = 0; i < ThreadCount0; i++)
		{
			if ((i % 2) == 0)//Making sure number of threads are same in all modes.
			{
				//  Create Write Thread
				Contexts0[i].hBlockDevice = hBlockDevice;
				Contexts0[i].engine = Engine;
				Contexts0[i].mode = mode;
				WriteBufferSize = PacketSize;
				Contexts0[i].BufferSize = WriteBufferSize;
				Contexts0[i].Buffer = ThreadBuffer0[i];
				Contexts0[i].quiet = Quiet;
				Contexts0[i].debug = Debug;
				//Contexts0[i].ExitOnError = ExitOnError;
				Contexts0[i].ExitOnError = FALSE;
				Contexts0[i].ThreadDone = 1;
				Contexts0[i].WaitThreadDone = 1;
				Contexts0[i].BytesXfered = 0;
				Contexts0[i].ThreadNumber = i;
				Contexts0[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts0[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts0[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts0[i].hEvent[k] = Contexts0[i].os[k].hEvent;
						//	printf("Event handle created at ThreadedWriteTest are %p , %p\n",Contexts0[i].hEvent[k],Contexts0[i].os[k].hEvent);
					}
					Contexts0[i].os[k].Offset = 0;
					Contexts0[i].os[k].OffsetHigh = 0;
				}
				Contexts0[i].osIndexTail = 0;
				Contexts0[i].osIndexHead = 0;
				Contexts0[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts0[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedWriteTest with error code %x\n", last_error_status);
				}
				hThread = CreateThread(NULL, 0, WriteThreadProc, &Contexts0[i], 0, NULL);

				if (NULL == hThread)
				{
					last_error_status = GetLastError();
					//printf("Failed to create thread %d\n", i);
					//Status = 1;
					Contexts0[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads0[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, WriteWaitThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts0[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads0[j] = hWaitThread;
				}
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
#if 1
				SetThreadAffinityMask(Threads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
				SetThreadAffinityMask(WaitThreads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
#endif
				j++;
			}
			else
			{
				Contexts0[i].ThreadDone = 0;
				Contexts0[i].WaitThreadDone = 0;
				continue;
			}
		}

		NumThreads0 = j;
		if (i != ThreadCount0)
		{
			// some create thread failed, bail out
			//printf("Some CreateThread was failed, stop\n");
			g_TimeUp0 = 0;
			Contexts0[i].ThreadDone = 0;
		}
	}
	else if (Engine == 1)
	{
		hBlockDevice = hBlockDevice1;
		g_TimeUp1 = 1;

		Contexts1 = new THREAD_CONTEXT[ThreadCount1];
		ContextCount1 = ThreadCount1;
		Threads1 = new HANDLE[ThreadCount1];
		WaitThreads1 = new HANDLE[ThreadCount1];

		if (Contexts1 == NULL || Threads1 == NULL || WaitThreads1 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedWriteTest with error code %x\n", last_error_status);
			return FALSE;
		}

		/*printf("\nStarting %d Write threads running for %d seconds, please wait...\n",
		ThreadCount1, TestTimeSeconds);*/

		pAffinity1 = 1 << ((2 * DEFAULT_THREAD_COUNT) + NUM_CORES_TO_LEAVE);
		for (i = 0, j = 0; i < ThreadCount1; i++)//Making sure number of threads are same in all cases.
		{
			if ((i % 2) == 0)//Making sure number of threads are same in all modes.
			{
				//  Create Write Thread
				Contexts1[i].hBlockDevice = hBlockDevice;
				Contexts1[i].engine = Engine;
				Contexts1[i].mode = mode;
				WriteBufferSize = PacketSize;
				Contexts1[i].BufferSize = WriteBufferSize;
				Contexts1[i].Buffer = ThreadBuffer1[i];
				Contexts1[i].quiet = Quiet;
				Contexts1[i].debug = Debug;
				Contexts1[i].ExitOnError = ExitOnError;
				Contexts1[i].ThreadDone = 1;
				Contexts1[i].WaitThreadDone = 1;
				Contexts1[i].BytesXfered = 0;
				Contexts1[i].ThreadNumber = i;
				Contexts1[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts1[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts1[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts1[i].hEvent[k] = Contexts1[i].os[k].hEvent;
					}
					Contexts1[i].os[k].Offset = 0;
					Contexts1[i].os[k].OffsetHigh = 0;
				}
				Contexts1[i].osIndexTail = 0;
				Contexts1[i].osIndexHead = 0;
				Contexts1[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts1[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedWriteTest with error code %x\n", last_error_status);
				}

				hThread = CreateThread(NULL, 0, WriteThreadProc, &Contexts1[i], 0, NULL);

				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//Status = 1;
					Contexts1[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads1[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, WriteWaitThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts1[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads1[j] = hWaitThread;
				}
#if 1
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
				SetThreadAffinityMask(WaitThreads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
#endif
				j++;
			}
			else
			{
				Contexts1[i].ThreadDone = 0;
				Contexts1[i].WaitThreadDone = 0;
				continue;
			}
		}
		NumThreads1 = j;
		if (i != ThreadCount1)
		{
			// some create thread failed, bail out
			//printf("Some CreateThread was failed, stop\n");
			g_TimeUp1 = 0;
			Contexts1[i].ThreadDone = 0;
		}
	}
	else
	{
		//printf("Wrong Engine %d\n ", Engine);
	}
	return status;
}

DWORD WINAPI ReadWaitThreadProc(LPVOID lpParameter)
{
	PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;
	DWORD			LastErrorStatus = 0;
	DWORD           Status;
	int             retry_count = 0;
	HANDLE         *hTempEvents;
	int				retry = 0;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	hTempEvents = Context->hEvent;

	while (1)
	{
		if ((MAX_EVENTS_PUSHED - Context->osIndexFree) >= STACKED_EVENTS_TO_CLEAR)
		{
			/*
			for(i=0;i<STACKED_EVENTS_TO_CLEAR;i++)
			printf(" Event handle values being checked in WaitforMultipleObjects are %p\n",*(hTempEvents + i));
			*/
#if 1
			Status = WaitForMultipleObjectsEx(STACKED_EVENTS_TO_CLEAR, hTempEvents, TRUE, INFINITE, TRUE);
			LastErrorStatus = GetLastError();
#endif
#if 0
			for (i = 0; i<STACKED_EVENTS_TO_CLEAR; i++)
			{
				Status = WaitForSingleObjectEx(Context->os[i].hEvent, 2000, TRUE);
				LastErrorStatus = GetLastError();
				if (Status != WAIT_OBJECT_0)
				{
					// Overlapped failed, fnd out why and exit
					//LastErrorStatus = GetLastError();
					printf("Write failed (Overlapped), Error code 0x%x.\n", LastErrorStatus);
					if (Context->ExitOnError)
					{
						printf("ExitonError selected. Write failed (Overlapped), Error code 0x%x.\n", LastErrorStatus);
						break;
					}
				}
				else
				{
					// GetOverlappedResult(Context->hBlockDevice, &os, &bytes, FALSE);
				}
			}
			Context->osIndexTail += STACKED_EVENTS_TO_CLEAR;
			if (Context->osIndexTail >= MAX_EVENTS_PUSHED)
				Context->osIndexTail = 0;
			hTempEvents += Context->osIndexTail;
			EnterCriticalSection(&(Context->cs));
			Context->osIndexFree += STACKED_EVENTS_TO_CLEAR;
			if (Context->osIndexFree > MAX_EVENTS_PUSHED)
				Context->osIndexFree = MAX_EVENTS_PUSHED;
			LeaveCriticalSection(&(Context->cs));
#endif
#if 1

			switch (Status)
			{
			case WAIT_IO_COMPLETION:
				//printf("WaitforMultipleObjectsEx failed with WAIT_IO_COMPLETION error in ReadWaitThread. LastError Status = %x\n", LastErrorStatus);
				break;
			case WAIT_TIMEOUT:
				//printf("WaitforMultipleObjectsEx failed with WAIT_TIMEOUT error in ReadWaitThread. LastError Status = %x\n", LastErrorStatus);
				break;
			case WAIT_FAILED:
				//printf("WaitforMultipleObjectsEx failed with WAIT_FAILED error in ReadWaitThread. LastError Status = %x\n", LastErrorStatus);
				break;
				/* default:
				printf("WaitforMultipleObjectsEx returned number of objects signaled or abandoned.\
				Status = %d LastErrorStatus = %x\n",Status,LastErrorStatus); */

			}
			if ((Status != WAIT_TIMEOUT) && (Status != WAIT_FAILED))
			{
#if 0
				for (i = 0; i<STACKED_EVENTS_TO_CLEAR; i++)
				{
					result = GetOverlappedResult(Context->hBlockDevice, &(Context->os[(Context->osIndexTail) + i]), &bytes, FALSE);
					LastErrorStatus = GetLastError();
					if (result == FALSE)
					{
						//	 printf("GetOverlappedResult failed with error %x\n",LastErrorStatus);
					}
					else
					{
						//	 printf("Number of bytes successfully transferred are %d\n",bytes);
						Context->BytesXfered += bytes;
					}
					//        ResetEvent(*(hTempEvents + i));
				}
#endif
				Context->osIndexTail += STACKED_EVENTS_TO_CLEAR;
				if (Context->osIndexTail >= MAX_EVENTS_PUSHED)
				{
					Context->osIndexTail = 0;
					hTempEvents = Context->hEvent;
				}
				hTempEvents += Context->osIndexTail;
				/*
				for(i=0;i<STACKED_EVENTS_TO_CLEAR;i++)
				printf(" Event handle values that will be checked next time in WaitforMultipleObjects are %p\n",*(hTempEvents + i));
				*/
				EnterCriticalSection(&(Context->cs));
				Context->osIndexFree += STACKED_EVENTS_TO_CLEAR;
				Context->BufferAvailable += ((Context->ChunkSize) * STACKED_EVENTS_TO_CLEAR);
#if 0
				if (Context->osIndexFree > MAX_EVENTS_PUSHED)
				{
					Context->osIndexFree = MAX_EVENTS_PUSHED;
					printf("ERROR osIndexFree crossed boundary, readwaitthreadproc recheck logic for engine %d\n", Context->engine);
				}
				if (Context->BufferAvailable > Context->MaxBufferAvailable)
				{
					Context->BufferAvailable = Context->MaxBufferAvailable;
					printf("ERROR BufferAvailable crossed boundary, readwaitthreadproc recheck logic for engine %d\n", Context->engine);
				}
#endif
				LeaveCriticalSection(&(Context->cs));
			}
			else
			{
				//printf("WaitForMultipleObjectsEx failed in ReadWaitThread with error code %x\n", LastErrorStatus);
				break;
			}
#endif
		}
		else
		{
			//		printf("Not pushing at the speed at which the events are cleared for write thread number %d.\n",Context->ThreadNumber);
			//		Sleep(10);
		}
#if 0
		if (Context->hBlockDevice == hBlockDevice0)
		{
			if (bRawEthernetMode)
			{
				if (g_TimeUp1_0 == 0)
				{
					if (retry > 10)
					{
						Context->WaitThreadDone = 0;
						break;
					}
					retry++;
				}
			}
			else
			{
				if (g_TimeUp0 == 0)
				{
					if (retry > 10)
					{
						Context->WaitThreadDone = 0;
						break;
					}
					retry++;
				}
			}
		}
		else if (Context->hBlockDevice == hBlockDevice1)
		{
			if (bRawEthernetMode)
			{
				if (g_TimeUp0_1 == 0)
				{
					if (retry > 10)
					{
						Context->WaitThreadDone = 0;
						break;
					}
					retry++;
				}
			}
			else
			{
				if (g_TimeUp1 == 0)
				{
					if (retry > 10)
					{
						Context->WaitThreadDone = 0;
						break;
					}
					retry++;
				}
			}
		}
#endif
		if (Context->ThreadDone == 0)
		{
			if ((Context->BufferAvailable < Context->MaxBufferAvailable) && (retry_count < 5))
			{
				retry_count++;
				continue;
			}
			break;
		}
	}
	Context->WaitThreadDone = 0;
	//printf("In ReadWaitThread BufferAvailable = %d and MaxBufferAvailable = %d\n", Context->BufferAvailable,Context->MaxBufferAvailable);
	if (Context->debug)
	{
		//printf("ReadWaitThread %d shutting down.\n", Context->ThreadNumber);
	}
	ExitThread(0);
}

DWORD WINAPI ReadThreadProc(LPVOID lpParameter)
{
	DWORD			bytes = 0;
	PTHREAD_CONTEXT Context = (PTHREAD_CONTEXT)lpParameter;
	DWORD			LastErrorStatus = 0;
	int				retry = 0;
	int MaxBufferSize = 0;
	int PacketSize = Context->BufferSize;
	int ChunkSize = 0;
	int BufferUtilized = 0;
	int PacketSent = 0;
	int EvtCount = 0;

	//printf("Started ReadThreadProc\r\n");

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (PacketSize % 4)
	{
		ChunkSize = PacketSize + (4 - (PacketSize % 4));
	}
	else
	{
		ChunkSize = PacketSize;
	}
	//MaxBufferSize = DEFAULT_BUFFER_SIZE * MIN_PACKETS_IN_BUFFER;
	MaxBufferSize = myReadBufferSize;
	//BufferUtilized = MaxBufferSize - (MaxBufferSize % (ChunkSize * TX_CONFIG_SEQNO));
	BufferUtilized = MaxBufferSize;

	EnterCriticalSection(&(Context->cs));
	Context->BufferAvailable = BufferUtilized;
	Context->MaxBufferAvailable = BufferUtilized;
	Context->ChunkSize = ChunkSize;
	LeaveCriticalSection(&(Context->cs));

	while (1)
	{
		if ((Context->osIndexFree > 0) && (Context->BufferAvailable > 0))
		{
			if (PacketSent + ChunkSize <= BufferUtilized)
			{
				if (!ReadFile(Context->hBlockDevice, Context->Buffer + PacketSent, PacketSize, &bytes, &(Context->os[Context->osIndexHead])))
				{
					(Context->osIndexHead)++;
					if (Context->osIndexHead == MAX_EVENTS_PUSHED)
						Context->osIndexHead = 0;
					LastErrorStatus = GetLastError();
					if (LastErrorStatus == ERROR_IO_PENDING)
					{
#if 0
						// Wait up to one second for the transfer to complete
						LastErrorStatus = WaitForSingleObjectEx(os.hEvent, 10000, TRUE);
						if (LastErrorStatus != WAIT_OBJECT_0)
						{
							// Overlapped failed, fnd out why and exit
							printf("ReadFile failed (Overlapped), Error code 0x%x.\n", LastErrorStatus);
							if (Context->ExitOnError)
							{
								break;
							}
						}
						else
						{
							GetOverlappedResult(Context->hBlockDevice, &os, &bytes, FALSE);
						}
#endif
					}
					else
					{
						//printf("ReadFile failed, Error code 0x%x.\n", LastErrorStatus);
						break;
					}
					PacketSent = PacketSent + ChunkSize;
					EnterCriticalSection(&(Context->cs));
					(Context->osIndexFree)--;
					(Context->BufferAvailable) -= ChunkSize;
					LeaveCriticalSection(&(Context->cs));
					EvtCount++;
					if (EvtCount == STACKED_EVENTS_TO_CLEAR)
						EvtCount = 0;
				}
				else
				{
					//printf("ReadFile on Thread %d failed, Error code 0x%x.\n", Context->ThreadNumber, GetLastError());
					break;
				}
			}
			else
			{
				//PacketSent = 0;
				g_TimeUp1_0 = 0;
			}
		}
		else
		{
			//printf("Not clearing at the speed at which the events are pushed for read thread number %d.\n",Context->ThreadNumber);
			//	Sleep(100);
		}

		if (Context->hBlockDevice == hBlockDevice0)
		{
			if (bRawEthernetMode)
			{
				if (g_TimeUp1_0 == 0 && EvtCount == 0)
				{
#if 0
					if (retry > 10)
					{
						Context->ThreadDone = 0;
						break;
					}
					retry++;
#endif
					break;
				}
			}
			else
			{
				//				if(g_TimeUp0 == 0  && EvtCount == 0)
				if (g_TimeUp0 == 0)
				{
#if 0
					if (retry > 10)
					{
						Context->ThreadDone = 0;
						break;
					}
					retry++;
#endif
					break;
				}
			}
		}
		else if (Context->hBlockDevice == hBlockDevice1)
		{
			if (bRawEthernetMode)
			{
				if (g_TimeUp0_1 == 0 && EvtCount == 0)
				{
#if 0
					if (retry > 10)
					{
						Context->ThreadDone = 0;
						break;
					}
					retry++;
#endif
					break;
				}
			}
			else
			{
				if (g_TimeUp1 == 0 && EvtCount == 0)
				{
#if 0
					if (retry > 10)
					{
						Context->ThreadDone = 0;
						break;
					}
					retry++;
#endif
					break;
				}
			}
		}
#if 0
		Sleep(1000);
#endif
	}
	Context->ThreadDone = 0;
	if (Context->debug)
	{
		//printf("ReadThread %d shutting down.\n", Context->ThreadNumber);
	}

	//printf("Finished ReadThreadProc\r\n");

	ExitThread(0);
}


BOOL ThreadedReadTest(int Engine, int PacketSize, int mode)
{
	BOOL		status = TRUE;
	DWORD_PTR	pAffinity0;
	DWORD_PTR	pAffinity1;
	ULONG		TestTimeSeconds = TestTime;
	float		GbpsTransferred = 0;
	int			i;
	int         j;
	int         k;
	DWORD       last_error_status;


	HANDLE hThread;
	HANDLE hWaitThread;
	HANDLE hBlockDevice;

	if (Engine == 0)
	{
		hBlockDevice = hBlockDevice0;
		g_TimeUp0 = 1;

		Contexts0 = new THREAD_CONTEXT[ThreadCount0];
		ContextCount0 = ThreadCount0;
		Threads0 = new HANDLE[ThreadCount0];
		WaitThreads0 = new HANDLE[ThreadCount0];

		if (Contexts0 == NULL || Threads0 == NULL || WaitThreads0 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedReadTest with error code %x\n",last_error_status);
			return FALSE;
		}

		/*printf("\nStarting %d Read threads running for %d seconds, please wait...\n",
		ThreadCount0, TestTimeSeconds);*/

		pAffinity0 = 1 << NUM_CORES_TO_LEAVE;
		for (i = 0, j = 0; i < ThreadCount0; i++)
		{
			if ((i % 2) == 0)//Making sure number of threads are same in all modes.
			{
				Contexts0[i].ThreadDone = 0;
				Contexts0[i].WaitThreadDone = 0;
				continue;
			}
			else
			{
				//  Create Read Thread
				Contexts0[i].hBlockDevice = hBlockDevice;
				Contexts0[i].engine = Engine;
				Contexts0[i].mode = mode;
				ReadBufferSize = PacketSize;
				Contexts0[i].BufferSize = ReadBufferSize;
				Contexts0[i].Buffer = ThreadBuffer0[i];
				Contexts0[i].quiet = Quiet;
				Contexts0[i].debug = Debug;
				Contexts0[i].ExitOnError = ExitOnError;
				Contexts0[i].ThreadDone = 1;
				Contexts0[i].WaitThreadDone = 1;
				Contexts0[i].BytesXfered = 0;
				Contexts0[i].ThreadNumber = i;
				Contexts0[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts0[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts0[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedReadTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts0[i].hEvent[k] = Contexts0[i].os[k].hEvent;
						//	printf("Event handle created at ThreadedReadTest are %p , %p\n",Contexts0[i].hEvent[k],Contexts0[i].os[k].hEvent);
					}
					Contexts0[i].os[k].Offset = 0;
					Contexts0[i].os[k].OffsetHigh = 0;
				}
				Contexts0[i].osIndexTail = 0;
				Contexts0[i].osIndexHead = 0;
				Contexts0[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts0[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedReadTest with error code %x\n", last_error_status);
				}

				hThread = CreateThread(NULL, 0, ReadThreadProc, &Contexts0[i], 0, NULL);

				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//this->Status = 1;
					Contexts0[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads0[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, ReadWaitThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts0[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads0[j] = hWaitThread;
				}
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
				SetThreadAffinityMask(WaitThreads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
				j++;
			}
		}

		NumThreads0 = j;

		if (i != ThreadCount0)
		{
			// some create thread failed, bail out
			//printf("Some CreateThread was failed, stop\n");
			g_TimeUp0 = 0;
			Contexts0[i].ThreadDone = 0;
			Contexts0[i].WaitThreadDone = 0;
		}
	}
	else if (Engine == 1)
	{
		hBlockDevice = hBlockDevice1;
		g_TimeUp1 = 1;

		Contexts1 = new THREAD_CONTEXT[ThreadCount1];
		ContextCount1 = ThreadCount1;
		Threads1 = new HANDLE[ThreadCount1];
		WaitThreads1 = new HANDLE[ThreadCount1];

		if (Contexts1 == NULL || Threads1 == NULL || WaitThreads1 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedReadTest with error code %x\n", last_error_status);
			return FALSE;
		}

		/*printf("\nStarting %d Read threads running for %d seconds, please wait...\n",
		ThreadCount1, TestTimeSeconds);*/

		pAffinity1 = 1 << ((2 * DEFAULT_THREAD_COUNT) + NUM_CORES_TO_LEAVE);
		for (i = 0, j = 0; i < ThreadCount1; i++)
		{
			if ((i % 2) == 0)//Making sure number of threads are same in all modes.
			{
				Contexts1[i].ThreadDone = 0;
				Contexts1[i].WaitThreadDone = 0;
				continue;
			}
			else
			{
				//  Create Read Thread
				Contexts1[i].hBlockDevice = hBlockDevice;
				Contexts1[i].engine = Engine;
				Contexts1[i].mode = mode;
				ReadBufferSize = PacketSize;
				Contexts1[i].BufferSize = ReadBufferSize;
				Contexts1[i].Buffer = ThreadBuffer1[i];
				Contexts1[i].quiet = Quiet;
				Contexts1[i].debug = Debug;
				Contexts1[i].ExitOnError = ExitOnError;
				Contexts1[i].ThreadDone = 1;
				Contexts1[i].WaitThreadDone = 1;
				Contexts1[i].BytesXfered = 0;
				Contexts1[i].ThreadNumber = i;
				Contexts1[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts1[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts1[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts1[i].hEvent[k] = Contexts1[i].os[k].hEvent;
					}
					Contexts1[i].os[k].Offset = 0;
					Contexts1[i].os[k].OffsetHigh = 0;
				}
				Contexts1[i].osIndexTail = 0;
				Contexts1[i].osIndexHead = 0;
				Contexts1[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts1[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedWriteTest with error code %x\n", last_error_status);
				}

				hThread = CreateThread(NULL, 0, ReadThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//this->Status = 1;
					Contexts1[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads1[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, ReadWaitThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts1[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads1[j] = hWaitThread;
				}
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
				SetThreadAffinityMask(WaitThreads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
				j++;

			}

		}
		NumThreads1 = j;

		if (i != ThreadCount1)
		{
			// some create thread failed, bail out
			//printf("Some CreateThread was failed, stop\n");
			g_TimeUp1 = 0;
			Contexts1[i].ThreadDone = 0;
			Contexts1[i].WaitThreadDone = 0;
		}

	}
	else
	{
		//printf("Wrong Engine %d\n ", Engine);
	}
	return status;
}


BOOL ThreadedReadWriteTest(int Engine, int PacketSize, int mode)
{
	DWORD_PTR	pAffinity0;
	DWORD_PTR	pAffinity1;
	BOOL		status = TRUE;
	ULONG		TestTimeSeconds = TestTime;
	float		GbpsTransferred = 0;
	int			TestThreadCount;
	int			i;
	int         j;
	int         k;
	DWORD       last_error_status;

	HANDLE hThread;
	HANDLE hWaitThread;
	HANDLE hBlockDevice;

	//printf("Started ThreadedReadWriteTest\r\n");

	if (Engine == 0)
	{
		hBlockDevice = hBlockDevice0;
		g_TimeUp0 = 1;
		g_TimeUp0_1 = 1;

		//		TestThreadCount = ThreadCount0 + 1;
		TestThreadCount = ThreadCount0;//Making sure number of threads are same in all modes.
#if 0
		if (TestThreadCount > ProcessorCount)
			TestThreadCount = ProcessorCount;
#endif
		Contexts0 = new THREAD_CONTEXT[TestThreadCount];
		ContextCount0 = TestThreadCount;
		Threads0 = new HANDLE[TestThreadCount];
		WaitThreads0 = new HANDLE[TestThreadCount];

		if (Contexts0 == NULL || Threads0 == NULL || WaitThreads0 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedReadWriteTest with error code %x\n", last_error_status);
			return FALSE;
		}

		//		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); //ABOVE_NORMAL);

		/*printf("\nStarting %d threads running for %d seconds, please wait...\n",
		TestThreadCount, TestTimeSeconds);*/
		if (!Quiet)
		{
			//printf("Creating %d threads...\n", ThreadCount0);
		}
#if 0
		// Since we are using several threads let the system balance the affinity.
		// Seems to perform better in the case.
		pAffinity = (DWORD)(1 << ProcessorCount);
#endif
		pAffinity0 = 1 << NUM_CORES_TO_LEAVE;
		for (i = 0, j = 0; i < TestThreadCount; i++, j++)
		{
			if ((i % 2) == 0)
			{
				//  Create Write Thread
				Contexts0[i].hBlockDevice = hBlockDevice;
				Contexts0[i].engine = Engine;
				Contexts0[i].mode = mode;
				WriteBufferSize = PacketSize;
				Contexts0[i].BufferSize = WriteBufferSize;
				Contexts0[i].Buffer = ThreadBuffer0[i];
				Contexts0[i].quiet = Quiet;
				Contexts0[i].debug = Debug;
				Contexts0[i].ExitOnError = ExitOnError;
				Contexts0[i].ThreadDone = 1;
				Contexts0[i].WaitThreadDone = 1;
				Contexts0[i].BytesXfered = 0;
				Contexts0[i].ThreadNumber = i;
				Contexts0[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts0[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts0[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedReadWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts0[i].hEvent[k] = Contexts0[i].os[k].hEvent;
						//	printf("Event handle created at ThreadedReadWriteTest are %p , %p\n",Contexts0[i].hEvent[k],Contexts0[i].os[k].hEvent);
					}
					Contexts0[i].os[k].Offset = 0;
					Contexts0[i].os[k].OffsetHigh = 0;
				}
				Contexts0[i].osIndexTail = 0;
				Contexts0[i].osIndexHead = 0;
				Contexts0[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts0[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedReadWriteTest with error code %x\n", last_error_status);
				}
				hThread = CreateThread(NULL, 0, WriteThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//Status = 1;
					Contexts0[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads0[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, WriteWaitThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts0[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads0[j] = hWaitThread;
				}
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
				SetThreadAffinityMask(WaitThreads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
			}
			else
			{
				//  Create Read Thread
				if (bRawEthernetMode)
				{
					Contexts0[i].hBlockDevice = hBlockDevice1;
				}
				else
				{
					Contexts0[i].hBlockDevice = hBlockDevice;
				}
				Contexts0[i].engine = Engine;
				Contexts0[i].mode = mode;
				ReadBufferSize = PacketSize;
				Contexts0[i].BufferSize = ReadBufferSize;
				Contexts0[i].Buffer = ThreadBuffer0[i];
				Contexts0[i].quiet = Quiet;
				Contexts0[i].debug = Debug;
				Contexts0[i].ExitOnError = ExitOnError;
				Contexts0[i].ThreadDone = 1;
				Contexts0[i].WaitThreadDone = 1;
				Contexts0[i].BytesXfered = 0;
				Contexts0[i].ThreadNumber = i;
				Contexts0[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts0[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts0[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedReadWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts0[i].hEvent[k] = Contexts0[i].os[k].hEvent;
						//	printf("Event handle created at ThreadedReadWriteTest are %p , %p\n",Contexts0[i].hEvent[k],Contexts0[i].os[k].hEvent);
					}
					Contexts0[i].os[k].Offset = 0;
					Contexts0[i].os[k].OffsetHigh = 0;
				}
				Contexts0[i].osIndexTail = 0;
				Contexts0[i].osIndexHead = 0;
				Contexts0[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts0[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedReadWriteTest with error code %x\n", last_error_status);
				}
				hThread = CreateThread(NULL, 0, ReadThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//this->Status = 1;
					Contexts0[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads0[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, ReadWaitThreadProc, &Contexts0[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts0[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads0[j] = hWaitThread;
				}
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
				SetThreadAffinityMask(WaitThreads0[j], pAffinity0);
				pAffinity0 = pAffinity0 << 1;
			}
#if 0		
			if (NULL == hThread)
			{
				printf("Failed to create thread %d\n", i);
				//this->Status = 1;
				Contexts0[i].ThreadDone = 0;
				break;
			}
			else
			{
				Threads0[i] = hThread;
			}
			//
			//  Set the processor Affinity, seems to perform better in this case.
			//
			SetThreadAffinityMask(Threads0[i], pAffinity);
			pAffinity = pAffinity >> 1;
#endif
		}
		NumThreads0 = TestThreadCount;
		if (i != TestThreadCount)
		{
			// some create thread failed, bail out
			//printf("ERROR: CreateThread failed, stop\n");
			g_TimeUp0 = 0;
			Contexts0[i].ThreadDone = 0;
		}
	}
	else if (Engine == 1)
	{
		hBlockDevice = hBlockDevice1;
		g_TimeUp1 = 1;
		g_TimeUp1_0 = 1;

		//TestThreadCount = ThreadCount1 + 1;
		TestThreadCount = ThreadCount1;//Making sure number of threads are same in all modes.
#if 0
		if (TestThreadCount > ProcessorCount)
			TestThreadCount = ProcessorCount;
#endif

		Contexts1 = new THREAD_CONTEXT[TestThreadCount];
		ContextCount1 = TestThreadCount;
		Threads1 = new HANDLE[TestThreadCount];
		WaitThreads1 = new HANDLE[TestThreadCount];

		if (Contexts1 == NULL || Threads1 == NULL || WaitThreads1 == NULL)
		{
			last_error_status = GetLastError();
			//printf("new failed at ThreadedReadWriteTest with error code %x\n", last_error_status);
			return FALSE;
		}
		//		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); //ABOVE_NORMAL);

		/*printf("\nStarting %d threads running for %d seconds, please wait...\n",
		TestThreadCount, TestTimeSeconds);*/
		if (!Quiet)
		{
			//printf("Creating %d threads...\n", ThreadCount1);
		}
#if 0
		// Since we are using several threads let the system balance the affinity.
		// Seems to perform better in the case.
		pAffinity = (DWORD)(1 << ProcessorCount);
#endif
		pAffinity1 = 1 << ((2 * DEFAULT_THREAD_COUNT) + NUM_CORES_TO_LEAVE);
		for (i = 0, j = 0; i < TestThreadCount; i++, j++)
		{
			if ((i % 2) == 0)
			{
				//  Create Write Thread
				Contexts1[i].hBlockDevice = hBlockDevice;
				Contexts1[i].engine = Engine;
				Contexts1[i].mode = mode;
				WriteBufferSize = PacketSize;
				Contexts1[i].BufferSize = WriteBufferSize;
				Contexts1[i].Buffer = ThreadBuffer1[i];
				Contexts1[i].quiet = Quiet;
				Contexts1[i].debug = Debug;
				Contexts1[i].ExitOnError = ExitOnError;
				Contexts1[i].ThreadDone = 1;
				Contexts1[i].WaitThreadDone = 1;
				Contexts1[i].BytesXfered = 0;
				Contexts1[i].ThreadNumber = i;
				Contexts1[i].ContextUsed = TRUE;
				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts1[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts1[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedWriteTest with error code %x\n", last_error_status);
					}
					else
					{
						Contexts1[i].hEvent[k] = Contexts1[i].os[k].hEvent;
					}
					Contexts1[i].os[k].Offset = 0;
					Contexts1[i].os[k].OffsetHigh = 0;
				}
				Contexts1[i].osIndexTail = 0;
				Contexts1[i].osIndexHead = 0;
				Contexts1[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts1[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedWriteTest with error code %x\n", last_error_status);
				}

				hThread = CreateThread(NULL, 0, WriteThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//Status = 1;
					Contexts1[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads1[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, WriteWaitThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts1[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads1[j] = hWaitThread;
				}
#if 1
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
				SetThreadAffinityMask(WaitThreads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
#endif
			}
			else
			{
				//  Create Read Thread
				if (bRawEthernetMode)
				{
					Contexts1[i].hBlockDevice = hBlockDevice0;
				}
				else
				{
					Contexts1[i].hBlockDevice = hBlockDevice;
				}
				Contexts1[i].engine = Engine;
				Contexts1[i].mode = mode;
				ReadBufferSize = PacketSize;
				Contexts1[i].BufferSize = ReadBufferSize;
				Contexts1[i].Buffer = ThreadBuffer1[i];
				Contexts1[i].quiet = Quiet;
				Contexts1[i].debug = Debug;
				Contexts1[i].ExitOnError = ExitOnError;
				Contexts1[i].ThreadDone = 1;
				Contexts1[i].WaitThreadDone = 1;
				Contexts1[i].BytesXfered = 0;
				Contexts1[i].ThreadNumber = i;
				Contexts1[i].ContextUsed = TRUE;

				for (k = 0; k<MAX_EVENTS_PUSHED; k++)
				{
					Contexts1[i].os[k].hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (Contexts1[i].os[k].hEvent == NULL)
					{
						last_error_status = GetLastError();
						//printf("CreateEvent failed in ThreadedWriteTest with error code %x\n",last_error_status);
					}
					else
					{
						Contexts1[i].hEvent[k] = Contexts1[i].os[k].hEvent;
					}
					Contexts1[i].os[k].Offset = 0;
					Contexts1[i].os[k].OffsetHigh = 0;
				}
				Contexts1[i].osIndexTail = 0;
				Contexts1[i].osIndexHead = 0;
				Contexts1[i].osIndexFree = MAX_EVENTS_PUSHED;
				if (!(InitializeCriticalSectionAndSpinCount(&(Contexts1[i].cs), SPIN_COUNT_FOR_CS)))
				{
					last_error_status = GetLastError();
					//printf("InitializeCriticalSectionAndSpinCount failed in ThreadedWriteTest with error code %x\n", last_error_status);
				}

				hThread = CreateThread(NULL, 0, ReadThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hThread)
				{
					//printf("Failed to create thread %d\n", i);
					//this->Status = 1;
					Contexts1[i].ThreadDone = 0;
					break;
				}
				else
				{
					Threads1[j] = hThread;
				}
				hWaitThread = CreateThread(NULL, 0, ReadWaitThreadProc, &Contexts1[i], 0, NULL);
				if (NULL == hWaitThread)
				{
					//printf("Failed to create write wait thread %d\n", i);
					//Status = 1;
					Contexts1[i].WaitThreadDone = 0;
					break;
				}
				else
				{
					WaitThreads1[j] = hWaitThread;
				}
				//
				//  Set the processor Affinity, seems to perform better in this case.
				//
				SetThreadAffinityMask(Threads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
				SetThreadAffinityMask(WaitThreads1[j], pAffinity1);
				pAffinity1 = pAffinity1 << 1;
			}
#if 0		
			if (NULL == hThread)
			{
				printf("Failed to create thread %d\n", i);
				//this->Status = 1;
				Contexts1[i].ThreadDone = 0;
				break;
			}
			else
			{
				Threads1[i] = hThread;
			}
			//
			//  Set the processor Affinity, seems to perform better in this case.
			//
			SetThreadAffinityMask(Threads1[i], pAffinity);
			pAffinity = pAffinity >> 1;
#endif
		}
		NumThreads1 = TestThreadCount;
		if (i != TestThreadCount)
		{
			// some create thread failed, bail out
			//printf("ERROR: CreateThread failed, stop\n");
			g_TimeUp1 = 0;
			Contexts1[i].ThreadDone = 0;
		}
	}
	else
	{
		//printf("Wrong Engine %d\n ", Engine);
	}

	//printf("Finished ThreadedReadWriteTest\r\n");

	return status;
}



//
//
//
//  Java Interface routines
//

void init(PUCHAR baseWriteBuffer, ULONG writeBufferSize, PUCHAR baseReadBuffer, ULONG readBufferSize)
{
	BOOL retValue = TRUE;
	int i = 0;
	hDMADevice = INVALID_HANDLE_VALUE;
	hBlockDevice0 = INVALID_HANDLE_VALUE;
	hBlockDevice1 = INVALID_HANDLE_VALUE;

	Contexts0 = NULL;
	Contexts1 = NULL;
	ContextCount0 = 0;
	ContextCount1 = 0;
	Threads0 = NULL;
	Threads1 = NULL;
	ThreadCount0 = DEFAULT_THREAD_COUNT;
	ThreadCount1 = DEFAULT_THREAD_COUNT;
	ProcessorCount = 8;

	CSInitialized = FALSE;
	TestTime = 30;
	Quiet = TRUE;
	bVerbose = FALSE;
	Debug = FALSE;
	ExitOnError = TRUE;
	Status = 0;
	AppNumber = 0;
	ReadBufferSize = DEFAULT_BUFFER_SIZE;
	WriteBufferSize = DEFAULT_BUFFER_SIZE;
	bRawEthernetMode = FALSE;
	bInternalXferMode = FALSE;

	// Make sure we have the correct driver configuration
	/*if (GetDriverChildConfig(0) != CHILD_DRIVER_CONFIG_XBLOCK)
	{
	printf("Xilinx XDMA Driver is incorrectly configured.\n");
	Status = -1;
	return;
	}*/
	// Check for Raw Ethernet mode
	if (GetRawEthernet(getBoardNumber()))
	{
		//printf("Xilinx XDMA Set in Raw Ethernet mode.\n");
		bRawEthernetMode = TRUE;
	}
	// Are we using application buffers?
	if (GetTestConfig(getBoardNumber()))
	{
		//printf("Xilinx XDMA Set to use internal buffers.\n");
		bInternalXferMode = TRUE;
	}

	hDMADevice = OpenDriverInterface(XDMA_DRIVER, 0);
	if (hDMADevice == INVALID_HANDLE_VALUE)
	{
		//printf("Xilinx XDMA Driver not found.\n");
		Status = GetLastError();
		return;
	}
	if (GetDriverChildConfig(0) == 5){
		//printf("Child Driver Config obtained is 5\n");
		hBlockDevice0 = OpenDriverInterface(XBLOCK_DRIVER, XBLOCK_DRIVER0);
		if (hBlockDevice0 == NULL)
		{
			//printf("Xilinx XBlock Driver not found.\n");
			Status = GetLastError();
			return;
		}

		ReadBuffer = WriteBuffer = NULL;
		/*		for (i=0; i < (DEFAULT_THREAD_COUNT + 1);i++)
		{
		ThreadBuffer0[i] =(PUCHAR)VirtualAlloc(NULL, DEFAULT_BUFFER_SIZE * MIN_PACKETS_IN_BUFFER, MEM_COMMIT, PAGE_READWRITE);
		}
		for (i=0; i < (DEFAULT_THREAD_COUNT + 1);i++)
		{
		ThreadBuffer1[i] =(PUCHAR)VirtualAlloc(NULL, DEFAULT_BUFFER_SIZE * MIN_PACKETS_IN_BUFFER, MEM_COMMIT, PAGE_READWRITE);
		}

		DWORD* baseWriteBuffer = reinterpret_cast<DWORD*>(ThreadBuffer0[0]);
		DWORD* baseReadBuffer = reinterpret_cast<DWORD*>(ThreadBuffer0[1]);

		for (int i = 0; i < 32768 * 1000 / 4; ++i) {
		baseWriteBuffer[i] = 0x13371337;
		}
		for (int i = 0; i < 32768 * 1000 / 4; ++i) {
		baseReadBuffer[i] = 0x00000000;
		}

		printf("Data at base write is: %X\r\n", baseWriteBuffer[0]);
		printf("Data at base read is: %X\r\n", baseReadBuffer[0]);

		*/
		ThreadBuffer0[0] = baseWriteBuffer;
		myWriteBufferSize = writeBufferSize;
		ThreadBuffer0[1] = baseReadBuffer;
		myReadBufferSize = readBufferSize;

	}
	if (!CSInitialized)
	{
		retValue = InitializeCriticalSectionAndSpinCount(&CriticalSection, SPIN_COUNT_FOR_CS);
		if (!retValue)
		{
			//printf("InitializeCritialSection failed.\n");
			Status = GetLastError();
			return;
		}
		CSInitialized = TRUE;
	}
	/*
	if (fopen_s(&f, "c:\\logs\\driverlog.txt", "w") == 0)
	{
	fprintf(f, "Initialized\n");
	}
	*/
	//pci_callback = env->GetMethodID(cls, "pciStateCallback", "([I)V");
	//eng_callback = env->GetMethodID(cls, "engStateCallback", "([[I)V");
	//trn_callback = env->GetMethodID(cls, "trnStatsCallback", "([F)V");
	//dma_callback = env->GetMethodID(cls, "dmaStatsCallback", "([[F)V");
	//sws_callback = env->GetMethodID(cls, "swsStatsCallback", "([[I)V");
	//power_callback = env->GetMethodID(cls, "powerStatsCallback", "([I)V");
	//log_callback = env->GetMethodID(cls, "showLogCallback", "(I)V");
	//led_callback = env->GetMethodID(cls, "ledStatsCallback", "([I)V");
}

int flush() {
	int i = 0;
	if (CSInitialized)
	{
		DeleteCriticalSection(&CriticalSection);
	}

	if (Contexts0)
	{
		for (int i = 0; i < ContextCount0; i++)
		{
			if (Contexts0[i].Buffer)
			{
				delete Contexts0[i].Buffer;
			}
		}
		delete Contexts0;
		Contexts0 = NULL;
	}
	if (Contexts1)
	{
		for (int i = 0; i < ContextCount1; i++)
		{
			if (Contexts1[i].Buffer)
			{
				delete Contexts1[i].Buffer;
			}
		}
		delete Contexts1;
		Contexts1 = NULL;
	}

	/*		for (i=0; i < (DEFAULT_THREAD_COUNT + 1);i++)
	{
	if(ThreadBuffer0[i])
	VirtualFree(ThreadBuffer0[i], 0, MEM_RELEASE);
	}

	for (i=0; i < (DEFAULT_THREAD_COUNT + 1);i++)
	{
	if(ThreadBuffer1[i])
	VirtualFree(ThreadBuffer1[i], 0, MEM_RELEASE);
	}
	*/

	if (hBlockDevice0 != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hBlockDevice0);
		hBlockDevice0 = INVALID_HANDLE_VALUE;
	}

	if (hBlockDevice1 != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hBlockDevice1);
		hBlockDevice1 = INVALID_HANDLE_VALUE;
	}

	if (hDMADevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDMADevice);
		hDMADevice = INVALID_HANDLE_VALUE;
	}
	return 0;
}

int startTestExternal(){
	int eng = 0;
	int testmode = 0;
	int maxsize = 32768;
	int tmode = 0;

	//printf(" Start testmode= %x   \n", testmode); 
	if (testmode == 0) // loopback
		tmode = ENABLE_LOOPBACK;
	else if (testmode == 1) // checker
		tmode = ENABLE_PKTCHK;
	else if (testmode == 2) // generator
		tmode = ENABLE_PKTGEN;
	else if (testmode == 3)
		tmode = ENABLE_PKTCHK | ENABLE_PKTGEN;

	tmode = TEST_START | tmode;
	AppNumber = eng;
	return startTest(eng, tmode, maxsize);
}

int stopTestExternal(){
	int eng = 0;
	int testmode = 0;
	int maxsize = 32768;
	int tmode = 0;
	int val = 0;
	if (testmode == 0) // loopback
		tmode = ENABLE_LOOPBACK;
	else if (testmode == 1) // checker
		tmode = ENABLE_PKTCHK;
	else if (testmode == 2) // generator
		tmode = ENABLE_PKTGEN;
	else if (testmode == 3)
		tmode = ENABLE_PKTCHK | ENABLE_PKTGEN;

	tmode = TEST_STOP | tmode;
	val = stopTest(eng, tmode, maxsize);
	//Sleep(1000);
	return val;
}

//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1DMAStats(JNIEnv *env, jobject obj)
//{
//    int j,i;
//	jfloat tmp[MAX_ARRAY_COUNT][4]={0};
//
//    EngStatsArray	DMAStatsArray;
//	 
//    OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	DMAStatsArray.Count = MAX_ARRAY_COUNT;
//
//	if (!DeviceIoControl(hDMADevice, IGET_DMA_STATISTICS, 
//		&DMAStatsArray, sizeof(EngStatsArray),
//		&DMAStatsArray, sizeof(EngStatsArray), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("GetDMAStats failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("GetDMAStats failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("GetDMAStats ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//   
//	int k = 0;
//
//	for (j = 0; j < DMAStatsArray.Count; j++)
//	{
//		if (k >= MAX_ENGS) k = 0;
//		
//		tmp[k][0] = (jfloat)DMAStatsArray.eng[j].Engine;
//		tmp[k][1] = (jfloat)((double)(DMAStatsArray.eng[j].LBR) * MULTIPLIER )/DIVISOR * DMAStatsArray.eng[j].scaling_factor;
//		tmp[k][2] = (jfloat)DMAStatsArray.eng[j].LAT;
//		tmp[k][3] = (jfloat)DMAStatsArray.eng[j].LWT;
//		k++;        
//	}
//     
//     jfloatArray row= (jfloatArray)env->NewFloatArray(4);
//     jobjectArray ret=env->NewObjectArray(MAX_ENGS, env->GetObjectClass(row), 0);
//
//     for(i=0;i<MAX_ENGS;i++) {
//    	row= (jfloatArray)env->NewFloatArray(4);
//        env->SetFloatArrayRegion(row,0,4,tmp[i]);
//    	env->SetObjectArrayElement(ret,i,row);
//     }
//     env->CallVoidMethod(obj, dma_callback, ret);
//     return 0;	
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1TRNStats(JNIEnv *env, jobject obj)
//{
//
//    int j;
//    TRNStatsArray TrnStatsArray;
//   	OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//
//    os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	TrnStatsArray.Count = MAX_ARRAY_COUNT;
//    
//    jfloatArray jf;
//    jf = env->NewFloatArray(2);
//	jfloat tmp[2]={0};
//
//   if (!DeviceIoControl(hDMADevice, IGET_TRN_STATISTICS, 
//		&TrnStatsArray, sizeof(TRNStatsArray),
//		&TrnStatsArray, sizeof(TRNStatsArray), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("GetTransStats failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("GetTransStats failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("GetTransStats ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//
//    for(j=0; j<TrnStatsArray.Count; j++)
//    {
//        //printf("Count after call %d", tsa.Count);
//		tmp[0] = (jfloat)((double)(TrnStatsArray.trn[j].LTX) * MULTIPLIER)/DIVISOR * TrnStatsArray.trn[j].scaling_factor;
//        tmp[1] = (jfloat)((double)(TrnStatsArray.trn[j].LRX) * MULTIPLIER)/DIVISOR * TrnStatsArray.trn[j].scaling_factor;
//    }
//
//    env->SetFloatArrayRegion(jf, 0, 2, tmp); 
//    env->CallVoidMethod(obj, trn_callback, jf);
//	return 0;
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1PowerStats(JNIEnv *env, jobject obj)
//{
//	PowerMonitor pPowerMonitor;
//
//	OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	if (!DeviceIoControl(hDMADevice, IGET_POWER_STATISTICS, 
//		NULL, 0, &pPowerMonitor, sizeof(PowerMonitor), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("GetPower Stats failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("GetPower Stats  failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("GetPower Stats  ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//   jintArray ji;
//   ji = env->NewIntArray(7);
//   jint tmp[7]={0};
//
//   tmp[0] = pPowerMonitor.VCCINT_PWR_CONS;
//   tmp[1] = pPowerMonitor.VCCAUX_PWR_CONS;
//   tmp[2] = pPowerMonitor.MGT_AVCC_PWR_CONS;
//   tmp[3] = pPowerMonitor.VCCBRAM_PWR_CONS;
//   tmp[4] = pPowerMonitor.DIE_TEMP_PWR_CONS;
//   //tmp[5] = getErrorCount0();
//  // tmp[6] = getErrorCount1(); 
//   //printf("vcc: %d vccaux: %d temp: %d\n", tmp[0], tmp[1], tmp[4]);
//   env->SetIntArrayRegion(ji, 0, 7, tmp); 
//   env->CallVoidMethod(obj, power_callback, ji);
//   return 0;
//}
//
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1SWStats (JNIEnv *env, jobject obj){
//
//    int j;
//    SWStatsArray SwStatsArray;
//    OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//
//    jint tmp[32][2] = {0};
//
//    os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	memset(&SwStatsArray, 0, sizeof(SWStatsArray));
//
//	SwStatsArray.Count = MAX_ARRAY_COUNT;
//	if (!DeviceIoControl(hDMADevice, IGET_SW_STATISTICS, 
//		&SwStatsArray, sizeof(SWStatsArray),
//		&SwStatsArray, sizeof(SWStatsArray), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("GetSWStats failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("GetSWStats failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("GetSWStats ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//
//    for (j = 0; j < SwStatsArray.Count; j++)
//    {
//        int		k;
//
//        /* Driver engine number does not directly map to that of GUI */
//        for (k = 0; k < MAX_ENGS; k++)
//        {
//            if (DMAConfig[k].Engine == SwStatsArray.sw[j].Engine)
//                break;
//        }
//
//        if (k >= MAX_ENGS) continue;
//        tmp[j][0] = SwStatsArray.sw[j].Engine;
//        tmp[j][1] = SwStatsArray.sw[j].LBR;
//    }
//	return 0;
//}
//
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1EngineState(JNIEnv *env, jobject obj)
//{
//    int i;
//    EngState EngInfo;
//    int state;
//  	OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//
//    jint tmp[MAX_ENGS][14] = {0}; 		
//    /* Get the state of all the engines */
//    for(i=0; i<MAX_ENGS; i++)
//    {
//        EngInfo.Engine = DMAConfig[i].Engine;
//		os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//		os.Offset = 0;
//		os.OffsetHigh = 0;
//				
//		if (!DeviceIoControl(hDMADevice, IGET_ENG_STATE, 
//		NULL, 0, &EngInfo, sizeof(EngInfo), &bytes, &os))
//		{
//			LastErrorStatus = GetLastError();
//			if (LastErrorStatus == ERROR_IO_PENDING)
//			{
//				// Wait here (forever) for the Overlapped I/O to complete
//				if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//				{
//					//printf("IGET_ENG_STATE on Engine %d failed\n", EngInfo.Engine);
//					for (int k = 0; k < 14; ++k)
//					   tmp[i][k] = 0;
//					// Overlapped failed, fnd out why and exit
//					LastErrorStatus = GetLastError();
//					printf("GetEngineState failed, Error code 0x%x.\n", LastErrorStatus);
//					return LastErrorStatus;
//				}
//			}
//			else
//			{
//				//printf("IGET_ENG_STATE on Engine %d failed\n", EngInfo.Engine);
//				for (int k = 0; k < 14; ++k)
//					tmp[i][k] = 0;
//				printf("GetEngineState failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//			//printf("IGET_ENG_STATE on Engine %d failed\n", EngInfo.Engine);
//            for (int k = 0; k < 14; ++k)
//               tmp[i][k] = 0;
//			LastErrorStatus = GetLastError();
//			printf("GetEngineState ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//		unsigned int testmode;
//
//            tmp[i][0] = EngInfo.Engine;
//            tmp[i][1] = EngInfo.BDs;
//            tmp[i][2] = EngInfo.Buffers;
//            tmp[i][3] = EngInfo.MinPktSize;
//            tmp[i][4] = EngInfo.MaxPktSize;
//            tmp[i][5] = EngInfo.BDerrs;
//            tmp[i][6] = EngInfo.BDSerrs;
//            tmp[i][7] = EngInfo.DataMismatch;
//            tmp[i][8] = EngInfo.IntEnab;
//            tmp[i][9] = EngInfo.TestMode;    
//            // These additional ones are for EngStats structure
//            testmode = EngInfo.TestMode;
//            state = (testmode & (TEST_START|TEST_IN_PROGRESS)) ? 1 : -1;
//            tmp[i][10] = state; // EngStats[i].TXEnab
//            state = (testmode & ENABLE_LOOPBACK)? 1 : -1;
//            tmp[i][11] = state; // EngStats[i].LBEnab
//            state = (testmode & ENABLE_PKTGEN)? 1 : -1;
//            tmp[i][12] = state; // EngStats[i].PktGenEnab
//            state = (testmode & ENABLE_PKTCHK)? 1 : -1;
//            tmp[i][13] = state; //EngStats[i].PktChkEnab
//		CloseHandle(os.hEvent);
//    }
//   	
//    jintArray row= (jintArray)env->NewIntArray(14);
//    jobjectArray ret=env->NewObjectArray(MAX_ENGS, env->GetObjectClass(row), 0);
//
//    for(i=0;i<MAX_ENGS;i++) {
//    	row= (jintArray)env->NewIntArray(14);
//    	env->SetIntArrayRegion(row,0,14,tmp[i]);
//    	env->SetObjectArrayElement(ret,i,row);
//    }
//    env->CallVoidMethod(obj, eng_callback, ret);
//    return 0; 
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_get_1PCIstate(JNIEnv *env, jobject obj)
//{
//	PCIState		PCIState;
//    OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//	
////	fprintf(f, "Calling PCIState\n");
//	
//	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	if (!DeviceIoControl(hDMADevice, IGET_PCI_STATE, 
//		NULL, 0, &PCIState, sizeof(PCIState), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("GetPCIState failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("GetPCIState failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("GetPCIState ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//    	
//     jintArray ji;
//     ji = env->NewIntArray(16);
//     jint tmp[16];
//
//	
//     tmp[0] = PCIState.Version;
//     tmp[1] = PCIState.LinkState;
//     tmp[2] = PCIState.LinkSpeed;
//     tmp[3] = PCIState.LinkWidth;
//     tmp[4] = PCIState.VendorId;
//     tmp[5] = PCIState.DeviceId;
//     tmp[6] = PCIState.IntMode;
//     tmp[7] = PCIState.MPS;
//     tmp[8] = PCIState.MRRS;
//     tmp[9] = PCIState.InitFCCplD;
//     tmp[10] = PCIState.InitFCCplH;
//     tmp[11] = PCIState.InitFCNPD;
//     tmp[12] = PCIState.InitFCNPH;
//     tmp[13] = PCIState.InitFCPD;
//     tmp[14] = PCIState.InitFCPH; 	
//     tmp[15] = PCIState.LinkUpCap;
//
//	
//     env->SetIntArrayRegion(ji, 0, 16, tmp); 
//     env->CallVoidMethod(obj, pci_callback, ji);
////	 fprintf(f, "PCI State Done \n");
//     return 0;
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_setLinkSpeed(JNIEnv *env, jobject jobj, jint speed){
//    /*DirectLinkChg dLink;
//
//    dLink.LinkSpeed = 1;   // default
//    dLink.LinkWidth = 1;   // default
//
//    dLink.LinkSpeed = speed;
//
//    if(ioctl(statsfd, ISET_PCI_LINKSPEED, &dLink) < 0)
//    {
//        //printf("ISET_PCI_LINKSPEED failed\n");
//        return -1;
//    }*/
//    return -1;
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_setLinkWidth(JNIEnv *env, jobject jobj, jint width){
//
//    /*DirectLinkChg dLink;
//
//    dLink.LinkSpeed = 1;   // default
//    dLink.LinkWidth = 1;   // default
//
//    dLink.LinkWidth = width;
//
//    if(ioctl(statsfd, ISET_PCI_LINKWIDTH, &dLink) < 0)
//    {
//        //printf("ISET_PCI_LINKWIDTH failed\n");
//        return -1;
//    }*/
//	return -1;
//}
//
//JNIEXPORT jint JNICALL Java_com_xilinx_gui_DriverInfoGen_LedStats(JNIEnv *env, jobject obj){
//     LedStats ledStats;
//
//     
//	 OVERLAPPED		os;			// OVERLAPPED structure for the operation
//    DWORD			bytes;
//	DWORD			LastErrorStatus = 0;
//	DWORD			Status = 0;
//
//	os.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//	os.Offset = 0;
//	os.OffsetHigh = 0;
//
//	if (!DeviceIoControl(hDMADevice, IGET_LED_STATISTICS, 
//		NULL, 0, &ledStats, sizeof(ledStats), &bytes, &os))
//	{
//		LastErrorStatus = GetLastError();
//		if (LastErrorStatus == ERROR_IO_PENDING)
//		{
//			// Wait here (forever) for the Overlapped I/O to complete
//			if (!GetOverlappedResult(hDMADevice, &os, &bytes, TRUE))
//			{
//				// Overlapped failed, fnd out why and exit
//				LastErrorStatus = GetLastError();
//	            printf("LEDStatistcs failed, Error code 0x%x.\n", LastErrorStatus);
//				return LastErrorStatus;
//			}
//		}
//		else
//		{
//            printf("LEDStatistcs failed, Error code 0x%x.\n", LastErrorStatus);
//			return LastErrorStatus;
//		}
//    }
//    else
//    {
//		LastErrorStatus = GetLastError();
//		printf("LEDStatistcs ioctl failed, Error code 0x%x.\n", LastErrorStatus);
//		return LastErrorStatus;
//    }
//	CloseHandle(os.hEvent);
//	 
//     jintArray ji;
//     ji = env->NewIntArray(6);
//     jint tmp[6];
//     
//     tmp[0] = ledStats.DdrCalib1;
//     tmp[1] = ledStats.DdrCalib2;
//     tmp[2] = ledStats.Phy0;
//     tmp[3] = ledStats.Phy1; 
//     tmp[4] = ledStats.Phy2;
//     tmp[5] = ledStats.Phy3; 
// 
//     env->SetIntArrayRegion(ji, 0, 6, tmp); 
//     env->CallVoidMethod(obj, led_callback, ji);
//     return 0;
//}
//
void FormatBuffer(unsigned char *buf, int bufferSize, int chunkSize, int pktSize, BOOL bRawEthernetMode)
{
	int i, j = 0;
	unsigned short TxSeqNo = 0;

	if (bRawEthernetMode)
	{
		while (j <  bufferSize)
		{
			*(unsigned short *)(buf + j + 0) = 0xFFFF;
			*(unsigned short *)(buf + j + 2) = 0xFFFF;
			*(unsigned short *)(buf + j + 4) = 0xFFFF;
			*(unsigned short *)(buf + j + 6) = 0xAABB;
			*(unsigned short *)(buf + j + 8) = 0xCCDD;
			*(unsigned short *)(buf + j + 10) = 0xEEFF;

			//- For jumbo frame, make T/L field opcode 0x8870	
			//- for certain lengths lesser than 1500B, 
			//there are opcode conflicts leading to dropped packets
			*(unsigned short *)(buf + j + 12) = 0x8870;

			/* Apply data pattern in the buffer */
			for (i = 14; i < chunkSize; i = i + 2)
				*(unsigned short *)(buf + j + i) = TxSeqNo;

			j += i;
			TxSeqNo++;
			if (TxSeqNo >= TX_CONFIG_SEQNO)
				TxSeqNo = 0;
		}
	}
	else
	{
		while (j  <  bufferSize)
		{
			for (i = 0; i < chunkSize; i = i + 2)
			{
				if (i == 0)
					*(unsigned short *)(buf + j + i) = pktSize;
				else
					*(unsigned short *)(buf + j + i) = TxSeqNo;

			}
			j += i;
			TxSeqNo++;
			if (TxSeqNo >= TX_CONFIG_SEQNO)
				TxSeqNo = 0;
		}
	}

}

