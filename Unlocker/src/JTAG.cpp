/*! \file
    \brief JTAG Routines

    \verbatim
    CF_Unlocker
    Copyright (C) 2009  Peter O'Donoghue

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
   +====================================================================
   |   1 Jun 2009 | Created
   +====================================================================
    \endverbatim
*/
#include "stddef.h"
#include "Log.h"
#include "Debug.h"
#include "Common.h"
#include "USBDM_API.h"
#include "BitVector.h"
#include "KnownDevices.h"
#include "JTAG.h"

// Useful vectors for use with JTAG interface
bitVector      JTAG_Chain::OnesVector(MAX_JTAG_IR_CHAIN_LENGTH,1);    //!< All 1's vector
bitVector      JTAG_Chain::ZeroesVector(MAX_JTAG_IR_CHAIN_LENGTH,0);  //!< All 0's vector

unsigned int   JTAG_Chain::deviceCount;               //!< Number of JTAG devices found
unsigned int   JTAG_Chain::currentDeviceNum;          //!< The currently selected device
JTAG_Device    JTAG_Chain::devices[MAX_JTAG_DEVICES]; //!< Information about each device
unsigned int   JTAG_Chain::irLength;                  //!< Length of IR chain
bitVector     *JTAG_Chain::irReg = NULL;              //!< Copy read from IR
unsigned int   JTAG_Chain::irPreambleLength;          //!< # of bits before IR register
unsigned int   JTAG_Chain::irPostambleLength;         //!< # of bits after IR register
unsigned int   JTAG_Chain::drPreambleLength;          //!< # of bits before DR register
unsigned int   JTAG_Chain::drPostambleLength;         //!< # of bits after DR register
const uint8_t       JTAG_Chain::one[]  = {0xFF};           //!< useful constant
const uint8_t       JTAG_Chain::zero[] = {0};              //!< useful constant


//! \brief Swaps the Endianess of a 32-bit number
//!
void swapEndian32(uint32_t *value) {
U32u temp1, temp2;

   temp1.longword = *value;
   temp2.bytes[0] = temp1.bytes[3];
   temp2.bytes[1] = temp1.bytes[2];
   temp2.bytes[2] = temp1.bytes[1];
   temp2.bytes[3] = temp1.bytes[0];
   *value         = temp2.longword;
}

