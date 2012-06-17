/*! \file
    \brief Implements USBDM dialogue

   USBDMDialogue.cpp

    \verbatim
    USBDM
    Copyright (C) 2009  Peter O'Donoghue

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
   -============================================================================
   |  1 Jul 2010 | wxWidgets version created                               - pgo
   +============================================================================
   \endverbatim
 */

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/imaglist.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>

#include "Common.h"
#include "Version.h"
#include "USBDMPanel.h"
#include "USBDMDialogue.h"
#include "USBDM_API.h"
#include "USBDM_AUX.h"
#include "ApplicationFiles.h"
#include "Log.h"
#include "hcs12UnsecureDialogue.h"

const wxString USBDMDialogue::settingsKey(_("usbdm"));

//========================================================================================================================
// USBDMDialogue methods

bool USBDMDialogue::getDialogueValues(USBDM_ExtendedOptions_t *bdmOptions) {

   print("USBDMDialogue::getDialogueValues()\n");

   // Reset all options to default
   bdmOptions->size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions->targetType = targetType;
   USBDM_GetDefaultExtendedOptions(bdmOptions);

   // Get dialogue values
   communicationPanel->getDialogueValues(bdmOptions);

   return true;
}

//! Display error dialogue and get response
//!
//! @param rc - error reason
//!
//! @return
//!  wxID_YES    => retry
//!  wxID_NO     => display configuration dialogue & retry
//!  wxID_CANCEL => give up
//!
int USBDMDialogue::getErrorAction(USBDM_ErrorCode rc) {

#if wxVERSION_NUMBER < 2900
   wxString prompt;
   if (forceDisplay)
      prompt = _("\n\n Return to connection dialogue?");
   else
      prompt = _("\n\n Open connection dialogue?");
   wxMessageDialog msgBox(this,
          _("Failed to connect to target.\n\n"
            "Reason: ") +
            wxString(USBDM_GetErrorString(rc), wxConvUTF8) +
            prompt,
            _("USBDM Connection Error"),
            wxYES_NO|wxICON_ERROR|wxSTAY_ON_TOP
   );
   int getYesNo = msgBox.ShowModal();
   if (getYesNo == wxID_NO) {
      getYesNo = wxID_CANCEL;
   }
   *forceDisplay = true;
#else
   wxMessageDialog msgBox(this,
          _("Failed to connect to target.\n\n"
            "Reason: ") +
            wxString(USBDM_GetErrorString(rc), wxConvUTF8),
            _("USBDM Connection Error"),
            wxYES_NO|wxCANCEL|wxICON_ERROR|wxSTAY_ON_TOP
   );
   //                            ID_YES            ID_NO          ID_CANCEL
   msgBox.SetYesNoCancelLabels(_("Retry"), _("Change Settings"), _("Cancel"));
   int getYesNo = msgBox.ShowModal();
   print(" USBDMDialogue::getErrorAction() => %d\n", getYesNo);
#endif
   return getYesNo;
}

//! Does the following:
//! - Loads persistent configuration
//! - Optionally display the USBDM dialogue
//! - Saves persistent configuration
//! - Opens the BDM
//! - Configures the BDM
//! - Sets target type
//!
//! @param forceDisplay - force display of configuration dialogue
//!
//! @return error code \n
//!   - BDM_RC_OK => ok
//!   - else      => various USBDM error codes.
//!
//! @note - The user may be prompted to retry/change configuration
//!
USBDM_ErrorCode USBDMDialogue::execute(bool forceDisplay) {

   print("USBDMDialogue::execute()\n");

   if (errorSet != BDM_RC_OK) {
      return errorSet;
   }
   loadSettingsFileFromAppDir(_("usbdm_"));
   forceDisplay =  forceDisplay || !dontDisplayDialogue;
   USBDM_ErrorCode rc;
   int getYesNo;
   do {
      if (forceDisplay) {
         ShowModal();
         saveSettingsFileToAppDir(_("usbdm_"));
      }
      rc = communicationPanel->openBdm();
      if (rc == BDM_RC_OK) {
         USBDM_ExtendedOptions_t bdmOptions;
         getDialogueValues(&bdmOptions);
         rc = USBDM_SetExtendedOptions(&bdmOptions);
      }
      if (rc == BDM_RC_OK) {
         rc = USBDM_SetTargetType(targetType);
      }
      if (rc == BDM_RC_OK) {
         // Check target Vdd present
         USBDMStatus_t status;
         USBDM_GetBDMStatus(&status);
         TargetVddState_t targetVdd = status.power_state;
         if ((targetVdd != BDM_TARGET_VDD_EXT)&&(targetVdd != BDM_TARGET_VDD_INT)) {
            rc = BDM_RC_VDD_NOT_PRESENT;
         }
      }
      if (rc == BDM_RC_OK) {
         break;
      }
      getYesNo = getErrorAction(rc);
      forceDisplay = (getYesNo == wxID_NO); // Retry with changed settings
   } while (getYesNo != wxID_CANCEL);
   return rc;
}

