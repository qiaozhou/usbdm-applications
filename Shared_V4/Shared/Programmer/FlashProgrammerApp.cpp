/*! \file
    \brief Flash Programming App

    FlashProgrammingApp.cpp

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
-=================================================================================
|   7 Apr 2012 | Added -reset, -power  options                         4.9.4 - pgo
|    Feb  2012 | Added -execute option                                 4.9.3 - pgo
|   5 May 2011 | Fixed Vdd options                                     4.4.3 - pgo
|  31 Jan 2011 | Added command line options (finally)                  4.4.1 - pgo
+=================================================================================
\endverbatim
*/

#if defined(FLASH_PROGRAMMER)

#include <wx/wx.h>
#include <wx/cmdline.h>

#include "Common.h"
#include "USBDM_API.h"
#include "DeviceData.h"
//#include "USBDM_API_Private.h"
#include "Log.h"
#include "USBDMDialogue.h"
#include "AppSettings.h"
#include "Common.h"
#include "Version.h"
#include "FlashProgramming.h"
#include "FlashImage.h"

#if TARGET==HCS08
const TargetType_t targetType = T_HCS08;
const char *logFilename("FlashProgrammer_HCS08.log");
#elif TARGET==HCS12
const TargetType_t targetType = T_HC12;
const char *logFilename("FlashProgrammer_HCS12.log");
#elif TARGET==RS08
const TargetType_t targetType = T_RS08;
const char *logFilename("FlashProgrammer_RS08.log");
#elif TARGET==CFV1
const TargetType_t targetType = T_CFV1;
const char *logFilename("FlashProgrammer_CFV1.log");
#elif TARGET==CFVx
const TargetType_t targetType = T_CFVx;
const char *logFilename("FlashProgrammer_CFVx.log");
#elif TARGET==ARM
#include "USBDM_ARM_API.h"
const TargetType_t targetType = T_ARM_JTAG;
const char *logFilename("FlashProgrammer_ARM.log");
#elif TARGET==MC56F80xx
#include "USBDM_DSC_API.h"
const TargetType_t targetType = T_MC56F80xx;
const char *logFilename("FlashProgrammer_DSC.log");
#else
#error "TARGET must be set"
#endif

// Declare the application class
class FlashProgrammerApp : public wxApp {
   DECLARE_CLASS( FlashProgrammerApp )
   DECLARE_EVENT_TABLE()

private:
   DeviceData::EraseOptions eraseOptions;
   SecurityOptions_t        deviceSecurity;
   USBDMDialogue           *dialogue;
   bool                     commandLine;
   bool                     verify;
   bool                     program;
   bool                     verbose;
   wxString                 hexFileName;
   double                   trimFrequency;
   long                     trimNVAddress;
   wxString                 deviceName;

   int                      returnValue;

   USBDM_ExtendedOptions_t  bdmOptions; // Used by command line only
   void doCommandLineProgram();

public:
   // Called on application startup
   virtual bool OnInit();
   virtual int  OnExit();
   virtual int  OnRun();
   virtual void OnInitCmdLine(wxCmdLineParser& parser);
   virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
   virtual ~FlashProgrammerApp();
};

// Implements FlashProgrammerApp & GetApp()
DECLARE_APP(FlashProgrammerApp)
IMPLEMENT_APP(FlashProgrammerApp)
IMPLEMENT_CLASS(FlashProgrammerApp, wxApp)

/*
 * FlashProgrammerApp event table definition
 */
BEGIN_EVENT_TABLE( FlashProgrammerApp, wxApp )
END_EVENT_TABLE()

USBDM_ErrorCode callBack(USBDM_ErrorCode status, int percent, const char *message) {
   fprintf(stdout, "%d%%: %s", percent, message);
   return PROGRAMMING_RC_OK;
}

