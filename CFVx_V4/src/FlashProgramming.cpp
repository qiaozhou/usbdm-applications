/*! \file
   \brief Utility Routines for programming CFVx Flash

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

inline uint16_t swap16(uint16_t data) {
   return ((data<<8)&0xFF00) + ((data>>8)&0xFF);
}
inline uint32_t swap32(uint32_t data) {
   return ((data<<24)&0xFF000000) + ((data<<8)&0xFF0000) + ((data>>8)&0xFF00) + ((data>>24)&0xFF);
}

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
   return getData16Le(data);
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
   // Add adddress of each flash region
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
   if (rc != PROGRAMMING_RC_OK)
      return rc;
   // Do Mass erase using TCL script
   rc = runTCLCommand("massEraseTarget");
   if (rc != PROGRAMMING_RC_OK)
      return rc;
   rc = resetAndConnectTarget();
   if (rc != PROGRAMMING_RC_OK)
      return rc;
   return PROGRAMMING_RC_OK;
}

//==============================================================================
// Flag masks
#define DO_INIT_FLASH         (1<<0) // Do (one-off) initialisation of flash
#define DO_MASS_ERASE         (1<<1) // Mass erase device
#define DO_ERASE_RANGE        (1<<2) // Erase range (including option region)
#define DO_BLANK_CHECK_RANGE  (1<<3) // Blank check region
#define DO_PROGRAM_RANGE      (1<<4) // Program range (including option region)
#define DO_VERIFY_RANGE       (1<<5) // Verify range
#define DO_UNLOCK_FLASH       (1<<6) // Unlock flash with default security options  (+mass erase if needed)
#define DO_LOCK_FLASH         (1<<7) // Lock flash with default security options
#define DO_MISC_OP            (1<<8) // Counting loop to determine clock speed
#define DO_MISC_OFFSET        (4)
#define DO_MISC_MASK          (0x1F<<DO_MISC_OFFSET)           // Bits 8..4 incl.
#define DO_TIMING_LOOP        (DO_MISC_OP|(0<<DO_MISC_OFFSET)) // Do timing loop
// 9 - 15 reserved
// 16-23 target/family specific
// Allows programming/erasing Option region, +DO_INIT_FLASH+DO_PROGRAM_RANGE+DO_ERASE_RANGE
#define DO_MODIFY_OPTION      (1<<16)
// 24-30 reserved
#define IS_COMPLETE           (1U<<31)

// Capability masks
#define CAP_MASS_ERASE     (1<<1)
#define CAP_ERASE_RANGE    (1<<2)
#define CAP_BLANK_CHECK    (1<<3)
#define CAP_PROGRAM_RANGE  (1<<4)
#define CAP_VERIFY_RANGE   (1<<5)
#define CAP_UNLOCK_FLASH   (1<<6)
#define CAP_TIMING         (1<<7)
#define CAP_DSC_OVERLAY    (1<<8)

// Alignment requirement on programming (i.e. minimum element)
#define CAP_ALIGN_OFFS     (28)
#define CAP_ALIGN_MASK     (3<<CAP_ALIGN_OFFS)
#define CAP_ALIGN_BYTE     (0<<CAP_ALIGN_OFFS) // Program individual bytes
#define CAP_ALIGN_2BYTE    (1<<CAP_ALIGN_OFFS) // Program halfwords
#define CAP_ALIGN_4BYTE    (2<<CAP_ALIGN_OFFS) // Program words
#define CAP_ALIGN_8BYTE    (3<<CAP_ALIGN_OFFS) // Program 8-bytes (longwords/phrase)
//
#define CAP_RELOCATABLE    (1<<31) // Code may be relocated

//=======================================================================
//! Loads the default Flash programming code to target memory
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - see loadTargetProgram(...) for details
//! @note - Tries device program code & then flashRegion specific if necessary
//!
USBDM_ErrorCode FlashProgrammer::loadTargetProgram() {
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
   return loadTargetProgram(flashProgram);
}

//=======================================================================
//! Loads the Flash programming code to target memory
//!
//! @param  flashProgram program to load
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note - Assumes the target has been connected to
//!         Confirms download (if necessary) and checks RAM upper boundary.
//!
//! Target Memory map
//! +-------------------------------------------------+ -+
//! |   FlashProgramHeader_t  flashProgramHeader;     |  |
//! +-------------------------------------------------+   > Unchanging written once
//! |   Flash program code....                        |  |
//! +-------------------------------------------------+ -+
//! |   FlashData_t           flashData;              |  > Write/Read
//! +-------------------------------------------------+ -+  
//! |   Data to program....                           |  > Write
//! +-------------------------------------------------+ -+
//!
USBDM_ErrorCode FlashProgrammer::loadTargetProgram(FlashProgramPtr flashProgram) {
   FlashProgramHeader_t *headerPtr;
   uint8_t buffer[4000];
   print("FlashProgrammer::loadTargetProgram()\n");

   if (!flashProgram) {
      // Try to get device general routines
      flashProgram = parameters.getFlashProgram();
   }
#if TARGET != HCS08
   if (!flashProgram) {
      print("FlashProgrammer::loadTargetProgram(...) - No flash program found for target\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#endif
   if (currentFlashProgram == flashProgram) {
      print("FlashProgrammer::loadTargetProgram(...) - re-using existing code\n");
      return PROGRAMMING_RC_OK;
   }
   currentFlashProgram = flashProgram;

   unsigned size;
   uint32_t loadAddress;
   USBDM_ErrorCode rc = loadSRec(flashProgram->flashProgram.c_str(),
                                 buffer,
                                 sizeof(buffer),
                                 &size,
                                 &loadAddress);
   if (rc !=  BDM_RC_OK) {
      print("FlashProgrammer::loadTargetProgram(...) - loadSRec() failed\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
//   print("FlashProgrammer::loadTargetProgram() - header offset = 0x%08X\n",
//         getData32Target(buffer)-loadAddress);

   // Find 'header' in download image
   uint32_t headerAddress = getData32Target(buffer);
   headerPtr = (FlashProgramHeader_t*) (buffer+headerAddress-loadAddress);

   uint32_t capabilities = targetToNative32(headerPtr->capabilities);

#if (TARGET != ARM) && (TARGET != MC56F80xx)
   // ARM & DSC are not relocatable but don't load at start of RAM
   uint32_t loadOffset = ((parameters.getRamStart()+1)&~1) - loadAddress;
   if (loadOffset != 0) {
      print("FlashProgrammer::loadTargetProgram() - Loading at non-default address, image@0x%04X (offset=%04X)\n", loadAddress, loadOffset);
      if ((capabilities&CAP_RELOCATABLE)==0) {
         print("FlashProgrammer::loadTargetProgram() - Image is not position-independent.\n");
         return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
      }
      loadAddress           += loadOffset;
      headerPtr->loadAddress = nativeToTarget16(loadAddress);
      headerPtr->entry       = nativeToTarget16(targetToNative16(headerPtr->entry)+loadOffset);
      print("FlashProgrammer::loadTargetProgram() - Loading at non-default address 0x%04X (offset=%04X)\n", loadAddress, loadOffset);
   }
#endif

#if TARGET == MC56F80xx
   // Update location of where programming info will be located
   flashProgramHeader.flashData = targetToNative32(headerPtr->flashData);
   if ((capabilities&CAP_DSC_OVERLAY)!=0) {
      // Loading code into shared RAM - load data offset by code size
      print("FlashProgrammer::loadTargetProgram() - loading data into overlayed RAM @ 0x%06X\n", flashProgramHeader.flashData);
   }
   else {
      // Loading code into separate program RAM - load data RAM separately
      print("FlashProgrammer::loadTargetProgram() - loading data into separate RAM @ 0x%06X\n", flashProgramHeader.flashData);
   }
#else
   // Update location of where programming info will be located
   flashProgramHeader.flashData = (loadAddress + size + 3)&~0x3;
   headerPtr->flashData   = nativeToTarget32(flashProgramHeader.flashData);
#endif

   flashData.frequency = targetBusFrequency;
   // Save location of RAM data buffer
   flashData.data      = (flashProgramHeader.flashData + sizeof(FlashData_t)+3)&~0x03;
   // Save maximum size of the buffer
   flashData.size       = (parameters.getRamEnd()-flashData.data)&~0x3;
   flashData.errorCode  = 0;

   // Save the programming data structure
   flashProgramHeader.loadAddress   = targetToNative32(headerPtr->loadAddress);
   flashProgramHeader.entry         = targetToNative32(headerPtr->entry);
   flashProgramHeader.capabilities  = targetToNative32(headerPtr->capabilities);
   flashProgramHeader.calibFactor   = targetToNative32(headerPtr->calibFactor);

   print("FlashProgrammer::loadTargetProgram()\n"
         "   flashProgramHeader.loadAddress    = 0x%08X\n"
         "   flashProgramHeader.entry          = 0x%08X\n"
         "   flashProgramHeader.capabilities   = 0x%08X(%s)\n"
         "   flashProgramHeader.calibFactor    = 0x%08X (%d)\n"
         "   flashProgramHeader.flashData      = 0x%08X\n"
         "   flashData.data                    = 0x%08X\n"
         "   flashData.size (max)              = 0x%08X\n",
         flashProgramHeader.loadAddress,
         flashProgramHeader.entry,
         flashProgramHeader.capabilities,getProgramCapabilityNames(flashProgramHeader.capabilities),
         flashProgramHeader.calibFactor, flashProgramHeader.calibFactor,
         flashProgramHeader.flashData,
         flashData.data,
         flashData.size
         );
#if TARGET != MC56F80xx
   // DSC deals with word addresses which are always aligned
   if ((flashData.data & 0x03) != 0){
      print("FlashProgrammer::loadTargetProgram() - flashData.data is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if ((flashProgramHeader.loadAddress & 0x01) != 0){
      print("FlashProgrammer::loadTargetProgram() - flashProgramHeader.loadAddress is not aligned\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if ((loadAddress<parameters.getRamStart()) ||
       (loadAddress>parameters.getRamEnd())) {
      print("FlashProgrammer::loadTargetProgram() - Program image location [0x%X] is outside target RAM [0x%X-0x%X]\n",
            loadAddress, parameters.getRamStart(), parameters.getRamEnd());
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
#endif
   // Sanity check buffer
   if (((uint32_t)(flashProgramHeader.flashData)<parameters.getRamStart()) ||
        ((uint32_t)(flashProgramHeader.flashData+100)>parameters.getRamEnd())) {
      print("FlashProgrammer::loadTargetProgram() - Data buffer location [0x%X-0x%X] is outside target RAM [0x%X-0x%X]\n",
            flashProgramHeader.flashData, flashProgramHeader.flashData+size, parameters.getRamStart(), parameters.getRamEnd());
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Probe Data RAM
   rc = probeMemory(MS_Word, flashProgramHeader.flashData);
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
   // Write the flash programming code to target memory
   if (WriteMemory(MS_Word,size,loadAddress,(uint8_t*)&buffer) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
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
"DO_MASS_ERASE|",         // Mass erase device
"DO_ERASE_RANGE|",        // Erase range (including option region)
"DO_BLANK_CHECK_RANGE|",  // Blank check region
"DO_PROGRAM_RANGE|",      // Program range (including option region)
"DO_VERIFY_RANGE|",       // Verify range
"DO_UNLOCK_FLASH|",       // Unlock flash with default security options  (+mass erase if needed)
"DO_LOCK_FLASH|",         // Lock flash with default security options
};

   buff[0] = '\0';
   for (index=0;
        index<sizeof(actionTable)/sizeof(actionTable[0]);
         index++) {
      if ((actions&(1<<index)) != 0) {
         strcat(buff,actionTable[index]);
      }
   }

   if ((actions&DO_MISC_OP) != 0) {
      switch (actions&DO_MISC_MASK) {
         case DO_TIMING_LOOP&DO_MISC_MASK: strcat(buff,"DO_TIMING_LOOP|"); break;
         default:                          strcat(buff,"DO_MISC_???|");    break;
      }
   }
   if (actions&IS_COMPLETE)
      strcat(buff,"IS_COMPLETE");

   return buff;
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
"CAP_INIT|",            // Do initialisation of flash
"CAP_MASS_ERASE|",      // Mass erase device
"CAP_ERASE_RANGE|",     // Erase range (including option region)
"CAP_BLANK_CHECK|",     // Blank check region
"CAP_PROGRAM_RANGE|",   // Program range (including option region)
"CAP_VERIFY_RANGE|",    // Verify range
"CAP_UNLOCK_FLASH|",    // Un/lock flash with default security options  (+mass erase if needed)
"CAP_TIMING|",          // Lock flash with default security options
"CAP_DSC_OVERLAY|",
};

   buff[0] = '\0';
   for (index=0;
        index<sizeof(actionTable)/sizeof(actionTable[0]);
         index++) {
      if ((actions&(1<<index)) != 0) {
         strcat(buff,actionTable[index]);
      }
   }
   switch (actions&CAP_ALIGN_MASK) {
      case CAP_ALIGN_BYTE:  strcat(buff,"CAP_ALIGN_BYTE|");  break;
      case CAP_ALIGN_2BYTE: strcat(buff,"CAP_ALIGN_2BYTE|"); break;
      case CAP_ALIGN_4BYTE: strcat(buff,"CAP_ALIGN_4BYTE|"); break;
      case CAP_ALIGN_8BYTE: strcat(buff,"CAP_ALIGN_8BYTE|"); break;
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
//! \brief Executes program on target.
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::executeTargetProgram(FlashData_t *flashHeader, uint32_t size) {

   if (size == 0) {
      flashHeader->data = 0;
   }
   print("FlashProgrammer::executeTargetProgram()\n"
         "   flags      = 0x%08X(%s)\n"
         "   controller = 0x%08X\n"
         "   frequency  = %d kHz\n"
         "   sectorSize = 0x%04X\n"
         "   address    = 0x%08X\n"
         "   size       = 0x%08X\n"
         "   data       = 0x%08X\n",
         targetToNative32(flashHeader->flags), getProgramActionNames(targetToNative32(flashHeader->flags)),
         targetToNative32(flashHeader->controller),
         targetToNative32(flashHeader->frequency),
         targetToNative16(flashHeader->sectorSize),
         targetToNative32(flashHeader->address),
         targetToNative32(flashHeader->size),
         targetToNative32(flashHeader->data)
         );
#if defined(LOG) && (TARGET==ARM)
   report("FlashProgrammer::executeTargetProgram()");
#endif
   print("FlashProgrammer::executeTargetProgram() - Writing Header+Data\n");

   flashHeader->errorCode = 0;

   MemorySpace_t memorySpace = MS_Word;

   // Write the flash parameters & data to target memory
   if (WriteMemory(memorySpace, sizeof(FlashData_t)+size,
                   flashProgramHeader.flashData,
                   (uint8_t*)flashHeader) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Set target PC to start of code & verify
   long unsigned targetRegPC;
   if (WritePC(flashProgramHeader.entry) != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - PC write failed\n");
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   if (ReadPC(&targetRegPC) != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - PC read failed\n");
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   if ((flashProgramHeader.entry) != targetRegPC) {
      print("FlashProgrammer::executeTargetProgram() - PC verify failed\n");
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   USBDMStatus_t status;
   USBDM_GetBDMStatus(&status);

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
   } while ((getRunStatus() == BDM_RC_BUSY) && (--timeout>0));
   TargetHalt();
   unsigned long value;
   ReadPC(&value);
   print("\nFlashProgrammer::executeTargetProgram() - Start PC = 0x%08X, end PC = 0x%08X\n", targetRegPC, value);
   // Read the flash parameters back from target memory
   FlashData_t flashProgramDataResult;
   if (ReadMemory(memorySpace, sizeof(flashProgramDataResult), flashProgramHeader.flashData, (uint8_t*)&flashProgramDataResult) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   flashProgramDataResult.flags     = targetToNative32(flashProgramDataResult.flags);
   flashProgramDataResult.errorCode = targetToNative16(flashProgramDataResult.errorCode);
   print("FlashProgrammer::executeTargetProgram() - complete, flags = 0x%08X(%s), errCode=%d\n",
         flashProgramDataResult.flags,
         getProgramActionNames(flashProgramDataResult.flags),
         flashProgramDataResult.errorCode);
   if ((timeout <= 0) && (flashProgramDataResult.errorCode == FLASH_ERR_OK)) {
      flashProgramDataResult.errorCode = FLASH_ERR_TIMEOUT;
      print("FlashProgrammer::executeTargetProgram() - Error, Timeout waiting for completion.\n");
   }
   if ((flashProgramDataResult.flags != IS_COMPLETE) && (flashProgramDataResult.errorCode == FLASH_ERR_OK)) {
//      uint8_t  buffer[1000];
//      USBDM_ReadMemory(1,sizeof(buffer), flashProgramHeader.loadAddress, buffer);
      flashProgramDataResult.errorCode = FLASH_ERR_UNKNOWN;
      print("FlashProgrammer::executeTargetProgram() - Error, Unexpected flag result.\n");
   }
   USBDM_ErrorCode rc = convertTargetErrorCode((FlashDriverError_t)flashProgramDataResult.errorCode);
   if (rc != BDM_RC_OK) {
      print("FlashProgrammer::executeTargetProgram() - Error - %s\n", USBDM_GetErrorString(rc));
#if TARGET == CFV1
      uint8_t SRSreg;
      USBDM_ReadMemory(1, 1, 0xFF9800, &SRSreg);
      print("FlashProgrammer::executeTargetProgram() - SRS = 0x%02X\n", SRSreg);
#endif
   }
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

   targetBusFrequency = 0;

   TimingData_t timingData = {0};
   timingData.flags      = nativeToTarget32(DO_TIMING_LOOP|IS_COMPLETE); // IS_COMPLETE as check - should be cleared
   timingData.controller = nativeToTarget32(-1);                         // Dummy value - not used

   print("FlashProgrammer::determineTargetSpeed()\n"
         "   flags      = 0x%08X(%s)\n"
         "   controller = 0x%08X\n",
         targetToNative32(timingData.flags), getProgramActionNames(targetToNative32(timingData.flags)),
         targetToNative32(timingData.controller)
         );
   MemorySpace_t memorySpace = MS_Word;
   // Write the flash parameters & data to target memory
   if (WriteMemory(memorySpace, sizeof(TimingData_t), flashProgramHeader.flashData, (uint8_t*)&timingData) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Set target PC to start of code & verify
   if (WritePC(flashProgramHeader.entry) != BDM_RC_OK) {
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
   TimingData_t timingDataResult;
   if (ReadMemory(memorySpace, sizeof(timingDataResult),
      flashProgramHeader.flashData, (uint8_t*)&timingDataResult) != BDM_RC_OK) {
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
   print("\nFlashProgrammer::determineTargetSpeed() - Start PC = 0x%08X, end PC = 0x%08X\n", flashProgramHeader.entry, value);
   if ((timingDataResult.flags != DO_TIMING_LOOP) && (timingDataResult.errorCode == FLASH_ERR_OK)) {
      timingDataResult.errorCode    = FLASH_ERR_UNKNOWN;
      print("FlashProgrammer::determineTargetSpeed() - Error, Unexpected flag result\n");
   }
   if (timingDataResult.errorCode != FLASH_ERR_OK) {
      print("FlashProgrammer::determineTargetSpeed() - Error\n");
      return convertTargetErrorCode((FlashDriverError_t)timingDataResult.errorCode);
   }
   targetBusFrequency = 50*int(0.5+((250.0 * timingDataResult.timingCount)/flashProgramHeader.calibFactor));
   flashData.frequency = targetBusFrequency;
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
//! @note - Assumes flash programming code has already been loaded to target.
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
      const FlashProgramPtr flashProgram = memoryRegionPtr->getFlashprogram();
      rc = loadTargetProgram(flashProgram);
      if (rc != PROGRAMMING_RC_OK) {
         return rc;
      }
      flashData.controller = memoryRegionPtr->getRegisterAddress();
      FlashData_t targetFlashData = {0};
      targetFlashData.flags      = nativeToTarget32(DO_INIT_FLASH|DO_MASS_ERASE);
      targetFlashData.controller = nativeToTarget32(flashData.controller);
      targetFlashData.frequency  = nativeToTarget32(flashData.frequency);
      targetFlashData.address    = nativeToTarget32(memoryRegionPtr->getDummyAddress());
      targetFlashData.sectorSize = nativeToTarget16(memoryRegionPtr->getSectorSize());
      rc = executeTargetProgram(&targetFlashData, 0);
      if (rc != PROGRAMMING_RC_OK)
         return rc;
   }
   return rc;
}

#if (TARGET == HCS08) || (TARGET == CFV1)
//=======================================================================
//! Updates the memory image from the target flash Clock trim location(s)
//!
//! @param flashImageDescription   = Flash image
//!
USBDM_ErrorCode FlashProgrammer::dummyTrimLocations(FlashImage *flashImageDescription) {

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
      flashImageDescription->setValue(start+index, data[index]);
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
//! @param flashImageDescription Description of flash contents to be programmed.
//! @param flashRegion           The memory region involved
//!
//!
USBDM_ErrorCode FlashProgrammer::setFlashSecurity(FlashImage  &flashImageDescription, MemoryRegionPtr flashRegion) {
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
               "mem[0x%04X-0x%04X] = ", securityAddress, securityAddress+size-1);
         printDump(data, size, securityAddress);
#endif
         flashImageDescription.loadData(size, securityAddress, data);
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
               "mem[0x%04X-0x%04X] = ", securityAddress, securityAddress+size-1);
         printDump(data, size, securityAddress);
#endif
         flashImageDescription.loadData(size, securityAddress, data);
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
               "mem[0x%04X-0x%04X] = ", securityAddress, securityAddress+size-1);
         printDump(data, size, securityAddress);
#endif
         flashImageDescription.loadData(size, securityAddress, data, true);
      }
         break;
      case SEC_DEFAULT:   // Unchanged
      default:
         print("FlashProgrammer::setFlashSecurity():Leaving flash image unchanged\n");
         break;
   }
   return PROGRAMMING_RC_OK;
}

//=======================================================================
//! Erase Target Flash memory
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//!
USBDM_ErrorCode FlashProgrammer::setFlashSecurity(FlashImage  &flashImageDescription) {
   print("FlashProgrammer::setFlashSecurity()\n");

   // Process each flash region
   USBDM_ErrorCode rc = BDM_RC_OK;
   for (int index=0; ; index++) {
      MemoryRegionPtr memoryRegionPtr = parameters.getMemoryRegion(index);
      if (memoryRegionPtr == NULL) {
         break;
      }
      rc = setFlashSecurity(flashImageDescription, memoryRegionPtr);
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
//! @param flashImageDescription Description of flash contents to be processed.
//! @param blockSize             Size of block to process (bytes)
//! @param flashAddress          Start address of block in flash to process
//! @param flashOperation        Operation to do on flash
//!
//! @return error code see \ref USBDM_ErrorCode.
//!
//! @note - Assumes flash programming code has already been loaded to target.
//! @note - The memory range must be within one page for paged devices.
//!
USBDM_ErrorCode FlashProgrammer::doFlashBlock(FlashImage    *flashImageDescription,
                                              unsigned int   blockSize,
                                              uint32_t       &flashAddress,
                                              uint32_t       flashOperation) {

   const unsigned int MaxSplitBlockSize = 0x4000;
   struct {
      FlashData_t targetFlashData;
      uint8_t     data[MaxSplitBlockSize+10];
   } buffer;
   unsigned int maxSplitBlockSize;
   unsigned int splitBlockSize;
   unsigned int oddBytes;

   print("FlashProgrammer::doFlashBlock() [0x%06X..0x%06X]\n", flashAddress, flashAddress+blockSize-1);

   if (!flashReady) {
      print("FlashProgrammer::doFlashBlock() - Error, Flash not ready\n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   // Maximum split block size must be made less than buffer RAM available
   maxSplitBlockSize = this->flashData.size;

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
            print("FlashProgrammer::doFlashBlock() - Block not within target memory\n");
            return PROGRAMMING_RC_ERROR_OUTSIDE_TARGET_FLASH;
         }
      uint32_t lastContiguous;
      if (memoryRegionPtr->findLastContiguous(flashAddress, &lastContiguous)) {
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
            rc = doFlashBlock(flashImageDescription, firstBlockSize, flashAddress, flashOperation);
            if (rc != PROGRAMMING_RC_OK) {
               return rc;
            }
            flashAddress += firstBlockSize;
            rc = doFlashBlock(flashImageDescription, blockSize-firstBlockSize, flashAddress, flashOperation);
            return rc;
         }
         break;
      }
   }
   MemType_t memoryType = memoryRegionPtr->getMemoryType();
   print("FlashProgrammer::doFlashBlock() - Processing %s\n", MemoryRegion::getMemoryTypeName(memoryType));
   USBDM_ErrorCode rc = loadTargetProgram(memoryRegionPtr->getFlashprogram());
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   flashData.controller = memoryRegionPtr->getRegisterAddress();
   flashData.sectorSize = memoryRegionPtr->getSectorSize();
   uint32_t alignMask = ((1<<((this->flashProgramHeader.capabilities&CAP_ALIGN_MASK)>>CAP_ALIGN_OFFS))-1);
   print("FlashProgrammer::doFlashBlock() - align mask = 0x%08X\n", alignMask);

   // splitBlockSize must be aligned
   maxSplitBlockSize &= ~alignMask;

   // Calculate any odd padding bytes at start of block
   oddBytes      = flashAddress & alignMask;
   // Round start address off to alignment requirements
   flashAddress  &= ~alignMask;
   // Pad block size with odd leading bytes
   blockSize += oddBytes;

   // Pad blocksize to alignment requirements
   blockSize = (blockSize+alignMask)&~alignMask;

   progressTimer->progress(0, NULL);

   while (blockSize>0) {
      unsigned flashIndex  = 0;
      unsigned size        = 0;
      // Determine size of block to process
      splitBlockSize = blockSize;
      if ((flashOperation & (DO_VERIFY_RANGE|DO_PROGRAM_RANGE)) != 0) {
         // Requires data transfer using buffer
         if (splitBlockSize>maxSplitBlockSize) {
            splitBlockSize = maxSplitBlockSize;
         }
         // Pad any odd leading bytes as 0xFF
         for (flashIndex=0; flashIndex<oddBytes; flashIndex++) {
            buffer.data[flashIndex] = 0xFF;
         }
         // Copy flash data to buffer
         for(flashIndex=0; flashIndex<splitBlockSize; flashIndex++) {
            buffer.data[flashIndex] = flashImageDescription->getValue(flashAddress+flashIndex);
         }
         // Pad trailing bytes to aligned address
         for (; (flashIndex&alignMask) != 0; flashIndex++) {
            buffer.data[flashIndex] = flashImageDescription->getValue(flashAddress+flashIndex);
         }
         // Actual data bytes to write
         size = flashIndex;
      }
      else {
         // No data transfer so no size limits
         flashIndex  = (blockSize+alignMask)&~alignMask;
         size = 0;
      }
      print("       splitBlock[0x%06X..0x%06X]\n", flashAddress, flashAddress+splitBlockSize-1);
      memset(&buffer.targetFlashData, 0, sizeof(buffer.targetFlashData));
      buffer.targetFlashData.flags      = nativeToTarget32(flashOperation);
      buffer.targetFlashData.controller = nativeToTarget32(flashData.controller);
      buffer.targetFlashData.frequency  = nativeToTarget32(flashData.frequency);
      buffer.targetFlashData.sectorSize = nativeToTarget16(flashData.sectorSize);
      buffer.targetFlashData.address    = nativeToTarget32(flashAddress);
      buffer.targetFlashData.size       = nativeToTarget32(flashIndex);
      buffer.targetFlashData.data       = nativeToTarget32(flashData.data);
      USBDM_ErrorCode rc = executeTargetProgram(&buffer.targetFlashData, size);
      if (rc != PROGRAMMING_RC_OK) {
         print("FlashProgrammer::doFlashBlock() - Error\n");
         return rc;
      }
      // Advance to next block of data
      flashAddress  += splitBlockSize;
      blockSize     -= splitBlockSize;
      oddBytes       = 0; // No odd bytes on subsequent blocks
      progressTimer->progress(splitBlockSize, NULL);
   }
   return PROGRAMMING_RC_OK;
}

//==============================================================================
//! Apply a flash operation to target based upon occupied flash addresses
//!
//! @param flashImageDescription Flash image to use
//! @param flashOperation        Operation to do on flash
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::applyFlashOperation(FlashImage *flashImageDescription, uint32_t operation) {
   USBDM_ErrorCode rc = PROGRAMMING_RC_OK;
   FlashImage::Enumerator *enumerator = flashImageDescription->getEnumerator();

   print("FlashProgrammer::applyFlashOperation() - Total Bytes = %d\n", flashImageDescription->getByteCount());
   // Go through each allocated block of memory applying operation
   while (enumerator->isValid()) {
      // Start address of block to program to flash
      uint32_t startBlock = enumerator->getAddress();

      // Find end of block to process
      enumerator->lastValid();
      uint32_t blockSize = enumerator->getAddress() - startBlock + 1;

      if (blockSize>0) {
         // Process block [startBlock..endBlock]
         rc = doFlashBlock(flashImageDescription, blockSize, startBlock, operation);
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
//! Program target from flash image
//!
//! @param flashImageDescription Flash image to program
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doProgram(FlashImage *flashImageDescription) {

   print("FlashProgrammer::doProgram()\n");
   progressTimer->restart("Programming && Verifying...");

   USBDM_ErrorCode rc;
   rc = applyFlashOperation(flashImageDescription,
                            DO_INIT_FLASH|DO_BLANK_CHECK_RANGE|DO_PROGRAM_RANGE|DO_VERIFY_RANGE);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doProgram() - Programming failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==============================================================================
//! Selective erase target.  Area erased is determined from flash image
//!
//! @param flashImageDescription Flash image used to determine regions to erase
//!
//! @return error code see \ref USBDM_ErrorCode
//!
// Todo This is sub-optimal as it may erase the same sector multiple times.
USBDM_ErrorCode FlashProgrammer::doSelectiveErase(FlashImage *flashImageDescription) {

   print("FlashProgrammer::doSelectiveErase()\n");
   progressTimer->restart("Selective Erasing...");

   USBDM_ErrorCode rc;
   rc = applyFlashOperation(flashImageDescription,
                            DO_INIT_FLASH|DO_ERASE_RANGE|DO_BLANK_CHECK_RANGE);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doSelectiveErase() - Selective erase failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==============================================================================
//! Verify target against flash image
//!
//! @param flashImageDescription Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doVerify(FlashImage *flashImageDescription) {

   print("FlashProgrammer::doVerify()\n");
   progressTimer->restart("Verifying...");

   USBDM_ErrorCode rc;
   rc = applyFlashOperation(flashImageDescription,
                            DO_INIT_FLASH|DO_VERIFY_RANGE);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doVerify() - Verifying failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//==============================================================================
//! Blank check target against flash image
//!
//! @param flashImageDescription Flash image to verify
//!
//! @return error code see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::doBlankCheck(FlashImage *flashImageDescription) {

   print("FlashProgrammer::doBlankCheck()\n");
   progressTimer->restart("Blank Checking...");

   USBDM_ErrorCode rc;
   rc = applyFlashOperation(flashImageDescription,
         DO_INIT_FLASH|DO_BLANK_CHECK_RANGE);
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::doBlankCheck() - Blank check failed, Reason= %s\n", USBDM_GetErrorString(rc));
   }
   return rc;
}

//=======================================================================
//! Verify Target Flash memory
//!
//! @param flashImageDescription Description of flash contents to be verified.
//! @param progressCallBack      Callback function to indicate progress
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note If target clock trimming is enabled then the Non-volatile clock trim
//!       locations are ignored.
//!
USBDM_ErrorCode FlashProgrammer::verifyFlash(FlashImage  *flashImageDescription, CallBackT progressCallBack) {

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
   progressTimer = new ProgressTimer(progressCallBack, flashImageDescription->getByteCount());
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
   rc = setFlashSecurity(*flashImageDescription);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#if (TARGET == CFV1) || (TARGET == HCS08)
   // Modify flash image according to trim options - to be consistent with what is programmed
   rc = dummyTrimLocations(flashImageDescription);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#endif
   // Load default flash programming code to target
   rc = loadTargetProgram();
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
#if (TARGET == CFVx)
   rc = determineTargetSpeed();
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
   rc = doVerify(flashImageDescription);

   print("FlashProgrammer::verifyFlash() - Verifying Time = %3.2f s, rc = %d\n", progressTimer->elapsedTime(), rc);

   return rc;
}

//=======================================================================
//! Program Target Flash memory
//!
//! @param flashImageDescription Description of flash contents to be programmed.
//! @param progressCallBack      Callback function to indicate progress
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note Assumes the target device has already been opened & USBDM options set.
//! @note The FTRIM etc. locations in the flash image may be modified with trim values.
//! @note Security locations within the flash image may be modified to effect the protection options.
//!
USBDM_ErrorCode FlashProgrammer::programFlash(FlashImage  *flashImageDescription, CallBackT progressCallBack) {
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
         flashImageDescription->getByteCount());
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
         flashImageDescription->getByteCount());
#endif

   if (progressTimer != NULL) {
      delete progressTimer;
   }
   progressTimer = new ProgressTimer(progressCallBack, flashImageDescription->getByteCount());
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
   rc = setFlashSecurity(*flashImageDescription);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Load default flash programming code to target
   // Need this for software speed determination
   rc = loadTargetProgram();
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
   rc = setFlashTrimValues(flashImageDescription);
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
   if (parameters.getEraseOption() == DeviceData::eraseAll) {
      // Erase all flash arrays
      rc = eraseFlash();
   }
   else if (parameters.getEraseOption() == DeviceData::eraseSelective) {
      // Selective erase area to be programmed - this may have collateral damage!
      rc = doSelectiveErase(flashImageDescription);
   }
   if (rc != PROGRAMMING_RC_OK) {
      print("FlashProgrammer::programFlash() - erasing failed, Reason= %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
#ifdef GDI
   mtwksDisplayLine("FlashProgrammer::programFlash() - Erase Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImageDescription->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
#ifdef LOG
   print("FlashProgrammer::programFlash() - Erase Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImageDescription->getByteCount()/(1+1024*progressTimer->elapsedTime()),  rc);
#endif

   // Program flash
   rc = doProgram(flashImageDescription);
#ifdef GDI
   mtwksDisplayLine("FlashProgrammer::programFlash() - Programming & verifying Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImageDescription->getByteCount()/(1024*progressTimer->elapsedTime()),  rc);
#endif
#ifdef LOG
   print("FlashProgrammer::programFlash() - Programming & verifying Time = %3.2f s, Speed = %2.2f kBytes/s, rc = %d\n",
         progressTimer->elapsedTime(), flashImageDescription->getByteCount()/(1+1024*progressTimer->elapsedTime()),  rc);
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

