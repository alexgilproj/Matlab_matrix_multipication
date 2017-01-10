/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_xilinx_gui_DrvStatus */

#ifndef _Included_com_xilinx_gui_DrvStatus
#define _Included_com_xilinx_gui_DrvStatus
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    getBoardNumber
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getBoardNumber
  (JNIEnv *, jobject);


/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    getMode
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getMode
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    getRawMode
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getRawMode
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    getTestConfig
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_xilinx_gui_DrvStatus_getTestConfig
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    setTestConfig
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setTestConfig
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    cmdDisable
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_cmdDisable
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    cmdEnable
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_cmdEnable
  (JNIEnv *, jobject, jint);
/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    setRawMode
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setRawMode
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     com_xilinx_gui_DrvStatus
 * Method:    setMode
 * Signature: (II)I
 */
JNIEXPORT void JNICALL Java_com_xilinx_gui_DrvStatus_setMode
  (JNIEnv *, jobject, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
