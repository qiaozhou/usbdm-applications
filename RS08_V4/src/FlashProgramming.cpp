/*! \file
   \brief Utility Routines for programming RS08 Flash

   FlashProgramming.cpp

   \verbatim
   Copyright (C) 2008  Peter O'Donoghue

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim

\verbatim
 Change History
+==============================================================================
| Revision History
+==============================================================================
|  1 Jun 12 | 4.9.5 Now handles arbitrary number of memory regions        - pgo
+-----------+------------------------------------------------------------------
| 30 May 12 | 4.9.5 Re-write of DSC programming                           - pgo
+-----------+------------------------------------------------------------------
| 12 Apr 12 | 4.9.4 Changed handling of empty images                      - pgo
+-----------+------------------------------------------------------------------
| 30 Mar 12 | 4.9.4 Added Intelligent security option                     - pgo
+-----------+------------------------------------------------------------------
| 25 Feb 12 | 4.9.1 Fixed alignment rounding problem on partial phrases   - pgo
+-----------+------------------------------------------------------------------
| 10 Feb 12 | 4.9.0 Major changes for HCS12 (Generalised code)            - pgo
+-----------+------------------------------------------------------------------
| 20 Nov 11 | 4.8.0 Major changes for Coldfire+ (Generalised code)        - pgo
+-----------+------------------------------------------------------------------
|  4 Oct 11 | 4.7.0 Added progress dialogues                              - pgo
+-----------+------------------------------------------------------------------
| 23 Apr 11 | 4.6.0 Major changes for CFVx programming                    - pgo
+-----------+------------------------------------------------------------------
|  6 Apr 11 | 4.6.0 Major changes for ARM programming                     - pgo
+-----------+------------------------------------------------------------------
|  3 Jan 11 | 4.4.0 Major changes for XML device files etc                - pgo
+-----------+------------------------------------------------------------------
| 17 Sep 10 | 4.0.0 Fixed minor bug in isTrimLocation()                   - pgo
+-----------+------------------------------------------------------------------
| 30 Jan 10 | 2.0.0 Changed to C++                                        - pgo
|           |       Added paged memory support                            - pgo
+-----------+------------------------------------------------------------------
| 15 Dec 09 | 1.1.1 setFlashSecurity() was modifying image unnecessarily  - pgo
+-----------+------------------------------------------------------------------
| 14 Dec 09 | 1.1.0 Changed Trim to use linear curve fitting              - pgo
|           |       FTRIM now combined with image value                   - pgo
+-----------+------------------------------------------------------------------
|  7 Dec 09 | 1.0.3 Changed SOPT value to disable RESET pin               - pgo
+-----------+------------------------------------------------------------------
| 29 Nov 09 | 1.0.2 Bug fixes after trim testing                          - pgo
+-----------+------------------------------------------------------------------
| 17 Nov 09 | 1.0.0 Created                                               - pgo
+==============================================================================
\endverbatim
*/
#define _WIN32_IE 0x0500 //!< Required for common controls?

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string>
#include <ctype.h>
#include <memory>
#include "Common.h"
#include "Log.h"
#include "FlashImage.h"
#include "FlashProgramming.h"
#include "USBDM_API.h"
#include "TargetDefines.h"
#include "Utils.h"
#include "ProgressTimer.h"
#include "SimpleSRecords.h"
#if TARGET == ARM
#include "USBDM_ARM_API.h"
#include "STM32F100xx.h"
#include "ARM_Definitions.h"
#elif TARGET == MC56F80xx
#include "USBDM_DSC_API.h"
#endif
#include "usbdmTcl.h"
#include "wxPlugin.h"
#ifdef GDI
#include "GDI.h"
#include "MetrowerksInterface.h"
#endif
#include "Names.h"

/* ======================================================================
 * Notes on BDM clock source (for default CLKSW):
 *
 *  CPU    BDM clock
 *  ----------------------
 *  RS08   bus clock
 *  HCS08  bus clock
 *  HC12   bus clock
 *  CFV1   bus clock
 *
 */
//=============================================================================

#if TARGET == ARM
#define WriteMemory(elementSize, byteCount, address, data) ARM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  ARM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) ARM_WriteRegister(ARM_RegPC, (regValue))
#define ReadPC(regValue)  ARM_ReadRegister(ARM_RegPC,  (regValue))
#define TargetGo()        ARM_TargetGo()
#define TargetHalt()      ARM_TargetHalt()
#define TARGET_TYPE T_ARM_JTAG
#elif TARGET == MC56F80xx
#define WriteMemory(elementSize, byteCount, address, data) DSC_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  DSC_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) DSC_WriteRegister(DSC_RegPC, (regValue))
#define ReadPC(regValue)  DSC_ReadRegister(DSC_RegPC,  (regValue))
#define TargetGo()        DSC_TargetGo()
#define TargetHalt()      DSC_TargetHalt()
#define TARGET_TYPE T_MC56F80xx
#elif TARGET == CFVx
#define WriteMemory(elementSize, byteCount, address, data) USBDM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  USBDM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) USBDM_WriteCReg(CFVx_CRegPC, (regValue))
#define ReadPC(regValue)  USBDM_ReadCReg(CFVx_CRegPC,  (regValue))
#define TargetGo()        USBDM_TargetGo()
#define TargetHalt()      USBDM_TargetHalt()
#define TARGET_TYPE T_CFVx
#elif TARGET == CFV1
#define WriteMemory(elementSize, byteCount, address, data) USBDM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  USBDM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) USBDM_WriteCReg(CFV1_CRegPC, (regValue))
#define ReadPC(regValue)  USBDM_ReadCReg(CFV1_CRegPC,  (regValue))
#define TargetGo()        USBDM_TargetGo()
#define TargetHalt()      USBDM_TargetHalt()
#define TARGET_TYPE T_CFV1
#elif TARGET == HCS12
#define WriteMemory(elementSize, byteCount, address, data) USBDM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  USBDM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) USBDM_WriteReg(HCS12_RegPC, (regValue))
#define ReadPC(regValue)  USBDM_ReadReg(HCS12_RegPC,  (regValue))
#define TargetGo()        USBDM_TargetGo()
#define TargetHalt()      USBDM_TargetHalt()
#define TARGET_TYPE T_HCS12
#elif TARGET == HCS08
#define WriteMemory(elementSize, byteCount, address, data) USBDM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  USBDM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) USBDM_WriteReg(HCS08_RegPC, (regValue))
#define ReadPC(regValue)  USBDM_ReadReg(HCS08_RegPC,  (regValue))
#define TargetGo()        USBDM_TargetGo()
#define TargetHalt()      USBDM_TargetHalt()
#define TARGET_TYPE T_HCS08
#elif TARGET == RS08
#define WriteMemory(elementSize, byteCount, address, data) USBDM_WriteMemory((elementSize), (byteCount), (address), (data))
#define ReadMemory(elementSize, byteCount, address, data)  USBDM_ReadMemory((elementSize), (byteCount), (address), (data))
#define WritePC(regValue) USBDM_WriteReg(RS08_RegPC, (regValue))
#define ReadPC(regValue)  USBDM_ReadReg(RS08_RegPC,  (regValue))
#define TargetGo()        USBDM_TargetGo()
#define TargetHalt()      USBDM_TargetHalt()
#define TARGET_TYPE T_RS08
#else
#error "Need to define macros for this target"
#endif

//=======================================================================================

inline uint16_t swap16(uint16_t data) {
   return ((data<<8)&0xFF00) + ((data>>8)&0xFF);
}
inline uint32_t swap32(uint32_t data) {
   return ((data<<24)&0xFF000000) + ((data<<8)&0xFF0000) + ((data>>8)&0xFF00) + ((data>>24)&0xFF);
}

