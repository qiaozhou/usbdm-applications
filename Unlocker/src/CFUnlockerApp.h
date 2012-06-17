/////////////////////////////////////////////////////////////////////////////
// Name:        cfunlockerapp.h
// Purpose:
// Author:      Peter O'Donoghue
// Modified by:
// Created:     26/08/2010 14:38:12
// RCS-ID:
// Copyright:   GPL License
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _CFUNLOCKERAPP_H_
#define _CFUNLOCKERAPP_H_

#include "wx/image.h"
#include "CFUnlockerPanel.h"

/*!
 * CFUnlockerApp class declaration
 */
class CFUnlockerApp: public wxApp
{
    DECLARE_CLASS( CFUnlockerApp )
    DECLARE_EVENT_TABLE()

public:
    /// Constructor
    CFUnlockerApp();

    void Init();

    /// Initialises the application
    virtual bool OnInit();

    /// Called on exit
    virtual int OnExit();

};

/*!
 * Application instance declaration
 */

DECLARE_APP(CFUnlockerApp)
#endif
