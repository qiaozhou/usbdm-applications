/*! \file
    \brief Provides persistent application settings

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
   |             |                                                         - pgo
   +============================================================================
   \endverbatim
*/
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
using namespace std;
#include <wx/wx.h>
#include <wx/filename.h>
#include "Log.h"
#include "ApplicationFiles.h"
#include "AppSettings.h"

//! Get settings filename
//!
//! @param rootFilename - Start of filename to use,
//!                       may be wxEmptyString in which case a default will be used.
//!                       A suffix is added to identify the target type.
//! @param targetType  - Target type used to select suffix
//!
//! @return wxString representing the filename
//!
wxString AppSettings::getSettingsFilename(const wxString &rootFilename, TargetType_t targetType) {

   wxString filename = rootFilename;

   if (filename == wxEmptyString) {
#ifdef FLASH_PROGRAMMER
      wxString filename = _("FlashProgrammer_");
#else
      wxString filename = _("TargetSettings_");
#endif
   }
   switch (targetType) {
      case T_HC12:         filename += _("HC12.cfg");      break;
      case T_HCS08:        filename += _("HCS08.cfg");     break;
      case T_RS08:         filename += _("RS08.cfg");      break;
      case T_CFV1:         filename += _("CFV1.cfg");      break;
      case T_CFVx:         filename += _("CFVx.cfg");      break;
      case T_JTAG:         filename += _("JTAG.cfg");      break;
      case T_MC56F80xx :   filename += _("MC56F80xx.cfg"); break;
      default:             filename += _("generic.cfg");   break;
   }
   return filename;
}

//! Write settings to file
//!
//! @param fp      - open file to write to
//! @param comment - wxString to add as comment line in file.
//!
void AppSettings::writeToFile(FILE *fp, const wxString &comment) const {
   map<wxString,Value*>::const_iterator it;

   fprintf(fp, "# %s\n\n", (const char *)comment.ToAscii()); fflush(fp);
   for (it=mymap.begin(); it != mymap.end(); it++ ) {
      switch (it->second->getType()) {
         case intType:
            fprintf(fp, "%s %d,%d\n", (const char *)it->first.ToAscii(), it->second->getType(),
                                      it->second->getIntValue()); fflush(fp);
            break;
         case stringType:
            fprintf(fp, "%s %d,%s\n", (const char *)it->first.ToAscii(), it->second->getType(),
                                      (const char *)it->second->getStringValue().ToAscii()); fflush(fp);
            break;
      }
   }
}

//! Write settings to file
//!
//! @param filename - name of file to write to (may have path)
//! @param comment  - wxString to add as comment line in file.
//!
bool AppSettings::writeToFile(const wxString &fileName, wxString const &comment) const {

   FILE *fp = fopen(fileName.ToAscii(), "wt");
   if (fp== NULL) {
#ifdef LOG
      const wxString cwd = wxFileName::GetCwd();
      print("AppSettings::writeToFile() - cwd = \'%s\'\n", (const char *)cwd.ToAscii());
#endif
      print("AppSettings::writeToFile() - Failed to open Settings File for writing: File = \'%s\'\n",
            (const char *)fileName.ToAscii());
      return false;
   }
   writeToFile(fp, comment);
   fclose(fp);
   return true;
}

//! Write settings to file in %APPDATA% system directory
//!
//! @param filename - name of file to write to (no path)
//! @param comment  - wxString to add as comment line in file.
//!
bool AppSettings::writeToAppDirFile(const wxString &fileName, const wxString &comment) const {

   FILE *fp = openApplicationFile(fileName, _("wt"));
   if (fp== NULL) {
      print("AppSettings::writeToAppDirFile() - Failed to open Settings File for writing: File = \'%s\'\n",
            (const char *)fileName.ToAscii());
      return false;
   }
   writeToFile(fp, comment);
   fclose(fp);
   return true;
}

//! Write settings to log file for debugging
//!
void AppSettings::printToLog() const {
   ::print("=============================================================\n");
   map<wxString,Value*>::const_iterator it;
   for (it=mymap.begin(); it != mymap.end(); it++ )
      switch (it->second->getType()) {
         case intType:
            ::print("%s => %d, %d\n", (const char *)it->first.ToAscii(), it->second->getType(), it->second->getIntValue());
            break;
         case stringType:
            ::print("%s => %d, '%s'\n", (const char *)it->first.ToAscii(), it->second->getType(),
                                        (const char *)it->second->getStringValue().ToAscii());
            break;
         default:
            ::print("Unknown attribute type\n");
            break;
      }
   ::print("=============================================================\n");
}

//! Read settings from file
//!
//! @param fp      - open file to read from
//!
void AppSettings::loadFromFile(FILE *fp) {
   char lineBuff[200];
   char *optionName;
   char *optionString;
   char *cptr;
   long int optionInt;
   long int optionType;
   char *cp;
   int lineNo = 0;
   print("AppSettings::loadFromFile(FILE *fp)\n");

   while (fgets(lineBuff, sizeof(lineBuff), fp) != NULL) {
      lineNo++;
//         print("original: %s", lineBuff);
      // Remove comments
      cp = strchr(lineBuff, '#');
      if (cp != NULL)
         *cp = '\0';
      // Remove eol
      cp = strchr(lineBuff, '\n');
      if (cp != NULL)
         *cp = '\0';
      cp = strchr(lineBuff, '\r');
      if (cp != NULL)
         *cp = '\0';
      // Discard empty lines (white space only)
      cp = lineBuff;
      while (*cp == ' ')
         cp++;
      if (*cp == '\0')
         continue;

//      print("comment stripped: \'%s\'\n", lineBuff);
      optionName = strtok(lineBuff, " =");
      cptr       = strtok(NULL, ", ");
      optionType = atoi(cptr);
      optionString = strtok(NULL, "");
      switch (optionType) {
         case intType:
            optionInt = atoi(optionString);
            addValue(wxString(optionName, wxConvUTF8), optionInt);
            break;
         case stringType:
            addValue(wxString(optionName, wxConvUTF8), optionString);
            break;
      }
   }
}

//! Load settings from file
//!
//! @param filename - name of file to read from (may include path)
//!
bool AppSettings::loadFromFile(const wxString &fileName) {

   print("AppSettings::loadFromFile(%s)\n", (const char *)fileName.ToAscii());

   FILE *fp = fopen(fileName.ToAscii(), "rt");
   if (fp== NULL) {
#ifdef LOG
      wxString cwd = wxFileName::GetCwd();
      print("AppSettings::loadFromFile() - cwd = \'%s\'\n", (const char*)cwd.ToAscii());
#endif
      print("AppSettings::loadFromFile() - Failed to open Settings File for reading: File = \'%s\'\n",
            (const char*)fileName.ToAscii());
     return false;
   }
   loadFromFile(fp);
   fclose(fp);
   return true;
}

//! Load settings from file in %APPDATA% system directory
//!
//! @param filename - name of file to read from (no path)
//!
bool AppSettings::loadFromAppDirFile(const wxString &fileName) {

   FILE *fp = openApplicationFile(fileName, _("rt"));
   if (fp== NULL) {
      print("AppSettings::loadFromAppDirFile() - Failed to open Settings File for reading: File = \'%s\'\n",
            (const char*)fileName.ToAscii());
      return false;
   }
   loadFromFile(fp);
   fclose(fp);
   return true;
}