inline uint32_t getData32Be(uint8_t *data) {
   return (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
}
inline uint32_t getData32Le(uint8_t *data) {
   return (data[3]<<24)+(data[2]<<16)+(data[1]<<8)+data[0];
}
inline uint32_t getData16Be(uint8_t *data) {
   return (data[0]<<8)+data[1];
}
inline uint32_t getData16Le(uint8_t *data) {
   return data[0]+(data[1]<<8);
}
inline uint32_t getData32Be(uint16_t *data) {
   return (data[0]<<16)+data[1];
}
inline uint32_t getData32Le(uint16_t *data) {
   return (data[1]<<16)+data[0];
}
inline const uint8_t *getData4x8Le(uint32_t data) {
   static uint8_t data8[4];
   data8[0]= data;
   data8[1]= data>>8;
   data8[2]= data>>16;
   data8[3]= data>>24;
   return data8;
}
inline const uint8_t *getData4x8Be(uint32_t data) {
   static uint8_t data8[4];
   data8[0]= data>>24;
   data8[1]= data>>16;
   data8[2]= data>>8;
   data8[3]= data;
   return data8;
}
inline const uint8_t *getData2x8Le(uint32_t data) {
   static uint8_t data8[2];
   data8[0]= data;
   data8[1]= data>>8;
   return data8;
}
inline const uint8_t *getData2x8Be(uint32_t data) {
   static uint8_t data8[2];
   data8[0]= data>>8;
   data8[1]= data;
   return data8;
}

#if (TARGET == ARM) || (TARGET == MC56F80xx)
#define targetToNative16(x) (x)
#define targetToNative32(x) (x)
#define nativeToTarget16(x) (x)
#define nativeToTarget32(x) (x)

inline uint32_t getData32Target(uint8_t *data) {
   return getData32Le(data);
}
inline uint32_t getData16Target(uint8_t *data) {
   return *data;
}
inline uint32_t getData32Target(uint16_t *data) {
   return getData32Le(data);
}
inline uint32_t getData16Target(uint16_t *data) {
   return *data;
}
#else
#define targetToNative16(x) swap16(x)
#define targetToNative32(x) swap32(x)
#define nativeToTarget16(x) swap16(x)
#define nativeToTarget32(x) swap32(x)

inline uint32_t getData32Target(uint8_t *data) {
   return getData32Be(data);
}
inline uint32_t getData16Target(uint8_t *data) {
   return getData16Be(data);
}
#endif
//=======================================================================
//! Calculate delay value for Flash programming. \n
//! See flash program for calculation method.
//!
//! @param delayValue  = Delay value passed to target program
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::calculateFlashDelay(uint8_t *delayValue) {
   unsigned long bdmFrequency;
   double busFrequency;
   USBDM_ErrorCode rc;

   rc = USBDM_GetSpeedHz(&bdmFrequency);
   if (rc != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM;
   }
   busFrequency = bdmFrequency;

   // The values are chosen to satisfy Tprog including loop overhead above.
   // The other delays are less critical (minimums only)
   // Assuming bus freq. = ~4 MHz clock => 250 ns, 4cy = 1us
   // For 30us@4MHz, 4x30 = 120 cy, N = (4x30-30-10)/4 = 20
   double tempValue = round(((busFrequency*30E-6)-30-10)/4);
   if ((tempValue<0) || (tempValue>255)) {
      // must be 8 bits
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   *delayValue = tempValue;
   print("FlashProgrammer::calculateFlashDelay() => busFreq=%f Hz, delValue=%d\n", busFrequency, *delayValue);

   return PROGRAMMING_RC_OK;
}

//=============================================================================
//! Connects to the target. \n
//! - Resets target to special mode
//! - Connects
//! - Runs initialisation script
//!
//! @return error code, see \ref USBDM_ErrorCode \n
//!   BDM_OK                       => Success \n
//!   PROGRAMMING_RC_ERROR_SECURED => Device is connected but secured (target connection speed may have been approximated)\n
//!                           USBDM_getStatus() may be used to determine connection method.
//!
USBDM_ErrorCode FlashProgrammer::resetAndConnectTarget(void) {
   USBDM_ErrorCode bdmRc;
   USBDM_ErrorCode rc;

   print("FlashProgrammer::resetAndConnectTarget()\n");

   if (parameters.getTargetName().empty()) {
      return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   }
   flashReady     = false;
   initTargetDone = false;

   // Reset to special mode to allow unlocking of Flash
#if TARGET == ARM
   bdmRc = ARM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_DEFAULT));
   if ((bdmRc != BDM_RC_OK) && (bdmRc != BDM_RC_SECURED))
      bdmRc = ARM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE));
#elif TARGET == MC56F80xx
   bdmRc = DSC_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_DEFAULT));
   if ((bdmRc != BDM_RC_OK) && (bdmRc != BDM_RC_SECURED))
      bdmRc = DSC_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE));
#elif (TARGET == CFVx) || (TARGET == HC12)
   bdmRc = USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_DEFAULT));
   if (bdmRc != BDM_RC_OK)
      bdmRc = USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE));
#else
   bdmRc = USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_DEFAULT));
   if (bdmRc != BDM_RC_OK)
      bdmRc = USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE));
#endif
   if (bdmRc == BDM_RC_SECURED) {
      print("FlashProgrammer::resetAndConnectTarget() ... Device is secured\n");
      return PROGRAMMING_RC_ERROR_SECURED;
   }
   if (bdmRc != BDM_RC_OK) {
      print( "FlashProgrammer::resetAndConnectTarget() ... Failed Reset, %s!\n",
            USBDM_GetErrorString(bdmRc));
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }
#if TARGET == HC12
   // A blank device can be very slow to complete reset
   // I think this is due to the delay in blank checking the entire Flash
   // It depends on the target clock so is problem for slow external clocks on HCS12
   // Trying to connect at this time upsets this check
   milliSleep(200);
#endif

   // Try auto Connect to target
#if TARGET == ARM
   bdmRc = ARM_Connect();
#elif TARGET == MC56F80xx
   bdmRc = DSC_Connect();
#else
   // BDM_RC_BDM_EN_FAILED usually means a secured device
   bdmRc = USBDM_Connect();
#endif
   switch (bdmRc) {
   case BDM_RC_SECURED:
   case BDM_RC_BDM_EN_FAILED:
      // Treat as secured & continue
      rc = PROGRAMMING_RC_ERROR_SECURED;
      print( "FlashProgrammer::resetAndConnectTarget() ... Partial Connect, rc = %s!\n",
            USBDM_GetErrorString(bdmRc));
      break;
   case BDM_RC_OK:
      rc = PROGRAMMING_RC_OK;
      break;
   default:
      print( "FlashProgrammer::resetAndConnectTarget() ... Failed Connect, rc = %s!\n",
            USBDM_GetErrorString(bdmRc));
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }
#if TARGET == ARM
   TargetHalt();
#endif
   // Use TCL script to set up target
   USBDM_ErrorCode rc2 = initialiseTarget();
   if (rc2 != PROGRAMMING_RC_OK)
      rc = rc2;

   return rc;
}