#if 1
void FlashProgrammerApp::doCommandLineProgram() {
   FlashImage flashImage;
   unsigned int deviceCount;
   FlashProgrammer flashProgrammer;

   // Assumes one and only 1 device
   USBDM_FindDevices(&deviceCount);
   if ((deviceCount == 0) || (USBDM_Open(0) != BDM_RC_OK)) {
      print("FlashProgrammerApp::doCommandLineProgram() - Failed to open BDM\n");
#ifdef _UNIX_
      fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - Failed to open BDM\n");
#endif
      returnValue = 1;
      return;
   }
   do {
      // Modify some options for programming
//      bdmOptions.autoReconnect      = AUTOCONNECT_NEVER,
      if ((USBDM_SetExtendedOptions(&bdmOptions) != BDM_RC_OK) || (USBDM_SetTargetType(targetType) != BDM_RC_OK)) {
         print("FlashProgrammerApp::doCommandLineProgram() - Failed to set BDM Option/Target type\n");
#ifdef _UNIX_
         fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - Failed to set BDM Option/Target type\n");
#endif
         returnValue = 1;
         break;
      }
      if (!hexFileName.IsEmpty() &&
         (flashImage.loadFile((const char *)hexFileName.ToAscii()) != BDM_RC_OK)) {
         print("FlashProgrammerApp::doCommandLineProgram() - Failed to load Hex file\n");
#ifdef _UNIX_
         fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - Failed to load Hex file\n");
#endif
         returnValue = 1;
         break;
      }
      // Find device details from database
      DeviceDataBase *deviceDatabase = new DeviceDataBase;
      try {
         deviceDatabase->loadDeviceData();
      } catch (MyException &exception) {
         print("FlashProgrammerApp::doCommandLineProgram() - Failed to load device database\nReason\n");
#ifdef _UNIX_
         fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - Failed to load device database\nReason\n");
#endif
         returnValue = 1;
         break;
      }
      const DeviceData *devicePtr = deviceDatabase->findDeviceFromName((const char *)deviceName.ToAscii());
      if (devicePtr == NULL) {
         print("FlashProgrammerApp::doCommandLineProgram() - Failed to find device\n");
#ifdef _UNIX_
         fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - Failed to find device\n");
#endif
         returnValue = 1;
         break;
      }
      DeviceData deviceData = *devicePtr;
      deviceData.setClockTrimFreq(trimFrequency);
      deviceData.setEraseOption(DeviceData::eraseSelective);
      deviceData.setEraseOption(eraseOptions);
      deviceData.setSecurity(deviceSecurity);
      if (trimNVAddress != 0)
         deviceData.setClockTrimNVAddress(trimNVAddress);
      if (flashProgrammer.setDeviceData(deviceData) != PROGRAMMING_RC_OK) {
         returnValue = 1;
         break;
      }
      USBDM_ErrorCode rc;
      if (program) {
         if (verbose) {
            rc = flashProgrammer.programFlash(&flashImage, callBack);
         }
         else{
            rc = flashProgrammer.programFlash(&flashImage, NULL);
         }
      }
      else {
         if (verbose) {
            rc = flashProgrammer.verifyFlash(&flashImage, callBack);
         }
         else{
            rc = flashProgrammer.verifyFlash(&flashImage);
         }
      }
      delete deviceDatabase;
      if (rc != PROGRAMMING_RC_OK) {
         print("FlashProgrammerApp::doCommandLineProgram() - failed, rc = %s\n", USBDM_GetErrorString(rc));
#ifdef _UNIX_
         fprintf(stderr, "FlashProgrammerApp::doCommandLineProgram() - failed, rc = %s\n", USBDM_GetErrorString(rc));
#endif
         returnValue = 1;
         break;
      }
   } while (false);
   print("FlashProgrammerApp::doCommandLineProgram() - Closing BDM\n");
   if (bdmOptions.leaveTargetPowered) {
#if (TARGET==HCS08) || (TARGET==RS08) || (TARGET==CFV1)
      USBDM_TargetReset((TargetMode_t)(RESET_SOFTWARE|RESET_NORMAL));
#elif (TARGET==HCS12) || (TARGET==CFVx) || (TARGET==ARM) || (TARGET==MC56F80xx)
      USBDM_TargetReset((TargetMode_t)(RESET_HARDWARE|RESET_NORMAL));
#else
#error "TARGET must be set"
#endif
   }
   USBDM_Close();

#ifdef _UNIX_
   if (returnValue == 0) {
      fprintf(stdout, "Operation completed successfully\n");
   }
#endif
}
#endif

// Initialize the application
bool FlashProgrammerApp::OnInit(void) {

//   fprintf(stderr, "Starting\n");
   returnValue = 0;

   // call for default command parsing behaviour
   if (!wxApp::OnInit())
      return false;

#if 0

   // Show the frame
   wxFrame *frame = new wxFrame((wxFrame*) NULL, -1, _("Hello wxWidgets World"));
   frame->CreateStatusBar();
   frame->SetStatusText(_("Hello World"));
   frame->Show(TRUE);
   SetTopWindow(frame);

#else
   const wxString settingsFilename(_("FlashProgrammer_"));
   const wxString title(_("Flash Programmer"));

   SetAppName(_("usbdm")); // So app files are kept in the correct directory

   openLogFile(logFilename);

#if TARGET == ARM
   ARM_SetLogFile(getLogFileHandle());
#elif TARGET == MC56F80xx
   DSC_SetLogFile(getLogFileHandle());
#endif

//   print("FlashProgrammerApp::OnInit()\n");

   USBDM_Init();
   if (commandLine) {
      doCommandLineProgram();
//      fprintf(stderr, "Programming Complete - rc = %d\n", returnValue);
   }
   else {
      // Create the main application window
      dialogue = new USBDMDialogue(targetType, title);
      SetTopWindow(dialogue);
      dialogue->Create(NULL);
      dialogue->execute(settingsFilename, hexFileName);
      dialogue->Destroy();
   }
   USBDM_Exit();
#endif
   return true;
}

