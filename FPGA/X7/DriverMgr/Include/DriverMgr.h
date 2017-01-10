//
// DriverMgr.h : Include file for the entry points for this Library.
//
// DriverMgr implements a subset of Device Manger functions such as:
//		Driver Disable,
//		Driver Enable,
//		Driver Status,
//		Rescan
//
//  by Pro Code Works, LLC.
//  -- http://www.ProCodeWorks.com
//

#define	MAX_NUMBER_DEVICES_SUPPORTED		32

#define	DEVICE_STATUS_NOT_FOUND				0
#define DEVICE_STATUS_PHANTOM				1
#define	DEVICE_STATUS_DISABLED				2
#define DEVICE_STATUS_ENABLED				3
#define	DEVICE_STATUS_HAS_PROBLEM			4
#define DEVICE_STATUS_HAS_PRIVATE_PROBLEM	5
#define DEVICE_STATUS_STARTED				6
#define DEVICE_STATUS_NOT_STARTED			7


typedef struct _DEV_STATUS {
	int		Instance;
    int		DriverStatus;
} DEV_STATUS, *PDEV_STATUS;

typedef struct _DEV_STATUS_LIST {
	int		DevStatusSize;
	int		ReturnNumDevices;
	DEV_STATUS	DevStats[MAX_NUMBER_DEVICES_SUPPORTED];
} DEV_STATUS_LIST, *PDEV_STATUS_LIST;

//
// DevTypes
//
#define	DEV_TYPE_XDMA	0
#define	DEV_TYPE_XBLOCK	1
#define	DEV_TYPE_XNET	2


#define	CHILD_DRIVER_CONFIG_MIX				0
#define	CHILD_DRIVER_CONFIG_XBLOCK			1
#define	CHILD_DRIVER_CONFIG_2_XNET			2

#define	RAW_ETHERNET_OFF					0
#define	RAW_ETHERNET_ON						1

#define	TEST_CONFIG_NORMAL_OPERATION		0
#define	TEST_CONFIG_INTERNAL_BUFFER			1

//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

int cmdStatus(int BoardNumber, int Device, PDEV_STATUS_LIST pDevStatus);
int cmdEnable(int BoardNumber, int Device);
int cmdDisable(int BoardNumber, int Device);
int cmdRescan();
long GetDriverChildConfig(int BoardNumber);
long SetDriverChildConfig(int BoardNumber, long DriverChildConfig);
long GetRawEthernet(int BoardNumber);
long SetRawEthernet(int BoardNumber, long RawEthernet);
long GetTestConfig(int BoardNumber);
long SetTestConfig(int BoardNumber,	long modeConfig);







