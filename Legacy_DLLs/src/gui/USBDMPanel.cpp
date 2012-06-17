/*! \file
    \brief Implements USBDM Flash Programmer dialogue

    USBDMPanel.cpp

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
   -==================================================================================
   | 30 Mar 2012 | Updated to using USBDM_ExtendedOptions                  - pgo V4.9
   | 26 Oct 2010 | Added PST capability checks on dialogue                 - pgo
   |  1 Jul 2010 | wxWidgets version created                               - pgo
   +==================================================================================
   \endverbatim
*/

// For compilers that support pre-compilation, includes <wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/imaglist.h>
#include <wx/gbsizer.h>

#include "Common.h"
#include "USBDMPanel.h"
#include "USBDM_API.h"
#include "USBDM_AUX.h"
#include "ApplicationFiles.h"
#include "Log.h"
#if TARGET == ARM
#include "USBDM_ARM_API.h"
#endif

const wxString USBDMPanel::settingsKey(_("usbdm"));

//===================================================================
//! Obtains the version of the BDM library as a string
//!
//! @return Version string (reference to static buffer)
//!
static const wxString &bdmGetDllVersion(void) {
   static wxString versionString;

   versionString = wxString(USBDM_DLLVersionString(), wxConvUTF8);

   versionString.Prepend(_("DLL Ver "));

   return versionString; // Software version of USBDM DLL
}

//! Structure to describe a configuration setting
//!
typedef struct {
   const wxString name;   //!< Name of setting
   int * const value;     //!< Location to modify
} ConfigLine;

//! Maps between a drop-down box 'name' and its value
typedef struct {
   int   value;           //!< Value to use
   const wxString name;   //!< Name to display in DD box
} DropDownType;

//! Mappings for Frequency drop-down box
static const DropDownType CFVx_Speeds[] = {
   {  250, _("250kHz") },
   {  500, _("500kHz") },
   {  750, _("750kHz") },
   { 1000, _("1MHz")   },
   { 1500, _("1.5MHz") },
   { 2000, _("2MHz")   },
   { 3000, _("3MHz")   },
   { 4000, _("4MHz")   },
   { 6000, _("6MHz")   },
   {12000, _("12MHz")  }, // Not supported on JTAG etc
   {  0,   wxEmptyString },
};

//===================================================================
//! Maps a value to a drop-down box index
//!
//! @param information  Array of data describing the DD box
//! @param value        Value to locate
//!
//! @return Dropdown box index, -1 if value not present
//!
static int searchDropDown(const DropDownType information[], int value) {
int sub;

   // Populate the list
   for (sub = 0; information[sub].value != 0; sub++)
      if (information[sub].value == value)
         return sub;
   return -1;
}

//! Load settings
//!
//! @param settings      - Object to load settings from
//!
//! @note - Settings may be filtered by by target type or BDM capabilities.
//!
void USBDMPanel::loadSettings(const AppSettings &settings) {

   print("USBDMPanel::loadSettings()\n");

   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = targetType;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);

   bdmOptions.targetVdd          = (TargetVddSelect_t)settings.getValue(settingsKey+_(".setTargetVdd"),             bdmOptions.targetVdd);
   bdmOptions.cycleVddOnReset    =                    settings.getValue(settingsKey+_(".cycleTargetVddOnReset"),    bdmOptions.cycleVddOnReset);
   bdmOptions.cycleVddOnConnect  =                    settings.getValue(settingsKey+_(".cycleTargetVddonConnect"),  bdmOptions.cycleVddOnConnect);
   bdmOptions.leaveTargetPowered =                    settings.getValue(settingsKey+_(".leaveTargetPowered"),       bdmOptions.leaveTargetPowered);
   bdmOptions.autoReconnect      =    (AutoConnect_t) settings.getValue(settingsKey+_(".automaticReconnect"),       bdmOptions.autoReconnect);
   bdmOptions.guessSpeed         =                    settings.getValue(settingsKey+_(".guessSpeedIfNoSYNC"),       bdmOptions.guessSpeed);
   bdmOptions.bdmClockSource     =    (ClkSwValues_t) settings.getValue(settingsKey+_(".bdmClockSource"),           bdmOptions.bdmClockSource);
   bdmOptions.useResetSignal     =                    settings.getValue(settingsKey+_(".useResetSignal"),           bdmOptions.useResetSignal);
   bdmOptions.maskInterrupts     =                    settings.getValue(settingsKey+_(".maskInterrupts"),           bdmOptions.maskInterrupts);
   bdmOptions.interfaceFrequency =                    settings.getValue(settingsKey+_(".targetClockFrequency"),     bdmOptions.interfaceFrequency);
   bdmOptions.usePSTSignals      =                    settings.getValue(settingsKey+_(".usePSTSignals"),            bdmOptions.usePSTSignals);

   bdmIdentification             =                    settings.getValue(settingsKey+_(".bdmSelection"),             wxEmptyString);

