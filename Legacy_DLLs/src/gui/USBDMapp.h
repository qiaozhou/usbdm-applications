/////////////////////////////////////////////////////////////////////////////
// Name:        usbdmapp.h
// Purpose:     
// Author:      Peter O'Donoghue
// Modified by: 
// Created:     Thu 01 Jul 2010 01:54:58 EST
// RCS-ID:      
// Copyright:   GPL License
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _USBDMAPP_H_
#define _USBDMAPP_H_

#include "wx/image.h"
#include "wx/aboutdlg.h"

/*!
 * USBDMApp class declaration
 */
class TestUsbdmApp: public wxApp
{    
    DECLARE_CLASS( TestUsbdmApp )
    DECLARE_EVENT_TABLE()

public:
    // Constructor
    TestUsbdmApp();

    void Init();
    void Exit();
    void ShowSimpleAboutDialog(wxCommandEvent& WXUNUSED(event));

//    // Initialises the application
//    virtual bool OnInit();
//
//    // Called on exit
//    virtual int OnExit();
};

/*!
 * Application instance declaration 
 */
// Implements USBDMApp& wxGetApp()
DECLARE_APP(TestUsbdmApp)

#endif
    // _USBDMAPP_H_
