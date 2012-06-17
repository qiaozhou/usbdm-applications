/*! \file
    \brief Flash Programming App

    GDI_Aux.cpp

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
|  25 Feb 2010 | Created                                               4.4.1 - pgo
+=================================================================================
\endverbatim
*/
#include "GDI.h"
#include "Common.h"

#ifdef LOG
static const char *returnStrings[] = {
   "DI_OK",                 //  = 0,
   "DI_ERR_STATE",          //  = 1,
   "DI_ERR_PARAM",          //  = 2,
   "DI_ERR_COMMUNICATION",  //  = 3,
   "DI_ERR_NOTSUPPORTED",   //  = 4,
   "DI_ERR_NONFATAL",       //  = 5,
   "DI_ERR_CANCEL",         //  = 6,
   "DI_ERR_FATAL",          //  = 7
};

const char *getGDIErrorString( DiReturnT returnCode) {
const char *errorString = "Unknown";

   if ((returnCode >= 0) &&
       ((unsigned)returnCode < (sizeof(returnStrings)/sizeof(returnStrings[0]))))
      errorString = returnStrings[returnCode];

   return errorString;
}
//static DiReturnT setErrorState(DiReturnT   errorCode,
//                               const wxString &errorString = wxEmptyString) {
//}
//#else
//static const char *getGDIErrorString( DiReturnT returnCode) { return ""; }
#endif
