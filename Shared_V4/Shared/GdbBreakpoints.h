/*
 * GdbBreakpoints.h
 *
 *  Created on: 28/07/2011
 *      Author: podonoghue
 */

#ifndef GDBBREAKPOINTS_H_
#define GDBBREAKPOINTS_H_

#include <stdint.h>

typedef enum {memoryBreak, hardBreak, writeWatch, readWatch, accessWatch} breakType;

extern void clearAllBreakpoints(void);
extern void checkAndAdjustBreakpointHalt(void);
extern int  insertBreakpoint(breakType type, uint32_t address, unsigned size);
extern int  removeBreakpoint(breakType type, uint32_t address, unsigned size);
extern void activateBreakpoints(void);
extern void deactivateBreakpoints(void);
extern int  atMemoryBreakpoint();

#endif /* GDBBREAKPOINTS_H_ */