//=============================================================================
//! Reads the System Device Identification Register
//!
//! @param targetSDID - location to return SDID
//! @param doInit - reset & re-connect to target first
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note Assumes the target has been reset in SPECIAL mode
//!
USBDM_ErrorCode FlashProgrammer::readTargetChipId(uint32_t *targetSDID, bool doInit) {
   uint8_t SDIDValue[4];

   print("FlashProgrammer::readTargetSDID()\n");

   if (parameters.getTargetName().empty()) {
      print("FlashProgrammer::readTargetSDID() - target name not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   *targetSDID = 0x0000;

   if (doInit) {
      USBDM_ErrorCode rc = resetAndConnectTarget();
      if (rc != PROGRAMMING_RC_OK)
         return rc;
   }
   if (ReadMemory(2, 2, parameters.getSDIDAddress(), SDIDValue) !=  BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   *targetSDID = getData16Target(SDIDValue);

   // Do a sanity check on SDID (may get these values if secured w/o any error being signalled)
   if ((*targetSDID == 0xFFFF) || (*targetSDID == 0x0000)) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   return PROGRAMMING_RC_OK;
}

//=============================================================================
//! Check the target SDID agrees with device parameters
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes the target has been connected to
//!
USBDM_ErrorCode FlashProgrammer::confirmSDID() {
   uint32_t targetSDID;
   USBDM_ErrorCode rc;

//   mtwksDisplayLine("confirmSDID() - #1\n");
   if (parameters.getTargetName().empty()) {
      print("FlashProgrammer::confirmSDID() - Error: device parameters not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
//   mtwksDisplayLine("confirmSDID() - #2\n");
   // Don't check Target SDID if zero
   if (parameters.getSDID() == 0x0000) {
      print("FlashProgrammer::confirmSDID(0x0000) => Skipping check\n");
      return PROGRAMMING_RC_OK;
   }
   // Get SDID from target
   rc = readTargetChipId(&targetSDID);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::confirmSDID(%4.4X) => Failed, error reading SDID, reason = %s\n",
            parameters.getSDID(),
            USBDM_GetErrorString(rc));
      // Return this error even though the cause may be different
      return PROGRAMMING_RC_ERROR_WRONG_SDID;
   }
   if (!parameters.isThisDevice(targetSDID)) {
      print("FlashProgrammer::confirmSDID(%4.4X) => Failed (Target SDID=%4.4X)\n",
            parameters.getSDID(),
            targetSDID);
      return PROGRAMMING_RC_ERROR_WRONG_SDID;
   }
   print("FlashProgrammer::confirmSDID(%4.4X) => OK\n", targetSDID);
   return PROGRAMMING_RC_OK;
}

//=============================================================================
//! Prepares the target \n
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes target has been reset & connected
//!
USBDM_ErrorCode FlashProgrammer::initialiseTarget() {

   if (initTargetDone) {
      print("FlashProgrammer::initialiseTarget() - already done, skipped\n");
      return PROGRAMMING_RC_OK;
   }
   initTargetDone = true;
   print("FlashProgrammer::initialiseTarget()\n");



#if (TARGET == HCS08)
   char args[200] = "initTarget \"";
   char *argPtr = args+strlen(args);   // Add address of each flash region
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      if (!memoryRegionPtr->isProgrammableMemory()) {
         continue;
      }
      strcat(argPtr, " 0x");
      argPtr += strlen(argPtr);
      itoa(memoryRegionPtr->getDummyAddress()&0xFFFF, argPtr, 16);
      argPtr += strlen(argPtr);
   }
   *argPtr++ = '\"';
   *argPtr++ = '\0';
#elif (TARGET == HCS12)
   char args[200] = "initTarget \"";
   char *argPtr = args+strlen(args);   // Add address of each flash region
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      if (!memoryRegionPtr->isProgrammableMemory()) {
         continue;
      }
      sprintf(argPtr, "{%s 0x%04X} ",
            memoryRegionPtr->getMemoryTypeName(),
            memoryRegionPtr->getDummyAddress()&0xFFFF);
      argPtr += strlen(argPtr);
   }
   *argPtr++ = '\"';
   *argPtr++ = '\0';
#elif (TARGET == RS08)
   char args[200] = "initTarget ";
   char *argPtr = args+strlen(args);sprintf(argPtr, "0x%04X 0x%04X 0x%04X",
         parameters.getSOPTAddress(),
         flashMemoryRegionPtr->getFOPTAddress(),
         flashMemoryRegionPtr->getFLCRAddress()
         );
   argPtr += strlen(argPtr);
   *argPtr++ = '\0';
#endif

   USBDM_ErrorCode rc = runTCLCommand(args);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::initialiseTargetFlash() - initTarget TCL failed\n");
      return rc;
   }
   return rc;
}

//=============================================================================
//! Prepares the target for Flash and eeprom operations. \n
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes target has been reset & connected
//!
USBDM_ErrorCode FlashProgrammer::initialiseTargetFlash() {
   USBDM_ErrorCode rc;

   print("FlashProgrammer::initialiseTargetFlash()\n");

   // Check if already configured
   if (flashReady) {
      return PROGRAMMING_RC_OK;
   }
   // Configure the target clock for Flash programming
   unsigned long busFrequency;
   rc = configureTargetClock(&busFrequency);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::initialiseTargetFlash() - Failed to get speed\n");
      return rc;
   }
   // Convert to kHz
   targetBusFrequency = (uint32_t)round(busFrequency/1000.0);
//   this->flashData.frequency = targetBusFrequency;

   print("FlashProgrammer::initialiseTargetFlash(): Target Bus Frequency = %ld kHz\n", targetBusFrequency);

   char buffer[100];
   sprintf(buffer, "initFlash %d", targetBusFrequency);
   rc = runTCLCommand(buffer);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::initialiseTargetFlash() - initFlash TCL failed\n");
      return rc;
   }
   // Flash is now ready for programming
   flashReady = TRUE;
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//! \brief Does Mass Erase of Target memory using TCL script.
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::massEraseTarget(void) {
   if (progressTimer != NULL) {
      progressTimer->restart("Mass Erasing Target");
   }
   USBDM_ErrorCode rc = initialiseTarget();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Do Mass erase using TCL script
   rc = runTCLCommand("massEraseTarget");
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
// Don't reset device as it may only be temporarily unsecured!
   return PROGRAMMING_RC_OK;
}

