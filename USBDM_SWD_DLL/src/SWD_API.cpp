/*! \file
    \brief ARM debugging interface

    \verbatim
    Copyright (C) 2009  Peter O'Donoghue

    Some of the following is based on material from OSBDM-JM60
       Target Interface Software Package
    Copyright (C) 2009  Freescale

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

    Change History
+==================================================================================
|  1 Aug 2012 | Created SWD version
+==================================================================================
\endverbatim
*/
#include "string.h"
#include "stdlib.h"
#include "assert.h"

#include "Common.h"
#include "Utils.h"
#include "Log.h"
#include "Conversion.h"
#ifdef DLL
#undef DLL
#endif
#include "USBDM_API.h"
#define DLL
#include "USBDM_SWD_API.h"
#include "ArmDebug.h"
#include "Utils.h"
#include "ARM_Definitions.h"
#include "Names.h"

// Debug MCU configuration register STM32F100xx
#define DBGMCU_CR  (0xE0042004)

void SWD_SetLogFile(FILE *fp) {
   setLogFileHandle(fp);
}

//! Obtain a 32-bit Little-Endian value from character array
//!
//! @param data - array of 4 bytes
//!
//! @return 32-bit value
//!
static inline uint32_t getData32Le(uint8_t *data) {
   return (data[0])+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}

//! Information describing the debug Interface
static struct DebugInformation {
   // Details from AHB-AP
   uint8_t          componentClass;              //!< Component class
   uint32_t         debugBaseaddr;               //!< Base address of Debug ROM
   unsigned         size4Kb;                     //!< Size of Debug ROM
   bool             MDM_AP_present;              //!< Indicates if target has Kinetis MDM-AP
   // Memory interface capabilities
   bool             memAccessLimitsChecked;      //!< Have they been checked?
   bool             byteAccessSupported;         //!< Byte/Halfword access supported?
   uint32_t         memAPConfig;                 //!< Memory CFG register contents
} debugInformation = {
      0x0,
      0x0,
      0,
      false,
      false,
      false,
      0x0,
};

//! Reads a word from target memory
//!
//! @note Breaks transfers on 2**10 boundary as TAR may not increment across this boundary
//!
//! @param address  - 32-bit starting address (aligned)
//! @param data     - pointer to buffer for word
//!
//! @return error code
//!
static USBDM_ErrorCode armReadMemoryWord(uint32_t address, uint32_t *data) {
   if (address != 0xE000EDF0) {
      print("     readMemoryWord(a=0x%08X)\n", address);
   }
   uint8_t buff[4];
   USBDM_ErrorCode rc = USBDM_ReadMemory(4,4,address, buff);
   *data = getData32Le(buff);
   return rc;
}

//! Writes a word to target memory
//!
//! @note Assumes aligned address
//! @note Breaks transfers on 2**10 boundary as TAR may not increment across this boundary
//!
//! @param address - 32-bit address
//! @param data    - ptr to buffer containing words
//!
//! @return error code
//!
static USBDM_ErrorCode armWriteMemoryWord(uint32_t address, const uint32_t *data) {
   print("     writeMemoryWord(a=0x%08X)\n", address);
   uint8_t buff[4] = {(*data)&0xFF, (*data>>8)&0xFF, (*data>>16)&0xFF, (*data>>24)&0xFF};
   USBDM_ErrorCode rc = USBDM_WriteMemory(4, 4, address, buff);
   return rc;
}

