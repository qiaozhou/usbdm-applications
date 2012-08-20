/*
 * gdbHandler.cpp
 *
 *  Created on: 06/03/2011
 *      Author: podonoghue
 */
#include <stdio.h>

#include <sys/types.h>
#include <string>
#include "Common.h"
#include "Log.h"
#include "Utils.h"
#include "signals.h"
#include "Names.h"
#include "wxPlugin.h"

using namespace std;

#include "USBDM_API.h"
#if (TARGET == ARM) || (TARGET == ARM_SWD)
#include "USBDM_ARM_API.h"
#include "ARM_Definitions.h"
#endif

#include "TargetDefines.h"

#include "DeviceData.h"
#include "FlashImage.h"
#include "FlashProgramming.h"

#include "tclInterface.h"
#include "ProgressTimer.h"

#include "GdbInput.h"
#include "GdbOutput.h"
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
#elif TARGET == ARM_SWD
#define USBDM_ReadPC(x)                      USBDM_ReadReg(ARM_RegPC, x);
#define USBDM_WritePC(x)                     USBDM_WriteReg(ARM_RegPC, x);
#define USBDM_ReadSP(x)                      USBDM_ReadReg(ARM_RegSP, x);
#define USBDM_WriteSP(x)                     USBDM_WriteReg(ARM_RegSP, x);
#define USBDM_ReadSR(x)                      USBDM_ReadReg(ARM_RegSR, x);
#define USBDM_WriteSR(x)                     USBDM_WriteReg(ARM_RegSR, x);
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
#define USBDM_Connect()                      ARM_Connect();

#else
#error "Unhandled TARGET"
#endif

#if TARGET==CFV1
#define TARGET_TYPE T_CFV1
#elif TARGET==CFVx
#define TARGET_TYPE T_CFVx
#elif TARGET==ARM
#define TARGET_TYPE T_ARM_JTAG
#elif TARGET==ARM_SWD
#define TARGET_TYPE T_ARM_SWD
#else
#error "Unhandled case"
#endif

USBDM_ErrorCode lastError = BDM_RC_OK;

void setErrorCode(USBDM_ErrorCode rc) {
   if ((lastError == BDM_RC_OK) && (rc != BDM_RC_OK)) {
      lastError = rc;
   }
}

void reportError(USBDM_ErrorCode rc) {
   if ((rc & BDM_RC_ERROR_HANDLED) == 0) {
      displayDialogue(USBDM_GetErrorString(rc), "USBDM GDB Server error", wxOK);
   }
}

static ProgressTimer *progressTimer;

inline uint16_t swap16(uint16_t data) {
   return ((data<<8)&0xFF00) + ((data>>8)&0xFF);
}
inline uint32_t swap32(uint32_t data) {
   return ((data<<24)&0xFF000000) + ((data<<8)&0xFF0000) + ((data>>8)&0xFF00) + ((data>>24)&0xFF);
}
inline uint16_t unchanged16(uint16_t data) {
   return data;
}
inline uint32_t unchanged32(uint32_t data) {
   return data;
}

#if (TARGET == ARM) || (TARGET == ARM_SWD)
#define targetToNative16(x)    unchanged16(x)
#define targetToNative32(x)    unchanged32(x)
#define nativeToTarget16(x)    unchanged16(x)
#define nativeToTarget32(x)    unchanged32(x)
#define bigendianToTarget16(x) swap16(x)
#define bigendianToTarget32(x) swap32(x)

#else
#define targetToNative16(x)    swap16(x)
#define targetToNative32(x)    swap32(x)
#define nativeToTarget16(x)    swap16(x)
#define nativeToTarget32(x)    swap32(x)
#define bigendianToTarget16(x) unchanged16(x)
#define bigendianToTarget32(x) unchanged32(x)

#endif

bool ackMode = true;

// Note - the following assume bigendian
inline bool hexToInt(char ch, int *value) {
   if ((ch >= '0') && (ch <= '9')) {
      *value = (ch - '0');
      return true;
   }
   if ((ch >= 'a') && (ch <= 'f')) {
      *value = (ch - 'a' + 10);
      return true;
   }
   if ((ch >= 'A') && (ch <= 'F')) {
      *value = (ch - 'A' + 10);
      return true;
   }
   return false;
}

inline bool hexToInt32(const char *ch, unsigned long *value) {
   *value = 0;
   for (int i=0; i<8; i++) {
      int temp;
      if (!hexToInt(*ch++, &temp))
         return false;
      *value *= 16;
      *value += temp;
   }
   return true;
}

inline long hexToInt16(const char *ch, unsigned long *value) {
   *value = 0;
   for (int i=0; i<4; i++) {
      int temp;
      if (!hexToInt(*ch++, &temp))
         return false;
      *value *= 16;
      *value += temp;
   }
   return true;
}

inline long hexToInt8(const char *ch, unsigned long *value) {
   *value = 0;
   for (int i=0; i<2; i++) {
      int temp;
      if (!hexToInt(*ch++, &temp))
         return false;
      *value *= 16;
      *value += temp;
   }
   return true;
}

enum RunState {halted, stepping, running, breaking};

static GdbOutput       *gdbOutput      = NULL;
static GdbInput        *gdbInput       = NULL;
static const GdbPacket *packet         = NULL;
static RunState         runState       = halted;
static unsigned         pollInterval   = 100;
static DeviceData       deviceData;

//! Description of currently selected device
//!
static DeviceData deviceOptions;

//! Flash image for programming
//!
static FlashImage *flashImage = NULL;

