/*
 * tclInterface.cpp
 *
 *  Created on: 01/05/2012
 *      Author: PODonoghue
 */
#include "tclInterface.h"

//=======================================================================
// Initialises TCL support for current target
//
USBDM_ErrorCode TclInterface::initTCL(void) {

   print("TclInterface::initTCL()\n");

//   FILE *fp = fopen("c:/delme.log", "wt");
//   tclInterpreter = createTclInterpreter(TARGET_TYPE, fp);
   tclInterpreter = createTclInterpreter(targetType, getLogFileHandle());
   if (tclInterpreter == NULL) {
      print("TclInterface::initTCL() - no TCL interpreter\n");
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
   // Run initial TCL script (loads routines)
   TclScriptPtr script = deviceData->getFlashScripts();
   if (!script) {
      print("TclInterface::initTCL() - no TCL script found\n");
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
#if defined(LOG) && 0
   print("TclInterface::initTCL()\n");
   print(script->toString().c_str());
#endif
   USBDM_ErrorCode rc = runTCLScript(script);
   if (rc != BDM_RC_OK) {
      print("TclInterface::initTCL() - runTCLScript() failed\n");
      return rc;
   }
   return BDM_RC_OK;
}

//=======================================================================
// Executes a TCL script in the current TCL interpreter
//
USBDM_ErrorCode TclInterface::runTCLScript(TclScriptPtr script) {
   print("TclInterface::runTCLScript(): Running TCL Script...\n");

   if (initRc != BDM_RC_OK) {
      return initRc;
   }
   if (evalTclScript(tclInterpreter, script->getScript().c_str()) != 0) {
      print("TclInterface::runTCLScript(): Failed\n");
      print(script->toString().c_str());
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
//   print("TclInterface::runTCLScript(): Script = %s\n",
//          (const char *)script->getScript().c_str());
//   print("TclInterface::runTCLScript(): Complete\n");
   return BDM_RC_OK;
}

//=======================================================================
// Executes a TCL command previously loaded in the TCL interpreter
//
USBDM_ErrorCode TclInterface::runTCLCommand(const char *command) {
   print("TclInterface::runTCLCommand(\"%s\")\n", command);

   if (initRc != BDM_RC_OK) {
      return initRc;
   }
   if (evalTclScript(tclInterpreter, command) != 0) {
      print("TclInterface::runTCLCommand(): TCL Command '%s' failed\n", command);
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
//   print("TclInterface::runTCLCommand(): Complete\n");
   return BDM_RC_OK;
}
