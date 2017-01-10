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
 ** \   \  /  \   Virtex-7 FPGA XT Connectivity Targeted Reference Design
 **  \___\/\___\
 ** 
 **  Device: xc7v690t
 **  Version: 1.0
 **  Reference: UG962
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
 * @file threads.cpp 
 *
 * Author: Xilinx, Inc.
 *
 * 2007-2010 (c) Xilinx, Inc. This file is licensed uner the terms of the GNU
 * General Public License version 2.1. This program is licensed "as is" without
 * any warranty of any kind, whether express or implied.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0  5/15/12  First release
 *
 *****************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <xpmon_be.h>

#ifdef DEBUG_VERBOSE /* Enable both normal and verbose logging */
#define log_verbose(args...)    printf(args)
#else
#define log_verbose(x...)
#endif


#define MIN_PKTSIZE 64
#define MAX_PKTSIZE (32*1024)

#define MAX_THREADS 8

#define PAGE_SIZE 4096
#define BUFFER_SIZE (PAGE_SIZE * 2048 * 2)

#define APP_SOCK_PATH "./App/xt_app_comm_socket"
#define GUI_SOCK_PATH "./gui/jnilib/src/xt_gui_comm_socket"
#define CHECK_INTERVAL 20
#define STAT_FILE_CHK_CMD "/usr/sbin/lsof /dev/xdma_stat 2>&1"

static char Flag_first;

unsigned int ErrCnt0=0;
unsigned int ErrCnt1=0;
unsigned int ErrCnt2=0;
unsigned int ErrCnt3=0;

TestCmd * testCmd1;
TestCmd * testCmd2;

TestCmd testcmd1,testcmd2;

TestCmd  *localCmd2;

typedef struct DataVerifyStatus
{
	int engine;
	int ErrCnt;
}DataVerifyComm_t;



int xlnx_thread_create(void *fp(void *),void *data);
void *Recv_Data_Verify_status(void *p);

int sendTestCmd(TestCmd testCmd);

int getErrorCount0(){
	return ErrCnt0;
}

int getErrorCount1(){
	return ErrCnt1;
}
int getErrorCount2(){
	return ErrCnt2;
}
int getErrorCount3(){
	return ErrCnt3;
}

/* Stop the running test */
int StopTest(int statsfd, int engine, int testmode, int maxSize)
{
	int retval=-1;
	TestCmd testCmd;

	testCmd.Engine = engine;
	testCmd.TestMode = testmode;
	testCmd.MaxPktSize = maxSize;

	testCmd.TestMode &= ~(TEST_START);
	log_verbose("mode is %x\n", testCmd.TestMode);

	if(ioctl(statsfd, ISTOP_TEST, &testCmd) != 0)
	{
		log_verbose("STOP_TEST on engine %d failed\n", engine);
		return -1;
	}

	sendTestCmd(testCmd);
        sleep(3);
	retval = 0;
	return retval;
}

int StartTest(int statsfd, int engine, int testmode, int maxSize)
{


	int retval=-1;
	TestCmd testCmd;
	int i, ret0,ret1,ret2,ret3,ret4;

	if(Flag_first == 0)
	{
		xlnx_thread_create(&Recv_Data_Verify_status, NULL);
		Flag_first++;
	} 

	testCmd.Engine = engine;
	testCmd.TestMode = testmode;
	testCmd.MaxPktSize = maxSize; 

	testCmd.TestMode |= TEST_START;
#ifdef RAW_ETH 
	testCmd.TestMode |= ENABLE_CRISCROSS;
#endif


	if(engine == 0)
	{
		ErrCnt0 = 0;
	}
	else
	{
		ErrCnt1 = 0;
	}

	log_verbose("mode is %x engine is %d\n", testCmd.TestMode,testCmd.Engine);

	log_verbose("statsfd %d", statsfd); 
	retval = ioctl(statsfd, ISTART_TEST, &testCmd);
	if(retval != 0)
	{
		printf("Error"); 
		return -1;
	}

	sendTestCmd(testCmd);
	retval = 0;
	//msg_info("Test Started\n");
	return retval;
}

int xlnx_thread_create(void *fp(void *),void *data)
{
	pthread_attr_t        attr;
	pthread_t             thread;
	int                   rc=0;

	rc = pthread_attr_init(&attr);
	if(rc != 0)
	{
		printf("pthread_attr_init() returns error: %s\n",strerror(errno));
		return -1;
	}
	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(rc != 0)
	{
		printf("pthread_attr_setdetachstate() returns error: %s\n",strerror(errno));
		return -1;
	}
	rc = pthread_create(&thread, &attr,fp, data);
	if(rc != 0)
	{
		printf("pthread_create() returns error: %s\n",strerror(errno));
		return -1;
	}
	return 0;
}

int sendTestCmd(TestCmd testCmd)
{
	int s,ret;
	struct sockaddr_un remote;

	if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path,APP_SOCK_PATH);

	ret = sendto(s, &testCmd, sizeof(testCmd), 0, 
			(struct sockaddr*)&remote, sizeof(remote));

	if(ret < 0)
		perror("sendto()");

	close(s);

	return ret;
}

void *Recv_Data_Verify_status(void *p)
{
	int s, t, len, flags;
	int sret,n;
	struct sockaddr_un local, remote;
	fd_set read_set;
	struct timeval  timeout;
	DataVerifyComm_t dcomm;

	if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) 
	{
		perror("socket");
		exit(1);
	}

	flags = fcntl(s,F_GETFL,0); 
	fcntl(s,F_SETFL,flags | O_NONBLOCK);

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, GUI_SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	if (bind(s, (struct sockaddr *)&local, len) == -1) 
	{
		perror("bind");
		exit(1);
	}



	t = sizeof(remote);

	for(;;) 
	{
		FD_ZERO(&read_set);
		FD_SET(s, &read_set);

		timeout.tv_sec  = CHECK_INTERVAL;
		timeout.tv_usec = 0;

		sret = select(s + 1, &read_set, NULL, NULL, &timeout);

		if(sret < 0)
		{
			perror("select");
			break;
		}
		if (FD_ISSET(s, &read_set))
		{
			if ((n = recvfrom(s, &dcomm, sizeof(dcomm), 0,
							(struct sockaddr *)&remote, (socklen_t *)&t)) == -1) 
			{
				perror("recvfrom");
				continue ;
			}
#if DATA_VERIFY
			printf("Received Data verify mismatch for engine %d\n",dcomm.engine);
			if(dcomm.engine == 0)
				ErrCnt0 = dcomm.ErrCnt;
			else if (dcomm.engine == 1)
				ErrCnt1 = dcomm.ErrCnt;
			else if (dcomm.engine == 2)
				ErrCnt2 = dcomm.ErrCnt;
			else if (dcomm.engine == 3)
				ErrCnt3 = dcomm.ErrCnt;
#endif

		}
	}

	close(s); 
	return NULL;
}
