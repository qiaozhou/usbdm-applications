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
   if (!wxPanel::Create(parent) || !CreateControls()) {
      return false;
   }
   return true;
}

//===================================================================
//! Set the panel internal state to the default
//!
void AdvancedPanel::Init() {
   print("AdvancedPanel::Init()\n");
   bdmOptions.size                = sizeof(USBDM_ExtendedOptions_t);
   bdmOptions.targetType          = T_OFF;
   currentDevice                  = NULL;
   USBDM_GetDefaultExtendedOptions(&bdmOptions);
}

enum {
   ID_POWER_OFF_DURATION_TEXT,
   ID_POWER_RECOVERY_INTERVAL_TEXT,
   ID_RESET_DURATION_TEXT,
   ID_RESET_RELEASE_INTERVAL_TEXT,
   ID_RESET_RECOVERY_INTERVAL_TEXT,
   ID_EEPROM_SIZE_CHOICE,
   ID_FLEXNVM_PARTITION_CHOICE,
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

#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
   //================================================================================
   itemStaticBox = new wxStaticBox(panel, wxID_ANY, _("FlexNVM Parameters"));
   itemStaticBoxSizer = new wxStaticBoxSizer(itemStaticBox, wxHORIZONTAL);
   panelBoxSizerV->Add(itemStaticBoxSizer, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

   gridBagSizer = new wxGridBagSizer(0,0);
   itemStaticBoxSizer->Add(gridBagSizer, 0, wxGROW|wxALL, 5);

   row = 0;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("EEEPROM Size"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   eeepromSizeChoiceControl = new wxChoice( panel, ID_EEPROM_SIZE_CHOICE, wxDefaultPosition , wxSize(150, -1));
   gridBagSizer->Add(eeepromSizeChoiceControl, wxGBPosition(row,1), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5);
   eeepromSizeChoiceControl->SetToolTip(_("Controls size of emulated EEPROM region(s) in FlexRAM"));

   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("bytes"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   row++;

   //===
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("FlexNVM Partition"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,0), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   flexNvmPartitionChoiceControl = new wxChoice( panel, ID_FLEXNVM_PARTITION_CHOICE, wxDefaultPosition , wxSize(150, -1));
   gridBagSizer->Add(flexNvmPartitionChoiceControl, wxGBPosition(row,1), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);
   flexNvmPartitionChoiceControl->SetToolTip(_("Controls how FlexNVM is divided between Data Flash and EEPROM backing store\n"
                                               "EEPROM backing store size >= 16 x EEEPROM size - affects flash lifetime"));
//   flexNvmPartitionChoiceControl->SetMinSize(wxSize(100,0));
   itemStaticText = new wxStaticText( panel, wxID_STATIC, _("Kbytes"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(itemStaticText, wxGBPosition(row,2), wxGBSpan(1,1), wxLEFT|wxRIGHT|wxTOP, 5);

   wxSize sz(flexNvmPartitionChoiceControl->GetMinSize());
   sz.x += 10;
   flexNvmPartitionChoiceControl->SetMinSize(sz);
   row++;
   //===
   flexNvmDescriptionStaticControl = new wxStaticText( panel, wxID_STATIC, _("Estimated 16-bit write cycles (based on JU128 specs) = ----------"), wxDefaultPosition, wxDefaultSize, 0 );
   gridBagSizer->Add(flexNvmDescriptionStaticControl, wxGBPosition(row,0), wxGBSpan(1,3), wxLEFT|wxRIGHT|wxTOP, 5);
   row++;

   populateEepromControl();
   populatePartitionControl();
#endif

   return true;
}

#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
//! Populates eeepromSizeChoiceControl with EEEPROM sizes
//! Selects 1st entry if reload is necessary (devcie changed)
//!
void AdvancedPanel::populateEepromControl() {
   static FlexNVMInfoPtr lastFlexNVMInfo;
   if (currentDevice == NULL) {
      print("AdvancedPanel::populateEepromControl() - currentDevice not set\n");
      lastFlexNVMInfo.reset();
      eeepromSizeChoiceControl->Clear();
      eeepromSizeChoiceControl->Append(_("[No device selected]"));
      eeepromSizeChoiceControl->Select(0);
      eeepromSizeChoiceControl->Enable(false);
      eeepromSizeChoice = 0;
      return;
   }
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      print("AdvancedPanel::populatePartitionControl() - device has no flexNVMInfo\n");
      lastFlexNVMInfo.reset();
      eeepromSizeChoiceControl->Clear();
      eeepromSizeChoiceControl->Append(_("[EEEPROM not supported]"));
      eeepromSizeChoiceControl->Select(0);
      eeepromSizeChoiceControl->Enable(false);
      eeepromSizeChoice = 0;
      return;
   }
   if (flexNVMInfo.get() == lastFlexNVMInfo.get()) {
      // No device change - no change in list
      print("AdvancedPanel::populateEepromControl() - no update as flexNVMInfo unchanged\n");
      return;
   }
   print("AdvancedPanel::populateEepromControl()\n");
   lastFlexNVMInfo = flexNVMInfo;
   vector<FlexNVMInfo::EeepromSizeValue> &list(flexNVMInfo->getEeepromSizeValues());
   vector<FlexNVMInfo::EeepromSizeValue>::iterator it;

   eeepromSizeChoiceControl->Clear();
   for ( it=list.begin(); it < list.end(); it++) {
      eeepromSizeChoiceControl->Append(it->description);
   }
   eeepromSizeChoiceControl->Select(0);
   eeepromSizeChoiceControl->Enable(true);
   eeepromSizeChoice = 0;
   flexNvmPartitionIndex = 0; // Previous value is now invalid as partition table may have changed
}

//! Finds table index for given eeeprom size (FlexRAM used as Emulated eeprom)
//!
//! @param eepromSize size to look for
//!
//! @return index into eeepromSizeValues (and eeepromSizeChoiceControl control)
//!
int AdvancedPanel::findEeepromSizeIndex(unsigned eepromSize) {
   if (currentDevice == NULL) {
      print("AdvancedPanel::findEeepromSizeIndex() - currentDevice not set\n");
      return 0;
   }
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      print("AdvancedPanel::findEeepromSizeIndex() - flexNVMInfo not set\n");
      return 0;
   }
   vector<FlexNVMInfo::EeepromSizeValue> &list(flexNVMInfo->getEeepromSizeValues());
   vector<FlexNVMInfo::EeepromSizeValue>::iterator it;

   int index;
   for ( it=list.begin(), index=0;
         it < list.end();
         it++, index++) {
      if (it->size == eepromSize) {
         return index;
      }
   }
   return 0; // default to 1st entry (zero size)
}

/*
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_DEVICE_TYPE_CHOICE
 */
void AdvancedPanel::OnEeepromSizeChoiceSelected( wxCommandEvent& event ) {
   // Get currently selected eeeprom choice
   eeepromSizeChoice = event.GetSelection();
   print("FlashPanel::OnEeepromSizeChoiceSelected(): EEPROM size choice = %d\n", eeepromSizeChoice);
   TransferDataToWindow();
}

//! Populates flexNvmPartitionChoiceControl with Partition values
//!
//! @note Entries are filtered by minimum size required to satisfy eeepromSizeChoice
//! @note flexNvmPartitionIndex is used to select entry if valid or 1st entry is used
//!
void AdvancedPanel::populatePartitionControl() {
   int flexNvmPartitionChoice = 0;      // Default to select 1st entry in populated control

   print("FlashPanel::populatePartitionControl()\n");
   flexNvmPartitionChoiceControl->Clear();
   if (currentDevice == NULL) {
      print("AdvancedPanel::populatePartitionControl() - currentDevice not set\n");
      flexNvmPartitionChoiceControl->Append(_("[No device selected]"));
      flexNvmPartitionChoiceControl->Select(0);
      flexNvmPartitionChoiceControl->Enable(false);
      flexNvmPartitionIndex = 0;
      return;
   }
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      print("AdvancedPanel::populatePartitionControl() - device has no flexNVMInfo\n");
      flexNvmPartitionChoiceControl->Append(_("[EEEPROM not supported]"));
      flexNvmPartitionChoiceControl->Select(0);
      flexNvmPartitionChoiceControl->Enable(false);
      flexNvmPartitionIndex = 0;
      return;
   }
   vector<FlexNVMInfo::EeepromSizeValue> &eeepromSizeValues(flexNVMInfo->getEeepromSizeValues());
   vector<FlexNVMInfo::FlexNvmPartitionValue> &flexNvmPartitionValues(flexNVMInfo->getFlexNvmPartitionValues());
   if (eeepromSizeChoice<=0) {
      flexNvmPartitionChoiceControl->Append(_("[All DFlash]"));
      flexNvmPartitionChoiceControl->Enable(false);
      flexNvmPartitionIndex = 0;
   }
   else {
      vector<FlexNVMInfo::FlexNvmPartitionValue>::iterator it;

      // Minimum required backing store for currently selected EEEPROM size
      unsigned minimumBackingStore = flexNVMInfo->getBackingRatio()*eeepromSizeValues[eeepromSizeChoice].size;
      int      newIndex            = 0;                                            // Default No EEPROM
      int index;
      for ( it=flexNvmPartitionValues.begin(), index = 0; it < flexNvmPartitionValues.end(); it++, index++) {
         if (it->backingStore >= minimumBackingStore) {
            int controlIndex = flexNvmPartitionChoiceControl->Append(it->description);
            // Save index as client data as not all entries may be present in control
            flexNvmPartitionChoiceControl->SetClientData(controlIndex, (void*)index);
            if (newIndex==0) {
               // Use 1st added choice entry as default
               newIndex = index;
            }
            if (index == (unsigned)flexNvmPartitionIndex) {
               // Previously selected value still available - use it selected entry
               flexNvmPartitionChoice = controlIndex; // record control entry to select
               newIndex               = index;        // record corresponding table entry
            }
         }
      }
      flexNvmPartitionChoiceControl->Enable(true);
      flexNvmPartitionIndex = newIndex;
   }
   flexNvmPartitionChoiceControl->Select(flexNvmPartitionChoice);
   print("FlashPanel::populatePartitionControl(), choice=%d, index=%d => size=%d\n",
          flexNvmPartitionChoice, flexNvmPartitionIndex, flexNvmPartitionValues[flexNvmPartitionIndex].backingStore);
}

//! Finds table index for given backingStoreSize size
//!
//! @param backingStoreSize size to look for
//!
//! @return index into flexNvmPartitionValues
//!
int AdvancedPanel::findPartitionControlIndex(unsigned backingStoreSize) {
   if (currentDevice == NULL) {
      print("AdvancedPanel::findPartitionControlIndex() - currentDevice not set\n");
      return 0;
   }
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      print("AdvancedPanel::findPartitionControlIndex() - flexNVMInfo not set\n");
      return 0;
   }
   vector<FlexNVMInfo::FlexNvmPartitionValue> &flexNvmPartitionValues(flexNVMInfo->getFlexNvmPartitionValues());
   vector<FlexNVMInfo::FlexNvmPartitionValue>::iterator it;
   int index;
   for ( it=flexNvmPartitionValues.begin(), index = 0; it < flexNvmPartitionValues.end(); it++, index++) {
      if (it->backingStore == backingStoreSize) {
         return index;
         }
      }
   return 0; // default to 1st entry (zero size)
}

/*
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_DEVICE_TYPE_CHOICE
 */
void AdvancedPanel::OnFlexNvmPartionChoiceSelected( wxCommandEvent& event ) {
   // Get currently selected FlexNVM partition choice
   flexNvmPartitionIndex = (int) event.GetClientData();
   print("FlashPanel::OnFlexNvmPartionChoiceSelected(): Partition value =0x%02X\n", flexNvmPartitionIndex);
   TransferDataToWindow();
}
#endif

//! Updates GUI from internal state
//!
//! @note values are validated
//!
void AdvancedPanel::updateFlashNVM() {
#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)

   static const double writeEfficiency = 0.5;     // Assume 16/32-bit writes
   static const double endurance       = 10000.0; // From JU128 specification sheet
   print("FlashPanel::updateFlashNVM()\n");
   populateEepromControl();
   eeepromSizeChoiceControl->SetSelection(eeepromSizeChoice);
   populatePartitionControl();
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      flexNvmDescriptionStaticControl->SetLabel(_("EEPROM emulation not available"));
      eeepromSizeChoice     = 0;
      flexNvmPartitionIndex = 0;
      return;
   }
   if (eeepromSizeChoice == 0) {
      flexNvmDescriptionStaticControl->SetLabel(_("EEPROM emulation will be disabled if device is mass-erased"));
      return;
   }
   vector<FlexNVMInfo::EeepromSizeValue>      &eeepromSizeValues(flexNVMInfo->getEeepromSizeValues());
   vector<FlexNVMInfo::FlexNvmPartitionValue> &flexNvmPartitionValues(flexNVMInfo->getFlexNvmPartitionValues());
   unsigned eeepromSize = eeepromSizeValues[eeepromSizeChoice].size;
   unsigned eepromSize  = flexNvmPartitionValues[flexNvmPartitionIndex].backingStore;
   double estimatedFlexRamWrites = (writeEfficiency*endurance*(eepromSize-2*eeepromSize))/eeepromSize;
   char buff[100];
   print("    eeepromSize=%d, eepromSize=%d, ratio=%.2g, estimatedFlexRamWrites=%.2g\n",
              eeepromSize, eepromSize, (double)eepromSize/eeepromSize, estimatedFlexRamWrites);
   snprintf(buff, sizeof(buff),"Estimated 16-bit write cycles (based on JU128 specs) = %.2g", estimatedFlexRamWrites);
   flexNvmDescriptionStaticControl->SetLabel(buff);
#endif
}

