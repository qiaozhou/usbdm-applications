#include <jni.h>
#include <stdio.h>
#include <string>
//#include <iconv.h>

#include "net_sourceforge_usbdm_connections_usbdm_Usbdm.h"
#include "USBDM_API.h"

/*
 * This file provides JNI wrappers for the following USBDM entry points:
 *
USBDM_API USBDM_ErrorCode  USBDM_Init(void);
USBDM_API USBDM_ErrorCode  USBDM_Exit(void);
USBDM_API USBDM_ErrorCode  USBDM_Open(unsigned char deviceNo);
USBDM_API USBDM_ErrorCode  USBDM_Close(void);
USBDM_API USBDM_ErrorCode  USBDM_FindDevices(unsigned int *deviceCount);
USBDM_API USBDM_ErrorCode  USBDM_ReleaseDevices(void);
USBDM_API USBDM_ErrorCode  USBDM_GetBDMDescription(const char **deviceDescription);
USBDM_API USBDM_ErrorCode  USBDM_GetBDMSerialNumber(const char **deviceDescription);
*/

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_init(JNIEnv *, jclass) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_init()\n");
	return USBDM_Init();
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    exit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_exit(JNIEnv *, jclass) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_exit()\n");
	return USBDM_Exit();
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    findDevices
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_findDevices(JNIEnv *env, jclass, jintArray countOfDevices) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_findDevices()\n");
	unsigned int deviceCount;
	USBDM_ErrorCode rc = USBDM_FindDevices(&deviceCount);
	if (rc == BDM_RC_OK) {
		jint jDeviceCount = deviceCount;
		env->SetIntArrayRegion(countOfDevices, 0, 1, &jDeviceCount);
	}
	return rc;
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    releaseDevices
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_releaseDevices(JNIEnv *, jclass) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_releaseDevices()\n");
	return USBDM_ReleaseDevices();
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    openBDM
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_openBDM(JNIEnv *, jclass, jint deviceNum) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_openBDM(%d)\n", (int)deviceNum);
	return USBDM_Open(deviceNum);
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    closeBDM
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_closeBDM(JNIEnv *, jclass) {
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_closeBDM()\n");
	return USBDM_Close();
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    getBDMDescription
 * Signature: ([C)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMDescription(JNIEnv *env, jclass, jbyteArray jString) {
	const char *stringUTF16LE;
	USBDM_ErrorCode rc = BDM_RC_OK;
	rc = USBDM_GetBDMDescription(&stringUTF16LE);
	if (rc == BDM_RC_OK) {
      int16_t *p = (int16_t *)stringUTF16LE;
      while (*p != 0) {
         p++;
      }
      jbyte jlen = (char*)p-stringUTF16LE;
      env->SetByteArrayRegion(jString, 0, 1, &jlen);
      env->SetByteArrayRegion(jString, 1, jlen, (const jbyte *)stringUTF16LE);
	}
	return rc;
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    getBDMSerialNumber
 * Signature: ([C)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMSerialNumber(JNIEnv *env, jclass, jbyteArray jString) {
   const char *stringUTF16LE;
   USBDM_ErrorCode rc = BDM_RC_OK;
   rc = USBDM_GetBDMSerialNumber(&stringUTF16LE);
   if (rc == BDM_RC_OK) {
      int16_t *p = (int16_t *)stringUTF16LE;
      while (*p != 0) {
         p++;
      }
      jbyte jlen = (char*)p-stringUTF16LE;
      env->SetByteArrayRegion(jString, 0, 1, &jlen);
      env->SetByteArrayRegion(jString, 1, jlen, (const jbyte *)stringUTF16LE);
   }
   return rc;
}

const char *fieldNames[] = {
	"BDMsoftwareVersion",   //!< BDM Firmware version
	"BDMhardwareVersion",   //!< Hardware version reported by BDM firmware
	"ICPsoftwareVersion",   //!< ICP Firmware version
	"ICPhardwareVersion",   //!< Hardware version reported by ICP firmware
	"capabilities",         //!< BDM Capabilities
	"commandBufferSize",    //!< Size of BDM Communication buffer
	"jtagBufferSize",       //!< Size of JTAG buffer (if supported)
};

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    getBDMFirmwareVersion
 * Signature: (Ledu/swin/mcu/debug/connections/usbdm/Usbdm/BdmInformation;)I
 */
JNIEXPORT jint JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion(JNIEnv *env, jclass objClass, jobject jbdmInfo) {
	USBDM_ErrorCode rc = BDM_RC_OK;
	USBDM_bdmInformation_t bdmInfo = {sizeof(USBDM_bdmInformation_t)};

	rc = USBDM_GetBdmInformation(&bdmInfo);
	if (rc != BDM_RC_OK)
		return rc;
//	fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion()\n");
	jclass cls = env->GetObjectClass(jbdmInfo);
   if (cls == NULL)
   	return BDM_RC_ILLEGAL_PARAMS;
//   fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion() #1\n");
   jint values[] = {bdmInfo.BDMsoftwareVersion,bdmInfo.BDMhardwareVersion,
						  bdmInfo.ICPsoftwareVersion,bdmInfo.ICPhardwareVersion,
   		           bdmInfo.capabilities,
   		           bdmInfo.commandBufferSize,
   		           bdmInfo.jtagBufferSize};
	if (rc == BDM_RC_OK) {
		for (unsigned indx=0; indx < sizeof(fieldNames)/sizeof(fieldNames[0]); indx++) {
//		   fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion() indx = %d\n", indx);
			jfieldID fieldID = env->GetFieldID(cls, fieldNames[indx], "I");
//         fprintf(stderr, "Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getBDMFirmwareVersion(): fieldID=%p\n", fieldID);
         if (fieldID == NULL)
         	return BDM_RC_ILLEGAL_PARAMS;
         env->SetIntField(jbdmInfo, fieldID, values[indx]);
		}
	}
	return rc;
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    getErrorString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getErrorString(JNIEnv *env, jclass, jint errorNum) {

	const char *message = USBDM_GetErrorString((USBDM_ErrorCode(errorNum)));

	return env->NewStringUTF(message);
}

/*
 * Class:     edu_swin_mcu_debug_connections_usbdm_Usbdm
 * Method:    getErrorString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_net_sourceforge_usbdm_connections_usbdm_Usbdm_getUsbdmPath(JNIEnv *env, jclass, jint errorNum) {

   const char *message = "Hello";

   return env->NewStringUTF(message);
}

extern "C"
void
//#ifdef __unix__
__attribute__ ((constructor))
//#endif
usbdm_dll_initialize(void) {
//   fprintf(stderr, "usbdm_dll_initialize()\n");
}

extern "C"
void
//#ifdef __unix__
__attribute__ ((destructor))
//#endif
usbdm_dll_uninitialize(void) {
//   fprintf(stderr, "usbdm_dll_uninitialize()\n");
}
