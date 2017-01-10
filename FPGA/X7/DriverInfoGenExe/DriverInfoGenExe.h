#ifndef __DRIVER_INFO_GEN_EXE__
#define __DRIVER_INFO_GEN_EXE__

void init(PUCHAR baseWriteBuffer, DWORD writeBufferSize, PUCHAR baseReadBuffer, DWORD readBufferSize);
int flush();
int startTestExternal();
int stopTestExternal();

#endif // __DRIVER_INFO_GEN_EXE__
