/*
 * Breakpoints.cpp
 *
 *  Created on: 28/07/2011
 *      Author: podonoghue
 */
#include <stddef.h>
#include "Common.h"
#include "Log.h"

#include "USBDM_API.h"
#if TARGET == ARM
#include "USBDM_ARM_API.h"
#include "ARM_Definitions.h"
#endif

#include "GdbBreakpoints.h"

#if TARGET == CFV1
#define USBDM_ReadPC(x)                      USBDM_ReadCReg(CFV1_CRegPC, x);
#define USBDM_WritePC(x)                     USBDM_WriteCReg(CFV1_CRegPC, x);
#define USBDM_ReadSP(x)                      USBDM_ReadCReg(CFV1_CRegSP, x);
#define USBDM_WriteSP(x)                     USBDM_WriteCReg(CFV1_CRegSP, x);
#define USBDM_ReadSR(x)                      USBDM_ReadCReg(CFV1_CRegSR, x);
#define USBDM_WriteSR(x)                     USBDM_WriteCReg(CFV1_CRegSR, x);
#elif TARGET == CFVx
#define USBDM_ReadPC(x)                      USBDM_ReadCReg(CFVx_CRegPC, x);
#define USBDM_WritePC(x)                     USBDM_WriteCReg(CFVx_CRegPC, x);
#define USBDM_ReadSP(x)                      USBDM_ReadCReg(CFVx_CRegSP, x);
#define USBDM_WriteSP(x)                     USBDM_WriteCReg(CFVx_CRegSP, x);
#define USBDM_ReadSR(x)                      USBDM_ReadCReg(CFVx_CRegSR, x);
#define USBDM_WriteSR(x)                     USBDM_WriteCReg(CFVx_CRegSR, x);
#elif TARGET == ARM
#define USBDM_ReadPC(x)                      ARM_ReadRegister(ARM_RegPC, x);
#define USBDM_WritePC(x)                     ARM_WriteRegister(ARM_RegPC, x);
#define USBDM_ReadSP(x)                      ARM_ReadRegister(ARM_RegSP, x);
#define USBDM_WriteSP(x)                     ARM_WriteRegister(ARM_RegSP, x);
#define USBDM_ReadSR(x)                      ARM_ReadRegister(ARM_RegSR, x);
#define USBDM_WriteSR(x)                     ARM_WriteRegister(ARM_RegSR, x);
#define USBDM_ReadReg(reg, x)                ARM_ReadRegister(reg, x);
#define USBDM_WriteReg(reg, x)               ARM_WriteRegister(reg, x);
#define USBDM_ReadMemory(eSz,bC,addr,data)   ARM_ReadMemory(eSz,bC,addr,data)
#define USBDM_WriteMemory(eSz,bC,addr,data)  ARM_WriteMemory(eSz,bC,addr,data)
#define USBDM_TargetReset(x)                 ARM_TargetReset(x)
#define USBDM_TargetHalt()                   ARM_TargetHalt()
#define USBDM_TargetStep()                   ARM_TargetStep()
#define USBDM_TargetGo()                     ARM_TargetGo()
#else
#error "Unhandled TARGET"
#endif

#if (TARGET == CFV1) || (TARGET == CFVx)
#define MAX_MEMORY_BREAKPOINTS   (10)
#define MAX_HARDWARE_BREAKPOINTS (4)
#define MAX_DATA_WATCHES     (1)   // Not implemented
#elif TARGET == ARM
#define MAX_MEMORY_BREAKPOINTS   (10)
#define MAX_HARDWARE_BREAKPOINTS (6)
#define MAX_DATA_WATCHES         (4)
#endif

const char *getBreakpointName(breakType type) {
static const char *names[] = {
      "memoryBreak", "hardBreak", "writeWatch", "readWatch", "accessWatch"
};
const char *typeName = "Invalid";

   if ((unsigned)type < sizeof(names)/sizeof(names[0]))
      typeName = names[type];
   return typeName;
}

