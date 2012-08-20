/*! \file
    \brief Provides device data

    DeviceData.cpp

    \verbatim
    USBDM
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
    \endverbatim

    \verbatim
   Change History
   -============================================================================
   | 12 Aug 2011 | Changed device directory so avoid need for installation - pgo
   | 15 June     | Changed to std::tr1::shared_ptr (from boost)            - pgo
   +============================================================================
   \endverbatim
*/
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
#include "Common.h"
#include "ApplicationFiles.h"
#include "DeviceData.h"
#include "Log.h"
#include "DeviceXmlParser.h"

static string emptyString("");

//! Allows meaningful printing of memory ranges
//!
std::ostream& operator<<(std::ostream& s, const MemoryRegion mr) {
   s << setiosflags (ios_base::showbase | ios_base::hex);
   s << mr.getMemoryTypeName(mr.type) <<  "[";
   for (unsigned rangeIndex=0; rangeIndex<mr.memoryRanges.size(); rangeIndex++) {
      s << mr.memoryRanges[rangeIndex].start << "..." << mr.memoryRanges[rangeIndex].end;
      if (rangeIndex+1<mr.memoryRanges.size())
         s << ',';
   }
   if (mr.registerAddress != 0)
      s << ", reg=" << mr.registerAddress;
   if (mr.pageAddress != 0)
      s << ", pp=" << mr.pageAddress;
   s << "]";
   s << resetiosflags (ios_base::showbase | ios_base::uppercase | ios_base::hex);
   return s;
}

//! Mappings for Clock types
const EnumValuePair ClockTypes::clockNames[] = {
   EnumValuePair(  CLKEXT,       ("External") ),
   EnumValuePair(  S08ICGV1,     ("S08ICGV1") ),
   EnumValuePair(  S08ICGV2,     ("S08ICGV2") ),
   EnumValuePair(  S08ICGV3,     ("S08ICGV3") ),
   EnumValuePair(  S08ICGV4,     ("S08ICGV4") ),
   EnumValuePair(  S08ICSV1,     ("S08ICSV1") ),
   EnumValuePair(  S08ICSV2,     ("S08ICSV2") ),
   EnumValuePair(  S08ICSV2x512, ("S08ICSV2x512") ),
   EnumValuePair(  S08ICSV3,     ("S08ICSV3") ),
   EnumValuePair(  S08ICSV4,     ("S08ICSV4") ),
   EnumValuePair(  S08MCGV1,     ("S08MCGV1") ),
   EnumValuePair(  S08MCGV2,     ("S08MCGV2") ),
   EnumValuePair(  S08MCGV3,     ("S08MCGV3") ),
   EnumValuePair(  RS08ICSOSCV1, ("RS08ICSOSCV1") ),
   EnumValuePair(  RS08ICSV1,    ("RS08ICSV1") ),
   EnumValuePair(  CLKINVALID,   ("Invalid Clock") ),
   EnumValuePair(  0,              ""),
};

//! Returns the default non-volatile flash location for the clock trim value
//!
//! @param  clockType - clock type being queried
//!
//! @return Address of clock trim location(s)
//!
uint32_t DeviceData::getDefaultClockTrimNVAddress(ClockTypes_t clockType) {
#if TARGET == RS08
    return 0x3FFA;
#elif TARGET == CFV1
    return 0x3FE;
#elif TARGET == HCS08
   switch (clockType) {
      case S08ICGV1 :
      case S08ICGV2 :
      case S08ICGV3 :
      case S08ICGV4 :      return 0xFFBE;

      case S08ICSV1 :
      case S08ICSV2 :
      case S08ICSV2x512 :
      case S08ICSV3 :
      case RS08ICSOSCV1 :
      case RS08ICSV1 :     return 0xFFAE;

      case S08ICSV4 :      return 0xFF6E;

      case S08MCGV1 :
      case S08MCGV2 :
      case S08MCGV3 :      return 0xFFAE;

      case CLKINVALID :
      case CLKEXT :
      default :            return 0U;
   }
#else
   return 0;
#endif
}

