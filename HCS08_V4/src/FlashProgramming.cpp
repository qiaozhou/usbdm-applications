/*! \file
   \brief Utility Routines for programming HCS08 Flash

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
//! Set PAGE registers (PPAGE/EPAGE)
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::setPageRegisters(uint32_t physicalAddress) {
   // Process each flash region
   USBDM_ErrorCode rc = BDM_RC_OK;
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      if (memoryRegionPtr && (memoryRegionPtr->getAddressType() != AddrPaged)) {
         return PROGRAMMING_RC_OK;
      }
      if (memoryRegionPtr && memoryRegionPtr->contains(physicalAddress)) {
         uint32_t ppageAddress   = memoryRegionPtr->getPageAddress();
         uint32_t virtualAddress = (physicalAddress&0xFFFF);
         if (ppageAddress == 0) {
            // Not paged memory
            print("FlashProgrammer::setPageRegisters() - Not mapped (VirtAddr=PhyAddr=%06X)\n", physicalAddress);
            return PROGRAMMING_RC_OK;
         }
         uint16_t pageNum16 = memoryRegionPtr->getPageNo(physicalAddress);
         if (pageNum16 == MemoryRegion::NoPageNo) {
            return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
         }
         uint8_t pageNum = (uint8_t)pageNum16;
         print("FlashProgrammer::setPageRegisters() - Setting PPAGE=%2.2X (PhyAddr=%06X, VirAddr=%04X)\n", pageNum, physicalAddress, virtualAddress);
         if (USBDM_WriteMemory(1, 1, ppageAddress, &pageNum) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_PPAGE_FAIL;
         }
         uint8_t pageNumRead;
         if (USBDM_ReadMemory(1, 1, ppageAddress, &pageNumRead) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_PPAGE_FAIL;
         }
         if (pageNum != pageNumRead) {
            return PROGRAMMING_RC_ERROR_PPAGE_FAIL;
         }
         return BDM_RC_OK;
      }
   }
   return rc;
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

   char args[200] = "initTarget \"";
   char *argPtr = args+strlen(args);

#if (TARGET == HCS08)
   // Add address of each flash region
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
#elif (TARGET == HCS12)
   // Add address of each flash region
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
#elif (TARGET == RS08)
   sprintf(argPtr, "0x%04X 0x%04X 0x%04X",
         parameters.getSOPTAddress(),
         flashRegion->getFOPTAddress(),
         flashRegion->getFLCRAddress()
         );
   argPtr += strlen(argPtr);
#endif

   *argPtr++ = '\"';
   *argPtr++ = '\0';

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
static const uint8_t HCS08_flashProgram[] =
{
     /* 00 */  0x32,0x00,0x38, // l0:      ldhx    controlAddress
     /* 03 */  0xA6,0x30,      //          lda     #(mFSTAT_FPVIOL+mFSTAT_FACCERR) ; Abort any command & clear errors
     /* 05 */  0xE7,0x05,      //          sta     _FSTAT,x
     /*    */                  // doFlash:
     /* 07 */  0x32,0x00,0x3C, // l1:      ldhx    buffAddress                     ; Get byte to program & inc. source ptr
     /* 0A */  0xF6,           //          lda     0,x
     /* 0B */  0xAF,0x01,      //          aix     #1
     /* 0D */  0x96,0x00,0x3C, // l2:      sthx    buffAddress
     /*    */                  //
     /* 10 */  0x32,0x00,0x3A, // l3:      ldhx    flashAddress                    ; Write to flash & inc. destination ptr
     /* 13 */  0xF7,           //          sta     0,x
     /* 14 */  0xAF,0x01,      //          aix     #1
     /* 16 */  0x96,0x00,0x3A, // l4:      sthx    flashAddress
     /*    */                  //
     /* 19 */  0x32,0x00,0x38, // l5:      ldhx    controlAddress
     /* 1C */  0xA6,0x25,      //          lda     #mBurstProg                     ; Write flash command
     /* 1E */  0xE7,0x06,      //          sta     _FCMD,x
     /* 20 */  0xA6,0x80,      //          lda     #mFSTAT_FCBEF                   ; Write mask to initiate command
     /* 22 */  0xE7,0x05,      //          sta     _FSTAT,x                ; [pwpp]
     /* 24 */  0x9D,           //          nop                             ; [p]     Need minimum 4~ from w cycle to r
     /*    */                  // loop:
     /* 25 */  0xE6,0x05,      //          lda     _FSTAT,x                ; [prpp]  So FCCF is valid
     /* 27 */  0x48,           //          lsla                                    ; FCCF now in MSB
     /* 28 */  0x2A,0xFB,      //          bpl     loop                            ; Loop if FCCF = 0
     /*    */                  //
     /* 2A */  0x48,           //          lsla                                    ; Non-zero means there was an error (or Blank check)
     /* 2B */  0x26,0x0A,      //          bne     complete
     /*    */                  //
     /* 2D */  0x32,0x00,0x3E, // l6:      ldhx    buffCount                       ; Decrement byte counter
     /* 30 */  0xAF,0xFF,      //          aix     #-1
     /* 32 */  0x96,0x00,0x3E, // l7:      sthx    buffCount
     /* 35 */  0x26,0xD0,      //          bne     doFlash                         ; Complete? - no - loop
     /*    */                  //
     /*    */                  // complete:
     /* 37 */  0x82,           //         bgnd                                    ; Stop here on error or complete
     /*    */                  //
     /*    */                  //   ; The following locations are changed for each block programmed
     /*    */                  //   ;
     /*    */                  //
     /* 38 */ // 0x1820        //   controlAddress: dc.w  $1820       ; Base address of flash/eeprom control regs (0x100 or 0x110)
     /* 3A */ // 0xC000        //   flashAddress:   dc.w  $C000       ; Address of next flash location to program
     /* 3C */ // 0x0040        //   buffAddress:    dc.w  flashBuff   ; Address of next data byte in buffer
     /* 3E */ // 0x003C        //   buffCount:      dc.w  flashSize   ; Count of bytes to process
     /* 40 */ //               //   flashBuff:      ds.b  60          ; Buffer for data to write
};

