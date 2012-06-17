/*! \file
    \brief Header file for USBDM_API.c

*/
#ifndef _USBDM_AUX_H_
#define _USBDM_AUX_H_

USBDM_ErrorCode USBDM_TargetConnectWithRetry(USBDMStatus_t *usbdmStatus, wxWindow *parent=NULL);
inline USBDM_ErrorCode USBDM_TargetConnectWithRetry(wxWindow *parent=NULL) {
   return USBDM_TargetConnectWithRetry(NULL, parent);
}
USBDM_ErrorCode getBDMStatus(USBDMStatus_t *usbdmStatus, wxWindow *parent=NULL);
USBDM_ErrorCode USBDM_OpenTargetWithConfig(TargetType_t target_type);
USBDM_ErrorCode USBDM_GetBDMSerialNumber(wxString &serialNumber);
USBDM_ErrorCode USBDM_GetBDMDescription(wxString &description);

enum {
   OPTIONS_NO_CONFIG_DISPLAY = 0x00001,
};

#endif //_USBDM_AUX_H_