/*
 * AdvancedPanel type definition
 */
IMPLEMENT_CLASS( AdvancedPanel, wxPanel )

/*
 * FlashPanel event table definition
 */
BEGIN_EVENT_TABLE( AdvancedPanel, wxPanel )
#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
EVT_CHOICE( ID_EEPROM_SIZE_CHOICE,         AdvancedPanel::OnEeepromSizeChoiceSelected )
EVT_CHOICE( ID_FLEXNVM_PARTITION_CHOICE,   AdvancedPanel::OnFlexNvmPartionChoiceSelected )
#endif
END_EVENT_TABLE()

#define settingsKey "AdvancedPanel"
const string powerOffDurationKey(        settingsKey ".powerOffDuration");
const string powerOnRecoveryIntervalKey( settingsKey ".powerOnRecoveryInterval");
const string resetDurationKey(           settingsKey ".resetDuration");
const string resetReleaseIntervalKey(    settingsKey ".resetReleaseInterval");
const string resetRecoveryIntervalKey(   settingsKey ".resetRecoveryInterval");
const string eeepromSizeKey(             settingsKey ".eeepromSize");
const string flexNvmPartitionSizeKey(    settingsKey ".flexNvmPartitionSize");

//!
//! @param settings - Object to load settings from
//!
void AdvancedPanel::loadSettings(const AppSettings &settings) {

   print("AdvancedPanel::loadSettings()\n");

//   Init();

   bdmOptions.powerOffDuration         = settings.getValue(powerOffDurationKey,        bdmOptions.powerOffDuration);
   bdmOptions.powerOnRecoveryInterval  = settings.getValue(powerOnRecoveryIntervalKey, bdmOptions.powerOnRecoveryInterval);
   bdmOptions.resetDuration            = settings.getValue(resetDurationKey,           bdmOptions.resetDuration);
   bdmOptions.resetReleaseInterval     = settings.getValue(resetReleaseIntervalKey,    bdmOptions.resetReleaseInterval);
   bdmOptions.resetRecoveryInterval    = settings.getValue(resetRecoveryIntervalKey,   bdmOptions.resetRecoveryInterval);

#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
   int eepromSize = settings.getValue(eeepromSizeKey,             0);
   eeepromSizeChoice = findEeepromSizeIndex(eepromSize);
   if (eeepromSizeChoice == 0) {
      flexNvmPartitionIndex = 0;
   }
   else {
      int partitionSize = settings.getValue(flexNvmPartitionSizeKey,        0);
      flexNvmPartitionIndex = findPartitionControlIndex(partitionSize);
   }
#endif
   TransferDataToWindow();
}

