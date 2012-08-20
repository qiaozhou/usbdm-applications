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
#include "USBDM_API.h"
#include "USBDMDialogue.h"
#include "ApplicationFiles.h"
#include "Log.h"
#include "AppSettings.h"
#if TARGET == MC56F80xx
#include "USBDM_DSC_API.h"
#endif
#if (TARGET==ARM) || (TARGET==ARM_SWD)
#include "USBDM_ARM_API.h"
#endif

const string USBDMDialogue::settingsKey("USBDMDialogue");

//========================================================================================================================
// USBDMDialogue methods

typedef USBDM_ErrorCode (*openTarget)(void);

#if defined(GDI) && defined(LEGACY)
//! Display the USBDM dialogue etc. Intended for use with Legacy GDI
//! The BDM is ready for use upon exit.
//!
//! @param deviceName
//! @param projectName
//! @param forceDisplay
//!
//! @return error code \n
//!   - BDM_RC_OK => ok
//!   - else => various USBDM error codes. An error pop-up has already been displayed.
//!
USBDM_ErrorCode USBDMDialogue::execute(string const     &deviceName,
                                       string const     &projectName,
                                       bool              forceDisplay,
                                       USBDM_ErrorCode (*openTarget)(void)) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   int getYesNo = wxNO;

   if (errorSet != BDM_RC_OK) {
      return errorSet;
   }
   loadSettingsFile(projectName) || loadSettingsFileFromAppDir(projectName);

   forceDisplay =  forceDisplay || !disableDialogueDisplay;
   do {
      if (forceDisplay) {
         ShowModal();
         if (!saveSettingsFile(projectName) && !saveSettingsFileToAppDir(projectName)) {
            wxMessageBox(_("Unable to save USBDM settings. \n"
                           "Please set \'eclipseHome\' attribute\n"
                           "in Debug Configurations"),
                         _("USBDM Failure"),
                         wxOK|wxICON_WARNING,
                         this
                         );
         }
      }
      rc = communicationPanel->openBdm();
      if (rc == BDM_RC_OK) {
         // Set BDM options from dialogue values
         USBDM_ExtendedOptions_t bdmOptions;
         getBdmOptions(bdmOptions);
         rc = USBDM_SetExtendedOptions(&bdmOptions);
      }
      if (rc == BDM_RC_OK) {
         rc = communicationPanel->setTarget();
      }
      if (openTarget != NULL) {
         rc = openTarget();
      }
      else {
#if TARGET == MC56F80xx
         if (rc == BDM_RC_OK) {
            rc = DSC_Connect();
         }
         if (rc != BDM_RC_OK) {
            DSC_TargetReset((TargetMode_t)(RESET_DEFAULT|RESET_SPECIAL));
            rc = DSC_Connect();
         }
#else
         if (rc == BDM_RC_OK) {
            rc = USBDM_Connect();
         }
#endif
      }
      if (rc != BDM_RC_OK) {
         USBDM_Close();
         print("USBDMDialogue::execute() - failed\n");
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
         getYesNo = msgBox.ShowModal();
         if (getYesNo == wxID_NO) {
            getYesNo = wxID_CANCEL;
         }
         forceDisplay = true;
#else
         wxString prompt;
         wxMessageDialog msgBox(this,
                                _("Failed to connect to target.\n\n"
                                  "Reason: ") +
                                wxString(USBDM_GetErrorString(rc), wxConvUTF8),
                                _("USBDM Connection Error"),
                                wxYES_NO|wxCANCEL|wxICON_ERROR|wxSTAY_ON_TOP
                                );
                                    // ID_YES         ID_NO             ID_CANCEL
         msgBox.SetYesNoCancelLabels(_("Retry"), _("Change Settings"), _("Cancel"));
         getYesNo = msgBox.ShowModal();
         forceDisplay = (getYesNo == wxID_NO); // Retry with changed settings
#endif
      }
   } while ((rc != BDM_RC_OK) && (getYesNo != wxID_CANCEL));
   return rc;
}