//! Returns the default non-volatile flash location for the clock trim value
//!
//! @return Address of clock trim location(s) for the current clock type
//!
uint32_t DeviceData::getDefaultClockTrimNVAddress()  const {
   return getDefaultClockTrimNVAddress(clockType);
}

//! Returns the default (nominal) trim frequency for the currently selected clock
//!
//! @return clock trim frequency in Hz.
//!
uint32_t DeviceData::getDefaultClockTrimFreq(ClockTypes_t clockType) {
   switch (clockType) {
      case S08ICGV1 :
      case S08ICGV2 :
      case S08ICGV3 :
      case S08ICGV4 :
         return 243000UL;

      case S08ICSV1 :
      case S08ICSV2 :
      case S08ICSV2x512 :
      case S08ICSV3 :
      case S08ICSV4 :
      case RS08ICSOSCV1 :
      case RS08ICSV1 :
         return 31250UL;

      case S08MCGV1 :
      case S08MCGV2 :
      case S08MCGV3 :
         return 31250UL;

      case CLKINVALID :
      case CLKEXT :
      default :
         return 0U;
   }
}

//! Returns the default (nominal) trim frequency for the currently selected clock
//!
//! @return clock trim frequency in Hz.
//!
uint32_t DeviceData::getDefaultClockTrimFreq()  const {
   return getDefaultClockTrimFreq(clockType);
}

//! Returns the default clock register address
//!
//! @return clock register address
//!
uint32_t DeviceData::getDefaultClockAddress(ClockTypes_t clockType) {
#if TARGET == CFV1
   return 0xFF8048; // There really isn't a sensible default
#elif TARGET == RS08
   return 0x0238;   // There really isn't a sensible default
#elif TARGET == HCS08
   switch (clockType) {
      case S08ICGV1 :
      case S08ICGV2 :
      case S08ICGV3 :
      case S08ICGV4 :
      case S08MCGV1 :
      case S08MCGV2 :
      case S08MCGV3 :
         return 0x0048;

      case S08ICSV1 :
      case S08ICSV2x512 :
         return 0x0038;

      case S08ICSV2 :
         return 0x0048;

      case S08ICSV3 :
         return 0x0038;

      case RS08ICSOSCV1 :
         return 0x023C;

      case RS08ICSV1 :
      case CLKINVALID :
      case CLKEXT :
      default :
         return 0U;
   }
#else
   return 0;
#endif
}

//! Returns the default (nominal) trim frequency for the currently selected clock
//!
//! @return clock trim frequency in Hz.
//!
uint32_t DeviceData::getDefaultClockAddress()  const {
   return getDefaultClockAddress(clockType);
}

//! Determines the clock type from a string description
//!
ClockTypes_t ClockTypes::getClockType(const std::string &typeName) {
int sub;

   for (sub=0; !clockNames[sub].getName().empty(); sub++) {
      if (clockNames[sub].getName().compare(typeName) == 0) {
         return (ClockTypes_t) clockNames[sub].getValue();
      }
   }
   print("getClockType(): Error: invalid clock type '%s'\n", typeName.c_str());
   return CLKINVALID;
}

//! Determines the clock type from a string description
//!
ClockTypes_t ClockTypes::getClockType(const char *_typeName) {
   string typeName(_typeName);
   return getClockType(typeName);
}

int ClockTypes::getClockIndex(const std::string &typeName) {
   return (int)getClockType(typeName);
}

int ClockTypes::getClockIndex(ClockTypes_t clockType) {
   return (int)clockType;
}