//! Save setting file
//!
//! @param settings      - Object to save settings to
//!
void AdvancedPanel::saveSettings(AppSettings &settings) {

   print("AdvancedPanel::saveSettings()\n");

   settings.addValue(powerOffDurationKey,          bdmOptions.powerOffDuration);
   settings.addValue(powerOnRecoveryIntervalKey,   bdmOptions.powerOnRecoveryInterval);
   settings.addValue(resetDurationKey,             bdmOptions.resetDuration);
   settings.addValue(resetReleaseIntervalKey,      bdmOptions.resetReleaseInterval);
   settings.addValue(resetRecoveryIntervalKey,     bdmOptions.resetRecoveryInterval);

#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo == NULL) {
      return;
   }
   vector<FlexNVMInfo::EeepromSizeValue>      &eeepromSizeValues(flexNVMInfo->getEeepromSizeValues());
   vector<FlexNVMInfo::FlexNvmPartitionValue> &flexNvmPartitionValues(flexNVMInfo->getFlexNvmPartitionValues());
   settings.addValue(eeepromSizeKey,               eeepromSizeValues[eeepromSizeChoice].size);
   if (eeepromSizeChoice>0) {
      settings.addValue(flexNvmPartitionSizeKey,   flexNvmPartitionValues[flexNvmPartitionIndex].backingStore);
   }