//! Converts a uint32_t value to an array of bytes in litteEndian order
//!
//! @param data - value to convert
//!
//! @return ptr to static buffer containing value
//!
inline const uint8_t *getData4x8Le(uint32_t data) {
   static uint8_t data8[4];
   data8[0]= data;
   data8[1]= data>>8;
   data8[2]= data>>16;
   data8[3]= data>>24;
   return data8;
}

//===========================================================
// RAM based software breakpoints based on HALT instruction
//

typedef struct {
   bool  inUse;
   uint32_t   address;
   uint8_t    opcode[2];
} memoryBreakInfo;

static memoryBreakInfo memoryBreakpoints[MAX_MEMORY_BREAKPOINTS] = {
      {false, 0, {0, 0}},
};

static void clearAllMemoryBreakpoints(void) {
   memoryBreakInfo *bpPtr;
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      bpPtr->inUse = false;
   }
}

static memoryBreakInfo *findMemoryBreakpoint(uint32_t address) {
   memoryBreakInfo *bpPtr;
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
//      if (bpPtr->inUse)
//         print("findMemoryBreakpoint() - checking PC=0x%08X against 0x%08X\n",
//                           address, bpPtr->address);
      if (bpPtr->inUse && (bpPtr->address == address))
         return bpPtr;
   }
   return NULL;
}

int atMemoryBreakpoint() {
   unsigned long pcAddress = 0;
   USBDM_ReadPC(&pcAddress);
   return (findMemoryBreakpoint(pcAddress) != NULL);
}

static memoryBreakInfo *findFreeMemoryBreakpoint(void) {
   memoryBreakInfo *bpPtr;
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      if (!bpPtr->inUse)
         return bpPtr;
   }
   return NULL;
}

//================================================================
// Hardware PC breakpoints using breakpoint hardware
//
// 4 available on Coldfire V1 devices.
//
typedef struct {
   bool  inUse;
   uint32_t   address;
} hardwareBreakInfo;

static hardwareBreakInfo hardwareBreakpoints[MAX_HARDWARE_BREAKPOINTS] = {
      {false, 0},
};

static void clearAllHardwareBreakpoints(void) {
   hardwareBreakInfo *bpPtr;
   for (bpPtr = hardwareBreakpoints;
        bpPtr < hardwareBreakpoints+MAX_HARDWARE_BREAKPOINTS;
        bpPtr++) {
      bpPtr->inUse = false;
   }
}

static hardwareBreakInfo *findHardwareBreakpoint(uint32_t address) {
   hardwareBreakInfo *bpPtr;
   for (bpPtr = hardwareBreakpoints;
        bpPtr < hardwareBreakpoints+MAX_HARDWARE_BREAKPOINTS;
        bpPtr++) {
//      if (bpPtr->inUse)
//         print("findHardwareBreakpoint() - checking PC=0x%08X against 0x%08X\n",
//                           address, bpPtr->address);
      if (bpPtr->inUse && (bpPtr->address == address))
         return bpPtr;
   }
   return NULL;
}

static hardwareBreakInfo *findFreeHardwareBreakpoint(void) {
   hardwareBreakInfo *bpPtr;
   for (bpPtr = hardwareBreakpoints;
        bpPtr < hardwareBreakpoints+MAX_HARDWARE_BREAKPOINTS;
        bpPtr++) {
      if (!bpPtr->inUse)
         return bpPtr;
   }
   return NULL;
}

//================================================================
// Hardware data read/write/access watchpoint using hardware
//
// 1 available on Coldfire V1 devices.
//
typedef struct {
   bool        inUse;
   breakType   type;
   uint32_t    address;
   uint32_t    size;
} dataBreakInfo;

static dataBreakInfo dataWatchPoints[MAX_DATA_WATCHES] = {
      {false, readWatch, 0},
};

