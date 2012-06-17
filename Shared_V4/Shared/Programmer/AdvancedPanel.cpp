/*! \file
    \brief Implements Dummy Panel displaying a message

   AdvancedPanel.cpp

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


// For compilers that support precompilation, includes <wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/imaglist.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>
#include "Common.h"
#include "AdvancedPanel.h"
#include "Log.h"

const string settingsKey("AdvancedPanel");

//========================================================================================================================
// AdvancedPanel

AdvancedPanel::AdvancedPanel( wxWindow* parent, TargetType_t targetType, HardwareCapabilities_t bdmCapabilities) {
   Init();
   Create(parent);
}

AdvancedPanel::AdvancedPanel(TargetType_t targetType, HardwareCapabilities_t bdmCapabilities) {
   Init();
}

bool AdvancedPanel::Create(wxWindow* parent) {

   print("AdvancedPanel::Create()\n");

   if (!wxPanel::Create(parent) || !CreateControls())
      return false;

   return true;
}

//===================================================================
//! Set the dialogue internal state to the default
//!
void AdvancedPanel::Init() {

   print("AdvancedPanel::Init()\n");

   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = T_OFF;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);
}

enum {
   ID_POWER_OFF_DURATION_TEXT,
   ID_POWER_RECOVERY_INTERVAL_TEXT,
   ID_RESET_DURATION_TEXT,
   ID_RESET_RELEASE_INTERVAL_TEXT,
   ID_RESET_RECOVERY_INTERVAL_TEXT,
};

//! Control creation for USBDM Flash programming settings
//!
bool AdvancedPanel::CreateControls() {

   print("AdvancedPanel::CreateControls()\n");

   wxPanel* panel = this;
   wxBoxSizer* panelBoxSizerV = new wxBoxSizer(wxVERTICAL);
   panel->SetSizer(panelBoxSizerV);

   wxStaticBox* itemStaticBox = new wxStaticBox(panel, wxID_ANY, _("BDM Parameters"));
   wxStaticBoxSizer* itemStaticBoxSizer = new wxStaticBoxSizer(itemStaticBox, wxHORIZONTAL);
   panelBoxSizerV->Add(itemStaticBoxSizer, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

   wxGridBagSizer* gridBagSizer = new wxGridBagSizer(0,0);
   itemStaticBoxSizer->Add(gridBagSizer, 0, wxGROW|wxALL, 5);

   wxStaticText* itemStaticText;
   int row = 0;

   TimeIntervalValidator timeIntervalValidator(NULL, NULL, 100, 10000);

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Power Off duration"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,2), wxLEFT|wxRIGHT|wxTOP, 5);

   timeIntervalValidator.setObject("Power Off Duration", &bdmOptions.powerOffDuration);
   powerOffDurationTextControl = new NumberTextEditCtrl( panel, ID_POWER_OFF_DURATION_TEXT, wxEmptyString, wxDefaultPosition, wxSize(80, -1), 0, timeIntervalValidator );
   gridBagSizer->Add(powerOffDurationTextControl, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   powerOffDurationTextControl->SetToolTip(_("Duration to power off when cycling target power"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("ms"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,3), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Power On Recovery interval"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,2), wxLEFT|wxRIGHT|wxTOP, 5);

   timeIntervalValidator.setObject("Power On Recovery interval", &bdmOptions.powerOnRecoveryInterval);
   powerOnRecoveryIntervalTextControl = new NumberTextEditCtrl( panel, ID_POWER_RECOVERY_INTERVAL_TEXT, wxEmptyString, wxDefaultPosition, wxSize(80, -1), 0, timeIntervalValidator );
   gridBagSizer->Add(powerOnRecoveryIntervalTextControl, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   powerOnRecoveryIntervalTextControl->SetToolTip(_("Interval to wait after power on of target"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("ms"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,3), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Reset duration"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,2), wxLEFT|wxRIGHT|wxTOP, 5);

   timeIntervalValidator.setObject("Reset duration", &bdmOptions.resetDuration);
   resetDurationTextControl = new NumberTextEditCtrl( panel, ID_RESET_DURATION_TEXT, wxEmptyString, wxDefaultPosition, wxSize(80, -1), 0, timeIntervalValidator );
   gridBagSizer->Add(resetDurationTextControl, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   resetDurationTextControl->SetToolTip(_("Duration to apply reset to target"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("ms"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,3), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Reset Release interval"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,2), wxLEFT|wxRIGHT|wxTOP, 5);

   timeIntervalValidator.setObject("Reset Release interval", &bdmOptions.resetReleaseInterval);
   resetReleaseIntervalTextControl = new NumberTextEditCtrl( panel, ID_RESET_RELEASE_INTERVAL_TEXT, wxEmptyString, wxDefaultPosition, wxSize(80, -1), 0, timeIntervalValidator );
   gridBagSizer->Add(resetReleaseIntervalTextControl, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   resetReleaseIntervalTextControl->SetToolTip(_("Interval to hold other target signals after reset release"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("ms"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,3), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Reset Recovery interval"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,2), wxLEFT|wxRIGHT|wxTOP, 5);

   timeIntervalValidator.setObject("Reset Recovery interval", &bdmOptions.resetRecoveryInterval);
   resetRecoveryIntervalTextControl = new NumberTextEditCtrl( panel, ID_RESET_RECOVERY_INTERVAL_TEXT, wxEmptyString, wxDefaultPosition, wxSize(80, -1), 0, timeIntervalValidator );
   gridBagSizer->Add(resetRecoveryIntervalTextControl, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   resetRecoveryIntervalTextControl->SetToolTip(_("Interval to wait after target reset"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("ms"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,3), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   return true;
}

/*
 * AdvancedPanel type definition
 */
