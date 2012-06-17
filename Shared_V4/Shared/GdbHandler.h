/*
 * gdbHandler.h
 *
 *  Created on: 22/05/2011
 *      Author: podonoghue
 */

#ifndef GDBHANDLER_H_
#define GDBHANDLER_H_
#include "GdbInput.h"
#include "GdbOutput.h"
#include "ProgressTimer.h"

void handleGdb(GdbInput *gdbIn, GdbOutput *gdbOutput, DeviceData &deviceData, ProgressTimer *progressTimer);
void reportError(USBDM_ErrorCode rc);

#endif /* GDBHANDLER_H_ */