//! Read Information that describes the debug interface
//!
static USBDM_ErrorCode armReadDebugInformation(void) {
   USBDM_ErrorCode rc;
   unsigned long dataIn;

   print("   readDebugInformation()\n");

   // Check if Kinetis MDM-AP is present
   rc = USBDM_ReadCReg(ARM_CRegMDM_AP_Ident, &dataIn);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   debugInformation.MDM_AP_present = (dataIn == 0x001C0000);
   if (debugInformation.MDM_AP_present) {
      print("   readDebugInformation(): MDM-AP (Kinetis) found (Id=0x%08X)\n", dataIn);
   }
   // Check if ARM AMBA-AHB-AP present
   rc = USBDM_ReadCReg(ARM_CRegAHB_AP_Id, &dataIn);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   if ((dataIn&0x0FFF000F)!= 0x04770001) {
      print("   readDebugInformation(): AMBA-AHB-AP not found (Id=0x%08X)!\n", dataIn);
      return BDM_RC_ARM_ACCESS_ERROR;
   }
   print("   readDebugInformation(): AMBA-AHB-AP found (Id=0x%08X)\n", dataIn);
   // Save the AHB_AP_CFG register
   rc = USBDM_ReadCReg(AHB_AP_CFG, &dataIn);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   debugInformation.memAPConfig = dataIn;
   bool bigEndian = (dataIn&AHB_AP_CFG_BIGENDIAN)!=0;
   print("   readDebugInformation(): AHB_AP.CFG => 0x%08X, %s\n",
         dataIn, bigEndian?"BigEndian":"LittleEndian");

   // Get Debug base address
   rc = USBDM_ReadCReg(AHB_AP_Base, &dataIn);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   print("   readDebugInformation(): AHB_AP.Base => 0x%08X\n", dataIn);
   debugInformation.debugBaseaddr = dataIn & 0xFFFFF000;

   // Read ID registers
   uint32_t buffer[4];
   for (int index=0; index<4; index++) {
      rc = armReadMemoryWord(debugInformation.debugBaseaddr+0xFF0+4*index, buffer+index);
      if (rc != BDM_RC_OK) {
         return rc;
      }
   }
   uint32_t id;
   id  = (buffer[0x0]&0xFF);
   id += (buffer[0x1]&0xFF)<<8;
   id += (buffer[0x2]&0xFF)<<16;
   id += (buffer[0x3]&0xFF)<<24;

   print("   armReadDebugInformation(): ID => 0x%08X\n", id);
   if ((id & 0xFFFF0FFF) != 0xB105000D) {
      print("   armReadDebugInformation(): ID invalid\n");
   }
   debugInformation.componentClass = (id>>12)&0xF;
   print("   armReadDebugInformation(): component class => 0x%X\n", debugInformation.componentClass);

   // Read Peripheral ID0 register
   rc = armReadMemoryWord(debugInformation.debugBaseaddr+0xFD0, buffer);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   id  = (buffer[0x0]>>4)&0xFF;
   debugInformation.size4Kb = 1<<id;
   print("   armReadDebugInformation(): 4Kb size => %d\n", debugInformation.size4Kb);

   return BDM_RC_OK;
}

static bool SWD_InitialiseDone = false;

//! Initialise ARM interface
//!
USBDM_ErrorCode SWD_Initialise() {
   USBDM_ErrorCode rc = BDM_RC_OK;
   unsigned long dataIn;

   print("SWD_Initialise()\n");

   SWD_InitialiseDone = false;

   // Check for target power
   USBDMStatus_t status;
   rc = USBDM_GetBDMStatus(&status);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   if ((status.power_state != BDM_TARGET_VDD_EXT)&&(status.power_state != BDM_TARGET_VDD_INT)) {
      return BDM_RC_VDD_NOT_PRESENT;
   }
   // Some manufacturer's recommend doing setup with RESET active
//   USBDM_ControlPins(PIN_RESET_LOW);

   // Connect to target
   rc = USBDM_Connect();
   if (rc != BDM_RC_OK) {
      return rc;
   }
   // Power up system & debug interface
   rc = USBDM_WriteDReg(SWD_DRegCONTROL, CSYSPWRUPREQ|CDBGPWRUPREQ);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   // Wait for power-up ACKs
   int retry = 20;
   do {
      rc = USBDM_ReadDReg(SWD_DRegSTATUS, &dataIn);
      print("   SWD_Initialise() DP_ControlStatus= 0x%08X\n", dataIn);
      milliSleep(100);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      if ((dataIn & (CSYSPWRUPACK|CDBGPWRUPACK)) == (CSYSPWRUPACK|CDBGPWRUPACK)) {
         break;
      }
   } while(retry-- > 0);
   if ((dataIn & (CSYSPWRUPACK|CDBGPWRUPACK)) != (CSYSPWRUPACK|CDBGPWRUPACK)) {
      return BDM_RC_ARM_PWR_UP_FAIL;
   }
   print("SWD_Initialise() System & Debug PWR-UP OK\n");

   // Read & check debug features
   rc = armReadDebugInformation();
   if (rc != BDM_RC_OK) {
      return rc;
   }
   SWD_InitialiseDone = true;

   return rc;
}

