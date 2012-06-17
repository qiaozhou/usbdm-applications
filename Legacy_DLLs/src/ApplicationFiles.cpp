/*! \file
    \brief Provides Access to Application files on WIN32

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
   | 16 Nov 2009 | Created                                                - pgo
   +============================================================================
   \endverbatim
*/

#include <stdio.h>
#include "USBDM_API.h"

#include <wx/wx.h>
#include <wx/stdpaths.h>

#include "Log.h"

//! Opens configuration file
//!
//! @param fileName   - Name of file (without path)
//! @param attributes - Attributes to use when opening the file.
//!                     This should be one of "r", "w", "rt" or "wt"
//!
//! @return file handle or NULL on error
//!
//! @note Attempts to open readonly files from two locations:
//! - Application data directory (e.g. %APPDATA%\usbdm, $HOME/.usbdm)
//! - Application directory (where the .exe is)
//! Files opened for writing only use the first directory.
//!
FILE *openApplicationFile(const wxString &filename, const wxString &attributes) {

   wxString configFilePath;
   FILE *configFile = NULL;

   // Try the Application Data directory
   configFilePath = wxStandardPaths::Get().GetUserDataDir();
   // fprintf(stderr, "openApplicationFile() - \"%s\"\n", (const char *)configFilePath.ToAscii());
   if (configFilePath != wxEmptyString) {
      // Create folder if necessary
      if (!::wxDirExists(configFilePath))
         wxMkdir(configFilePath);

      // Append filename
      configFilePath += _("/") + filename;

      // Open the file
      configFile = fopen(configFilePath.To8BitData(), attributes.To8BitData());
   }

   // Try the Executable directory for readonly files
   if ((configFile == NULL ) && (attributes[0] == 'r')) {
      configFilePath = wxStandardPaths::Get().GetExecutablePath();
      if (configFilePath != wxEmptyString) {
         // Append filename
         configFilePath += _("/") + filename;

         // Open the file
         configFile = fopen(configFilePath.To8BitData(), attributes.To8BitData());
      }
   }

   if (configFile != NULL) {
      print("openApplicationFile() - Opened \'%s\'\n", (const char *)configFilePath.To8BitData());
   }
   return configFile;
}

//! Check for existence of a configuration file
//!
//! @param fileName   - Name of file (without path)
//! @param attributes - Attributes to use when opening the file.
//!                     This should be one of "r", "w", "rt" or "wt"
//!
//! @return BDM_RC_OK if file exists
//!
//! @note Attempts to locate the file in  two locations:
//! - Application data directory (e.g. %APPDATA%\usbdm, $HOME/.usbdm)
//! - Application directory (where the .exe is)
//!
int checkExistsApplicationFile(const wxString &filename) {

   wxString configFilePath;

   // Try the Application Data directory first
   configFilePath = wxStandardPaths::Get().GetUserDataDir() + _("/") + filename;;
   if (::wxFileExists(configFilePath))
      return BDM_RC_OK;

   // Try the Executable directory for readonly files
   configFilePath = wxStandardPaths::Get().GetExecutablePath() + _("/") + filename;
   if (::wxFileExists(configFilePath))
      return BDM_RC_OK;

   return BDM_RC_FAIL;
}