//!
//! Create XML description of current device memory map in GDB expected format
//!
//! @param buffer     - location to return address of static buffer initialised.
//! @param bufferSize - size of buffer data
//!
static void createMemoryMapXML(const char **buffer, unsigned *bufferSize) {
   // Prefix for memory map XML
   static const char xmlPrefix[] =
      "<?xml version=\"1.0\"?>\n"
      "<!DOCTYPE memory-map\n"
      "   PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\"\n"
      "   \"http://sourceware.org/gdb/gdb-memory-map.dtd\">\n"
      "<memory-map version=\"1.0.0\" >\n";
   // Suffix for memory map XML
   static const char xmlSuffix[] =
      "</memory-map>\n";

   static char xmlBuff[2000] = {0};
   char *xmlPtr;

   xmlPtr = xmlBuff;
   *bufferSize = 0;
   strcpy(xmlPtr, xmlPrefix);
   xmlPtr += sizeof(xmlPrefix)-1; // Discard trailing '\0'

   for (int memIndex=0; true; memIndex++) {
      MemoryRegionPtr pMemoryregion(deviceData.getMemoryRegion(memIndex));
      if (!pMemoryregion) {
//       print("FlashProgrammer::setDeviceData() finished\n");
         break;
      }
      print("createDeviceXML() memory area #%d", memIndex);
      for (unsigned memRange=0; memRange<pMemoryregion->memoryRanges.size(); memRange++) {
         uint32_t start, size;
         const MemoryRegion::MemoryRange *memoryRange = pMemoryregion->getMemoryRange(memRange);
         if (memoryRange == NULL)
            break;
         switch (pMemoryregion->getMemoryType()) {
         case MemXRAM:
         case MemXROM:
         case MemPROM:
         case MemPRAM:
            print(" - XRAM/XROM - Ignored\n");
            break;
         case MemFlexNVM:
         case MemFlexRAM:
            print(" - FlexNVM/FlexNVM - Ignored\n");
            break;
         case MemEEPROM:
            print(" - EEPROM - Ignored\n");
            break;
         case MemDFlash:
         case MemPFlash:
         case MemFLASH:
            start = memoryRange->start;
            size  = memoryRange->end - start + 1;
            print(" - FLASH[0x%08X..0x%08X]\n", memoryRange->start, memoryRange->end);
            xmlPtr += sprintf(xmlPtr,
                           "   <memory type=\"flash\" start=\"0x%X\" length=\"0x%X\" > \n"
                           "      <property name=\"blocksize\">0x400</property> \n"
                           "   </memory>\n",
                           start, size);
            break;
         case MemIO:  // Treat I/O as RAM
         case MemRAM:
            start = memoryRange->start;
            size  = memoryRange->end - start + 1;
            print(" - RAM[0x%08X..0x%08X]\n", memoryRange->start, memoryRange->end);
            xmlPtr += sprintf(xmlPtr,
                           "<memory type=\"ram\" start=\"0x%X\" length=\"0x%X\" /> \n",
                           start, size);
            break;
         case MemROM:
            start = memoryRange->start;
            size  = memoryRange->end - start + 1;
            print(" - ROM[0x%08X..0x%08X]\n", memoryRange->start, memoryRange->end);
            xmlPtr += sprintf(xmlPtr,
                           "<memory type=\"rom\" start=\"0x%X\" length=\"0x%X\" /> \n",
                           start, size);
            break;
         case MemInvalid:
            print(" - Invalid\n");
            break;
         }
      }
   }
   strcpy(xmlPtr, xmlSuffix);
   xmlPtr += sizeof(xmlSuffix)-1; // Discard trailing '0'

//   print("XML = \n\"%s\"\n", xmlBuff);
   *buffer = xmlBuff;
   *bufferSize = xmlPtr-xmlBuff;
}

static const char targetXML[] =
      "<?xml version=\"1.0\"?>\n"
      "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n"
      "<target version=\"1.0\">\n"
#if (TARGET == ARM) ||(TARGET == ARM_SWD) || (TARGET == CFV1) || (TARGET == CFVx)
      "   <xi:include href=\"targetRegs.xml\"/>\n"
#endif
      "</target>\n"
      ;

#if (TARGET == CFV1) || (TARGET == CFVx)
static const char targetRegsXML[] =
      "<?xml version=\"1.0\"?>\n"
      "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
      "<feature name=\"org.gnu.gdb.coldfire.core\">\n"
      "   <reg name=\"d0\" bitsize=\"32\"/>\n"
      "   <reg name=\"d1\" bitsize=\"32\"/>\n"
      "   <reg name=\"d2\" bitsize=\"32\"/>\n"
      "   <reg name=\"d3\" bitsize=\"32\"/>\n"
      "   <reg name=\"d4\" bitsize=\"32\"/>\n"
      "   <reg name=\"d5\" bitsize=\"32\"/>\n"
      "   <reg name=\"d6\" bitsize=\"32\"/>\n"
      "   <reg name=\"d7\" bitsize=\"32\"/>\n"
      "   <reg name=\"a0\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"a1\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"a2\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"a3\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"a4\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"a5\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"fp\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   \n"
      "   <flags id=\"ps.type\" size=\"4\">\n"
      "      <field name=\"C\"  start=\"0\"  end=\"0\"/>\n"
      "      <field name=\"V\"  start=\"1\"  end=\"1\"/>\n"
      "      <field name=\"Z\"  start=\"2\"  end=\"2\"/>\n"
      "      <field name=\"N\"  start=\"3\"  end=\"3\"/>\n"
      "      <field name=\"X\"  start=\"4\"  end=\"4\"/>\n"
      "      <field name=\"I0\" start=\"8\"  end=\"8\"/>\n"
      "      <field name=\"I1\" start=\"9\"  end=\"9\"/>\n"
      "      <field name=\"I2\" start=\"10\" end=\"10\"/>\n"
      "      <field name=\"M\"  start=\"12\" end=\"12\"/>\n"
      "      <field name=\"S\"  start=\"13\" end=\"13\"/>\n"
      "      <field name=\"T0\" start=\"14\" end=\"14\"/>\n"
      "      <field name=\"T1\" start=\"15\" end=\"15\"/>\n"
      "   </flags>\n"
      "   \n"
      "   <reg name=\"ps\" bitsize=\"32\" type=\"ps.type\"/>\n"
      "   <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\n"
      "   \n"
      "</feature>\n";