static void clearAllDataWatchPoints(void) {
   dataBreakInfo *bpPtr;
   for (bpPtr = dataWatchPoints;
        bpPtr < dataWatchPoints+MAX_DATA_WATCHES;
        bpPtr++) {
      bpPtr->inUse = false;
   }
}

static dataBreakInfo *findDataWatchPoint(uint32_t address) {
   dataBreakInfo *bpPtr;
   for (bpPtr = dataWatchPoints;
        bpPtr < dataWatchPoints+MAX_DATA_WATCHES;
        bpPtr++) {
      if (bpPtr->inUse && (bpPtr->address == address))
         return bpPtr;
   }
   return NULL;
}

static dataBreakInfo *findFreeDataWatchPoint(void) {
   dataBreakInfo *bpPtr;
   for (bpPtr = dataWatchPoints;
        bpPtr < dataWatchPoints+MAX_DATA_WATCHES;
        bpPtr++) {
      if (!bpPtr->inUse)
         return bpPtr;
   }
   return NULL;
}

//! Insert breakpoint
//!
//! @param type     one of memoryBreak, hardBreak, writeWatch, readWatch, accessWatch
//! @param address  address for breakpoint/watchpoint
//! @param size     range to watch (watchpoint only)
//!
//! @return true => OK\n
//!         false => error
//!
//! @note writeWatch, readWatch, accessWatch not currently working
//!
int insertBreakpoint(breakType type, uint32_t address, unsigned size) {
   print("insertBreakpoint(%s, a=%08X, s=%d)\n", getBreakpointName(type), address, size);

   switch (type) {
   case memoryBreak: {
      if (findMemoryBreakpoint(address)) {
         return true; // Already set - ignore
      }
      memoryBreakInfo *bpPtr = findFreeMemoryBreakpoint();
      if (bpPtr == NULL) {
         return false; // Too many breakpoints
      }
      bpPtr->address = address;
      bpPtr->inUse   = true;
      return true; // Done
      }
      break;
   case hardBreak: {
      if (findHardwareBreakpoint(address)) {
         return true; // Already set - ignore
      }
      hardwareBreakInfo *bpPtr = findFreeHardwareBreakpoint();
      if (bpPtr == NULL) {
         return false; // Too many breakpoints
      }
      bpPtr->address = address;
      bpPtr->inUse   = true;
      return true; // Done
      }
      break;
   case writeWatch:
   case readWatch:
   case accessWatch: {
      dataBreakInfo *bpPtr = findDataWatchPoint(address);
      if (bpPtr != NULL) {
         if (size > bpPtr->size)
            bpPtr->size = size;
         if (type != bpPtr->type)
            bpPtr->type = accessWatch;
         return true; // Already set - update
      }
      bpPtr = findFreeDataWatchPoint();
      if (bpPtr == NULL) {
         return false; // Too many breakpoints
      }
      bpPtr->address = address;
      bpPtr->size    = size;
      bpPtr->type    = (breakType)type;
      bpPtr->inUse   = true;
      return true; // Done
      }
      break;
   default:
      return false; // Unknown type
   }
}

//! Remove breakpoint
//!
//! @param type     one of memoryBreak, hardBreak, writeWatch, readWatch, accessWatch
//! @param address  address for breakpoint/watchpoint
//! @param size     not used
//!
int removeBreakpoint(breakType type, uint32_t address, unsigned size) {
   print("removeBreakpoint(%s, a=%08X, s=%d)\n", getBreakpointName(type), address, size);
   switch (type) {
   case memoryBreak: {
      memoryBreakInfo *bpPtr = findMemoryBreakpoint(address);
      if (bpPtr == NULL) {
         return false; // Non-existent breakpoint
      }
      bpPtr->inUse     = false;
      bpPtr->address   = 0;
      bpPtr->opcode[0] = 0;
      bpPtr->opcode[1] = 0;
      print("removeBreakpoint(t=MEM, a=%08X, s=%d) - done\n", address, size);
      return true; // Done
   }
   break;
   case hardBreak: {
      hardwareBreakInfo *bpPtr = findHardwareBreakpoint(address);
      if (bpPtr == NULL) {
         return false; // Non-existent breakpoint
      }
      bpPtr->inUse     = false;
      print("removeBreakpoint(t=HW, a=%08X, s=%d) - done\n", address, size);
      return true; // Done
      }
      break;
   case writeWatch:
   case readWatch:
   case accessWatch: {
      dataBreakInfo *bpPtr = findDataWatchPoint(address);
      if (bpPtr == NULL) {
         return false; // Non-existent breakpoint
      }
      bpPtr->inUse     = false;
      return true; // Done
      }
      break;
   default:
      return false; // Unknown type
   }
}