//==============================================================================
//! Program to download to target RAM
//! This program is used to program the target Flash from a
//! buffer also in target RAM.
//!
static const uint8_t  RS08_flashProgram[48] = {
//                 ;********************************************************************************
//                 ; Fast RAM ($0 - $D)
//                 ;
//                 ; Programming parameters (values given are dummies for testing)
//                 ;
//                 ; This area of memory is re-written for each block (<= 8 bytes) written to flash
//                 ; The last byte (SourceAddr) is used as a flag to control execution as well as the
//                 ; source buffer address/count.  It is the last byte written by the BDM to start programming.
//                 ; The values must lie within a single flash page (64 byte modulo)
//                 ;
//                 ; Example values are for a write of 6 bytes [$11,$22,$33,$44,$55,$66] => [$3F3A..$3F3F]
//                 ;
// 0000     FLCRAddress       dc.b     HIGH_6_13(FLCR)                 ; Page # for FLCR Reg
// 0001     Buffer            dc.b     $11,$22,$33,$44,$55,$66,$77,$88 ; Buffer for data to program (<= 8 bytes)
// 0009     DestinationAddr   dc.b     MAP_ADDR_6(Destination)         ; Address in Paging Window for destination
// 000A     DestinationPage   dc.b     HIGH_6_13(Destination)          ; Page for destination
// 000B     DelayValue        dc.b     22                              ; Delay value (4MHz=>N=22, 8MHz=>N=44)
// 000C     SourceAddr        dc.b     Buffer                          ; Address in Zero Page buffer for source
// 000D     ByteCount         dc.b     BufferSize                      ; # of byte to write <page & buffer size
//
//                 ;********************************************************************************
//                 ; Flash programming code in Z-Page RAM
//                 ;
//                         //       org   RAMStart
//                         // Start:
/* 0000 */ 0x8D,           //     clr   ByteCount                  ; Set Idle mode
//                         // Idle:
/* 0001 */ 0xCD,           //     lda   ByteCount
/* 0002 */ 0x37,0xFD,      //     beq   Idle                       ; Wait for some bytes to program
/* 0004 */ 0xC0,           //     lda   FLCRAddress
/* 0005 */ 0xFF,           //     sta   PAGESEL                    ;
/* 0006 */ 0x10,0xD1,      //     bset  FLCR_PGM,MAP_ADDR_6(FLCR)  ; Enable Flash programming
/* 0008 */ 0xCA,           //     lda   DestinationPage            ; Set window to page to program
/* 0009 */ 0xFF,           //     sta   PAGESEL
/* 000A */ 0x3F,0xC0,      //     clr   MAP_ADDR_6(0)              ; Dummy write to Flash block
/* 000C */ 0xAD,0x1C,      //     bsr   delay30us                  ; Wait Tnvs
/* 000E */ 0x16,0xD1,      //     bset  FLCR_HVEN,MAP_ADDR_6(FLCR) ; Enable Flash high voltage
/* 0010 */ 0xAD,0x18,      //     bsr   delay30us                  ; Wait Tpgs
//                         // Loop:                                ; Copy byte to flash (program)
/* 0012 */ 0xCA,           //     lda   DestinationPage            ; 2 cy - Set window to page to program
/* 0013 */ 0xFF,           //     sta   PAGESEL                    ; 2 cy
/* 0014 */ 0xCC,           //     lda   SourceAddr                 ; 3 cy
/* 0015 */ 0xEF,           //     sta   X                          ; 2 cy
/* 0016 */ 0xCE,           //     lda   D[X]                       ; 3 cy
/* 0017 */ 0x4E,0x09,0x0F, //     mov   DestinationAddr,X          ; 5 cy
/* 001A */ 0xEE,           //     sta   D[X]                       ; 2 cy
/* 001B */ 0xAD,0x0D,      //     bsr   delay30us                  ; 3 cy    - Wait Tprog
/* 001D */ 0x29,           //     inc   DestinationAddr            ; 4 cy    - Update ptrs/counter
/* 001E */ 0x2C,           //     inc   SourceAddr
/* 001F */ 0x3B,0x0D,0xF0, //     dbnz  ByteCount,Loop             ; 4 cy    - Finished? - no, loop for next byte
/* 0022 */ 0x11,0xD1,      //     bclr  FLCR_PGM,MAP_ADDR_6(FLCR)  ; Disable Flash programming
/* 0024 */ 0xAD,0x04,      //     bsr   delay30us                  ; Wait Tnvh
/* 0026 */ 0x17,0xD1,      //     bclr  FLCR_HVEN,MAP_ADDR_6(FLCR) ; Disable High voltage
/* 0028 */ 0x30,0xD6,      //     bra   Start                      ; Back for more
//
//                 ;*********************************************************************
//                 ; A short delay (~30us) sufficient to satisfy all the required delays
//                 ; The PAGESEL register is set to point at the FLCR page
//                 ;
//                 ;         Tnvs  > 5us
//                 ;         Tpgs  > 10us
//                 ;         Tnvh  > 5us
//                 ;  40us > Tprog > 20us
//                 ;
//                 ; The delay may be adjusted through DelayValue
//                 ; The values are chosen to satisfy Tprog including loop overhead above (30).
//                 ; The other delays are less critical (minimums only)
//                 ; Example:
//                 ; bus freq. = ~4 MHz clock => 250 ns, 4cy = 1us
//                 ; For 30us@4MHz, 4x30 = 120 cy, so N = (4x30-30-10)/4 = 20
//                 ; Examples (30xFreq-40)/4:
//                 ;  20 MHz   N=140
//                 ;   8 MHz   N=50
//                 ;   4 MHz   N=20
//                 ;   2 MHz   N=5
//                        // delay30us:
/* 002A */ 0xCB,          //      lda   DelayValue              ;   3 cy
/* 002B */ 0x4B,0xFE,     //      dbnza *                       ; 4*N cy
/* 002D */ 0xC0,          //      lda   FLCRAddress             ;   2 cy
/* 002E */ 0xFF,          //      sta   PAGESEL                 ;   2 cy
/* 002F */ 0xBE,          //      rts                           ;   3 cy
};

