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
#  ifndef WINAPI
#     define WINAPI __attribute__((__stdcall__)) //!< -
#  endif
#  define EXPORT_API WINAPI __declspec(dllexport)
#else
#  define EXPORT_API __attribute__ ((visibility ("default")))

#endif

#ifndef useWxWidgets

#define wxYES                   0x00000002
#define wxOK                    0x00000004
#define wxNO                    0x00000008
#define wxYES_NO                (wxYES | wxNO)
#define wxCANCEL                0x00000010

#define wxYES_DEFAULT           0x00000000
#define wxNO_DEFAULT            0x00000080

#define wxICON_EXCLAMATION      0x00000100
#define wxICON_HAND             0x00000200
#define wxICON_WARNING          wxICON_EXCLAMATION
#define wxICON_ERROR            wxICON_HAND
#define wxICON_QUESTION         0x00000400
#define wxICON_INFORMATION      0x00000800
#define wxICON_STOP             wxICON_HAND
#define wxICON_ASTERISK         wxICON_INFORMATION
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
EXPORT_API
char getPathSeparator(void);

#else

#define displayDialogue(message, caption, style)          \
      wxMessageBox(                                       \
         wxString((message), wxConvUTF8), /* message */   \
         wxString((caption), wxConvUTF8), /* caption */   \
         (style),                         /* style   */   \
         NULL                             /* parent  */   \
         );

#endif

#if defined __cplusplus
    }
#endif


#endif /* WXPLUGIN_H_ */
