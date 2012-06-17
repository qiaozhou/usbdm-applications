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
|  12 Apr 2012 | Created                                               4.9.4 - pgo
+=================================================================================
\endverbatim
*/

#include <wx/wx.h>
#include <wx/cmdline.h>

#include "Common.h"
#include "MainDialogue.h"
#include "Log.h"

// Declare the application class
class JS16_BootloaderApp : public wxApp {
   DECLARE_CLASS( JS16_BootloaderApp )
   DECLARE_EVENT_TABLE()

private:
   MainDialogue  *dialogue;
   int            returnValue;

public:
   // Called on application startup
   virtual bool OnInit();
   virtual int  OnExit();
   virtual int  OnRun();
   virtual ~JS16_BootloaderApp();
};

// Implements JS16_BootloaderApp & GetApp()
DECLARE_APP(JS16_BootloaderApp)
IMPLEMENT_APP(JS16_BootloaderApp)
IMPLEMENT_CLASS(JS16_BootloaderApp, wxApp)

/*
 * JS16_BootloaderApp event table definition
 */
BEGIN_EVENT_TABLE( JS16_BootloaderApp, wxApp )
END_EVENT_TABLE()

// Initialise the application
bool JS16_BootloaderApp::OnInit(void) {

   returnValue = 0;

   // call for default command parsing behaviour
   if (!wxApp::OnInit())
      return false;

//   const wxString settingsFilename(_("JS16_Bootloader_"));
//   const wxString title(_("JS16 Bootloader"));

   SetAppName(_("usbdm")); // So app files are kept in the correct directory

   openLogFile("JS16_Bootloader.log");

   // Create the main application window
   dialogue = new MainDialogue(NULL);
   SetTopWindow(dialogue);
   dialogue->ShowModal();
   dialogue->Destroy();

   return true;
}

int JS16_BootloaderApp::OnRun(void) {
   print("FlashProgrammerApp::OnRun()\n");
   int exitcode = wxApp::OnRun();
   if (exitcode != 0)
      return exitcode;
   // Everything is done in OnInit()!
   print("FlashProgrammerApp::OnRun() - return value = %d\n", returnValue);
   return returnValue;
}

int JS16_BootloaderApp::OnExit(void) {

   print("JS16_BootloaderApp::OnExit()\n");
   return wxApp::OnExit();
}

JS16_BootloaderApp::~JS16_BootloaderApp() {
   print("JS16_BootloaderApp::~JS16_BootloaderApp()\n");
   closeLogFile();
}

