/*
 * wxPlugin.h
 *
 *  Created on: 28/04/2011
 *      Author: PODonoghue
 */

#ifndef WXPLUGIN_H_
#define WXPLUGIN_H_

#if defined __cplusplus
    extern "C" {
#endif

#ifdef WIN32
#ifndef WINAPI
#define WINAPI __attribute__((__stdcall__)) //!< -
#endif

#define EXPORT_API extern "C" WINAPI __declspec(dllexport)
#else
#define EXPORT_API extern "C" __attribute__ ((visibility ("default")))
#endif

#include <wx/defs.h>

// Opaque type
typedef void WindowHandle;

EXPORT_API
WindowHandle *getWindowHandle(const char *windowName);
EXPORT_API
int setDefaultWindowParent(WindowHandle *windowHandle);
EXPORT_API
int displayDialogue(const char *message, const char *caption, long style);
EXPORT_API
int getDataDir(char *buff, int size);
EXPORT_API
int getUserDataDir(char *buff, int size);
#ifdef _WIN32
EXPORT_API
int getInstallationDir(char *buff, int size);
#endif
EXPORT_API
char getPathSeparator(void);

#if defined __cplusplus
    }
#endif


#endif /* WXPLUGIN_H_ */