#else
static const char targetRegsXML[] =
   "<?xml version=\"1.0\"?>\n"
   "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
   "<feature name=\"org.gnu.gdb.arm.core\" >\n"
   "   <reg name=\"r0\"   bitsize=\"32\" />\n"
   "   <reg name=\"r1\"   bitsize=\"32\" />\n"
   "   <reg name=\"r2\"   bitsize=\"32\" />\n"
   "   <reg name=\"r3\"   bitsize=\"32\" />\n"
   "   <reg name=\"r4\"   bitsize=\"32\" />\n"
   "   <reg name=\"r5\"   bitsize=\"32\" />\n"
   "   <reg name=\"r6\"   bitsize=\"32\" />\n"
   "   <reg name=\"r7\"   bitsize=\"32\" />\n"
   "   <reg name=\"r8\"   bitsize=\"32\" />\n"
   "   <reg name=\"r9\"   bitsize=\"32\" />\n"
   "   <reg name=\"r10\"  bitsize=\"32\" />\n"
   "   <reg name=\"r11\"  bitsize=\"32\" />\n"
   "   <reg name=\"r12\"  bitsize=\"32\" />\n"
   "   <reg name=\"sp\"   bitsize=\"32\" type=\"data_ptr\"/>\n"
   "   <reg name=\"lr\"   bitsize=\"32\" type=\"code_ptr\"/>\n"
   "   <reg name=\"pc\"   bitsize=\"32\" type=\"code_ptr\"/>\n"
#if 0
   "   <reg name=\"cpsr\" bitsize=\"32\" type=\"uint32\"/>\n"
#elif 1
   "   <flags id=\"cpsr.type\" size=\"4\">\n"
   "      <field name=\"N\"      start=\"31\"  end=\"31\" />\n"
   "      <field name=\"Z\"      start=\"30\"  end=\"30\" />\n"
   "      <field name=\"C\"      start=\"29\"  end=\"29\" />\n"
   "      <field name=\"V\"      start=\"28\"  end=\"28\" />\n"
   "      <field name=\"Q\"      start=\"27\"  end=\"27\" />\n"
   "      <field name=\"IT1\"    start=\"26\"  end=\"26\" />\n"
   "      <field name=\"IT0\"    start=\"25\"  end=\"25\" />\n"
   "      <field name=\"T\"      start=\"24\"  end=\"24\" />\n"
   "      <field name=\"IT7\"    start=\"15\"  end=\"15\" />\n"
   "      <field name=\"IT6\"    start=\"14\"  end=\"14\" />\n"
   "      <field name=\"IT5\"    start=\"13\"  end=\"13\" />\n"
   "      <field name=\"IT4\"    start=\"12\"  end=\"12\" />\n"
   "      <field name=\"IT2\"    start=\"11\"  end=\"11\" />\n"
   "      <field name=\"IT3\"    start=\"10\"  end=\"10\" />\n"
   "      <field name=\"E8\"     start=\"8\"   end=\"8\"  />\n"
   "      <field name=\"E7\"     start=\"7\"   end=\"7\"  />\n"
   "      <field name=\"E6\"     start=\"6\"   end=\"6\"  />\n"
   "      <field name=\"E5\"     start=\"5\"   end=\"5\"  />\n"
   "      <field name=\"E4\"     start=\"4\"   end=\"4\"  />\n"
   "      <field name=\"E3\"     start=\"3\"   end=\"3\"  />\n"
   "      <field name=\"E2\"     start=\"2\"   end=\"2\"  />\n"
   "      <field name=\"E1\"     start=\"1\"   end=\"1\"  />\n"
   "      <field name=\"E0\"     start=\"0\"   end=\"0\"  />\n"
   "   </flags>\n"
   "   <reg name=\"cpsr\"      bitsize=\"32\" type=\"cpsr.type\"/>\n"
#else
   "   <flags id=\"flags.type\" size=\"1\">\n"
   "      <field name=\"N\"      start=\"7\"  end=\"7\" />\n"
   "      <field name=\"Z\"      start=\"6\"  end=\"6\" />\n"
   "      <field name=\"C\"      start=\"5\"  end=\"5\" />\n"
   "      <field name=\"V\"      start=\"4\"  end=\"4\" />\n"
   "      <field name=\"Q\"      start=\"3\"  end=\"3\" />\n"
   "      <field name=\"IT1\"    start=\"2\"  end=\"2\" />\n"
   "      <field name=\"IT0\"    start=\"1\"  end=\"1\" />\n"
   "      <field name=\"T\"      start=\"0\"  end=\"0\" />\n"
   "   </flags>\n"
   "   <struct id=\"cpsr.type\">\n"
   "      <field name=\"Except\"   type = \"uint16\"  bitSize=\"10\"   />\n"
   "      <field name=\"\"         type = \"uint8\"   bitSize=\"1\"    />\n"
   "      <field name=\"ICI/IT\"   type = \"uint8\"   bitSize=\"6\"    />\n"
   "      <field name=\"\"         type = \"uint8\"   bitSize=\"8\"    />\n"
   "      <field name=\"Flags\"    type = \"flags.type\" />\n"
   "   </struct>\n"
   "   <reg name=\"cpsr\" bitsize=\"32\" type=\"cpsr.type\"/>\n"
#endif
      "   <reg name=\"msp\"       bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <reg name=\"psp\"       bitsize=\"32\" type=\"data_ptr\"/>\n"
      "   <struct id=\"misc.type\">\n"
      "      <field name=\"primask\"   type = \"uint8\"    />\n"
      "      <field name=\"basepri\"   type = \"uint8\"    />\n"
      "      <field name=\"faultmask\" type = \"uint8\"    />\n"
      "      <field name=\"control\"   type = \"uint8\"    />\n"
      "   </struct>\n"
      "   <reg name=\"misc\" bitsize=\"32\" type=\"misc.type\"/>\n"
   "</feature>\n"
   ;
#endif

//!
//!
//! Convert a binary value to a HEX char
//!
//! @param num - number o convert (0x0 - 0xF)
//! @return converted char ('0'-'9' or 'A' to 'F') - no error checks!
//!
//static char hexChar(int num) {
//const char chars[] = "0123456789ABCDEF";
//   return chars[num&0x0F];
//}

typedef struct {
   unsigned long value;
   int           size;
} Register_t;