//! Load settings from a settings object
//!
//! @param appSettings - Object containing settings
//!
bool USBDMDialogue::loadSettings(const AppSettings &appSettings) {
   print("USBDMDialogue::loadSettings(AppSettings)\n");

   communicationPanel->Init();
   communicationPanel->loadSettings(appSettings);

   dontDisplayDialogue = appSettings.getValue(settingsKey+_(".dontDisplayDialogue"), false);
   return true;
}

//! Load setting file
//!
//! @param fileName - Name of file to use (without path)
//!
bool USBDMDialogue::loadSettingsFile(const wxString &fileName) {

   print("USBDMDialogue::loadSettingsFile(%s)\n", (const char *)fileName.ToAscii());

   AppSettings appSettings;
   wxString settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   if (!appSettings.loadFromFile(settingsFilename)) {
      print("USBDMDialogue::loadSettingsFile() - no settings file found\n");
      return false;
   }
   appSettings.printToLog();

   // Load the saved settings
   return loadSettings(appSettings);
}

//! Load setting file from %APPDATA%
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::loadSettingsFileFromAppDir(wxString const &fileName) {

   print("USBDMDialogue::loadSettingsFileFromAppDir(%s)\n", (const char *)fileName.ToAscii());

   AppSettings appSettings;
   wxString settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   if (!appSettings.loadFromAppDirFile(settingsFilename)) {
      print("USBDMDialogue::loadSettingsFileFromAppDir() - no settings file found\n");
      return false;
   }
   appSettings.printToLog();

   // Load the saved settings
   return loadSettings(appSettings);
}

//! Save setting file
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::saveSettings(AppSettings &appSettings) {

   print("USBDMDialogue::saveSettings(AppSettings)\n");

   communicationPanel->saveSettings(appSettings);
   appSettings.addValue(settingsKey+_(".dontDisplayDialogue"), dontDisplayDialogue);

   appSettings.printToLog();
   return true;
}

//! Save setting file
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::saveSettingsFile(wxString const &fileName) {

   AppSettings appSettings;
   wxString settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   print("USBDMDialogue::saveSettingsFile(%s)\n", (const char *)settingsFilename.ToAscii());

   saveSettings(appSettings);

   if (!appSettings.writeToFile(settingsFilename, _(VERSION_STRING))) {
      wxMessageBox(_("Unable to save USBDM settings."),
                   _("USBDM Failure"),
                   wxOK|wxICON_WARNING,
                   this
                   );
      return false;
   }
   return true;
}

//! Save setting file
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::saveSettingsFileToAppDir(wxString const &fileName) {

   print("USBDMDialogue::saveSettingsFileToAppDir(%s)\n", (const char *)fileName.ToAscii());

   AppSettings appSettings;
   wxString settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   TransferDataFromWindow();
   saveSettings(appSettings);

   if (!appSettings.writeToAppDirFile(settingsFilename, _(VERSION_STRING))) {
      wxMessageBox(_("Unable to save USBDM settings."),
                   _("USBDM Failure"),
                   wxOK|wxICON_WARNING,
                   this
                   );
      return false;
   }
   return true;
}

//===================================================================
//===================================================================
//===================================================================

//! USBDMDialogue constructor
//!
//! @param targetType      : Target type - controls which dialogue & options are displayed
//! @param bdmCapabilities : Describes the bdm capabilities
//! @param caption         : Base caption to display on dialogue
//!
//! @note: It is necessary to call Create()
//!
USBDMDialogue::USBDMDialogue(TargetType_t targetType, const wxString &caption) :
                             targetType(targetType),
                             caption(caption),
                             bdmCapabilities(BDM_CAP_NONE),
                             errorSet(BDM_RC_OK) {
   Init();
}

//! USBDMDialogue constructors
//!
//! @param parent          : Parent window to pass to Create()
//! @param targetType      : Target type - controls which dialogue & options are displayed
//! @param caption         : Base caption to display on dialogue
//!
//! @note: Calls Create() to creates the dialogue
//!
USBDMDialogue::USBDMDialogue( wxWindow* parent, TargetType_t targetType, const wxString &caption) :
                              targetType(targetType),
                              caption(caption),
                              bdmCapabilities(BDM_CAP_NONE),
                              errorSet(BDM_RC_OK) {

   print("USBDMDialogue::USBDMDialogue()\n");
   Init();
   Create(parent);
}

//! USBDMDialogue creator
//!
//! @param _caption : Partial caption to display on dialogue
//!
bool USBDMDialogue::Init() {

   print("USBDMDialogue::Init()\n");

   dontDisplayDialogue = false;
   return true;
}

