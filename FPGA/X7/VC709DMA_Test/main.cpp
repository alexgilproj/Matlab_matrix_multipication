#include <Windows.h>
#include <stdio.h>

#include "DMATransfer.h"

int main(int argc, char* argv[]) {
	//printf("Started main.\r\n");

	DWORD BUF_SIZE = 536870912;

	UCHAR* baseWriteBuffer = new UCHAR[BUF_SIZE];
	UCHAR* baseReadBuffer = new UCHAR[BUF_SIZE];

	for (unsigned int i = 0; i < BUF_SIZE; ++i) {
		baseWriteBuffer[i] = i % 256;
	}
	for (unsigned int i = 0; i < BUF_SIZE; ++i) {
		baseReadBuffer[i] = 0x00;
	}

	for (int bla = 0; bla < 100; ++bla) {
		DMATransfer(baseWriteBuffer, BUF_SIZE, baseReadBuffer, BUF_SIZE);
	}

	delete[] baseWriteBuffer;
	delete[] baseReadBuffer;

	return 0;
}
