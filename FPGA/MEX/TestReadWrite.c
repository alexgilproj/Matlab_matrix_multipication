#include <windows.h>
#include <stdio.h>
#include "mex.h"

#define KERNEL_64BIT

#include "status_strings.h"
#include "vc709_lib.h"


WDC_DEVICE_HANDLE DeviceFindAndOpen(DWORD dwVendorId, DWORD dwDeviceId);
BOOL DeviceFind(DWORD dwVendorId, DWORD dwDeviceId, WD_PCI_SLOT* pSlot);
WDC_DEVICE_HANDLE DeviceOpen(const WD_PCI_SLOT* pSlot);
void DeviceClose(WDC_DEVICE_HANDLE hDev);

void DeviceReadWriteData(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction, DWORD dwOffset, UINT32* data);


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
    WDC_DEVICE_HANDLE hDev = NULL;
    DWORD dwStatus;

    DWORD addr;
    UINT32 data_in;
    UINT32 data_out = 0xFFFFFFFF;

    /* Check number of input and output arguments. */
    if (nrhs != 2) {
        mexPrintf("Unsupported number of input arguments: %d\n", nrhs);
        return;
    }
    if (nlhs > 1) {
        mexPrintf("Unsupported number of output arguments: %d\n", nrhs);
        return;
    }

    /* Check that input argument is a scalar. */
    if (mxGetNumberOfElements(prhs[0]) != 1 || !mxIsDouble(prhs[0])) {
        mexPrintf("Input must be a scalar of type double.\n");
        return;
    }

    addr = (DWORD)mxGetScalar(prhs[0]);
    data_in = (UINT32)mxGetScalar(prhs[1]);

    /* Validate DWORD-aligned address.*/
    if (addr % 4 != 0) {
        mexPrintf("Address isn\'t 4-byte aligned.\n");
        return;
    }

    /* Initialize the VC709 library */
    dwStatus = VC709_LibInit();
    if (dwStatus != WD_STATUS_SUCCESS) {
        mexPrintf("Failed to initialize the VC709 library: %s\n", VC709_GetLastErr());
        return;
    }

    /* Find and open a VC709 device (by default ID) */
    if (VC709_DEFAULT_VENDOR_ID != 0) {
        hDev = DeviceFindAndOpen(VC709_DEFAULT_VENDOR_ID, VC709_DEFAULT_DEVICE_ID);
    }

    if (hDev != NULL) {
        DeviceReadWriteData(hDev, WDC_WRITE, addr, &data_in);
        DeviceReadWriteData(hDev, WDC_READ, addr, &data_out);
    }

    plhs[0] = mxCreateDoubleScalar(data_out);

    /* Perform necessary cleanup before exiting the program */
    if (hDev != NULL) {
        DeviceClose(hDev);
    }

    dwStatus = VC709_LibUninit();
    if (dwStatus != WD_STATUS_SUCCESS) {
        mexPrintf("Failed to uninit the VC709 library: %s", VC709_GetLastErr());
        return;
    }
}


/* Find and open a VC709 device */
WDC_DEVICE_HANDLE DeviceFindAndOpen(DWORD dwVendorId, DWORD dwDeviceId) {
    WD_PCI_SLOT slot;

    if (!DeviceFind(dwVendorId, dwDeviceId, &slot)) {
        return NULL;
    }
    else {
        return DeviceOpen(&slot);
    }
}

/* Find a VC709 device */
BOOL DeviceFind(DWORD dwVendorId, DWORD dwDeviceId, WD_PCI_SLOT* pSlot) {
    DWORD dwStatus;
    WDC_PCI_SCAN_RESULT scanResult;

    if (dwVendorId == 0) {
        return FALSE;
    }

    BZERO(scanResult);
    dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId, &scanResult);
    if (dwStatus != WD_STATUS_SUCCESS) {
        mexPrintf("DeviceFind: Failed scanning the PCI bus. Error: 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        return FALSE;
    }

    if (scanResult.dwNumDevices == 0) {
        mexPrintf("No matching device was found for search criteria (Vendor ID 0x%lX, Device ID 0x%lX)\n", dwVendorId, dwDeviceId);
        return FALSE;
    }

    else if (scanResult.dwNumDevices > 1) {
        mexPrintf("Too many matching devices were found for search criteria (Vendor ID 0x%lX, Device ID 0x%lX)\n", dwVendorId, dwDeviceId);
        return FALSE;
    }
    else {
        *pSlot = scanResult.deviceSlot[0];
        return TRUE;
    }
}

/* Open a handle to a VC709 device */
WDC_DEVICE_HANDLE DeviceOpen(const WD_PCI_SLOT* pSlot) {
    WDC_DEVICE_HANDLE hDev;
    DWORD dwStatus;
    WD_PCI_CARD_INFO deviceInfo;

    /* Retrieve the device's resources information */
    BZERO(deviceInfo);
    deviceInfo.pciSlot = *pSlot;
    dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
    if (dwStatus != WD_STATUS_SUCCESS) {
        mexPrintf("DeviceOpen: Failed retrieving the device's resources information.\nError 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));
        return NULL;
    }

    /* NOTE: You can modify the device's resources information here, if
    necessary (mainly the deviceInfo.Card.Items array or the items number -
    deviceInfo.Card.dwItems) in order to register only some of the resources
    or register only a portion of a specific address space, for example. */

    /* Open a handle to the device */
    hDev = VC709_DeviceOpen(&deviceInfo);
    if (hDev == NULL) {
        mexPrintf("DeviceOpen: Failed opening a handle to the device: %s", VC709_GetLastErr());
        return NULL;
    }

    return hDev;
}

/* Close handle to a VC709 device */
void DeviceClose(WDC_DEVICE_HANDLE hDev) {
    if (hDev == NULL) {
        return;
    }

    if (!VC709_DeviceClose(hDev)) {
        mexPrintf("DeviceClose: Failed closing VC709 device: %s", VC709_GetLastErr());
    }
}

void DeviceReadWriteData(WDC_DEVICE_HANDLE hDev, WDC_DIRECTION direction, DWORD dwOffset, UINT32* data) {
    DWORD dwNumAddrSpaces;
    DWORD dwAddrSpace;
    WDC_ADDR_MODE mode = WDC_MODE_32; /* Default transfer size is 32 bit. */
    BOOL fBlock = FALSE; /* Default transfer mode is non-blocking transfers. */
    DWORD dwStatus;

    /* Initialize active address space (BAR)*/
    dwNumAddrSpaces = VC709_GetNumAddrSpaces(hDev);

    /* Find the first active address space */
    for (dwAddrSpace = 0; dwAddrSpace < dwNumAddrSpaces; ++dwAddrSpace) {
        if (WDC_AddrSpaceIsActive(hDev, dwAddrSpace)) {
            break;
        }
    }

    /* Sanity check */
    if (dwAddrSpace == dwNumAddrSpaces) {
        mexPrintf("MenuReadWriteAddr: Error - no active address spaces found\n");
        return;
    }

    if (direction == WDC_READ) {
        dwStatus = WDC_ReadAddr32(hDev, dwAddrSpace, dwOffset, data);
    }
    else {
        dwStatus = WDC_WriteAddr32(hDev, dwAddrSpace, dwOffset, *data);
    }

    if (dwStatus != WD_STATUS_SUCCESS) {
        mexPrintf("Failed to %s offset 0x%lX in BAR %ld. Error 0x%lx - %s\n", (WDC_READ == direction) ? "read from" : "write to", dwOffset, dwAddrSpace, dwStatus, Stat2Str(dwStatus));
        return;
    }
}
