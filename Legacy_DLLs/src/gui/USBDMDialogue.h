/*
 * FlashProgrammerDialogue.h
 *
 *  Created on: 07/07/2010
 *      Author: pgo
 */

#ifndef USBDMDIALOGUE_H_
#define USBDMDIALOGUE_H_

#include <wx/frame.h>
#include <wx/notebook.h>
#include <wx/combo.h>
#include <wx/dialog.h>
#include <wx/panel.h>

#include "USBDMPanel.h"
#include "USBDM_API.h"
#include "Log.h"

#define BASE_CAPTION _("USBDM Configuration")

/*!
 * USBDMDialogue class declaration
 */
class USBDMDialogue: public wxDialog {

    DECLARE_CLASS( USBDMDialogue )
    DECLARE_EVENT_TABLE()

private:
   static const wxString   settingsKey;
   TargetType_t            targetType;
   wxString                caption;
   HardwareCapabilities_t  bdmCapabilities;
   USBDM_ErrorCode         errorSet;
   bool                    dontDisplayDialogue;

   // Control Identifiers
   enum {
      ID_FLASH_PROGRAMMER_DIALOGUE  = 10000,
      ID_NOTEBOOK,
      ID_DONT_SHOW_AGAIN_CHECKBOX,
      ID_BDM_VERSION_STRING,
      ID_DLL_VERSION_STRING,
      ID_BUTTON_DEFAULT,
      ID_COMMUNICATION              = 11000,
      ID_FLASH_PROGRAMMING          = 12000,
   };

   wxStaticText   *versionStaticControl;
   wxCheckBox     *dontShowAgainControl;
   USBDMPanel     *communicationPanel;

public:
   /// Constructors
   USBDMDialogue(                  TargetType_t targetType, const wxString &caption=BASE_CAPTION);
   USBDMDialogue(wxWindow* parent, TargetType_t targetType, const wxString &caption=BASE_CAPTION);

   USBDM_ErrorCode execute(bool forceDisplay);

   /// Destructor
   ~USBDMDialogue();

   //! Returns the BDM options from the internal state
   //!
   //! @param bdmOptions - where to return options
   //!
   void getBDMOptions( USBDM_ExtendedOptions_t &bdmOptions ) {
      getDialogueValues(&bdmOptions);
   };

private:
   static FILE *openSettingsFile(TargetType_t targetType, const wxString &_filename, const wxString  &attributes);

   static bool loadSettingsFromFile(FILE *configFile, BDM_Options_t *bdmOptionsSet);
   static bool saveSettingsToFile(FILE *configFile,   BDM_Options_t *bdmOptions);

   bool setDialogueValuesToDefault();
   bool TransferDataToWindow();
   bool TransferDataFromWindow();
   bool loadSettingsFileFromAppDir(wxString const &fileName);
   bool saveSettingsFileToAppDir(wxString const &fileName);

   FILE *openSettingsFile(const wxString &filename, const wxString  &attributes);
   int getErrorAction(USBDM_ErrorCode rc);

   bool loadSettings(const AppSettings &appSettings);
   bool saveSettings(AppSettings &appSettings);
   bool loadSettingsFile(wxString const &fileName);
   bool saveSettingsFile(wxString const &fileName);
   USBDM_ErrorCode checkTargetUnSecured();
   USBDM_ErrorCode hcs12Check(void);

   // Initialises member variables
   bool Init();

   // Creates the controls and sizers
   void CreateControls();
   bool Create( wxWindow* parent);

   // Event handlers
   void OnDontShowAgainControlClick( wxCommandEvent& event );
   void OnDefaultClick( wxCommandEvent& event );
   void OnOkClick( wxCommandEvent& event );

   wxBitmap GetBitmapResource( const wxString& name );
   wxIcon GetIconResource( const wxString& name );
   static bool ShowToolTips();

   bool getDialogueValues(USBDM_ExtendedOptions_t *bdmOptions);
};

#endif /* USBDMDIALOGUE_H */
