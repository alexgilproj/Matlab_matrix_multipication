#include <stdio.h>
#include <Windows.h>

#include "DriverInfoGenExe.h"

int main() {
	//printf("Started main.\r\n");

	DWORD BUF_SIZE = 536870912;

	UCHAR* baseWriteBuffer = new UCHAR[BUF_SIZE];
	UCHAR* baseReadBuffer = new UCHAR[BUF_SIZE];

	for (int bla = 0; bla < 100; ++bla) {

		//for (unsigned int i = 0; i < BUF_SIZE; ++i) {
		//	baseWriteBuffer[i] = i % 256;
		//}
		//for (unsigned int i = 0; i < BUF_SIZE; ++i) {
		//	baseReadBuffer[i] = 0x00;
		//}

		//printf("Data at base write is: %X\r\n", ((DWORD*)baseWriteBuffer)[8388607]);
		//printf("Data at base read is: %X\r\n", ((DWORD*)baseReadBuffer)[8388607]);


		init(baseWriteBuffer, BUF_SIZE, baseReadBuffer, BUF_SIZE);

		startTestExternal();

		//Sleep(2000);

		stopTestExternal();
		//printf("Data at base write is: %X\r\n", ((DWORD*)baseWriteBuffer)[8388607]);
		//printf("Data at base read is: %X\r\n", ((DWORD*)baseReadBuffer)[8388607]);

		flush();
	}

	delete[] baseWriteBuffer;
	delete[] baseReadBuffer;

	return 0;
}
