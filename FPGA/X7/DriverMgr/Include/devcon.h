/*++

--*/

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <regstr.h>
#include <cfgmgr32.h>
#include <string.h>
#include <malloc.h>
#include <newdev.h>
#include <objbase.h>
#include <strsafe.h>
#include "DriverMgr.h"

#define PCI_CLASS_STR			TEXT("PCI")
#define PCI_VEN_STR				TEXT("PCI\\VEN_10EE")
#define XDMA_CLASS_STR			TEXT("XDMA")
#define XBLOCK_VEN_STR			TEXT("XDMA\\XBLOCK")
#define XNET_VEN_STR			TEXT("XDMA\\XNET")

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

typedef struct _CMD_CONTEXT {
    DWORD	Count;
    DWORD	Control;
	int		Instance;
	int		DevType;
	PDEV_STATUS_LIST pDevStats;
} CMD_CONTEXT, *PCMD_CONTEXT;


typedef int (*CallbackFunc)(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in PCMD_CONTEXT Context);

int EnumerateDevices(__in CallbackFunc Callback, __in PCMD_CONTEXT Context);

int ControlCallback(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in PCMD_CONTEXT pContext);
int FindCallback(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo,	__in PCMD_CONTEXT Context);


LPTSTR GetDeviceStringProperty(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop);
LPTSTR GetDeviceDescription(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo);

// Functions located in DevCon.cpp
//
BOOL WildCompareHwIds(__in PZPWSTR Array, __in LPCTSTR MatchString);
LPTSTR * GetDevMultiSz(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo, __in DWORD Prop);
void DelMultiSz(__in PZPWSTR Array);
BOOL DeviceDescr(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo);
BOOL DeviceStatus(__in HDEVINFO Devs, __in PSP_DEVINFO_DATA DevInfo);

#define FIND_DEVICE         0x00000001 // display device
#define FIND_STATUS         0x00000002 // display status of device

//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

