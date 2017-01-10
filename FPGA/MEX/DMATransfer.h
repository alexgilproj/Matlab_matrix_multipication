#pragma once

#ifndef __DMA_TRANSFER_H__
#define __DMA_TRANSFER_H__

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	void DMATransfer(LPVOID writeBuffer, ULONG writeBufferSize, LPVOID readBuffer, ULONG readBufferSize);

#ifdef __cplusplus
}
#endif

#endif // __DMA_TRANSFER_H__