int FlashProgrammerApp::OnRun(void) {
   print("FlashProgrammerApp::OnRun()\n");
   if (!commandLine) {
      int exitcode = wxApp::OnRun();
      if (exitcode != 0)
         return exitcode;
   }
   // Everything is done in OnInit()!
   print("FlashProgrammerApp::OnRun() - return value = %d\n", returnValue);
   return returnValue;
}

int FlashProgrammerApp::OnExit(void) {

//   print("FlashProgrammerApp::OnExit()\n");
   return wxApp::OnExit();
}

FlashProgrammerApp::~FlashProgrammerApp() {
//   print("FlashProgrammerApp::~FlashProgrammerApp()\n");
//   fprintf(stderr, "FlashProgrammerApp::~FlashProgrammerApp()\n");
   closeLogFile();
}

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
      { wxCMD_LINE_PARAM,    NULL,         NULL, _("Name of the S19 Hex file to load"),                     wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
      { wxCMD_LINE_OPTION, _("device"),    NULL, _("Target device e.g. MC9S08AW16A"),                       wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("vdd"),       NULL, _("Supply Vdd to target (3V3 or 5V)"),                     wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("trim"),      NULL, _("Trim internal clock to frequency (in kHz) e.g. 32.7"),  wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("nvloc"),     NULL, _("Trim non-volatile memory location (hex)"),              wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("erase"),     NULL, _("Erase method (Mass, All, Selective, None)"),            wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_SWITCH, _("execute"),   NULL, _("Leave target power on & reset to normal mode at completion"), },
      { wxCMD_LINE_SWITCH, _("secure"),    NULL, _("Leave device secure after programming") },
      { wxCMD_LINE_SWITCH, _("unsecure"),  NULL, _("Leave device unsecure after programming") },
      { wxCMD_LINE_SWITCH, _("masserase"), NULL, _("Equivalent to erase=Mass") },
      { wxCMD_LINE_SWITCH, _("noerase"),   NULL, _("Equivalent to erase=None") },
      { wxCMD_LINE_SWITCH, _("verify"),    NULL, _("Verify flash contents") },
      { wxCMD_LINE_OPTION, _("reset"),     NULL, _("Reset timing (active,release,recovery) 100-10000 ms"),  wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("power"),     NULL, _("Power timing (off,recovery) 100-10000 ms"),             wxCMD_LINE_VAL_STRING },
      { wxCMD_LINE_OPTION, _("speed"),     NULL, _("Interface speed (CFVx/Kinetis/DSC) kHz"),               wxCMD_LINE_VAL_STRING },
#ifdef _UNIX_
      { wxCMD_LINE_SWITCH, _("verbose"),   NULL, _("Print progress messages to stdout") },
#endif
      { wxCMD_LINE_SWITCH, _("program"),   NULL, _("Program and verify flash contents"), },
      { wxCMD_LINE_NONE }
};

void FlashProgrammerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (g_cmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (_("-"));
    parser.SetLogo(_("USBDM Flash Programmer \n"));

#if (wxCHECK_VERSION(2, 9, 0))
    parser.AddUsageText(_(
          "\nExamples:\n"
          "Programming an image with clock trimming:\n"
          "  FlashProgrammer Image.s19 -device=MC9S08AW16A -trim=243 -program -secure\n"
          "This will program a MC9S08AW16A with the contents of the file Image.s19 and\n"
          "trim the internal clock to 243kHz. The Flash image will be modified so that\n"
          "the device will be secure after programming.\n\n"
          "Programming the clock trim in an already programmed chip:\n"
          "  FlashProgrammer -device=MC9S08QG8 -trim=35.25 -erase=None -program \n"
          "This will trim the internal clock of MC9S08QG8 to 35.25kHz without erasing\n"
          "the present flash contents. It is necessary that the clock trim locations \n"
          "in flash are still unprogrammed (0xFF) when using the -trim option. The \n"
          "target must not be secured and cannot be made secured when using -erase=None."
          ));
#endif
}