//! Determines the clock name from a clock type
//!
//! @param clockType Clocktype to map to name
//!
//! @return string describing the clock
//!
const string ClockTypes::getClockName(ClockTypes_t clockType) {
int sub;

   for (sub=0; !clockNames[sub].getName().empty(); sub++) {
      if (clockNames[sub].getValue() == clockType)
         return clockNames[sub].getName();
   }
   print("getClockName(): Error: invalid clock type %d\n", clockType);
   return emptyString;
}

//! Checks if the SDID is used by the device
//!
//! @param desiredSDID The SDID to check against
//!
//! @return true/false result of check
//!
bool DeviceData::isThisDevice(uint32_t  desiredSDID, bool acceptZero) const {
   unsigned int index;

   if (((targetSDIDs.size() == 0)||(targetSDIDs[0]) == 0) && acceptZero) {
      // 0x0000 or empty matches all
      return true;
   }
   for (index=0; index<targetSDIDs.size(); index++) {
//      print("Comparing 0x%04X with 0x%04X\n", targetSDID[index], desiredSDID);
      if (((targetSDIDs[index]^desiredSDID)&targetSDIDMask) == 0x0000)
         return true;
   }
   return false;
}

//! Checks if any of the SDIDs provided are used by the device
//!
//! @param desiredSDIDs map of {SDIDaddress,SDIDs} to check against
//!
//! @return true/false result of check
//!
bool DeviceData::isThisDevice(std::map<uint32_t,uint32_t> desiredSDIDs, bool acceptZero) const {
   map<uint32_t,uint32_t>::iterator sdidEntry;

   if (((targetSDIDs.size() == 0)||(targetSDIDs[0]) == 0) && acceptZero) {
      // 0x0000 or empty matches all
      return true;
   }
   for (sdidEntry = desiredSDIDs.begin();
        sdidEntry != desiredSDIDs.end();
        sdidEntry++) {
      if ((SDIDAddress == sdidEntry->first) && isThisDevice(sdidEntry->second, acceptZero ))
         return true;
      }
   return false;
}

//! Determines the memory region containing an address in given memory space
//!
//! @param address     - The address to check
//! @param memorySpace - Memory space to check (MS_None, MS_Program, MS_Data)
//!
//! @return shared_ptr for Memory region found (or NULL if none found)
//!
MemoryRegionPtr DeviceData::getMemoryRegionFor(uint32_t address, MemorySpace_t memorySpace) {
   // Check cached location to avoid searching in many cases
   if ((lastMemoryRegionUsed != NULL) &&
        lastMemoryRegionUsed->isCompatibleType(memorySpace) &&
        lastMemoryRegionUsed->contains(address)
        ) {
      return lastMemoryRegionUsed;
   }
   for (int index=0; ; index++) {
      lastMemoryRegionUsed = getMemoryRegion(index);
      if (lastMemoryRegionUsed == NULL) {
         break;
      }
      if (lastMemoryRegionUsed->isCompatibleType(memorySpace) &&
          lastMemoryRegionUsed->contains(address)
          ) {
         return lastMemoryRegionUsed;
      }
   }
   return MemoryRegionPtr();
}

//! Determines the memory type for an address
//!
//! @param address     - The address to check
//! @param memorySpace - Memory space to check (MS_None, MS_Program, MS_Data)
//!
//! @return Memory type of region (or MemInvalid if no information available)
//!
MemType_t DeviceData::getMemoryType(uint32_t address, MemorySpace_t memorySpace) {
   MemoryRegionPtr memoryRegion = getMemoryRegionFor(address, memorySpace);
   if (memoryRegion != NULL) {
      return memoryRegion->getMemoryType();
   }
   return MemInvalid;
}

