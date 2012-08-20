/*! \file
   \brief Utility Routines for Metrowerks (eclipse)

   MetrowerksInterface.cpp

   \verbatim
   Copyright (C) 2008  Peter O'Donoghue

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim

\verbatim
 Change History
+===========================================================================================
| Jul 16 2011 | Corrected errors in Codewarrior keys                              - pgo V4.7
| Feb 26 2011 | Changes for Eclipse 10.1 (handling of default trim)               - pgo V4.4
| Nov 10 1010 | 4.?.? Created                                                     - pgo
+===========================================================================================
\endverbatim
*/

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <stdlib.h>
using namespace std;

#include "Common.h"
#include "ApplicationFiles.h"
#include "GDI.h"
#include "GDI_Aux.h"
#include "Names.h"
#include "USBDM_AUX.h"
#include "Log.h"
#include "USBDM_API.h"
#include "USBDM_API_Private.h"
#include "Metrowerks.h"
#include "MetrowerksInterface.h"
#include "DeviceData.h"
#include "wxPlugin.h"

const string emptyString("");
DiReturnT setErrorState(DiReturnT errorCode, const char *errorString = NULL);

static CallbackF metrowerksCallbackFunction;
static CallbackF metrowerksFeedbackFunction;

//! Key to use to obtain selected processor
#if TARGET == RS08
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.RS08 Debugger.processor"));
#elif TARGET == HCS08
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.HC08 Debugger.processor"));
#elif TARGET == HCS12
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.S12Z Debugger.processor"));
#elif TARGET == CFV1
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.CF Debugger.processor"));
#elif TARGET == CFVx
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.CF Debugger.processor"));
#elif (TARGET == ARM) || (TARGET == ARM_SWD)
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.ARM Debugger.processor"));
#elif TARGET == MC56F80xx
const string processorKey(("com.freescale.cdt.debug.cw.CW_SHADOWED_PREF.DSC Debugger.processor"));
#endif
//! Key to use to obtain project name
//const string projectkey(("org.eclipse.cdt.launch.PROJECT_ATTR"));
//! Key to use to obtain project home directory
//const string homeKey(("com.freescale.cdt.debug.cw.core.settings.GdiConnection.Generic.eclipseHome"));
//! Key to use to obtain USBDM options from Codewarrior
static const string baseKey       = ("net.sourceforge.usbdm.connections.usbdm.");
#if TARGET == CFVx
static const string suffixKeyCfvx = ("cfvx.");
#endif

static const string KeyDefaultBdmSerialNumber    = "defaultBdmSerialNumber";
static const string KeySetTargetVdd              = "setTargetVdd";
static const string KeyCycleTargetVddOnReset     = "cycleTargetVddOnReset";
static const string KeyCycleTargetVddOnConnect   = "cycleTargetVddOnConnect";
static const string KeyLeaveTargetPowered        = "leaveTargetPowered";
static const string KeyAutomaticReconnect        = "automaticReconnect";
static const string KeyUsePSTSignals             = "usePSTSignals";
static const string KeyConnectionSpeed           = "connectionSpeed";
static const string KeyClockTrimNVAddress        = "clockTrimNVAddress";
static const string KeyClockTrimFrequency        = "clockTrimFrequency";
static const string KeyUseResetSignal            = "useResetSignal";
static const string KeyTrimTargetClock           = "trimTargetClock";
static const string KeyUseAltBDMClock            = "useAltBDMClock";
//static const string KeyTargetClockFrequency      = "targetClockFrequency"; // HCS12 may need these
//static const string KeyGuessSpeedIfNoSYNC        = "guessSpeedIfNoSYNC";
static const string KeyMaskInterrupt             = "maskInterrupt";
static const string KeyEraseMethod               = "eraseMethod";

static const string KeyPowerOffDuration          = "powerOffDuration";
static const string KeyPowerOnRecoveryInterval   = "powerOnRecoveryInterval";
static const string KeyResetDuration             = "resetDuration";
static const string KeyResetReleaseInterval      = "resetReleaseInterval";
static const string KeyResetRecoveryInterval     = "resetRecoveryInterval";

struct ProjectAccess_t {
   const char * section;
   int          mode;       // 0=>R, 1 =>W
   const char * entry;
   char       **buffer;
   int          bufLength;
};

void setCallback( DiCallbackT dcCallbackType,
					   CallbackF   Callback ) {

	switch (dcCallbackType) {
		case DI_CB_MTWKS_EXTENSION :
		      print("setCallback(DI_CB_MTWKS_EXTENSION)\n");
				metrowerksCallbackFunction = Callback;
				break;
      case DI_CB_FEEDBACK :
            print("setCallback(DI_CB_FEEDBACK)\n");
            metrowerksFeedbackFunction = Callback;
            break;
      default :
            print("setCallback(Unknown = %d)\n", dcCallbackType);
            break;
	}
}

