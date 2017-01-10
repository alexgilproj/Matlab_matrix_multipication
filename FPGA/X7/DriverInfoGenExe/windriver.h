#pragma once

#include <windows.h>
#include <setupapi.h>

#include <stdio.h>
#include <tchar.h>

#include <stdlib.h>
#include <malloc.h>
#ifndef CTL_CODE
#include <winioctl.h>
#endif // Not Debug version.

#include "../Include/public.h"
#include "../Include/xpmon_be.h"

#define	MAX_DEVICE_PATH_LENGTH	256
#define	MAX_APP_NAME_SIZE		16

/*  NOTE: 32768 is the largest size the Packet Generator can handle */
#ifdef RAW_ETH
#define DEFAULT_BUFFER_SIZE		16383
#define MAXIMUM_BUFFER_SIZE		16383
#else
#define DEFAULT_BUFFER_SIZE		32768
#define MAXIMUM_BUFFER_SIZE		32768
#endif

#define NUM_CORES_TO_LEAVE       0

#define MAX_EVENTS_PUSHED        100
#define STACKED_EVENTS_TO_CLEAR  50

#define MIN_PACKETS_IN_BUFFER   1000     
#define DEFAULT_THREAD_COUNT	2

#define     APP0_WRITE_DMA_ENGINE	0
#define     APP0_READ_DMA_ENGINE	4

#define     APP1_WRITE_DMA_ENGINE	1
#define     APP1_READ_DMA_ENGINE	5

#define     APP2_WRITE_DMA_ENGINE   2
#define     APP2_READ_DMA_ENGINE    6

#define     APP3_WRITE_DMA_ENGINE   3
#define     APP3_READ_DMA_ENGINE    7

#define	MAX_NUMBER_APPS				4
#define	DEFAULT_APP					1

#define	NUMBER_CONCURRENT_READS		10

/*
typedef struct _APP_TABLE_ENTRY
{
    unsigned char	WRITE_ENGINE;
    unsigned char	READ_ENGINE;
}APP_TABLE_ENTRY, *PAPP_TABLE_ENTRY;

static const APP_TABLE_ENTRY APP_TABLE[MAX_NUMBER_APPS] =
{
	{APP0_WRITE_DMA_ENGINE,	APP0_READ_DMA_ENGINE},
	{APP1_WRITE_DMA_ENGINE,	APP1_READ_DMA_ENGINE},
	{APP2_WRITE_DMA_ENGINE, APP2_READ_DMA_ENGINE},
	{APP3_WRITE_DMA_ENGINE, APP3_READ_DMA_ENGINE}
};
*/

typedef struct _THREAD_CONTEXT
{
    HANDLE  hBlockDevice;
    ULONG   BufferSize;
    PUCHAR  Buffer;
	ULONGLONG	BytesXfered;
	int     engine;
	int     mode;
    BOOL    quiet;
    BOOL    debug;
	BOOL    ExitOnError;
	BOOL    ThreadDone;
	BOOL    WaitThreadDone;
	OVERLAPPED os[MAX_EVENTS_PUSHED];
	int		ThreadNumber;
	int     osIndexTail;
	int     osIndexHead;
	int     osIndexFree;
	HANDLE  hEvent[MAX_EVENTS_PUSHED];
	int     BufferAvailable;
	int     MaxBufferAvailable;
	int     ChunkSize;
	BOOL    ContextUsed;
	CRITICAL_SECTION cs;
} THREAD_CONTEXT, *PTHREAD_CONTEXT;


// I/O Request element
typedef struct _IOREQ {
	// Overlapped MUST be the first element in this structure, Do Not Change!
    OVERLAPPED os;                      // OVERLAPPED structure for the operation
	// Overlapped MUST be the first element in this structure, Do Not Change!
    HANDLE  hBlockDevice;
	ULONGLONG	BytesXfered;
    PUCHAR	pBuf;						// pointer to data buffer
    DWORD   dwSize;                     // size of the read/buffer
	DWORD	Flags;						// Flags
} IOREQ, * PIOREQ;

#define	FLAGS_BUFFER_IN_USE			0x0001
#define FLAGS_BUFFER_READ			0x0000
#define FLAGS_BUFFER_WRITE			0x0000
#define	FLAGS_BUFFER_SHUTDOWN		0x8000

BOOL    Quiet;
BOOL    Debug;
BOOL    ExitOnError;
ULONG   Status;
ULONG   TestTime;
int		AppNumber;
BOOL	bVerbose;
int		ThreadCount0;
int		ThreadCount1;
int     NumThreads0;
int     NumThreads1;
int		ProcessorCount;
ULONG   ReadBufferSize;
ULONG   WriteBufferSize;
ULONG   myReadBufferSize;
ULONG   myWriteBufferSize;

int     ContextCount;
int     ContextCount0;
int     ContextCount1;

int		ChildDriverConfig;
BOOL	bRawEthernetMode;
BOOL	bInternalXferMode;

HANDLE  hDMADevice;
HANDLE  hBlockDevice0;
HANDLE  hBlockDevice1;

__field_bcount(ReadBufferSize) PUCHAR ReadBuffer;
__field_bcount(WriteBufferSize) PUCHAR WriteBuffer;
	
__field_bcount(WriteBufferSize) PUCHAR ThreadBuffer0[DEFAULT_THREAD_COUNT + 1];
__field_bcount(WriteBufferSize) PUCHAR ThreadBuffer1[DEFAULT_THREAD_COUNT + 1];

__field_ecount(ThreadCount) HANDLE *Threads0;
__field_ecount(ThreadCount) HANDLE *WaitThreads0;
__field_ecount(ThreadCount) HANDLE *Threads1;
__field_ecount(ThreadCount) HANDLE *WaitThreads1;
__field_ecount(ThreadCount) PTHREAD_CONTEXT Contexts;
__field_ecount(ThreadCount) PTHREAD_CONTEXT Contexts0;
__field_ecount(ThreadCount) PTHREAD_CONTEXT Contexts1;
__field_ecount(ThreadCount) HANDLE *EHandles;


 CRITICAL_SECTION CriticalSection;
 BOOL    CSInitialized;


HANDLE OpenDriverInterface(INT DriverIndex, INT DriverNumber);
DWORD SetTestParams(DWORD TestMode, DWORD PacketSize, DWORD Engine);
BOOL startTest(int eng, int m, int size);
BOOL stopTest(int eng, int m, int size);
void waitThreadsStop(int engine);
DWORD WINAPI WriteThreadProc(LPVOID lpParameter);
DWORD WINAPI ReadThreadProc(LPVOID lpParameter);
DWORD WINAPI WriteWaitThreadProc(LPVOID lpParameter);
DWORD WINAPI ReadWaitThreadProc(LPVOID lpParameter);
BOOL ThreadedWriteTest(int Engine,int size, int mode);
BOOL ThreadedReadTest(int Engine,int size, int mode);
BOOL ThreadedReadWriteTest(int Engine,int size, int mode);
BOOL MyThreadedReadWriteTest(int Engine, int size, int mode);
DWORD DrainHardwareFifo(INT Engine,PUCHAR buffer,INT size);
BOOL XlxCancelTransfers(int eng);
void FormatBuffer(unsigned char *buf,int bufferSize,int chunkSize,int pktSize,BOOL bRawEthernetMode);

int g_TimeUp0 = 0;
int g_TimeUp1 = 0;
int g_TimeUp0_1 = 0;
int g_TimeUp1_0 = 0;

int mode;
int packetsize;