#if (TARGET == ARM) || (TARGET == ARM_SWD)
// Maps GDB register numbers to USBDM register numbers (or -1 is invalid)
int registerMap[] = {
      ARM_RegR0,   ARM_RegR1,   ARM_RegR2,  ARM_RegR3,
      ARM_RegR4,   ARM_RegR5,   ARM_RegR6,  ARM_RegR7,
      ARM_RegR8,   ARM_RegR9,   ARM_RegR10, ARM_RegR11,
      ARM_RegR12,  ARM_RegSP,   ARM_RegLR,  ARM_RegPC, // r0-r12,sp,lr,pc
      ARM_RegxPSR, ARM_RegMSP,  ARM_RegPSP,            // psr, main sp, process sp
      ARM_RegMISC,   // [31:24]=CONTROL,[23:16]=FAULTMASK,[15:8]=BASEPRI,[7:0]=PRIMASK.
};
#elif TARGET == CFV1
unsigned registerMap[] = {
      CFV1_RegD0,CFV1_RegD1,CFV1_RegD2,CFV1_RegD3,
      CFV1_RegD4,CFV1_RegD5,CFV1_RegD6,CFV1_RegD7,
      CFV1_RegA0,CFV1_RegA1,CFV1_RegA2,CFV1_RegA3,
      CFV1_RegA4,CFV1_RegA5,CFV1_RegA6,CFV1_RegA7,
      0x100+CFV1_CRegSR, 0x100+CFV1_CRegPC,         // +0x100 indicates USBDM_ReadCReg
};
#elif TARGET == CFVx
unsigned registerMap[] = {
      CFVx_RegD0,CFVx_RegD1,CFVx_RegD2,CFVx_RegD3,
      CFVx_RegD4,CFVx_RegD5,CFVx_RegD6,CFVx_RegD7,
      CFVx_RegA0,CFVx_RegA1,CFVx_RegA2,CFVx_RegA3,
      CFVx_RegA4,CFVx_RegA5,CFVx_RegA6,CFVx_RegA7,
      0x100+CFVx_CRegSR, 0x100+CFVx_CRegPC,        // +0x100 indicates USBDM_ReadCReg
};
#endif
#define NUMREGISTERS (sizeof(registerMap)/sizeof(registerMap[0]))

bool isValidRegister(unsigned regNo) {
   if (regNo >= sizeof(registerMap)/sizeof(registerMap[0]))
      return false;
   else
      return registerMap[regNo]>=0;
}

//! Read register into string buffer as hex chars
//!
//! @param regNo - number of register to read (GDB numbering)
//! @param cPtr  - ptr to buffer
//!
//! @return number of chars written
//!
//! @note characters are written in target byte order
//!
static int readReg(unsigned regNo, char *cPtr) {
   unsigned long regValue;

   if (!isValidRegister(regNo)) {
      print("reg[%d] => Invalid\n", regNo);
      return sprintf(cPtr, "%08lX", 0xFF000000UL+regNo);
//      return sprintf(cPtr, "12345678");
   }
   int usbdmRegNo = registerMap[regNo];

#if (TARGET == ARM) || (TARGET == ARM_SWD)
   USBDM_ReadReg((ARM_Registers_t)usbdmRegNo, &regValue);
   print("%s(0x%02X) => %08lX\n", getARMRegName(usbdmRegNo), usbdmRegNo, regValue);
   regValue = bigendianToTarget32(regValue);
#elif (TARGET == CFV1)
   if (usbdmRegNo < 0x100) {
      USBDM_ReadReg((CFV1_Registers_t)usbdmRegNo, &regValue);
      print("%s => %08lX\n", getCFV1RegName(regNo), regValue);
   }
   else {
      USBDM_ReadCReg((CFV1_Registers_t)(usbdmRegNo-0x100), &regValue);
      print("%s => %08lX\n", getCFV1ControlRegName(regNo), regValue);
   }
#elif(TARGET == CFVx)
   if (usbdmRegNo < 0x100) {
      USBDM_ReadReg((CFV1_Registers_t)usbdmRegNo, &regValue);
      print("%s => %08lX\n", getCFVxRegName(regNo), regValue);
   }
   else {
      USBDM_ReadCReg((CFV1_Registers_t)(usbdmRegNo-0x100), &regValue);
      print("%s => %08lX\n", getCFVxControlRegName(regNo), regValue);
   }
#endif
   return sprintf(cPtr, "%0*lX", 8, regValue);
}

//! Read all registers from target
//!
//! @note values are returned in target byte order
//!
static void readRegs(void) {
   unsigned regNo;
   char buff[1000];
   char *cPtr = buff;
   for (regNo = 0; regNo<NUMREGISTERS; regNo++) {
      cPtr += readReg(regNo, cPtr);
   }
//   for (regNo = NUMREGISTERS; regNo<(NUMREGISTERS+10); regNo++) {
//      cPtr += readReg(regNo, cPtr);
//   }
   *cPtr = '\0';
   gdbOutput->sendGdbString(buff);
}

//! Write to target register
//!
//! @param regNo     - number of register to read (GDB numbering)
//! @param regValue  - value to write
//!
static void writeReg(unsigned regNo, unsigned long regValue) {
   if (!isValidRegister(regNo))
      return;
   int usbdmRegNo = registerMap[regNo];

#if (TARGET == ARM) || (TARGET == ARM_SWD)
   USBDM_WriteReg((ARM_Registers_t)usbdmRegNo, regValue);
   print("%s(0x%02X) <= %08lX\n", getARMRegName(usbdmRegNo), usbdmRegNo, regValue);
   regValue = bigendianToTarget32(regValue);
#elif (TARGET == CFV1) || (TARGET == CFVx)
   if (usbdmRegNo < 0x100) {
      USBDM_WriteReg((CFV1_Registers_t)usbdmRegNo, regValue);
      print("reg[%d] <= %08lX\n", regNo, regValue);
   }
   else {
      USBDM_WriteCReg((CFV1_Registers_t)(usbdmRegNo-0x100), regValue);
      print("reg[%d] <= %08lX\n", regNo, regValue);
   }
#endif
}

//! Write target registers from string buffer containing hex chars
//!
//! @param ccPtr  - ptr to buffer
//!
//! @note characters are written in target byte order
//!
static void writeRegs(const char *ccPtr) {
   unsigned long regValue = 0;
   unsigned regNo;

   for (regNo = 0; regNo<NUMREGISTERS; regNo++) {
      if (!hexToInt32(ccPtr, &regValue))
         break;
      ccPtr += 8;
      regValue = bigendianToTarget32(regValue);
      writeReg(regNo, regValue);
   }
   gdbOutput->sendGdbString("OK");
}

