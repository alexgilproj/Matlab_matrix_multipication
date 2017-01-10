#include <Windows.h>

#include "DriverInfoGen.h"

#include "DMATransfer.h"

void DMATransfer(LPVOID writeBuffer, ULONG writeBufferSize, LPVOID readBuffer, ULONG readBufferSize) {
	init((PUCHAR)writeBuffer, writeBufferSize, (PUCHAR)readBuffer, readBufferSize);
	startTestExternal();
	stopTestExternal();
	flush();
}