IMPLEMENT_CLASS( AdvancedPanel, wxPanel )

const string AdvancedPanel::settingsKey("AdvancedPanel");

//!
//! @param settings      - Object to load settings from
//!
void AdvancedPanel::loadSettings(const AppSettings &settings) {

   print("AdvancedPanel::loadSettings()\n");

   bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType = T_OFF;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);

   bdmOptions.powerOffDuration         = settings.getValue(settingsKey+".powerOffDuration",        bdmOptions.powerOffDuration);
   bdmOptions.powerOnRecoveryInterval  = settings.getValue(settingsKey+".powerRecoveryInterval",   bdmOptions.powerOnRecoveryInterval);
   bdmOptions.resetDuration            = settings.getValue(settingsKey+".resetDuration",           bdmOptions.resetDuration);
   bdmOptions.resetReleaseInterval     = settings.getValue(settingsKey+".resetReleaseInterval",    bdmOptions.resetReleaseInterval);
   bdmOptions.resetRecoveryInterval    = settings.getValue(settingsKey+".resetRecoveryInterval",   bdmOptions.resetRecoveryInterval);
}

//! Save setting file
//!
//! @param settings      - Object to save settings to
//!
void AdvancedPanel::saveSettings(AppSettings &settings) {

   print("AdvancedPanel::saveSettings()\n");

   settings.addValue(settingsKey+".powerOffDuration",          bdmOptions.powerOffDuration);
   settings.addValue(settingsKey+".powerRecoveryInterval",     bdmOptions.powerOnRecoveryInterval);
   settings.addValue(settingsKey+".resetDuration",             bdmOptions.resetDuration);
   settings.addValue(settingsKey+".resetReleaseInterval",      bdmOptions.resetReleaseInterval);
   settings.addValue(settingsKey+".resetRecoveryInterval",     bdmOptions.resetRecoveryInterval);
}

//! Get bdm options from dialogue
//!
//! @param _bdmOptions  Options structure to modify
//!
//! @note - _bdmOptions should be set to default values beforehand
//!
void AdvancedPanel::getDialogueValues(USBDM_ExtendedOptions_t *_bdmOptions) {

   print("USBDMPanel::getDialogueValues()\n");

   _bdmOptions->powerOffDuration          = bdmOptions.powerOffDuration;
   _bdmOptions->powerOnRecoveryInterval   = bdmOptions.powerOnRecoveryInterval;
   _bdmOptions->resetDuration             = bdmOptions.resetDuration;
   _bdmOptions->resetReleaseInterval      = bdmOptions.resetReleaseInterval;
   _bdmOptions->resetRecoveryInterval     = bdmOptions.resetRecoveryInterval;
}

bool AdvancedPanel::TransferDataToWindow() {
   print("AdvancedPanel::TransferDataToWindow()\n");
   return wxPanel::TransferDataToWindow();
}

bool AdvancedPanel::TransferDataFromWindow() {
   print("AdvancedPanel::TransferDataFromWindow()\n");
   return wxPanel::TransferDataFromWindow();
}