//! Indicates if breakpoints have been inserted/activated in target
static bool breakpointsActive = false;

#if (TARGET == CFV1) || (TARGET == CFVx)

#define TDR_TRC_HALT (1<<30)
#define TDR_L1T      (1<<14)
#define TDR_L1EBL    (1<<13)
#define TDR_L1EPC    (1<<1)
#define TDR_L1EA_INC (1<<3)
#define TDR_DISABLE  (0)

//! Activate breakpoints. \n
//! This may involve changing target code for RAM breakpoints or
//! modifying target breakpoint hardware
//!
void activateBreakpoints(void) {
   print("activateBreakpoints()\n");
   static const uint8_t haltOpcode[] = {0x4a, 0xc8};
   memoryBreakInfo *bpPtr;
   if (breakpointsActive)
      return;
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      if (bpPtr->inUse) {
         print("activateBreakpoints(%s@%08X)\n", getBreakpointName(memoryBreak), bpPtr->address);
         USBDM_ReadMemory(2,2,bpPtr->address,bpPtr->opcode);
         USBDM_WriteMemory(2,2,bpPtr->address,haltOpcode);
         breakpointsActive = true;
      }
   }
   uint32_t tdrValue = TDR_DISABLE;
   if (hardwareBreakpoints[0].inUse) {
      tdrValue |= TDR_TRC_HALT|TDR_L1T|TDR_L1EBL|TDR_L1EPC;
      USBDM_WriteDReg(CFVx_DRegPBR0, hardwareBreakpoints[0].address&~0x1);
      USBDM_WriteDReg(CFVx_DRegPBMR, 0x00000000);
      breakpointsActive = true;
      print("activateBreakpoints(%s@%08X)\n", getBreakpointName(hardBreak),
                                              hardwareBreakpoints[0].address&~0x1);
   }
   if (hardwareBreakpoints[1].inUse) {
      tdrValue |= TDR_TRC_HALT|TDR_L1T|TDR_L1EBL|TDR_L1EPC;
      USBDM_WriteDReg(CFVx_DRegPBR1, hardwareBreakpoints[1].address|0x1);
      breakpointsActive = true;
      print("activateBreakpoints(%s@%08X)\n", getBreakpointName(hardBreak),
                                              hardwareBreakpoints[1].address&~0x1);
   }
   else {
      USBDM_WriteDReg(CFVx_DRegPBR1,0);
   }
   if (hardwareBreakpoints[2].inUse) {
      tdrValue |= TDR_TRC_HALT|TDR_L1T|TDR_L1EBL|TDR_L1EPC;
      USBDM_WriteDReg(CFVx_DRegPBR2, hardwareBreakpoints[2].address|0x1);
      breakpointsActive = true;
      print("activateBreakpoints(%s@%08X)\n", getBreakpointName(hardBreak),
                                              hardwareBreakpoints[2].address&~0x1);
   }
   else {
      USBDM_WriteDReg(CFVx_DRegPBR2,0);
   }
   if (hardwareBreakpoints[3].inUse) {
      tdrValue |= TDR_TRC_HALT|TDR_L1T|TDR_L1EBL|TDR_L1EPC;
      USBDM_WriteDReg(CFVx_DRegPBR3, hardwareBreakpoints[3].address|0x1);
      breakpointsActive = true;
      print("activateBreakpoints(%s@%08X)\n", getBreakpointName(hardBreak),
                                              hardwareBreakpoints[3].address&~0x1);
   }
   else {
      USBDM_WriteDReg(CFV1_DRegPBR3,0);
   }
   if (dataWatchPoints[0].inUse) {
      tdrValue |= TDR_TRC_HALT|TDR_L1T|TDR_L1EBL|TDR_L1EA_INC;
      USBDM_WriteDReg(CFVx_DRegABLR, dataWatchPoints[0].address);
      USBDM_WriteDReg(CFVx_DRegABHR, dataWatchPoints[0].address+dataWatchPoints[0].size-1);
      breakpointsActive = true;
   }
   USBDM_WriteDReg(CFVx_DRegTDR, tdrValue);
}
//! De-activate breakpoints. \n
//! This may involve changing target code for RAM breakpoints or
//! modifying target breakpoint hardware.
//!
void deactivateBreakpoints(void) {
   print("deactivateBreakpoints()\n");
   memoryBreakInfo *bpPtr;
   if (!breakpointsActive)
      return;
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      if (bpPtr->inUse) {
         print("deactivateBreakpoints(MEM@%08X)\n", bpPtr->address);
         USBDM_WriteMemory(2,2,bpPtr->address,bpPtr->opcode);
      }
   }
   USBDM_WriteDReg(CFV1_DRegTDR, TDR_DISABLE);
   breakpointsActive = false;
}
//! RAM based breakpoints leave the PC pointing at the instruction following
//  the HALT instruction.  This routine checks for this situation and adjusts
//! the target PC.
//!
void checkAndAdjustBreakpointHalt(void) {
   // Processor halted
   unsigned long pcAddress = 0;
   USBDM_ReadPC(&pcAddress);
   pcAddress -= 2;
   if (breakpointsActive && (findMemoryBreakpoint(pcAddress) != NULL)) {
      print("checkAndAdjustBreakpointHalt() - adjusting PC=%08lX\n", pcAddress);
      USBDM_WritePC(pcAddress);
   }
}