//! \brief Initialises the JTAG chain
//! \ref JTAG_Chain::devices is populated with the devices found
//!
USBDM_ErrorCode JTAG_Chain::initialiseJTAGChain(void) {
uint32_t buff;
unsigned int deviceNum;
USBDM_ErrorCode rc = BDM_RC_OK;

   print("initialiseJTAGChain()\n");

   deviceCount = 0;
   irLength    = 0;
   deviceNum   = 0;

#if (DEBUG & DEBUG_NOTLIVE) == 0
   // Find total length of JTAG IR chain
   //===========================================================================
   //
   rc = USBDM_JTAG_Reset();
   if (rc != BDM_RC_OK)
      return rc;

   rc = USBDM_JTAG_SelectShift(JTAG_SHIFT_IR);    // Select IR chain
   if (rc != BDM_RC_OK)
      return rc;

   rc = findChainLength(irLength);       // Find length
   if (rc != BDM_RC_OK)
      return rc;

   print("initialiseJTAGChain(): Total length of JTAG IRs => %d\n", irLength);

   // Chain still in SHIFT-IR

   // Count number of JTAG devices
   //===========================================================================
   //
   // Force all devices to bypass mode (Command is all '1's)
   bitVector bypassAll(irLength,1); // Create vector to set all device IRs to BYPASS
   rc = USBDM_JTAG_Write(bypassAll.getLength(), JTAG_EXIT_IDLE,  bypassAll.getArray());
   if (rc != BDM_RC_OK)
      return rc;

   rc = USBDM_JTAG_SelectShift(JTAG_SHIFT_DR);  // Shifting IDCODE/BYPASS register
   if (rc != BDM_RC_OK)
      return rc;

   rc = findChainLength(deviceCount);       // Find length
   if (rc != BDM_RC_OK)
      return rc;

   print("initialiseJTAGChain(): Number of JTAG devices => %d\n", deviceCount);

   // Re-fill the bypass chain with 0 to ensure we can differentiate
   // BYPASS and IDCODE register in next phase
   rc = USBDM_JTAG_Write(deviceCount, JTAG_EXIT_IDLE, ZeroesVector.getArray());
   if (rc != BDM_RC_OK)
      return rc;

   // Get the IDCODE for each device
   // The number of devices should agree with the above!
   rc = USBDM_JTAG_Reset();          // Loads IDCODE/BYPASS command into IR
   if (rc != BDM_RC_OK)
      return rc;
   rc = USBDM_JTAG_SelectShift(JTAG_SHIFT_DR);  // Shifting IDCODE/BYPASS register
   if (rc != BDM_RC_OK)
      return rc;

   uint8_t  temp;
   // Read each device IDCODE
   for (deviceNum = 0; deviceNum < deviceCount; deviceNum++) {
      rc = USBDM_JTAG_Read(1, JTAG_STAY_SHIFT, &temp);       // Read a single bit
      if (rc != BDM_RC_OK)
         return rc;
      //printf("Temp = %x\n",temp);
      if (temp == 0) {// BYPASS register - No IDCODE
         devices[deviceNum].idcode     = 0x00;
         devices[deviceNum].irLength   = 2;
         devices[deviceNum].deviceData = KnownDevices::lookUpDevice(0x00);
      }
      else {
         rc = USBDM_JTAG_Read(31, JTAG_STAY_SHIFT, (uint8_t *)&buff); // Read rest of IDCODE
         if (rc != BDM_RC_OK)
            return rc;
         //printf("Buff = %lX\n",buff);
         swapEndian32(&buff);
         buff = (buff<<1) | 0x1;
         devices[deviceNum].idcode     = buff;
         const DeviceData *deviceData  = KnownDevices::lookUpDevice(buff);
         devices[deviceNum].irLength   = deviceData->instructionLength;
         devices[deviceNum].deviceData = deviceData;
      }
   }
   rc = USBDM_JTAG_Read(1, JTAG_EXIT_IDLE, &temp);  // Return JTAG to idle state
   if (rc != BDM_RC_OK)
      return rc;

   // Read IR reg - used for validation
   if (irReg != NULL)
      delete irReg;
   irReg = new bitVector(irLength, 0);
   rc = USBDM_JTAG_SelectShift(JTAG_SHIFT_IR);                      // Shifting IR register
   if (rc != BDM_RC_OK)
      return rc;
   rc = USBDM_JTAG_Read(irReg->getLength(), JTAG_STAY_SHIFT, irReg->getArray());  // Read IR reg (status)
   if (rc != BDM_RC_OK)
      return rc;
   rc = USBDM_JTAG_Write(bypassAll.getLength(), JTAG_EXIT_IDLE, bypassAll.getArray());  // fill safe command
   if (rc != BDM_RC_OK)
      return rc;
   print("initialiseJTAGChain(): JTAG IRs => %s\n", irReg->toBinString());
#endif

#if (DEBUG & DEBUG_DEVICES) != 0
   const DeviceData *deviceData;

   // Debug - add dummy devices
   buff = (0x04B<<12)|(FREESCALE_JEDEC<<1)|1;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;

   buff = (0x053<<12)|(FREESCALE_JEDEC<<1)|1;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;

   buff = (0x123<<12)|(FREESCALE_JEDEC<<1)|1;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;

   buff = 0x12345678UL|1;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;

   buff = 0x0;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;

   buff = 0x06E5E093UL;
   devices[deviceNum  ].idcode     = buff;
   deviceData                      = KnownDevices::lookUpDevice(buff);
   devices[deviceNum  ].irLength   = deviceData->instructionLength;
   devices[deviceNum++].deviceData = deviceData;
#endif

   deviceCount = deviceNum;

   print("initialiseJTAGChain(): Number of JTAG devices => %d\n", deviceCount);

   rc = validateJTAGChain();
   if (rc != BDM_RC_OK)
      return rc;

   reportDeviceIDs();

   return rc;
}

//! \brief Does some simple checks on the JTAG chain validity
//!
//! Checks length is compatible with devices found
//! Checks that IR reg boundaries appear to be correct (not conclusive)
//!
USBDM_ErrorCode JTAG_Chain::validateJTAGChain(void) {

   // Validate Chain
   // Expected IR length = IR length
   // If one unknown device then calculate its IR length
   unsigned int deviceNum;
   unsigned int irLengthCheck = 0;
   unsigned int unknownDeviceCount = 0;
   unsigned int unknownDevice=0;

   for (deviceNum = 0; deviceNum < deviceCount; deviceNum++) {
      if (devices[deviceNum].deviceData->irLengthKnown) {
         irLengthCheck += devices[deviceNum].irLength;
      }
      else {
         unknownDeviceCount++;
         unknownDevice = deviceNum;
      }
   }

   // Sanity check on IR length
   if ((irLengthCheck + (2*unknownDeviceCount)> irLength) ||
       ((unknownDeviceCount = 0) && (irLengthCheck != irLength))) {
      print("validateJTAGChain(): IR Length check failed, Unknown=%d, Measured IRL=%d, Expected IRL%s%d\n",
              unknownDeviceCount,
              irLength,
              unknownDeviceCount?">=":"=",
              irLengthCheck+2*unknownDeviceCount);
      return BDM_RC_OK;
   }

   // We can calculate the IR length of one unknown device
   if (unknownDeviceCount==1) {
      devices[unknownDevice].irLength = irLength - irLengthCheck;
   }

   return BDM_RC_FAIL;
}