#endif
}

//! Get bdm options from dialogue
//!
//! @param _bdmOptions  Options structure to modify
//!
//! @note - _bdmOptions should be set to default values beforehand
//!
void AdvancedPanel::getBdmOptions(USBDM_ExtendedOptions_t *_bdmOptions) {

   print("USBDMPanel::getDialogueValues()\n");

   _bdmOptions->powerOffDuration          = bdmOptions.powerOffDuration;
   _bdmOptions->powerOnRecoveryInterval   = bdmOptions.powerOnRecoveryInterval;
   _bdmOptions->resetDuration             = bdmOptions.resetDuration;
   _bdmOptions->resetReleaseInterval      = bdmOptions.resetReleaseInterval;
   _bdmOptions->resetRecoveryInterval     = bdmOptions.resetRecoveryInterval;
}

//! Get bdm options from dialogue
//!
//! @param _bdmOptions  Options structure to modify
//!
//! @note - _bdmOptions should be set to default values beforehand
//!
void AdvancedPanel::getDeviceOptions(DeviceDataPtr deviceData) {
   print("USBDMPanel::getDeviceOptions()\n");

   DeviceData::FlexNVMParameters flexParameters = {0xFF,0xFF}; // Default value for no parameters
   FlexNVMInfoPtr flexNVMInfo = currentDevice->getflexNVMInfo();
   if (flexNVMInfo != NULL) {
      vector<FlexNVMInfo::EeepromSizeValue>      &eeepromSizeValues(flexNVMInfo->getEeepromSizeValues());
      vector<FlexNVMInfo::FlexNvmPartitionValue> &flexNvmPartitionValues(flexNVMInfo->getFlexNvmPartitionValues());
      flexParameters.eeepromSize  = eeepromSizeValues[eeepromSizeChoice].value;
      flexParameters.partionValue = flexNvmPartitionValues[flexNvmPartitionIndex].value;
   }
   deviceData->setFlexNVMParameters(&flexParameters);
}

bool AdvancedPanel::TransferDataToWindow() {
   updateFlashNVM();
   return wxPanel::TransferDataToWindow();
}

bool AdvancedPanel::TransferDataFromWindow() {
   print("AdvancedPanel::TransferDataFromWindow()\n");
   return wxPanel::TransferDataFromWindow();
}

void AdvancedPanel::setCurrentDevice(DeviceData *currentDevice) {
   print("AdvancedPanel::setCurrentDevice()\n");
   this->currentDevice = currentDevice;
   updateFlashNVM();
}
