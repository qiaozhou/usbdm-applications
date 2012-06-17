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
|  9 Apr 2012 | Added USBDM_ExtendedOptions_t use                            V4.9.4
|  9 Apr 2012 | Added RESET_DEFAULT to TargetReset()                         V4.9.4
| 18 Feb 2012 | Checked for target Vdd in ARM_Initialise                     V4.9.1
| 19 Oct 2011 | ARM_WriteMemory() alignment bug                              V4.7.2
| 27 Feb 2011 | Created
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
#include "JTAGSequence.h"
#ifdef DLL
#undef DLL
#endif
#include "USBDM_API.h"
#define DLL
#include "USBDM_ARM_API.h"
#include "ArmDebug.h"
#include "Utils.h"
#include "JTAGUtilities.h"
#include "ARM_Definitions.h"
#include "Names.h"

typedef enum {TARGET_UNKNOWN, TARGET_KINETIS, TARGET_STM32F10x} TargetSubTypesT;

// Debug MCU configuration register STM32F100xx
#define DBGMCU_CR  (0xE0042004)

// Create a 16-bit (compressed) address for MEM-AP access
#define ADDR16(x) (((x)>>24U)&0xFFUL),((x)&0xFFUL)
//
#define DATA32(x) (((x)>>24U)&0xFFUL),(((x)>>16)&0xFFUL),(((x)>>8)&0xFFUL),((x)&0xFFUL)
#define JTAG16(x) (((x)>>8)&0xFF),((x)&0xFF)

static bool debugJTAG = false;

#define JTAG_READ_MEMORY_HEADER_SIZE  (10) // Overhead used by readMemory()
#define JTAG_WRITE_MEMORY_HEADER_SIZE (10) // Overhead used by writeMemory()

typedef struct {
   bool     initialised;
   unsigned maxMemoryWriteSize;  //!< Maximum #bytes for DSC_WriteMemory()
   unsigned maxMemoryReadSize;   //!< Maximum #bytes for DSC_ReadMemory()
} ArmInfo_t ;

//! Information describing the interface
//!
static ArmInfo_t armInfo = {false, 100, 100};
USBDM_ExtendedOptions_t bdmOptions;

static USBDM_ErrorCode readMemoryWord(uint32_t address, unsigned numWords, uint32_t *data);
static USBDM_ErrorCode writeMemoryWord(uint32_t address, unsigned numWords, const uint32_t *data);
static USBDM_ErrorCode readAP(uint32_t dapAddress, uint32_t *dataIn);

void ARM_SetLogFile(FILE *fp) {
   setLogFileHandle(fp);
//   print("ARM_SetLogFile()\n");
}