//! Selects the device in the JTAG chain to be the target of
//! any operations.
//!
//! @param devNum : device number in the chain (0 based from TDO end)
//!
void JTAG_Chain::selectTargetDevice(unsigned int devNum) {
   if (devNum < deviceCount) {
      currentDeviceNum  = devNum;

      // Determine pre- and post-amble bits for the DR chain
      // assuming other devices are in BYPASS (1-bit each)
      drPreambleLength  = currentDeviceNum;
      drPostambleLength = deviceCount-currentDeviceNum-1;

      // Determine pre- and post-amble bits for the IR chain.
      // This requires the IR length of all devices.
      irPreambleLength = 0;
      for (devNum=0; devNum<currentDeviceNum; devNum++) {
         irPreambleLength += devices[devNum].irLength;
      }
      irPostambleLength = 0;
      for (devNum=currentDeviceNum+1; devNum<deviceCount; devNum++) {
         irPostambleLength += devices[devNum].irLength;
      }
   }
   print("selectTargetDevice(): currrent device #=%d\n", currentDeviceNum);
   print("selectTargetDevice(): DR Pre=%d, Post=%d\n", irPreambleLength, irPostambleLength);
   print("selectTargetDevice(): IR Pre=%d, Post=%d\n", drPreambleLength, drPostambleLength);
}

//! Write to the IR of the currently selected device
//! The other devices in the chain are set to BYPASS
//! The device is left in exitAction state
//!
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
void JTAG_Chain::writeIR(const uint8_t *data, const uint8_t exitAction) {
   // Other devices in the chain are set to BYPASS
   USBDM_JTAG_SelectShift(JTAG_SHIFT_IR);
   if (irPreambleLength > 0)
      USBDM_JTAG_Write(irPreambleLength,           JTAG_STAY_SHIFT,  OnesVector.getArray());
   if (irPostambleLength > 0) {
      USBDM_JTAG_Write(getTargetDevice().irLength, JTAG_STAY_SHIFT,  data);
      USBDM_JTAG_Write(irPostambleLength,          exitAction,   OnesVector.getArray());
   }
   else
      USBDM_JTAG_Write(getTargetDevice().irLength, exitAction,  data);
}

//! Write to the IR of the currently selected device
//! The other devices in the chain are set to BYPASS
//! The device is left in exitAction state
//!
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
void JTAG_Chain::writeIR(bitVector &data, const uint8_t exitAction) {
   if (data.getLength() != getTargetDevice().irLength) {
      print("JTAG_Chain::writeIR(): IR length incorrect");
      return;
   }
   writeIR(data.getArray(), exitAction);
}

//! Write to the DR of the currently selected device
//! The device is left in exitAction state
//!
//! @param numBits    : Number of bits to write
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::writeDR(const unsigned int numBits, const uint8_t *data, const uint8_t exitAction) {

   // Select DR chain (move to DR-SHIFT)
   USBDM_JTAG_SelectShift(JTAG_SHIFT_DR);

   // Check numBits agree with register length
 //  unsigned int chainLength = findChainLength();
 //  if (chainLength != numBits+drPreambleLength+drPostambleLength) {
 //     reset();
 //     return;
 //  }
   // Write value to DR of current device & move to JTAG_EXIT_IDLE
   if (drPostambleLength > 0) {
      USBDM_JTAG_Write(numBits,           JTAG_STAY_SHIFT,  data);
      USBDM_JTAG_Write(drPostambleLength, exitAction,   OnesVector.getArray());
   }
   else
      USBDM_JTAG_Write(numBits,           exitAction,  data);
}

//! Write to the DR of the currently selected device
//! The device is left in exitAction state
//!
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::writeDR(bitVector &data, const uint8_t exitAction) {
   writeDR(data.getLength(), data.getArray(), exitAction);
}

