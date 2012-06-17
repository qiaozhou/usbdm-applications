/*
 * JTAG.hpp
 *
 *  Created on: 14/06/2009
 *      Author: podonoghue
 */
#ifndef JTAG_HPP_
#define JTAG_HPP_

#include "USBDM_API.h"

//! Information about a device in the JTAG chain
//!
class  JTAG_Device {
public:
   uint32_t          idcode;     //!< JTAG IDCODE value
   unsigned int      irLength;   //!< JTAG IR Register length
   const DeviceData *deviceData; //!< Data for this type of device
} ;

//! Table of information about devices found in the JTAG chain
//!
class  JTAG_Chain {
private:

public:
   //! Maximum number of devices in the JTAG chain
   static const unsigned int MAX_JTAG_DEVICES = 20;
   //! Maximum length of JTAG chain supported
   static const unsigned int MAX_JTAG_IR_CHAIN_LENGTH = MAX_JTAG_DEVICES*10;
#if (MAX_JTAG_IR_CHAIN_LENGTH > 240)
#error "JTAG Chain too long"
#endif
   static unsigned int   deviceCount;
   static unsigned int   currentDeviceNum;
   static JTAG_Device    devices[MAX_JTAG_DEVICES];
   static unsigned int   irLength;
   static bitVector     *irReg;
   static unsigned int   irPreambleLength;
   static unsigned int   irPostambleLength;
   static unsigned int   drPreambleLength;
   static unsigned int   drPostambleLength;

   static USBDM_ErrorCode  initialiseJTAGChain(void);
   static USBDM_ErrorCode  validateJTAGChain(void);
   static void             reportDeviceIDs(void);

   //!
   //! Resets all devices in the JTAG chain
   //!
   static void reset(void) {
      USBDM_JTAG_Reset();
   }
   static void writeIR(bitVector &data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void writeIR(const uint8_t  *data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void writeDR(bitVector &data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void writeDR(const unsigned int numBits, const uint8_t *data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void write(bitVector &data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void write(const unsigned int numBits, const uint8_t *data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void readDR(const unsigned int numBits, uint8_t *data, const uint8_t exitAction = JTAG_EXIT_IDLE);
   static void readDR(bitVector &data, const uint8_t exitAction = JTAG_EXIT_IDLE);

   //!
   //! Returns information about the currently selected device
   //!
   static JTAG_Device &getTargetDevice(void) {
      return devices[currentDeviceNum];
      }

   static void selectTargetDevice(unsigned int devNum);
   static USBDM_ErrorCode findChainLength(unsigned int &chainLength);

   // Useful vectors for use with JTAG interface
   static bitVector  OnesVector;    //!< All 1's vector
   static bitVector  ZeroesVector;  //!< All 0's vector
   static uint8_t const one[];
   static uint8_t const zero[];
} ;

#endif /* JTAG_HPP_ */
