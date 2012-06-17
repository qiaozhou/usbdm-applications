/*! \file
    \brief Provides Debug logging facilities

    Log.cpp

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
#include "Common.h"
#include "ApplicationFiles.h"
#include "Log.h"
#include "Version.h"

#ifdef LOG

   //! File handle for logging file
   static FILE *logFile=NULL;
   static bool loggingEnabled = true;

   FILE *getLogFileHandle(void) {
      return logFile;
   }

   void setLogFileHandle(FILE *fp) {
      logFile = fp;
   }

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

      if (!loggingEnabled)
         return;
//      if (logFile == NULL) {
//         openLogFile("DefaultLogFile.log", "Auto opened log file");
//      }
      if (logFile == NULL) {
         return;
      }
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

      if (!loggingEnabled)
         return;
//      if (logFile == NULL) {
//         openLogFile("DefaultLogFile.txt", "Auto opened log file");
//      }
      if (logFile == NULL) {
         return;
      }
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
      unsigned int addressShift = 0;
      unsigned int elementSize  = 1;
      unsigned int address      = startAddress;
      bool         littleEndian = (organization & DLITTLE_ENDIAN)!= 0;

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
//      print("org=%x, sz=%d, es=%d, as=%d\n", organization, size, elementSize, addressShift);
      int eolFlag = true;
      while(size>0) {
         if (eolFlag) {
            eolFlag = false;
            fprintf(logFile,"   %8.8X:", address>>addressShift);
         }
         unsigned char dataTemp[4];
         unsigned int sub;
         for(sub=0; (sub<elementSize) && (size>0); sub++) {
            dataTemp[sub] = *data++;
            address++;
            size--;
            if ((address&0xF) == 0)
               eolFlag = true;
         }
         unsigned int indx;
         if (littleEndian) {
            indx=sub-1;
            do {
               fprintf(logFile, "%02X", dataTemp[indx]);
            } while (indx-- > 0) ;
         }
         else {
            for(indx=0; indx<sub; indx++) {
               fprintf(logFile, "%02X", dataTemp[indx]);
            }
         }
         fprintf(logFile," ");
         if (eolFlag)
            fprintf(logFile,"\n");
      }
      if (!eolFlag)
         fprintf(logFile,"\n");
      fflush(logFile);
	}

#if ((TARGET != ARM) && (TARGET != MC56F80xx)) || !defined(DLL)
	/*! \brief Open the log file
	 *
	 *  @param LogFileName Path of file to open for logging. \n
	 *                     If NULL a default path is used.
	 */
	void openLogFile(const char *logFileName, const char *description){
	   time_t time_now;

	   if (logFile != NULL) {
	      fclose(logFile);
	//      return;
	   }
	   bool oldLoggingEnabled = loggingEnabled; // Prevent auto log file opening
	   loggingEnabled = false;
      logFile = openApplicationFile(logFileName, "wt");
      loggingEnabled = oldLoggingEnabled;
      if (logFile == NULL) {
         logFile = fopen("C:\\Documents and Settings\\PODonoghue\\Application Data\\usbdm\\xx.log", "wt");
//         print("openLogFile() - failed to open %s\n", logFileName);
      }
      if (logFile == NULL) {
         return;
      }
      print("%s - %s, Compiled on %s, %s.\n",
            description,
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
      loggingEnabled = false;
      fclose(logFile);
      logFile = NULL;
	}
#endif
#endif // LOG
