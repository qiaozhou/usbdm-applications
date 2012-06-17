/*! \file
    \brief Implements USBDM dialogue

    CFUnlockerDialogue.cpp

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
   | 24 Oct 2010 | wxWidgets version created                               - pgo
   +============================================================================
   \endverbatim
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wx/imaglist.h"
////@end includes

#include "CFUnlockerDialogue.h"

#include "USBDM_API.h"
#include "USBDMPanel.h"

/*
 * CFUnlockerDialogue type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CFUnlockerDialogue, wxDialog )


//===================================================================
//===================================================================
//===================================================================

//! CFUnlockerDialogue constructors
//!
//!
CFUnlockerDialogue::CFUnlockerDialogue()
{
   Init();
}

//! CFUnlockerDialogue constructors
//!
//! @param parent          : Parent window to pass to Create()
//! @note: Calls Create() to creates the dialogue
//!
CFUnlockerDialogue::CFUnlockerDialogue( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Init();
    Create(parent, id, caption, pos, size, style);
}



//! CFUnlockerDialgue creator
//!
//! @param parent     : Parent window
//!
bool CFUnlockerDialogue::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, wxSize(-1,-1), style );


   CreateControls();

   if (GetSizer()) {
      GetSizer()->SetSizeHints(this);
   }
   Centre();

   return true;
}

//! CFUnlockerDialogue destructor
//!

CFUnlockerDialogue::~CFUnlockerDialogue()
{
}


/*
 * Member initialisation
 */

void CFUnlockerDialogue::Init()
{
   usbdmPanel = NULL;
   cfUnlockerPanel = NULL;
}


/*
 * Control creation for CFUnlockerDialogue
 */

void CFUnlockerDialogue::CreateControls()
{
    CFUnlockerDialogue* frame = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    frame->SetSizer(itemBoxSizer2);

    noteBook = new wxNotebook( frame, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxBK_DEFAULT );

#if TARGET == CFVx
    usbdmPanel = new USBDMPanel( noteBook, T_CFVx);
#elif TARGET == MC56F80xx
    usbdmPanel = new USBDMPanel( noteBook, T_MC56F80xx);
#endif

    noteBook->AddPage(usbdmPanel, _("Connection"));

    cfUnlockerPanel = new ColdfireUnlockerPanel( noteBook, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

    noteBook->AddPage(cfUnlockerPanel, _("Unlocker"));

    itemBoxSizer2->Add(noteBook, 0, wxGROW|wxALL, 5);
}


/*
 * Should we show tooltips?
 */

bool CFUnlockerDialogue::ShowToolTips()
{
    return true;
}

/*
 * Get bitmap resources
 */

wxBitmap CFUnlockerDialogue::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CFUnlockerDialogue bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CFUnlockerDialogue bitmap retrieval
}

/*
 * Get icon resources
 */

wxIcon CFUnlockerDialogue::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CFUnlockerDialogue icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CFUnlockerDialogue icon retrieval
}

//
//  CFUnlockerDialogue event table definition
//

BEGIN_EVENT_TABLE( CFUnlockerDialogue, wxDialog )

//   EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK, CFUnlockerDialogue::OnSelChanging )
   EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK, CFUnlockerDialogue::OnSelChanging )
   EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK,  CFUnlockerDialogue::OnSelChanged )
END_EVENT_TABLE()

//! wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK
//!
//! @param event The event to handle
//!
void CFUnlockerDialogue::OnSelChanging( wxNotebookEvent& event ) {
   print("CFUnlockerDialogue::OnNotebookPageChanging() - %d => %d\n", event.GetOldSelection(), event.GetSelection());
   USBDM_ErrorCode rc = BDM_RC_OK;

   int leavingPage = event.GetOldSelection();
   if (leavingPage < 0) {
      return;
   }
   // Validate page before leaving
   wxPanel *panel = static_cast<wxPanel *>(noteBook->GetPage(leavingPage));
//   if (!panel->TransferDataFromWindow()) {
//      event.Veto();
//      return;
//   }
   if (panel == usbdmPanel) {
      // Leaving Communication page - Try to open BDM
      print("CFUnlockerDialogue::OnNotebookPageChanging() - opening BDM\n");
      rc = usbdmPanel->openBdm();
      if (rc != BDM_RC_OK) {
         print("CFUnlockerDialogue::OnNotebookPageChanging() - openBdm() failed\n");
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
   else if (event.GetOldSelection() == 1) {
      // Leaving Programming page
      print("CFUnlockerDialogue::OnSelChanging() - closing BDM\n");
      USBDM_Close();
      }
   if (rc != BDM_RC_OK)
      event.Veto();
}

//! wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK
//!
//! @param event The event to handle
//!
void CFUnlockerDialogue::OnSelChanged( wxNotebookEvent& event ) {
   print("USBDMDialogue::OnSelChanged(%d => %d)\n", event.GetOldSelection(), event.GetSelection());
   int enteringPage = event.GetSelection();
   if (enteringPage < 0) {
      return;
   }
   wxPanel *panel = static_cast<wxPanel *>(noteBook->GetPage(event.GetSelection()));
   if (panel == usbdmPanel) {
      // Entering Communication page - Close the BDM
      USBDM_Close();
      }
   if (panel == cfUnlockerPanel) {
      USBDM_ExtendedOptions_t bdmOptions;
      getBdmOptions(bdmOptions);
      cfUnlockerPanel->setBdmOptions(&bdmOptions);
   }
}