//! Obtain a 32-bit Big-Endian value from character array
//!
//! @param data - array of 4 bytes
//!
//! @return 32-bit value
//!
static inline uint32_t getData32Be(uint8_t *data) {
   return (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
}

//! Obtain information about the ARM interface
//!
//! @param armInfo_ structure to populate (may be NULL)
//!
//! @note the size entry in info must be set before the call
//!
static USBDM_ErrorCode GetBdmInfo(void) {
   USBDM_ErrorCode rc;

   if (armInfo.initialised)
      return BDM_RC_OK;

   USBDM_bdmInformation_t bdmInfo = {sizeof(USBDM_bdmInformation_t), 0};

   // Get maximum JTAG sequence length
   rc = USBDM_GetBdmInformation(&bdmInfo);
   if (rc != BDM_RC_OK)
      return rc;

   // Calculate permitted read & write length in bytes
   // Allow for JTAG header + USB header (5 bytes) & make multiple of 4
   armInfo.maxMemoryReadSize  = (bdmInfo.jtagBufferSize-JTAG_READ_MEMORY_HEADER_SIZE-8)  & ~3;
   armInfo.maxMemoryWriteSize = (bdmInfo.jtagBufferSize-JTAG_WRITE_MEMORY_HEADER_SIZE) & ~3;
   armInfo.initialised = true;

   print("   GetBdmInfo(): usbdmBufferSize = %u\n", bdmInfo.jtagBufferSize);
   print("   GetBdmInfo(): JTAG_READ_MEMORY_HEADER_SIZE=%d, MaxDataSize = %u\n",
         JTAG_READ_MEMORY_HEADER_SIZE, armInfo.maxMemoryReadSize);
   print("   GetBdmInfo(): JTAG_WRITE_MEMORY_HEADER_SIZE=%d, MaxDataSize = %u\n",
         JTAG_WRITE_MEMORY_HEADER_SIZE, armInfo.maxMemoryWriteSize);

   return BDM_RC_OK;
}

//! Read IDCODE from JTAG TAP
//!
//! @param idCode   - 32-bit IDCODE returned from TAP
//! @param resetTAP - Optionally resets the TAP to RUN-TEST/IDLE before reading IDCODE
//!                   This will enable the MASTER TAP!
//!
//! @note - resetTAP=true will enable the Master TAP & disable the Code TAP
//! @note - Leaves Core TAP in RUN-TEST/IDLE
//!
static USBDM_ErrorCode readIDCODE(uint32_t *idCode, uint8_t command, uint8_t length, bool resetTAP) {
   // Sequence using readIdcode command to read IDCODE
   uint8_t readCoreIdCodeSequence[] = {
      JTAG_MOVE_IR_SCAN,                              // Write IDCODE command to IR
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(ARM_JTAG_MASTER_IR_LENGTH), command,
      JTAG_SET_EXIT_IDLE,                             // Read IDCODE from DR
      JTAG_SHIFT_IN_Q(length),
      JTAG_END
   };
   // Sequence using Test-Logic-Reset to read IDCODE
   uint8_t readCoreIdCodeByResetSequence[] = {
      JTAG_TEST_LOGIC_RESET,     // Reset TAP (loads IDCODE)
      JTAG_MOVE_DR_SCAN,         // Move to scan DR reg
      JTAG_SET_EXIT_IDLE,        // Exit to idle afterwards
      JTAG_SHIFT_IN_Q(length),   // Read IDCODE from DR
      JTAG_END
   };
   JTAG32 idcode(0,32);
   USBDM_ErrorCode rc;
   if (resetTAP)
      rc = executeJTAGSequence(sizeof(readCoreIdCodeByResetSequence), readCoreIdCodeByResetSequence,
                               4, idcode.getData(32), debugJTAG);
   else
      rc = executeJTAGSequence(sizeof(readCoreIdCodeSequence), readCoreIdCodeSequence,
                               4, idcode.getData(32), debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   readIDCODE() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   *idCode = idcode;
   return rc;
}
#if 0
//! Write Abort value to ABORT register
//!
//! @note - Expects TAP in RUN-TEST/IDLE
//! @note - Leaves TAP in RUN-TEST/IDLE
//!
static USBDM_ErrorCode writeAbort(void) {
   uint8_t sequence[] = {
      // Write DPACC_SEL command to IR, move to DR scan
      JTAG_MOVE_IR_SCAN,
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(ARM_JTAG_MASTER_IR_LENGTH), JTAG_DP_ABORT_SEL_COMMAND,
      // Write operation/read status, stay in shift-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_OUT_Q(3), 0x00,
      // Write rest of data, move idle afterwards
      JTAG_SET_EXIT_IDLE,
      JTAG_SHIFT_OUT_Q(32), 0x00, 0x00, 0x00, 0x01,
      JTAG_END,
   };
   USBDM_ErrorCode rc;
   rc = executeJTAGSequence(sizeof(sequence), sequence, 0, NULL, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   writeAbort() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   print("   writeABORT()\n");
   return rc;
}
#endif

//! write DPACC Control/Status register
//!
//! @param dataOut   - 32-bit data value to send
//!
static USBDM_ErrorCode writeDP_ControlStatus(uint32_t dataOut) {
   uint8_t sequence[] = {
      // Write DPACC_SEL command to IR, move to JTAG_SHIFT_DR
      JTAG_MOVE_IR_SCAN,
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(ARM_JTAG_MASTER_IR_LENGTH), JTAG_DP_DPACC_SEL_COMMAND,

      // Write operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_WRITE|DP_CTRL_STAT_REG,                            // 1
      // Write/read data, exit & re-enter SHIFT-DR afterwards
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(32), dataOut>>24, dataOut>>16, dataOut>>8, dataOut,

      // Write dummy-read operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_RDBUFF_REG,                                // 1
      // Read data, exit to IDLE after
      JTAG_SET_EXIT_IDLE,
      JTAG_SHIFT_OUT_VARA, 32,      // Send random data, discard received

      JTAG_END,
   };
   USBDM_ErrorCode rc;
   uint8_t inBuffer[20];
   rc = executeJTAGSequence(sizeof(sequence), sequence, 2, inBuffer, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   writeDP_ControlStatus() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   // Check transfer
   uint8_t ack1 = inBuffer[0];
   uint8_t ack2 = inBuffer[1];
   if ((ack1 != ACK_OK_FAULT) || (ack2 != ACK_OK_FAULT)) {
      print("   writeDP_ControlStatus() - Failed, ACKs incorrect, A1=%X, A2=%X\n",
            ack1, ack2);
      return BDM_RC_ARM_ACCESS_ERROR;
   }
   return BDM_RC_OK;
}

//! read DPACC Control/Status register
//!
//! @param dataIn    - 32-bit data value received (NULL if not required)
//!
static USBDM_ErrorCode readDP_ControlStatus(uint32_t *dataIn) {
   uint8_t sequence[] = {
      // Write DPACC_SEL command to IR, move to JTAG_SHIFT_DR
      JTAG_MOVE_IR_SCAN,
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(ARM_JTAG_MASTER_IR_LENGTH), JTAG_DP_DPACC_SEL_COMMAND,

      // Write operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_CTRL_STAT_REG,                            // 1
      // Write/read data, exit & re-enter SHIFT-DR afterwards
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_VARA, 32,      // Send random data, discard received

      // Write dummy-read operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_RDBUFF_REG,                               // 1
      // Read data, exit to IDLE after
      JTAG_SET_EXIT_IDLE,
      JTAG_SHIFT_IN_Q(32),      // Capture received data                           // 4

      JTAG_END,
   };
   USBDM_ErrorCode rc;
   uint8_t inBuffer[20];
   rc = executeJTAGSequence(sizeof(sequence), sequence, 6, inBuffer, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   readDP_ControlStatus() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   // Check transfer
   uint8_t ack1 = inBuffer[0];
   uint8_t ack2 = inBuffer[1];
   if ((ack1 != ACK_OK_FAULT) || (ack2 != ACK_OK_FAULT)) {
      print("   readDP_ControlStatus() - Failed, ACKs incorrect, A1=%X, A2=%X\n",
            ack1, ack2);
      return BDM_RC_ARM_ACCESS_ERROR;
   }
   *dataIn = (inBuffer[2]<<24)+(inBuffer[3]<<24)+(inBuffer[4]<<16)+inBuffer[5];
   return BDM_RC_OK;
}

//! read DPACC Control/Status register
//!
//! @param dataIn    - 32-bit data value received (NULL if not required)
//!
static USBDM_ErrorCode readDP_All(void) {
   uint8_t sequence[] = {
      // Write DPACC_SEL command to IR, move to JTAG_SHIFT_DR
      JTAG_MOVE_IR_SCAN,
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_Q(ARM_JTAG_MASTER_IR_LENGTH), JTAG_DP_DPACC_SEL_COMMAND,

      // Write operation+dummy/read status+dummy, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_CTRL_STAT_REG,                            // 1 - [0]
      // Write/read data, exit & re-enter SHIFT-DR afterwards
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_OUT_VARA, 32,      // Send random data, discard received

      // Write dummy-read operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_SELECT_REG,                               // 1 - [1]
      // Write/read data, exit & re-enter SHIFT-DR afterwards
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_IN_Q(32),      // Capture received data                           // 4 - [2..5]

      // Write dummy-read operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_RDBUFF_REG,                               // 1 - [6]
      // Write/read data, exit & re-enter SHIFT-DR afterwards
      JTAG_SET_EXIT_SHIFT_DR,
      JTAG_SHIFT_IN_Q(32),      // Capture received data                           // 4 - [7..10]

      // Write dummy-read operation/read status, stay in SHIFT-DR
      JTAG_SET_STAY_SHIFT,
      JTAG_SHIFT_IN_OUT_Q(3), DP_READ|DP_RDBUFF_REG,                               // 1 - [11]
      // Read data, exit to IDLE after
      JTAG_SET_EXIT_IDLE,
      JTAG_SHIFT_IN_Q(32),      // Capture received data                           // 4 - [12..15]

      JTAG_END,
   };
   USBDM_ErrorCode rc;
   uint8_t inBuffer[40];
   rc = executeJTAGSequence(sizeof(sequence), sequence, 16, inBuffer, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   readDP_ControlStatus() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   printDump(inBuffer, 16);

   // Check transfer
   if ((inBuffer[0] != ACK_OK_FAULT) || (inBuffer[1] != ACK_OK_FAULT) || (inBuffer[6] != ACK_OK_FAULT) ||
       (inBuffer[11] != ACK_OK_FAULT)) {
      print("   readDP_All() - Failed, ACKs incorrect\n");
   }
   print("   readDP_All() - A0=%2X, A-CTRL/STAT=%2X, A-SEL=%2X, A-RDBUGG=%2X \n",
         inBuffer[0], inBuffer[1], inBuffer[6], inBuffer[11]);
   print("   readDP_All() - CTRL/STAT=%08X, SEL=%08X, RDBUFF=%08X\n",
         getData32Be(inBuffer+2), getData32Be(inBuffer+7), getData32Be(inBuffer+12));

   return BDM_RC_OK;
}

//! Check sticky error status
//!
//! @param statusValue - Value read from DP control/status register
//!
static USBDM_ErrorCode checkSticky(uint32_t statusValue) {
   static int recurse = 0;
   if ((statusValue & STICKYERR) != 0) {
      print("   checkSticky() - Failed, DP_STAT => 0x%08X\n", statusValue);
      if (recurse==0) {
         recurse++;
         readDP_All(); // For debug
         // Clear sticky flag
         writeDP_ControlStatus(statusValue|STICKYERR);
         uint32_t tarValue;
         readAP(AHB_AP_TAR, &tarValue);
         print("   TAR = 0x%08X\n", tarValue);
         uint32_t cswValue;
         readAP(AHB_AP_CSW, &cswValue);
         print("   CSW = 0x%08X\n", cswValue);
         recurse--;
      }
      else {
         print("   checkSticky() - Recursion\n");
      }
      return BDM_RC_ARM_ACCESS_ERROR;
   }
   return BDM_RC_OK;
}

//! Check sticky error status
//!
//! @param inBuffer - Value read from DP control/status register as 4 consecutive bytes (LE-32)
//!
static USBDM_ErrorCode checkSticky(uint8_t inBuffer[4]) {
   uint32_t status = (inBuffer[0]<<24)+(inBuffer[1]<<16)+(inBuffer[2]<<8)+inBuffer[3];
   return checkSticky(status);
}

#if 0
//! Check sticky error status by reading DP control/status
//!
//! Reads the DP control/status register from target & check for errors
//!
static USBDM_ErrorCode checkSticky(void) {
   uint32_t dataIn;
   USBDM_ErrorCode rc;
   rc = readDP_ControlStatus(&dataIn);
   if (rc != BDM_RC_OK) {
      print("checkSticky() - Failed, can't read Control/Status!\n");
      return BDM_RC_ARM_ACCESS_ERROR;
   }
   return checkSticky(dataIn);
}
#endif

//! Write to AP register (sets AP_SELECT & APACC)
//!
//! @param address - The address is divided between: \n
//!    A[31:24] => DP-AP-SELECT[31:24] (AP Select) \n
//!    A[7:4]   => DP-AP-SELECT[7:4]   (Bank select within AP) \n
//!    A[3:2]   => APACC[3:2]          (Register select within bank)
//! @param value - the value to write to AP register
//!
static USBDM_ErrorCode writeAP(uint32_t dapAddress, uint32_t dataOut) {
   const uint8_t jtagSequence[] = {
      JTAG_ARM_WRITEAP, 1, ADDR16(dapAddress),   // Write AP register
      JTAG_END,
      DATA32(dataOut),
   };
   uint8_t inBuffer[4];
   switch (dapAddress) {
   case MDM_AP_Status:  print("   writeAP(MDM-AP.Status,  D=%s(0x%08X)\n", getMDM_APStatusName(dataOut), dataOut);  break;
   case MDM_AP_Control: print("   writeAP(MDM-AP.Control, D=%s(0x%08X)\n", getMDM_APControlName(dataOut),dataOut);  break;
   default:             print("   writeAP(A=0x%08X, D=0x%08X)\n", dapAddress, dataOut); break;
   }
   USBDM_ErrorCode rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4, inBuffer, debugJTAG);
   if (rc == BDM_RC_OK) {
      rc = checkSticky(inBuffer);
   }
   if (rc != BDM_RC_OK) {
      print("   writeAP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   return BDM_RC_OK;
}

//! Reads from AP register
//!
//! @param address - The address is divided between: \n
//!    A[31:24] => DP-AP-SELECT[31:24] (AP Select) \n
//!    A[7:4]   => DP-AP-SELECT[7:4]   (Bank select within AP) \n
//!    A[3:2]   => APACC[3:2]          (Register select within bank)
//! @param value - where to place the value read
//!
static USBDM_ErrorCode readAP(uint32_t dapAddress, uint32_t *dataIn) {
   const uint8_t jtagSequence[] = {
      JTAG_ARM_READAP, 1, ADDR16(dapAddress),   // Read AP register
      JTAG_END,
   };
   uint8_t inBuffer[4+4];
   USBDM_ErrorCode rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4+4, inBuffer, debugJTAG);
   if (rc == BDM_RC_OK) {
      rc = checkSticky(inBuffer+4);
   }
   *dataIn = (inBuffer[0]<<24)+(inBuffer[1]<<16)+(inBuffer[2]<<8)+inBuffer[3];
   if (rc != BDM_RC_OK) {
      print("   readAP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   else {
      switch (dapAddress) {
      case MDM_AP_Status:  print("   readAP(MDM-AP.Status,  D=%s(0x%08X)\n", getMDM_APStatusName(*dataIn), *dataIn);  break;
      case MDM_AP_Control: print("   readAP(MDM-AP.Control, D=%s(0x%08X)\n", getMDM_APControlName(*dataIn),*dataIn);  break;
      default:             print("   readAP(A=0x%08X, D=0x%08X)\n", dapAddress, *dataIn); break;
      }
   }
   return BDM_RC_OK;
}

//! Information describing the debug Interface
static struct DebugInformation {
   // Details from AHB-AP
   uint8_t               componentClass;              //!< Component class
   uint32_t              debugBaseaddr;               //!< Base address of Debug ROM
   unsigned         size4Kb;                     //!< Size of Debug ROM
   TargetSubTypesT  subType;                     //!< Identifies sub-type
   // Memory interface capabilities
   bool             memAccessLimitsChecked;      //!< Have they been checked?
   bool             byteAccessSupported;         //!< Byte/Halfword access supported?
   bool             bigEndian;                   //!< Memory is bigEndian?
   uint32_t              memAPControlStatusDefault;   //!< Default value to use when writing to APControlStatus Register
   uint32_t              memAPConfig;                 //!< Memory CFG register contents
} debugInformation = {
      0x0,
      0x0,
      0,
      TARGET_UNKNOWN,
      false,
      false,
      0x0,
      0x0,
};

//! Checks if byte & halfword memory accesses are supported by MEM-AP
//!
//! @return true/false value
//!
static bool byteAccessSupported(void) {
   const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_BYTE;
   const uint8_t jtagSequence[] = {
      JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue), // Write CSW
      JTAG_ARM_READAP, 1, ADDR16(AHB_AP_CSW),                   // Read back CSW
      JTAG_END,
   };
   if (!debugInformation.memAccessLimitsChecked) {
      debugInformation.memAccessLimitsChecked = true;
      // Check CSW
      uint8_t inBuffer[8];
      USBDM_ErrorCode rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4+4, inBuffer, debugJTAG);
      if (rc != BDM_RC_OK) {
         print("   byteAccessSupported() - Failed, access = %s\n", USBDM_GetErrorString(rc));
         return false;
      }
      uint32_t dataIn = (inBuffer[0]<<24)+(inBuffer[1]<<16)+(inBuffer[2]<<8)+inBuffer[3];
      if ((dataIn&AHB_AP_CSW_SIZE_MASK) != AHB_AP_CSW_SIZE_BYTE) {
         print("byteAccessSupported(): Hardware byte access not supported\n");
      }
      else {
         debugInformation.byteAccessSupported = true;
//         print("byteAccessSupported(): Hardware byte access support enabled\n");
      }
   }
   return debugInformation.byteAccessSupported;
}

//! Read a byte from target memory
//!
//! @param address - 32-bit address
//! @param data    - ptr to buffer for byte
//!
//! @return error code
//!
static USBDM_ErrorCode readMemoryByte(uint32_t address, uint8_t *data) {

   USBDM_ErrorCode rc;
   uint8_t   inBuffer[8];

//   print("     readMemoryByte(a=0x%08X)\n", address);

   if (!byteAccessSupported()) {
      // Read aligned word & extract required portion
      uint32_t inData;
      rc = readMemoryWord(address&~0x3, 1, &inData);
      if (rc != BDM_RC_OK) {
         print("   readMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      switch (address&0x3) {
      case 0: *data = (uint8_t)(inData>>24); break;
      case 1: *data = (uint8_t)(inData>>16); break;
      case 2: *data = (uint8_t)(inData>>8);  break;
      case 3: *data = (uint8_t)(inData);     break;
      }
//      print("     readMemoryByte-32(): 0x%04X\n", *data);
   }
   else {
      const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_BYTE;
      const uint8_t jtagSequence[] = {
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue), // Setup Control Status Word
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),  // Setup transfer address
         JTAG_ARM_READAP, 1, ADDR16(AHB_AP_DRW),                   // Do transfer & get final CSW
         JTAG_END,
      };
      // Read data from memory via DRW + status
      rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4+4, inBuffer, debugJTAG);
      if (rc != BDM_RC_OK) {
         print("   readMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      rc = checkSticky(inBuffer+4);
      if (rc != BDM_RC_OK) {
         print("   readMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      if (debugInformation.bigEndian) {
         switch (address&0x3) {
         case 0: *data = inBuffer[0]; break;
         case 1: *data = inBuffer[1]; break;
         case 2: *data = inBuffer[2]; break;
         case 3: *data = inBuffer[3]; break;
         }
      }
      else { // LittleEndian
         switch (address&0x3) {
         case 3: *data = inBuffer[0]; break;
         case 2: *data = inBuffer[1]; break;
         case 1: *data = inBuffer[2]; break;
         case 0: *data = inBuffer[3]; break;
         }
      }
//      print("     readMemoryByte-HW(): 0x%04X\n", *data);
   }
   return BDM_RC_OK;
}

//! Read aligned halfword from target memory
//!
//! @param address - 32-bit address
//! @param data    - ptr to buffer for bytes
//!
//! @return error code
//!
static USBDM_ErrorCode readMemoryHalfword(uint32_t address, uint16_t *data) {
   USBDM_ErrorCode rc;

   uint8_t   inBuffer[8];

//   print("     readMemoryHalfword(a=0x%08X)\n", address);

   if (!byteAccessSupported()) {
      // Read aligned word & extract required portion
      uint32_t inData;
      rc = readMemoryWord(address&~0x3, 1, &inData);
      if (rc != BDM_RC_OK) {
         print("   readMemoryHalfword() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      if (address&0x2) {
         *data = (uint16_t)(inData>>16);
      }
      else {
         *data = (uint16_t)inData;
      }
//      print("     readMemoryHalfword-32(): 0x%04X\n", *data);
   }
   else {
      const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_HALFWORD;
      const uint8_t jtagSequence[] = {
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue), // Setup Control Status Word
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),  // Setup transfer address
         JTAG_ARM_READAP, 1, ADDR16(AHB_AP_DRW),                   // Do transfer & get final CSW
         JTAG_END,
      };
      // Read data from memory via DRW + status
      rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4+4, inBuffer, debugJTAG);
      rc = checkSticky(inBuffer+4);
      if (rc != BDM_RC_OK) {
         print("   readMemoryHalfword() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      if (debugInformation.bigEndian) {
         if (address&0x02)
            *data = (inBuffer[2]<<8)+inBuffer[3];
         else
            *data = (inBuffer[0]<<8)+inBuffer[1];
      }
      else { // LittleEndian
         if (address&0x02)
            *data = (inBuffer[0]<<8)+inBuffer[1];
         else
            *data = (inBuffer[2]<<8)+inBuffer[3];
      }
//      print("     readMemoryHalfword-HW(): 0x%04X\n", *data);
   }
   return BDM_RC_OK;
}

//! Read words from target memory
//!
//! @note Breaks transfers on 2**10 boundary as TAR may not increment across this boundary
//!
//! @param address - 32-bit starting address
//! @param data    - ptr to buffer for bytes
//!
//! @return error code
//!
static USBDM_ErrorCode readMemoryWord(uint32_t address, unsigned numWords, uint32_t *data) {
   USBDM_ErrorCode rc;
//   if (address != 0xE000EDF0)
//      print("     readMemoryWord(a=0x%08X-0x%08X,nW=0x%X,nB=0x%X)\n", address, address+(4*numWords)-1, numWords, 4*numWords);

   const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_WORD|AHB_AP_CSW_INC_SINGLE;
   bool initialTransfer   = true;
   bool writeAddressToTAR = true;
   uint8_t   inBuffer[armInfo.maxMemoryReadSize];
   uint8_t   outBuffer[armInfo.maxMemoryWriteSize];

   while (numWords > 0) {
      const int tSize = 5;
      bool crossedBoundary = false;
      uint8_t *outDataPtr = outBuffer;
      uint8_t transferSize = armInfo.maxMemoryReadSize-4; // Allow for return status value
      if (initialTransfer) {
//         print("     readMemoryWord() - CSW <= 0x%08X)\n", cswValue);
         // Initial transfer includes CSW register setup
         const uint8_t initSequence[] = {
            JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue),
         };
         memcpy(outDataPtr, initSequence, sizeof(initSequence));
         outDataPtr   += sizeof(initSequence);
         transferSize -= sizeof(initSequence);
      }
      if (writeAddressToTAR) {
//         print("     readMemoryWord() - TAR <= 0x%08X)\n", address);
         // Transfer includes TAR register setup
         const uint8_t tarSequence[] = {
            JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),
         };
         memcpy(outDataPtr, tarSequence, sizeof(tarSequence));
         outDataPtr   += sizeof(tarSequence);
         transferSize -= sizeof(tarSequence);
      }
      transferSize -= tSize;
      if (transferSize>4*numWords)
         transferSize = 4*numWords;
      // Check if range crosses page boundary
      uint32_t bytesRemainingInPage = ((address+ARM_PAGE_SIZE)&~(ARM_PAGE_SIZE-1)) - address;
      if (transferSize>=bytesRemainingInPage) {
         crossedBoundary = true;
         transferSize = bytesRemainingInPage;
//         print("     readMemoryWord(a=0x%08X-0x%08X) - limiting to page boundary\n",
//               address, address+transferSize-1);
      }
      transferSize /= 4;
      const uint8_t transferSequence[] = {
         JTAG_ARM_READAP, transferSize, ADDR16(AHB_AP_DRW), // Do transfer & get final CSW
         JTAG_END,
      };
      assert(sizeof(transferSequence)==tSize);
      memcpy(outDataPtr, transferSequence, sizeof(transferSequence));
      outDataPtr += sizeof(transferSequence);
      // Read data from memory via DRW + status
      rc = executeJTAGSequence(outDataPtr-outBuffer, outBuffer, 4*transferSize+4, inBuffer, debugJTAG);
      if (rc != BDM_RC_OK) {
         print("   readMemoryWord() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
//      print("     readMemoryWord() -  block(a=0x%08X-0x%08X)\n", address, address+(4*transferSize)-1);
      writeAddressToTAR = crossedBoundary;
      initialTransfer   = false;
      numWords -= transferSize;
      address += 4*transferSize;
      uint8_t *inDataPtr = inBuffer;
      // Copy data from buffer
      while (transferSize-->0) {
//       print("     readMemoryWord(a=0x%08X,n=0x%X) => inBuffer=0x%08X\n", address, numWords, *(uint32_t*)inBuffer);
         // Assemble LE -> BE words
         *data  = (inDataPtr[0]<<24)+(inDataPtr[1]<<16)+(inDataPtr[2]<<8)+inDataPtr[3];
//       print("     readMemoryWord(a=0x%08X,n=0x%X) => inBuffer=0x%08X\n", address, numWords, *data);
         inDataPtr += 4;
         data++;
      }
      rc = checkSticky(inDataPtr);
      if (rc != BDM_RC_OK) {
         print("   readMemoryWord() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
   }
   return BDM_RC_OK;
}

//! Read from target memory
//!
//! @param address   - Address to access
//! @param numBytes  - Number of bytes to transfer
//! @param data      - Ptr to data buffer
//!
//! @return error code
//!
//! @note The transfer will be decomposed to a minimal set of byte/halfword/word accesses
//!
static USBDM_ErrorCode readMemory(uint32_t address, unsigned numBytes, uint8_t *data) {
   USBDM_ErrorCode rc = BDM_RC_OK;

//   print("     readMemory(a=0x%08X, n=0x%X)\n", address, numBytes);

   // Do any odd byte at start
   if (address&0x01) {
      // Do odd byte at start
//      print("     readMemory() - Doing odd prefix byte\n");
      rc = readMemoryByte(address, data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += 1;
      address  += 1;
      numBytes -= 1;
   }
   // Do any odd halfword at start
   if ((address&0x02) && (numBytes>=2)) {
//      print("     readMemory() - Doing odd prefix halfword\n");
      rc = readMemoryHalfword(address, (uint16_t*)data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += 2;
      address  += 2;
      numBytes -= 2;
   }
   // Do middle bytes as words
   if (numBytes>=4) {
      rc = readMemoryWord(address, numBytes/4, (uint32_t*)data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += numBytes&~0x3;
      address  += numBytes&~0x3;
      numBytes &= 0x03;
   }
   // Do any odd word at end
   if (numBytes>=2) {
//      print("     readMemory() - Doing odd suffix halfword\n");
      rc = readMemoryHalfword(address, (uint16_t*)data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += 2;
      address  += 2;
      numBytes -= 2;
   }
   if (numBytes>=1) {
      // Do odd byte at end
//      print("     readMemory() - Doing odd suffix byte\n");
      rc = readMemoryByte(address, data);
      if (rc != BDM_RC_OK)
         return rc;
   }
   return BDM_RC_OK;
}

//! Write byte to target memory
//!
//! @param address - 32-bit address
//! @param data    - byte to write
//!
//! @return error code
//!
static USBDM_ErrorCode writeMemoryByte(uint32_t address, const uint8_t data) {
   USBDM_ErrorCode rc;
   print("     writeMemoryByte(a=0x%08X, d=0x%08X)\n", address, data);

   if (!byteAccessSupported()) {
      // Read aligned word & replace required portion
      uint32_t inData, outData;
      rc = readMemoryWord(address&~0x3, 1, &inData);
      if (rc != BDM_RC_OK) {
         print("   writeMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      switch (address&0x3) {
      case 3: outData = (inData & ~(0xFFUL<<24)) | (data<<24UL) ; break;
      case 2: outData = (inData & ~(0xFFUL<<16)) | (data<<16UL) ; break;
      case 1: outData = (inData & ~(0xFFUL<<8))  | (data<<8UL)  ; break;
      case 0: outData = (inData & ~(0xFFUL))     | (data)       ; break;
      }
      rc = writeMemoryWord(address&~0x3, 1, &outData);
      if (rc != BDM_RC_OK) {
         print("   writeMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
   }
   else {
      const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_BYTE;
      uint32_t outData = (data<<24UL)|(data<<16UL)|(data<<8UL)|data;
      uint8_t inBuffer[4];
      const uint8_t jtagSequence[] = {
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue), // Setup Control Status Word
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),  // Setup transfer address
         JTAG_ARM_WRITEAP, 1,ADDR16(AHB_AP_DRW),                   // Do transfer & get final CSW
         JTAG_END,
         DATA32(outData),                                          // Data to send
      };
      // Write data to memory via DRW + get status
      rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4, inBuffer, debugJTAG);
      if (rc == BDM_RC_OK) {
         rc = checkSticky(inBuffer);
      }
      if (rc != BDM_RC_OK) {
         print("   writeMemoryByte() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
   }
   return BDM_RC_OK;
}

//! Write halfword to target memory
//!
//! @param address - 32-bit address
//! @param data    - halfword to write
//!
//! @return error code
//!
static USBDM_ErrorCode writeMemoryHalfword(uint32_t address, const uint16_t data) {
   USBDM_ErrorCode rc;

   print("     writeMemoryHalfword(a=0x%08X,d=0x%04X)\n", address, data);

   if (!byteAccessSupported()) {
      // Read aligned word & modify required portion
      uint32_t inData, outData;
      rc = readMemoryWord(address&~0x3, 1, &inData);
      if (rc != BDM_RC_OK) {
         print("   writeMemoryHalfword() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
      switch (address&0x2) {
      case 2: outData = (inData & ~(0xFFFFUL<<16)) | (data<<16UL) ; break;
      case 0: outData = (inData & ~(0xFFFFUL))     | (data)       ; break;
      }
      rc = writeMemoryWord(address&~0x3, 1, &outData);
      if (rc != BDM_RC_OK) {
         print("   writeMemoryHalfword() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
   }
   else {
      const uint32_t cswValue = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_HALFWORD;
      uint32_t outData = (data<<16UL)|data;
      uint8_t inBuffer[4];
      const uint8_t jtagSequence[] = {
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue), // Setup Control Status Word
         JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),  // Setup transfer address
         JTAG_ARM_WRITEAP, 1,ADDR16(AHB_AP_DRW),                   // Do transfer & get final CSW
         JTAG_END,
         DATA32(outData),                                          // Data to send
      };
      // Write data to memory via DRW + get status
      rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 4, inBuffer, debugJTAG);
      if (rc == BDM_RC_OK) {
         rc = checkSticky(inBuffer);
      }
      if (rc != BDM_RC_OK) {
         print("   writeMemoryHalfword() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
         return rc;
      }
   }
   return BDM_RC_OK;
}

//! Write words to target memory
//!
//! @note Assumes aligned address
//! @note Breaks transfers on 2**10 boundary as TAR may not increment across this boundary
//!
//! @param address - 32-bit address
//! @param data    - ptr to buffer containing words
//!
//! @return error code
//!
static USBDM_ErrorCode writeMemoryWord(uint32_t address, unsigned numWords, const uint32_t *data) {
   USBDM_ErrorCode rc;
   print("     writeMemoryWord(a=0x%08X-0x%08X,nW=0x%X,nB=0x%X)\n", address, address+(4*numWords)-1, numWords, 4*numWords);

   const uint32_t cswValue     = debugInformation.memAPControlStatusDefault|AHB_AP_CSW_SIZE_WORD|AHB_AP_CSW_INC_SINGLE;
   bool initialTransfer   = true;
   bool writeAddressToTAR = true;
   uint8_t   outBuffer[armInfo.maxMemoryWriteSize];

   assert(armInfo.maxMemoryWriteSize>40);

   while (numWords > 0) {
      const int tSize = 5;
      bool crossedBoundary = false;
      uint8_t *outDataPtr = outBuffer;
      uint8_t transferSize = armInfo.maxMemoryWriteSize;
      if (initialTransfer) {
//         print("     writeMemoryWord() - CSW <= 0x%08X)\n", cswValue);
         // Initial transfer includes CSW register setup
         const uint8_t initSequence[] = {
            JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_CSW), DATA32(cswValue),
         };
         memcpy(outDataPtr, initSequence, sizeof(initSequence));
         outDataPtr   += sizeof(initSequence);
         transferSize -= sizeof(initSequence);
      }
      if (writeAddressToTAR) {
//         print("     writeMemoryWord() - TAR <= 0x%08X)\n", address);
         // Transfer includes TAR register setup
         const uint8_t tarSequence[] = {
            JTAG_ARM_WRITEAP_I, ADDR16(AHB_AP_TAR), DATA32(address),
         };
         memcpy(outDataPtr, tarSequence, sizeof(tarSequence));
         outDataPtr   += sizeof(tarSequence);
         transferSize -= sizeof(tarSequence);
      }
      transferSize -= tSize;
      if (transferSize>4*numWords)
         transferSize = 4*numWords;
      // Check if range crosses page boundary
      uint32_t bytesRemainingInPage = ((address+ARM_PAGE_SIZE)&~(ARM_PAGE_SIZE-1)) - address;
      if (transferSize>=bytesRemainingInPage) {
         crossedBoundary = true;
         transferSize = bytesRemainingInPage;
//         print("     writeMemoryWord(a=0x%08X-0x%08X) - limiting to page boundary\n",
//               address, address+transferSize-1);
      }
      transferSize /= 4;
      const uint8_t transferSequence[] = {
         JTAG_ARM_WRITEAP, transferSize, ADDR16(AHB_AP_DRW),
         JTAG_END,
      };
      assert(sizeof(transferSequence)==tSize);
      memcpy(outDataPtr, transferSequence, sizeof(transferSequence));
      outDataPtr += sizeof(transferSequence);
      numWords   -= transferSize;
      // Copy data to buffer
      int index=0;
      for (index=transferSize; index-->0;) {
//         print("     writeMemoryWord(a=0x%08X,n=0x%X) => *data=0x%08X\n", address, index, *data);
         // Assemble as BE words
         *outDataPtr++ = *data>>24;
         *outDataPtr++ = *data>>16;
         *outDataPtr++ = *data>>8;
         *outDataPtr++ = *data;
         data++;
      }
//      print("     writeMemoryWord() -  block(a=0x%08X-0x%08X)\n", address, address+(4*transferSize)-1);
      // Write data to memory & obtain status
      uint8_t inBuffer[4];
      rc = executeJTAGSequence(outDataPtr-outBuffer, outBuffer, 4, inBuffer, debugJTAG);
      if (rc == BDM_RC_OK) {
         rc = checkSticky(inBuffer);
      }
      if (rc != BDM_RC_OK) {
         print("     writeMemoryWord(), block(a=0x%08X-0x%08X), Failed, rc = %s\n",
               address, address+(4*transferSize)-1, USBDM_GetErrorString(rc));
         print("     writeMemoryWord(A=%s)\n", ARM_GetMemoryName(address));
         return rc;
      }
      writeAddressToTAR = crossedBoundary;
      initialTransfer = false;
      address += 4*transferSize;
   }
   return BDM_RC_OK;
}

//! Write to target memory
//!
//! @param address   - Address to access
//! @param numBytes  - Number of bytes to transfer
//! @param data      - Ptr to data buffer
//!
//! @return error code
//!
//! @note The transfer will be decomposed to a minimal set of byte/halfword/word accesses
//!
static USBDM_ErrorCode writeMemory(uint32_t address, unsigned numBytes, const uint8_t *data) {
   USBDM_ErrorCode rc;

//   print("     writeMemory(a=0x%08X)\n", address);

   // Do any odd byte at start
   if (address&0x01) {
//      print("     writeMemory() - Doing odd prefix byte\n");
      rc = writeMemoryByte(address, *data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += 1;
      address  += 1;
      numBytes -= 1;
   }
   // Do any odd halfword at start
   if ((address&0x02) && (numBytes >= 2)) {
//      print("     writeMemory() - Doing odd prefix halfword\n");
      rc = writeMemoryHalfword(address, (*data)+((*(data+1))<<8));
      if (rc != BDM_RC_OK)
         return rc;
      data     += 2;
      address  += 2;
      numBytes -= 2;
   }
   // Do middle bytes as words
   if (numBytes >= 4) {
      rc = writeMemoryWord(address, numBytes/4, (uint32_t*)data);
      if (rc != BDM_RC_OK)
         return rc;
      data     += numBytes&~0x3;
      address  += numBytes&~0x3;
      numBytes -= numBytes&~0x3;
   }
   // Do odd word at end
   if (numBytes>=2) {
//      print("     writeMemory() - Doing odd suffix halfword\n");
      rc = writeMemoryHalfword(address, (*data)+((*(data+1))<<8));
      if (rc != BDM_RC_OK)
         return rc;
      data     += 2;
      address  += 2;
      numBytes -= 2;
   }
   // Do odd byte at end
   if (numBytes>=1) {
//      print("     writeMemory() - Doing odd suffix byte\n");
      rc = writeMemoryByte(address, *data);
      if (rc != BDM_RC_OK)
         return rc;
   }
   return BDM_RC_OK;
}
#if 0
static USBDM_ErrorCode initViaMDM_AP(void) {
   const uint32_t controlRequestValue =
//         MDM_AP_Control_Core_Hold_Reset|
         MDM_AP_Control_System_Reset_Request|
         MDM_AP_Control_Core_Hold_Reset|
         MDM_AP_Control_Debug_Request;
   const uint32_t controlReleaseValue =
         MDM_AP_Control_Core_Hold_Reset|
         MDM_AP_Control_Debug_Request;
   uint32_t inData;

   print("   initViaMDM_AP()\n");
//   debugJTAG = true;
   writeAP(MDM_AP_Control, controlRequestValue);
   milliSleep(100);
   readAP(MDM_AP_Status, &inData);
   print("   initViaMDM_AP() MDM-SP-Status=0x%08X\n", inData);
   if ((inData&MDM_AP_System_Reset) != 0)
      print("initViaMDM_AP() - reset not confirmed\n");
   writeAP(MDM_AP_Control, controlReleaseValue);
   milliSleep(100);
   readAP(MDM_AP_Status, &inData);
   print("   initViaMDM_AP() MDM-SP-Status=0x%08X\n", inData);
   if ((inData&MDM_AP_System_Reset) == 0)
      print("   initViaMDM_AP() - reset release not confirmed\n");
   if ((inData&MDM_AP_System_Security) != 0)
      print("   initViaMDM_AP() - device is secured\n");
   debugJTAG = false;

//   if (debugInformation.subType==TARGET_KINETIS) {
//      // Release Freescale MDM-AP-Control
//      writeAP(MDM_AP_Control, 0);
//   }

   return BDM_RC_OK;
}

//! Read information about the Freescale specific MDM-AP
//!
static USBDM_ErrorCode readMDM_AP(void) {
//   print("MDM_AP_Status=0x%08X, ADDR16(MDM_AP_Status)=0x%04X\n",
//         MDM_AP_Status, ADDR16(MDM_AP_Status));

   const uint8_t jtagSequence[] = {
         JTAG_ARM_READAP, 1, ADDR16(MDM_AP_Status),    // Do transfer & get final CSW
         JTAG_ARM_READAP, 1, ADDR16(MDM_AP_Control),   // Do transfer & get final CSW
         JTAG_ARM_READAP, 1,ADDR16(MDM_AP_Id),        // Do transfer & get final CSW
      JTAG_END,
   };
   uint8_t inBuffer[100];
   USBDM_ErrorCode rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 24, inBuffer, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   readMDM_AP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   rc = checkSticky(inBuffer+4);
   if (rc != BDM_RC_OK) {
      print("   readMDM_AP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   rc = checkSticky(inBuffer+12);
   if (rc != BDM_RC_OK) {
      print("   readMDM_AP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   rc = checkSticky(inBuffer+20);
   if (rc != BDM_RC_OK) {
      print("   readMDM_AP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   uint32_t status  = (inBuffer[0]<<24) +(inBuffer[1]<<16) +(inBuffer[2]<<8) +(inBuffer[3]);
   uint32_t control = (inBuffer[8]<<24) +(inBuffer[9]<<16) +(inBuffer[10]<<8)+(inBuffer[11]);
   uint32_t id      = (inBuffer[16]<<24)+(inBuffer[17]<<16)+(inBuffer[18]<<8)+(inBuffer[19]);
   print("   readMDM_AP() status=0x%08X, control=0x%08X, id=0x%08X\n", status, control, id);
   if (id != 0x001C0000)
      print("   readMDM_AP() expected id=0x001C0000, actual id=0x%08X\n", id);
   return BDM_RC_OK;
}
#endif

//! Read Information that describes the debug interface
//!
static USBDM_ErrorCode readDebugInformation(void) {
   USBDM_ErrorCode rc;
   uint32_t dataIn;

   print("   readDebugInformation()\n");

   const uint8_t jtagSequence[] = {
         // Each of these returns 8 bytes = data-32 + DP-Control/Status
         JTAG_ARM_READAP, 1,ADDR16(MDM_AP_Id),
         JTAG_ARM_READAP, 1,ADDR16(AHB_AP_Id),
         JTAG_ARM_READAP, 1,ADDR16(AHB_AP_CSW),
         JTAG_ARM_READAP, 1,ADDR16(AHB_AP_CFG),
         JTAG_ARM_READAP, 1,ADDR16(AHB_AP_Base),
      JTAG_END,
   };
   uint8_t inBuffer[5*8];
   rc = executeJTAGSequence(sizeof(jtagSequence), jtagSequence, 5*8, inBuffer, debugJTAG);
   if (rc != BDM_RC_OK) {
      print("   readMDM_AP() - Failed, reason = %s\n", USBDM_GetErrorString(rc));
      return rc;
   }
   for (unsigned index=4; index<sizeof(inBuffer); index+=8) {
      rc = checkSticky(inBuffer+index);
      if (rc != BDM_RC_OK) {
         print("   readMDM_AP() - Failed, indx=%d, reason = %s\n", index, USBDM_GetErrorString(rc));
         return rc;
      }
   }
   // Check ID value from MDM-AP is Freescale
   dataIn = getData32Be(inBuffer);
   debugInformation.subType = (dataIn == 0x001C0000)?TARGET_KINETIS:TARGET_STM32F10x;
   print("   readDebugInformation(): MDM_AP_Id => 0x%08X, %s-AP\n", dataIn,
         (debugInformation.subType==TARGET_KINETIS)?"Freescale":"Other");
//   if (debugInformation.subType==TARGET_KINETIS) {
//      readMDM_AP();
//      initViaMDM_AP();
//   }

   // Check ID value from AHB-AP identifies an ARM AMBA-AHB-AP
   dataIn = getData32Be(inBuffer+8);
   print("   readDebugInformation(): AHB_AP_Id => 0x%08X, OK\n", dataIn);
   if ((dataIn&0x0FFF000F)!= 0x04770001)
      return BDM_RC_ARM_ACCESS_ERROR;

   // Save the default AHB_AP_CSW
   dataIn = getData32Be(inBuffer+16);
   dataIn &= ~(AHB_AP_CSW_INC_MASK|AHB_AP_CSW_SIZE_MASK);
   debugInformation.memAPControlStatusDefault = dataIn;
   print("   readDebugInformation(): AHB_AP_CSW default => 0x%08X\n", dataIn);

   // Save the AHB_AP_CFG register
   dataIn = getData32Be(inBuffer+24);
   debugInformation.memAPConfig = dataIn;
   debugInformation.bigEndian = (dataIn&AHB_AP_CFG_BIGENDIAN)!=0;
   print("   readDebugInformation(): AHB_AP_CFG => 0x%08X, %s\n",
         dataIn, (dataIn&1)?"BigEndian":"LittleEndian");

   // Get Debug base address
   dataIn = getData32Be(inBuffer+32);
   print("   readDebugInformation(): AHB_AP_Base => 0x%08X\n", dataIn);
   debugInformation.debugBaseaddr = dataIn & 0xFFFFF000;

//   uint32_t buffer[8];
//   uint32_t id;
//   // Read ID registers
//   rc = readMemoryWord(debugInformation.debugBaseaddr+0xFF0, 4, buffer);
//   if (rc != BDM_RC_OK)
//      return rc;
//   id  = (buffer[0x0]&0xFF);
//   id += (buffer[0x1]&0xFF)<<8;
//   id += (buffer[0x2]&0xFF)<<16;
//   id += (buffer[0x3]&0xFF)<<24;
//
//   print("   readDebugInformation(): ID => 0x%08X\n", id);
//   if ((id & 0xFFFF0FFF) != 0xB105000D) {
//      print("   readDebugInformation(): ID invalid\n");
//   }
//   debugInformation.componentClass = (id>>12)&0xF;
//   print("   readDebugInformation(): component class => 0x%X\n", debugInformation.componentClass);

//   // Read Peripheral ID0 register
//   rc = readMemoryWord(debugInformation.debugBaseaddr+0xFD0, 1, buffer);
//   if (rc != BDM_RC_OK)
//      return rc;
//   id  = (buffer[0x0]>>4)&0xFF;
//   debugInformation.size4Kb = 1<<id;
//   print("   readDebugInformation(): 4Kb size => %d\n", debugInformation.size4Kb);

   return BDM_RC_OK;
}

bool ARM_InitialiseDone = false;

//! Initialise ARM interface
//!
USBDM_ErrorCode ARM_Initialise() {
   USBDM_ErrorCode rc = BDM_RC_OK;
   uint32_t dataIn;

   print("   ARM_Initialise()\n");

   rc = GetBdmInfo();
   if (rc != BDM_RC_OK) {
      return rc;
   }
   bdmOptions.size = sizeof(bdmOptions);
   bdmOptions.targetType = T_ARM_JTAG;
   rc = USBDM_GetExtendedOptions(&bdmOptions);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   USBDM_SetSpeed(500 /* kHz */);

   debugJTAG = false;

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

   // Read IDCODE by JTAG_RESET
   uint32_t hardwareIdcode;
   rc = readIDCODE(&hardwareIdcode, 0, JTAG_ARM_IDCODE_LENGTH, true);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   print("   ARM_Initialise() CHIP IDCODE by JTAG_RESET = 0x%08X\n", hardwareIdcode);
   if ((hardwareIdcode == 0xFFFFFFFF)||(hardwareIdcode == 0x00000000)) {
      print("   ARM_Initialise() CHIP IDCODE Invalid - no device?\n");
      return BDM_RC_NO_CONNECTION;
   }
   if (hardwareIdcode == ARM_Cortex_M3_IDCODE) {
      print("   ARM_Initialise() Detected ARM-Cortex3, Assuming STM chip - setting TDR=1/TIR=5\n");
      // Assume single STM32F100 for the moment
      const uint8_t sequence[] = {
         JTAG_SET_PADDING,  // #4x16-bits - sets HDR HIR TDR TIR
         JTAG16(0), // HDR=0, No devices downstream
         JTAG16(0), // HIR=0
         JTAG16(1), // TDR=1, One device upstream in bypass
         JTAG16(5), // TIR=5  with IR length = 5-bits
         JTAG_END,
      };
      rc = executeJTAGSequence(sizeof(sequence), sequence, 0, NULL, true);
      if (rc != BDM_RC_OK) {
         return rc;
      }
   }
   // Try reading ARM_IDCODE using IDCODE command to check chain setup
   uint32_t idcode;
   rc = readIDCODE(&idcode, JTAG_ARM_IDCODE_COMMAND, JTAG_ARM_IDCODE_LENGTH, false);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   print("   ARM_Initialise() ARM IDCODE  = 0x%08X\n", idcode);
   if (hardwareIdcode != idcode) {
      print("   ARM_Initialise() - IDCODEs do not agree\n");
   }
   if (idcode == ARM_Cortex_M3_IDCODE) {
      print("   ARM Core = ARM_Cortex_M3\n");
   }
   else if (idcode == ARM_Cortex_M4_IDCODE) {
      print("   ARM Core = ARM_Cortex_M4\n");
      // Read device IDCODE (Freescale only)
      rc = readIDCODE(&idcode, JTAG_IDCODE_COMMAND, JTAG_IDCODE_LENGTH, false);
      if (rc != BDM_RC_OK)
         return rc;
      print("   ARM_Initialise() JTAG IDCODE  = 0x%08X\n", idcode);
   }
   else {
      print("   ARM_Initialise() - Unrecognised device\n");
      return BDM_RC_UNKNOWN_DEVICE;
   }
   // Turn on power & wait for ACKs
   // Power up system & debug interface
   rc = writeDP_ControlStatus(CSYSPWRUPREQ|CDBGPWRUPREQ);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   int retry = 20;
   do {
      rc = readDP_ControlStatus(&dataIn);
      print("   ARM_Initialise() DP_ControlStatus= 0x%08X\n", dataIn);
      milliSleep(100);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      if ((dataIn & CSYSPWRUPACK) == 0) {
         continue;
      }
      if ((dataIn & CDBGPWRUPACK) != 0) {
         break;
      }
   } while(retry-- > 0);
   if ((dataIn & CSYSPWRUPACK) == 0) {
      return BDM_RC_ARM_PWR_UP_FAIL;
   }
   if ((dataIn & CDBGPWRUPACK) == 0) {
      return BDM_RC_ARM_PWR_UP_FAIL;
   }
   print("   ARM_Initialise() System & Debug PWR-UP OK\n");

#if 0
   // Reset Debug Interface
   rc = writeDP_ControlStatus(CSYSPWRUPREQ|CDBGPWRUPREQ|CDBGSTREQ);
   if (rc != BDM_RC_OK)
      return rc;
   retry = 20;
   // Check ACKs
   do {
      rc = readDP_ControlStatus(&dataIn);
      print("   ARM_Initialise() DP_ControlStatus= 0x%08X\n", dataIn);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      if ((dataIn & CDBGSTACK) != 0) {
         break;
      }
   } while(retry-- > 0);
   if ((dataIn & CDBGSTACK) == 0) {
      return BDM_RC_ARM_PWR_UP_FAIL;
   }
   print("   ARM_Initialise() DEBUG-RESET assert OK\n");

   // Release Reset Debug Interface
   rc = writeDP_ControlStatus(CSYSPWRUPREQ|CDBGPWRUPREQ);
   if (rc != BDM_RC_OK) {
      return rc;
   }
   retry = 20;
   // Check ACKs
   do {
      rc = readDP_ControlStatus(&dataIn);
      print("   ARM_Initialise() DP_ControlStatus= 0x%08X\n", dataIn);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      if ((dataIn & CDBGSTACK) == 0)
         break;
   } while(retry-- > 0);
   if ((dataIn & CDBGSTACK) != 0)
      return BDM_RC_ARM_PWR_UP_FAIL;
   print("   ARM_Initialise() DEBUG-RESET release OK\n");
#endif

//   USBDM_ControlPins(PIN_RELEASE);

   readDebugInformation();

//   ARM_Connect();
//
//   uint32_t status;
//   ARM_GetStatus(&status);
//   print("   ARM_Initialise() - complete\n");
   return rc;
}

//! Reset Target
//!
//! @param targetMode - reset mode
//!
USBDM_ErrorCode ARM_TargetReset(TargetMode_t targetMode) {
   USBDM_ErrorCode rc = BDM_RC_FAIL;

   print("   ARM_TargetReset(%s)\n", getTargetModeName(targetMode));

   TargetMode_t resetMethod = (TargetMode_t)(targetMode&RESET_METHOD_MASK);
   TargetMode_t resetMode   = (TargetMode_t)(targetMode&RESET_MODE_MASK);
   if (resetMethod == RESET_DEFAULT) {
      resetMethod = RESET_HARDWARE;
   }
   print("   ARM_TargetReset() - modified=(%s)\n", getTargetModeName((TargetMode_t)(resetMethod|resetMode)));

#ifdef LOG
   ArmStatus status;
   ARM_GetStatus(&status);
   print("   ARM_TargetReset()- Target is %s before reset\n", (status.dhcsr&(DHCSR_S_HALT|DHCSR_S_LOCKUP))?"Halted":"Running");
#endif

   uint32_t dhcsrValue = DHCSR_DBGKEY|DHCSR_C_DEBUGEN; //DHCSR_C_HALT
   uint32_t dbgValue   = (DBGMCU_IWDG_STOP|DBGMCU_WWDG_STOP);

   uint32_t demcrValue;
   if (resetMode==RESET_SPECIAL) {
      print("   ARM_TargetReset()- Doing +Special reset\n");
      // Set catch on reset vector fetch
      demcrValue = DEMCR_TRCENA|DEMCR_VC_HARDERR|DEMCR_VC_INTERR|DEMCR_VC_BUSERR|DEMCR_VC_STATERR|
                   DEMCR_VC_CHKERR|DEMCR_VC_NOCPERR|DEMCR_VC_MMERR|DEMCR_VC_CORERESET;
   }
   else {
      // Disable catch on reset vector fetch
      demcrValue = DEMCR_TRCENA|DEMCR_VC_HARDERR|DEMCR_VC_INTERR|DEMCR_VC_BUSERR|DEMCR_VC_STATERR|
                   DEMCR_VC_CHKERR|DEMCR_VC_NOCPERR|DEMCR_VC_MMERR;
   }
   switch (resetMethod) {
//   case RESET_POWER:
//      // Todo This doesn't make sense!
//      print("   ARM_TargetReset()- Doing Power reset\n");
//      // DEMCR
//      rc = writeMemoryWord(DEMCR, 1, &demcrValue);
//      if (rc != BDM_RC_OK) {
//         rc = writeMemoryWord(DEMCR, 1, &demcrValue);
//      }
//      if (rc != BDM_RC_OK) {
//         return rc;
//      }
//      // DHCSR
//      rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
//      if (rc != BDM_RC_OK) {
//         rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
//      }
//      if (rc != BDM_RC_OK) {
//         return rc;
//      }
//      // Pass to BDM to cycle power
//      rc = USBDM_TargetReset((TargetMode_t)(resetMethod|resetMode));
//      if (rc != BDM_RC_OK) {
//         return rc;
//      }
//      if (debugInformation.subType==TARGET_STM32F10x) {
//         writeMemoryWord(DBGMCU_CR, 1, &dbgValue);
//      }
//      break;
//
   case RESET_ALL:
   case RESET_HARDWARE :
      // Force hardware reset
      print("   ARM_TargetReset()- Doing Hardware reset\n");
      USBDM_ControlPins(PIN_RESET_LOW);
      if (debugInformation.subType==TARGET_STM32F10x) {
         // Disable watchdog while Reset is asserted (as per STM recommendation)
         print("   ARM_TargetReset()- Attempting to disable Watchdog\n");
         writeMemoryWord(DBGMCU_CR, 1, &dbgValue);
      }
      // DEMCR
      rc = writeMemoryWord(DEMCR, 1, &demcrValue);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      // DHCSR
      rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
      if (rc != BDM_RC_OK) {
         return rc;
      }
      milliSleep(bdmOptions.resetDuration);
      writeMemoryWord(DBGMCU_CR, 1, &dbgValue);
      // Release hardware reset
      USBDM_ControlPins(PIN_RELEASE);
      milliSleep(bdmOptions.resetRecoveryInterval);
      break;

      // Disable watchdog timers etc in debug mode
   case RESET_SOFTWARE:
      // Do software (local) reset via ARM debug function
      print("   ARM_TargetReset()- Doing Software reset\n");
      if (debugInformation.subType==TARGET_KINETIS) {
#if 1
         uint32_t dhcsrValue = DHCSR_DBGKEY|DHCSR_C_DEBUGEN;

         // Use special Freescale MDM-AP functions
         print("   ARM_TargetReset()- Using Freescale MDM-AP\n");
         // Hold device in reset
         rc = writeAP(MDM_AP_Control, MDM_AP_Control_Debug_Request|MDM_AP_Control_System_Reset_Request);
         if (rc != BDM_RC_OK) {
            return rc;
         }
         // Release any hardware reset
         USBDM_ControlPins(PIN_RELEASE);
         milliSleep(50);
         // Set up usual Cortex debug registers
         rc = writeMemoryWord(DEMCR, 1, &demcrValue);
         if (rc == BDM_RC_OK) {
            // DHCSR
            rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
         }
         // Release reset
         USBDM_ErrorCode rc2 = writeAP(MDM_AP_Control, 0);
         milliSleep(100);
         if (rc2 != BDM_RC_OK) {
            return rc2;
         }
         if (rc == BDM_RC_ARM_ACCESS_ERROR) {
            // May indicate the device is secured
            uint32_t mdmApStatus;
            rc2 = readAP(MDM_AP_Status, &mdmApStatus);
            if ((rc2 == BDM_RC_OK) && ((mdmApStatus & MDM_AP_System_Security) != 0)) {
               print("   ARM_TargetReset()- Checking Freescale MDM-AP - device is secured\n");
               return BDM_RC_SECURED;
            }
            return BDM_RC_ARM_ACCESS_ERROR;
         }
         return rc;
#else
         // Use special Freescale MDM-AP functions
         print("   ARM_TargetReset()- Using Freescale MDM-AP\n");
         rc = writeAP(MDM_AP_Control, MDM_AP_Control_Core_Hold_Reset|MDM_AP_Control_System_Reset_Request|
                                      MDM_AP_Control_Debug_Request);
         if (rc != BDM_RC_OK) {
            return rc;
         }
         rc = writeAP(MDM_AP_Control, MDM_AP_Control_Core_Hold_Reset);
         if (rc != BDM_RC_OK) {
            return rc;
         }
         milliSleep(50);
         // DEMCR
         rc = writeMemoryWord(DEMCR, 1, &demcrValue);
         if (rc == BDM_RC_OK) {
            // DHCSR
            rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
         }
         // Release reset before checking error status
         USBDM_ErrorCode rc2 = writeAP(MDM_AP_Control, 0);
         milliSleep(100);
         if (rc2 != BDM_RC_OK) {
            return rc2;
         }
         if (rc == BDM_RC_ARM_ACCESS_ERROR) {
            // May indicate the device is secured
            uint32_t mdmApStatus;
            rc2 = readAP(MDM_AP_Status, &mdmApStatus);
            if ((rc2 == BDM_RC_OK) && ((mdmApStatus & MDM_AP_System_Security) != 0)) {
               print("   ARM_TargetReset()- Checking Freescale MDM-AP - device is secured\n");
               return BDM_RC_SECURED;
            }
            return BDM_RC_ARM_ACCESS_ERROR;
         }
         return rc;
#endif
      }
      else {
         // This may be unreliable if the target is in a reset loop
         // Try to disable watch-dog
         if (debugInformation.subType==TARGET_STM32F10x) {
            print("   ARM_TargetReset()- Attempting to disable Watchdog\n");
            writeMemoryWord(DBGMCU_CR, 1, &dbgValue);
         }
         // DEMCR
         rc = writeMemoryWord(DEMCR, 1, &demcrValue);
         if (rc != BDM_RC_OK) {
            rc = writeMemoryWord(DEMCR, 1, &demcrValue);
         }
         if (rc != BDM_RC_OK) {
            return rc;
         }
         // DHCSR
         rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
         if (rc != BDM_RC_OK) {
            rc = writeMemoryWord(DHCSR, 1, &dhcsrValue);
         }
         if (rc != BDM_RC_OK) {
            return rc;
         }
         // Request system reset via AIRCR
         uint32_t aiscrValue = AIRCR_VECTKEY|AIRCR_SYSRESETREQ;
         rc = writeMemoryWord(AIRCR, 1, &aiscrValue);
         if (rc != BDM_RC_OK) {
            return rc;
         }
      }
      break;
   default:
      print("   ARM_TargetReset()- Illegal options\n");
      return BDM_RC_ILLEGAL_PARAMS;
   }
#ifdef LOG
   ARM_GetStatus(&status);
   print("   ARM_TargetReset()- Target is %s after reset\n", (status.dhcsr&(DHCSR_S_HALT|DHCSR_S_LOCKUP))?"Halted":"Running");
#endif
   return BDM_RC_OK;
}

//! Connect to ARM Target
//!
USBDM_ErrorCode ARM_Connect() {
   print("   ARM_Connect()\n");

   USBDM_ErrorCode rc;
   int retry = 10;
   do {
      uint32_t debugOnValue = DHCSR_DBGKEY|DHCSR_C_DEBUGEN|DHCSR_C_HALT;
      rc = writeMemoryWord(DHCSR, 1, &debugOnValue);
      if (rc != BDM_RC_OK) {
         print("   ARM_Connect() DHCSR write failed\n");
         continue;
      }
      uint32_t dataIn;
      if (rc == BDM_RC_OK) {
         rc = readMemoryWord(DHCSR, 1, &dataIn);
         if (rc != BDM_RC_OK) {
            print("   ARM_Connect() DHCSR read failed\n");
            continue;
         }
         else {
            print("   ARM_Connect() DHCSR value = %s(0x%08X)\n", getDHCSRName(dataIn), dataIn);
         }
      }
      if ((dataIn&DHCSR_C_DEBUGEN) == 0) {
         print("   ARM_Connect() Debug enable failed\n");
         // May indicate the device is secured
         if (debugInformation.subType==TARGET_KINETIS) {
               uint32_t mdmApStatus;
               rc = readAP(MDM_AP_Status, &mdmApStatus);
               if ((rc == BDM_RC_OK) && ((mdmApStatus & MDM_AP_System_Security) != 0)) {
                  print("   ARM_Connect()- Checking Freescale MDM-AP - device is secured\n");
                  return BDM_RC_SECURED;
               }
         }
         continue;
      }
      else {
         print("   ARM_Connect() Debug Enable complete\n");
         break;
      }
   } while (--retry >0);
   if (retry == 0)
      return BDM_RC_BDM_EN_FAILED;
   return BDM_RC_OK;
}

//! Get ARM target status
//!
//! @param status - true => halted, false => running
//!
USBDM_ErrorCode ARM_GetStatus(ArmStatus *status) {
   USBDM_ErrorCode rc;
   uint32_t dataIn;
   ArmStatus defaultStatus = {0,0xFFFFFFFF,0,0};

   print("   ARM_GetStatus()\n");

   //ToDo - Consider Kinetis specific MDM_AP_Status
   *status = defaultStatus;

   if (debugInformation.subType==TARGET_KINETIS) {
      // Freesale MDM-AP present
      rc = readAP(MDM_AP_Status, &dataIn);
      if (rc != BDM_RC_OK) {
         print("   ARM_GetStatus() Can't read MDM_AP_Status!\n");
         return BDM_RC_BDM_EN_FAILED;
      }
      status->mdmApStatus = dataIn;
   }

   //      *status = (dataIn&MDM_AP_Status_Core_Halted) != 0;
   //      print("   ARM_GetStatus() => MDM_AP_Status=0x%08X (%s)\n",
   //            dataIn,
   //            (*status)?"Halted":"Running");
   //      return BDM_RC_OK;
   //   }
   //   else {
   rc = readMemoryWord(DEMCR, 1, &dataIn);
   if (rc != BDM_RC_OK) {
      print("   ARM_GetStatus() Can't read DEMCR!\n");
   }
   else {
      print("   ARM_GetStatus(): DEMCR status=%s(0x%08X)\n", getDEMCRName(dataIn), dataIn);
   }
   // Generic Debug
   rc = readMemoryWord(DHCSR, 1, &dataIn);
   if (rc != BDM_RC_OK) {
      print("   ARM_GetStatus() Can't read DHCSR!\n");
      return BDM_RC_BDM_EN_FAILED;
   }
   //      print("   ARM_GetStatus() DHCSR value = 0x%08X\n", dataIn);
   //      if ((dataIn&DHCSR_C_DEBUGEN) == 0) {
   //         print("   ARM_Initialise() Debug enable not set!\n");
   //         return BDM_RC_BDM_EN_FAILED;
   //      }
   if ((dataIn&DHCSR_S_LOCKUP) != 0) {
      const uint32_t dataOut = DHCSR_DBGKEY|DHCSR_C_HALT|DHCSR_C_DEBUGEN;
      print("   ARM_GetStatus() Clearing Lockup, DHCSR status=%s(0x%08X)\n", getDHCSRName(dataIn), dataIn);
      writeMemoryWord(DHCSR, 1, &dataOut);
      rc = readMemoryWord(DHCSR, 1, &dataIn);
      if (rc != BDM_RC_OK) {
         print("   ARM_GetStatus() Can't read DHCSR!\n");
         return BDM_RC_BDM_EN_FAILED;
      }
   }
   status->dhcsr = dataIn;

   print("   ARM_GetStatus() => DHCSR status=%s(0x%08X) (%s)\n",
         getDHCSRName(dataIn), dataIn,
         (dataIn&(DHCSR_S_HALT|DHCSR_S_LOCKUP))?"Halted":"Running");
   if (debugInformation.subType==TARGET_KINETIS) {
      print("                   => MDM_AP=%s(0x%08X)\n",
            getMDM_APStatusName(status->mdmApStatus), status->mdmApStatus);
   }
   return BDM_RC_OK;
   //   }
}

//! Write data to target memory
//!
//! @param elementSize = Size of data elements (1/2/4 bytes)
//! @param byteCount   = Number of _bytes_ to transfer
//! @param address     = Memory address
//! @param data        = Ptr to block of data to write
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode  ARM_WriteMemory( unsigned int  elementSize,
                                  unsigned int  byteCount,
                                  unsigned int  address,
                                  unsigned const char *data) {
   USBDM_ErrorCode rc;
   print("   ARM_WriteMemory(A=%s, Sz=%d, #=0x%X)\n", ARM_GetMemoryName(address), elementSize, byteCount);
   printDump(data, byteCount, address, BYTE_DISPLAY|BYTE_ADDRESS);
   rc = writeMemory(address, byteCount, data);
   if (address == DHCSR) {
      print("   ARM_WriteMemory(DHCSR) <= %s(0x%08X)\n", getDHCSRName(0xFFFF&*(uint32_t*)data), *(uint32_t*)data);
   }
   return rc;
}

//! Read data from target memory
//!
//! @param elementSize = Size of data (1/2/4 bytes)
//! @param byteCount   = Number of bytes to transfer
//! @param address     = Memory address
//! @param data        = Where to place data
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ARM_API
USBDM_ErrorCode  ARM_ReadMemory( unsigned int  elementSize,
                                 unsigned int  byteCount,
                                 unsigned int  address,
                                 unsigned char *data) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   print("   ARM_ReadMemory(A=%s, eSz=%d, #=%d)\n", ARM_GetMemoryName(address), elementSize, byteCount);
   rc = readMemory(address, byteCount, data);
   if (rc == BDM_RC_OK) {
      printDump(data, byteCount, address, BYTE_DISPLAY|BYTE_ADDRESS);
   }
   if (address == DHCSR) {
      print("   ARM_ReadMemory(DHCSR) => %s(0x%08X)\n", getDHCSRName(*(uint32_t*)data), *(uint32_t*)data);
   }
   return rc;
}

//! Read Register
//!
//! @param regNo     - Register number
//! @param regValue  - Value from register
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ErrorCode ARM_ReadRegister(ARM_Registers_t regNo, unsigned long *regValue) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   uint32_t regWriteValue[] = {DCRSR_READ|(regNo&DCRSR_REGMASK)};
   uint32_t dataIn;
   uint32_t value;
   unsigned retryCount = 40;
   rc = writeMemoryWord(DCRSR,1,regWriteValue);
   if (rc != BDM_RC_OK)
      return rc;
   do {
      rc = readMemoryWord(DHCSR,1,&dataIn);
      if (rc != BDM_RC_OK)
         return rc;
   } while (((dataIn & DHCSR_S_REGRDY) == 0) && (retryCount-- > 0));
   if (retryCount == 0)
      return BDM_RC_ARM_ACCESS_ERROR;
   rc = readMemoryWord(DCRDR,1,&value);
   *regValue = value;
   if (rc != BDM_RC_OK)
      return rc;
   print("   ARM_ReadRegister(%s) => 0x%X\n", getARMRegName(regNo), *regValue);
   return rc;
}

//! Write Register
//!
//! @param regNo     - Register number
//! @param regValue  - Value for register
//!
USBDM_ErrorCode ARM_WriteRegister(ARM_Registers_t regNo, unsigned long regValue) {
   USBDM_ErrorCode rc = BDM_RC_OK;
   uint32_t regWriteValue[] = {DCRSR_WRITE|(regNo&DCRSR_REGMASK)};
   uint32_t value = regValue;
   print("   ARM_WriteRegister(%s) <= 0x%X\n", getARMRegName(regNo), regValue);
   rc = writeMemoryWord(DCRDR,1,&value);
   if (rc != BDM_RC_OK)
      return rc;
   return writeMemoryWord(DCRSR,1,regWriteValue);
}

//! Read Control Register (
//!
//! @param regNo     - Register number in AP address space
//!                     e.g. MDM-AP.Status  = 0x0100000
//!                          MDM-AP.Control = 0x0100004
//!                          MDM-AP.Ident   = 0x010003F
//! @param regValue  - Value from register
//!
USBDM_ErrorCode ARM_ReadCReg(unsigned int regNo, unsigned long *regValue) {
   uint32_t value = 0;
   USBDM_ErrorCode rc = readAP(regNo, &value);
   *regValue = value;
   return rc;
}

//! Write Control Register (
//!
//! @param regNo     - Register number in AP address space
//!                     e.g. MDM-AP.Status  = 0x0100000
//!                          MDM-AP.Control = 0x0100004
//!                          MDM-AP.Ident   = 0x010003F
//! @param regValue  - Value from register
//!
USBDM_ErrorCode ARM_WriteCReg(unsigned int regNo, unsigned long regValue) {
   return writeAP(regNo, regValue);
}

//! Start Target execution at current PC
//!
//!
USBDM_ErrorCode ARM_TargetGo() {
   print("   ARM_TargetGo\n");
   uint32_t debugGoValue[] = {DHCSR_DBGKEY|DHCSR_C_DEBUGEN};
   return writeMemoryWord(DHCSR, 1, debugGoValue);
}

//! Execute N instructions from current PC
//!
USBDM_ErrorCode ARM_TargetStepN(unsigned numSteps) {
   print("   ARM_TargetStep(%d)\n", numSteps);
   uint32_t debugStepValue;

   ARM_ReadMemory(4,4,DHCSR,(uint8_t*)&debugStepValue);
//   readMemoryWord(DHCSR, 1, &debugStepValue);
   debugStepValue &= DHCSR_C_MASKINTS; // Preserve DHCSR_C_MASKINTS value
   debugStepValue |= DHCSR_DBGKEY|DHCSR_C_STEP|DHCSR_C_DEBUGEN;
   USBDM_ErrorCode rc = ARM_WriteMemory(4,4,DHCSR,(uint8_t*)&debugStepValue);
//   USBDM_ErrorCode rc = writeMemoryWord(DHCSR, 1, &debugStepValue);
   if (rc != BDM_RC_OK)
      print("   ARM_TargetStep() - step failed, reason = %s\n", USBDM_GetErrorString(rc));
   return BDM_RC_OK;
}

//! Execute 1 instruction from current PC
//!
USBDM_ErrorCode ARM_TargetStep(void) {
   return ARM_TargetStepN(1);
}

//! Halts target execution
//!
USBDM_ErrorCode ARM_TargetHalt(void) {
   print("   ARM_TargetHalt()\n");

   uint32_t debugOnValue[] = {DHCSR_DBGKEY|DHCSR_C_HALT|DHCSR_C_DEBUGEN};
   USBDM_ErrorCode rc = writeMemoryWord(DHCSR, 1, debugOnValue);
   if (rc != BDM_RC_OK)
      print("ARM_TargetHalt() - failed write\n");
   ArmStatus status;
   ARM_GetStatus(&status);
   return BDM_RC_OK;
//   return writeAP(MDM_AP_Control, (1<<2));
}