#define MAX_MTWKS_DISPLAY_STRING_LENGTH (2000)
static char mtwksDisplayLineBuffer[MAX_MTWKS_DISPLAY_STRING_LENGTH];

/*! \brief Informs Codewarrior of MEE ID
 *
 *  @param dnExecId ID to use
 */
DiReturnT mtwksSetMEE(DiUInt32T dnExecId) {
  DiReturnT rc;
  DiUInt32T meeValue = dnExecId;
  if (metrowerksCallbackFunction == NULL) {
     print("mtwksGetStringValue() - callback not set\n");
     return setErrorState(DI_ERR_NONFATAL, ("Callback function not set"));
  }

  metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION,
                             MTWKS_CB_SETMEEID,
                             &meeValue, &rc);
  return DI_OK;
}

 /*! \brief Writes a value to the Metrowerks status line
  *
  *  @param format Format and parameters as for printf()
  */
DiReturnT mtwksDisplayLine(const char *format, ...) {
   DiReturnT rc;

   va_list list;

   CallbackF callbackFunction = metrowerksFeedbackFunction;

   if (callbackFunction == NULL) {
      callbackFunction = metrowerksCallbackFunction;
   }

   if (callbackFunction == NULL) {
      print("mtwksGetStringValue() - callback not set\n");
      return setErrorState(DI_ERR_NONFATAL, ("Callback function not set"));
   }

   if (format == NULL) {
      strcpy(mtwksDisplayLineBuffer, "Empty\n");
   }
   else {
      va_start(list, format);
      vsnprintf(mtwksDisplayLineBuffer, MAX_MTWKS_DISPLAY_STRING_LENGTH, format, list);
      va_end(list);
   }
   print("mtwksDisplayLine(%s)\n", mtwksDisplayLineBuffer);
   callbackFunction(DI_CB_MTWKS_EXTENSION,
                    MTWKS_CB_DISPLAYLINE,
                    mtwksDisplayLineBuffer, &rc);
   return DI_OK;
}

#if !defined(LEGACY)
static DiReturnT mtwksGetStringValue( const string &entry, string &value) {

char             buffer[300];
char            *bufPtr = buffer;
ProjectAccess_t  deviceTypeAccess = {
	"",	            // section
   0,          		// 0=>R, 1=>W
   entry.c_str(),    // entry (=key)
   &bufPtr,    		// **value
   sizeof(buffer),  	// length of string buffer (incl. '\0')
};
DiReturnT rc;

   // Clear buffer for return value
   buffer[0] = '\0';
//   strcpy(buffer, "No value");

   // Assume failure
   value = string("");

   if (metrowerksCallbackFunction == NULL) {
      print("mtwksGetStringValue() - callback not set\n");
      return setErrorState(DI_ERR_NONFATAL, ("Callback function not set"));
   }

//   print("Callback(MTWKS_CB_PROJECTACCESS, Section: %s, Entry: %s)\n",
//         deviceTypeAccess.section,
//         deviceTypeAccess.entry);

   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION,
                              MTWKS_CB_PROJECTACCESS,
                              &deviceTypeAccess, &rc);
//   print("Callback(MTWKS_CB_PROJECTACCESS, Section: %s, Entry: %s, %c) => %s\n",
//         deviceTypeAccess.section,
//         deviceTypeAccess.entry,
//         (deviceTypeAccess.mode&0x01)?'W':'R',
//         getGDIErrorString(rc));
//   print("     Buffer = \"%s\"\n", *deviceTypeAccess.buffer);
//   print("====================================================\n");

   if (rc == DI_OK) {
   	value = string(buffer);
   }
   return setErrorState(rc);
}

//!  Loads int target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, int &value, int defaultValue) {

	string sValue;
	string keyBuffer(baseKey);
#if  TARGET==CFVx
   keyBuffer += suffixKeyCfvx;
#endif
   keyBuffer += key;

   value = defaultValue;
	if (mtwksGetStringValue(keyBuffer.c_str(), sValue) != DI_OK) {
	   print("getAttribute(%s) => Failed\n", (const char *)key.c_str());
	   value = defaultValue;
		return BDM_RC_ILLEGAL_PARAMS;
	}
	if (sValue != emptyString) {
		long int temp = strtol(sValue.c_str(), NULL, 10);
//ToDo some kind of error checking on conversion
//		if () {
//		   print("getAttribute(%s) => %s, Failed int conversion\n", (const char *)key.ToAscii(), (const char *)sValue.ToAscii());
//			return BDM_RC_ILLEGAL_PARAMS;
//		}
		value = (int)temp;
	}
   print("getAttribute(%s) => %d\n", keyBuffer.c_str(), value);
	return BDM_RC_OK;
}

