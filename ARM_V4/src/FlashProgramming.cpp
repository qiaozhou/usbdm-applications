/*! \file
   \brief Utility Routines for programming ARM (Kinetis) Flash

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

static const char *getFlashOperationName(FlashOperation actions);
void report(const char *msg);
static const char *getProgramActionNames(unsigned int actions);
static const char *getProgramCapabilityNames(unsigned int actions);

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

#if (TARGET == HCS08) || (TARGET == HCS12)
//=======================================================================
//! Gets the page number portion of a physical address (Flash)
//!
//! @param physicalAddress - address to examine
//! @param pageNo          - corresponding page number
//!
USBDM_ErrorCode FlashProgrammer::getPageAddress(MemoryRegionPtr memoryRegionPtr, uint32_t physicalAddress, uint8_t *pageNo) {

   *pageNo = 0x00;
   if ((memoryRegionPtr == NULL) || !memoryRegionPtr->contains(physicalAddress)) {
      print("FlashProgrammer::getPageAddress(0x%06X) - Invalid Flash address\n", physicalAddress);
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if (memoryRegionPtr->getAddressType() != AddrPaged) {
      print("FlashProgrammer::getPageAddress(0x%06X) - Not paged\n", physicalAddress);
      return PROGRAMMING_RC_OK;
   }
   uint32_t ppageAddress = memoryRegionPtr->getPageAddress();
   if (ppageAddress == 0) {
      print("FlashProgrammer::getPageAddress(0x%06X) - Not mapped\n", physicalAddress);
      return PROGRAMMING_RC_OK;
   }
   uint32_t virtualAddress = (physicalAddress&0xFFFF);
   uint16_t pageNum16 = memoryRegionPtr->getPageNo(physicalAddress);
   if (pageNum16 == MemoryRegion::NoPageNo) {
      print("FlashProgrammer::getPageAddress(0x%06X) - No page #!\n", physicalAddress);
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   *pageNo = (uint8_t)pageNum16;
   print("FlashProgrammer::getPageAddress(0x%06X) - PPAGE=%2.2X (Mapped Address=%06X)\n", physicalAddress, *pageNo, ((*pageNo)<<16)|virtualAddress);

   return PROGRAMMING_RC_OK;
}

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
#endif

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
   if (ReadMemory(4, 4, parameters.getSDIDAddress(), SDIDValue) !=  BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   *targetSDID = getData32Target(SDIDValue);

   // Do a sanity check on SDID (may get these values if secured w/o any error being signalled)
   if ((*targetSDID == 0xFFFFFFFF) || (*targetSDID == 0x00000000)) {
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
#if (TARGET==CFV1)   
   // Configure the target clock for Flash programming
   unsigned long busFrequency;
   rc = configureTargetClock(&busFrequency);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::initialiseTargetFlash() - Failed to get speed\n");
      return rc;
   }
   // Convert to kHz
   uint32_t targetBusFrequency = (uint32_t)round(busFrequency/1000.0);
   flashOperationInfo.targetBusFrequency = targetBusFrequency;

   print("FlashProgrammer::initialiseTargetFlash(): Target Bus Frequency = %ld kHz\n", targetBusFrequency);
#endif

   char buffer[100];
   sprintf(buffer, "initFlash %d", flashOperationInfo.targetBusFrequency);
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
// Flag masks
#define DO_INIT_FLASH         (1<<0) // Do initialisation of flash
#define DO_ERASE_BLOCK        (1<<1) // Erase entire flash block e.g. Flash, FlexNVM etc
#define DO_ERASE_RANGE        (1<<2) // Erase range (including option region)
#define DO_BLANK_CHECK_RANGE  (1<<3) // Blank check region
#define DO_PROGRAM_RANGE      (1<<4) // Program range (including option region)
#define DO_VERIFY_RANGE       (1<<5) // Verify range
#define DO_PARTITION_FLEXNVM  (1<<7) // Program FlexNVM DFLASH/EEPROM partitioning
#define DO_TIMING_LOOP        (1<<8) // Counting loop to determine clock speed

// 24-30 reserved
#define IS_COMPLETE           (1U<<31)

// Capability masks
#define CAP_ERASE_BLOCK        (1<<1)
#define CAP_ERASE_RANGE        (1<<2)
#define CAP_BLANK_CHECK_RANGE  (1<<3)
#define CAP_PROGRAM_RANGE      (1<<4)
#define CAP_VERIFY_RANGE       (1<<5)
#define CAP_PARTITION_FLEXNVM  (1<<7)
#define CAP_TIMING             (1<<8)

#define CAP_DSC_OVERLAY        (1<<11) // Indicates DSC code in pMEM overlays xRAM
#define CAP_DATA_FIXED         (1<<12) // Indicates TargetFlashDataHeader is at fixed address
#define CAP_RELOCATABLE        (1<<31) // Code may be relocated

#define OPT_SMALL_CODE         (0x80)

//=======================================================================
//! Loads the default Flash programming code to target memory
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - see loadTargetProgram(...) for details
//! @note - Tries device program code & then flashRegion specific if necessary
//!
USBDM_ErrorCode FlashProgrammer::loadTargetProgram(FlashOperation flashOperation) {
   // Try to get device general routines
   FlashProgramPtr flashProgram = parameters.getFlashProgram();
   if (!flashProgram) {
      print("FlashProgrammer::loadTargetProgram() - No default flash program found - searching memory regions\n");
      // Try code from any flash region
      for (int index=0; ; index++) {
         MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
         if (memoryRegionPtr == NULL) {
            break;
         }
         flashProgram = memoryRegionPtr->getFlashprogram();
         if (flashProgram != NULL) {
            break;
         }
      }
   }
   if (!flashProgram) {
      print("FlashProgrammer::loadTargetProgram() - No flash program found for target\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   return loadTargetProgram(flashProgram, flashOperation);
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//! @param  flashProgram      program to load
//! @param  flashOperation    intended operation in case of partial loading
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been connected to
//!         Confirms download (if necessary) and checks RAM boundaries.
//!
USBDM_ErrorCode FlashProgrammer::loadTargetProgram(FlashProgramPtr flashProgram, FlashOperation flashOperation) {
   memoryElementType     buffer[4000];

   print("FlashProgrammer::loadTargetProgram(%s)\n", getFlashOperationName(flashOperation));

   if (!flashProgram) {
      // Try to get device general routines
      flashProgram = parameters.getFlashProgram();
   }
   if (!flashProgram) {
      print("FlashProgrammer::loadTargetProgram(...) - No flash program found for target\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Reload flash code if
   //  - code changed
   //  - operation changed
   //  - alignment changed
   if (currentFlashProgram != flashProgram)  {
      print("FlashProgrammer::loadTargetProgram() - reloading due to change in flash code\n");
   }
   else if ((currentFlashOperation == OpNone) || (currentFlashOperation != flashOperation)) {
      print("FlashProgrammer::loadTargetProgram() - reloading due to change in flash operation\n");
   }
   else if (currentFlashAlignment != flashOperationInfo.alignment) {
      print("FlashProgrammer::loadTargetProgram() - reloading due to change in flash alignment\n");
   }
   else {
      print("FlashProgrammer::loadTargetProgram() - re-using existing code\n");
      return PROGRAMMING_RC_OK;
   }
   currentFlashOperation = OpNone;

   unsigned size; // In memoryElementType
   uint32_t loadAddress;
   USBDM_ErrorCode rc = loadSRec(flashProgram->flashProgram.c_str(),
                                 buffer,
                                 sizeof(buffer)/sizeof(buffer[0]),
                                 &size,
                                 &loadAddress);
   if (rc !=  BDM_RC_OK) {
      print("FlashProgrammer::loadTargetProgram(...) - loadSRec() failed\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#if TARGET == MC56F80xx
   MemorySpace_t memorySpace = MS_XWord;
#else      
   MemorySpace_t memorySpace = MS_Byte;
#endif   
   // Probe RAM buffer
   rc = probeMemory(memorySpace, parameters.getRamStart());
   if (rc == BDM_RC_OK) {
      rc = probeMemory(memorySpace, parameters.getRamEnd());
   }
   if (rc != BDM_RC_OK) {
      return rc;
   }
   targetProgramInfo.smallProgram = false;
   return loadLargeTargetProgram(buffer, loadAddress, size, flashProgram, flashOperation);
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//! @param  buffer            buffer containing program image
//! @param  loadAddress       address to load image at
//! @param  size              size of image (in memoryElementType)
//! @param  flashProgram      flash program corresponding to image
//! @param  flashOperation    intended operation in case of partial loading
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been connected to
//!         Confirms download (if necessary) and checks RAM upper boundary.
//!         targetProgramInfo is updated with load information
//!
//! Target Memory map
//! +---------------------------------------------------+ -+
//! |   LargeTargetImageHeader  flashProgramHeader;     |  |
//! +---------------------------------------------------+   > Unchanging written once
//! |   Flash program code....                          |  |
//! +---------------------------------------------------+ -+
//!
USBDM_ErrorCode FlashProgrammer::loadLargeTargetProgram(memoryElementType *buffer,
                                                        uint32_t           loadAddress,
                                                        uint32_t           size,
                                                        FlashProgramPtr    flashProgram,
                                                        FlashOperation     flashOperation) {
   print("FlashProgrammer::loadLargeTargetProgram()\n");

   // Find 'header' in download image
   LargeTargetImageHeader *headerPtr = (LargeTargetImageHeader*) (buffer+(getData32Target(buffer)-loadAddress));
   if (headerPtr > (LargeTargetImageHeader*)(buffer+size)) {
      print("FlashProgrammer::loadLargeTargetProgram() - Header ptr out of range\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Save the programming data structure
   uint32_t codeLoadAddress   = targetToNative32(headerPtr->loadAddress);
   uint32_t codeEntry         = targetToNative32(headerPtr->entry);
   uint32_t capabilities      = targetToNative32(headerPtr->capabilities);
   uint32_t dataHeaderAddress = targetToNative32(headerPtr->flashData);

   print("Loaded Image (unmodified) :\n"
         "   flashProgramHeader.loadAddress     = 0x%08X\n"
         "   flashProgramHeader.entry           = 0x%08X\n"
         "   flashProgramHeader.capabilities    = 0x%08X(%s)\n"
         "   flashProgramHeader.flashData       = 0x%08X\n",
         codeLoadAddress,
         codeEntry,
         capabilities,getProgramCapabilityNames(capabilities),
         dataHeaderAddress
         );
   if (codeLoadAddress != loadAddress) {
      print("FlashProgrammer::loadLargeTargetProgram() - Inconsistent actual (0x%06X) and image load addresses (0x%06X).\n",
            loadAddress, codeLoadAddress);
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   uint32_t codeLoadSize = size*sizeof(memoryElementType);

   if ((capabilities&CAP_RELOCATABLE)!=0) {
      // Relocate Code
      codeLoadAddress = (parameters.getRamStart()+3)&~3; // Relocate to start of RAM
      if (loadAddress != codeLoadAddress) {
         print("FlashProgrammer::loadLargeTargetProgram() - Loading at non-default address, load@0x%04X (relocated from=%04X)\n",
               codeLoadAddress, loadAddress);
         // Relocate entry point
         codeEntry += codeLoadAddress - loadAddress;
      }
   }
#if TARGET != MC56F80xx
   if ((codeLoadAddress < parameters.getRamStart()) || (codeLoadAddress > parameters.getRamEnd())) {
      print("FlashProgrammer::loadLargeTargetProgram() - Image load address is invalid.\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if ((codeEntry < parameters.getRamStart()) || (codeEntry > parameters.getRamEnd())) {
      print("FlashProgrammer::loadLargeTargetProgram() - Image Entry point is invalid.\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#endif
#if TARGET == MC56F80xx
   // Update location of where programming info will be located
   if ((capabilities&CAP_DSC_OVERLAY)!=0) {
      // Loading code into shared RAM - load data offset by code size
      print("FlashProgrammer::loadLargeTargetProgram() - loading data into overlayed RAM @ 0x%06X\n", dataHeaderAddress);
   }
   else {
      // Loading code into separate program RAM - load data RAM separately
      print("FlashProgrammer::loadLargeTargetProgram() - loading data into separate RAM @ 0x%06X\n", dataHeaderAddress);
   }
#else
   if ((capabilities&CAP_DATA_FIXED)==0) {
      // Relocate Data Entry to immediately after code
      dataHeaderAddress = codeLoadAddress + size;
      print("FlashProgrammer::loadLargeTargetProgram() - Relocating flashData @ 0x%06X\n", dataHeaderAddress);
   }
#endif

   // Required flash flashAlignmentMask
   uint32_t flashAlignmentMask = flashOperationInfo.alignment-1;
   uint32_t procAlignmentMask  = 2-1;

   // Save location of entry point
   targetProgramInfo.entry        = codeEntry;
   // Were to load flash buffer (including header)
   targetProgramInfo.headerAddress  = dataHeaderAddress;
   // Save offset of RAM data buffer
   uint32_t dataLoadAddress = dataHeaderAddress+sizeof(LargeTargetFlashDataHeader);
   // Align buffer address to worse case alignment for processor read
   dataLoadAddress = (dataLoadAddress+procAlignmentMask)&~procAlignmentMask;
   targetProgramInfo.dataOffset   = dataLoadAddress-dataHeaderAddress;
   // Save maximum size of the buffer (in memoryElementType)
   targetProgramInfo.maxDataSize  = parameters.getRamEnd()-dataLoadAddress+1;
   // Align buffer size to worse case alignment for processor read
   targetProgramInfo.maxDataSize  = targetProgramInfo.maxDataSize&~procAlignmentMask;
   // Align buffer size to flash alignment requirement
   targetProgramInfo.maxDataSize  = targetProgramInfo.maxDataSize&~flashAlignmentMask;
   // Save target program capabilities
   targetProgramInfo.capabilities = capabilities;
   // Save clock calibration factor
   targetProgramInfo.calibFactor   = 1;

   print("FlashProgrammer::loadLargeTargetProgram() - AlignmentMask=0x%08X\n",
         flashAlignmentMask);
   print("FlashProgrammer::loadLargeTargetProgram() -   Program code[0x%06X...0x%06X]\n",
         codeLoadAddress, codeLoadAddress+size-1);
   print("FlashProgrammer::loadLargeTargetProgram() -     Parameters[0x%06X...0x%06X]\n",
         targetProgramInfo.headerAddress,
         targetProgramInfo.headerAddress+targetProgramInfo.dataOffset-1);
   print("FlashProgrammer::loadLargeTargetProgram() -     RAM buffer[0x%06X...0x%06X]\n",
         targetProgramInfo.headerAddress+targetProgramInfo.dataOffset,
         targetProgramInfo.headerAddress+targetProgramInfo.dataOffset+targetProgramInfo.maxDataSize-1);
   print("FlashProgrammer::loadLargeTargetProgram() -          Entry=0x%06X\n", targetProgramInfo.entry);

   // RS08, HCS08, HCS12 are byte aligned
   // MC56F80xx deals with word addresses which are always aligned
   if ((codeLoadAddress & procAlignmentMask) != 0){
      print("FlashProgrammer::loadLargeTargetProgram() - codeLoadAddress is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if (((targetProgramInfo.headerAddress+targetProgramInfo.dataOffset) & procAlignmentMask) != 0){
      print("FlashProgrammer::loadLargeTargetProgram() - flashProgramHeader.dataOffset is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#if (TARGET!=ARM)
   if ((targetProgramInfo.entry & procAlignmentMask) != 0){
      print("FlashProgrammer::loadLargeTargetProgram() - flashProgramHeader.entry is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#else
   if ((targetProgramInfo.entry & procAlignmentMask) != 1){
      print("FlashProgrammer::loadLargeTargetProgram() - flashProgramHeader.entry is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#endif
   // Sanity check buffer
   if (((uint32_t)(targetProgramInfo.headerAddress+targetProgramInfo.dataOffset)<parameters.getRamStart()) ||
       ((uint32_t)(targetProgramInfo.headerAddress+targetProgramInfo.dataOffset+targetProgramInfo.maxDataSize-1)>parameters.getRamEnd())) {
      print("FlashProgrammer::loadLargeTargetProgram() - Data buffer location [0x%06X..0x%06X] is outside target RAM [0x%06X-0x%06X]\n",
            targetProgramInfo.headerAddress+targetProgramInfo.dataOffset,
            targetProgramInfo.headerAddress+targetProgramInfo.dataOffset+targetProgramInfo.maxDataSize-1,
            parameters.getRamStart(), parameters.getRamEnd());
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if (targetProgramInfo.maxDataSize<40) {
      print("FlashProgrammer::loadLargeTargetProgram() - Data buffer is too small - 0x%X \n", targetProgramInfo.maxDataSize);
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#if TARGET == MC56F80xx
   MemorySpace_t memorySpace = MS_PWord;
#else      
   MemorySpace_t memorySpace = MS_Byte;
#endif   
   headerPtr->flashData   = nativeToTarget32(targetProgramInfo.headerAddress);

   print("Loaded Image (modified) :\n"
         "   flashProgramHeader.loadAddress     = 0x%08X\n"
         "   flashProgramHeader.entry           = 0x%08X\n"
         "   flashProgramHeader.capabilities    = 0x%08X(%s)\n"
         "   flashProgramHeader.flashData       = 0x%08X\n",
         targetToNative32(headerPtr->loadAddress),
         targetToNative32(headerPtr->entry),
         capabilities,getProgramCapabilityNames(capabilities),
         targetToNative32(headerPtr->flashData)
         );
   if (currentFlashProgram != flashProgram)  {
      print("FlashProgrammer::loadLargeTargetProgram(...) - reloading due to change in flash code\n");
      // Write the flash programming code to target memory
      if (WriteMemory(memorySpace, codeLoadSize, codeLoadAddress, (uint8_t *)buffer) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
   }
   else {
      print("FlashProgrammer::loadLargeTargetProgram() - Suppressing code load as unchanged\n");
   }
   currentFlashProgram   = flashProgram;
   currentFlashOperation = flashOperation;
   currentFlashAlignment = flashOperationInfo.alignment;

   // Loaded routines support extended operations
   targetProgramInfo.programOperation = DO_BLANK_CHECK_RANGE|DO_PROGRAM_RANGE|DO_VERIFY_RANGE;
   return BDM_RC_OK;
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//! @param  buffer            buffer containing data to load
//! @param  headerAddress       address to load at
//! @param  size              size of data in buffer
//! @param  flashProgram      program to load
//! @param  flashOperation    operation to do
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been connected to
//!         Confirms down-load (if necessary) and checks RAM upper boundary.
//!
//! Target Memory map (RAM buffer)
//! +-----------------------------------------+ -+
//! |   SmallTagetFlashDataHeader flashData;  |   > Write/Read
//! +-----------------------------------------+ -+
//! |   Data to program....                   |   > Write
//! +-----------------------------------------+ -+
//! |   Flash program code....                |   > Unchanging written once
//! +-----------------------------------------+ -+
//!
USBDM_ErrorCode FlashProgrammer::loadSmallTargetProgram(memoryElementType *buffer,
                                                        uint32_t           loadAddress,
                                                        uint32_t           size,
                                                        FlashProgramPtr    flashProgram,
                                                        FlashOperation     flashOperation) {

   print("FlashProgrammer::loadSmallTargetProgram()\n");
   return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
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

//! \brief Maps a Flash action vector to Text
//!
//! @param capability => capability vector
//!
//! @return pointer to static string buffer describing the XCSR
//!
static const char *getProgramActionNames(unsigned int actions) {
unsigned index;
static char buff[250] = "";
static const char *actionTable[] = {
"DO_INIT_FLASH|",         // Do initialisation of flash
"DO_ERASE_BLOCK|",        // Mass erase device
"DO_ERASE_RANGE|",        // Erase range (including option region)
"DO_BLANK_CHECK_RANGE|",  // Blank check region
"DO_PROGRAM_RANGE|",      // Program range (including option region)
"DO_VERIFY_RANGE|",       // Verify range
"??|",
"DO_PARTITION_FLEXNVM|",  // Partition FlexNVM boundary
"DO_TIMING_LOOP|",        // Execute timing loop on target
};
   buff[0] = '\0';
   for (index=0;
        index<sizeof(actionTable)/sizeof(actionTable[0]);
         index++) {
      uint32_t mask = 1<<index;
      if ((actions&mask) != 0) {
         strcat(buff,actionTable[index]);
         actions &= ~mask;
      }
   }
   if (actions&IS_COMPLETE) {
      actions &= ~IS_COMPLETE;
      strcat(buff,"IS_COMPLETE|");
   }
   if (actions != 0) {
      strcat(buff,"???");
   }
   return buff;
}

//! \brief Maps a Flash action vector to Text
//!
//! @param capability => capability vector
//!
//! @return pointer to static string buffer describing the XCSR
//!
static const char *getFlashOperationName(FlashOperation flashOperation) {
   switch (flashOperation) {
   case OpNone                             : return "OpNone";                          break;
   case OpSelectiveErase                   : return "OpSelectiveErase";                break;
   case OpBlockErase                       : return "OpMassErase";                     break;
   case OpBlankCheck                       : return "OpBlankCheck";                    break;
   case OpProgram                          : return "OpProgram";                       break;
   case OpVerify                           : return "OpVerify";                        break;
   case OpWriteRam                         : return "OpWriteRam";                      break;
   case OpPartitionFlexNVM                 : return "OpPartitionFlexNVM";              break;
   case OpTiming                           : return "OpTiming";                        break;
   default: break;
   }
   return "Op???";
}

//! \brief Maps a Flash capability vector to Text
//!
//! @param capability => capability vector
//!
//! @return pointer to static string buffer describing the XCSR
//!
static const char *getProgramCapabilityNames(unsigned int actions) {
unsigned index;
static char buff[250] = "";
static const char *actionTable[] = {
"??|",                     // Do initialisation of flash
"CAP_ERASE_BLOCK|",        // Mass erase device
"CAP_ERASE_RANGE|",        // Erase range (including option region)
"CAP_BLANK_CHECK_RANGE|",  // Blank check region
"CAP_PROGRAM_RANGE|",      // Program range (including option region)
"CAP_VERIFY_RANGE|",       // Verify range
"??|",
"DO_PARTITION_FLEXNVM|",   // Un/lock flash with default security options  (+mass erase if needed)
"CAP_TIMING|",             // Lock flash with default security options
};

   buff[0] = '\0';
   for (index=0;
        index<sizeof(actionTable)/sizeof(actionTable[0]);
         index++) {
      if ((actions&(1<<index)) != 0) {
         strcat(buff,actionTable[index]);
      }
   }
   if (actions&CAP_DSC_OVERLAY) {
      strcat(buff,"CAP_DSC_OVERLAY|");
   }
   if (actions&CAP_DATA_FIXED) {
      strcat(buff,"CAP_DATA_FIXED|");
   }
   if (actions&CAP_RELOCATABLE) {
      strcat(buff,"CAP_RELOCATABLE");
   }
   return buff;
}

USBDM_ErrorCode FlashProgrammer::convertTargetErrorCode(FlashDriverError_t rc) {

   switch (rc) {
   case FLASH_ERR_OK                : return PROGRAMMING_RC_OK;
   case FLASH_ERR_LOCKED            : return PROGRAMMING_RC_ERROR_SECURED;               // Flash is still locked
   case FLASH_ERR_ILLEGAL_PARAMS    : return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;        // Parameters illegal
   case FLASH_ERR_PROG_FAILED       : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;  // STM - Programming operation failed - general
   case FLASH_ERR_PROG_WPROT        : return PROGRAMMING_RC_ERROR_SECURED;               // STM - Programming operation failed - write protected
   case FLASH_ERR_VERIFY_FAILED     : return PROGRAMMING_RC_ERROR_FAILED_VERIFY;         // Verify failed
   case FLASH_ERR_ERASE_FAILED      : return PROGRAMMING_RC_ERROR_NOT_BLANK;             // Not blank after erase
   case FLASH_ERR_TRAP              : return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED; // Program trapped (illegal instruction/location etc.)
   case FLASH_ERR_PROG_ACCERR       : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;  // Kinetis/CFVx - Programming operation failed - ACCERR
   case FLASH_ERR_PROG_FPVIOL       : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;  // Kinetis/CFVx - Programming operation failed - FPVIOL
   case FLASH_ERR_PROG_MGSTAT0      : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;  // Kinetis - Programming operation failed - MGSTAT0
   case FLASH_ERR_CLKDIV            : return PROGRAMMING_RC_ERROR_NO_VALID_FCDIV_VALUE;  // CFVx - Clock divider not set
   case FLASH_ERR_ILLEGAL_SECURITY  : return PROGRAMMING_RC_ERROR_ILLEGAL_SECURITY;      // CFV1+,Kinetis - Illegal value for security location
   case FLASH_ERR_UNKNOWN           : return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED; // Unspecified error
   case FLASH_ERR_TIMEOUT           : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;  // Unspecified error
   default                          : return PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND;
   }
}

//=======================================================================
#if (TARGET == CFVx)
static bool usePSTSignals = false;
//! Check DSC run status
//!
//! @return BDM_RC_OK   - halted\n
//!         BDM_RC_BUSY - running\n
//!         other       - error
//!
USBDM_ErrorCode getRunStatus(void) {
   USBDMStatus_t USBDMStatus;
   USBDM_ErrorCode rc = USBDM_GetBDMStatus(&USBDMStatus);
   if (rc != BDM_RC_OK) {
      print("getRunStatus() - Failed, rc=%s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   if (usePSTSignals) {
      // Check processor state using PST signals
      if (USBDMStatus.halt_state == TARGET_RUNNING) {
		   return BDM_RC_BUSY;
      }
      else {
         return BDM_RC_OK;
      }
   }
   else {
      // Probe D0 register - if fail assume processor running!
      unsigned long int dummy;
      rc = USBDM_ReadReg(CFVx_RegD0, &dummy);
      if (rc == BDM_RC_OK) {
         // Processor halted
         return BDM_RC_OK;
      }
      else /* if (rc == BDM_RC_CF_BUS_ERROR) */{
         // Processor executing
         return BDM_RC_BUSY;
      }
   }
}
#endif