//! Process command line arguments
//!
bool FlashProgrammerApp::OnCmdLineParsed(wxCmdLineParser& parser) {
   wxString  sValue;
   bool      success = true;

   commandLine  = false;
   verbose      = false;

//   USBDM_Init();

   if (parser.GetParamCount() > 0) {
      hexFileName = parser.GetParam(0);
   }
   if (parser.Found(_("verify")) || parser.Found(_("program"))) {
      commandLine           = true;
      bdmOptions.size       = sizeof(USBDM_ExtendedOptions_t);
      bdmOptions.targetType = targetType;
      if (USBDM_GetDefaultExtendedOptions(&bdmOptions) != BDM_RC_OK) {
         success = false;
      }
#if (TARGET==HCS08) || (TARGET==RS08)
      eraseOptions = DeviceData::eraseMass;
#elif (TARGET==HCS12) || (TARGET==CFV1) || (TARGET==CFVx) || (TARGET==ARM) || (TARGET==MC56F80xx)
      eraseOptions = DeviceData::eraseAll;
#else
#error "TARGET must be set"
#endif
#ifdef _UNIX_
      if (parser.Found(_("verbose"))) {
         verbose = true;
      }
#endif
      if (parser.Found(_("masserase"))) {
         eraseOptions = DeviceData::eraseMass;
      }
      if (parser.Found(_("noerase"))) {
         eraseOptions = DeviceData::eraseNone;
      }
      if (parser.Found(_("secure"))) {
         deviceSecurity = SEC_SECURED;
      }
      else if (parser.Found(_("unsecure"))) {
         deviceSecurity = SEC_UNSECURED;
      }
      else {
         deviceSecurity = SEC_DEFAULT;
      }
      verify   = parser.Found(_("verify"));
      program  = parser.Found(_("program"));

      if (parser.Found(_("nvloc"), &sValue)) {
         unsigned long uValue;
         if (!sValue.ToULong(&uValue, 16)) {
            success = false;
         }
         trimNVAddress = uValue;
      }
      else {
         trimNVAddress = 0;
      }
      if (parser.Found(_("erase"), &sValue)) {
         if (sValue.CmpNoCase(_("Mass")) == 0) {
            eraseOptions = DeviceData::eraseMass;
         }
         else if (sValue.CmpNoCase(_("All")) == 0) {
            eraseOptions = DeviceData::eraseAll;
         }
         else if (sValue.CmpNoCase(_("Selective")) == 0) {
            eraseOptions = DeviceData::eraseSelective;
         }
         else if (sValue.CmpNoCase(_("None")) == 0) {
            eraseOptions = DeviceData::eraseNone;
         }
         else {
            success = false;
         }
      }
      if (parser.Found(_("vdd"), &sValue)) {
         if (sValue.CmpNoCase(_("3V3")) == 0) {
            bdmOptions.targetVdd = BDM_TARGET_VDD_3V3;
         }
         else if (sValue.CmpNoCase(_("5V")) == 0) {
            bdmOptions.targetVdd = BDM_TARGET_VDD_5V;
         }
         else {
            success = false;
         }
      }
      if (parser.Found(_("trim"), &sValue)) {
         double    dValue;
         if (!sValue.ToDouble(&dValue)) {
            success = false;
         }
         trimFrequency = dValue * 1000;
      }
      else {
         trimFrequency = 0;
      }
      // Must specify device
      if (parser.Found(_("device"), &sValue)) {
         deviceName = sValue;
      }
      else {
         success = false;
      }
      // Reset options
      if (parser.Found(_("reset"), &sValue)) {
         unsigned long uValue=100000; // invalid so faults later

         int index1 = 0;
         int index2 = sValue.find(',');
         wxString t = sValue.substr(index1, index2-index1);
         if (!t.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.resetDuration = uValue;

         index1 = index2+1;
         index2 = sValue.find(',', index1);
         t = sValue.substr(index1, index2-index1);
         if (!t.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.resetReleaseInterval = uValue;

         index1 = index2+1;
         index2 = sValue.length();
         t = sValue.substr(index1, index2-index1);
         if (!t.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.resetRecoveryInterval = uValue;
      }
      // Power options
      if (parser.Found(_("power"), &sValue)) {
         unsigned long uValue=100000; // invalid so faults later

         int index1 = 0;
         int index2 = sValue.find(',');
         wxString t = sValue.substr(index1, index2-index1);
         if (!t.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.powerOffDuration = uValue;

         index1 = index2+1;
         index2 = sValue.length();
         t = sValue.substr(index1, index2-index1);
         if (!t.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.powerOnRecoveryInterval = uValue;
      }
      if (parser.Found(_("speed"), &sValue)) {
         unsigned long uValue;
         if (sValue.ToULong(&uValue, 10)) {
            success = false;
         }
         bdmOptions.interfaceFrequency = uValue;
      }
      bdmOptions.leaveTargetPowered = parser.Found(_("execute"));
      // Programming includes verification
      if (program) {
         verify = false;
      }
   }
   if (!success) {
      parser.Usage();
   }
    return success;
}

#endif /* defined(FLASH_PROGRAMMER) */