/*
         ;****************************************************
         ;* Table of fixup addresses for re-location
         ;* These locations are patched by the loader
         ;*
         org   $1000
 5630   01 08         dc.w  l0+1,l1+1,l2+1,l3+1,l4+1,l5+1,l6+1,l7+1
        0E 11
        17 1A
        2E 33
*/
#define FLASH_PARAMETER_SIZE   (8U) //!< Size of parameter block

//! Fixup locations in \ref HCS08_flashProgram
static const int HCS08_fixUps[] = {
   0x01,0x08,0x0E,0x11,0x17,0x1A,0x2E,0x33,0
};

//=======================================================================
//!  Relocate (apply fixups) to program image
//!
//!  @param loadAddress - The address the image will be loaded at
//!  @param data        - Image to relocate
//!
//!  @note uses fixUps
//!
static void HCS08_doFixups(uint16_t loadAddress, uint8_t *data) {
int sub;

   for (sub=0; HCS08_fixUps[sub] != 0; sub++) {
      int addr;
      addr  = (data[HCS08_fixUps[sub]]<<8)+data[HCS08_fixUps[sub]+1];
      addr += loadAddress;
      data[HCS08_fixUps[sub]]   = (uint8_t)(addr>>8);
      data[HCS08_fixUps[sub]+1] = (uint8_t)addr;
   }
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been initialised for programming.
//!         Confirms download and checks RAM upper boundary.
//!
USBDM_ErrorCode FlashProgrammer::loadTargetProgram() {

   uint8_t programImage[sizeof(HCS08_flashProgram)];
   uint8_t verifyBuffer[sizeof(HCS08_flashProgram)];

   print("FlashProgrammer::loadTargetProgram()\n");

   // Create & patch image of program for target memory
   memcpy(programImage, HCS08_flashProgram, sizeof(HCS08_flashProgram));
   // Patch relocated addresses
   HCS08_doFixups(parameters.getRamStart(), programImage);

   // Probe Data RAM
   USBDM_ErrorCode rc = probeMemory(MS_Word, parameters.getRamStart());
   if (rc == BDM_RC_OK) {
      rc = probeMemory(MS_Word, parameters.getRamEnd()&~0x3);
   }
#if TARGET == MC56F80xx
   if (rc == BDM_RC_OK) {
      rc = probeMemory(MS_PWord, loadAddress);
   }
#endif
   if (rc != BDM_RC_OK) {
      return rc;
   }
   // Write the flash programming code to target memory & verify
   if (WriteMemory(1,sizeof(HCS08_flashProgram),parameters.getRamStart(),programImage) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   if (ReadMemory(1,sizeof(HCS08_flashProgram),parameters.getRamStart(),verifyBuffer) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   if (memcmp(programImage, verifyBuffer,sizeof(HCS08_flashProgram)) != 0) {
      return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   }
   return rc;
}

//! Probe RAM location
//!
//! @param memorySpace - Memory space and size of probe
//! @param address     - Address to probe
//!
//! @return BDM_RC_OK if successful
//!
USBDM_ErrorCode FlashProgrammer::probeMemory(MemorySpace_t memorySpace, uint32_t address) {
   static const uint8_t probe1[] = {0xA5, 0xF0,0xA5, 0xF0,};
   static const uint8_t probe2[] = {0x0F, 0x5A,0x0F, 0x5A,};
   uint8_t probeResult1[sizeof(probe1)];
   uint8_t probeResult2[sizeof(probe2)];
   uint8_t savedData[sizeof(probe1)];

   if (ReadMemory(memorySpace,memorySpace&MS_SIZE,address,savedData) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_READ;
   if (WriteMemory(memorySpace,memorySpace&MS_SIZE,address,probe1) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   if (ReadMemory(memorySpace,memorySpace&MS_SIZE,address,probeResult1) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_READ;
   if (WriteMemory(memorySpace,memorySpace&MS_SIZE,address,probe2) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   if (ReadMemory(memorySpace,memorySpace&MS_SIZE,address,probeResult2) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_READ;
   if (WriteMemory(memorySpace,memorySpace&MS_SIZE,address,savedData) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   if ((memcmp(probe1, probeResult1, memorySpace&MS_SIZE) != 0) ||
       (memcmp(probe2, probeResult2, memorySpace&MS_SIZE) != 0)) {
      print("FlashProgrammer::loadTargetProgram() - RAM memory probe failed @0x%08X\n", address);
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//! Programs a block of Target Flash memory
//!
//! @param flashImage Description of flash contents to be programmed.
//! @param blockSize             Size of block to program (bytes)
//! @param flashAddress          Start address of block to program
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//! @note - The memory range must be within one page for paged devices.
//!
USBDM_ErrorCode FlashProgrammer::programBlock(FlashImage    *flashImage,
                                              unsigned int   blockSize,
                                              uint32_t       flashAddress) {

   const uint32_t ramBufferAddress = parameters.getRamStart()+sizeof(HCS08_flashProgram);
   const unsigned int MaxSplitBlockSize = 0x4000;
   uint8_t buffer[MaxSplitBlockSize+10];
   unsigned int maxSplitBlockSize;
   unsigned int splitBlockSize;
   unsigned long int statusReg;
   unsigned long targetRegPC, targetRegA;
   int timeout;
   USBDM_ErrorCode rc;

   print("FlashProgrammer::programBlock() [0x%06X..0x%06X]\n", flashAddress, flashAddress+blockSize-1);

   // Check if at least 20 bytes are available for programming buffer
   if (parameters.getRamEnd()-parameters.getRamStart()+1U <=
            sizeof(HCS08_flashProgram)+FLASH_PARAMETER_SIZE+20)
      return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;

   if (!flashReady) {
      print("FlashProgrammer::doFlashBlock() - Error, Flash not ready\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Maximum split block size must be made less than buffer RAM available
   maxSplitBlockSize = parameters.getRamEnd()-parameters.getRamStart()+1 -
                     sizeof(HCS08_flashProgram)-FLASH_PARAMETER_SIZE;

   // Maximum split block size must be made less than above buffer
   if (maxSplitBlockSize > MaxSplitBlockSize) {
      maxSplitBlockSize = MaxSplitBlockSize;
   }
   // OK for empty block
   if (blockSize==0) {
      return PROGRAMMING_RC_OK;
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
   uint16_t controller = memoryRegionPtr->getRegisterAddress();
   rc = setPageRegisters(flashAddress);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   while (blockSize>0) {
      // Determine size of block to program
      splitBlockSize = blockSize;
      if (splitBlockSize>maxSplitBlockSize)
         splitBlockSize = maxSplitBlockSize;

      print("       splitBlock[0x%06X..0x%06X]\n", flashAddress, flashAddress+splitBlockSize-1);
      unsigned sub;
      // Copy flash data to buffer
      for(sub=0; sub<splitBlockSize; sub++) {
         buffer[FLASH_PARAMETER_SIZE+sub] =
               flashImage->getValue(flashAddress+sub);
      }
      int indx=0;
      // Address of flash/eeprom control registers
      buffer[indx++] = (uint8_t)(controller>>8);
      buffer[indx++] = (uint8_t)controller;
      // Flash/eeprom location to program
      buffer[indx++] = (uint8_t)(flashAddress>>8);
      buffer[indx++] = (uint8_t)flashAddress;
      // Source address
      buffer[indx++] = (uint8_t)((ramBufferAddress+FLASH_PARAMETER_SIZE)>>8);
      buffer[indx++] = (uint8_t)(ramBufferAddress+FLASH_PARAMETER_SIZE);
      // Number of bytes to program
      buffer[indx++] = (uint8_t)(sub>>8);
      buffer[indx++] = (uint8_t) sub;

      // Write the flash parameters & data to target memory
      if (USBDM_WriteMemory(1, FLASH_PARAMETER_SIZE+sub,
                            ramBufferAddress, buffer) != BDM_RC_OK)
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
#ifdef LOG
//      progressTimer->progress(1, NULL);
#endif
      // Set target PC to start of code & verify
      if (USBDM_WriteReg(HCS08_RegPC, parameters.getRamStart()) != BDM_RC_OK)
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      if (USBDM_ReadReg(HCS08_RegPC, &targetRegPC) != BDM_RC_OK)
         return PROGRAMMING_RC_ERROR_BDM_READ;
      if (parameters.getRamStart() != targetRegPC)
         return PROGRAMMING_RC_ERROR_BDM_WRITE;

//      USBDM_TargetStep(); USBDM_ReadStatusReg(&statusReg); USBDM_ReadCReg(CFV1_CRegPC,  &targetRegPC); // For debug

      // Execute the Flash program on target
      if (USBDM_TargetGo() != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM;
      }
      // Wait for target stop at execution completion
      timeout = 20;
      do {
         milliSleep(100);
         if (USBDM_ReadStatusReg(&statusReg) != BDM_RC_OK) {
            USBDM_Connect();
            USBDM_TargetHalt();
            USBDM_ReadReg(HCS08_RegPC,  &targetRegPC); // For debug
            return PROGRAMMING_RC_ERROR_BDM_READ;
         }
      } while (((statusReg & HC08_BDCSCR_BDMACT) == 0) && (--timeout>0));

      if (timeout <= 0) {
         USBDM_TargetHalt();
      }
      // Get program execution result
      if (USBDM_ReadReg(HCS08_RegA,  &targetRegA) != BDM_RC_OK) {
         print("programBlock() - Failed to read status value (reg A)\n");
         rc = PROGRAMMING_RC_ERROR_BDM_READ;
      }
      else if (USBDM_ReadReg(HCS08_RegPC,  &targetRegPC) != BDM_RC_OK) {
         print("programBlock() - Failed to read completion PC\n");
         rc = PROGRAMMING_RC_ERROR_BDM_READ;
      }
      else if (timeout <= 0) {
         print("programBlock() - Timeout\n");
         rc = PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;
      }
      else if (targetRegPC != ramBufferAddress) {
         // Program failed to complete
         print("programBlock() - Incorrect completion PC=0x%4X, Expected=0x%4X\n", targetRegPC, ramBufferAddress);
         rc = PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;
      }
      else if ((targetRegA) != 0) {
         // Unexpected execution result
         print("programBlock() - unexpected Flash status at completion, FSTAT(CBEIF,CCIF,PVIOL,ACCERR,-,BLANK,-,-)=0x%2.2X\n",
               targetRegA&0xFF);
         rc = PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;
      }
#if defined(LOG) && 0
      if (rc != PROGRAMMING_RC_OK) {
         struct {
               uint16_t controlAddr;
               uint16_t flashPtr;
               uint16_t buffPtr;
               uint16_t countRemaining; } flashResult;
         if (USBDM_ReadMemory(1, sizeof(flashResult), ramBufferAddress, (uint8_t*)&flashResult) != BDM_RC_OK) {
            print("programBlock() - Failed to read status block\n");
            return PROGRAMMING_RC_ERROR_BDM_READ;
         }
         flashResult.controlAddr    = targetToNative16(flashResult.controlAddr);
         flashResult.flashPtr       = targetToNative16(flashResult.flashPtr);
         flashResult.buffPtr        = targetToNative16(flashResult.buffPtr);
         flashResult.countRemaining = targetToNative16(flashResult.countRemaining);
         print("Control Address = 0x%04X\n"
               "Flash Address   = 0x%04X\n"
               "Buffer Address  = 0x%04X\n"
               "Count remaining = %d\n",
               flashResult.controlAddr, flashResult.flashPtr, flashResult.buffPtr, flashResult.countRemaining);
      }
#endif
      if (rc != PROGRAMMING_RC_OK) {
         print("FlashProgrammer::doFlashBlock() - Error\n");
         return rc;
      }
      // Advance to next block of data
      flashAddress  += splitBlockSize;
      blockSize     -= splitBlockSize;
      progressTimer->progress(splitBlockSize, NULL);
   }
   return PROGRAMMING_RC_OK;
}

#if (TARGET == HCS08) || (TARGET == CFV1)
//=======================================================================
//! Updates the memory image from the target flash Clock trim location(s)
//!
//! @param flashImage   = Flash image
//!
USBDM_ErrorCode FlashProgrammer::dummyTrimLocations(FlashImage *flashImage) {

unsigned size  = 0;
uint32_t start = 0;

   // Not using trim -> do nothing
   if ((parameters.getClockTrimNVAddress() == 0) ||
       (parameters.getClockTrimFreq() == 0)) {
      //print("Not using trim\n");
      return PROGRAMMING_RC_OK;
   }
   switch (parameters.getClockType()) {
      case S08ICGV1:
      case S08ICGV2:
      case S08ICGV3:
      case S08ICGV4:
         start = parameters.getClockTrimNVAddress();
         size  = 2;
         break;
      case S08ICSV1:
      case S08ICSV2:
      case S08ICSV2x512:
      case S08ICSV3:
      case S08MCGV1:
      case S08MCGV2:
      case S08MCGV3:
      case RS08ICSOSCV1:
      case RS08ICSV1:
         start = parameters.getClockTrimNVAddress();
         size  = 2;
         break;
      case CLKEXT:
      default:
         break;
   }
   if (size == 0) {
      return PROGRAMMING_RC_OK;
   }
   // Read existing trim information from target
   uint8_t data[10];
   USBDM_ErrorCode rc = ReadMemory(1,size,start,data);
   if (rc != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   print("FlashProgrammer::dummyTrimLocations() - Modifying image[0x%06X..0x%06X]\n",
         start, start+size-1);
   // Update image
   for(uint32_t index=0; index < size; index++ ) {
      flashImage->setValue(start+index, data[index]);
   }
   return PROGRAMMING_RC_OK;
}
#endif

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

      // ToDo - adjust currentBlockSize so flash read doesn't cross page boundary
      USBDM_ErrorCode rc;
      rc = setPageRegisters(flashAddress);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
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
      USBDM_ErrorCode rc = setPageRegisters(startBlock);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
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
                                              CallBackT progressCallBack) {
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
   rc = loadTargetProgram();
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
               return rc;
            }
         }
         rc = programBlock(flashImage, blockSize, startBlock);
         if (rc != PROGRAMMING_RC_OK) {
            print("FlashProgrammer::programFlash() - programming failed, Reason= %s\n", USBDM_GetErrorString(rc));
            break;
         }
      }
      // Advance to start of next occupied region
      enumerator->nextValid();
   }
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

