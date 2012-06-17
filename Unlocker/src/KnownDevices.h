/*
 * KnownDevices.hpp
 *
 *  Created on: 14/06/2009
 *      Author: podonoghue
 */

#ifndef KNOWNDEVICES_HPP_
#define KNOWNDEVICES_HPP_

#include "Common.h"

//! Information about a known target device
class DeviceData {
public:
   unsigned int         index;                //<! Index in list box
   unsigned int         idcode;               //<! JTAG IDCODE (REV field is ignored)
   unsigned int         instructionLength;    //<! Length of JTAG instruction register
   unsigned int         dataLength;           //<! Length of JTAG instruction register
   int                  irLengthKnown;        //<! Length of JTAG IR
   unsigned int         unlockInstruction;    //<! JTAG LOCKOUT_RECOVERY instruction
   unsigned int         idcodeInstruction;    //<! JTAG IDCODE instruction
   unsigned int         flashEquation;        //<! Flash divider equation
   unsigned long        fMin;                 //<! Minimum Flash clock freq.
   unsigned long        fMax;                 //<! Maximum Flash clock freq.
   const char          *name;                 //<! Name of device
   const char          *description;          //<! Description of device
};

//! Table of known target devices
class KnownDevices {
public:
   static const unsigned int     MAX_KNOWN_DEVICES = 200;

   static unsigned int           deviceCount;
   static DeviceData             deviceData[MAX_KNOWN_DEVICES];

   static const DeviceData      *lookUpDevice(uint32_t idcode);
   static void                   loadConfigFile(void);

   // Some 'dummy' devices types
   static const DeviceData nonFreescaleDevice;
   static const DeviceData unknownDevice;
   static const DeviceData disabledDevice;
   static const DeviceData customDevice;
   static const DeviceData unRecognizedDevice;
} ;

// Macros for use with IDCODE
#define JEDEC_ID(x)        (((x)>>1)&0x7FF)   //!< Extract JEDEC code from IDCODE
#define PART_NUM(x)        (((x)>>12)&0xFFFF) //!< Extract Manufacturer part number from IDCODE
#define REVISION(x)        (((x)>>28)&0xF)    //!< Extract Revision from IDCODE
#define FREESCALE_PIN(x)   (((x)>>12)&0x3FF)  //!< Extract Freescale PIN code from IDCODE

//! JEDEC code for a freescale device
#define FREESCALE_JEDEC (0x00E)

#endif /* KNOWNDEVICES_HPP_ */
