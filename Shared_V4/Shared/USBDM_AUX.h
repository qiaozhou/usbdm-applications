/*! \file
    \brief Header file for USBDM_AUX.cpp

*/
#ifndef _USBDM_AUX_H_
#define _USBDM_AUX_H_

#include <string>
#include <vector>
#include "USBDM_API.h"

//! Information about a BDM
class BdmInformation {
public:
   unsigned                deviceNumber;
   std::string             serialNumber;
   std::string             description;
   USBDM_ErrorCode         suitable;
   USBDM_bdmInformation_t  info;
   BdmInformation(unsigned      deviceNumber,
                  std::string   serialNumber,
                  std::string   description ) :
      deviceNumber(deviceNumber),
      serialNumber(serialNumber),
      description(description),
      suitable(BDM_RC_FAIL)
      {
      info.size              = sizeof(USBDM_bdmInformation_t);
      info.capabilities      = BDM_CAP_NONE;
      info.commandBufferSize = 0;
      info.jtagBufferSize    = 0;
   }
};

//! Create list of connected BDMs
//!
USBDM_ErrorCode USBDM_FindBDMs(TargetType_t targetType, std::vector<BdmInformation> &bdmInformation);


enum RetryMode {
   retryMask       = 0x0F,   // Mask for basic options
   retryAlways     = 0,      // Always retry - on error the user has already been informed.
   retryNever      = 1,      // Never retry - the user has not been informed of any error (quiet)
   retryNotPartial = 2,      // Don't retry on partial connection (BDM_RC_SECURED,BDM_RC_BDM_EN_FAILED)
   retryByReset   = (1<<4),  // Retry silently using reset if necessary (if supported by target to entry debug mode)
   retryByPower   = (1<<5),  // Retry by cycling BDM controlled power & prompting user
   retryWithInit  = (1<<6),  // Option for ARM & DSC - do DSC/ARM_Initialise() first
   retryWithReset = (1<<7),  // Immediately reset to special mode - useful to avoid Watchdog timeout
};

USBDM_ErrorCode USBDM_TargetConnectWithRetry(USBDMStatus_t *usbdmStatus, RetryMode retry=retryAlways);
inline USBDM_ErrorCode USBDM_TargetConnectWithRetry(RetryMode retry=retryAlways) {
   return USBDM_TargetConnectWithRetry(NULL, retry);
}

void milliSleep(int milliSeconds);

USBDM_ErrorCode getBDMStatus(USBDMStatus_t *usbdmStatus);

USBDM_ErrorCode USBDM_SetTargetTypeWithRetry(TargetType_t targetType);

USBDM_ErrorCode USBDM_GetBDMSerialNumber(std::string &serialNumber);
USBDM_ErrorCode USBDM_GetBDMDescription(std::string &description);

USBDM_ErrorCode USBDM_OpenBySerialNumber(TargetType_t targetType, const std::string &serialnumber);
USBDM_ErrorCode USBDM_OpenBySerialNumberWithRetry(TargetType_t targetType, const std::string &serialnumber);
USBDM_ErrorCode USBDM_SetOptionsWithRetry(USBDM_ExtendedOptions_t *bdmOptions);

USBDM_ErrorCode USBDM_GetBDMSerialNumber(std::string &serialNumber);
USBDM_ErrorCode USBDM_GetBDMDescription(std::string &description);

void runGuiEventLoop(void);

enum {
   OPTIONS_NO_CONFIG_DISPLAY = 0x00001,
};

#endif //_USBDM_AUX_H_