//! Determines the page number for an address
//!
//! @param address - The address to check
//!
//! @return page number (PPAGE value) or  MemoryRegion::NoPageNo if not paged/found
//!
uint16_t DeviceData::getPageNo(uint32_t address) {
   // Check cached location to avoid searching in most cases
   if ((lastMemoryRegionUsed != NULL) && lastMemoryRegionUsed->contains(address))
      return lastMemoryRegionUsed->getPageNo(address);

   for (int index=0; ; index++) {
      lastMemoryRegionUsed = getMemoryRegion(index);
      if (lastMemoryRegionUsed == NULL)
         break;
      if (lastMemoryRegionUsed->contains(address))
         return lastMemoryRegionUsed->getPageNo(address);
   }
   return MemoryRegion::NoPageNo;
}

//! Searches the known devices for a device with given name
//!
//! @param targetName - Name of device
//!
//! @returns entry found or NULL if no suitable device found
//!
//! @note - If the device is an alias then it will return the true device
//!
const DeviceData *DeviceDataBase::findDeviceFromName(const string &targetName) {

   static int recursionCheck = 0;
//   print("findDeviceFromName(%s)\n", (const char *)targetName.c_str());

   if (recursionCheck++>5) {
      throw MyException("Recursion limit in DeviceDataBase::findDeviceFromName");
   }
   // Note - Assumes ASCII string
   char buff[50];
   strncpy(buff, targetName.c_str(), sizeof(buff));
   buff[sizeof(buff)-1] = '\0';
   strupr(buff);

   const DeviceData *theDevice = NULL;
   vector<DeviceData *>::iterator it;
   for (it = deviceData.begin(); it != deviceData.end(); it++) {
      if (strcmp((*it)->getTargetName().c_str(), buff) == 0) {
         theDevice = *it;
         print("findDeviceFromName(%s) found %s%s\n",
               buff, (const char *)(theDevice->getTargetName().c_str()), theDevice->isAlias()?"(alias)":"");
         if (theDevice->isAlias()) {
            theDevice = findDeviceFromName(theDevice->getAliasName());
         }
         break;
      }
   }
   recursionCheck--;
   if (theDevice == NULL) {
      print("findDeviceFromName(%s) => Device not found\n", (const char *)targetName.c_str());
   }
   return theDevice;
}

//! Searches the known devices for a device with given name
//!
//! @param targetName - Name of device
//!
//! @returns index or -1 if not found
//!
int DeviceDataBase::findDeviceIndexFromName(const string &targetName) {

   vector<DeviceData *>::iterator it;
   for (it = deviceData.begin(); it != deviceData.end(); it++) {
      if ((*it)->getTargetName().compare(targetName) == 0)
         return it - deviceData.begin();
   }
   print("findDeviceFromName(%s) => Device not found\n", targetName.c_str());
   return -1;
}

//! A generic device to use as a default
const DeviceData *DeviceDataBase::defaultDevice = NULL;

//! \brief Loads the known devices list from the configuration file.
//!
void DeviceDataBase::loadDeviceData(void) {
#if TARGET == HCS08
   #define configFilename "hcs08_devices.xml"
#elif TARGET == RS08
   #define configFilename "rs08_devices.xml"
#elif TARGET == CFV1
   #define configFilename "cfv1_devices.xml"
#elif TARGET == HC12
   #define configFilename "hcs12_devices.xml"
#elif TARGET == CFVx
   #define configFilename "cfvx_devices.xml"
#elif (TARGET == ARM) || (TARGET == ARM_SWD)
   #define configFilename "arm_devices.xml"
#elif TARGET == MC56F80xx
   #define configFilename "dsc_devices.xml"
#endif

   print("DeviceDataBase::loadDeviceData()\n");
   try {
      string appFilePath = getApplicationFilePath("Device_data/" configFilename);
      if (appFilePath.empty()) {
         throw MyException("DeviceDataBase::loadDeviceData() - failed to find device database file");
      }
      DeviceXmlParser::loadDeviceData(appFilePath, this);
   }
   catch (MyException &exception) {
      // Create dummy default device
//      defaultDevice = *aDevice.insert(aDevice.end(), new DeviceData()).base();
      print("DeviceDataBase::loadDeviceData() - Exception \'%s\'\n", exception.what());
      deviceData.clear();
      DeviceData *aDevice = new DeviceData();
      aDevice->setTargetName("Invalid Database");
      setDefaultDevice(aDevice);
      addDevice(aDevice);
      throw exception;
   }
   catch (...) {
      print("DeviceDataBase::loadDeviceData() - Unknown exception\n");
      deviceData.clear();
      DeviceData *aDevice = new DeviceData();
      aDevice->setTargetName("Invalid Database");
      setDefaultDevice(aDevice);
      addDevice(aDevice);
      throw MyException("DeviceDataBase::loadDeviceData() - Unknown exception");
   }
#if defined(LOG) && 0
   listDevices();
#endif
   print("DeviceDataBase::loadDeviceData() - %d devices loaded\n", deviceData.size());
}

