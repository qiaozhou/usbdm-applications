
/*! \file
    \brief Header file for Log.c (Logging features)
*/
#ifndef _LOG_H_
#define _LOG_H_

//#include <wx/wx.h>
#define BYTE_ADDRESS (0<<0)  // Addresses identify a byte in memory
#define WORD_ADDRESS (1<<0)  // Addresses identify a word in memory
#define BYTE_DISPLAY (0<<2)  // Display as bytes (8-bits)
#define WORD_DISPLAY (1<<2)  // Display as words (16-bits)
#define LONG_DISPLAY (2<<2)  // Display as longs (32-bits)

#ifndef LOG
// Dummy routines if logging is not required
   inline void print(const char *format, ...) { return; }
   inline void print(const wchar_t *format, ...) { return; }
   inline void printDump(unsigned const char *data,
                         unsigned int size,
                         unsigned int dummyStartAddress=0x0000,
                         unsigned int organization=BYTE_ADDRESS|BYTE_DISPLAY) { return; }
   inline void openLogFile(const char *description="USBDM Log File") { return; }
   inline void closeLogFile(void) { return; }
   inline void enableLogging(bool value) { return; }
#else // LOG
void print(const char *format, ...);
void print(const wchar_t *format, ...);
void printDump(unsigned const char *data,
               unsigned int size,
               unsigned int dummyStartAddress=0x0000,
               unsigned int organization=BYTE_ADDRESS|BYTE_DISPLAY);
void openLogFile(const char *description="USBDM Log File");
void closeLogFile(void);
void enableLogging(bool value);
#endif

// To Enable Debug log file
//#define LOG

#endif // _LOG_H_
