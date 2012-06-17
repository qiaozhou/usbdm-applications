/*! \file
    \brief Provides Debug logging facilities

    \verbatim
    Turbo BDM Light - message log
    Copyright (C) 2005  Daniel Malik

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
   -===============================================================================
   | 16 Nov 2009 | Relocated log file directory for Vista.                    - pgo
   |  5 Nov 2009 | Completed restructure for V1                               - pgo
   | 22 May 2009 | Added Speed options for CFVx & JTAG targets                - pgo
   |  3 Apr 2009 | Re-enabled connection retry on successful connect          - pgo
   | 30 Mar 2009 | Changed timing of reset/bkgd release in extendedConnect()  - pgo
   | 27 Dec 2008 | Added extendedConnect() and associated changes             - pgo
   +===============================================================================
   \endverbatim
*/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>
#include "USBDM_API.h"
#include "Log.h"
#include "Version.h"

#ifdef UNICODE
#undef UNICODE
#endif

#define CONFIG_WITH_SLASHES "/" CONFIGURATION_DIRECTORY_NAME "/"

#ifdef LOG

   //! File handle for logging file
   static FILE *logFile=NULL;
   static bool loggingEnabled = true;

   /*! \brief Provides a print function which prints data into a log file.
    *
    *  @param format Format and parameters as for printf()
    */
   void enableLogging(bool value) {
//      if (value == false)
//         print("Disabling logging\n");
      loggingEnabled = value;
//      if (value == true)
//         print("Enabling logging\n");
   }

	/*! \brief Provides a print function which prints data into a log file.
	 *
	 *  @param format Format and parameters as for printf()
	 */
	void print(const wchar_t *format, ...) {
		va_list list;

		if ((logFile == NULL) || (!loggingEnabled))
		   return;

		if (format == NULL) {
		   format = L"print() - Error - empty format string!\n";
		}
		va_start(list, format);
		//fprintf(log_file, "%04.3f: ",1.0*GetTickCount()/1000);
        vfwprintf(logFile, format, list);
		va_end(list);
		fflush(logFile);
	}

	/*! \brief Provides a print function which prints data into a log file.
	 *
	 *  @param format Format and parameters as for printf()
	 */
	void print(const char *format, ...) {
		va_list list;

		if ((logFile == NULL) || (!loggingEnabled))
		   return;

		if (format == NULL) {
		   format = "print() - Error - empty format string!\n";
		}
		va_start(list, format);
		//fprintf(log_file, "%04.3f: ",1.0*GetTickCount()/1000);
		vfprintf(logFile, format, list);
		va_end(list);
		fflush(logFile);
	}

#define ADDRESS_SIZE_MASK    (3<<0)
#define DISPLAY_SIZE_MASK    (3<<2)

	/*! \brief Print a formatted dump of binary data in Hex
	 *
	 * @param data         Pointer to data to print
	 * @param size         Number of bytes to print
	 * @param startAddress Address to display against values
	 * @param organization Size of data & address increment
	 */
	void printDump(unsigned const char *data,
	               unsigned int size,
	               unsigned int startAddress,
	               unsigned int organization) {
      int eolFlag = true;
      unsigned int sub;
      unsigned int addressShift = 0;
      unsigned int elementSize  = 1;
      unsigned int address      = startAddress;

      if ((logFile == NULL) || (!loggingEnabled))
         return;

      switch (organization&DISPLAY_SIZE_MASK){
         case BYTE_DISPLAY:
            elementSize  = 1;
            break;
         case WORD_DISPLAY:
            elementSize  = 2;
            break;
         case LONG_DISPLAY:
            elementSize  = 4;
            break;
      }
      switch (organization&ADDRESS_SIZE_MASK){
         case BYTE_ADDRESS:
            address = startAddress;
            addressShift = 0;
            break;
         case WORD_ADDRESS:
            address      = startAddress<<1;
            addressShift = 1;
            break;
      }
//      print("org=%x, es=%d, as=%d\n", organization, elementSize, addressShift);
      while(size>0) {
         if (eolFlag){
            eolFlag = false;
            fprintf(logFile,"%8.8X:", address>>addressShift);
         }
         for(sub=0; (sub<elementSize) && (size>0); sub++) {
            fprintf(logFile,"%02X",*(data++));
            size--;
            address++;
            if ((address&0xF) == 0)
               eolFlag = true;
         }
         fprintf(logFile," ");
         if (eolFlag)
            fprintf(logFile,"\n");
      }
      if (!eolFlag)
         fprintf(logFile,"\n");
      fflush(logFile);
	}