//! Write to the data chain of the currently selected device
//! The device is left in exitAction state.
//!
//! @param numBits    : Number of bits to write
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::write(const unsigned int numBits, const uint8_t *data, const uint8_t exitAction) {

   // Check numBits agree with register length
//   unsigned int chainLength = findChainLength();
//   if (chainLength != numBits+drPreambleLength+drPostambleLength) {
//      reset();
//      return;
//   }
   // Write value to DR of current device & move to exitAction
   if (drPostambleLength > 0) {
      USBDM_JTAG_Write(numBits,           JTAG_STAY_SHIFT,  data);
      USBDM_JTAG_Write(drPostambleLength, exitAction,   OnesVector.getArray());
   }
   else
      USBDM_JTAG_Write(numBits,           exitAction,  data);
}

//! Write to the DR of the currently selected device
//! The device is left in exitAction state
//!
//! @param data       : data to write
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::write(bitVector &data, const uint8_t exitAction) {
   write(data.getLength(), data.getArray(), exitAction);
}

//! Reads from the DR of the currently selected device
//! The device is left in exitAction state
//!
//! @param data       : data buffer for data read
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::readDR(bitVector &data, const uint8_t exitAction) {
uint8_t dummy[MAX_JTAG_DEVICES];

   USBDM_JTAG_SelectShift(JTAG_SHIFT_DR);
   if (drPreambleLength > 0)
      USBDM_JTAG_Read(drPreambleLength,  JTAG_STAY_SHIFT,  dummy);
   USBDM_JTAG_Read(data.getLength(),  exitAction,  data.getArray());
}

//! Reads from the DR of the currently selected device
//! The device is left in exitAction state
//!
//! @param numBits    : Number of bits to read
//! @param data       : data buffer for data read
//! @param exitAction : action for JTAG chain after operation
//!
//! @note: assumes other devices in the chain have been set to BYPASS
//!
void JTAG_Chain::readDR(const unsigned int numBits, uint8_t *data, const uint8_t exitAction) {
   uint8_t dummy[MAX_JTAG_DEVICES];

   USBDM_JTAG_SelectShift(JTAG_SHIFT_DR);
   if (drPreambleLength > 0)
      USBDM_JTAG_Read(drPreambleLength,  JTAG_STAY_SHIFT,  dummy);
   USBDM_JTAG_Read(numBits, exitAction,  data);
}

//! Determines the length of the currently configured JTAG chain
//! The device is left in IR/DR_SHIFT.
//!
//! @note: assumes all devices are already in IR/DR-SHIFT mode
//!
USBDM_ErrorCode JTAG_Chain::findChainLength(unsigned int &length) {
   uint8_t temp;
   USBDM_ErrorCode rc;

   // Fill chain with Zeroes
   rc = USBDM_JTAG_Write(ZeroesVector.getLength(),  JTAG_STAY_SHIFT,  ZeroesVector.getArray());
   if (rc != BDM_RC_OK)
      return rc;

   // Write a single '1'
   rc = USBDM_JTAG_Write(1, JTAG_STAY_SHIFT,  one);
   if (rc != BDM_RC_OK)
      return rc;

   // Count length of chain
   length = 0;
   do {
      rc = USBDM_JTAG_Read(1,  JTAG_STAY_SHIFT,  &temp);
      if (rc != BDM_RC_OK)
         return rc;
      length++;
   } while ((temp == 0) && (length < MAX_JTAG_IR_CHAIN_LENGTH));

   if (length >= MAX_JTAG_IR_CHAIN_LENGTH)
      return BDM_RC_FAIL;

   return BDM_RC_OK;
}

#if 1
//! Debugging - reports devices found in JTAG chain
//!
void JTAG_Chain::reportDeviceIDs(void) {
// Report the IDCODE for each device
uint32_t buff;
unsigned int devices;

   for (devices = 0; devices < JTAG_Chain::deviceCount; devices++) {
      buff = JTAG_Chain::devices[devices].idcode;
      print("Device[%d] : ",      devices);
      print("IDCODE => %8.8X, ", buff);
      print("PRN => %1.1X, ",   (buff>>28)&0x0F);
      print("DC => %2.2X, ",    (buff>>22)&0x3F);
      print("PIN => %3.3X, ",   (buff>>12)&0x3FF);
      print("JEDEC => %3.3X ",  (buff>>1)&0x7FF);
      if (JTAG_Chain::devices[devices].deviceData != NULL) {
         print("IrLength %s %d ",
                 JTAG_Chain::devices[devices].deviceData->irLengthKnown?"=":">",
                 JTAG_Chain::devices[devices].deviceData->instructionLength
                 );
         print("Name: %10s ",
                 JTAG_Chain::devices[devices].deviceData->name);
         print("Desc: %10s ",
                 JTAG_Chain::devices[devices].deviceData->description);
      }
      print("\n");
   }
}
#endif

