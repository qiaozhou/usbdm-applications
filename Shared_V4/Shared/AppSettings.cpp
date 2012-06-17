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
#include <string>
using namespace std;
#include <string.h>

#include "Common.h"
#include "Log.h"
#include "ApplicationFiles.h"
#include "AppSettings.h"

//! Get settings filename
//!
//! @param rootFilename - Start of filename to use,
//!                       may be empty in which case a default will be used.
//!                       A suffix is added to identify the target type.
//! @param targetType  - Target type used to select suffix
//!
//! @return string representing the filename
//!
string AppSettings::getSettingsFilename(const string &rootFilename, TargetType_t targetType) {

   string filename(rootFilename);
   if (filename.empty()) {
#ifdef FLASH_PROGRAMMER
      filename = "FlashProgrammer_";
#else
      filename = "TargetSettings_";
#endif
   }
   switch (targetType) {
      case T_HC12:         filename += "HC12.cfg";      break;
      case T_HCS08:        filename += "HCS08.cfg";     break;
      case T_RS08:         filename += "RS08.cfg";      break;
      case T_CFV1:         filename += "CFV1.cfg";      break;
      case T_CFVx:         filename += "CFVx.cfg";      break;
      case T_JTAG:         filename += "JTAG.cfg";      break;
      case T_ARM_JTAG:     filename += "ARM.cfg";       break;
      case T_MC56F80xx :   filename += "MC56F80xx.cfg"; break;
      default:             filename += "generic.cfg";   break;
   }
   return filename;
}

//! Write settings to file
//!
//! @param fp      - open file to write to
//! @param comment - string to add as comment line in file.
//!
void AppSettings::writeToFile(FILE *fp, const string &comment) const {
   map<string,Value*>::const_iterator it;

   fprintf(fp, "# %s\n\n", comment.c_str()); fflush(fp);
   for (it=mymap.begin(); it != mymap.end(); it++ ) {
      switch (it->second->getType()) {
         case intType:
            fprintf(fp, "%s %d,%d\n", it->first.c_str(), it->second->getType(),
                                      it->second->getIntValue()); fflush(fp);
            break;
         case stringType:
            fprintf(fp, "%s %d,%s\n", it->first.c_str(), it->second->getType(),
                                      it->second->getStringValue().c_str()); fflush(fp);
            break;
      }
   }
}

//! Write settings to file
//!
//! @param fileName - name of file to write to (may have path)
//! @param comment  - string to add as comment line in file.
//!
bool AppSettings::writeToFile(const string &fileName, string const &comment) const {

   FILE *fp = fopen(fileName.c_str(), "wt");
   if (fp== NULL) {
      print("AppSettings::writeToFile() - Failed to open Settings File for writing: File = \'%s\'\n",
            fileName.c_str());
      return false;
   }
   writeToFile(fp, comment.c_str());
   fclose(fp);
   return true;
}

//! Write settings to file in %APPDATA% system directory
//!
//! @param fileName - name of file to write to (no path)
//! @param comment  - string to add as comment line in file.
//!
bool AppSettings::writeToAppDirFile(const string &fileName, const string &comment) const {
   FILE *fp = openApplicationFile(fileName.c_str(), "wt");
   if (fp== NULL) {
      print("AppSettings::writeToAppDirFile() - Failed to open Settings File for writing: File = \'%s\'\n",
            fileName.c_str());
      return false;
   }
   writeToFile(fp, comment.c_str());
   fclose(fp);
   return true;
}

//! Write settings to log file for debugging
//!
void AppSettings::printToLog() const {
   print("=============================================================\n");
   map<string,Value*>::const_iterator it;
   for (it=mymap.begin(); it != mymap.end(); it++ )
      switch (it->second->getType()) {
         case intType:
            ::print("%s => %d, %d\n", it->first.c_str(), it->second->getType(), it->second->getIntValue());
            break;
         case stringType:
            ::print("%s => %d, '%s'\n", it->first.c_str(), it->second->getType(),
                                        it->second->getStringValue().c_str());
            break;
         default:
            ::print("Unknown attribute type\n");
            break;
      }
   print("=============================================================\n");
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
            addValue(string(optionName), optionInt);
            break;
         case stringType:
            addValue(string(optionName), optionString);
            break;
      }
   }
}

//! Load settings from file
//!
//! @param fileName - name of file to read from (may include path)
//!
bool AppSettings::loadFromFile(const string &fileName) {

   print("AppSettings::loadFromFile(%s)\n", fileName.c_str());

   FILE *fp = fopen(fileName.c_str(), "rt");
   if (fp== NULL) {
      print("AppSettings::loadFromFile() - Failed to open Settings File for reading: File = \'%s\'\n",
            fileName.c_str());
     return false;
   }
   loadFromFile(fp);
   fclose(fp);
   return true;
}

//! Load settings from file in %APPDATA% system directory
//!
//! @param fileName - name of file to read from (no path)
//!
bool AppSettings::loadFromAppDirFile(const string &fileName) {
   string fn(fileName.c_str());
   FILE *fp = openApplicationFile(fn, "rt");
   if (fp== NULL) {
      print("AppSettings::loadFromAppDirFile() - Failed to open Settings File for reading: File = \'%s\'\n",
            fileName.c_str());
      return false;
   }
   loadFromFile(fp);
   fclose(fp);
   return true;
}