//   print("USBDMPanel::loadSettings() - bdmIdentification=%s \n", (const char *)bdmIdentification.ToAscii());

   findBDMs();
}

//! Save settings
//!
//! @param settings      - Object to save settings to
//!
//! @note - Settings may be filtered by by target type or BDM capabilities.
//!
void USBDMPanel::saveSettings(AppSettings &settings) {
USBDM_ExtendedOptions_t bdmOptions;

   print("USBDMPanel::saveSettings()\n");

   getDialogueValues(&bdmOptions);

   settings.addValue(settingsKey+_(".setTargetVdd"),             bdmOptions.targetVdd);
   settings.addValue(settingsKey+_(".cycleTargetVddOnReset"),    bdmOptions.cycleVddOnReset);
   settings.addValue(settingsKey+_(".cycleTargetVddonConnect"),  bdmOptions.cycleVddOnConnect);
   settings.addValue(settingsKey+_(".leaveTargetPowered"),       bdmOptions.leaveTargetPowered);
   settings.addValue(settingsKey+_(".automaticReconnect"),       bdmOptions.autoReconnect);
   settings.addValue(settingsKey+_(".guessSpeedIfNoSYNC"),       bdmOptions.guessSpeed);
   settings.addValue(settingsKey+_(".bdmClockSource"),           bdmOptions.bdmClockSource);
   settings.addValue(settingsKey+_(".useResetSignal"),           bdmOptions.useResetSignal);
   settings.addValue(settingsKey+_(".maskInterrupts"),           bdmOptions.maskInterrupts);
   settings.addValue(settingsKey+_(".interfaceFrequency"),       bdmOptions.interfaceFrequency);
   settings.addValue(settingsKey+_(".usePSTSignals"),            bdmOptions.usePSTSignals);

   settings.addValue(settingsKey+_(".bdmSelection"),             bdmIdentification);
}

//! Get bdm options from dialogue
//!
//! @param _bdmOptions  Options structure to modify
//!
//! @note - _bdmOptions should be set to default values beforehand
//!
void USBDMPanel::getDialogueValues(USBDM_ExtendedOptions_t *_bdmOptions) {

   print("USBDMPanel::getDialogueValues()\n");

   _bdmOptions->targetVdd          =  bdmOptions.targetVdd;
   _bdmOptions->cycleVddOnReset    =  bdmOptions.cycleVddOnReset;
   _bdmOptions->cycleVddOnConnect  =  bdmOptions.cycleVddOnConnect;
   _bdmOptions->leaveTargetPowered =  bdmOptions.leaveTargetPowered;
   _bdmOptions->autoReconnect      =  bdmOptions.autoReconnect;
   _bdmOptions->guessSpeed         =  bdmOptions.guessSpeed;
   _bdmOptions->bdmClockSource     =  bdmOptions.bdmClockSource;
   _bdmOptions->useResetSignal     =  bdmOptions.useResetSignal;
   _bdmOptions->maskInterrupts     =  bdmOptions.maskInterrupts;
   _bdmOptions->interfaceFrequency =  bdmOptions.interfaceFrequency;
   _bdmOptions->usePSTSignals      =  bdmOptions.usePSTSignals;
}

USBDMPanel::USBDMPanel( wxWindow* parent, TargetType_t targetType) {
   this->targetType = targetType;
   Init();
   Create(parent);
}

USBDMPanel::USBDMPanel(TargetType_t targetType) {
   this->targetType = targetType;
   Init();
}

//===================================================================
//! Set the dialogue internal state to the default
//!
void USBDMPanel::Init() {

//   print("USBDMPanel::Init()\n");

   bdmDeviceNum = -1;
   
   // Set options to default
   // TransferDataToWindow() will validate these for the particular dialog/BDM being used.
   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = targetType;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);

   findBDMs();
}

//! USBDMParametersDialogue creator
//!
//! @param parent     : Parent window
//!
//! @return error code
//! - BDM_RC_OK => success
//! - else => failed
USBDM_ErrorCode USBDMPanel::Create(wxWindow* parent) {
//   print("USBDMPanel::Create()\n");

   if (!wxPanel::Create( parent, ID_COMMUNICATION, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL))
      return BDM_RC_FAIL;

   CreateControls();
   if (GetSizer()) {
       GetSizer()->SetSizeHints(this);
   }
   return BDM_RC_OK;
}

