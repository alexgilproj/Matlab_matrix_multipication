#ifndef __DRIVER_INFO_GEN_EXE__
#define __DRIVER_INFO_GEN_EXE__

void init();
int flush();
int startTestExternal(int eng, int testmode, int maxsize);
int stopTestExternal(int eng, int testmode, int maxsize);

#endif // __DRIVER_INFO_GEN_EXE__