//! USBDMDialogue creator
//!
//! @param parent     : Parent window
//!
bool USBDMDialogue::Create( wxWindow* parent) {

   print("USBDMDialogue::Create()\n");

   if (errorSet) {
      return false;
   }
   wxString completeCaption(caption);

   switch(targetType) {
      case T_HC12 :
         completeCaption  += _(" - HCS12");
         break;
      case T_HCS08 :
         completeCaption  += _(" - HCS08");
         break;
      case T_CFV1 :
         completeCaption  += _(" - Coldfire V1 ");
         break;
      default :
         errorSet = BDM_RC_ILLEGAL_PARAMS;
         return false;
         break;
   }
   wxDialog::Create( parent, ID_FLASH_PROGRAMMER_DIALOGUE, completeCaption, wxDefaultPosition, wxSize(-1,-1), wxCAPTION|wxSTAY_ON_TOP);

   CreateControls();

   if (GetSizer()) {
      GetSizer()->SetSizeHints(this);
   }
   Centre();

   return true;
}

//! USBDMDialogue destructor
//!
USBDMDialogue::~USBDMDialogue() {
}

//! Control creation for FlashParametersPanel
//!
void USBDMDialogue::CreateControls() {

   USBDMDialogue* frame = this;

   print("USBDMDialogue::CreateControls()\n");

   wxBoxSizer* frameBoxSizerV = new wxBoxSizer(wxVERTICAL);
   frame->SetSizer(frameBoxSizerV);

   communicationPanel = new USBDMPanel(targetType);
   USBDM_ErrorCode rc = communicationPanel->Create(frame);
   if (rc != BDM_RC_OK) {
      return;
   }
   frameBoxSizerV->Add(communicationPanel, 0, wxGROW|wxALL, 0);

//------------------------------------------------
   wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
   frameBoxSizerV->Add(buttonSizer, 0, wxGROW|wxALL, 5);

   wxButton* defaultButton = new wxButton( frame, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxDefaultSize, 0 );
   buttonSizer->Add(defaultButton, 0, wxGROW|wxALL, 5);
   buttonSizer->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxButton* okButton = new wxButton( frame, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
   buttonSizer->Add(okButton, 0, wxGROW|wxALL, 5);
   okButton->SetDefault();

   dontShowAgainControl = new wxCheckBox( frame, ID_DONT_SHOW_AGAIN_CHECKBOX, _("Don't show this dialogue in future"), wxDefaultPosition, wxDefaultSize, 0 );
   dontShowAgainControl->SetValue(false);
   frameBoxSizerV->Add(dontShowAgainControl, 0, wxALIGN_LEFT|wxALL, 5);
}

//===================================================================
//! Copy internal state to Dialogue controls
//!
bool USBDMDialogue::TransferDataToWindow() {

   print("USBDMDialogue::TransferDataToWindow()\n");
   dontShowAgainControl->Set3StateValue(dontDisplayDialogue?wxCHK_CHECKED:wxCHK_UNCHECKED);
   return communicationPanel->TransferDataToWindow();
}

//===================================================================
//! Copy internal state from Dialogue controls
//!
bool USBDMDialogue::TransferDataFromWindow() {

   print("USBDMDialogue::TransferDataFromWindow()\n");
   dontDisplayDialogue = dontShowAgainControl->IsChecked();
   return communicationPanel->TransferDataFromWindow();
}

//===================================================================
//! Set dialogue state to default values
//!
bool USBDMDialogue::setDialogueValuesToDefault() {

   print("USBDMDialogue::setDialogueValuesToDefault()\n");
   Init();
   communicationPanel->Init();
   TransferDataToWindow();
   return true;
}

//! Should we show tooltips?
//!
bool USBDMDialogue::ShowToolTips() {
    return true;
}

//! Get bitmap resources
//!
//! @param event The event to handle
//!
wxBitmap USBDMDialogue::GetBitmapResource( const wxString& name ) {
    // Bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
}

//! Get icon resources
//!
//! @param event The event to handle
//!
wxIcon USBDMDialogue::GetIconResource( const wxString& name ) {
    // Icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
}

/*
 * USBDMDialogue type definition
 */
IMPLEMENT_CLASS( USBDMDialogue, wxDialog )

//
//  USBDMDialogue event table definition
//
BEGIN_EVENT_TABLE( USBDMDialogue, wxDialog )
   EVT_CHECKBOX( ID_DONT_SHOW_AGAIN_CHECKBOX, USBDMDialogue::OnDontShowAgainControlClick )
   EVT_BUTTON( wxID_DEFAULT, USBDMDialogue::OnDefaultClick )
   EVT_BUTTON( wxID_OK, USBDMDialogue::OnOkClick )
END_EVENT_TABLE()

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RECONNECT_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMDialogue::OnDontShowAgainControlClick( wxCommandEvent& event ) {
   dontDisplayDialogue = event.IsChecked();
}

//! wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_DEFAULT
//!
//! @param event The event to handle
//!
void USBDMDialogue::OnDefaultClick( wxCommandEvent& event ) {
   print("USBDMDialogue::OnDefaultClick()\n");
   setDialogueValuesToDefault();
}

//! wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
//!
//! @param event The event to handle
//!
void USBDMDialogue::OnOkClick( wxCommandEvent& event ) {
//   fprintf(stderr, "Clicked OK\n"); fflush(stderr);
//   USBDM_ErrorCode rc = communicationPanel->openBdm();
//   if (rc == BDM_RC_OK)
   EndModal(BDM_RC_OK);
}
