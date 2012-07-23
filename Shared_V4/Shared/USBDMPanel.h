/*
 * FlashProgrammerDialogue.h
 *
 *  Created on: 07/07/2010
 *      Author: pgo
 */

#ifndef USBDMPANEL_H_
#define USBDMPANEL_H_
#include <vector>

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/combo.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include "USBDM_API.h"
#include "USBDM_AUX.h"
#include "Log.h"
#include "AppSettings.h"

class USBDMPanel : public wxPanel {

   DECLARE_CLASS( USBDMPanel )
   DECLARE_EVENT_TABLE()

public:
   USBDMPanel( wxWindow* parent, TargetType_t targetType);
   USBDMPanel(TargetType_t targetType);

   ~USBDMPanel() {
//      print("~USBDMPanel\n");
   }

   USBDM_ErrorCode openBdm(void);
   USBDM_ErrorCode setTarget(void);

   USBDM_ErrorCode Create(wxWindow* parent);
   void Init();

   void loadSettings(const AppSettings &settings);
   void saveSettings(AppSettings &settings);

   bool setDialogueValuesToDefault();
   bool TransferDataToWindow();
   bool TransferDataFromWindow();
   void getDialogueValues(USBDM_ExtendedOptions_t *_bdmOptions);

   void populateBDMChoices(void);

private:
   // Control Identifiers
   enum {
      ID_COMMUNICATION = 11000,
      ID_REFRESH_BDM_BUTTON,
      ID_BDM_SELECT_CHOICE,
      ID_VDD_SELECT_BOX,
      ID_CYCLE_VDD_ON_RESET_CHECKBOX,
      ID_CYCLE_TARGET_VDD_ON_CONNECTION_CHECKBOX,
      ID_LEAVE_TARGET_ON_CHECKBOX,
      ID_MANUALLY_CYCLE_VDD_CHECKBOX,
      ID_RECONNECT_CHECKBOX,
      ID_SPEED_SELECT_CHOICE,
      ID_SPEED_REMINDER_STATIC,
      ID_ALT_BDM_CLOCK_CHECKBOX,
      ID_BUS_CLK_RADIOBUTTON,
      ID_CLKSW_NORMAL_RADIO,
      ID_GUESS_SPEED_CHECKBOX,
      ID_USE_RESET_CHECKBOX,
      ID_USE_PST_SIGNAL_CHECKBOX,
      ID_BDM_CLOCK_SELECT_RADIOBOX,
      ID_BDM_VERSION_STRING,
      ID_DLL_VERSION_STRING,
      ID_BDM_DESCRIPTION_STRING,
      ID_MASK_INTERRUPTS_WHEN_STEPPING,
   };

   USBDM_ErrorCode CreateControls();

   static const string settingsKey;

   void bdmUpdateTargetVersion(void);
   void findBDMs(void);

   // Event handlers
   void OnRefreshBDMClick( wxCommandEvent& event );
   void OnBDMSelectComboSelected( wxCommandEvent& event );
   void OnVddSelectBoxSelected( wxCommandEvent& event );
   void OnCycleVddOnResetCheckboxClick( wxCommandEvent& event );
   void OnCycleTargetVddOnConnectionCheckboxClick( wxCommandEvent& event );
   void OnLeaveTargetOnCheckboxClick( wxCommandEvent& event );
   void OnReconnectCheckboxClick( wxCommandEvent& event );
   void OnSpeedSelectComboSelected( wxCommandEvent& event );
   void OnGuessSpeedCheckboxClick( wxCommandEvent& event );
   void OnUseResetCheckboxClick( wxCommandEvent& event );
   void OnUseUsePstSignalCheckboxClick( wxCommandEvent& event );
   void OnBdmClockSelectRadioboxSelected( wxCommandEvent& event );
   void OnMaskInterruptsWhenSteppingCheckboxClick( wxCommandEvent& event );

   //! Bit masks describing target properties appearing on dialogue
   typedef enum {
      HAS_NONE            = 0,
      HAS_CLK_SW          = (1<<0), // Selection of ALT & BUS clock options
      HAS_GUESS_SPEED     = (1<<2), // Guess speed option
      HAS_USE_RESET       = (1<<3), // Use reset signal option
      HAS_SELECT_SPEED    = (1<<5), // Has speed selection list
      HAS_MASK_INTERRUPTS = (1<<6), // Has an option to mask interrupts when stepping
   } TargetOptions;

   // Controls
   wxButton*      bdmRefreshButtonControl;
   wxChoice*      bdmSelectChoiceControl;
   wxRadioBox*    targetVddControl;
   wxCheckBox*    cycleVddOnResetControl;
   wxCheckBox*    cycleVddOnConnectionControl;
   wxCheckBox*    leaveTargetPoweredControl;
   wxChoice*      connectionSpeedControl;
   wxCheckBox*    automaticallyReconnectControl;
   wxCheckBox*    useResetSignalControl;
   wxCheckBox*    usePstSignalControl;
   wxCheckBox*    guessTargetSpeedControl;
   wxRadioBox*    bdmClockSelectControl;
   wxStaticText*  firmwareStaticControl;
   wxStaticText*  bdmDescriptionStaticControl;
   wxCheckBox*    maskInterruptWhenSteppingControl;
   wxCheckBox*    dontShowAgainControl;

   vector<BdmInformation>  bdmInformation;     // Table of connected BDMs
   wxString                bdmIdentification;
   int                     bdmDeviceNum;

   TargetType_t            targetType;
   TargetOptions           targetProperties;
   USBDM_ExtendedOptions_t bdmOptions;

};

#endif /* USBDMPANEL_H_ */
