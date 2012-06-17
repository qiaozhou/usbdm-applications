/*
 * tclInterface.h
 *
 *  Created on: 01/05/2012
 *      Author: PODonoghue
 */

#ifndef TCLINTERFACE_H_
#define TCLINTERFACE_H_

#include "USBDM_API.h"
#include "DeviceData.h"
#include "usbdmTcl.h"

class TclInterface {
private:
   TargetType_t     targetType;
   DeviceData      *deviceData;
   Tcl_Interp      *tclInterpreter;  //!< TCL interpreter
   USBDM_ErrorCode  initRc;

public:
   TclInterface(TargetType_t targetType, DeviceData *deviceData) :
      targetType(targetType),
      deviceData(deviceData),
      tclInterpreter(NULL),
      initRc(BDM_RC_OK)
   {
      initRc = initTCL();
   }
public:
   ~TclInterface () {
      if (tclInterpreter != NULL) {
         freeTclInterpreter(tclInterpreter);
      }
   }
   //=======================================================================
   // Initialises TCL support for current target
   //
private:
   USBDM_ErrorCode initTCL(void);

   //=======================================================================
   // Executes a TCL script in the current TCL interpreter
   //
public:
   USBDM_ErrorCode runTCLScript(TclScriptPtr script);

   //=======================================================================
   // Executes a TCL command previously loaded in the TCL interpreter
   //
public:
   USBDM_ErrorCode runTCLCommand(const char *command);
};

#endif /* TCLINTERFACE_H_ */
