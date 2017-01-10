#ifndef __DRIVER_INFO_GEN__
#define __DRIVER_INFO_GEN__

void init(PUCHAR baseWriteBuffer, ULONG writeBufferSize, PUCHAR baseReadBuffer, ULONG readBufferSize);
int flush();
int startTestExternal();
int stopTestExternal();

#endif // __DRIVER_INFO_GEN__