//! Connect to ARM Target
//!
USBDM_ErrorCode SWD_Connect() {

   print("SWD_Connect()\n");

   USBDM_ErrorCode rc;
   int retry = 0;
   do {
      if (!SWD_InitialiseDone || (retry >= 1)) {
         rc = SWD_Initialise();
         if (rc != BDM_RC_OK) {
            continue;
         }
      }
      rc = USBDM_Connect();
      if (rc == BDM_RC_OK) {
         break;
      }
   } while (retry < 2);
   retry = 4;
#if 1
   do {
      uint32_t debugOnValue = DHCSR_DBGKEY|DHCSR_C_DEBUGEN|DHCSR_C_HALT;
      rc = armWriteMemoryWord(DHCSR, &debugOnValue);
      if (rc != BDM_RC_OK) {
         print("SWD_Connect() DHCSR write failed\n");
         continue;
      }
      uint32_t dataIn;
      if (rc == BDM_RC_OK) {
         rc = armReadMemoryWord(DHCSR, &dataIn);
         if (rc != BDM_RC_OK) {
            print("   SWD_Connect() DHCSR read failed\n");
            continue;
         }
         else {
            print("SWD_Connect() DHCSR value = %s(0x%08X)\n", getDHCSRName(dataIn), dataIn);
         }
      }
      if ((dataIn&DHCSR_C_DEBUGEN) == 0) {
         print("SWD_Connect() Debug enable failed\n");
         // May indicate the device is secured
         if (debugInformation.MDM_AP_present) {
               unsigned long mdmApStatus;
               rc = USBDM_ReadCReg(ARM_CRegMDM_AP_Status, &mdmApStatus);
               if ((rc == BDM_RC_OK) && ((mdmApStatus & MDM_AP_System_Security) != 0)) {
                  print("   SWD_Connect()- Checking Freescale MDM-AP - device is secured\n");
                  return BDM_RC_SECURED;
               }
         }
         continue;
      }
      else {
         print("   SWD_Connect() Debug Enable complete\n");
         break;
      }
   } while (--retry >0);
   if (retry == 0) {
      return BDM_RC_BDM_EN_FAILED;
   }
   return BDM_RC_OK;
#endif
}

//! Get ARM target status
//!
//! @param status - true => halted, false => running
//!
USBDM_ErrorCode SWD_GetStatus(ArmStatus *status) {
   USBDM_ErrorCode rc;
   uint32_t dataIn;
   ArmStatus defaultStatus = {0,0xFFFFFFFF,0,0};

   print("   SWD_GetStatus()\n");

   //ToDo - Consider Kinetis specific MDM_AP_Status
   *status = defaultStatus;

   if (debugInformation.MDM_AP_present) {
      // Freesale MDM-AP present
      unsigned long mdmStatus;
      rc = USBDM_ReadCReg(ARM_CRegMDM_AP_Status, &mdmStatus);
      if (rc != BDM_RC_OK) {
         print("   SWD_GetStatus() Can't read MDM_AP_Status!\n");
         return BDM_RC_BDM_EN_FAILED;
      }
      status->mdmApStatus = mdmStatus;
   }

   //      *status = (dataIn&MDM_AP_Status_Core_Halted) != 0;
   //      print("   SWD_GetStatus() => MDM_AP_Status=0x%08X (%s)\n",
   //            dataIn,
   //            (*status)?"Halted":"Running");
   //      return BDM_RC_OK;
   //   }
   //   else {
   rc = armReadMemoryWord(DEMCR, &dataIn);
   if (rc != BDM_RC_OK) {
      print("   SWD_GetStatus() Can't read DEMCR!\n");
   }
   else {
      print("   SWD_GetStatus(): DEMCR status=%s(0x%08X)\n", getDEMCRName(dataIn), dataIn);
   }
   // Generic Debug
   rc = armReadMemoryWord(DHCSR, &dataIn);
   if (rc != BDM_RC_OK) {
      print("   SWD_GetStatus() Can't read DHCSR!\n");
      return BDM_RC_BDM_EN_FAILED;
   }
   //      print("   SWD_GetStatus() DHCSR value = 0x%08X\n", dataIn);
   //      if ((dataIn&DHCSR_C_DEBUGEN) == 0) {
   //         print("   SWD_Initialise() Debug enable not set!\n");
   //         return BDM_RC_BDM_EN_FAILED;
   //      }
   if ((dataIn&DHCSR_S_LOCKUP) != 0) {
      const uint32_t dataOut = DHCSR_DBGKEY|DHCSR_C_HALT|DHCSR_C_DEBUGEN;
      print("   SWD_GetStatus() Clearing Lockup, DHCSR status=%s(0x%08X)\n", getDHCSRName(dataIn), dataIn);
      armWriteMemoryWord(DHCSR, &dataOut);
      rc = armReadMemoryWord(DHCSR, &dataIn);
      if (rc != BDM_RC_OK) {
         print("   SWD_GetStatus() Can't read DHCSR!\n");
         return BDM_RC_BDM_EN_FAILED;
      }
   }
   status->dhcsr = dataIn;

   print("   SWD_GetStatus() => DHCSR status=%s(0x%08X) (%s)\n",
         getDHCSRName(dataIn), dataIn,
         (dataIn&(DHCSR_S_HALT|DHCSR_S_LOCKUP))?"Halted":"Running");
   if (debugInformation.MDM_AP_present) {
      print("                   => MDM_AP=%s(0x%08X)\n",
            getMDM_APStatusName(status->mdmApStatus), status->mdmApStatus);
   }
   return BDM_RC_OK;
   //   }
}