static void readMemory(uint32_t address, uint32_t numBytes) {
   unsigned char buff[1000] = {0};

//   print("readMemory(addr=%X, size=%X)\n", address, numBytes);
   if (USBDM_ReadMemory(1, numBytes, address, buff) != BDM_RC_OK) {
      // Ignore errors
      memset(buff, 0xAA, numBytes);
//      gdbOutput->sendGdbString("E11");
//      return;
   }
   gdbOutput->putGdbPreamble();
   gdbOutput->putGdbHex(numBytes, buff);
   gdbOutput->sendGdbBuffer();
}

//! Convert a hex string to a series of byte values
//!
//! @param numBytes - number of bytes to convert
//! @param dataIn   - ptr to string of Hex chars (2 * numBytes)
//! @param dataOut  - ptr to output buffer (numBytes)
//!
//! @return true => ok conversion\n
//!         false => failed
//!
static bool convertFromHex(unsigned numBytes, const char *dataIn, unsigned char *dataOut) {
//   print("convertFromHex()\n");
   for (unsigned index=0; index<numBytes; index++) {
      unsigned long value;
      if (!hexToInt8(dataIn, &value)) {
         return false;
      }
//      print("convertFromHex() %2.2s => %2.2X\n", dataIn, value);
      dataIn += 2;
      *dataOut++ = value;
   }
   return true;
}

static void writeMemory(const char *ccPtr, uint32_t address, uint32_t numBytes) {
   unsigned char buff[1000];

   print("writeMemory(addr=%X, size=%X)\n", address, numBytes);
   convertFromHex(numBytes, ccPtr, buff);
   USBDM_WriteMemory(1, numBytes, address, buff);
   gdbOutput->sendGdbString("OK");
}

#if (TARGET == ARM) || (TARGET == ARM_SWD)
#define T_RUNNING (1<<0) // Executing
#define T_HALT    (1<<1) // Debug halt
#define T_SLEEP   (1<<2) // Low power sleep

//! @param status - status
//!
//! @return \n
//!     DI_OK              => OK \n
//!     DI_ERR_FATAL       => Error see \ref currentErrorString
//!
static USBDM_ErrorCode getTargetStatus (int *status) {
   USBDM_ErrorCode BDMrc;
   ArmStatus armStatus;
   static int lastStatus;
   static int failureCount = 0;

   BDMrc = ARM_GetStatus(&armStatus);
   if (BDMrc != BDM_RC_OK) {
      print("getTargetStatus() - Failed\n");
   }
   else {
      print("getTargetStatus() - ARM_GetStatus() OK\n");
   }
   if (BDMrc != BDM_RC_OK) {
      print("getTargetStatus() - Doing autoReconnect\n");
      ARM_Initialise();
      if (ARM_Connect() != BDM_RC_OK) {
         print("ARM_Connect()=> re-connect failed\n");
      }
      else {
         print("ARM_Connect()=> re-connect OK\n");
         // retry after connect
         BDMrc = ARM_GetStatus(&armStatus);
      }
   }
   if (BDMrc != BDM_RC_OK) {
      print("getTargetStatus() - Failed, rc=%s\n",
            USBDM_GetErrorString(BDMrc), failureCount);
      // Give up after 10 tries!
      print("getTargetStatus() - Returning FATAL error\n");
      return BDMrc;
   }
   // Reset on OK status
   failureCount = 0;
   if ((armStatus.dhcsr & DHCSR_S_SLEEP) != 0) {
      // Stopped - low power sleep, treated as special
      *status = T_SLEEP;
      if (lastStatus != *status) {
         print("getTargetStatus() status change => T_SLEEP)\n");
      }
   }
   else if ((armStatus.dhcsr & (DHCSR_S_HALT|DHCSR_S_LOCKUP)) != 0) {
      // Processor in debug halt
      *status = T_HALT;
      if (lastStatus != *status) {
         print("getTargetStatus() status change => T_HALT)\n");
      }
   }
   else {
      // Processor executing
      // Processor in debug halt
      *status = T_RUNNING;
      if (lastStatus != *status) {
         print("getTargetStatus() status change => T_RUNNING)\n");
      }
   }
   lastStatus = *status;
   return BDM_RC_OK;
}
#endif

//! Checks if the target is running or runState
//!
//! @return true => target runState
//!
static bool isTargetHalted(void) {
   USBDM_ErrorCode rc;
//   static bool errorReported = false;

   // Check connection
//   rc = USBDM_Connect();
//   if (rc != BDM_RC_OK) {
//      if (!errorReported) {
//         reportError(rc);
//      }
//      errorReported = true;
//   }
//   else {
//      errorReported = false;
//   }
#if TARGET == CFV1
   unsigned long value;
//   print("isTargetHalted()\n");
   rc = USBDM_ReadStatusReg(&value);
   if (rc != BDM_RC_OK) {
      print("isTargetHalted()=> Status read failed\n");
      return true;
   }
   if ((value & CFV1_XCSR_ENBDM) == 0) {
      print("isTargetHalted()=> ENBDM=0\n");
      return true;
   }
   return (value&CFV1_XCSR_RUNSTATE) != 0;
#elif TARGET == CFVx
   // Crude - Assume if register read succeeds then target is runState
   USBDM_Connect();
   unsigned long value;
   rc = USBDM_ReadReg(CFVx_RegD0, &value);
   return (rc == BDM_RC_OK);
#elif (TARGET == ARM) || (TARGET == ARM_SWD)
   int status;
   rc = getTargetStatus(&status);
   if (rc != BDM_RC_OK) {
      print("isTargetHalted()=> Status read failed\n");
      return true;
   }
   return (status&T_RUNNING) == 0;
#else
#error "Unhandled case in isTargetHalted()"
#endif
}

static void sendXML(unsigned size, unsigned offset, const char *buffer, unsigned bufferSize) {
   gdbOutput->putGdbPreamble();
   if (offset >= bufferSize) {
      gdbOutput->putGdbString("l");
   }
   else {
      if (size > (bufferSize-offset)) {
         // Remainder fits in this pkt
         size = bufferSize-offset;
         gdbOutput->putGdbChar('l');
      }
      else {
         // More pkts to follow
         gdbOutput->putGdbChar('m');
      }
      gdbOutput->putGdbString(buffer+offset, size);
   }
   gdbOutput->sendGdbBuffer();
}

