/*
 * usbdmTcl.h
 *
 *  Created on: 03/06/2011
 *      Author: pgo
 */

#ifndef USBDMTCL_H_
#define USBDMTCL_H_

#if defined __cplusplus
    extern "C" {
#endif

#ifdef __unix__
   #define TCL_API __attribute__ ((visibility ("default")))
#else
   #define TCL_API  __declspec(dllexport)
#endif

TCL_API Tcl_Interp *createTclInterpreter(TargetType_t target, FILE *fp);
TCL_API void freeTclInterpreter(Tcl_Interp *interp);
TCL_API int evalTclScript(Tcl_Interp *interp, const char *script);

#if defined __cplusplus
    }
#endif

#endif /* USBDMTCL_H_ */
