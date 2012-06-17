/*
 * CFUnlockerApp.c
 *
 *  Created on: 23/10/2010
 *      Author: podonoghue
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "USBDM_API.h"
#include "Log.h"
#include "CFUnlockerApp.h"
#include "CFUnlockerDialogue.h"
#include "CFUnlockerPanel.h"

/*
 * Application instance implementation
 */
IMPLEMENT_APP( CFUnlockerApp )

/*
 * CFUnlockerApp type definition
 */
IMPLEMENT_CLASS( CFUnlockerApp, wxApp )

/*
 * CFUnlockerApp event table definition
 */
BEGIN_EVENT_TABLE( CFUnlockerApp, wxApp )
END_EVENT_TABLE()

/*
 * Constructor for CFUnlockerApp
 */
CFUnlockerApp::CFUnlockerApp()
{
    Init();
    wxGetApp().SetAppName(_("usbdm"));
    openLogFile("CFUnlocker.log");
}

/*
 * Member initialisation
 */
void CFUnlockerApp::Init()
{
}

/*
 * Initialisation for CFUnlockerApp
 */
bool CFUnlockerApp::OnInit()
{
//   ColdfireUnlockerPanel* mainWindow = new ColdfireUnlockerPanel(NULL);
//   /* int returnValue = */ mainWindow->ShowModal();

   CFUnlockerDialogue *mainWindow = new CFUnlockerDialogue(NULL);
   mainWindow->ShowModal();
   mainWindow->Destroy();

   return false; // Terminate app on return
}

/*
 * Cleanup for CFUnlockerApp
 */
int CFUnlockerApp::OnExit()
{
   return wxApp::OnExit();
}