static int doQCommands(const GdbPacket *pkt) {
int offset, size;
const char *cmd = pkt->buffer;

   if (strncmp(cmd, "qSupported", sizeof("qSupported")-1) == 0) {
      char buff[200];
      sprintf(buff,"QStartNoAckMode+;qXfer:memory-map:read+;PacketSize=%X;qXfer:features:read+",GdbPacket::MAX_MESSAGE-10);
//      sprintf(buff,"QStartNoAckMode+;qXfer:memory-map:read+;PacketSize=%X",GdbPacket::MAX_MESSAGE-10);
      gdbOutput->sendGdbString(buff);
   }
   else if (strncmp(cmd, "qC", sizeof("qC")-1) == 0) {
      gdbOutput->sendGdbString(""); //("QC-1");
   }
   else if (strncmp(cmd, "qfThreadInfo", sizeof("qfThreadInfo")-1) == 0) {
      gdbOutput->sendGdbString("m-1"); //("m0");
   }
   else if (strncmp(cmd, "qsThreadInfo", sizeof("qsThreadInfo")-1) == 0) {
      gdbOutput->sendGdbString("l");
   }
   else if (strncmp(cmd, "qThreadExtraInfo", sizeof("qThreadExtraInfo")-1) == 0) {
      gdbOutput->sendGdbString("Runnable");
   }
   else if (strncmp(cmd, "qAttached", sizeof("qAttached")-1) == 0) {
      gdbOutput->sendGdbString("0");
   }
   else if (strncmp(cmd, "QStartNoAckMode", sizeof("QStartNoAckMode")-1) == 0) {
      gdbOutput->sendGdbString("OK");
      ackMode = false;
   }
   else if (strncmp(cmd, "qTStatus", sizeof("qTStatus")-1) == 0) {
      gdbOutput->sendGdbString("");
   }
   else if (strncmp(cmd, "qOffsets", sizeof("qOffsets")-1) == 0) {
//      gdbOutput->sendGdbString("TextSeg=08000000;DataSeg=20000000");
#if (TARGET == CFV1) || (TARGET == CFVx)
      gdbOutput->sendGdbString("Text=0;Data=0;Bss=0");
#elif (TARGET == ARM)
      gdbOutput->sendGdbString("TextSeg=0000000;DataSeg=00000000");
#endif
   }
   else if (strncmp(cmd, "qXfer:memory-map:read::", sizeof("qXfer:memory-map:read::")-1) == 0) {
      if (sscanf(cmd,"qXfer:memory-map:read::%X,%X",&offset, &size) != 2) {
         print("Ill formed:\'%s\'", cmd);
         gdbOutput->sendGdbString("");
      }
      else {
         print("memory-map:read::%X:%X\n", offset, size);
         unsigned xmlBufferSize;
         const char *xmlBuffer;
         createMemoryMapXML(&xmlBuffer, &xmlBufferSize);
         sendXML(size, offset, xmlBuffer, xmlBufferSize);
      }
   }
   else if (strncmp(cmd, "qXfer:features:read:target.xml:", sizeof("qXfer:features:read:target.xml:")-1) == 0) {
      if (sscanf(cmd,"qXfer:features:read:target.xml:%X,%X",&offset, &size) != 2) {
         print("Ill formed:\'%s\'", cmd);
         gdbOutput->sendGdbString("");
      }
      else {
         print("qXfer:features:read:target.xml:%X:%X\n", offset, size);
         sendXML(size, offset, targetXML, sizeof(targetXML));
      }
   }
   else if (strncmp(cmd, "qXfer:features:read:targetRegs.xml:", sizeof("qXfer:features:read:targetRegs.xml:")-1) == 0) {
      if (sscanf(cmd,"qXfer:features:read:targetRegs.xml:%X,%X",&offset, &size) != 2) {
         print("Ill formed:\'%s\'", cmd);
         gdbOutput->sendGdbString("");
      }
      else {
         print("qXfer:features:read:targetRegs.xml:%X:%X\n", offset, size);
         sendXML(size, offset, targetRegsXML, sizeof(targetRegsXML));
      }
   }
   else {
      gdbOutput->sendGdbString("");
   }
   return 0;
}

static int programImage(FlashImage *flashImage) {
   FlashProgrammer flashProgrammer;

   deviceData.setEraseOption(DeviceData::eraseAll);
   deviceData.setSecurity(SEC_SMART);
   deviceData.setClockTrimFreq(0);
   deviceData.setClockTrimNVAddress(0);
   if (flashProgrammer.setDeviceData(deviceData) != PROGRAMMING_RC_OK) {
      return -1;
   }
   USBDM_ErrorCode rc;
   rc = flashProgrammer.programFlash(flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      print("programImage() - failed, rc = %s\n", USBDM_GetErrorString(rc));
      return -1;
   }
   // Initialise the target after programming
   USBDM_TargetReset((TargetMode_t)(RESET_DEFAULT|RESET_SPECIAL));
   flashProgrammer.initTCL();
   flashProgrammer.runTCLCommand("initTarget");
   flashProgrammer.releaseTCL();
   print("programImage() - Complete\n");
   return 0;
}

static int doVCommands(const GdbPacket *pkt) {
   int address, length;
   const char *cmd = pkt->buffer;

   if (strncmp(cmd, "vFlashErase", 11) == 0) {
      // vFlashErase:addr,length
      if (sscanf(cmd, "vFlashErase:%x,%x", &address, &length) != 2) {
         gdbOutput->sendGdbString("E11");
      }
      else {
         print("doVCommands() - vFlashErase:0x%X:0x%X\n", address, length);
         gdbOutput->sendGdbString("OK");
      }
   }
   else if (strncmp(cmd, "vFlashWrite", 11) == 0) {
      // vFlashWrite:addr:XX...
      if (sscanf(cmd, "vFlashWrite:%x:", &address) != 1) {
         print("doVCommands() - vFlashWrite:error:\n");
         gdbOutput->sendGdbString("E11");
      }
      else {
         print("doVCommands() - vFlashWrite:0x%X:\n", address);
         if (flashImage == NULL) {
            flashImage = new FlashImage();
         }
         const char *vPtr = strchr(pkt->buffer,':');
         vPtr = strchr(++vPtr, ':');
         vPtr++;
         int size=pkt->size-(vPtr-pkt->buffer);
         bool newLine = true;
         while(size-->0) {
            flashImage->setValue(address, *vPtr);
            if (newLine)
               print("\n%8.8X:", address);
            print("%2.2X", (unsigned char)*vPtr);
            address++;
            vPtr++;
            newLine = (address & 0x0F) == 0;
         }
         print("\n");
         gdbOutput->sendGdbString("OK");
      }
   }
   else if (strncmp(cmd, "vFlashDone", 10) == 0) {
      // vFlashDone
      print("doVCommands() - vFlashDone\n");
      if (flashImage != NULL) {
         int rc = programImage(flashImage);
         delete flashImage;
         if (rc != PROGRAMMING_RC_OK) {
            print("doVCommands() - vFlashDone: Programming failed\n");
            gdbOutput->sendGdbString("E11");
            return 0;
         }
         else
            print("doVCommands() - vFlashDone: Programming complete\n");
      }
      else {
         print("doVCommands() - vFlashDone: Memory image empty, programming skipped\n");
      }
      gdbOutput->sendGdbString("OK");
   }
   else {
      gdbOutput->sendGdbString("");
   }
   return 0;
}