//=======================================================================
#if (TARGET == MC56F80xx)
//! Check DSC run status
//!
//! @return BDM_RC_OK   - halted\n
//!         BDM_RC_BUSY - running\n
//!         other       - error
//!
USBDM_ErrorCode getRunStatus(void) {
   OnceStatus_t onceStatus;
   USBDM_ErrorCode rc = DSC_GetStatus(&onceStatus);
   if (rc != BDM_RC_OK) {
      print("getRunStatus() - Failed, rc=%s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   switch (onceStatus) {
   case stopMode:
   case debugMode:
   case unknownMode:
   default:
      return BDM_RC_OK;
   case executeMode :
   case externalAccessMode:
      return BDM_RC_BUSY;
   }
}
#endif

#if (TARGET == ARM)
void report(const char *msg) {
   ArmStatus status;
   ARM_GetStatus(&status);
   uint8_t buff[10];
   if (ARM_ReadMemory(2,2,0x40052000,buff) == BDM_RC_OK) {
      print("%s::report() - WDOG_STCTRLH=0x%02X\n", msg, getData16Target(buff));
   }
   else {
      print("%s::report() - Unable to read WDOG_STCTRLHL\n", msg);
   }
   uint8_t mc_srsh;
   uint8_t mc_srsl;
   // Assume if we can read SRS locations then they are valid (Kinetis chip)
   if ((ARM_ReadMemory(1,1, MC_SRSH, &mc_srsh) == BDM_RC_OK) &&
       (ARM_ReadMemory(1,1, MC_SRSL, &mc_srsl) == BDM_RC_OK)) {
      print("%s::report() - MC_SRSH=0x%02X, MC_SRSL=0x%02X \n", msg, mc_srsh, mc_srsl);
   }
   else {
      print("%s::report() - Unable to read MC_SRSH,MC_SRSL\n", msg);
   }
}
#endif

USBDM_ErrorCode FlashProgrammer::initLargeTargetBuffer(memoryElementType *buffer) {
   LargeTargetFlashDataHeader *pFlashHeader = (LargeTargetFlashDataHeader*)buffer;

   pFlashHeader->errorCode       = nativeToTarget16(-1);
   pFlashHeader->controller      = nativeToTarget32(flashOperationInfo.controller);
//   pFlashHeader->watchdogAddress = nativeToTarget32(parameters.getSOPTAddress());
   pFlashHeader->frequency       = nativeToTarget32(flashOperationInfo.targetBusFrequency);
   pFlashHeader->sectorSize      = nativeToTarget16(flashOperationInfo.sectorSize);
   pFlashHeader->address         = nativeToTarget32(flashOperationInfo.flashAddress);
   pFlashHeader->dataSize        = nativeToTarget32(flashOperationInfo.dataSize);
   pFlashHeader->dataAddress     = nativeToTarget32(targetProgramInfo.headerAddress+targetProgramInfo.dataOffset);

   uint32_t operation = 0;
   switch(currentFlashOperation) {
   case OpNone:
   case OpWriteRam:
   default:
      print("FlashProgrammer::initLargeTargetBuffer() - unexpected operation %s\n", getFlashOperationName(currentFlashOperation));
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   case OpSelectiveErase:
      operation = DO_INIT_FLASH|DO_ERASE_RANGE;
      break;
   case OpProgram:
      operation = DO_INIT_FLASH|targetProgramInfo.programOperation;
      break;
   case OpVerify:
      operation = DO_INIT_FLASH|DO_VERIFY_RANGE;
      break;
   case OpBlankCheck:
      operation = DO_INIT_FLASH|DO_BLANK_CHECK_RANGE;
      break;
   case OpBlockErase:
      operation = DO_INIT_FLASH|DO_ERASE_BLOCK;
      break;
#if (TARGET == MC56F80xx) || (TARGET == HCS12) || (TARGET == CFVx)
   case OpTiming:
      operation = DO_TIMING_LOOP;
      break;
#endif
#if (TARGET==ARM) || (TARGET==CFV1)
   case OpPartitionFlexNVM:
      operation = DO_INIT_FLASH|DO_PARTITION_FLEXNVM;
      // Frequency field used for partition information
      pFlashHeader->frequency = nativeToTarget32(flashOperationInfo.flexNVMPartition);
      break;
#endif
   }
   pFlashHeader->flags = nativeToTarget32(operation);

   print("FlashProgrammer::initLargeTargetBuffer() - flashOperationInfo.flashAddress = 0x%08X\n", flashOperationInfo.flashAddress);
   print("FlashProgrammer::initLargeTargetBuffer() - pFlashHeader->address = 0x%08X\n", pFlashHeader->address);

   print("FlashProgrammer::initLargeTargetBuffer()\n"
         "   currentFlashOperation  = %s\n"
         "   flags                  = %s\n"
         "   controller             = 0x%08X\n"
//         "   watchdogAddress        = 0x%08X\n"
         "   frequency              = %d (0x%X)\n"
         "   sectorSize             = 0x%04X\n"
         "   address                = 0x%08X\n"
         "   dataSize               = 0x%08X\n"
         "   dataAddress            = 0x%08X\n",
         getFlashOperationName(currentFlashOperation),
         getProgramActionNames(targetToNative32(pFlashHeader->flags)),
         targetToNative32(pFlashHeader->controller),
//         targetToNative32(pFlashHeader->watchdogAddress),
         targetToNative32(pFlashHeader->frequency),targetToNative32(pFlashHeader->frequency),
         targetToNative16(pFlashHeader->sectorSize),
         targetToNative32(pFlashHeader->address),
         targetToNative32(pFlashHeader->dataSize),
         targetToNative32(pFlashHeader->dataAddress)
         );
   return BDM_RC_OK;
}

USBDM_ErrorCode FlashProgrammer::initSmallTargetBuffer(memoryElementType *buffer) {
   return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
}

//=======================================================================
//! \brief Executes program on target.
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @param pBuffer  - buffer including space for header describing operation (may be NULL)
//! @param dataSize - size of data following header in memoryElementType units
//!
USBDM_ErrorCode FlashProgrammer::executeTargetProgram(memoryElementType *pBuffer, uint32_t dataSize) {

   print("FlashProgrammer::executeTargetProgram(..., dataSize=0x%X)\n", dataSize);

   USBDM_ErrorCode rc = BDM_RC_OK;
   memoryElementType buffer[1000];
   if (pBuffer == NULL) {
      if (dataSize != 0) {
         print("FlashProgrammer::executeTargetProgram() - Error: No buffer but size non-zero\n");
         return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
      }
      pBuffer = buffer;
   }
   if (targetProgramInfo.smallProgram) {
      rc = initSmallTargetBuffer(pBuffer);
   }
   else {
      rc = initLargeTargetBuffer(pBuffer);
   }
   if (rc != BDM_RC_OK) {
      return rc;
   }
#if defined(LOG) && (TARGET==ARM)
   report("FlashProgrammer::executeTargetProgram()");
#endif
   print("FlashProgrammer::executeTargetProgram() - Writing Header+Data\n");

#if (TARGET==RS08) ||(TARGET==HCS08) || (TARGET==HCS12)
   MemorySpace_t memorySpace = MS_Byte;
#elif (TARGET == MC56F80xx)
   MemorySpace_t memorySpace = MS_XWord;
#else
   MemorySpace_t memorySpace = MS_Word;
#endif

   // Write the flash parameters & data to target memory
   if (WriteMemory(memorySpace,
                   (targetProgramInfo.dataOffset+dataSize)*sizeof(memoryElementType),
                   targetProgramInfo.headerAddress,
                   (uint8_t *)pBuffer) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Set target PC to start of code & verify
   long unsigned targetRegPC;
   if (WritePC(targetProgramInfo.entry&~0x1) != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - PC write failed\n");
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   if (ReadPC(&targetRegPC) != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - PC read failed\n");
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   if ((targetProgramInfo.entry&~0x1) != targetRegPC) {
      print("FlashProgrammer::executeTargetProgram() - PC verify failed\n");
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   ArmStatus status;
   ARM_GetStatus(&status);
#if defined(LOG) && 0
   for (int num=0; num<1000; num++) {
      USBDM_ErrorCode rc = ARM_TargetStep();
      if (rc != BDM_RC_OK) {
         print("FlashProgrammer::executeTargetProgram() - TargetStep() Failed, rc=%s\n",
               USBDM_GetErrorString(rc));
         return rc;
      }
      unsigned long currentPC;
      rc = ReadPC(&currentPC);
      if (rc != BDM_RC_OK) {
         print("FlashProgrammer::executeTargetProgram() - ReadPC() Failed, rc=%s\n",
               USBDM_GetErrorString(rc));
         report("FlashProgrammer::executeTargetProgram()");
         return rc;
      }
      if ((pcValue<(targetRegPC-0x1000))||(pcValue>(targetRegPC+0x1000))) {
         print("FlashProgrammer::executeTargetProgram() - Read PC out of range, PC=0x%08X\n",
               pcValue);
         report("FlashProgrammer::executeTargetProgram()");
         return PROGRAMMING_RC_ERROR_BDM;
      }
      uint8_t  iBuffer[8];
      rc = ReadMemory(1, sizeof(iBuffer), currentPC, (uint8_t *)iBuffer);
      if (rc != BDM_RC_OK) {
         print("FlashProgrammer::executeTargetProgram() - ReadMemory() Failed, rc=%s\n",
               USBDM_GetErrorString(rc));
         report("FlashProgrammer::executeTargetProgram()");
         return rc;
      }
      print("FlashProgrammer::executeTargetProgram() - Step: PC=0x%06X => %02X %02X %02X %02X\n",
             currentPC, iBuffer[3], iBuffer[2], iBuffer[1], iBuffer[0]);
   }
#endif
   // Execute the Flash program on target
   if (TargetGo() != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - TargetGo() failed\n");
      return PROGRAMMING_RC_ERROR_BDM;
   }
   progressTimer->progress(0, NULL);
#ifdef LOG
   print("FlashProgrammer::executeTargetProgram() - Polling");
   int dotCount = 50;
#endif
   // Wait for target stop at execution completion
   int timeout = 400; // x 10 ms
   do {
      milliSleep(10);
#ifdef LOG
      print(".");
      if (++dotCount == 100) {
         print("\n");
         dotCount = 0;
      }
#endif
      if (ARM_GetStatus(&status) != BDM_RC_OK) {
         TargetHalt();
         ReadPC( &targetRegPC); // For debug
         return PROGRAMMING_RC_ERROR_BDM_READ;
      }
      if ((status.dhcsr & (DHCSR_S_HALT|DHCSR_S_LOCKUP)) != 0) {
         break;
      }
   } while (--timeout>0);
   TargetHalt();
   unsigned long value;
   ReadPC(&value);
   print("\nFlashProgrammer::executeTargetProgram() - Start PC = 0x%08X, end PC = 0x%08X\n", targetRegPC, value);
   // Read the flash parameters back from target memory
   ResultStruct executionResult;
   if (ReadMemory(memorySpace, sizeof(ResultStruct),
                  targetProgramInfo.headerAddress,
                  (uint8_t*)&executionResult) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   uint16_t errorCode = targetToNative16(executionResult.errorCode);
   if ((timeout <= 0) && (errorCode == FLASH_ERR_OK)) {
      errorCode = FLASH_ERR_TIMEOUT;
      print("FlashProgrammer::executeTargetProgram() - Error, Timeout waiting for completion.\n");
   }
   if (targetProgramInfo.smallProgram) {
      print("FlashProgrammer::executeTargetProgram() - complete, errCode=%d\n", errorCode);
   }
   else {
      uint32_t flags = targetToNative32(executionResult.flags);
      if ((flags != IS_COMPLETE) && (errorCode == FLASH_ERR_OK)) {
         errorCode = FLASH_ERR_UNKNOWN;
         print("FlashProgrammer::executeTargetProgram() - Error, Unexpected flag result.\n");
      }
      print("FlashProgrammer::executeTargetProgram() - complete, flags = 0x%08X(%s), errCode=%d\n",
            flags, getProgramActionNames(flags),
            errorCode);
   }
   rc = convertTargetErrorCode((FlashDriverError_t)errorCode);
   if (rc != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - Error - %s\n", USBDM_GetErrorString(rc));
#if (TARGET == MC56F80xx) && 0
      executionResult.data = targetToNative16(executionResult.data);
      executionResult.dataSize = targetToNative16(executionResult.dataSize);
      print("   Address = 0x%06X, Data = 0x%04X\n", executionResult.data, executionResult.dataSize);
#endif
#if TARGET == CFV1
      uint8_t SRSreg;
      USBDM_ReadMemory(1, 1, 0xFF9800, &SRSreg);
      print("FlashProgrammer::executeTargetProgram() - SRS = 0x%02X\n", SRSreg);
#endif
#if TARGET == HCS08
      uint8_t SRSreg;
      USBDM_ReadMemory(1, 1, 0x1800, &SRSreg);
      print("FlashProgrammer::executeTargetProgram() - SRS = 0x%02X\n", SRSreg);
#endif
   }
#if defined(LOG) && 0
   uint8_t stackBuffer[50];
   ReadMemory(memorySpace, sizeof(stackBuffer), 0x3F0, stackBuffer);
#endif
   return rc;
}

#if (TARGET == CFVx) || (TARGET == HCS12) || (TARGET == MC56F80xx)
//=======================================================================
//! \brief Determines the target execution speed
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::determineTargetSpeed(void) {

   print("FlashProgrammer::determineTargetSpeed()\n");

   uint32_t targetBusFrequency = 0;

   flashOperationInfo.alignment = 1;
   flashOperationInfo.dataSize  = 0;

   // Load flash programming code to target
   USBDM_ErrorCode rc = loadTargetProgram(OpTiming);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   LargeTargetTimingDataHeader timingData = {0};
   timingData.flags      = nativeToTarget32(DO_TIMING_LOOP|IS_COMPLETE); // IS_COMPLETE as check - should be cleared
   timingData.controller = nativeToTarget32(-1);                         // Dummy value - not used

   print("FlashProgrammer::determineTargetSpeed()\n"
         "   flags      = 0x%08X(%s)\n",
         targetToNative32(timingData.flags), getProgramActionNames(targetToNative32(timingData.flags))
         );
   MemorySpace_t memorySpace = MS_Word;
   // Write the flash parameters & data to target memory
   if (WriteMemory(memorySpace, sizeof(LargeTargetTimingDataHeader),
                   targetProgramInfo.headerAddress, (uint8_t*)&timingData) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Set target PC to start of code & verify
   if (WritePC(targetProgramInfo.entry) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Execute the Flash program on target for 1 second
   if (TargetGo() != BDM_RC_OK) {
      print("FlashProgrammer::determineTargetSpeed() - USBDM_TargetGo failed\n");
      return PROGRAMMING_RC_ERROR_BDM;
   }
   milliSleep(1000);
   if (TargetHalt() != BDM_RC_OK) {
      print("FlashProgrammer::determineTargetSpeed() - USBDM_TargetHalt failed\n");
      return PROGRAMMING_RC_ERROR_BDM;
   }
   // Read the flash parameters back from target memory
   LargeTargetTimingDataHeader timingDataResult;
   if (ReadMemory(memorySpace, sizeof(timingDataResult),
                  targetProgramInfo.headerAddress, (uint8_t*)&timingDataResult) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   timingDataResult.flags        = targetToNative32(timingDataResult.flags);
   timingDataResult.errorCode    = targetToNative16(timingDataResult.errorCode);
   timingDataResult.timingCount  = targetToNative32(timingDataResult.timingCount);

   print("FlashProgrammer::determineTargetSpeed() - complete, flags = 0x%08X(%s), errCode=%d\n",
         timingDataResult.flags,
         getProgramActionNames(timingDataResult.flags),
         timingDataResult.errorCode);
   unsigned long value;
   ReadPC(&value);
   print("\nFlashProgrammer::determineTargetSpeed() - Start PC = 0x%08X, end PC = 0x%08X\n", targetProgramInfo.entry, value);
   if ((timingDataResult.flags != DO_TIMING_LOOP) && (timingDataResult.errorCode == FLASH_ERR_OK)) {
      timingDataResult.errorCode    = FLASH_ERR_UNKNOWN;
      print("FlashProgrammer::determineTargetSpeed() - Error, Unexpected flag result\n");
   }
   if (timingDataResult.errorCode != FLASH_ERR_OK) {
      print("FlashProgrammer::determineTargetSpeed() - Error\n");
      return convertTargetErrorCode((FlashDriverError_t)timingDataResult.errorCode);
   }
   targetBusFrequency = 50*int(0.5+((250.0 * timingDataResult.timingCount)/targetProgramInfo.calibFactor));
   flashOperationInfo.targetBusFrequency = targetBusFrequency;
   print("FlashProgrammer::determineTargetSpeed() - Count = %d(0x%X) => Bus Frequency = %d kHz\n",
         timingDataResult.timingCount, timingDataResult.timingCount, targetBusFrequency);
   return PROGRAMMING_RC_OK;
}
#endif

//=======================================================================
//! Erase Target Flash memory
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
USBDM_ErrorCode FlashProgrammer::eraseFlash(void) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   print("FlashProgrammer::eraseFlash()\n");
   progressTimer->restart("Erasing all flash blocks...");

   // Process each flash region
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      if (!memoryRegionPtr->isProgrammableMemory()) {
         continue;
      }
      MemType_t memoryType = memoryRegionPtr->getMemoryType();
      print("FlashProgrammer::eraseFlash() - Erasing %s\n", MemoryRegion::getMemoryTypeName(memoryType));

      uint32_t addressFlag = 0;

#if (TARGET == HCS08) || (TARGET == HCS12)
      if (memoryRegionPtr->getAddressType() == AddrLinear) {
         addressFlag |= ADDRESS_LINEAR;
      }
      if (memoryRegionPtr->getMemoryType() == MemEEPROM) {
         addressFlag |= ADDRESS_EEPROM;
      }
#endif
#if (TARGET == MC56F80xx)
      if (memoryType == MemXROM) {
         // |0x80 => XROM, |0x03 => Bank1 (Data)
         addressFlag |= 0x83000000;
      }
#endif      
#if (TARGET == CFV1) || (TARGET == ARM)
      if ((memoryType == MemFlexNVM) || (memoryType == MemDFlash)) {
         // Flag need for DFLASH/flexNVM access
         addressFlag |= (1<<23);
      }
#endif
      flashOperationInfo.controller        = memoryRegionPtr->getRegisterAddress();
      flashOperationInfo.flashAddress      = memoryRegionPtr->getDummyAddress()|addressFlag;
      flashOperationInfo.sectorSize        = memoryRegionPtr->getSectorSize();
      flashOperationInfo.alignment         = memoryRegionPtr->getAlignment();
      flashOperationInfo.pageAddress       = memoryRegionPtr->getPageAddress();
      flashOperationInfo.dataSize          = 0;
      flashOperationInfo.flexNVMPartition  = (uint32_t)-1;

      const FlashProgramPtr flashProgram = memoryRegionPtr->getFlashprogram();
      rc = loadTargetProgram(flashProgram, OpBlockErase);
      if (rc != PROGRAMMING_RC_OK) {
         print("FlashProgrammer::eraseFlash() - loadTargetProgram() failed \n");
         return rc;
      }
      rc = executeTargetProgram();
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
   }
   return rc;
}

#if (TARGET == CFV1) || (TARGET == ARM) || (TARGET == HCS08)
//=======================================================================
//! Selective erases the target memory security areas.
//! This is only of use when the target is unsecured but the security
//! needs to be modified.
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
USBDM_ErrorCode FlashProgrammer::selectiveEraseFlashSecurity(void) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   print("FlashProgrammer::selectiveEraseFlashSecurity()\n");
   progressTimer->restart("Erasing all flash security areas...");

   // Process each flash region
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      if (!memoryRegionPtr->isProgrammableMemory()) {
         continue;
      }
      uint32_t securityAddress = memoryRegionPtr->getSecurityAddress();
      if (securityAddress == 0) {
         continue;
      }
      SecurityInfoPtr securityInfo = memoryRegionPtr->getSecureInfo();
      uint32_t securitySize = securityInfo->getSize();
      if (securityInfo == NULL) {
         continue;
      }
      print("FlashProgrammer::selectiveEraseFlashSecurity() - erasing security area %s[0x%06X..0x%06X]\n",
            memoryRegionPtr->getMemoryTypeName(), securityAddress, securityAddress+securitySize-1);

      flashOperationInfo.controller        = memoryRegionPtr->getRegisterAddress();
      flashOperationInfo.sectorSize        = memoryRegionPtr->getSectorSize();
      flashOperationInfo.alignment         = memoryRegionPtr->getAlignment();
      flashOperationInfo.pageAddress       = memoryRegionPtr->getPageAddress();
      flashOperationInfo.dataSize          = securitySize;
      flashOperationInfo.flexNVMPartition  = (uint32_t)-1;

      const FlashProgramPtr flashProgram = memoryRegionPtr->getFlashprogram();
      rc = loadTargetProgram(flashProgram, OpSelectiveErase);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
      uint32_t addressFlag = 0;

#if (TARGET == HCS08) || (TARGET == HCS12)
      if (memoryRegionPtr->getAddressType() == AddrLinear) {
         addressFlag |= ADDRESS_LINEAR;
      }
      if (memoryRegionPtr->getMemoryType() == MemEEPROM) {
         addressFlag |= ADDRESS_EEPROM;
      }
#endif
#if (TARGET == MC56F80xx)
      MemType_t memoryType = memoryRegionPtr->getMemoryType();
      if (memoryType == MemXROM) {
         // |0x80 => XROM, |0x03 => Bank1 (Data)
         addressFlag |= 0x83000000;
      }
#endif
#if (TARGET == CFV1) || (TARGET == ARM)
      MemType_t memoryType = memoryRegionPtr->getMemoryType();
      if ((memoryType == MemFlexNVM) || (memoryType == MemDFlash)) {
         // Flag need for DFLASH/flexNVM access
         addressFlag |= (1<<23);
      }
#endif
      flashOperationInfo.flashAddress = securityAddress|addressFlag;
      if (flashOperationInfo.sectorSize == 0) {
         return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
      }
      rc = executeTargetProgram();
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
   }
   return rc;
}
#endif

#if (TARGET == CFV1) || (TARGET == ARM)
//=======================================================================
//! Program FlashNVM partion (DFlash/EEPROM backing store)
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::partitionFlexNVM() {
   uint8_t eeepromSize  = parameters.getFlexNVMParameters()->eeepromSize;
   uint8_t partionValue = parameters.getFlexNVMParameters()->partionValue;
   USBDM_ErrorCode rc = BDM_RC_OK;
   if ((eeepromSize==0xFF)&&(partionValue==0xFF)) {
      print("FlashProgrammer::programPartition() - skipping FlexNVM parameter programming as unprogrammed values\n");
      return BDM_RC_OK;
   }
   print("FlashProgrammer::programPartition(eeepromSize=0x%02X, partionValue=0x%02X)\n", eeepromSize, partionValue);
   progressTimer->restart("Partitioning DFlash...");

   // Find flexNVM region
   MemoryRegionPtr memoryRegionPtr;
   for (int index=0; ; index++) {
      memoryRegionPtr = parameters.getMemoryRegion(index);
      if ((memoryRegionPtr == NULL) ||
          (memoryRegionPtr->getMemoryType() == MemFlexNVM)) {
         break;
      }
   }
   if (memoryRegionPtr == NULL) {
      print("FlashProgrammer::programPartition() - No FlexNVM Region found\n");
      return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   }
   MemType_t memoryType = memoryRegionPtr->getMemoryType();
   print("FlashProgrammer::programPartition() - Partitioning %s\n", MemoryRegion::getMemoryTypeName(memoryType));
   const FlashProgramPtr flashProgram = memoryRegionPtr->getFlashprogram();
   rc = loadTargetProgram(flashProgram, OpPartitionFlexNVM);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   flashOperationInfo.flexNVMPartition  = (eeepromSize<<24UL)|(partionValue<<16UL);
   rc = executeTargetProgram();
   if (rc == PROGRAMMING_RC_ERROR_FAILED_FLASH_COMMAND) {
      // This usually means this error - more useful message
      rc = PROGRAMMING_RC_FLEXNVM_CONFIGURATION_FAILED;
   }
   return rc;
}
#endif

#if (TARGET == HCS12)
//=======================================================================
//! Checks for unsupported device
//!
USBDM_ErrorCode FlashProgrammer::checkUnsupportedTarget() {
   USBDM_ErrorCode rc;
   uint32_t targetSDID;
#define brokenUF32_SDID (0x6311)
//#define brokenUF32_SDID (0x3102) // for testing using C128

   // Get SDID from target
   rc = readTargetChipId(&targetSDID);
   if (rc != PROGRAMMING_RC_OK)
      return rc;

   // It's fatal to try unsecuring this chip using the BDM or
   // unsafe to program it to the secure state.
   // See errata MUCts01498
   if ((targetSDID == brokenUF32_SDID) &&
       ((parameters.getSecurity() != SEC_UNSECURED) ||
        (checkTargetUnSecured() != PROGRAMMING_RC_OK))) {
      print("FlashProgrammer::checkUnsupportedTarget() - Can't unsecure/secure UF32 due to hardware bug - See errata MUCts01498\n");
      return PROGRAMMING_RC_ERROR_CHIP_UNSUPPORTED;
   }
   return PROGRAMMING_RC_OK;
}
#endif

#if (TARGET == MC56F80xx) || (TARGET == HCS12)
//==================================================================================
//! Determines the target frequency by either of these methods: \n
//!   -  BDM SYNC Timing \n
//!   -  Execution of a timing program on target (only if unsecured & no SYNC)
//!
//! @param busFrequency  : Target bus frequency (in Hz)
//!
//! @return error code, see \ref USBDM_ErrorCode \n
//!      - PROGRAMMING_RC_OK                 - speed accurately determined \n
//!      - PROGRAMMING_RC_ERROR_SPEED_APPROX - speed estimated (not suitable for programming) \n
//!      - PROGRAMMING_RC_ERROR_FAILED_CLOCK - speed timing program failed
//!
//! @note - Assumes the target has been initialised. \n
//!       - Re-connects to target.
//!
USBDM_ErrorCode FlashProgrammer::getTargetBusSpeed(unsigned long *busFrequency) {
unsigned long connectionFrequency;
USBDMStatus_t bdmStatus;
USBDM_ErrorCode rc;

//   print("FlashProgrammer::getTargetBusSpeed()\n");

   // Check target connection
   // BDM_RC_BDM_EN_FAILED may mean the target is secured
   rc = USBDM_Connect();
   if (((rc != BDM_RC_OK) && (rc != BDM_RC_BDM_EN_FAILED)) ||
       (USBDM_GetBDMStatus(&bdmStatus) != BDM_RC_OK)) {
      print("FlashProgrammer::getTargetBusSpeed()- failed connection\n");
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }

   // If BDM SYNC worked then use that speed
   if ((bdmStatus.connection_state == SPEED_SYNC) &&
       (USBDM_GetSpeedHz(&connectionFrequency) == BDM_RC_OK)) {
      // Use speed determined by BDM SYNC pulse
      *busFrequency = parameters.getBDMtoBUSFactor()*connectionFrequency;
      print("FlashProgrammer::getTargetBusSpeed() - Using SYNC method, Bus Frequency = %ld kHz\n",
            (unsigned long)round(*busFrequency/1000.0));
      return PROGRAMMING_RC_OK;
   }

   // We can only approximate the target speed if secured & guessed
   if ((checkTargetUnSecured() == PROGRAMMING_RC_ERROR_SECURED) &&
       (bdmStatus.connection_state == SPEED_GUESSED) &&
       (USBDM_GetSpeedHz(&connectionFrequency) == BDM_RC_OK)) {
      // Use speed determined by BDM guessing
      *busFrequency = parameters.getBDMtoBUSFactor()*connectionFrequency;
      print("FlashProgrammer::getTargetBusSpeed() - Using Approximate method, Bus Frequency = %ld kHz\n",
            (unsigned long)round(*busFrequency/1000.0));
      return PROGRAMMING_RC_ERROR_SPEED_APPROX;
   }
   // We must have a connection for the next method
   if (rc != BDM_RC_OK) {
      print("FlashProgrammer::getTargetBusSpeed()- failed connection\n");
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }
   //
   // Try to determine target speed by down-loading a timing program to the target
   //
   USBDM_ErrorCode flashRc = determineTargetSpeed();
   if (flashRc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::getTargetBusSpeed()- failed connection\n");
      return flashRc;
   }
   *busFrequency = 1000*flashOperationInfo.targetBusFrequency;

   print("FlashProgrammer::getTargetBusSpeed() - Using Timing Program method, Bus Frequency = %ld kHz\n",
        (unsigned long)round(*busFrequency/1000.0));

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
//! Applies a flash operation to a block of Target Flash memory
//!
//! @param flashImage       Description of flash contents to be processed.
//! @param blockSize        Size of block to process (bytes)
//! @param flashAddress     Start address of block in flash to process
//! @param flashOperation   Operation to do on flash
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//! @note - The memory range must be within one page for paged devices.
//!
USBDM_ErrorCode FlashProgrammer::doFlashBlock(FlashImage     *flashImage,
                                              unsigned int    blockSize,
                                              uint32_t       &flashAddress,
                                              FlashOperation flashOperation) {

   print("FlashProgrammer::doFlashBlock(%s, [0x%06X..0x%06X])\n",
         getFlashOperationName(flashOperation), flashAddress, flashAddress+blockSize-1);

   if (!flashReady) {
      print("FlashProgrammer::doFlashBlock() - Error, Flash not ready\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // OK for empty block
   if (blockSize==0) {
      return PROGRAMMING_RC_OK;
   }
   //
   // Find flash region to program - this will recurse to handle sub regions
   //
   MemorySpace_t memorySpace = MS_None;
   uint32_t      offset      = 0;
#if (TARGET == MC56F80xx)
   if (flashAddress >= FlashImage::DataOffset) {
      memorySpace = MS_Data;
      offset      = FlashImage::DataOffset;
   }
   else {
      memorySpace = MS_Program;
      offset      = 0;
   }
#endif
   MemoryRegionPtr memoryRegionPtr;
   for (int index=0; ; index++) {
      memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         print("FlashProgrammer::doFlashBlock() - Block not within target memory\n");
         print("  %s[0x%06X...] is not within target memory (checked %d regions).\n",
               getMemSpaceName(memorySpace), flashAddress, index);
         return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
      }
      print("  Checking %s[0x%06X..0x%06X]\n",
            memoryRegionPtr->getMemoryTypeName(),
            memoryRegionPtr->getMemoryRange(0)->start,
            memoryRegionPtr->getMemoryRange(0)->end);
      uint32_t lastContiguous;
      if (memoryRegionPtr->findLastContiguous((flashAddress-offset), &lastContiguous, memorySpace)) {
         lastContiguous = lastContiguous + offset; // Convert to last byte
         // Check if programmable
         if (!memoryRegionPtr->isProgrammableMemory()) {
               print("FlashProgrammer::doFlashBlock() - Block not programmable memory\n");
               return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
         }
         // Check if block crosses boundary and will need to be split
         if ((flashAddress+blockSize-1) > lastContiguous) {
            print("FlashProgrammer::doFlashBlock() - Block crosses FLASH boundary - recursing\n");
            uint32_t firstBlockSize = lastContiguous - flashAddress + 1;
            USBDM_ErrorCode rc;
            rc = doFlashBlock(flashImage, firstBlockSize, flashAddress, flashOperation);
            if (rc != PROGRAMMING_RC_OK) {
               return rc;
            }
            rc = doFlashBlock(flashImage, blockSize-firstBlockSize, flashAddress, flashOperation);
            return rc;
         }
         else {
            // Check if programmable
            if (!memoryRegionPtr->isProgrammableMemory()) {
               if (((memoryRegionPtr->getMemoryType() == MemRAM) ||
                     (memoryRegionPtr->getMemoryType() == MemPRAM) ||
                     (memoryRegionPtr->getMemoryType() == MemXRAM)) && doRamWrites) {
                  if (flashOperation == OpWriteRam) {
                     break;
                  }
                  else {
                     // Ignore block
                     print("FlashProgrammer::doFlashBlock() - Skipping block %s[0x%06X..0x%06X]\n",
                           memoryRegionPtr->getMemoryTypeName(),
                           flashAddress-offset, flashAddress-offset+blockSize-1);
                     flashAddress += blockSize;
                     return PROGRAMMING_RC_OK;
                  }
               }
               print("FlashProgrammer::doFlashBlock() - Block is not located in programmable memory\n");
               print("  %s[0x%06X..0x%06X] is not programmable.\n",
                     memoryRegionPtr->getMemoryTypeName(), flashAddress, lastContiguous+offset);
               return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
            }
         }
         break;
      }
   }
   MemType_t memoryType = memoryRegionPtr->getMemoryType();
   print("FlashProgrammer::doFlashBlock() - Processing %s[0x%06X..0x%06X]\n",
         MemoryRegion::getMemoryTypeName(memoryType),
         flashAddress, flashAddress+blockSize-1);

   flashOperationInfo.controller = memoryRegionPtr->getRegisterAddress();
   flashOperationInfo.alignment  = memoryRegionPtr->getAlignment();
   flashOperationInfo.sectorSize = memoryRegionPtr->getSectorSize();

   if (flashOperationInfo.sectorSize == 0) {
      print("FlashProgrammer::doFlashBlock() - Error: sector size is 0\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   USBDM_ErrorCode rc = loadTargetProgram(memoryRegionPtr->getFlashprogram(), flashOperation);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Maximum split block size must be made less than buffer RAM available
   unsigned int maxSplitBlockSize = targetProgramInfo.maxDataSize;

   const unsigned int MaxSplitBlockSize = 0x4000;
   memoryElementType  buffer[MaxSplitBlockSize+50];
   memoryElementType *bufferData = buffer+targetProgramInfo.dataOffset;

   // Maximum split block size must be made less than buffer size
   if (maxSplitBlockSize > MaxSplitBlockSize) {
      maxSplitBlockSize = MaxSplitBlockSize;
   }
   uint32_t alignMask = memoryRegionPtr->getAlignment()-1;
   print("FlashProgrammer::doFlashBlock() - align mask = 0x%08X\n", alignMask);

   // splitBlockSize must be aligned
   maxSplitBlockSize &= ~alignMask;

   // Calculate any odd padding bytes at start of block
   unsigned int oddBytes = flashAddress & alignMask;

   uint32_t addressFlag = 0;

#if (TARGET == HCS08) || (TARGET == HCS12)
   if (memoryRegionPtr->getMemoryType() == MemEEPROM) {
      print("FlashProgrammer::doFlashBlock() - setting EEPROM address flag\n");
      addressFlag |= ADDRESS_EEPROM;
   }
   // Set up linear address
   if (memoryRegionPtr->getAddressType() == AddrLinear) {
      // Set Linear address
      print("FlashProgrammer::doFlashBlock() - setting Linear address\n");
      addressFlag |= ADDRESS_LINEAR;
   }
#endif
#if (TARGET == MC56F80xx)
      if (memoryType == MemXROM) {
         // |0x80 => XROM, |0x03 => Bank1 (Data)
         print("FlashProgrammer::doFlashBlock() - setting MemXROM address\n");
         addressFlag = 0x83000000;
      }
#endif
#if (TARGET == CFV1) || (TARGET == ARM)
      if ((memoryType == MemFlexNVM) || (memoryType == MemDFlash)) {
         // Flag need for DFLASH/flexNVM access
         addressFlag |= (1<<23);
      }
#endif
   // Round start address off to alignment requirements
   flashAddress  &= ~alignMask;

   // Pad block size with odd leading bytes
   blockSize += oddBytes;

   // Pad block size to alignment requirements
   blockSize = (blockSize+alignMask)&~alignMask;

   progressTimer->progress(0, NULL);

   while (blockSize>0) {
      unsigned flashIndex  = 0;
      unsigned size        = 0;
      // Determine size of block to process
      unsigned int splitBlockSize = blockSize;
      if ((flashOperation == OpProgram)||(flashOperation == OpVerify)) {
//         print("FlashProgrammer::doFlashBlock() #2  maxSplitBlockSize=0x%06X, splitBlockSize=0x%06X, blockSize=0x%X\n", maxSplitBlockSize, splitBlockSize, blockSize);
         // Requires data transfer using buffer
         if (splitBlockSize>maxSplitBlockSize) {
            splitBlockSize = maxSplitBlockSize;
         }
         // Pad any odd leading elements as 0xFF..
         for (flashIndex=0; flashIndex<oddBytes; flashIndex++) {
            bufferData[flashIndex] = (memoryElementType)-1;
         }
         // Copy flash data to buffer
         for(flashIndex=0; flashIndex<splitBlockSize; flashIndex++) {
            bufferData[flashIndex] = flashImage->getValue(flashAddress+flashIndex);
         }
         // Pad trailing elements to aligned address
         for (; (flashIndex&alignMask) != 0; flashIndex++) {
            bufferData[flashIndex] = flashImage->getValue(flashAddress+flashIndex);
         }
         // Actual data bytes to write
         size = flashIndex;
      }
      else {
         // No data transfer so no size limits
         flashIndex  = (blockSize+alignMask)&~alignMask;
         size = 0;
      }
      uint32_t targetAddress = flashAddress;

#if (TARGET == HCS08) || (TARGET == HCS12)
      // Map paged address
      if (memoryRegionPtr->getAddressType() != AddrLinear) {
         uint8_t pageNo;
         rc = getPageAddress(memoryRegionPtr, flashAddress, &pageNo);
         if (rc != PROGRAMMING_RC_OK) {
            return rc;
         }
         targetAddress = (pageNo<<16)|(flashAddress&0xFFFF);
      }
#endif
      flashOperationInfo.flashAddress = addressFlag|targetAddress;
      flashOperationInfo.dataSize     = flashIndex;
      if (splitBlockSize==0) {
         print("FlashProgrammer::doFlashBlock() - Error: splitBlockSize size is 0\n");
         return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
      }
      flashOperationInfo.pageAddress  = memoryRegionPtr->getPageAddress();
      USBDM_ErrorCode rc;
      if (flashOperation == OpWriteRam) {
         print("         ramBlock[0x%06X..0x%06X]\n", flashAddress, flashAddress+splitBlockSize-1);
         rc = WriteMemory(MS_XWord, splitBlockSize, flashAddress, (uint8_t *)buffer+targetProgramInfo.dataOffset);
      }
      else {
         print("       splitBlock[0x%06X..0x%06X]\n", flashAddress, flashAddress+splitBlockSize-1);
         print("FlashProgrammer::doFlashBlock() - flashOperationInfo.flashAddress = 0x%08X\n", flashOperationInfo.flashAddress);
         rc = executeTargetProgram(buffer, size);
      }
      if (rc != PROGRAMMING_RC_OK) {
         print("FlashProgrammer::doFlashBlock() - Error\n");
         return rc;
      }
      // Advance to next block of data
      flashAddress  += splitBlockSize;
      blockSize     -= splitBlockSize;
      oddBytes       = 0; // No odd bytes on subsequent blocks
      progressTimer->progress(splitBlockSize*sizeof(memoryElementType), NULL);
   }
   return PROGRAMMING_RC_OK;
}

//==============================================================================
//! Apply a flash operation to target based upon occupied flash addresses
//!
//! @param flashImage       Flash image to use
//! @param flashOperation   Operation to do on flash
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::applyFlashOperation(FlashImage     *flashImage,
                                                     FlashOperation  flashOperation) {
   USBDM_ErrorCode rc = PROGRAMMING_RC_OK;
   FlashImage::Enumerator *enumerator = flashImage->getEnumerator();

   print("FlashProgrammer::applyFlashOperation(%s) - Total Bytes = %d\n",
         getFlashOperationName(flashOperation), flashImage->getByteCount());
   // Go through each allocated block of memory applying operation
   while (enumerator->isValid()) {
      // Start address of block to program to flash
      uint32_t startBlock = enumerator->getAddress();

      // Find end of block to process
      enumerator->lastValid();
      uint32_t blockSize = enumerator->getAddress() - startBlock + 1;

      if (blockSize>0) {
         // Process block [startBlock..endBlock]
         rc = doFlashBlock(flashImage, blockSize, startBlock, flashOperation);
         if (rc != PROGRAMMING_RC_OK) {
            print("FlashProgrammer::applyFlashOperation() - Error \n");
            break;
         }
      }
      else {
         print("FlashProgrammer::applyFlashOperation() - empty block\n");
      }
      enumerator->setAddress(startBlock);
   }
   delete enumerator;
   return rc;
}

//==============================================================================
//! Blank check, program and verify target from flash image
//!
//! @param flashImage Flash image to program
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doProgram(FlashImage *flashImage) {
   print("FlashProgrammer::doProgram()\n");

   // Load target flash code to check programming options
   flashOperationInfo.alignment = 2;
   loadTargetProgram(OpBlankCheck);
   USBDM_ErrorCode rc;
   if ((targetProgramInfo.programOperation&DO_BLANK_CHECK_RANGE) == 0) {
      // Do separate blank check if not done by program operation
      rc = doBlankCheck(flashImage);
   }
   if ((targetProgramInfo.programOperation&DO_VERIFY_RANGE) == 0) {
      progressTimer->restart("Programming");
   }
   else {
      progressTimer->restart("Programming && Verifying...");
   }
   rc = applyFlashOperation(flashImage, OpProgram);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doProgram() - Programming failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   if ((targetProgramInfo.programOperation&DO_VERIFY_RANGE) == 0) {
      // Do separate verify operation
      progressTimer->restart("Verifying...");
      rc = doVerify(flashImage);
   }
   return rc;
}

//==============================================================================
//! Selective erase target.  Area erased is determined from flash image
//!
//! @param flashImage - Flash image used to determine regions to erase
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! Todo This is sub-optimal as it may erase the same sector multiple times.
//!
USBDM_ErrorCode FlashProgrammer::doSelectiveErase(FlashImage *flashImage) {

   print("FlashProgrammer::doSelectiveErase()\n");
   progressTimer->restart("Selective Erasing...");

   USBDM_ErrorCode rc;
   rc = applyFlashOperation(flashImage, OpSelectiveErase);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doSelectiveErase() - Selective erase failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==================================================================================
//! doReadbackVerify - Verifies the Target memory against memory image
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
USBDM_ErrorCode FlashProgrammer::doReadbackVerify(FlashImage *flashImage) {
   const unsigned MAX_BUFFER=0x800;
   memoryElementType buffer[MAX_BUFFER];
   int checkResult = TRUE;
   int blockResult;
   print("FlashProgrammer::doReadbackVerify()\n");

   FlashImage::Enumerator *enumerator = flashImage->getEnumerator();

   while (enumerator->isValid()) {
      uint32_t startBlock = enumerator->getAddress();
#if (TARGET==HCS08)||(TARGET==HC12)
      USBDM_ErrorCode rc = setPageRegisters(startBlock);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
#endif
      // Find end of block to verify
      enumerator->lastValid();
      unsigned regionSize = enumerator->getAddress() - startBlock + 1;
      print("FlashProgrammer::doReadbackVerify() - Verifying Block[0x%8.8X..0x%8.8X]\n", startBlock, startBlock+regionSize-1);
      MemorySpace_t memorySpace = MS_Byte;
      while (regionSize>0) {
         unsigned blockSize = regionSize;
         if (blockSize > MAX_BUFFER) {
            blockSize = MAX_BUFFER;
         }
         if (ReadMemory(memorySpace, blockSize*sizeof(memoryElementType), startBlock, (uint8_t *)buffer) != BDM_RC_OK) {
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
         print("FlashProgrammer::doReadbackVerify() - Verifying Sub-block[0x%8.8X..0x%8.8X]=>%s\n",
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

//==============================================================================
//! Verify target against flash image
//!
//! @param flashImage Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doTargetVerify(FlashImage *flashImage) {

   print("FlashProgrammer::doTargetVerify()\n");
   USBDM_ErrorCode rc = applyFlashOperation(flashImage, OpVerify );
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doTargetVerify() - Target Verifying failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==============================================================================
//! Verify target against flash image
//!
//! @param flashImage Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doVerify(FlashImage *flashImage) {
   USBDM_ErrorCode rc = PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   print("FlashProgrammer::doVerify()\n");
   progressTimer->restart("Verifying...");

   // Try target verify then read-back verify
//   rc = doTargetVerify(flashImage);
   if (rc == PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS) {
     rc = doReadbackVerify(flashImage);
   }
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doVerify() - verifying failed, Reason= %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   return rc;
}

//==============================================================================
//! Blank check target against flash image
//!
//! @param flashImage - Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doBlankCheck(FlashImage *flashImage) {

   print("FlashProgrammer::doBlankCheck()\n");
   progressTimer->restart("Blank Checking...");

   USBDM_ErrorCode rc = applyFlashOperation(flashImage, OpBlankCheck);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doBlankCheck() - Blank check failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==============================================================================
//! Write RAM portion of image
//!
//! @param flashImage - Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doWriteRam(FlashImage *flashImage) {

   print("FlashProgrammer::doWriteRam()\n");
   progressTimer->restart("Writing RAM...");

   USBDM_ErrorCode rc = applyFlashOperation(flashImage, OpWriteRam);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doWriteRam() - failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
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
                                             CallBackT    progressCallBack) {

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
#if (TARGET == CFVx) || (TARGET == MC56F80xx)
   rc = determineTargetSpeed();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
   // Set up for Flash operations (clock etc)
   rc = initialiseTargetFlash();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
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

   // Ignore resetAndConnectTarget() failure if mass erasing target as
   // it is possible to mass erase some targets without a complete debug connection
   if ((rc != PROGRAMMING_RC_OK) && (parameters.getEraseOption() != DeviceData::eraseMass)) {
      return rc;
   }
   bool secured = checkTargetUnSecured() != PROGRAMMING_RC_OK;

   // Check target security
   if (secured && (parameters.getEraseOption() != DeviceData::eraseMass)) {
      // Can't program if secured
      return PROGRAMMING_RC_ERROR_SECURED;
   }
#if (TARGET == HCS12)
   // Check for nasty chip of death
   rc = checkUnsupportedTarget();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
   // Mass erase 
   if (parameters.getEraseOption() == DeviceData::eraseMass) {
      rc = massEraseTarget();
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
   }
   // Check target SDID
   rc = confirmSDID();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Modify flash image according to security options
   rc = setFlashSecurity(*flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#if (TARGET == CFVx) || (TARGET == MC56F80xx)
   rc = determineTargetSpeed();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
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
#if (TARGET == CFV1) || (TARGET == ARM)
   if (parameters.getEraseOption() == DeviceData::eraseMass) {
      // Erase the security area as Mass erase programs it
      rc = selectiveEraseFlashSecurity();
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
   }
   // Program EEPROM/DFLASH Split
   rc = partitionFlexNVM();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
   if (parameters.getEraseOption() == DeviceData::eraseAll) {
      // Erase all flash arrays
      rc = eraseFlash();
   }
   else if (parameters.getEraseOption() == DeviceData::eraseSelective) {
      // Selective erase area to be programmed - this may have collateral damage!
      rc = doSelectiveErase(flashImage);
   }
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::programFlash() - erasing failed, Reason= %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
#ifdef GDI
   mtwksDisplayLine("FlashProgrammer::programFlash() - Erase Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
#ifdef LOG
   print("FlashProgrammer::programFlash() - Erase Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1+1024*progressTimer->elapsedTime()),  rc);
#endif
   // Program flash
   rc = doProgram(flashImage);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::programFlash() - programing failed, Reason= %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   if (doRamWrites){
      doWriteRam(flashImage);
   }
#ifdef GDI
#if (TARGET == CFV1) || (TARGET == HCS08)
   if (parameters.getClockTrimFreq() != 0) {
      uint16_t trimValue = parameters.getClockTrimValue();
      mtwksDisplayLine("FlashProgrammer::programFlash() - Device Trim Value = %2.2X.%1X\n", trimValue>>1, trimValue&0x01);
   }
#endif
   mtwksDisplayLine("FlashProgrammer::programFlash() - Programming & verifying Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
#ifdef LOG
#if (TARGET == CFV1) || (TARGET == HCS08)
   if (parameters.getClockTrimFreq() != 0) {
      uint16_t trimValue = parameters.getClockTrimValue();
      print("FlashProgrammer::programFlash() - Device Trim Value = %2.2X.%1X\n", trimValue>>1, trimValue&0x01);
   }
#endif
   print("FlashProgrammer::programFlash() - Programming & verifying Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImage->getByteCount()/(1+1024*progressTimer->elapsedTime()),  rc);
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