//! Control creation for USBDM Communication settings
//!
//! @return error code
//! - BDM_RC_OK => success
//! - else => failed
USBDM_ErrorCode USBDMPanel::CreateControls(void) {

//   print("USBDMPanel::CreateControls()\n");

   // Determine dialogue features
   switch(targetType) {
      case T_HC12 :
         targetProperties = (TargetOptions)(HAS_CLK_SW|HAS_GUESS_SPEED);
         break;
      case T_HCS08 :
         targetProperties = (TargetOptions)(HAS_CLK_SW|HAS_USE_RESET|HAS_MASK_INTERRUPTS);
         break;
      case T_CFV1 :
         targetProperties = (TargetOptions)(HAS_CLK_SW|HAS_USE_RESET);
         break;
      default :
         targetProperties = HAS_NONE;
         break;
   }
   wxPanel* panel = this;
   wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
   panel->SetSizer(panelSizer);

   wxStaticBox      *staticTextBox;
   wxStaticBoxSizer *staticBoxSizer;
   wxBoxSizer       *itemBoxSizer;

   //-----------------------------------------------
   staticTextBox = new wxStaticBox(panel, wxID_ANY, _("Select BDM"));
   staticBoxSizer = new wxStaticBoxSizer(staticTextBox, wxVERTICAL);
   panelSizer->Add(staticBoxSizer, 0, wxGROW|wxALL, 5);

   itemBoxSizer = new wxBoxSizer(wxHORIZONTAL);
   staticBoxSizer->Add(itemBoxSizer, 0, wxGROW|wxALL, 5);

   bdmSelectChoiceControl = new wxChoice( panel, ID_BDM_SELECT_CHOICE, wxDefaultPosition, wxDefaultSize);
   itemBoxSizer->Add(bdmSelectChoiceControl, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
   populateBDMChoices();

   bdmRefreshButtonControl = new wxButton( panel, ID_REFRESH_BDM_BUTTON, _("&Detect"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer->Add(bdmRefreshButtonControl, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   bdmDescriptionStaticControl = new wxStaticText( panel, ID_BDM_DESCRIPTION_STRING, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
   staticBoxSizer->Add(bdmDescriptionStaticControl, 0, wxGROW|wxALL, 5);

   //-----------------------------------------------
   staticTextBox = new wxStaticBox(panel, wxID_ANY, _("Target Vdd Control"));
   staticBoxSizer = new wxStaticBoxSizer(staticTextBox, wxVERTICAL);
   panelSizer->Add(staticBoxSizer, 0, wxGROW|wxALL, 5);

   itemBoxSizer = new wxBoxSizer(wxVERTICAL);
   staticBoxSizer->Add(itemBoxSizer, 0, wxGROW|wxALL, 5);

   wxArrayString targetVddControlStrings;
   targetVddControlStrings.Add(_("&Off"));
   targetVddControlStrings.Add(_("&3.3V"));
   targetVddControlStrings.Add(_("&5V"));
   targetVddControl = new wxRadioBox( panel, ID_VDD_SELECT_BOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, targetVddControlStrings, 1, wxRA_SPECIFY_ROWS );
   targetVddControl->SetSelection(0);
   itemBoxSizer->Add(targetVddControl, 0, wxGROW|wxALL, 5);

   cycleVddOnResetControl = new wxCheckBox( panel, ID_CYCLE_VDD_ON_RESET_CHECKBOX, _("&Cycle target Vdd on reset"), wxDefaultPosition, wxDefaultSize, 0 );
   cycleVddOnResetControl->SetValue(false);
   itemBoxSizer->Add(cycleVddOnResetControl, 0, wxLEFT|wxRIGHT|wxTOP, 10);

   cycleVddOnConnectionControl = new wxCheckBox( panel, ID_CYCLE_TARGET_VDD_ON_CONNECTION_CHECKBOX, _("Cycle &target Vdd on connection problems"), wxDefaultPosition, wxDefaultSize, 0 );
   cycleVddOnConnectionControl->SetValue(false);
   itemBoxSizer->Add(cycleVddOnConnectionControl, 0, wxLEFT|wxRIGHT|wxTOP, 10);

   leaveTargetPoweredControl = new wxCheckBox( panel, ID_LEAVE_TARGET_ON_CHECKBOX, _("&Leave target powered on exit"), wxDefaultPosition, wxDefaultSize, 0 );
   leaveTargetPoweredControl->SetValue(false);
   itemBoxSizer->Add(leaveTargetPoweredControl, 0, wxALL, 10);

//   promptToManualCycleVddControl = new wxCheckBox( panel, ID_MANUALLY_CYCLE_VDD_CHECKBOX, _("&Prompt to manually cycle target Vdd"), wxDefaultPosition, wxDefaultSize, 0 );
//   promptToManualCycleVddControl->SetValue(false);
//   itemBoxSizer->Add(promptToManualCycleVddControl, 0, wxALL, 5);

   //-----------------------------------------------
   staticTextBox = new wxStaticBox(panel, wxID_ANY, _("Connection control"));
   staticBoxSizer = new wxStaticBoxSizer(staticTextBox, wxVERTICAL);
   panelSizer->Add(staticBoxSizer, 0, wxGROW|wxALL, 5);
   automaticallyReconnectControl = new wxCheckBox( panel, ID_RECONNECT_CHECKBOX, _("&Automatically re-connect"), wxDefaultPosition, wxDefaultSize, 0 );
   automaticallyReconnectControl->SetValue(false);
   staticBoxSizer->Add(automaticallyReconnectControl, 0, wxALL, 5);

   if (targetProperties & HAS_CLK_SW) {
      wxArrayString bdmClockSelectControlStrings;
      bdmClockSelectControlStrings.Add(_("&Default"));
      bdmClockSelectControlStrings.Add(_("&Bus Clock/2"));
      bdmClockSelectControlStrings.Add(_("&Alt"));
      bdmClockSelectControl = new wxRadioBox( panel, ID_BDM_CLOCK_SELECT_RADIOBOX, _("BDM Clock Select"), wxDefaultPosition, wxDefaultSize, bdmClockSelectControlStrings, 1, wxRA_SPECIFY_ROWS );
      bdmClockSelectControl->SetSelection(0);
      staticBoxSizer->Add(bdmClockSelectControl, 0, wxGROW|wxALL, 5);
   }
   if (targetProperties & HAS_GUESS_SPEED) {
      guessTargetSpeedControl = new wxCheckBox( panel, ID_GUESS_SPEED_CHECKBOX, _("&Guess speed if no sync"), wxDefaultPosition, wxDefaultSize, 0 );
      guessTargetSpeedControl->SetValue(false);
      staticBoxSizer->Add(guessTargetSpeedControl, 0, wxGROW|wxALL, 5);
   }
   if (targetProperties & HAS_USE_RESET) {
      useResetSignalControl = new wxCheckBox( panel, ID_USE_RESET_CHECKBOX, _("Use &RESET signal"), wxDefaultPosition, wxDefaultSize, 0 );
      useResetSignalControl->SetValue(false);
      staticBoxSizer->Add(useResetSignalControl, 0, wxLEFT|wxALL, 5);
   }
   if (targetType == T_CFVx) {
      usePstSignalControl = new wxCheckBox( panel, ID_USE_PST_SIGNAL_CHECKBOX, _("Monitor &PST signals"), wxDefaultPosition, wxDefaultSize, 0 );
      usePstSignalControl->SetValue(false);
      staticBoxSizer->Add(usePstSignalControl, 0, wxLEFT|wxALL, 5);
   }
   if (targetProperties & HAS_MASK_INTERRUPTS) {
      maskInterruptWhenSteppingControl = new wxCheckBox( panel, ID_MASK_INTERRUPTS_WHEN_STEPPING, _("Mask interrupts when stepping"), wxDefaultPosition, wxDefaultSize, 0 );
      maskInterruptWhenSteppingControl->SetValue(false);
      staticBoxSizer->Add(maskInterruptWhenSteppingControl, 0, wxLEFT|wxALL, 5);
   }

   //------------------------------------------------
   wxBoxSizer* versionBoxSizerH = new wxBoxSizer(wxHORIZONTAL);
   panelSizer->Add(versionBoxSizerH, 0, wxGROW|wxALL, 0);

   versionStaticControl = new wxStaticText( panel, ID_BDM_VERSION_STRING, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
   versionBoxSizerH->Add(versionStaticControl, 0, wxGROW|wxALL, 5);

   versionBoxSizerH->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxStaticText *dllVersionstaticControl = new wxStaticText( panel, ID_DLL_VERSION_STRING, bdmGetDllVersion(), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
   versionBoxSizerH->Add(dllVersionstaticControl, 0, wxGROW|wxALL, 5);

   return BDM_RC_OK;
}

//===================================================================
//! Set the internal dialogue values.
//!
//!
bool USBDMPanel::setDialogueValuesToDefault() {

//   print("USBDMPanel::setDialogueValuesToDefault()\n");
   Init();
   TransferDataToWindow();
   return true;
}

//===================================================================
//! Update the dialogue from internal state
//!
bool USBDMPanel::TransferDataToWindow() {

   print("USBDMPanel::TransferDataToWindow(), BDM = \'%s\'\n", (const char *)bdmIdentification.ToAscii());
   HardwareCapabilities_t  bdmCapabilities;

   if (!bdmIdentification.IsEmpty() && bdmSelectChoiceControl->SetStringSelection(bdmIdentification)) {
      bdmDeviceNum = bdmSelectChoiceControl->GetSelection();
   }
   if (bdmDeviceNum >= 0) {
      bdmCapabilities = bdmInformation[bdmDeviceNum].capabilities;
      wxString versionString;
      USBDM_Version_t version = bdmInformation[bdmDeviceNum].version;
      versionString.Printf(_("BDM Firmware Ver %d.%d"),
                           (version.bdmSoftwareVersion>>4)&0x0F,
                           version.bdmSoftwareVersion&0x0F);
      versionStaticControl->SetLabel(versionString);
      bdmDescriptionStaticControl->SetLabel(bdmInformation[bdmDeviceNum].description);
   }
   else {
      bdmCapabilities = BDM_CAP_NONE;
      versionStaticControl->SetLabel(wxEmptyString);
      bdmDescriptionStaticControl->SetLabel(wxEmptyString);
   }
   if (!(bdmCapabilities & BDM_CAP_VDDCONTROL)) {
      // BDM doesn't have Vdd control - Vdd control options disabled
      targetVddControl->Enable(false);
      bdmOptions.targetVdd          = BDM_TARGET_VDD_OFF;
      bdmOptions.cycleVddOnReset    = false;
      bdmOptions.leaveTargetPowered = false;
      bdmOptions.cycleVddOnConnect  = false;
   }
   else
      targetVddControl->Enable(true);
   targetVddControl->Select(bdmOptions.targetVdd);

   bool checkBoxEnable = (bdmOptions.targetVdd != BDM_TARGET_VDD_OFF);
   cycleVddOnResetControl->Enable(checkBoxEnable);
   cycleVddOnConnectionControl->Enable(checkBoxEnable);
   leaveTargetPoweredControl->Enable(checkBoxEnable);
   cycleVddOnConnectionControl->Set3StateValue(bdmOptions.cycleVddOnConnect?wxCHK_CHECKED:wxCHK_UNCHECKED);
   cycleVddOnResetControl->Set3StateValue(bdmOptions.cycleVddOnReset?wxCHK_CHECKED:wxCHK_UNCHECKED);
   leaveTargetPoweredControl->Set3StateValue(bdmOptions.leaveTargetPowered?wxCHK_CHECKED:wxCHK_UNCHECKED);

   if (!(bdmCapabilities & (BDM_CAP_RST|BDM_CAP_HCS12))) {
      // BDM doesn't support reset control - Reset must be false
      // (Control not present in dialogue)
      bdmOptions.useResetSignal  = false;
   }
   else if (targetProperties & HAS_USE_RESET) {
      // Reset control present
      useResetSignalControl->Enable(true);
      useResetSignalControl->Set3StateValue(bdmOptions.useResetSignal?wxCHK_CHECKED:wxCHK_UNCHECKED);
   }
   else {// Reset control not present in dialogue as reset MUST be used
      bdmOptions.useResetSignal = true;
   }
   if (targetType == T_CFVx) {
      if (bdmCapabilities & BDM_CAP_PST) {
         // BDM supports PST monitoring
         usePstSignalControl->Set3StateValue(bdmOptions.usePSTSignals?wxCHK_CHECKED:wxCHK_UNCHECKED);
      }
      else {
         usePstSignalControl->Set3StateValue(wxCHK_UNCHECKED);
         usePstSignalControl->Enable(false);
         bdmOptions.usePSTSignals = false;
      }
   }
   if (targetProperties & HAS_GUESS_SPEED) {
      // Guess speed control is present
      guessTargetSpeedControl->Set3StateValue(bdmOptions.guessSpeed?wxCHK_CHECKED:wxCHK_UNCHECKED);
   }
   else {
      // Control not present and option unused
      bdmOptions.guessSpeed = false;
   }
   if (targetProperties & HAS_CLK_SW) {
      // Alternative clock available on this target & present in dialogue
      int index;
      switch (bdmOptions.bdmClockSource) {
		 default:
         case CS_DEFAULT :      index = 0; break;
         case CS_NORMAL_CLK :   index = 1; break;
         case CS_ALT_CLK :      index = 2; break;
      }
      bdmClockSelectControl->Select(index);
   }
   else {
      // No BDM clock options
      bdmOptions.bdmClockSource = CS_DEFAULT;
   }
//   promptToManualCycleVddControl->Set3StateValue(bdmOptions.manuallyCycleVdd?wxCHK_CHECKED:wxCHK_UNCHECKED);

   automaticallyReconnectControl->Set3StateValue(bdmOptions.autoReconnect?wxCHK_CHECKED:wxCHK_UNCHECKED);

   if (targetProperties & HAS_SELECT_SPEED) {
      int index = searchDropDown(CFVx_Speeds, bdmOptions.interfaceFrequency);
      if (index < 0)
         index = 0;
      connectionSpeedControl->SetSelection(index);
   }
   if (targetProperties & HAS_MASK_INTERRUPTS) {
      maskInterruptWhenSteppingControl->Set3StateValue(bdmOptions.maskInterrupts?wxCHK_CHECKED:wxCHK_UNCHECKED);
   }
   return true;
}

bool USBDMPanel::TransferDataFromWindow() {

   print("USBDMPanel::TransferDataFromWindow()\n");
   // NOP - it is assumed that the internal state is always kept consistent with the controls
   return true;
}

/*
 * USBDMParametersDialogue type definition
 */
IMPLEMENT_CLASS( USBDMPanel, wxPanel )

/*
 * USBDMPanel event table definition
 */
BEGIN_EVENT_TABLE( USBDMPanel, wxPanel )
   EVT_CHOICE(   ID_BDM_SELECT_CHOICE,                         USBDMPanel::OnBDMSelectComboSelected )
   EVT_BUTTON(   ID_REFRESH_BDM_BUTTON,                        USBDMPanel::OnRefreshBDMClick )
   EVT_RADIOBOX( ID_VDD_SELECT_BOX,                            USBDMPanel::OnVddSelectBoxSelected )
   EVT_CHECKBOX( ID_CYCLE_VDD_ON_RESET_CHECKBOX,               USBDMPanel::OnCycleVddOnResetCheckboxClick )
   EVT_CHECKBOX( ID_CYCLE_TARGET_VDD_ON_CONNECTION_CHECKBOX,   USBDMPanel::OnCycleTargetVddOnConnectionCheckboxClick )
   EVT_CHECKBOX( ID_LEAVE_TARGET_ON_CHECKBOX,                  USBDMPanel::OnLeaveTargetOnCheckboxClick )
//   EVT_CHECKBOX( ID_MANUALLY_CYCLE_VDD_CHECKBOX,               USBDMPanel::OnManuallyCycleVddCheckboxClick )
   EVT_CHECKBOX( ID_RECONNECT_CHECKBOX,                        USBDMPanel::OnReconnectCheckboxClick )
   EVT_CHECKBOX( ID_GUESS_SPEED_CHECKBOX,                      USBDMPanel::OnGuessSpeedCheckboxClick )
   EVT_CHECKBOX( ID_USE_RESET_CHECKBOX,                        USBDMPanel::OnUseResetCheckboxClick )
   EVT_CHECKBOX( ID_USE_PST_SIGNAL_CHECKBOX,                   USBDMPanel::OnUseUsePstSignalCheckboxClick )
   EVT_RADIOBOX( ID_BDM_CLOCK_SELECT_RADIOBOX,                 USBDMPanel::OnBdmClockSelectRadioboxSelected )
   EVT_CHOICE(   ID_SPEED_SELECT_CHOICE,                       USBDMPanel::OnSpeedSelectComboSelected )
   EVT_CHECKBOX( ID_MASK_INTERRUPTS_WHEN_STEPPING,             USBDMPanel::OnMaskInterruptsWhenSteppingCheckboxClick )
END_EVENT_TABLE()

//! wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_VDD_SELECT_BOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnVddSelectBoxSelected( wxCommandEvent& event ) {

   switch(event.GetSelection()) {
      case 1  :    bdmOptions.targetVdd = BDM_TARGET_VDD_3V3; break;
      case 2  :    bdmOptions.targetVdd = BDM_TARGET_VDD_5V;  break;
      case 0  :
      default :    bdmOptions.targetVdd = BDM_TARGET_VDD_OFF; break;
   }
   bool checkBoxEnable = (bdmOptions.targetVdd != BDM_TARGET_VDD_OFF);
   cycleVddOnResetControl->Enable(checkBoxEnable);
   cycleVddOnConnectionControl->Enable(checkBoxEnable);
   leaveTargetPoweredControl->Enable(checkBoxEnable);
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CYCLE_VDD_ON_RESET_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnCycleVddOnResetCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.cycleVddOnReset = event.IsChecked();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CYCLE_TARGET_VDD_ON_CONNECTION_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnCycleTargetVddOnConnectionCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.cycleVddOnConnect = event.IsChecked();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_LEAVE TARGET_ON_CHECKBOX
//!
void USBDMPanel::OnLeaveTargetOnCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.leaveTargetPowered = event.IsChecked();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_MANUALLY_CYCLE_VDD_CHECKBOX
//!
//! @param event The event to handle
//!
//void USBDMPanel::OnManuallyCycleVddCheckboxClick( wxCommandEvent& event ) {
//   bdmOptions.manuallyCycleVdd = event.IsChecked();
//}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_RECONNECT_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnReconnectCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.autoReconnect = event.IsChecked()?AUTOCONNECT_STATUS:AUTOCONNECT_NEVER;
}

//! wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_BDM_CLOCK_SELECT_RADIOBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnBdmClockSelectRadioboxSelected( wxCommandEvent& event ) {

   switch(event.GetSelection()) {
      case 1  :    bdmOptions.bdmClockSource = CS_NORMAL_CLK; break;
      case 2  :    bdmOptions.bdmClockSource = CS_ALT_CLK;  break;
      case 0  :
      default :    bdmOptions.bdmClockSource = CS_DEFAULT; break;
   }
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_USE_RESET_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnUseResetCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.useResetSignal = event.IsChecked();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_USE_PST_SIGNAL_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnUseUsePstSignalCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.usePSTSignals = event.IsChecked();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_GUESS_SPEED_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnGuessSpeedCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.guessSpeed = event.IsChecked();
}

//! wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_SPEED_SELECT_COMBO
//!
//! @param event The event to handle
//!
void USBDMPanel::OnSpeedSelectComboSelected( wxCommandEvent& event ) {
   bdmOptions.interfaceFrequency = CFVx_Speeds[event.GetSelection()].value;
//   print("USBDMPanel::OnSpeedSelectComboSelected() sel = %d, f = %d\n", event.GetSelection(), bdmOptions.interfaceSpeed);
}

//! Opens the currently selected BDM & sets options
//!
USBDM_ErrorCode USBDMPanel::openBdm(void) {

//   print("USBDMPanel::openBdm(), deviceNum = %d\n", bdmDeviceNum);

   USBDM_ErrorCode rc = BDM_RC_OK;

   if (bdmDeviceNum < 0) { // No devices available
      rc = BDM_RC_NO_USBDM_DEVICE;
   }
   else {
      rc = USBDM_Open(bdmDeviceNum);
   }
   return rc;
}

//! Opens the currently selected BDM
//!
USBDM_ErrorCode USBDMPanel::setTarget(void) {

//   print("USBDMPanel::setTarget()\n");

   USBDM_ErrorCode rc = BDM_RC_OK;

   rc = USBDM_SetTargetType(targetType);
   return rc;
}

//! Update the list of connected BDMs
//!
void USBDMPanel::findBDMs(void) {
//   print("USBDMPanel::findBDMs()\n");

	//   USBDM_Close(); // Close any open devices
	   unsigned deviceCount;
	   USBDM_FindDevices(&deviceCount);

	   bdmInformation.clear();

	   print("USBDMPanel::findBDMs(), bdmIdentification = \'%s\'\n", (const char *)bdmIdentification.ToAscii());
	   if (deviceCount==0) {
	      bdmDeviceNum = -1;
	      bdmIdentification = wxEmptyString;
	      print("USBDMPanel::findBDMs() - no devices\n");
	      return;
	   }
	   bdmDeviceNum = 0; // Default to 1st device in list
      int unknownBdmCount = 0;
	   for (unsigned index=0; index<deviceCount; index++) {
	      HardwareCapabilities_t bdmCapabilities;
	      USBDM_Version_t usbdmVersion = {0,0,0,0};
	      BdmInformation bdmInfo(index, _("Device not responding"), _("Unknown"),
	                             BDM_CAP_NONE, usbdmVersion);
	      do {
	         if (USBDM_Open(index) != BDM_RC_OK)
	            break;
	         if (USBDM_GetVersion(&usbdmVersion) != BDM_RC_OK)
	            break;
	         bdmInfo.version      = usbdmVersion;
            USBDM_GetBDMDescription(bdmInfo.description);
	         if (usbdmVersion.bdmSoftwareVersion < 0x31) {
	            bdmInfo.serialNumber.Printf(_("Firmware obsolete #%d"), ++unknownBdmCount);
	            break;
	         }
	         if (USBDM_GetCapabilities(&bdmCapabilities) != BDM_RC_OK)
	            break;
	         // Check capabilities again target needs
	         bool okTarget = false;
	         switch(targetType) {
                case T_HC12      : okTarget = (bdmCapabilities & BDM_CAP_HCS12)    != 0;  break;
                case T_HCS08     : okTarget = (bdmCapabilities & BDM_CAP_HCS08)    != 0;  break;
                case T_RS08      : okTarget = (bdmCapabilities & BDM_CAP_RS08)     != 0;  break;
                case T_MC56F80xx : okTarget = (bdmCapabilities & BDM_CAP_DSC)      != 0;  break;
                case T_CFV1      : okTarget = (bdmCapabilities & BDM_CAP_CFV1)     != 0;  break;
                case T_CFVx      : okTarget = (bdmCapabilities & BDM_CAP_CFVx)     != 0;  break;
                case T_JTAG      : okTarget = (bdmCapabilities & BDM_CAP_JTAG)     != 0;  break;
                case T_ARM_JTAG  : okTarget = (bdmCapabilities & BDM_CAP_ARM_JTAG) != 0;  break;
	            default :
	                break;
	         }
	         if (!okTarget) {
	            bdmInfo.serialNumber = _("Target not supported by BDM");
	            break;
	         }
	         bdmInfo.capabilities = bdmCapabilities;
             if (USBDM_GetBDMSerialNumber(bdmInfo.serialNumber) != BDM_RC_OK)
                break;
	      } while (false);
	      USBDM_Close();
	      bdmInformation.push_back(bdmInfo);
	      if ((bdmDeviceNum == 0) && (bdmIdentification.IsSameAs(bdmInfo.serialNumber, false))) {
		      print("USBDMPanel::findBDMs() - selecting previously used BDM\n");
	    	  bdmDeviceNum = index;
	      }
	   }
	   print("USBDMPanel::findBDMs() - %d devices located\n", deviceCount);
}

//! Populate the BDM Choice box
//!
void USBDMPanel::populateBDMChoices(void) {
//   print("USBDMPanel::populateBDMChoices()\n");

//   USBDM_Close(); // Close any open devices

   findBDMs();
   unsigned deviceCount = bdmInformation.size();
   bdmSelectChoiceControl->Clear();

//   print("USBDMPanel::populateBDMChoices(), bdmIdentification = \'%s\'\n", (const char *)bdmIdentification.ToAscii());
   if (deviceCount==0) {
      // No devices found
      bdmSelectChoiceControl->Append(_("No devices Found"));
      bdmSelectChoiceControl->Enable(false);
      bdmSelectChoiceControl->Select(0);
      print("USBDMPanel::populateBDMChoices() - no devices\n");
      return;
   }

   // Add device names to choice box
   vector<BdmInformation>::iterator it;
   for ( it=bdmInformation.begin(); it < bdmInformation.end(); it++ ) {
      bdmSelectChoiceControl->Append(it->serialNumber);
   }
   // Select 1st device by default
   bdmSelectChoiceControl->Select(0);
   bdmDeviceNum = 0;

   bdmSelectChoiceControl->Enable(deviceCount>1);
   print("USBDMPanel::populateBDMChoices() - %d devices\n", deviceCount);
}

//! wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_DEFAULT
//!
//! @param event The event to handle
//!
void USBDMPanel::OnRefreshBDMClick( wxCommandEvent& event ) {
//   print("USBDMPanel::OnRefreshBDMClick()\n");
   populateBDMChoices();
   TransferDataToWindow();
}

//! wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_SPEED_SELECT_COMBO
//!
//! @param event The event to handle
//!
void USBDMPanel::OnBDMSelectComboSelected( wxCommandEvent& event ) {
//   print("USBDMPanel::OnBDMSelectComboSelected()\n");

   bdmDeviceNum = event.GetSelection();
   bdmIdentification = bdmInformation[bdmDeviceNum].serialNumber;

   TransferDataToWindow();
}

//! wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_GUESS_SPEED_CHECKBOX
//!
//! @param event The event to handle
//!
void USBDMPanel::OnMaskInterruptsWhenSteppingCheckboxClick( wxCommandEvent& event ) {
   bdmOptions.maskInterrupts = event.IsChecked();
}

