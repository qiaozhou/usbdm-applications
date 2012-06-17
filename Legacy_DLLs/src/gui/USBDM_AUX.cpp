/*! \file
   \brief USBDM utility functions

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
+=================================================================================
|  1 Aug 2010 | Created for Linux version                                    - pgo
+=================================================================================
\endverbatim
*/

#include <wx/wx.h>
#include <wx/progdlg.h>
#include <wx/choicdlg.h>
#include "Log.h"
#include "USBDM_API.h"
#include "USBDM_AUX.h"
#include "hcs12UnsecureDialogue.h"
#include "TargetDefines.h"
#include "Names.h"
#include "USBDMDialogue.h"

// Used to suppress retry dialogue if previous attempt failed
static bool extendedRetry = true;

//! Gets BDM Status with checks for fatal errors with no recovery option
//!
//! At this time the only fatal errors are:
//!   - USBDM_GetBDMStatus() failed i.e. the BDMs gone away - quietly fails
//!   - Target supply overload - flags error to user & fails
//!
//! @param USBDMStatus status value from BDM
//!
//! @return \n
//!     DI_OK              => OK \n
//!     DI_ERR_FATAL       => Error see \ref currentErrorString
//!
USBDM_ErrorCode getBDMStatus(USBDMStatus_t *USBDMStatus, wxWindow *parent) {
   USBDM_ErrorCode rc;

   print("getBDMStatus()\n");

   // USBDM_GetBDMStatus() should always succeed
   rc = USBDM_GetBDMStatus(USBDMStatus);
   if (rc != BDM_RC_OK) {
      print("getBDMStatus() - failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   // Check for Fatal power supply problems
   if (USBDMStatus->power_state == BDM_TARGET_VDD_ERR) {
      wxMessageBox(_("Overload of the BDM Target Vdd supply has been detected.  \n"
                     "The target supply has been disabled.\n\n"
                     "Please restart the debugging session to restore power."),
                   _("USBDM Interface - Target Power Failure"),
                   wxICON_ERROR|wxOK|wxSTAY_ON_TOP,
                   parent
                   );
      print("getBDMStatus() - failed, reason = Vdd overload\n");
      return BDM_RC_VDD_NOT_PRESENT;
   }
   return BDM_RC_OK;
}

//! Connects to Target.
//!
//! This will cause the BDM module to attempt to connect to the Target.
//! In most cases the BDM module will automatically determine the connection
//! speed and successfully connect.  If unsuccessful, it may be necessary
//! to manually set the speed using set_speed().
//!
//! @note If there are connection problems, the user may be prompted to cycle
//!       the target power on some targets.
//!
//! @param usbdmStatus - Current status of the BDM, may be NULL if not needed.
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode USBDM_TargetConnectWithRetry(USBDMStatus_t *usbdmStatus, wxWindow *parent) {

   print("USBDM_TargetConnectWithRetry()\n");

   USBDM_ErrorCode rc;
   
   USBDMStatus_t status;
   rc = getBDMStatus(&status);
   if (usbdmStatus != NULL)
      *usbdmStatus = status;
   if (rc != BDM_RC_OK)
      return rc; // Fatal error

   rc = USBDM_Connect();
   if (rc == BDM_RC_OK) {
      if (!extendedRetry)
         print("USBDM_TargetConnectWithRetry() - Enabling Extended Retry\n");
      extendedRetry = true;
      return rc;
   }
   // Quietly retry
   rc = USBDM_Connect();
   if (rc == BDM_RC_OK) {
      if (!extendedRetry)
         print("USBDM_TargetConnectWithRetry() - Enabling Extended Retry\n");
      extendedRetry = true;
      return rc;
   }
   if (extendedRetry) {
      // Connection failure - retry
      int getYesNo;
      do {
         wxString message;
         print("USBDM_TargetConnectWithRetry() - retry\n");

         USBDM_ControlInterface(0, SI_BKGD_LOW|SI_RESET_LOW); // Set BKGD & RESET low

         // Check for 'interesting cases'
         if (status.power_state == BDM_TARGET_VDD_NONE) {
            message =      _("Target Vdd supply interrupted.\n\n"
                             "Please restore power to the target.\n\n"
                             "Retry connection?");
         }
         else if (status.reset_recent == RESET_DETECTED) {
            message =      _("Target RESET detected.\n\n"
                             "Retry connection?");
         }
         else if (rc == BDM_RC_BDM_EN_FAILED) {
            message =      _("Enabling the BDM interface on target failed.\n"
                             "The target may be secured.\n\n"
                             "Retry connection?");
         }
         else {
            message =       _("Connection with the target has failed.\n\n"
                              "Please cycle power to the target.\n\n"
                              "Retry connection?");
         }
         getYesNo = wxMessageBox(message,
                                 _("USBDM - Target Connection Failure"),
                                 wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION|wxSTAY_ON_TOP,
                                 parent
                                 );
         USBDM_ControlPins(PIN_BKGD_LOW|PIN_RESET_3STATE);   // Release RESET (BKGD stays low)
         wxMilliSleep(100 /* ms */);                           // Give target time to recover from reset
         USBDM_ControlPins(PIN_RELEASE);                     // Release BKGD
         wxMilliSleep(500 /* ms */);                           // Give target time to recover from reset

         // Get status twice to clear spurious reset flag
         getBDMStatus(&status);
         USBDM_ErrorCode rc2 = getBDMStatus(&status);
         if (usbdmStatus != NULL)
            *usbdmStatus = status;
         if (rc2 != BDM_RC_OK) {
            // Fatal error
            print("USBDM_TargetConnectWithRetry() - USBDM_GetBDMStatus() failed!\n");
            break;
         }
         // Retry connection even if aborted - this may leave connection speed correctly set
         rc = USBDM_Connect();    // Try connect again
      } while ((rc != BDM_RC_OK) && (getYesNo == wxYES));
   }
   if (rc != BDM_RC_OK) {
      print("USBDM_TargetConnectWithRetry() - failed, (disabling Extended Retry)\n");
   }
   // Only enable re-try if successful to stop nagging
   extendedRetry = (rc == BDM_RC_OK);

   return rc;
}

/*!
 * Converts a UTF-16-LE to a wxString
 *
 * @param str - the string to convert
 *
 * @return str converted to a wxString
 *
 */
static wxString convertUtfToString(const char *str) {
   wxMBConvUTF16LE converter;
   wchar_t unicodeBuff[200];
   converter.MB2WC(unicodeBuff, str, sizeof(unicodeBuff));
   return wxString(unicodeBuff);
}

/*!
 * Converts a wxString to a UTF-16-LE
 *
 * @param str - the string to convert
 *
 * @return str converted to a UTF-16-LE (in static buffer)
 *
 */
const char *convertStringToUtf(const wxString &str) {
   wxMBConvUTF16LE converter;
   static char utfBuffer[200];
   converter.WC2MB((char*)utfBuffer, (const wchar_t*)str, sizeof(utfBuffer));
   return utfBuffer;
}

//! Reads USB Serial Number from BDM interface
//!
//! @param serialNumber - serialNumber
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode USBDM_GetBDMSerialNumber(wxString &serialNumber) {
const char *serialNumberPtr;
USBDM_ErrorCode rc;

   rc = USBDM_GetBDMSerialNumber(&serialNumberPtr);
   if (rc != BDM_RC_OK)
      return rc;
   serialNumber = convertUtfToString(serialNumberPtr);
   return BDM_RC_OK;
}

//! Reads USB Description from BDM interface
//!
//! @param description - serialNumber
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode USBDM_GetBDMDescription(wxString &description) {
const char *descriptionPtr;
USBDM_ErrorCode rc;

   rc = USBDM_GetBDMDescription(&descriptionPtr);
   if (rc != BDM_RC_OK)
      return rc;
   description = convertUtfToString(descriptionPtr);
   return BDM_RC_OK;
}

#ifdef TBDML
USBDM_ErrorCode checkTargetUnSecured() {
const int FlashControl(0x0100);
const int FSECAddress(FlashControl+1);
uint8_t FSECValue;

   USBDM_ErrorCode rc = USBDM_ReadMemory(1, 1, FSECAddress, &FSECValue);
   if (rc != BDM_RC_OK) {
      print( "USBDMDialogue::checkTargetUnSecured() => error => assumed secured\n");
      return rc;
   }
   if ((FSECValue & 0x03) != 0x2) {
      print("USBDMDialogue::checkTargetUnSecured() => secured\n");
      return BDM_RC_FAIL;
   }
   else {
      print("USBDMDialogue::checkTargetUnSecured() => unsecured\n");
      return BDM_RC_OK;
   }
}

//! Special handling for HCS12
//!
USBDM_ErrorCode hcs12Check(void) {
   USBDM_ErrorCode rc;
   unsigned long connectionFrequency;
   USBDMStatus_t usbdmStatus;

   // Get connection speed
   rc = USBDM_GetSpeedHz(&connectionFrequency);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   rc = USBDM_GetBDMStatus(&usbdmStatus);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   // Round to 10's kHz
   connectionFrequency = round(connectionFrequency/10000);

   if (checkTargetUnSecured() != BDM_RC_FAIL) {
      // Not secured! - Some thing strange going on - give up
      return rc;
   }
   int getYesNo = wxMessageBox(_("USBDM can't enable BDM mode on the target.\n"
                                 "This may indicate the device is secured.\n\n"
                                 "Do you wish to try unsecuring the device?"),
                               _("USBDM cannot enable BDM mode."),
                               wxYES_NO|wxNO_DEFAULT|wxICON_WARNING
                               );
   if (getYesNo == wxYES) {
      rc = HCS12Unsecure::unsecureHCS12Target();
   }
   else {
      rc = BDM_RC_BDM_EN_FAILED;
   }
   return rc;
}
#endif

//! Open target device
//!
//! Does the following:
//! - Loads persistent configuration
//! - Optionally displays the USBDM dialogue
//! - Saves persistent configuration
//! - Opens the BDM
//! - Configures the BDM
//! - Sets target type
//!
//! @param targetType type of target
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode USBDM_OpenTargetWithConfig(TargetType_t targetType) {

   print("USBDM_OpenTargetWithConfig(%s)\n", getTargetTypeName(targetType));

   if (targetType == T_OFF) {
      return USBDM_SetTargetType(T_OFF);
   }
   // Do configuration dialogue
   USBDMDialogue dialogue(NULL, targetType);
   USBDM_ErrorCode rc = dialogue.execute(false);
#ifdef TBDML
   if ((targetType == T_HC12) && (rc == BDM_RC_BDM_EN_FAILED)) {
      // BDM_RC_BDM_EN_FAILED may indicate a secured HCS12 device
      rc = hcs12Check(); // Special handling for secured devices
   }
#endif
   return rc;
}

//! Display Target selection dialogue.
//!
//! @param target   Target processor type selected.
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode targetSelectDialog(TargetType_t *targetType){
wxString choices[] = {_("HCS12"), _("HCS08"), _("RS08"), _("CFV1"), _("CFV 2, 3 or 4"), _("JTAG"), _("EZFLASH"), _("MC56F80xx"), };

   wxSingleChoiceDialog dialogue(NULL, wxEmptyString, _("Choose Target"), sizeof(choices)/sizeof(choices[0]), choices, NULL,
                                 wxDEFAULT_DIALOG_STYLE | wxOK );
   dialogue.ShowModal();
   *targetType = (TargetType_t)dialogue.GetSelection();
   print("targetSelectDialog() => %s\n", getTargetTypeName(*targetType));

   return BDM_RC_OK;
}

#if 0
//! Open target device
//!
//! This includes the following steps:
//!   - Displays USBDM dialogue
//!      - The dialogue allows the user to select the BDM (if more than one)
//!      - Choose target options (Vdd etc)
//!   - Opens selected BDM
//!   - Sets target options
//!   - Sets target device
//!  The user may be prompted to change options if opening the target fails.
//!  The target is left open ready for use
//!
//! @param targetType type of target
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode _USBDM_OpenTargetWithConfig(TargetType_t targetType, USBDM_ExtendedOptions_t *bdmOptions_) {

   USBDM_ErrorCode         rc = BDM_RC_OK;
   USBDM_ExtendedOptions_t bdmOptions;

   print("USBDM_OpenTargetWithConfig(%s)\n", getTargetTypeName(targetType));

   if (targetType != T_OFF) {
      // Do configuration dialogue
      rc = USBDM_ConfigureDialog(targetType, &bdmOptions);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      if (bdmOptions_!= NULL) {
         *bdmOptions_ = bdmOptions;
      }
      // Configure BDM
      rc = USBDM_SetExtendedOptions(&bdmOptions);
      if (rc != BDM_RC_OK) {
         return rc;
      }
   }
   rc = USBDM_SetTargetTypeWithRetry(targetType);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   return rc;
}

//! Display the USBDM dialogue etc.
//! The BDM is opened and ready for use upon exit.
//!
//! @param forceDisplay - force display of configuration dialogue
//!
//! @return error code \n
//!   - BDM_RC_OK => ok
//!   - else => various USBDM error codes. An error pop-up has already been displayed.
//!
USBDM_ErrorCode ::_execute(bool forceDisplay) {

   print("USBDMDialogue::execute()\n");

   USBDM_ErrorCode rc = BDM_RC_OK;
   int getYesNo = wxNO;

   if (errorSet != BDM_RC_OK) {
      return errorSet;
   }
   loadSettingsFileFromAppDir(_("usbdm_"));

   forceDisplay =  forceDisplay || !dontDisplayDialogue;
   do {
      USBDM_Close();
      if (forceDisplay) {
         ShowModal();
         saveSettingsFileToAppDir(_("usbdm_"));
      }
      USBDM_ExtendedOptions_t bdmOptions;
      getDialogueValues(&bdmOptions);
      USBDM_SetExtendedOptions(&bdmOptions);
      rc = communicationPanel->openBdm();
      if (rc == BDM_RC_OK) {
         rc = USBDM_Connect();
      }
      getYesNo = wxID_CANCEL;
      if ((rc != BDM_RC_OK) && ((targetType != T_HC12) || (rc != BDM_RC_BDM_EN_FAILED))) {
         // BDM_RC_BDM_EN_FAILED may indicate a secured HCS12 device so ignore
         print("USBDMDialogue::execute() - failed\n");
         getYesNo = getErrorAction(rc, &forceDisplay);
      }
   } while ((rc != BDM_RC_OK) && (getYesNo != wxID_CANCEL));

   if ((targetType == T_HC12) && (rc == BDM_RC_BDM_EN_FAILED)) {
      // BDM_RC_BDM_EN_FAILED may indicate a secured HCS12 device
      rc = hcs12Check(); // Special handling for secured devices
   }
   return rc;
}
#endif
#if 0
//===================================================================
//! Sets target MCU type with retry
//!
//! @param target_type type of target
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
//! @note  The user may be prompted to supply target power.
//! @note  The user is alerted to any problems.
//!
USBDM_ErrorCode USBDM_SetTargetTypeWithRetry(TargetType_t targetType, wxWindow *parent) {
   USBDM_ErrorCode rc;
   int getYesNo;

   print("USBDM_SetTargetTypeWithRetry(): Initial Attempt\n");

   rc  = USBDM_SetTargetType(targetType);

   if (rc == BDM_RC_VDD_NOT_PRESENT) {
      // Target power can sometimes take a long while to rise
      // Give it another second to reduce 'noise'
      wxMilliSleep(1000);
      rc  = USBDM_SetTargetType(targetType);
   }
   while (rc == BDM_RC_VDD_NOT_PRESENT) {
      print("USBDM_SetTargetTypeWithRetry() - retry\n");

      getYesNo = wxMessageBox(_("The target appears to have no power.\n\n"
                                "Please supply power to the target.\n\n"
                                "Retry connection?"),
                              _("USBDM - Target Connection Failure"),
                              wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION|wxSTAY_ON_TOP,
                              parent
                              );
      if ((rc != BDM_RC_OK) && (getYesNo != wxYES)) {
         print("USBDM_SetTargetTypeWithRetry() - no further retries\n");
         break;
      }
      print("USBDM_SetTargetTypeWithRetry() - retry\n");
      rc = USBDM_SetTargetType(targetType);
   }
   if ((rc != BDM_RC_OK) && (rc != BDM_RC_VDD_NOT_PRESENT)) {
      print("USBDM_SetTargetTypeWithRetry() - USBDM_SetTarget() failed\n");
      wxMessageBox(_("Failed to set target.\n\n"
                     "Reason: ") +
                     wxString(USBDM_GetErrorString(rc), wxConvUTF8),
                   _("USBDM Error"),
                   wxOK|wxICON_ERROR|wxSTAY_ON_TOP,
                   parent
                  );
   }
   // Get status to clear reset flag on possible power-up
   USBDMStatus_t status;
   USBDM_GetBDMStatus(&status);

   // Only enable re-try if successful to stop nagging
   extendedRetry = (rc == BDM_RC_OK);

   return rc;
}
#endif