void DeviceDataBase::listDevices() {

   vector<DeviceData *>::iterator it;
   int lineCount = 0;
   try {
      for (it = deviceData.begin(); it != deviceData.end(); it++) {
         const DeviceData *deviceData = (*it);
         if (deviceData == NULL) {
            print("Null device pointer\n");
            continue;
         }
         bool aliased = deviceData->isAlias();
         if (aliased) {
            deviceData = findDeviceFromName(deviceData->getTargetName());
            if (deviceData == NULL) {
               print("Failed to find alias\n");
               continue;
            }
         }
#if (TARGET == ARM) || (TARGET == CFVx) || (TARGET==ARM_SWD)
         if (lineCount == 0) {
            print("\n"
                  "#                     SDID                                                \n"
                  "#    Target           Address    SDID          Script? Flash?              \n"
                  "#=========================================================================\n");
         }
         print("%-20s%s 0x%08X 0x%08X %7s %7s\n",
               deviceData->getTargetName().c_str(), aliased?"(A)":"   ",
//                  ClockTypes::getClockName(deviceData->getClockType()).c_str(),
//                  deviceData->getClockAddress(),
//                  deviceData->getClockTrimNVAddress(),
//                  deviceData->getClockTrimFreq()/1000.0,
               deviceData->getSDIDAddress(),
               deviceData->getSDID(),
               deviceData->getFlashScripts()?"Yes":"No",
               deviceData->getFlashProgram()?"Yes":"No"
         );
#else
         if (lineCount == 0) {
            print("\n"
                  "#                      RAM          Clock    Clock   NVTRIM    Trim                                     \n"
                  "# Target         Start     End      Name     Addr     Addr     Freq.  SDIDA    SDID  Scripts? FlashP?   \n"
                  "#=======================================================================================================\n");
         }
         print("%-14s%s 0x%06X 0x%06X %10s 0x%06X 0x%06X %6.2f %08X %08X %4s %4s\n",
               deviceData->getTargetName().c_str(), aliased?"(A)":"   ",
               deviceData->getRamStart(), deviceData->getRamEnd(),
               ClockTypes::getClockName(deviceData->getClockType()).c_str(),
               deviceData->getClockAddress(),
               deviceData->getClockTrimNVAddress(),
               deviceData->getClockTrimFreq()/1000.0,
               deviceData->getSDIDAddress(),
               deviceData->getSDID(),
               deviceData->getFlashScripts()?"Y":"N",
               deviceData->getFlashProgram()?"Y":"N"
         );
#endif
#if 1
         for (int regionNum=0; deviceData->getMemoryRegion(regionNum) != NULL; regionNum++) {
            MemoryRegionPtr reg=deviceData->getMemoryRegion(regionNum);
            print("      %10s: ", reg->getMemoryTypeName());
            if (reg->getFlashprogram())
               print("FP=Yes, ");
            else
               print("        ");
            print("SS=%6d ", reg->getSectorSize());
            for(unsigned index=0; ; index++) {
               const MemoryRegion::MemoryRange *memoryRange = reg->getMemoryRange(index);
               if (memoryRange == NULL) {
                  break;
               }
               print("(0x%08X,0x%08X", memoryRange->start, memoryRange->end);
               if ((memoryRange->pageNo != MemoryRegion::DefaultPageNo) &&
                   (memoryRange->pageNo != MemoryRegion::NoPageNo) &&
                   (memoryRange->pageNo != ((memoryRange->start>>16)&0xFF))){
                  print(",P=0x%02X)", memoryRange->pageNo);
               }
               else {
                  print(")", memoryRange->start, memoryRange->end);
               }
            }
            print("\n");
         }
         print("\n");
#endif
//         const TclScript *tclScript;
//         tclScript = deviceData->getPreSequence();
//         if (tclScript != NULL) {
//            print("preSequence \n=========================\n"
//                  "%s\n========================\n", tclScript->toString().c_str());
//         }
//         tclScript = deviceData->getPostSequence();
//         if (tclScript != NULL) {
//            print("postSequence \n=========================\n"
//                  "%s\n========================\n", tclScript->toString().c_str());
//         }
//         tclScript = deviceData->getUnsecureSequence();
//         if (tclScript != NULL) {
//            print("unsecureSequence \n=========================\n"
//                  "%s\n========================\n", tclScript->toString().c_str());
//         }
         //         std::stringstream buffer;
         //         buffer.exceptions(std::ios::badb(*it) | std::ios::failbit);
         //         buffer.str("");
         //         buffer << setw(15) << deviceData->getTargetName();
         //         buffer << setw(14) << ClockTypes::getClockName(deviceData->getClockType());
         //         buffer << setiosflags (ios_base::showbase | ios_base::hex);
         //         buffer << setw(8) << deviceData->getClockAddress();
         //         buffer << setw(8) << deviceData->getClockAddress();
         //         buffer << resetiosflags (ios_base::showbase | ios_base::hex);
         //         buffer << setw(6) << deviceData->getClockTrimFreq()/1000.0;
         //         for (int memIndex=0; true; memIndex++) {
         //            const MemoryRegion *pMemRegion = deviceData->getMemoryRegion(memIndex);
         //            if (pMemRegion == NULL)
         //               break;
         //             buffer << " " << *pMemRegion;
         //         }
         //         for (int sdidIndex=0; true; sdidIndex++) {
         //            uint32_t sdid = deviceData->getSDID(sdidIndex);
         //            if (sdid == 0)
         //               break;
         //             buffer << " " << sdid;
         //         }
         //         buffer << endl;
         //         print(buffer.str().c_str());
         lineCount = (lineCount+1)%40;
      }
   } catch (...) {

   }
#if 0
   print("================================================\n"
         "Shared data, #elements = %d\n", sharedInformation.size());
   map<const string, SharedInformationItemPtr>::iterator mapIt;
   for (mapIt = sharedInformation.begin(); mapIt != sharedInformation.end(); mapIt++) {
      std::map<const std::string, SharedInformationItem> sharedInformation;
      const string &key   = mapIt->first;
//      const TclScript *item     = dynamic_cast<const TclScript *> (mapIt->second);
      print("key = %s\n", key.c_str());
//      if (item == NULL) {
//         print("Failed cast\n");
//      }
//      else {
//         print("%s", item->toString().c_str());
//      }
   }
   print("================================================\n");
#endif
}