#elif TARGET == ARM

static const uint8_t haltOpcode[] = {0x00, 0xBE};

const uint8_t *getFpCompAddress(uint32_t address) {
   uint32_t fpCompValue = address&FP_COMP_ADDR_MASK;
   fpCompValue |= (address&0x02)?FP_COMP_BKPT_UPPER:FP_COMP_BKPT_LOWER;
   fpCompValue |= FP_COMP_ENABLE;
   return getData4x8Le(fpCompValue);
}
//! Activate breakpoints. \n
//! This may involve changing target code for RAM breakpoints or
//! modifying target breakpoint hardware
//!
void activateBreakpoints(void) {
   print("activateBreakpoints()\n");
   memoryBreakInfo *bpPtr;
   if (breakpointsActive)
      return;
   // Memory breakpoints
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      if (bpPtr->inUse) {
         print("activateBreakpoints(%s@%08X)\n", getBreakpointName(memoryBreak), bpPtr->address);
         USBDM_ReadMemory(sizeof(haltOpcode),sizeof(haltOpcode),bpPtr->address,bpPtr->opcode);
         USBDM_WriteMemory(sizeof(haltOpcode),sizeof(haltOpcode),bpPtr->address,haltOpcode);
         breakpointsActive = true;
      }
   }
   // Hardware breakpoints
   uint32_t fp_ctrl = FP_CTRL_DISABLE;
   for (int breakPtNum=0; breakPtNum<MAX_HARDWARE_BREAKPOINTS; breakPtNum++) {
      if (hardwareBreakpoints[breakPtNum].inUse) {
         print("activateBreakpoints(%s@%08X)\n", getBreakpointName(hardBreak),
                                                 hardwareBreakpoints[breakPtNum].address&~0x1);
         fp_ctrl = FP_CTRL_ENABLE;
         ARM_WriteMemory(4, 4, FP_COMP0+4*breakPtNum, getFpCompAddress(hardwareBreakpoints[breakPtNum].address));
         breakpointsActive = true;
      }
      else {
         ARM_WriteMemory(4, 4, FP_COMP0+4*breakPtNum, getData4x8Le(FP_COMP_DISABLE));
      }
   }
   ARM_WriteMemory(4, 4, FP_CTRL, getData4x8Le(fp_ctrl));
   // Hardware watches
   for (int watchPtNum=0; watchPtNum<MAX_DATA_WATCHES; watchPtNum++) {
      if (dataWatchPoints[watchPtNum].inUse) {
         unsigned size       = dataWatchPoints[watchPtNum].size;
         unsigned bpSize     = 1;
         unsigned sizeValue  = 0;
         while ((bpSize<size) && (sizeValue<15)) {
            sizeValue++;
            bpSize <<= 1;
         }
         int mode = DWT_FUNCTION_READ_WATCH;
         switch (dataWatchPoints[watchPtNum].type) {
         case readWatch:   mode = DWT_FUNCTION_READ_WATCH;  break;
         case writeWatch:  mode = DWT_FUNCTION_WRITE_WATCH; break;
         case accessWatch: mode = DWT_FUNCTION_RW_WATCH;    break;
         default : break;
         }
         print("activateBreakpoints(%s@%08X)\n", getBreakpointName(dataWatchPoints[watchPtNum].type),
                                                 dataWatchPoints[watchPtNum].address);
         print("activateBreakpoints(%s@%08X, bpSize=%d, sizeValue=%d)\n",
               getBreakpointName(dataWatchPoints[watchPtNum].type),
               dataWatchPoints[watchPtNum].address&(~(bpSize-1)),
               bpSize, sizeValue );
         ARM_WriteMemory(4, 4, DWT_COMP0+16*watchPtNum,     getData4x8Le(dataWatchPoints[watchPtNum].address));
         ARM_WriteMemory(4, 4, DWT_MASK0+16*watchPtNum,     getData4x8Le(sizeValue));
         ARM_WriteMemory(4, 4, DWT_FUNCTION0+16*watchPtNum, getData4x8Le(mode));
         breakpointsActive = true;
      }
      else {
         ARM_WriteMemory(4, 4, DWT_FUNCTION0+16*watchPtNum, getData4x8Le(DWT_FUNCTION_NONE));
      }
   }
}
//! De-activate breakpoints. \n
//! This may involve changing target code for RAM breakpoints or
//! modifying target breakpoint hardware.
//!
void deactivateBreakpoints(void) {
   print("deactivateBreakpoints()\n");
   memoryBreakInfo *bpPtr;
   if (!breakpointsActive)
      return;
   // Memory breakpoints
   for (bpPtr = memoryBreakpoints;
        bpPtr < memoryBreakpoints+MAX_MEMORY_BREAKPOINTS;
        bpPtr++) {
      if (bpPtr->inUse) {
         print("deactivateBreakpoints(MEM@%08X)\n", bpPtr->address);
         USBDM_WriteMemory(sizeof(haltOpcode),sizeof(haltOpcode),bpPtr->address,bpPtr->opcode);
      }
   }
   // Hardware breakpoints
   ARM_WriteMemory(4, 4, FP_CTRL, getData4x8Le(FP_CTRL_DISABLE));
   breakpointsActive = false;
}

//! No adjustment needed for memory breakpoints
//!
void checkAndAdjustBreakpointHalt(void) {
}

#else
#error "Unhandled breakpoint case"
#endif

void clearAllBreakpoints(void) {
   clearAllMemoryBreakpoints();
   clearAllHardwareBreakpoints();
   clearAllDataWatchPoints();
}
