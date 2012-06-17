//============================================================================
// Name        : TryTcl.cpp
// Author      : Peter O'Donoghue
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <iostream>
using namespace std;
#include <string.h>
#ifdef WIN32
#include "tcl.h"
#else
#include "tcl8.5/tcl.h"
#endif

int stringCommand(ClientData notneededhere, Tcl_Interp *tclInterp, int argc, const char *argv[]) {
   int index;

   cerr << "Hello from command, argc = " << argc << "\n";
   for (index=0; index < argc; index++) {
      cerr << "argv[" << index << "] = '" << argv[index] << "'\n";
   }
   Tcl_SetResult(tclInterp, (char*)"0", TCL_STATIC);
   return TCL_OK;
}

int objCommand(ClientData notneededhere, Tcl_Interp *tclInterp, int objc, Tcl_Obj *const objv[]) {
   int index;

   cerr << "Hello from command, argc = " << objc << "\n";
   for (index=0; index < objc; index++) {
      cerr << "objv[" << index << "] = '" << Tcl_GetStringFromObj(objv[index], NULL) << "'\n";
   }
   Tcl_SetResult(tclInterp, (char*)"0", TCL_STATIC);
   return TCL_OK;
}

int addMyCommands(Tcl_Interp *tclInterp) {
   Tcl_Command_* rc;

   rc = Tcl_CreateCommand(tclInterp, "stringCommand", stringCommand, NULL,  NULL);
   rc = Tcl_CreateObjCommand(tclInterp, "objCommand", objCommand, NULL,  NULL);

   return TCL_OK;
}

int main(int argc, char *argv[]) {
	int rc;

	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!

	Tcl_FindExecutable(argv[0]);
	Tcl_Interp *tclInterp = Tcl_CreateInterp();
	if (tclInterp == NULL) {
	   cerr << "Failed to create TCL interpreter\n";
	}
	addMyCommands(tclInterp);
   rc = Tcl_Eval(tclInterp, "puts { Hello there from TCL }");
   rc = Tcl_Eval(tclInterp, "stringCommand a b c d e f");
   rc = Tcl_Eval(tclInterp, "objCommand a b c d e f");

	cerr << "rc = " << rc << "\n";

	Tcl_DeleteInterp(tclInterp);
	return 0;
}
