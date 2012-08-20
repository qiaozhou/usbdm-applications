/*
 * FlashProgrammerDialogue.h
 *
 *  Created on: 07/07/2010
 *      Author: pgo
 */

#ifndef ADVANCEDPANEL_H
#define ADVANCEDPANEL_H

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/validate.h>
#include "Log.h"
#include "AppSettings.h"
#include "FlashProgramming.h"
#include "AppSettings.h"
#include "NumberTextEditCtrl.h"

class TimeIntervalValidator : public wxTextValidator {
   const char *name;
   unsigned   *value;
   unsigned    min, max;

public:
   TimeIntervalValidator(const char *name, unsigned *value, unsigned min, unsigned max) :
      wxTextValidator(wxFILTER_NUMERIC, NULL),
      name(name),
      value(value),
      min(min),
      max(max) {
//      print("TimeIntervalValidator()\n");
   }
public:
   NumberTextEditCtrl *GetWindow() const {
      return static_cast<NumberTextEditCtrl *>(wxValidator::GetWindow());
   }
   bool Validate(wxWindow *parent) {
//      print("TimeIntervalValidator::Validate()\n");
      unsigned value = GetWindow()->GetDecimalValue();
      bool isOK = ((value>=min)&&(value<=max));
      if(!isOK) {
         wxString msg = wxString::Format(_("Field \'%s\' is invalid\n"
                                           "Permitted range [%d to %d]"),
                                         name, min, max);
         wxMessageBox(msg,_("Input field invalid"), wxOK, parent);
      }
      return isOK;
   }
   bool TransferToWindow() {
//      print("TimeIntervalValidator::TransferToWindow()\n");
      if (*value<min) {
         *value = min;
      }
      if (*value>max) {
         *value = max;
      }
      GetWindow()->SetDecimalValue(*value);
      return true;
   }
   bool TransferFromWindow() {
//      print("TimeIntervalValidator::TransferFromWindow()\n");
      if (!Validate(NULL)) {
         return false;
      }
      *value = GetWindow()->GetDecimalValue();
      return true;
   }
   //! @param name Assumed to be a statically allocated string and is used by reference
   //!
   void setObject(const char *_name, unsigned *_value) {
      name = _name;
      value = _value;
   }
   void setMin(unsigned _min) {
      min = _min;
   }
   void setMax(unsigned _max) {
      max = _max;
   }
   wxObject *Clone(void) const {
//      print("Clone()\n");
      return new TimeIntervalValidator(this->name, this->value, this->min, this->max);
   }
};

/*!
 * FlashPanel class declaration
 */
class AdvancedPanel: public wxPanel {

    DECLARE_CLASS( AdvancedPanel )
    DECLARE_EVENT_TABLE()

private:
   // Creates the controls and sizers
   bool CreateControls();
   void populateEepromControl();
   void populatePartitionControl();
   int  findEeepromSizeIndex(unsigned eepromSize);
   int  findPartitionControlIndex(unsigned backingStoreSize);
   void updateFlashNVM();
   void OnEeepromSizeChoiceSelected( wxCommandEvent& event );
   void OnFlexNvmPartionChoiceSelected( wxCommandEvent& event );

   NumberTextEditCtrl*           powerOffDurationTextControl;
   NumberTextEditCtrl*           powerOnRecoveryIntervalTextControl;
   NumberTextEditCtrl*           resetDurationTextControl;
   NumberTextEditCtrl*           resetReleaseIntervalTextControl;
   NumberTextEditCtrl*           resetRecoveryIntervalTextControl;
#if (TARGET==CFV1) || (TARGET==ARM) || (TARGET==ARM_SWD)
   wxChoice*                     eeepromSizeChoiceControl;
   wxChoice*                     flexNvmPartitionChoiceControl;
   wxStaticText*                 flexNvmDescriptionStaticControl;
#endif
   USBDM_ExtendedOptions_t       bdmOptions;
   static const string           settingsKey;
   int                           eeepromSizeChoice;      // Choice for eeepromSizeChoice control & index for eeepromSizeValues[]
   int                           flexNvmPartitionIndex;  // Index for flexNvmPartitionValues[]
   DeviceData                   *currentDevice;

public:
   // Constructors
   AdvancedPanel(TargetType_t targetType=T_HCS08, HardwareCapabilities_t bdmCapabilities=BDM_CAP_ALL);
   AdvancedPanel(wxWindow* parent, TargetType_t targetType=T_HCS08, HardwareCapabilities_t bdmCapabilities=BDM_CAP_ALL);

   // Destructor
   ~AdvancedPanel() {
//      print("~AdvancedPanel()\n");
   };

   // Create the window
   bool Create( wxWindow* parent);

   // Set internal state to default
   void Init();

   void loadSettings(const AppSettings &settings);
   void saveSettings(AppSettings &settings);

   void getBdmOptions(USBDM_ExtendedOptions_t       *_bdmOptions);
   void getDeviceOptions(DeviceDataPtr deviceData);

//   void OnPowerOffDurationTextUpdated( wxCommandEvent& event );
//   void OnPowerRecoveryTextUpdated( wxCommandEvent& event );
//   void OnResetDurationTextUpdated( wxCommandEvent& event );
//   void OnResetReleaseIntervalTextUpdated( wxCommandEvent& event );
//   void OnResetRecoveryIntervalTextUpdated( wxCommandEvent& event );
   bool TransferDataToWindow();
   bool TransferDataFromWindow();

   void setCurrentDevice(DeviceData *currentDevice);
};

#endif /* ADVANCEDPANEL_H */