#define THREAD_STAT "'thread':-1.-1;"

#if (TARGET == CFV1) || (TARGET == CFVx)
#define PC_STAT "11:%08lX"
#elif TARGET == ARM
#define PC_STAT "0F:%08lX"
#endif
#undef PC_STAT
#define PC_STAT ""

static void reportLocation(char mode, int reason) {
   char buff[100];
   char *cPtr = buff;

   cPtr += sprintf(buff, "%c%2.2X", mode, reason);
#if (TARGET == CFV1)||(TARGET == CFVx)
   static const int regsToReport[] = {17, 15, 14, 16}; // PC, SP, FP, SR
#elif (TARGET == ARM) || (TARGET == ARM_SWD)
   static const int regsToReport[] = {15, 14, 13, 16}; // PC, LR, SP, PSR
#endif
   for (unsigned index=0; index<(sizeof(regsToReport)/sizeof(regsToReport[0])); index++) {
      cPtr += sprintf(cPtr, "%X:", regsToReport[index]);
      cPtr += readReg(regsToReport[index], cPtr);
      *cPtr++ = ';';
   }
   *cPtr++ = '\0';
   gdbOutput->sendGdbString(buff);
   print("reportLocation \n");
}

static int doGdbCommand(const GdbPacket *pkt) {
   unsigned address;
   unsigned numBytes;
   const char *ccptr;
   int type;
   unsigned kind;
   int regNo;
   int value;
   char buff[100] = {0};

//   print("doGdbCommand()\n");
   if (pkt->isBreak()) {
      print("Break......\n");
      USBDM_Connect();
      USBDM_TargetHalt();
      runState = breaking;
      return 0;
   }
   switch (pkt->buffer[0]) {
   case 'g' : // 'g' - Read general registers.
//   Reply:
//   -  'XX...' Each byte of register data is described by two hex digits. The bytes
//      with the register are transmitted in target byte order. The size of
//      each register and their position within the 'g' packet are determined
//      by the gdb internal gdbarch functions and gdbarch_register_name.
//   -  'E NN' for an error.
      print("Read Regs =>\n");
      readRegs();
      break;
   case 'G' : // 'G XX...' - Write general registers.
//      See [read registers packet] for a description of the XX... data.
//   Reply:
//     - 'OK' for success
//     - 'E NN' for an error
      print("Write Regs =>\n");
      writeRegs(pkt->buffer+1);
      break;
   case 'm' : // 'm addr,length' - Read memory
      if (sscanf(pkt->buffer, "m%X,%x:", &address, &numBytes) != 2) {
         gdbOutput->sendGdbString("E01");
      }
      else {
         print("readMemory [0x%08X..0x%08X]\n", address, address+numBytes-1);
         readMemory(address, numBytes);
      }
//      Read length bytes of memory starting at address addr. Note that addr may
//      not be aligned to any particular boundary.
//      The stub need not use any particular size or alignment when gathering data
//      from memory for the response; even if addr is word-aligned and length is a
//      multiple of the word size, the stub is free to use byte accesses, or not. For
//      this reason, this packet may not be suitable for accessing memory-mapped I/O
//      devices.
//      Reply:
//      'XX...' Memory contents; each byte is transmitted as a two-digit hexadecimal
//      number. The reply may contain fewer bytes than requested if
//      the server was able to read only part of the region of memory.
//      'E NN' NN is errno
      break;
   case 'M' : // 'M addr,length:XX...' - Write memory
      if ((sscanf(pkt->buffer, "M%X,%x:", &address, &numBytes) != 2) ||
          ((ccptr = strchr(pkt->buffer, ':')) == NULL)) {
         gdbOutput->sendGdbString("E01");
      }
      else {
         print("writeMemory [0x%08X...0x%08X] %2s...\n", address, address+numBytes-1, ccptr+1);
         writeMemory(ccptr+1, address, numBytes);
      }
//      Write length bytes of memory starting at address addr. XX. . . is the data;
//      each byte is transmitted as a two-digit hexadecimal number.
//      Reply:
//      'OK' for success
//      'E NN' for an error (this includes the case where only part of the data was
//      written).
      break;
   case 'c' : // 'c [addr]' - Continue
      if (sscanf(pkt->buffer, "c%X", &address) == 1) {
         // Set PC to address
         address = bigendianToTarget32(address);
         print("Continue @addr=%X\n", address);
         USBDM_WritePC(address);
      }
      else {
         print("Continue @PC\n");
      }
      if (atMemoryBreakpoint()) {
         // Do 1 step before installing memory breakpoints
         print("Continue - stepping one instruction...\n");
         USBDM_TargetStep();
      }
      activateBreakpoints();
      print("Continue - executing...\n");
      USBDM_TargetGo();
      runState = running;
      pollInterval = 1; // Poll fast
//      Continue. addr is address to resume. If addr is omitted, resume at current
//      address.
//      Reply: See [Stop Reply Packets] for the reply specifications.
      break;
   case 's' : // 's' [addr] - Single step.
      if (sscanf(pkt->buffer, "s%X", &address) > 1) {
         // Set PC to address
//         bigendianToTarget32(address);
         print("Single step @addr=%X\n", address);
         USBDM_WritePC(address);
      }
      else {
         print("Single step @PC\n");
      }
      runState = stepping;
      pollInterval = 1; // Poll fast
      USBDM_TargetStep();
//      addr is the address at which to resume. If addr is omitted, resume at same address.
//      Reply: See [Stop Reply Packets], page for the reply specifications.
      break;
   case 'Z' : // 'z type,addr,kind' - insert/remove breakpoint
      if (sscanf(pkt->buffer, "Z%d,%x,%d", &type, &address, &kind) != 3) {
         gdbOutput->sendGdbString("E11");
         break;
      }
      if (insertBreakpoint((breakType)type, address, kind)) {
         gdbOutput->sendGdbString("OK");
      }
      else {
         gdbOutput->sendGdbString("E11");
      }
      break;
   case 'z' : // 'z type,addr,kind' - insert/remove breakpoint
      if (sscanf(pkt->buffer, "z%d,%x,%d", &type, &address, &kind) != 3) {
         gdbOutput->sendGdbString("E11");
         break;
      }
      if (removeBreakpoint((breakType)type, address, kind)) {
         gdbOutput->sendGdbString("OK");
      }
      else {
         gdbOutput->sendGdbString("E11");
      }
      break;
   case 'P' :
      // 'P n...=r...' Write register n... with value r.... The register number n
      // is in hexadecimal,
      if (sscanf(pkt->buffer, "P%x=%x", &regNo, &value) != 2) {
         gdbOutput->sendGdbString("E11");
         break;
      }
//      print("GDB-P regNo=%x, val=%X\n", regNo, value);
      if (isValidRegister(regNo)) {
         value = bigendianToTarget32(value);
         writeReg(regNo, value);
         gdbOutput->sendGdbString("OK");
      }
      else {
         gdbOutput->sendGdbString("E11");
      }
      break;
   case 'p' : // 'p n...' Read register n...
      if (sscanf(pkt->buffer, "p%x", &regNo) != 1) {
         gdbOutput->sendGdbString("E11");
         break;
      }
      readReg(regNo, buff);
      gdbOutput->sendGdbString(buff);
//      if (isValidRegister(regNo)) {
//         readReg(regNo, buff);
//         gdbOutput->sendGdbString(buff);
//      }
//      else {
//         gdbOutput->sendGdbString("E11");
//      }
      break;
   case 'H' : // 'Hc num' Set thread
      gdbOutput->sendGdbString("OK");
      break;
   case '?' : // '?' Indicate the reason the target runState.
//      The reply is the same as for step and continue. This packet has a special interpretation
//      when the target is in non-stop mode;
      reportLocation('T', TARGET_SIGNAL_TRAP);
      break;
   case 'k' : // Kill
      print("Kill...\n");
      return -1;
   case 'D' : // Detach
      print("Detach...\n");
      gdbOutput->sendGdbString("OK");
      break;
   case 'q' : // q commands
   case 'Q' : // Q commands
      doQCommands(pkt);
      break;
   case 'v' : // v commands
      doVCommands(pkt);
      break;
   default : // Unrecognised command
      gdbOutput->sendGdbString("");
      break;
   }
   return 0;
}

