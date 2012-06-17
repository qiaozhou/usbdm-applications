/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class net_sourceforge_usbdm_connections_usbdm_Usbdm */

#ifndef _Included_net_sourceforge_usbdm_connections_usbdm_Usbdm
#define _Included_net_sourceforge_usbdm_connections_usbdm_Usbdm
#ifdef __cplusplus
extern "C" {
#endif
#undef net_sourceforge_usbdm_connections_usbdm_Usbdm_BDM_RC_OK
#define net_sourceforge_usbdm_connections_usbdm_Usbdm_BDM_RC_OK 0L
/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_init
  (JNIEnv *, jclass);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    exit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_exit
  (JNIEnv *, jclass);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    findDevices
 * Signature: ([I)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_findDevices
  (JNIEnv *, jclass, jintArray);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    releaseDevices
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_releaseDevices
  (JNIEnv *, jclass);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    openBDM
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_openBDM
  (JNIEnv *, jclass, jint);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    closeBDM
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_closeBDM
  (JNIEnv *, jclass);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    getBDMDescription
 * Signature: ([C)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMDescription
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    getBDMSerialNumber
 * Signature: ([C)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMSerialNumber
  (JNIEnv *, jclass, jbyteArray);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    getBDMFirmwareVersion
 * Signature: (Lnet/sourceforge/usbdm/connections/usbdm/Usbdm/BdmInformation;)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion
  (JNIEnv *, jclass, jobject);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    getErrorString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getErrorString
  (JNIEnv *, jclass, jint);

/*
 * Class:     net_sourceforge_usbdm_connections_usbdm_Usbdm
 * Method:    getBDMFirmwareVersion
 * Signature: (Lnet/sourceforge/usbdm/connections/usbdm/Usbdm/BdmInformation;)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getUsbdmPath
  (JNIEnv *, jclass, jobject);

#ifdef __cplusplus
}
#endif
#endif