#elif defined(FLASH_PROGRAMMER)
//! This displays the Dialogue which represents the entire application
//! for the stand-alone flash programmers.
//!
//! @param settingsFilename Base name of settings file to use
//!
//! @return BDM_RC_OK - no useful return vale
//!
USBDM_ErrorCode USBDMDialogue::execute(wxString const &settingsFilename, wxString const &hexFilename) {

   loadSettingsFileFromAppDir((const char *)settingsFilename.ToAscii());
   TransferDataToWindow();
   if (!hexFilename.IsEmpty() && (flashParametersPanel->loadHexFile(hexFilename, true) != PROGRAMMING_RC_OK)) {
      print("USBDMDialogue::execute() - Failed to load Hex file\n");
   }
   ShowModal();
   if (!saveSettingsFileToAppDir((const char *)settingsFilename.ToAscii())) {
      wxMessageBox(_("Unable to save USBDM settings."),
                   _("USBDM Failure"),
                   wxOK|wxICON_WARNING,
                   this
                   );
   }
   USBDM_Close();
   return BDM_RC_OK;
}
#endif
//! Load settings from a settings object
//!
//! @param appSettings - Object containing settings
//!
bool USBDMDialogue::loadSettings(const AppSettings &appSettings) {
   print("USBDMDialogue::loadSettings(AppSettings)\n");

   advancedPanel->Init();
   communicationPanel->Init();

#if defined(FLASH_PROGRAMMER)
   flashParametersPanel->Init();
   flashParametersPanel->loadSettings(appSettings);
   advancedPanel->setCurrentDevice(flashParametersPanel->getCurrentDevice());
#endif

   communicationPanel->loadSettings(appSettings);
   advancedPanel->loadSettings(appSettings);

#ifdef GDI
   disableDialogueDisplay = appSettings.getValue(settingsKey+".dontDisplayDialogue", false);
#endif

   return true;
}