//!  Loads TargetVddSelect_t target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, TargetVddSelect_t &value, TargetVddSelect_t defaultValue) {
   int iValue;
   USBDM_ErrorCode rc = getAttribute(key, iValue, (int)defaultValue);
   value = (TargetVddSelect_t) iValue;
   return rc;
}

//!  Loads ClkSwValues_t target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, ClkSwValues_t &value, ClkSwValues_t defaultValue) {
   int iValue;
   USBDM_ErrorCode rc = getAttribute(key, iValue, (int)defaultValue);
   value = (ClkSwValues_t) iValue;
   return rc;
}

//!  Loads AutoConnect_t target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, AutoConnect_t &value, AutoConnect_t defaultValue) {
   int iValue;
   USBDM_ErrorCode rc = getAttribute(key, iValue, (int)defaultValue);
   value = (AutoConnect_t) iValue;
   return rc;
}

//!  Loads bool target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, bool &value, bool defaultValue) {
   int iValue;
   USBDM_ErrorCode rc = getAttribute(key, iValue, (int)defaultValue);
   value = (bool) iValue;
   return rc;
}

//!  Loads unsigned target option from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, unsigned &value, unsigned defaultValue) {
   int iValue;
   USBDM_ErrorCode rc = getAttribute(key, iValue, (int)defaultValue);
   value = (unsigned) iValue;
   return rc;
}

//!  Loads string target options from Codewarrior
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
static USBDM_ErrorCode getAttribute(const string &key, string &value, const string &defaultValue) {

	string sValue;
	string keyBuffer(baseKey);
#if  TARGET==CFVx
   keyBuffer += suffixKeyCfvx;
#endif
	keyBuffer += key;

	value = defaultValue;
	if (mtwksGetStringValue(keyBuffer, sValue) != DI_OK) {
	   print("getAttribute(%s) => Failed\n", (const char *)key.c_str());
		return BDM_RC_ILLEGAL_PARAMS;
	}
	if (sValue != emptyString)
		value = sValue;

   print("getAttribute(%s) => %s\n", (const char *)key.c_str(), (const char *)value.c_str());
	return BDM_RC_OK;
}

//!  Does the following:
//!   - Loads device name from Codewarrior
//!   - Loads corresponding device data from database
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode getDeviceData(DeviceData &deviceData) {
   DiReturnT       diRC = DI_OK;
   string          deviceName;
   int value;

   // Device name
   diRC = mtwksGetStringValue(processorKey, deviceName);
   if ((diRC != DI_OK) || (deviceName == emptyString)) {
      print("getDeviceInfo() - Device name not set\n");
      print("key = %s\n", processorKey.c_str());
      return  BDM_RC_UNKNOWN_DEVICE;
   }
   print("getDeviceData() - Device name = \'%s\'\n", (const char *)deviceName.c_str());

   DeviceDataBase deviceDataBase;
   try {
      deviceDataBase.loadDeviceData();
   }
   catch (MyException &exception) {
      print("getDeviceData() - Failed to load device database\n");
      string("Failed to load device database\nReason: ")+exception.what();
      displayDialogue((string("Failed to load device database\nReason: ")+exception.what()).c_str(),
                   "Error loading devices",
                   wxOK);
      return BDM_RC_DEVICE_DATABASE_ERROR;
   }
   catch (...){
      print("getDeviceData() - Failed to load device database\n");
      displayDialogue("Failed to load device database\n",
                      "Error loading devices",
                      wxOK);
      return BDM_RC_DEVICE_DATABASE_ERROR;
   }
   const DeviceData *dev = deviceDataBase.findDeviceFromName(deviceName);
   print("getDeviceData() - Found device \'%s\' in database\n", (const char *)dev->getTargetName().c_str());
   if (dev == NULL) {
      print("getDeviceData() - Unknown device\n");
      mtwksDisplayLine("Unrecognised device - using default settings");
      dev = deviceDataBase.getDefaultDevice();
//      return BDM_RC_UNKNOWN_DEVICE;
   }
   deviceData = *dev;
   getAttribute(KeyTrimTargetClock, value, 0);
   if (value != 0) {
      getAttribute(KeyClockTrimFrequency, value, deviceData.getClockTrimFreq());
      if (value == 0) {
         value = deviceData.getClockTrimFreq();
      }
      deviceData.setClockTrimFreq(value);
      getAttribute(KeyClockTrimNVAddress, value, deviceData.getClockTrimNVAddress());
      if (value == 0) {
         value = deviceData.getClockTrimNVAddress();
      }
      deviceData.setClockTrimNVAddress(value);
   }
   else {
      deviceData.setClockTrimFreq(0);
   }
   int eraseOptions;
   getAttribute(KeyEraseMethod, eraseOptions, (int)DeviceData::eraseMass);
   deviceData.setEraseOption((DeviceData::EraseOptions)eraseOptions);
   return BDM_RC_OK;
}

