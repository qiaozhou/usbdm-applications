/*! \file
    \brief Header file for USBDM_CFF.c

*/
#ifndef USBDM_CFF_H_
#define USBDM_CFF_H_

#ifndef WINAPI
#define WINAPI __attribute__((__stdcall__)) //!< -
#endif

#if defined __cplusplus
    extern "C" {
#endif

#ifdef WIN32
   #if defined(DLL)
      //! This definition is used when USBDM_API.h is being exported (creating DLL)
      #define CFF_API extern "C" __declspec(dllexport)
   #elif defined (TESTER) || defined (DOC) || defined (BOOTLOADER)
      //! This empty definition is used for the command-line version that statically links the DLL routines
   #define CFF_API extern "C"
   #else
       //! This definition is used when USBDM_API.h is being imported (linking against DLL)
      #define CFF_API extern "C" __declspec(dllimport)
   #endif
#endif

#ifdef __unix__
   #if defined(DLL)
      //! This definition is used when USBDM_API.h is being exported (creating DLL)
      #define CFF_API extern "C" __attribute__ ((visibility ("default")))
   #else
       //! This definition is used when USBDM_API.h is being imported etc
      #define CFF_API extern "C"
   #endif
#endif

#if defined __cplusplus
    }
#endif


#endif /* USBDM_CFF_H_ */