#ifdef __unix__
	  static FILE *openApplicationFile(const char *fileName) {

     char tempPathname[64];
     int tempFiledescriptor = -1;
     FILE *fp = NULL;
     strcpy(tempPathname, "/var/tmp/usbdm-XXXXXX"); // Filename template
     tempFiledescriptor=mkstemp(tempPathname);
     if (tempFiledescriptor >=0)
        fp = fdopen(tempFiledescriptor, "w"); // Convert to stream
     return fp;
	}
#endif

#ifdef WIN32
#define _WIN32_IE 0x0500      //!< Required for later system calls.
#define _WIN32_WINNT 0x0500   //!< Required for later system calls.
#include <shlobj.h>
	//! Obtain the path of the configuration directory or file
	//!
	//! @param configFilePath - Buffer of size MAX_PATH to return path in.
	//! @param filename - Configuration file name to append to configuration directory path.\n
	//!                   If NULL then the configuration directory (with trailing \) is returned.
	//!
	//! @return error code - BDM_RC_OK => success \n
	//!                    - BDM_RC_FAIL => failure
	//!
	//! @note The configuration directory will be created if it doesn't aleady exist.
	//!
	static int getApplicationDirectoryPath(const char **_configFilePath,
	                                       const char *filename) {
	   static char configFilePath[MAX_PATH];

	   memset(configFilePath, '\0', MAX_PATH);
	   *_configFilePath = configFilePath;
	   if (filename == NULL)
	      filename = "\\";

	   // Obtain local app folder (create if needed)
	   if (SHGetFolderPath(NULL,
	                       CSIDL_APPDATA|CSIDL_FLAG_CREATE,
	                       NULL,
	                       0,
	                       configFilePath) != S_OK)
	      return BDM_RC_FAIL;

	   // Append application directory name
	   if (strlen(configFilePath)+sizeof(CONFIG_WITH_SLASHES) >= MAX_PATH)
	      return BDM_RC_FAIL;

	   strcat(configFilePath, CONFIG_WITH_SLASHES);

	   // Check if folder exists or can be created
	   if ((GetFileAttributes(configFilePath) == INVALID_FILE_ATTRIBUTES) &&
	       (SHCreateDirectoryEx( NULL, configFilePath, NULL ) != S_OK))
	      return BDM_RC_FAIL;

	   // Append filename if non-NULL
	   if (strlen(configFilePath)+strlen(filename) >= MAX_PATH)
	      return BDM_RC_FAIL;

	   strcat(configFilePath, filename);

	   print("getApplicationDirectoryPath()=\'%s\'\n", configFilePath );
	   return BDM_RC_OK;
	}

	//! Opens configuration file
	//!
	//! @param fileName   - Name of file (without path)
	//!
	//! @return file handle or NULL on error
	//!
	//! @note Attempts to open from:
	//! - Application data directory (e.g. %APPDATA%\usbdm, $HOME/.usbdm)
	//!
	static FILE *openApplicationFile(const char *fileName) {
      FILE *configFile = NULL;
      const char *configFilePath;
      int rc;

	   rc = getApplicationDirectoryPath(&configFilePath, fileName);
	   if (rc == BDM_RC_OK)
	      configFile = fopen(configFilePath, "wt");

	   if (configFile != NULL) {
	      print("openApplicationFile() - Opened \'%s\' from APPDATA directory\n", fileName);
	      return configFile;
	   }
	   return configFile;
	}
#endif

	/*! \brief Open the log file
	 *
	 *  @param LogFileName Path of file to open for logging. \n
	 *                     If NULL a default path is used.
	 */
	void openLogFile(const char *filename){
	   time_t time_now;

	   if (logFile != NULL)
	      fclose(logFile);
      logFile = openApplicationFile(filename);
      print("%s - %s, Compiled on %s, %s.\n",
            filename,
            VERSION_STRING, __DATE__,__TIME__);

      time(&time_now);
      print("Log file created on: %s"
            "==============================================\n\n", ctime(&time_now));
	}

   /*!  \brief Close the log file
    *
    */
   void closeLogFile(void){
      time_t time_now;

      if (logFile == NULL)
         return;

      loggingEnabled = true;

      time(&time_now);
      print("\n==========================================\n"
            "End of log file: %s\r", ctime(&time_now));
      fclose(logFile);
      logFile = NULL;
	}
#endif // LOG