DeviceDataBase::~DeviceDataBase() {
   print("DeviceDataBase::~DeviceDataBase()\n");
   sharedInformation.clear();

   std::vector<DeviceData *>::iterator itDevice = deviceData.begin();
   while (itDevice != deviceData.end()) {
      (*itDevice)->valid = false;
      delete (*itDevice);
      itDevice++;
   }
   deviceData.clear();
}

DeviceData::~DeviceData() {
//   print("DeviceData::~DeviceData()\n");
}

std::string SecurityInfo::getSecurityInfo() const
{
    return securityInfo;
}

unsigned SecurityInfo::getSize() const
{
    return size;
}

bool SecurityInfo::getMode() const
{
    return mode;
}

int aToi(char ch) {
   if ((ch >= '0') && (ch <= '9'))
      return ch -'0';
   else if ((ch >= 'a') && (ch <= 'f'))
      return ch -'a' + 10;
   else if ((ch >= 'A') && (ch <= 'F'))
      return ch -'A' + 10;
   else
      return -1;
}

const uint8_t *SecurityInfo::getData() {
   unsigned       cSub = 0;
   static uint8_t data[40];
   for (unsigned sub=0; sub<size; sub++) {
      while ((aToi(securityInfo[cSub]) < 0) && (cSub < securityInfo.length())) {
         cSub++;
      }
      data[sub]  = aToi(securityInfo[cSub++])<<4;
      data[sub] += aToi(securityInfo[cSub++]);
   }
   return data;
}