//=======================================================================
//!  Relocate (apply fixups) to program image
//!
//!  @param buffer - writable copy of RS08_flashProgram for patching
//!
void FlashProgrammer::RS08_doFixups(uint8_t buffer[]) {
   // A bit ugly
   buffer[0x07] = RS08_WIN_ADDR(flashMemoryRegionPtr->getFLCRAddress());
   buffer[0x0F] = RS08_WIN_ADDR(flashMemoryRegionPtr->getFLCRAddress());
   buffer[0x23] = RS08_WIN_ADDR(flashMemoryRegionPtr->getFLCRAddress());
   buffer[0x27] = RS08_WIN_ADDR(flashMemoryRegionPtr->getFLCRAddress());
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//!  This routine does the following:
//!   - Downloads the Flash programming code into the direct memory of
//!     the RS08 target and then verifies this by reading it back.
//!   - Starts the target program execution (program will be idle).
//!   - Instructs the BDM to enable the Flash programming voltage
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been initialised for programming.
//!         Confirms download and checks RAM upper boundary.
//!
USBDM_ErrorCode FlashProgrammer::loadAndStartExecutingTargetProgram() {
USBDM_ErrorCode BDMrc;
uint8_t programImage[sizeof(RS08_flashProgram)];
uint8_t verifyBuffer[sizeof(RS08_flashProgram)];

   print("FlashProgrammer::loadAndExecuteTargetProgram()\n");

   // Create & patch image of program for target memory
   memcpy(programImage, RS08_flashProgram, sizeof(RS08_flashProgram));
   // Patch relocated addresses
   RS08_doFixups(programImage);

   // Write Flash programming code to Target memory
   BDMrc = USBDM_WriteMemory(1,
                          sizeof(programImage),
                          parameters.getRamStart(),
                          programImage );
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_WRITE;

   // Read back to verify
   BDMrc = USBDM_ReadMemory(1,
                         sizeof(programImage),
                         parameters.getRamStart(),
                         verifyBuffer );
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_READ;

   // Verify correctly loaded
   if (memcmp(verifyBuffer,
              programImage,
              sizeof(programImage)) != 0) {
      print("FlashProgrammer::loadAndExecuteTargetProgram() - program load verify failed\n");
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   // Start flash code on target
   BDMrc = USBDM_WriteReg(RS08_RegCCR_PC, parameters.getRamStart());
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_WRITE;

   BDMrc = USBDM_TargetGo();
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM;

   BDMrc = USBDM_SetTargetVpp(BDM_TARGET_VPP_STANDBY);
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM;

   BDMrc = USBDM_SetTargetVpp(BDM_TARGET_VPP_ON);
   if (BDMrc != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM;

   return PROGRAMMING_RC_OK;
}

//==================================================================================
//! Write data to RS08 Flash memory - (within a single page of Flash, & buffer size)
//!
//! @param byteCount   = Number of bytes to transfer
//! @param address     = Memory address
//! @param data        = Ptr to block of data to write
//! @param delayValue  = Delay value for target program
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::writeFlashBlock( unsigned int        byteCount,
                                               unsigned int        address,
                                               unsigned const char *data,
                                               uint8_t             delayValue) {
   int rc;
   uint8_t flashCommand[14]  = {0x00};
   uint8_t flashStatus;
//   const uint32_t PTADAddress  = 0x10;
//   const uint32_t PTADDAddress = 0x11;
//   const uint32_t PTCDAddress  = 0x4A;
//   const uint32_t PTCDDAddress = 0x4B;
//   const uint8_t  zero    = 0x00;
//   const uint8_t  allOnes = 0xFF;

   print("FlashProgrammer::writeFlashBlock(count=0x%X(%d), addr=[0x%06X..0x%06X])\n",
         byteCount, byteCount, address, address+byteCount-1);
   printDump(data, byteCount, address);

   // 0000     FLCRAddress       dc.b     HIGH_6_13(FLCR)                 ; Page # for FLCR Reg
   // 0001     Buffer            dc.b     $11,$22,$33,$44,$55,$66,$77,$88 ; Buffer for data to program (<= 8 bytes)
   // 0009     DestinationAddr   dc.b     MAP_ADDR_6(Destination)         ; Address in Paging Window for destination
   // 000A     DestinationPage   dc.b     HIGH_6_13(Destination)          ; Page for destination
   // 000B     DelayValue        dc.b     22                              ; Delay value (4MHz=>N=22, 8MHz=>N=44)
   // 000C     SourceAddr        dc.b     Buffer                          ; Address in Zero Page buffer for source
   // 000D     ByteCount         dc.b     BufferSize                      ; # of byte to write <page & buffer size

   // Set up data to write
   flashCommand[0x0] = RS08_PAGENO(flashMemoryRegionPtr->getFLCRAddress());
   if (byteCount<=8) {
      // Use tiny buffer
      unsigned int sub;
      for(sub=1; sub<=byteCount; sub++) {
         flashCommand[sub] = *data++;
      }
   }
   flashCommand[0x9] = RS08_WIN_ADDR(address);
   flashCommand[0xA] = RS08_PAGENO(address);
   flashCommand[0xB] = delayValue;
   flashCommand[0xC] = 1; // default to tiny buffer
   flashCommand[0xD] = byteCount;

   // If any of the following fail disable flash voltage
   do {
      // Transfer data to flash programming code
//      USBDM_WriteMemory(1, 1, PTADAddress,  &allOnes);
//      USBDM_WriteMemory(1, 1, PTCDAddress,  &allOnes);
      if (byteCount>8) {
         // Using large buffer
         rc = USBDM_WriteMemory(1,
                                byteCount,
                                parameters.getRamStart()+sizeof(RS08_flashProgram),
                                data);
         if (rc != BDM_RC_OK)
            break;
         // Set buffer address
         flashCommand[0xC] = parameters.getRamStart()+sizeof(RS08_flashProgram);
      }
      // Write command data - triggers flash write
      rc = USBDM_WriteMemory(1, sizeof(flashCommand), 0, flashCommand);
      if (rc != BDM_RC_OK)
         break;

      // Poll for completion - should be complete on 1st poll!
      int reTry = 2;
      do {
         if (byteCount>8) {
            milliSleep( 5 /* ms */);
         }
         rc = USBDM_ReadMemory(1, sizeof(flashStatus), 0xD, &flashStatus);
      } while ((rc == BDM_RC_OK) && (flashStatus != 0) && (reTry-->0) );

//      USBDM_WriteMemory(1, 1, PTCDAddress,  &zero);

      if (rc != BDM_RC_OK)
         break;

      // Check status
      if (flashStatus != 0) {
         rc = BDM_RC_FAIL;
         break;
      }
   } while (FALSE);

   if (rc != BDM_RC_OK) {
      // Kill flash voltage on any error
      USBDM_SetTargetVpp(BDM_TARGET_VPP_OFF);
#ifdef LOG
      {
         unsigned long temp;
         // re-sync and try to get some useful info from target
         USBDM_Connect();
         USBDM_ReadMemory(1, sizeof(flashStatus), 0xD, &flashStatus);
         USBDM_ReadReg(RS08_RegA, &temp);
         USBDM_ReadMemory(1, sizeof(flashCommand), 0, flashCommand);
      }
#endif
      return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//! Programs a block of Target Flash memory
//! The data is subdivided based upon buffer size and Flash alignment
//!
//! @param flashImage Description of flash contents to be programmed.
//! @param blockSize             Size of block to program (bytes)
//! @param flashAddress          Start address of block to program
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::programBlock(FlashImage    *flashImage,
                                              unsigned int   blockSize,
                                              uint32_t       flashAddress) {
   unsigned int bufferSize;
   unsigned int bufferAddress;
   uint8_t delayValue;
   unsigned int splitBlockSize;
   uint8_t buffer[RS08_FLASH_PAGE_SIZE];
   USBDM_ErrorCode rc;

   print("FlashProgrammer::programBlock() [0x%06X..0x%06X]\n", flashAddress, flashAddress+blockSize-1);

   if (!flashReady) {
      print("FlashProgrammer::doFlashBlock() - Error, Flash not ready\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // OK for empty block
   if (blockSize==0) {
      return PROGRAMMING_RC_OK;
   }
   rc = calculateFlashDelay(&delayValue);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   //
   // Find flash region to program - this will recurse to handle sub regions
   //
   MemoryRegionPtr memoryRegionPtr;
   for (int index=0; ; index++) {
      memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
            print("FlashProgrammer::programBlock() - Block not within target memory\n");
            return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
         }
      uint32_t lastContiguous;
      if (memoryRegionPtr->findLastContiguous(flashAddress, &lastContiguous)) {
         // Check if programmable
         if (!memoryRegionPtr->isProgrammableMemory()) {
               print("FlashProgrammer::programBlock() - Block not programmable memory\n");
               return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
         }
         // Check if block crosses boundary and will need to be split
         if ((flashAddress+blockSize-1) > lastContiguous) {
            print("FlashProgrammer::doFlashBlock() - Block crosses FLASH boundary - recursing\n");
            uint32_t firstBlockSize = lastContiguous - flashAddress + 1;
            USBDM_ErrorCode rc;
            rc = programBlock(flashImage, firstBlockSize, flashAddress);
            if (rc != PROGRAMMING_RC_OK) {
               return rc;
            }
            flashAddress += firstBlockSize;
            rc = programBlock(flashImage, blockSize-firstBlockSize, flashAddress);
            return rc;
         }
         break;
      }
   }
   MemType_t memoryType = memoryRegionPtr->getMemoryType();
   print("FlashProgrammer::doFlashBlock() - Processing %s\n", MemoryRegion::getMemoryTypeName(memoryType));
   // Initially assume buffer directly follows program in direct memory
   bufferAddress = parameters.getRamStart() + sizeof(RS08_flashProgram);

   // Calculate available space
   bufferSize = parameters.getRamEnd() - bufferAddress + 1;

   if (bufferSize <= 8) {
      // Use small buffer in Tiny RAM
      bufferSize    = 8;
      bufferAddress = 1;
   }
   // Limit buffer to a single flash page
   if (bufferSize>RS08_FLASH_PAGE_SIZE) {
      bufferSize = RS08_FLASH_PAGE_SIZE;
   }
   while (blockSize > 0) {
      // Data block must lie within a single Flash page
      splitBlockSize = RS08_FLASH_PAGE_SIZE - (flashAddress%RS08_FLASH_PAGE_SIZE); // max # of bytes remaining in this page

      // Has to fit in buffer
      if (splitBlockSize>bufferSize) {
         splitBlockSize = bufferSize;
      }
      // Can't write more than we have left
      if (splitBlockSize>blockSize) {
         splitBlockSize = blockSize;
      }
      // Copy flash data to buffer
      unsigned int sub;
      for(sub=0; sub<splitBlockSize; sub++) {
         buffer[sub] = flashImage->getValue(flashAddress+sub);
      }
      // Write block to flash
      rc = writeFlashBlock(splitBlockSize, flashAddress, buffer, delayValue);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
      // Advance to next block of data
      flashAddress += splitBlockSize;
      blockSize    -= splitBlockSize;
      progressTimer->progress(splitBlockSize, NULL);
   };
   return PROGRAMMING_RC_OK;
}

//! \brief Does Blank Check of Target Flash.
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note The target is not reset so current security state persists after erase.
//!
USBDM_ErrorCode FlashProgrammer::blankCheckTarget() {
   const unsigned bufferSize = 0x400;
   uint8_t buffer[bufferSize];
   print("FlashProgrammer::blankCheckMemory():Blank checking target memory...\n");

   int memoryRegionIndex;
   MemoryRegionPtr memoryRegionPtr;
   for (memoryRegionIndex = 0;
        (memoryRegionPtr = parameters.getMemoryRegion(memoryRegionIndex));
        memoryRegionIndex++) {
      MemType_t memoryType = memoryRegionPtr->getMemoryType();
      if ((memoryType == MemFLASH) || (memoryType == MemEEPROM)) {
         int memoryRangeIndex;
         const MemoryRegion::MemoryRange *memoryRange;
         for (memoryRangeIndex = 0;
              (memoryRange = memoryRegionPtr->getMemoryRange(memoryRangeIndex));
              memoryRangeIndex++) {
            uint32_t address = memoryRange->start;
            uint32_t memSize = 1 + memoryRange->end-memoryRange->start;
            while (memSize >0) {
               unsigned blockSize = memSize;
               if (blockSize>bufferSize)
                  blockSize = bufferSize;
               USBDM_ErrorCode rc = ReadMemory(1, blockSize, address, buffer);
               if (rc != BDM_RC_OK) {
                  print("FlashProgrammer::blankCheckMemory():Memory is not blank!\n");
                  return PROGRAMMING_RC_ERROR_NOT_BLANK;
               }
               for (unsigned index=0; index<blockSize; index++) {
                  if (buffer[index] != 0xFF)
                     return PROGRAMMING_RC_ERROR_NOT_BLANK;
               }
               address += blockSize;
               memSize -= blockSize;
            }
         }
      }
   }
   return PROGRAMMING_RC_OK;
}
//=======================================================================
//! Check security state of target
//!
//! @return PROGRAMMING_RC_OK => device is unsecured           \n
//!         PROGRAMMING_RC_ERROR_SECURED => device is secured  \n
//!         else error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//!
USBDM_ErrorCode FlashProgrammer::checkTargetUnSecured() {
   USBDM_ErrorCode rc = initialiseTarget();
   if (rc != PROGRAMMING_RC_OK)
      return rc;
   if (runTCLCommand("isUnsecure") != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::checkTargetUnSecured() - secured\n");
      return PROGRAMMING_RC_ERROR_SECURED;
   }
   print("FlashProgrammer::checkTargetUnSecured() - unsecured\n");
   return PROGRAMMING_RC_OK;
}

static const char *secValues[] = {"default", "secured", "unsecured", "intelligent"};

//==============================================================================================
//! Modifies the Security locations in the flash image according to required security options
//!
//! @param flashImage    Flash contents to be programmed.
//! @param flashRegion   The memory region involved
//!
//!
USBDM_ErrorCode FlashProgrammer::setFlashSecurity(FlashImage &flashImage, MemoryRegionPtr flashRegion) {
   uint32_t securityAddress = flashRegion->getSecurityAddress();
   const uint8_t *data;
   int size;

   if (securityAddress == 0) {
      print("FlashProgrammer::setFlashSecurity(): No security area, not modifying flash image\n");
      return PROGRAMMING_RC_OK;
   }
   switch (parameters.getSecurity()) {
      case SEC_SECURED: { // SEC=on
         SecurityInfoPtr securityInfo = flashRegion->getSecureInfo();
         if (securityInfo == NULL) {
            print("FlashProgrammer::setFlashSecurity(): Error - No settings for security area!\n");
            return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
         }
         size = securityInfo->getSize();
         data = securityInfo->getData();
#ifdef LOG
         print("FlashProgrammer::setFlashSecurity(): Setting image as secured, \n"
               "mem[0x%06X-0x%06X] = ", securityAddress, securityAddress+size/sizeof(memoryElementType)-1);
         printDump(data, size, securityAddress);
#endif
         flashImage.loadDataBytes(size, securityAddress, data);
      }
         break;
      case SEC_UNSECURED: { // SEC=off
         SecurityInfoPtr unsecurityInfo = flashRegion->getUnsecureInfo();
         if (unsecurityInfo == NULL) {
            print("FlashProgrammer::setFlashSecurity(): Error - No settings for security area!\n");
            return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
         }
         size = unsecurityInfo->getSize();
         data = unsecurityInfo->getData();
#ifdef LOG
         print("FlashProgrammer::setFlashSecurity(): Setting image as unsecured, \n"
               "mem[0x%06X-0x%06X] = ", securityAddress, securityAddress+size/sizeof(memoryElementType)-1);
         printDump(data, size, securityAddress);
#endif
         flashImage.loadDataBytes(size, securityAddress, data);
      }
         break;
      case SEC_INTELLIGENT: { // SEC=off if not already modified
         SecurityInfoPtr unsecurityInfo = flashRegion->getUnsecureInfo();
         if (unsecurityInfo == NULL) {
            print("FlashProgrammer::setFlashSecurity(): Error - No settings for security area!\n");
            return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
         }
         size = unsecurityInfo->getSize();
         data = unsecurityInfo->getData();
#ifdef LOG
         print("FlashProgrammer::setFlashSecurity(): Setting image as intelligently unsecured, \n"
               "mem[0x%06X-0x%06X] = ", securityAddress, securityAddress+size/sizeof(memoryElementType)-1);
         printDump(data, size, securityAddress);
#endif
         flashImage.loadDataBytes(size, securityAddress, data, true);
      }
         break;
      case SEC_DEFAULT:   // Unchanged
      default:
         print("FlashProgrammer::setFlashSecurity(): Leaving flash image unchanged\n");
         break;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//! Erase Target Flash memory
//!
//! @param flashImage  -  Flash image used to determine regions to erase.
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::setFlashSecurity(FlashImage &flashImage) {
   print("FlashProgrammer::setFlashSecurity()\n");

   // Process each flash region
   USBDM_ErrorCode rc = BDM_RC_OK;
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      rc = setFlashSecurity(flashImage, memoryRegionPtr);
      if (rc != BDM_RC_OK) {
         break;
      }
   }
   return rc;
}

//=======================================================================
//! Checks if a block of Target Flash memory is blank [== 0xFF]
//!
//! @param flashImage  Used to determine memory model
//! @param blockSize              Size of block to check
//! @param flashAddress           Start address of block to program
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::blankCheckBlock(FlashImage   *flashImage,
                                                 unsigned int  blockSize,
                                                 unsigned int  flashAddress){
#define CHECKBUFFSIZE 0x8000
uint8_t buffer[CHECKBUFFSIZE];
unsigned int currentBlockSize;
unsigned int index;

   print("FlashProgrammer::blankCheckBlock() - Blank checking block[0x%04X..0x%04X]\n", flashAddress,
                                                     flashAddress+blockSize-1);
   while (blockSize>0) {
      currentBlockSize = blockSize;
      if (currentBlockSize > CHECKBUFFSIZE)
         currentBlockSize = CHECKBUFFSIZE;
      if (ReadMemory(1, currentBlockSize, flashAddress, buffer)!= BDM_RC_OK)
         return PROGRAMMING_RC_ERROR_BDM_READ;
      for (index=0; index<currentBlockSize; index++) {
         if (buffer[index] != 0xFF) {
            print("FlashProgrammer::blankCheckBlock() - Blank checking [0x%4.4X] => 0x%2.2X - failed\n",
                            flashAddress+index, buffer[index]);
            return PROGRAMMING_RC_ERROR_NOT_BLANK;
         }
      }
      flashAddress += currentBlockSize;
      blockSize    -= currentBlockSize;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
// Executes a TCL script in the current TCL interpreter
//
USBDM_ErrorCode FlashProgrammer::runTCLScript(TclScriptPtr script) {
   print("FlashProgrammer::runTCLScript(): Running TCL Script...\n");

   if (tclInterpreter == NULL)
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;

   if (evalTclScript(tclInterpreter, script->getScript().c_str()) != 0) {
      print("FlashProgrammer::runTCLScript(): Failed\n");
      print(script->toString().c_str());
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
//   print("FlashProgrammer::runTCLScript(): Script = %s\n",
//          (const char *)script->getScript().c_str());
//   print("FlashProgrammer::runTCLScript(): Complete\n");
   return PROGRAMMING_RC_OK;
}

//=======================================================================
// Executes a TCL command previously loaded in the TCL interpreter
//
USBDM_ErrorCode FlashProgrammer::runTCLCommand(const char *command) {
   print("FlashProgrammer::runTCLCommand(): Running TCL Command '%s'\n", command);

   if (!useTCLScript) {
      print("FlashProgrammer::runTCLCommand(): Not using TCL\n");
      return PROGRAMMING_RC_OK;
   }
   if (tclInterpreter == NULL) {
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if (evalTclScript(tclInterpreter, command) != 0) {
      print("FlashProgrammer::runTCLCommand(): TCL Command '%s' failed\n", command);
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
//   print("FlashProgrammer::runTCLCommand(): Complete\n");
   return PROGRAMMING_RC_OK;
}

//=======================================================================
// Initialises TCL support for current target
//
USBDM_ErrorCode FlashProgrammer::initTCL(void) {

//   print("FlashProgrammer::initTCL()\n");

   // Set up TCL interpreter only once
   if (tclInterpreter != NULL)
      return PROGRAMMING_RC_OK;
	  
   useTCLScript = false;

//   FILE *fp = fopen("c:/delme.log", "wt");
//   tclInterpreter = createTclInterpreter(TARGET_TYPE, fp);
   tclInterpreter = createTclInterpreter(TARGET_TYPE, getLogFileHandle());
   if (tclInterpreter == NULL) {
      print("FlashProgrammer::initTCL() - no TCL interpreter\n");
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
   // Run initial TCL script (loads routines)
   TclScriptPtr script = parameters.getFlashScripts();
   if (!script) {
      print("FlashProgrammer::initTCL() - no TCL script found\n");
      return PROGRAMMING_RC_ERROR_TCL_SCRIPT;
   }
#if defined(LOG) && 0
   print("FlashProgrammer::initTCL()\n");
   print(script->toString().c_str());
#endif
   USBDM_ErrorCode rc = runTCLScript(script);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::initTCL() - runTCLScript() failed\n");
      return rc;
   }
   useTCLScript = true;
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//  Release the current TCL interpreter
//
USBDM_ErrorCode FlashProgrammer::releaseTCL(void) {
   if (tclInterpreter != NULL) {
      freeTclInterpreter(tclInterpreter);
      tclInterpreter = NULL;
   }
   return PROGRAMMING_RC_OK;
}

//==================================================================================
//! doVerify - Verifies the Target memory against memory image
//!
//! @param flashImage Description of flash contents to be verified.
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note Assumes target connection has been established
//! @note Assumes callback has been set up if used.
//! @note If target clock trimming is enabled then the Non-volatile clock trim
//!       locations are ignored.
//!
USBDM_ErrorCode FlashProgrammer::doVerify(FlashImage *flashImage) {
   const unsigned MAX_BUFFER=0x800;
   uint8_t buffer[MAX_BUFFER];
   int checkResult = TRUE;
   int blockResult;
   print("FlashProgrammer::doVerify()\n");

   progressTimer->restart("Verifying...");
   FlashImage::Enumerator *enumerator = flashImage->getEnumerator();

  while (enumerator->isValid()) {
      uint32_t startBlock = enumerator->getAddress();
      // Find end of block to verify
      enumerator->lastValid();
      unsigned regionSize = enumerator->getAddress() - startBlock + 1;
      print("FlashProgrammer::doVerify() - Verifying Block[0x%8.8X..0x%8.8X]\n", startBlock, startBlock+regionSize-1);

      while (regionSize>0) {
         unsigned blockSize = regionSize;
         if (blockSize > MAX_BUFFER) {
            blockSize = MAX_BUFFER;
         }
         if (ReadMemory(1, blockSize, startBlock, buffer) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_BDM_READ;
         }
         blockResult = TRUE;
         uint32_t testIndex;
         for (testIndex=0; testIndex<blockSize; testIndex++) {
            if (flashImage->getValue(startBlock+testIndex) != buffer[testIndex]) {
               blockResult = FALSE;
#ifndef LOG
               break;
#endif
//               print("Verifying location[0x%8.8X]=>failed, image=%2.2X != target=%2.2X\n",
//                     startBlock+testIndex,
//                     (uint8_t)(flashImage->getValue(startBlock+testIndex]),
//                     buffer[testIndex]);
            }
         }
         print("FlashProgrammer::doVerify() - Verifying Sub-block[0x%8.8X..0x%8.8X]=>%s\n",
               startBlock, startBlock+blockSize-1,blockResult?"OK":"FAIL");
         checkResult = checkResult && blockResult;
         regionSize -= blockSize;
         startBlock += blockSize;
         progressTimer->progress(blockSize, NULL);
#ifndef LOG
         if (!checkResult) {
            break;
         }
#endif
      }
#ifndef LOG
      if (!checkResult) {
         break;
      }
#endif
      // Advance to start of next occupied region
      enumerator->nextValid();
   }
   if (enumerator != NULL)
      delete enumerator;

   return checkResult?PROGRAMMING_RC_OK:PROGRAMMING_RC_ERROR_FAILED_VERIFY;
}

//=======================================================================
//! Verify Target Flash memory
//!
//! @param flashImage        - Description of flash contents to be verified.
//! @param progressCallBack  - Callback function to indicate progress
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note If target clock trimming is enabled then the Non-volatile clock trim
//!       locations are ignored.
//!
USBDM_ErrorCode FlashProgrammer::verifyFlash(FlashImage  *flashImage, 
                                             CallBackT progressCallBack) {

   USBDM_ErrorCode rc;
   if ((this == NULL) || (parameters.getTargetName().empty())) {
      print("FlashProgrammer::programFlash() - Error: device parameters not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   print("===========================================================\n"
         "FlashProgrammer::verifyFlash()\n"
         "\tprogressCallBack = %p\n"
         "\tDevice = \'%s\'\n"
         "\tTrim, F=%ld, NVA@%4.4X, clock@%4.4X\n"
         "\tRam[%4.4X...%4.4X]\n"
         "\tErase=%s\n"
         "\tSecurity=%s\n",
         progressCallBack,
         parameters.getTargetName().c_str(),
         parameters.getClockTrimFreq(),
         parameters.getClockTrimNVAddress(),
         parameters.getClockAddress(),
         parameters.getRamStart(),
         parameters.getRamEnd(),
         DeviceData::getEraseOptionName(parameters.getEraseOption()),
         secValues[parameters.getSecurity()]);

   this->doRamWrites = false;

   if (progressTimer != NULL) {
      delete progressTimer;
   }
   progressTimer = new ProgressTimer(progressCallBack, flashImage->getByteCount());
   progressTimer->restart("Initialising...");

   flashReady = FALSE;
   currentFlashProgram.reset();

   rc = initTCL();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   if (parameters.getTargetName().empty()) {
      print("FlashProgrammer::verifyFlash() - Error: device parameters not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Set up the target for Flash operations
   rc = resetAndConnectTarget();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   rc = checkTargetUnSecured();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Modify flash image according to security options - to be consistent with what is programmed
   rc = setFlashSecurity(*flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#if (TARGET == CFV1) || (TARGET == HCS08)
   // Modify flash image according to trim options - to be consistent with what is programmed
   rc = dummyTrimLocations(flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
   rc = doVerify(flashImage);

   print("FlashProgrammer::verifyFlash() - Verifying Time = %3.2f s, rc = %d\n", progressTimer->elapsedTime(), rc);

   return rc;
}

//=======================================================================
//! Program Target Flash memory
//!
//! @param flashImage        - Description of flash contents to be programmed.
//! @param progressCallBack  - Callback function to indicate progress
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note The FTRIM etc. locations in the flash image may be modified with trim values.
//! @note Security locations within the flash image may be modified to effect the protection options.
//!
USBDM_ErrorCode FlashProgrammer::programFlash(FlashImage  *flashImage, 
                                              CallBackT    progressCallBack, 
                                              bool         doRamWrites) {
   USBDM_ErrorCode rc;
   bool targetBlank = false;
   if ((this == NULL) || (parameters.getTargetName().empty())) {
      print("FlashProgrammer::programFlash() - Error: device parameters not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#ifdef GDI
   mtwksDisplayLine("===========================================================\n"
         "FlashProgrammer::programFlash()\n"
         "\tDevice = \'%s\'\n"
         "\tTrim, F=%ld, NVA@%4.4X, clock@%4.4X\n"
         "\tRam[%4.4X...%4.4X]\n"
         "\tErase=%s\n"
         "\tSecurity=%s\n"
         "\tTotal bytes=%d\n",
         parameters.getTargetName().c_str(),
         parameters.getClockTrimFreq(),
         parameters.getClockTrimNVAddress(),
         parameters.getClockAddress(),
         parameters.getRamStart(),
         parameters.getRamEnd(),
         DeviceData::getEraseOptionName(parameters.getEraseOption()),
         secValues[parameters.getSecurity()],
         flashImage->getByteCount());
#else
   print("===========================================================\n"
         "FlashProgrammer::programFlash()\n"
         "\tprogressCallBack = %p\n"
         "\tDevice = \'%s\'\n"
         "\tTrim, F=%ld, NVA@%4.4X, clock@%4.4X\n"
         "\tRam[%4.4X...%4.4X]\n"
         "\tErase=%s\n"
         "\tSecurity=%s\n"
         "\tTotal bytes=%d\n",
         progressCallBack,
         parameters.getTargetName().c_str(),
         parameters.getClockTrimFreq(),
         parameters.getClockTrimNVAddress(),
         parameters.getClockAddress(),
         parameters.getRamStart(),
         parameters.getRamEnd(),
         DeviceData::getEraseOptionName(parameters.getEraseOption()),
         secValues[parameters.getSecurity()],
         flashImage->getByteCount());
#endif

   this->doRamWrites = doRamWrites;
   if (progressTimer != NULL) {
      delete progressTimer;
   }
   progressTimer = new ProgressTimer(progressCallBack, flashImage->getByteCount());
   progressTimer->restart("Initialising...");

   flashReady = FALSE;
   currentFlashProgram.reset();

   rc = initTCL();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   if (parameters.getTargetName().empty()) {
      print("FlashProgrammer::programFlash() - Error: device parameters not set\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Connect to target
   rc = resetAndConnectTarget();
   if (rc != PROGRAMMING_RC_OK) {
      if ((rc != PROGRAMMING_RC_ERROR_SECURED) ||
          (parameters.getEraseOption() != DeviceData::eraseMass))
         return rc;
   }
   bool secured = checkTargetUnSecured() != PROGRAMMING_RC_OK;

   // Check target security
   if (secured && (parameters.getEraseOption() != DeviceData::eraseMass)) {
      // Can't program if secured
      return PROGRAMMING_RC_ERROR_SECURED;
   }
   // Check target SDID _before_ erasing device
   rc = confirmSDID();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Modify flash image according to security options
   rc = setFlashSecurity(*flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Mass erase 
   if (parameters.getEraseOption() == DeviceData::eraseMass) {
      rc = massEraseTarget();
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
#if !defined(LOG) || 1
      targetBlank = true;
#endif
   }
#if (TARGET == RS08) || (TARGET == CFV1) || (TARGET == HCS08)
   // Calculate clock trim values & update memory image
   rc = setFlashTrimValues(flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif   
   // Set up for Flash operations (clock etc)
   rc = initialiseTargetFlash();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   //
   // The above leaves the Flash ready for programming
   //

   // Load default flash programming code to target
   rc = loadAndStartExecutingTargetProgram();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   print("FlashProgrammer::programFlash() - Erase Time = %3.2f s\n", progressTimer->elapsedTime());

   // Program flash
   FlashImage::Enumerator *enumerator = flashImage->getEnumerator();

   progressTimer->restart("Programming...");
   print("FlashProgrammer::programFlash() - Total Bytes = %d\n", flashImage->getByteCount());
   while(enumerator->isValid()) {
      // Start address of block to program to flash
      uint32_t startBlock = enumerator->getAddress();

      // Find end of block to program
      enumerator->lastValid();
      uint32_t blockSize = enumerator->getAddress() - startBlock + 1;

      //print("Block size = %4.4X (%d)\n", blockSize, blockSize);
      if (blockSize>0) {
         // Program block [startBlock..endBlock]
         if (!targetBlank) {
            // Need to check if range is currently blank
            // It is not permitted to re-program flash
            rc = blankCheckBlock(flashImage, blockSize, startBlock);
            if (rc != PROGRAMMING_RC_OK) {
               USBDM_SetTargetVpp(BDM_TARGET_VPP_OFF);
               return rc;
            }
         }
         rc = programBlock(flashImage, blockSize, startBlock);
         if (rc != PROGRAMMING_RC_OK) {
            USBDM_SetTargetVpp(BDM_TARGET_VPP_OFF);
            print("FlashProgrammer::programFlash() - programming failed, Reason= %s\n", USBDM_GetErrorString(rc));
            break;
         }
      }
      // Advance to start of next occupied region
      enumerator->nextValid();
   }
   USBDM_SetTargetVpp(BDM_TARGET_VPP_OFF);

   delete enumerator;
   enumerator = NULL;

   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::programFlash() - erasing failed, Reason= %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
#ifdef GDI
   if (parameters.getClockTrimFreq() != 0) {
      uint16_t trimValue = parameters.getClockTrimValue();
      mtwksDisplayLine("FlashProgrammer::programFlash() - Device Trim Value = %2.2X.%1X\n", trimValue>>1, trimValue&0x01);
   }
   mtwksDisplayLine("FlashProgrammer::programFlash() - Programming Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
#ifdef LOG
   print("FlashProgrammer::programFlash() - Programming Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
   rc = doVerify(flashImage);

#ifdef GDI
   mtwksDisplayLine("FlashProgrammer::programFlash() - Verifying Time = %3.2f s, rc = %d\n", progressTimer->elapsedTime(), rc);
#endif
#ifdef LOG
   print("FlashProgrammer::programFlash() - Verifying Time = %3.2f s, rc = %d\n", progressTimer->elapsedTime(), rc);
#endif
   return rc;
}

//=======================================================================
//! Set device data for flash operations
//!
//! @param DeviceData      data describing the device
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::setDeviceData(const DeviceData  &theParameters) {
   currentFlashProgram.reset();
   parameters = theParameters;
   print("FlashProgrammer::setDeviceData(%s)\n", parameters.getTargetName().c_str());
   releaseTCL();
   initTCL();
   // Find the flash region to obtain flash parameters
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr;
      memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
            break;
         }
      if (memoryRegionPtr->isProgrammableMemory()) {
         flashMemoryRegionPtr = memoryRegionPtr;
         break;
      }
   }
   if (flashMemoryRegionPtr == NULL) {
      print("FlashProgrammer::setDeviceData() - No flash memory found\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
FlashProgrammer::~FlashProgrammer() {
//      print("~FlashProgrammer()\n");
   if (progressTimer != NULL) {
      delete progressTimer;
   }
   releaseTCL();
}

