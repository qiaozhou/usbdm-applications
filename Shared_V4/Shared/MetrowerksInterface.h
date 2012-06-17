/*
 * MetrowerksInterface.h
 *
 *  Created on: 20/11/2010
 *      Author: podonoghue
 */

#ifndef METROWERKSINTERFACE_H_
#define METROWERKSINTERFACE_H_
class DeviceData;

DiReturnT getAttribute(const char *key, int *value, int defaultValue);
USBDM_ErrorCode getDeviceData(DeviceData &deviceData);
USBDM_ErrorCode loadSettings(TargetType_t             targetType,
                             USBDM_ExtendedOptions_t 	&bdmOptions,
									  std::string              &bdmSerialNumber
									  );
// Unused
DiReturnT mtwksDisplayLine(const char *format, ...);

/*! \brief Informs Codewarrior of MEE ID
 *
 *  @param dnExecId ID to use
 */
DiReturnT mtwksSetMEE(DiUInt32T dnExecId);

void setCallback( DiCallbackT dcCallbackType, CallbackF Callback );

DiReturnT loadNames(std::string &deviceName, std::string &projectName);

#endif // METROWERKSINTERFACE_H_
