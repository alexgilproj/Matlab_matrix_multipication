#ifndef _VERSION_H_
#define _VERSION_H_

#define VER_MAJOR_NUM               1
#define VER_MINOR_NUM               3
#define VER_SUBMINOR_NUM			05
#define VER_BUILD_NUM               0005

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VER_MAJOR_NUM_STR       STRINGIZE(VER_MAJOR_NUM)
#define VER_MINOR_NUM_STR       STRINGIZE(VER_MINOR_NUM)
#define VER_SUBMINOR_NUM_STR    STRINGIZE(VER_SUBMINOR_NUM)
#define VER_BUILD_NUM_STR       STRINGIZE(VER_BUILD_NUM)

#define VER_PRODUCTVERSION			VER_MAJOR_NUM, VER_MINOR_NUM, VER_SUBMINOR_NUM, VER_BUILD_NUM
#define VER_PRODUCTVERSION_STR		VER_MAJOR_NUM_STR "." VER_MINOR_NUM_STR "." VER_SUBMINOR_NUM_STR "." VER_BUILD_NUM_STR

#define VER_LEGALCOPYRIGHT_YEARS    "2012-2013"
#define VER_COMPANYNAME_STR         "Xilinx Incorporated"
#define VER_LEGALCOPYRIGHT_STR      "Copyright (C) " VER_LEGALCOPYRIGHT_YEARS " " VER_COMPANYNAME_STR

#endif