//! Obtain string describing the memory type
//!
//! @param memoryType - Memory type
//!
//! @return - ptr to static string describing type
//!
const char *MemoryRegion::getMemoryTypeName(MemType_t memoryType) {
   static const char *names[] = {
            "MemInvalid",
            "MemRAM",
            "MemEEPROM",
            "MemFLASH",
            "MemFlexNVM",
            "MemFlexRAM",
            "MemROM",
            "MemIO",
            "MemPFlash",
            "MemDFlash",
            "MemXRAM",
            "MemPRAM",
            "MemXROM",
            "MemPROM",
   };
   if((unsigned )((memoryType)) >= (sizeof (names) / sizeof (names[0])))
      memoryType = (MemType_t)((0));
   return names[memoryType];
}

//! Indicates if a programmable type e.g Flash, eeprom etc.
//!
//! @param memoryType - Memory type
//!
//! @return - true/false result
//!
bool MemoryRegion::isProgrammableMemory(MemType_t memoryType) {
   switch (memoryType) {
      case MemEEPROM  :
      case MemFLASH   :
      case MemFlexNVM :
      case MemPFlash  :
      case MemDFlash  :
      case MemXROM    :
      case MemPROM    :
         return true;
         break;
      default :
         return false;
   }
}

//! Indicates if a memory type is compatible with a memory space e.g. MemPROM lies in MS_Program
//!
//! @param memoryType  - Memory type
//! @param memorySpace - Memory space
//!
//! @return - true/false result
//!
bool MemoryRegion::isCompatibleType(MemType_t memoryType, MemorySpace_t memorySpace) {
   switch(memorySpace&MS_SPACE) {
      default         : return true;
      case MS_Program : return ((memoryType == MemPRAM) || (memoryType == MemPROM));
      case MS_Data    : return ((memoryType == MemXRAM) || (memoryType == MemXROM));
   }
}


//=============================================================================================

//! Return minimum required ration between backing store in EEPROM and EEEPROM
//!
//! @return ratio e.g. 16 => EEPROM size >= 16* EEEPROM size
//!
unsigned FlexNVMInfo::getBackingRatio() const {
    return backingRatio;
}

//! Adds a permitted EEEPROM values for use in partition command
//!
//! @param eeepromSizeValue - Description of value
//!
void FlexNVMInfo::addEeepromSizeValues(const EeepromSizeValue &eeepromSizeValue) {
   this->eeepromSizeValues.push_back(eeepromSizeValue);
}

//! Adds a permitted Partition values for use in partition command
//!
//! @param flexNvmPartitionValue  - Description of value
//!
void FlexNVMInfo::addFlexNvmPartitionValues(const FlexNvmPartitionValue &flexNvmPartitionValue) {
   this->flexNvmPartitionValues.push_back(flexNvmPartitionValue);
}

void FlexNVMInfo::setBackingRatio(unsigned  backingRatio) {
    this->backingRatio = backingRatio;
}