//!  Loads general settings from Codewarrior Eclipse environment
//!
//!  @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode loadSettings(TargetType_t             targetType,
                             USBDM_ExtendedOptions_t &bdmOptions,
			  			           string                  &bdmSerialNumber
						           ) {
   print("loadSettings(%s)\n", getTargetTypeName(targetType));

   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = targetType;
   USBDM_ErrorCode bdmRC = USBDM_GetDefaultExtendedOptions(&bdmOptions);
   if (bdmRC != BDM_RC_OK) {
      return bdmRC;
   }
//   int rc    = DI_OK;
//   int value = 1;
//   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION, MTWKS_CB_SETMEEID, &value, &rc);
//   char message[] = "EError Message\n";
//   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION, MTWKS_MSG, &message, &rc);
//   char message2[] = "WWarning Message\n";
//   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION, MTWKS_MSG, &message2, &rc);
//   char message3[] = "IInformation Message\n";
//   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION, MTWKS_MSG, &message3, &rc);
//   char message4[] = " Message\n";
//   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION, MTWKS_MSG, &message4, &rc);
	getAttribute(KeySetTargetVdd, 				bdmOptions.targetVdd, 				bdmOptions.targetVdd);
	getAttribute(KeyCycleTargetVddOnReset, 	bdmOptions.cycleVddOnReset, 		bdmOptions.cycleVddOnReset);
	getAttribute(KeyCycleTargetVddOnConnect, 	bdmOptions.cycleVddOnConnect, 	bdmOptions.cycleVddOnConnect);
	getAttribute(KeyLeaveTargetPowered, 		bdmOptions.leaveTargetPowered, 	bdmOptions.leaveTargetPowered);
	getAttribute(KeyMaskInterrupt, 				bdmOptions.maskInterrupts, 		bdmOptions.maskInterrupts);
	getAttribute(KeyAutomaticReconnect, 		bdmOptions.autoReconnect, 			bdmOptions.autoReconnect);
	getAttribute(KeyUseResetSignal,           bdmOptions.useResetSignal, 		bdmOptions.useResetSignal);
	getAttribute(KeyUsePSTSignals,            bdmOptions.usePSTSignals,   		bdmOptions.usePSTSignals);
	getAttribute(KeyUseAltBDMClock,           bdmOptions.bdmClockSource, 		bdmOptions.bdmClockSource);
	getAttribute(KeyConnectionSpeed,          bdmOptions.interfaceFrequency,   bdmOptions.interfaceFrequency);
	bdmOptions.interfaceFrequency /= 1000; // Convert to kHz

   getAttribute(KeyPowerOffDuration,         bdmOptions.powerOffDuration,       bdmOptions.powerOffDuration);
   getAttribute(KeyPowerOnRecoveryInterval,  bdmOptions.powerOnRecoveryInterval,bdmOptions.powerOnRecoveryInterval);
   getAttribute(KeyResetDuration,            bdmOptions.resetDuration,          bdmOptions.resetDuration);
   getAttribute(KeyResetReleaseInterval,     bdmOptions.resetReleaseInterval,   bdmOptions.resetReleaseInterval);
   getAttribute(KeyResetRecoveryInterval,    bdmOptions.resetRecoveryInterval,  bdmOptions.resetRecoveryInterval);

	getAttribute(KeyDefaultBdmSerialNumber,   bdmSerialNumber,                 emptyString);
	printBdmOptions(&bdmOptions);
	return BDM_RC_OK;
}
#endif

#ifdef LEGACY
DiReturnT loadNames(string &deviceName, string &projectPath) {
   DiReturnT rc;

   print("loadNames()\n");

   if (metrowerksCallbackFunction == NULL) {
      print("mtwksGetStringValue() - callback not set\n");
      return setErrorState(DI_ERR_NONFATAL, ("Callback function not set"));
   }

   // Set autodetect & obtain device Name
   autoConfig_t configValue;
   configValue.options  = 0x01;
   configValue.deviceID = 0x00;
   metrowerksCallbackFunction(DI_CB_MTWKS_EXTENSION,
                              MTWKS_LEGACY_CB_COLDFIREAUTOCONFIG,
                              &configValue, &rc);
   if (rc != DI_OK) {
      return setErrorState(rc, ("MTWKS_LEGACY_CB_COLDFIREAUTOCONFIG failed"));
   }

   print("metrowerksCallbackFunction(MTWKS_CB_HC12AUTOCONFIG) => "
         "opt=0x%X, deviceID=0x%X, deviceName=\'%s\'\n",
         configValue.options, configValue.deviceID, configValue.deviceName);

   deviceName = string(configValue.deviceName);

   // Create name of settings file (in %APPDATA% directory)
   projectPath = "USBDM_";
   projectPath += deviceName;
   projectPath += "_";

   return DI_OK;
}
#endif