//! Load setting file
//!
//! @param fileName - Name of file to use (without path)
//!
bool USBDMDialogue::loadSettingsFile(const string &fileName) {

   print("USBDMDialogue::loadSettingsFile(%s)\n", (const char *)fileName.c_str());

   AppSettings appSettings;
   string settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

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
bool USBDMDialogue::loadSettingsFileFromAppDir(string const &fileName) {

   AppSettings appSettings;
   string settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   print("USBDMDialogue::loadSettingsFileFromAppDir(%s)\n", settingsFilename.c_str());

   if (!appSettings.loadFromAppDirFile(settingsFilename)) {
      print("USBDMDialogue::loadSettingsFileFromAppDir() - no settings file found\n");
//      return false;
   }
   appSettings.printToLog();

   // Load the saved settings
   return loadSettings(appSettings);
}

//! Save setting file
//!
//! @param appSettings
//!
bool USBDMDialogue::saveSettings(AppSettings &appSettings) {

   print("USBDMDialogue::saveSettings(AppSettings)\n");

   TransferDataFromWindow();
#if defined(FLASH_PROGRAMMER)
   flashParametersPanel->saveSettings(appSettings);
#endif
   communicationPanel->saveSettings(appSettings);
   advancedPanel->saveSettings(appSettings);
#ifdef GDI
   appSettings.addValue(settingsKey+".dontDisplayDialogue", disableDialogueDisplay);
#endif

   appSettings.printToLog();

   return true;
}

//! Save setting file
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::saveSettingsFile(string const &fileName) {

   AppSettings appSettings;
   string settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   print("USBDMDialogue::saveSettingsFile(%s)\n", settingsFilename.c_str());

   saveSettings(appSettings);

   if (!appSettings.writeToFile(settingsFilename, USBDM_VERSION_STRING)) {
      return false;
   }
   return true;
}

//! Save setting file
//!
//! @param fileName      - Name of file to use (without path)
//!
bool USBDMDialogue::saveSettingsFileToAppDir(string const &fileName) {

   print("USBDMDialogue::saveSettingsFileToAppDir(%s)\n", fileName.c_str());

   AppSettings appSettings;
   string settingsFilename(AppSettings::getSettingsFilename(fileName, targetType));

   saveSettings(appSettings);

   if (!appSettings.writeToAppDirFile(settingsFilename, USBDM_VERSION_STRING)) {
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
                              errorSet(BDM_RC_OK),
                              binaryFilename(wxEmptyString) {

   print("USBDMDialogue::USBDMDialogue()\n");

   Init();
   Create(parent);

   print("USBDMDialogue::USBDMDialogue() #2\n");
}

//! Set the dialogue internal state to the default (not including pages)
//!
bool USBDMDialogue::Init() {

//   print("USBDMDialogue::Init()\n");

   disableDialogueDisplay = false;
   return true;
}

//! USBDMDialogue creator
//!
//! @param parent     : Parent window
//! @param style      : Window style
//!
bool USBDMDialogue::Create( wxWindow* parent, long style) {

//   print("USBDMDialogue::Create()\n");

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
      case T_RS08 :
         completeCaption  += _(" - RS08");
         break;
      case T_CFV1 :
         completeCaption  += _(" - Coldfire V1 ");
         break;
      case T_CFVx :
         completeCaption  += _(" - Coldfire V2, 3 & 4 ");
         break;
//      case T_EZFLASH :
//         completeCaption  += _(" - EZFLASH");
//         break;
      case T_JTAG :
         completeCaption  += _(" - JTAG");
         break;
      case T_MC56F80xx :
         completeCaption  += _(" - MC56F80xx ");
         break;
      case T_ARM_JTAG :
      case T_ARM_SWD :
         completeCaption  += _(" - ARM ");
         break;
      default :
         completeCaption += _(" - OPPS");
         errorSet = BDM_RC_ILLEGAL_PARAMS;
         return false;
         break;
   }

   wxDialog::Create( parent, ID_FLASH_PROGRAMMER_DIALOGUE, completeCaption, wxDefaultPosition, wxSize(-1,-1), style);

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

//   print("USBDMDialogue::CreateControls()\n");

   wxBoxSizer* frameBoxSizerV = new wxBoxSizer(wxVERTICAL);
   frame->SetSizer(frameBoxSizerV);

   noteBook = new wxNotebook( frame, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxBK_DEFAULT );
   frameBoxSizerV->Add(noteBook, 0, wxGROW, 5);

   communicationPanel = new USBDMPanel(noteBook, targetType);
   noteBook->AddPage(communicationPanel, _("USBDM"));

#if defined(FLASH_PROGRAMMER)
   flashParametersPanel = new FlashPanel(noteBook, targetType, bdmCapabilities);
   noteBook->AddPage(flashParametersPanel, _("Target"));
#endif

   advancedPanel = new AdvancedPanel(noteBook, targetType, bdmCapabilities);
   noteBook->AddPage(advancedPanel, _("Advanced"));

//------------------------------------------------
   wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
   frameBoxSizerV->Add(buttonSizer, 0, wxGROW|wxALL, 5);

#ifndef FLASH_PROGRAMMER
   wxButton* defaultButton = new wxButton( frame, wxID_DEFAULT, _("&Default"), wxDefaultPosition, wxDefaultSize, 0 );
   buttonSizer->Add(defaultButton, 0, wxGROW|wxALL, 5);
#endif

   buttonSizer->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

#ifdef FLASH_PROGRAMMER
   wxButton* okButton = new wxButton( frame, wxID_OK, _("C&lose"), wxDefaultPosition, wxDefaultSize, 0 );
#else
   wxButton* okButton = new wxButton( frame, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
#endif
   buttonSizer->Add(okButton, 0, wxGROW|wxALL, 5);
   okButton->SetDefault();

#ifndef FLASH_PROGRAMMER
   dontShowAgainControl = new wxCheckBox( frame, ID_DONT_SHOW_AGAIN_CHECKBOX, _("Don't show this dialogue in future"), wxDefaultPosition, wxDefaultSize, 0 );
   dontShowAgainControl->SetValue(false);
   frameBoxSizerV->Add(dontShowAgainControl, 0, wxALIGN_LEFT|wxALL, 5);
#endif
}

//===================================================================
//! Copy internal state to Dialogue controls
//!
bool USBDMDialogue::TransferDataToWindow() {

//   print("USBDMDialogue::TransferDataToWindow()\n");

#ifndef FLASH_PROGRAMMER
   dontShowAgainControl->Set3StateValue(disableDialogueDisplay?wxCHK_CHECKED:wxCHK_UNCHECKED);
#endif

   int rc = true;
//#if defined(FLASH_PROGRAMMER)
//   rc = flashParametersPanel->TransferDataToWindow();
//#endif
//   rc = rc &&
//        communicationPanel->TransferDataToWindow() &&
//        advancedPanel->TransferDataToWindow();
////   print("USBDMDialogue::TransferDataToWindow() - #2, rc = %d\n", rc);
   return rc;
}

//===================================================================
//! Copy internal state from Dialogue controls
//!
bool USBDMDialogue::TransferDataFromWindow() {

//   print("USBDMDialogue::TransferDataFromWindow()\n");

#ifndef FLASH_PROGRAMMER
   disableDialogueDisplay = dontShowAgainControl->IsChecked();
#endif

   int rc = true;
//#if defined(FLASH_PROGRAMMER)
//   rc = flashParametersPanel->TransferDataFromWindow();
//#endif
//   rc = rc &&
//          communicationPanel->TransferDataFromWindow() &&
//          advancedPanel->TransferDataFromWindow();
   return rc;
}

//===================================================================
//! Set dialogue state to default values
//!
bool USBDMDialogue::setDialogueValuesToDefault() {

   print("USBDMDialogue::setDialogueValuesToDefault()\n");

   Init();
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
//! @param name
//!
wxBitmap USBDMDialogue::GetBitmapResource( const wxString& name ) {
    // Bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
}

//! Get icon resources
//!
//! @param name
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
#ifdef GDI
   EVT_BUTTON( wxID_OK, USBDMDialogue::OnOkClick )
#endif
   EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK, USBDMDialogue::OnSelChanging )
   EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK, USBDMDialogue::OnSelChanged )
END_EVENT_TABLE()

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RECONNECT_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMDialogue::OnDontShowAgainControlClick( wxCommandEvent& event ) {
   disableDialogueDisplay = event.IsChecked();
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
   if (TransferDataFromWindow()) {
      EndModal(BDM_RC_OK);
   }
}

//! Returns the Device options from the internal state
//!
//! @param bdmOptions - where to return options
//!
void USBDMDialogue::getDeviceOptions( DeviceDataPtr deviceData ) {
   // Poll each page for changes
   advancedPanel->getDeviceOptions(deviceData);
};

//! Returns the BDM options from the internal state
//!
//! @param bdmOptions - where to return options
//!
void USBDMDialogue::getBdmOptions( USBDM_ExtendedOptions_t &bdmOptions ) {
   // Set options to default
   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = targetType;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);

   // Poll each page for changes
   communicationPanel->getDialogueValues(&bdmOptions);
   advancedPanel->getBdmOptions(&bdmOptions);
};

//! wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK
//!
//! @param event The event to handle.
//!
void USBDMDialogue::OnSelChanging( wxNotebookEvent& event ) {
//   print("USBDMDialogue::OnSelChanging(%d => %d)\n", event.GetOldSelection(), event.GetSelection());

   int leavingPage = event.GetOldSelection();
   if (leavingPage < 0) {
      return;
   }
   // Validate page before leaving
   wxPanel *panel = static_cast<wxPanel *>(noteBook->GetPage(leavingPage));
   if (!panel->TransferDataFromWindow()) {
      event.Veto();
      return;
   }
#ifdef FLASH_PROGRAMMER
   if (panel == communicationPanel) {
      // Leaving Communication page - Try to open BDM
//      print("USBDMDialogue::OnNotebookPageChanging() - opening BDM\n");
      USBDM_ErrorCode rc = communicationPanel->openBdm();
      if (rc != BDM_RC_OK) {
         print("USBDMDialogue::OnSelChanging() - openBdm() failed\n");
         wxMessageBox(_("Failed to open BDM.\n\n"
                        "Reason: ") +
                      wxString(USBDM_GetErrorString(rc), wxConvUTF8),
                      _("USBDM Connection Error"),
                      wxOK|wxICON_ERROR,
                      this
                      );
         event.Veto();
      }
   }
#endif
}

//! wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK
//!
//! @param event The event to handle.
//!
void USBDMDialogue::OnSelChanged( wxNotebookEvent& event ) {
//   print("USBDMDialogue::OnSelChanged(%d => %d)\n", event.GetOldSelection(), event.GetSelection());
#ifdef FLASH_PROGRAMMER
   int enteringPage = event.GetSelection();
   if (enteringPage < 0) {
      return;
   }
   wxPanel *panel = static_cast<wxPanel *>(noteBook->GetPage(enteringPage));
   if (panel == communicationPanel) {
      // Entering Communication page - Try to close BDM
      USBDM_Close();
      }
   if (panel == flashParametersPanel) {
      USBDM_ExtendedOptions_t bdmOptions;
      getBdmOptions(bdmOptions);
      flashParametersPanel->setBdmOptions(&bdmOptions);

      DeviceDataPtr deviceDataPtr(new DeviceData);
      getDeviceOptions(deviceDataPtr);
      flashParametersPanel->setDeviceOptions(deviceDataPtr);
   }
   if (panel == advancedPanel) {
      advancedPanel->setCurrentDevice(flashParametersPanel->getCurrentDevice());
   }
#endif
}