static void gdbLoop(void) {
   unsigned pollCount = 0;
   print("gdbLoop()...\n");
//   gdbInput->flush();
   do {
      packet = gdbInput->getGdbPacket();
      if (packet != NULL) {
//         print("getGdbPacket()=:%03d:\'%*s\'\n", packet->size, packet->size, packet->buffer);
         print("After getGdbPacket, Time = %f\n", progressTimer->elapsedTime());
         if (ackMode) {
            gdbOutput->sendAck();
         }
         if (doGdbCommand(packet) < 0) {
            return;
         }
      }
      else {
         if (gdbInput->isEOF()) {
            return;
         }
      }
//      print("gdbLoop() - polling\n");
      milliSleep(1);
      if (pollCount++ >= pollInterval) {
         pollCount = 0;
         if (isTargetHalted()) {
//            print("Polling - runState\n");
            switch(runState) {
            case halted :  // ??? -> halted
               break;
            case breaking : // user break -> halted
               print("Target has halted (breaking)\n");
               deactivateBreakpoints();
               checkAndAdjustBreakpointHalt();
               reportLocation('T', TARGET_SIGNAL_INT);
               break;
            case stepping : // stepping -> halted
               print("Target has halted (stepping)\n");
               deactivateBreakpoints();
               checkAndAdjustBreakpointHalt();
               reportLocation('T', TARGET_SIGNAL_TRAP);
               break;
            default:       // ???     -> halted
            case running : // running -> halted
               print("Target has halted (running)\n");
               deactivateBreakpoints();
               checkAndAdjustBreakpointHalt();
               reportLocation('T', TARGET_SIGNAL_TRAP);
               break;
            }
            runState     = halted;
            pollInterval = 1000; // Slow poll when running
         }
         else {
//            print("Polling - running\n");
            if (runState == halted) {
               runState = running;
            }
            pollInterval = 10; // Poll fast
         }
      }
   } while (true);
   print("gdbLoop() - Exiting GDB Loop\n");
}

void handleGdb(GdbInput *gdbInput, GdbOutput *gdbOutput, DeviceData &deviceData, ProgressTimer *progressTimer) {
   print("handleGdb()\n");
   fprintf(stderr, "Initialising target...\n");
   TclInterface *tclInterface = new TclInterface(TARGET_TYPE, &deviceData);
   if (tclInterface == NULL) {
      fprintf(stderr, "Failed to create TCL interpreter)\n");
      return;
   }
   USBDM_ErrorCode rc = tclInterface->runTCLCommand("initTarget");
   if (rc != BDM_RC_OK) {
      fprintf(stderr, "Failed (rc = %d, %s)\n", rc, USBDM_GetErrorString(rc));
      return;
   }
   print("After runTCLCommand, Time = %f\n", progressTimer->elapsedTime());
   fprintf(stderr, "Done\n");
   ::gdbInput      = gdbInput;
   ::gdbOutput     = gdbOutput;
   ::deviceData    = deviceData;
   ::progressTimer = progressTimer;
   clearAllBreakpoints();
   gdbLoop();
   delete tclInterface;
}
