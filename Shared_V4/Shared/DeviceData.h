/*
 * DeviceData.hpp
 *
 *  Created on: 27/02/2010
 *      Author: podonoghue
 */

#ifndef DEVICEDATA_HPP_
#define DEVICEDATA_HPP_
#include <vector>
#include <map>
#include <string>
#include <streambuf>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <tr1/memory>
#include "Common.h"
#include "MyException.h"
#include "Log.h"
#include "USBDM_API.h"

//! RS08/HCS08/CFV1 clock types
//!
typedef enum {
   CLKINVALID = -1,
   CLKEXT = 0,
   S08ICGV1,S08ICGV2,S08ICGV3,S08ICGV4,
   S08ICSV1,S08ICSV2,S08ICSV2x512,S08ICSV3,S08ICSV4,
   RS08ICSOSCV1,
   RS08ICSV1,
   S08MCGV1,S08MCGV2,S08MCGV3,
} ClockTypes_t;

//! memory types
//!
typedef enum {
   MemInvalid  = 0,
   MemRAM      = 1,
   MemEEPROM   = 2,
   MemFLASH    = 3,
   MemFlexNVM  = 4,
   MemFlexRAM  = 5,
   MemROM      = 6,
   MemIO       = 7,
   MemPFlash   = 8,
   MemDFlash   = 9,
   MemXRAM     = 10, // DSC
   MemPRAM     = 11, // DSC
   MemXROM     = 12, // DSC
   MemPROM     = 13, // DSC
} MemType_t;

typedef enum {
  AddrLinear,
  AddrPaged,
} AddressType;

class EnumValuePair {
private:
   int                 value;
   const std::string   name;
public:
   int               getValue() const { return value; };
   const std::string getName()  const { return name; };
   EnumValuePair(int value, const std::string &name)
       : value(value), name(name) {
   }
};

//! Information on clock types
//!
class ClockTypes {
private:
   //! Mappings for Clock types
   static const EnumValuePair clockNames[];

public:
   static ClockTypes_t        getClockType(const std::string &typeName);
   static ClockTypes_t        getClockType(const char *typeName);
   static const std::string   getClockName(ClockTypes_t clockType);
   static int                 getClockIndex(const std::string &typeName);
   static int                 getClockIndex(ClockTypes_t clockType);
   ClockTypes() {
   };
};

//! SecurityInfo options when programming
//! These options affect the handling of the NVOPT/NVSEC location
//!
typedef enum {
   SEC_DEFAULT      = 0,   //!< Leave Flash image unchanged
   SEC_SECURED      = 1,   //!< Make Flash image secured
   SEC_UNSECURED    = 2,   //!< Make Flash image unsecured
   SEC_SMART        = 3,   //!< Modify flash image unless already modified
   SEC_INTELLIGENT  = 3,   //!< Modify flash image unless already modified
} SecurityOptions_t;

class SharedInformationItem {
public:
   virtual ~SharedInformationItem() {};
   int operator==(const SharedInformationItem &other) {
      return this == &other;
   }
};

typedef std::tr1::shared_ptr<SharedInformationItem> SharedInformationItemPtr;

class TclScript: public SharedInformationItem {
private:
   std::string script;
public:
   TclScript(std::string script) :
      script(script) {
//      print("TclScript()\n");
   }
   const std::string toString() const {
      return std::string(
            "TCL Script\n"
            "============================================================================" +
            script +
            "============================================================================\n");
   }
   const std::string getScript() const {
      return script;
   }
   ~TclScript() {
//      print("~TclScript()\n");
   }
};
typedef std::tr1::shared_ptr<TclScript>   TclScriptPtr;

class FlashProgram: public SharedInformationItem {
public:
   std::string flashProgram;
   FlashProgram(std::string script) :
      flashProgram(script) {
//      print("FlashProgram()\n");
   }
   const std::string toString() const {
      return std::string(
            "flashProgram\n"
            "=========================================================================" +
            flashProgram +
            "=========================================================================\n");
   }
   ~FlashProgram() {
//      print("~FlashProgram()\n");
   }
};
typedef std::tr1::shared_ptr<FlashProgram> FlashProgramPtr;

class SecurityInfo: public SharedInformationItem {
private:
   unsigned    size;
   bool        mode;
   std::string securityInfo;

public:
    SecurityInfo(int size, bool mode, std::string data)
    : size(size), mode(mode), securityInfo(data)
    {
//        print("SecurityInfo()\n");
    }
    ~SecurityInfo() {
//        print("~SecurityInfo()\n");
    }
    const std::string toString() const {
       return std::string("SecurityInfo - ") + std::string(mode?"secure":"unsecure") +
              "\n========================================================================="
              + std::string(securityInfo) +
              "=========================================================================\n";
    }
    std::string    getSecurityInfo() const;
    unsigned       getSize() const;
    bool           getMode() const;
    const uint8_t *getData();
};
typedef std::tr1::shared_ptr<SecurityInfo> SecurityInfoPtr;

class FlexNVMInfo: public SharedInformationItem {

public:
   class EeepromSizeValue {
   public:
      std::string    description;   // Description of this value
      uint8_t        value;         // EEPROM Data Set Size (as used in Program Partition command)
      unsigned       size;          // EEEPROM size in bytes (FlexRAM used for EEPROM emulation)
      EeepromSizeValue(std::string description, uint8_t value, unsigned size)
      : description(description), value(value), size(size)
      {}
   } ;
   class FlexNvmPartitionValue {
   public:
      std::string    description;  // Description of this value
      uint8_t        value;        // FlexNVM Partition Code (as used in Program Partition command)
      unsigned       backingStore; // EEPROM backing store size in bytes
      FlexNvmPartitionValue(std::string description, uint8_t value, unsigned backingStore)
      : description(description), value(value), backingStore(backingStore)
      {}
   };

private:
    unsigned backingRatio;
    std::vector<EeepromSizeValue>      eeepromSizeValues;
    std::vector<FlexNvmPartitionValue> flexNvmPartitionValues;

public:
    FlexNVMInfo(int backingRatio = 16)
    :backingRatio(backingRatio)
    {
        print("FlexNVMInfo()\n");
    }

    ~FlexNVMInfo()
    {
        print("~FlexNVMInfo()\n");
    }

    const std::string toString() const
    {
        return std::string("FlexNVMInfo - \n");
    }

    std::vector<EeepromSizeValue>      &getEeepromSizeValues();
    std::vector<FlexNvmPartitionValue> &getFlexNvmPartitionValues();

    void addEeepromSizeValues(const EeepromSizeValue &eeepromSizeValue);
    void addFlexNvmPartitionValues(const FlexNvmPartitionValue &flexNvmPartitionValue);

    unsigned getBackingRatio() const;
    void setBackingRatio(unsigned  backingRatio);
};
typedef std::tr1::shared_ptr<FlexNVMInfo> FlexNVMInfoPtr;

// Represents a collection of related memory ranges
//
// This may be used to represent a non-contiguous range of memory locations that are related
// e.g. two ranges of Flash that are controlled by the same Flash controller as occurs in some HCS08s
//
class MemoryRegion {

private:
   MemoryRegion(MemoryRegion &);
//   MemoryRegion &operator=(MemoryRegion &);

public:
   static const uint16_t DefaultPageNo = 0xFFFF;
   static const uint16_t NoPageNo      = 0xFFFE;
   class MemoryRange {
      public:
         uint32_t start;
         uint32_t end;
         uint16_t pageNo;
   };
   std::vector<MemoryRange> memoryRanges;  //!< Memory ranges making up this region
   MemType_t       type;                   //!< Type of memory regions
   AddressType     addressType;            //!< Linear/Paged addressing
   uint32_t        registerAddress;        //!< Control register addresses
   uint32_t        pageAddress;            //!< Paging register address
   uint32_t        securityAddress;        //!< Non-volatile option address
   uint32_t        sectorSize;             //!< Size of sectors i.e. minimum erasable unit
   uint32_t        lastIndexUsed;          //!< Last used memoryRanges index
   FlashProgramPtr flashProgram;           //!< Region-specific flash algorithm
   SecurityInfoPtr unsecureInfo;           //!< Region-specific unsecure data
   SecurityInfoPtr secureInfo;             //!< Region-specific secure data

private:
   //! Find the index of the memory range containing the given address
   //!
   //! @param address - address to look for
   //!
   //! @return range index or -1 if not found
   //!
   //! @note - Uses cache
   //!
   int findMemoryRangeIndex(uint32_t address) {
      if (type == MemInvalid)
         return -1;
      // Check cached address
      if ((lastIndexUsed < memoryRanges.size()) &&
          (memoryRanges[lastIndexUsed].start <= address) && (address <= memoryRanges[lastIndexUsed].end)) {
            return lastIndexUsed;
         }
      // Look through all memory ranges
      for (lastIndexUsed=0; lastIndexUsed<memoryRanges.size(); lastIndexUsed++) {
         if ((memoryRanges[lastIndexUsed].start <= address) && (address <= memoryRanges[lastIndexUsed].end)) {
            return lastIndexUsed;
         }
      }
//      // clear cache
//      lastIndexUsed = 1000;
      return -1;
   }

public:
   MemoryRegion (MemType_t type = MemInvalid,
                 uint32_t registerAddress = 0,
                 uint32_t pageAddress = 0,
                 uint32_t securityAddress = 0,
                 uint32_t sectorSize = 0) :
      type(type),
      addressType(AddrPaged),
      registerAddress(registerAddress),
      pageAddress(pageAddress),
      securityAddress(securityAddress),
      sectorSize(sectorSize),
      lastIndexUsed((uint32_t)-1)
   {
   }
   //! Add a memory range to this memory region
   //!
   //! @param startAddress - start address (inclusive)
   //! @param endAddress   - end address (inclusive)
   //! @param pageNo       - page number (if used)
   //!
   void addRange (uint32_t startAddress, uint32_t endAddress, uint16_t pageNo=DefaultPageNo) {
//      if (memRangeCount >= (sizeof(memoryRanges)/sizeof(memoryRanges[0])))
//         throw MyException("Too many memory ranges");
//      print("addRange(0x%6X, 0x%6X, 0x%2X)\n", startAddress, endAddress, pageNo);
      if (memoryRanges.size() == memoryRanges.capacity()) {
         unsigned newCapacity = 2*memoryRanges.capacity();
         if (newCapacity < 8) {
            newCapacity = 8;
         }
//         print("addRange() - resizing %d=>%d\n", memoryRanges.capacity(), newCapacity);
         memoryRanges.reserve(newCapacity);
      }
      if (pageAddress == 0) {
         // Non-paged memory
         pageNo = NoPageNo;
      }
      else if (pageNo == DefaultPageNo) {
         // Page no is usually just the 8-bit address
         pageNo = (startAddress>>16)&0xFF;
      }
      MemoryRange memoryRangeE = {startAddress, endAddress, pageNo};
      memoryRanges.push_back(memoryRangeE);
   }

   //! Check if an address is within this memory region
   //! @param address - address to check
   //!
   //! @return true/false result
   //!
   bool contains(uint32_t address) {
      return findMemoryRangeIndex(address) >= 0;
   }

   //! Find the last contiguous address relative to the address
   //!
   //! @param address        - start address to check
   //! @param lastContinuous - the end address of the largest contiguous memory range including address
   //!
   //! @return true  = start address is within memory
   //!         false = start address is not within memory
   //!
   bool findLastContiguous(uint32_t address, uint32_t *lastContinuous, MemorySpace_t memorySpace = MS_None) {
      if (!isCompatibleType(memorySpace)) {
         return false;
      }
      int index = findMemoryRangeIndex(address);
      if (index<0) {
         // Start address not within region
         return false;
      }
      *lastContinuous = memoryRanges[index].end;
      return true;
   }

   //! Get page number for address
   //! @param address - address to check
   //!
   //! @return MemoryRegion::NoPageNo if not paged/within memory
   //!
   uint16_t getPageNo(uint32_t address) {
      if (!contains(address))
         return MemoryRegion::NoPageNo;
      return memoryRanges[lastIndexUsed].pageNo;
   }

   //! Obtain string describing the memory type
   //!
   //! @param type - Memory type
   //!
   //! @return - ptr to static string describing type
   //!
   static const char *getMemoryTypeName(MemType_t type);

   //! Indicates if a programmable type e.g Flash, eeprom etc.
   //!
   //! @param type - Memory type
   //!
   //! @return - true/false result
   //!
   static bool isProgrammableMemory(MemType_t memoryType);

   //! Indicates if the memory region is of a programmable type e.g Flash, eeprom etc.
   //!
   //! @return - true/false result
   //!
   bool isProgrammableMemory() {
      return isProgrammableMemory(type);
   }
   //! Get name of memory type
   //!
   const char *getMemoryTypeName(void) const { return getMemoryTypeName(type); }

   //! Get security address (Flash location)
   //!
   uint32_t getSecurityAddress() const { return securityAddress; }

   // Only available on Flash & EEPROM
   uint32_t    getFCLKDIVAddress() const { return getRegisterAddress() + 0x0; }
   uint32_t    getFSECAddress()    const { return getRegisterAddress() + 0x1; }
   uint32_t    getFTSTMODAddress() const { return getRegisterAddress() + 0x1; }
   uint32_t    getFCNFGAddress()   const { return getRegisterAddress() + 0x3;}
   uint32_t    getFPROTAddress()   const { return getRegisterAddress() + 0x4; }
   uint32_t    getFSTATAddress()   const { return getRegisterAddress() + 0x5;}
   uint32_t    getFCMDAddress()    const { return getRegisterAddress() + 0x6;}
   uint32_t    getFADDRAddress()   const { return getRegisterAddress() + 0x8;}
   uint32_t    getFDATAAddress()   const { return getRegisterAddress() + 0xA;}
   uint32_t    getDummyAddress()   const { return memoryRanges[0].end & 0xFFFFFFF0; }
   uint32_t    getSectorSize()     const { return sectorSize; }
   AddressType getAddressType()    const { return addressType; }
#if TARGET == RS08
   uint32_t    getFOPTAddress()    const { return getRegisterAddress();}
   uint32_t    getFLCRAddress()    const { return getRegisterAddress() + 1;}
#endif

   friend std::ostream & operator <<(std::ostream & s, const MemoryRegion mr);
   bool valid() const { return type != MemInvalid; }
   void setSecurityAddress(uint32_t address) { securityAddress = address; }

   MemType_t getMemoryType() const { return type; }
   static bool isCompatibleType(MemType_t memType, MemorySpace_t memorySpace);
   bool isCompatibleType(MemorySpace_t memorySpace) {
      return isCompatibleType(type, memorySpace);
   }
   const char *getMemoryTypeName() {return getMemoryTypeName(type);}
   uint32_t  getPageAddress() const { return pageAddress; }
   uint32_t  getRegisterAddress() const {
      if (registerAddress == 0) {
         throw MyException("Register address not defined for memory region");
      }
      return registerAddress;
   }
   const MemoryRange *getMemoryRange(unsigned index) const {
      if (index >= memoryRanges.size())
         return NULL;
      return &memoryRanges[index];
   }
   const MemoryRange *getMemoryRangeFor(uint32_t address) {
      int index = findMemoryRangeIndex(address);
      if (index<0)
         return NULL;
      return &memoryRanges[index];
   }
   const FlashProgramPtr &getFlashprogram() const { return flashProgram; }
   const SecurityInfoPtr &getSecureInfo()   const { return secureInfo; }
   const SecurityInfoPtr &getUnsecureInfo() const { return unsecureInfo; }

   void setFlashProgram(FlashProgramPtr program) { flashProgram = program; }
   void setAddressType(AddressType type)         { addressType = type; }

   void setSecureInfo(SecurityInfoPtr info) {
      if (info->getMode() != true) {
         throw MyException("MemoryRegion::setSecureInfo() - info has wrong type");
      }
      secureInfo = info;
   }
   void setUnsecureInfo(SecurityInfoPtr info) {
      if (info->getMode() != false) {
         throw MyException("MemoryRegion::setSecureInfo() - info has wrong type");
      }
      unsecureInfo = info;
   }
};

typedef std::tr1::shared_ptr<MemoryRegion> MemoryRegionPtr;

//! Information required to program a target
//!
class DeviceData {
public:
   //! How to handle erasing of flash before programming
   typedef enum  {
      eraseNone,        //! Don't erase
      eraseMass,        //! Mass erase / unsecure
      eraseAll,         //! Erase all flash arrays
      eraseSelective,   //! Erase flash block selectively
   } EraseOptions;
   static const char *getEraseOptionName(EraseOptions option) {
      switch (option) {
      case eraseNone      : return "EraseNone";
      case eraseMass      : return "EraseMass";
      case eraseAll       : return "EraseAll";
      case eraseSelective : return "EraseSelective";
      default :             return "Illegal erase option";
      }
   }
   typedef struct {
      uint8_t eeepromSize;
      uint8_t partionValue;
   } FlexNVMParameters;

private:
   std::string          targetName;             //!< Name of target
   uint32_t             ramStart;               //!< Start of internal RAM
   uint32_t             ramEnd;                 //!< End of internal RAM
   ClockTypes_t         clockType;              //!< Type of clock
   uint32_t             clockAddress;           //!< Address of Clock register block
   uint32_t             clockTrimNVAddress;     //!< Address of Non-volatile storage of trim value
   unsigned long int    clockTrimFreq;          //!< Trim frequency in Hz of the _internal_ clock e.g. 32.7 kHz (0 => no trim required)
   bool                 connectionFreqGiven;    //!< Use connectionFreq if needed
   unsigned long int    connectionFreq;         //!< BDM connection frequency in Hz
   uint32_t             COPCTLAddress;          //!< Address of COPCTL register
   uint32_t             SOPTAddress;            //!< Address of SOPT1 register
   uint32_t             SDIDAddress;            //!< Address of SDID registers
   SecurityOptions_t    security;               //!< Determines security options of programmed target (modifies NVFOPT value)
   EraseOptions         eraseOption;            //!< How to handle erasing of flash before programming
   uint16_t             clockTrimValue;         //!< Clock trim value calculated for a particular device
   unsigned int         targetSDIDCount;        //!< Count of valid SDIDs intargetSDID[]
   uint16_t             targetSDID[10];         //!< System Device Identification Register value (0=> don't know/care)
   uint16_t             targetSDIDMask;         //!< Mask for valid bits in SDID
//   uint32_t             securityAddress;        //!< Address of Security area in Flash
   unsigned             memoryRegionCount;
   MemoryRegionPtr      memoryRegions[10];      //!< Different memory regions e.g. EEPROM, RAM etc.
   MemoryRegionPtr      lastMemoryRegionUsed;
   TclScriptPtr         flashScripts;
   FlashProgramPtr      flashProgram;
   FlexNVMParameters    flexNVMParameters;
   FlexNVMInfoPtr       flexNVMInfo;

public:
   bool                 valid;
   static const DeviceData    defaultDevice;
   static const unsigned int  BDMtoBUSFactor = 1;   //!< Factor relating measured BDM frequency to Target BUS frequency\n
                                                    //!< busFrequency = connectionFreq * BDMtoBUSFactor
public:
   const std::string getTargetName()              const { return targetName; };
   uint32_t          getRamStart()                const { return ramStart; };
   uint32_t          getRamEnd()                  const { return ramEnd; };
   ClockTypes_t      getClockType()               const { return clockType; };
   uint32_t          getClockAddress()            const { return clockAddress; };
   uint32_t          getClockTrimNVAddress()      const { return clockTrimNVAddress; };
   uint16_t          getClockTrimValue()          const { return clockTrimValue; };
   unsigned long     getClockTrimFreq() /*Hz*/    const { return clockTrimFreq; };
   unsigned long     getConnectionFreq() /*Hz*/   const { return connectionFreq; };
   SecurityOptions_t getSecurity()                const { return security; };
   EraseOptions      getEraseOption()             const { return eraseOption; };
#if (TARGET == HC12)||(TARGET == MC56F80xx)
   uint32_t          getCOPCTLAddress()           const { return COPCTLAddress; };
#else
   uint32_t          getSOPTAddress()             const { return SOPTAddress; };
#endif

   uint32_t          getSDIDAddress()             const { return SDIDAddress; };
   uint16_t          getSDIDMask()                const { return targetSDIDMask; }

   unsigned int      getBDMtoBUSFactor()          const { return BDMtoBUSFactor; }

   FlashProgramPtr   getFlashProgram()            const { return flashProgram; }
   TclScriptPtr      getFlashScripts()            const { return flashScripts; }
   FlexNVMInfoPtr    getflexNVMInfo()             { return flexNVMInfo; }

   MemoryRegionPtr getMemoryRegion(unsigned index) const {
      if (index >= memoryRegionCount)
         return MemoryRegionPtr();
      return memoryRegions[index];
   }

   MemoryRegionPtr getMemoryRegionFor(uint32_t address, MemorySpace_t memorySpace=MS_None);

   uint16_t getSDID(unsigned index=0) const {
      if (index >= targetSDIDCount)
         return 0;
      return targetSDID[index];
   }
   static uint32_t   getDefaultClockTrimFreq(ClockTypes_t clockType);
   static uint32_t   getDefaultClockTrimNVAddress(ClockTypes_t clockType);
   static uint32_t   getDefaultClockAddress(ClockTypes_t clockType);
   uint32_t          getDefaultClockTrimFreq() const;
   uint32_t          getDefaultClockTrimNVAddress() const;
   uint32_t          getDefaultClockAddress() const;

   bool              isThisDevice(std::map<uint32_t,uint32_t> targetSDIDs) const ;
   bool              isThisDevice(uint32_t targetSDID) const ;

   MemType_t         getMemoryType(uint32_t address, MemorySpace_t memorySpace=MS_None);
   uint16_t          getPageNo(uint32_t address);

   void  addSDID(uint16_t newSDID) {
      if (targetSDIDCount>= sizeof(targetSDID)/sizeof(targetSDID[0]))
         throw MyException("DeviceData::addSDID() - Too many SDIDs");
      targetSDID[targetSDIDCount++] = newSDID;
   }
   void addMemoryRegion(MemoryRegionPtr pMemoryRegion) {
      if (memoryRegionCount >= (sizeof(memoryRegions)/sizeof(memoryRegions[0])))
         throw MyException("DeviceData::addMemoryRegion() - Too many memory regions");
      memoryRegions[memoryRegionCount++] = pMemoryRegion;
      if (((pMemoryRegion->getMemoryType() == MemRAM)||
           (pMemoryRegion->getMemoryType() == MemXRAM)) && (ramStart == 0)) {
         const MemoryRegion::MemoryRange *mr = pMemoryRegion->getMemoryRange(0);
//         print("%s[0x%X...0x%X]\n",
//               MemoryRegion::getMemoryTypeName(pMemoryRegion->getMemoryType()),
//               mr->start,
//               mr->end);
         ramStart = mr->start;
         ramEnd   = mr->end;
      }
//      cerr << *pMemoryRegion << endl;
   }

   void setTargetName(const std::string &value)       { targetName = value; };
   void setRamStart(uint32_t value)                   { ramStart = value; };
   void setRamEnd(uint32_t value)                     { ramEnd = value; };
   void setClockType(ClockTypes_t value)              { clockType = value; };
   void setClockAddress(uint32_t value)               { clockAddress = value; };
   void setClockTrimNVAddress(uint32_t value)         { clockTrimNVAddress = value; };
   void setClockTrimValue(uint16_t value)             { clockTrimValue = value; };
   void setClockTrimFreq(unsigned long value) /*Hz*/  { clockTrimFreq = value; };
   void setConnectionFreq(unsigned long value /*Hz*/) { connectionFreq = value; };
   void setSecurity(SecurityOptions_t value)          { security = value; };
   void setEraseOption(EraseOptions value)            { eraseOption = value; };
#if (TARGET == HC12)||(TARGET == MC56F80xx)
   void setCOPCTLAddress(uint32_t value)              { COPCTLAddress = value; };
#else
   void setSOPTAddress(uint32_t value)                { SOPTAddress = value; };
#endif
   void setSDIDAddress(uint32_t value)                { SDIDAddress = value; };
   void setSDIDMask(uint16_t value)                   { targetSDIDMask = value; };
//   void setSecurityAddress(uint32_t value)            { securityAddress = value; };
   void setFlashScripts(TclScriptPtr script)          { flashScripts = script; }
   void setFlashProgram(FlashProgramPtr program)      { flashProgram = program; }
   void setflexNVMInfo(FlexNVMInfoPtr info)           { flexNVMInfo = info; }
   void setFlexNVMParameters(const FlexNVMParameters *parameters) {
      flexNVMParameters = *parameters;
   }
   const FlexNVMParameters *getFlexNVMParameters() {
      return &flexNVMParameters;
   }

//#if (TARGET == HC12)
//   void setFSECAddress(uint32_t value)                     { FSECAddress = value; }
//#elif (TARGET == HCS08) || (TARGET == RS08)
//   void setFOPTAddress(uint32_t value)                     { FOPTAddress = value; }
//#endif

   DeviceData( const std::string    &targetName,
               uint32_t             ramStart,
               uint32_t             ramEnd,
               ClockTypes_t         clockType,
               uint32_t             clockAddress,
               uint32_t             clockTrimNVAddress,
               unsigned long int    clockTrimFreq,
               uint32_t             flashStart              = 0,
               uint32_t             flashEnd                = 0,
               uint32_t             COPCTLAddress           = 0x003C,
               uint32_t             SOPTAddress             = 0,
               uint32_t             SDIDAddress             = 0,
               SecurityOptions_t    security                = SEC_DEFAULT,
               bool                 noMassErase             = false,
               bool                 connectFrequencyGiven   = false,
               unsigned int         connectFrequency        = 8000,
               uint16_t             clockTrimValue          = 0
                  ) : targetName(targetName),
                      ramStart(ramStart),
                      ramEnd(ramEnd),
                      clockType(clockType),
                      clockAddress(clockAddress),
                      clockTrimNVAddress(clockTrimNVAddress),
                      clockTrimFreq(clockTrimFreq),
                      connectionFreqGiven(false),
                      connectionFreq(0),
                      COPCTLAddress(COPCTLAddress),
                      SOPTAddress(SOPTAddress),
                      SDIDAddress(SDIDAddress),
                      security(security),
                      eraseOption(eraseAll),
                      clockTrimValue(clockTrimValue),
                      targetSDIDCount(0),
                      targetSDIDMask(0),
                      memoryRegionCount(0),
                      valid(true)
                      {
//      print("DeviceData::DeviceData()\n");
   };
   DeviceData() : targetName(""),
                  ramStart(0),
                  ramEnd(0),
                  clockType(CLKINVALID),
                  clockAddress(0),
                  clockTrimNVAddress(0),
                  clockTrimFreq(0),
                  connectionFreqGiven(false),
                  connectionFreq(0),
                  COPCTLAddress(0),
                  SOPTAddress(0),
                  SDIDAddress(0),
                  security(SEC_DEFAULT),
                  eraseOption(eraseAll),
                  clockTrimValue(0),
                  targetSDIDCount(0),
                  targetSDIDMask(0),
                  memoryRegionCount(0),
                  valid(true)
                  {
//      print("DeviceData::DeviceData() - default\n");
   }
   ~DeviceData();
};

typedef std::tr1::shared_ptr<DeviceData> DeviceDataPtr;
typedef std::tr1::shared_ptr<const DeviceData> ConstDeviceDataPtr;

//! Information required to program a Device
//!
class DeviceDataBase {

private:
   std::vector<DeviceData *>  deviceData;                   // List of devices
   std::map<const std::string, SharedInformationItemPtr> sharedInformation;   // Shared information that may be referenced by devices
   static const DeviceData  *defaultDevice;
   DeviceDataBase (DeviceDataBase &);
   DeviceDataBase &operator=(DeviceDataBase &);

public:
   bool   loadDeviceData();

   const DeviceData *findDeviceFromName(const std::string &targetName);
   int findDeviceIndexFromName(const std::string &targetName);
   const DeviceData &operator[](unsigned index) {
      if (index > deviceData.size())
         throw MyException("DeviceDataBase::operator[] - illegal index");
      return *deviceData[index];
   };
   std::vector<DeviceData *>::iterator begin() {
      return deviceData.begin();
   }
   std::vector<DeviceData *>::iterator end() {
      return deviceData.end();
   }
   void   listDevices();
   static const DeviceData *getDefaultDevice() { return defaultDevice; }
   static const DeviceData *setDefaultDevice(const DeviceData *defaultDevice) {
      DeviceDataBase::defaultDevice = defaultDevice;
      return defaultDevice;
   }
   unsigned getNumDevice() const { return this->deviceData.size(); }

   DeviceData *addDevice(DeviceData *device) {
      std::vector<DeviceData*>::iterator itDev = deviceData.insert(deviceData.end(),device);
      return *itDev;
   }
   void addSharedData(std::string key, SharedInformationItem *sharedData) {
      SharedInformationItemPtr psharedData(sharedData);
      sharedInformation.insert(std::pair<const std::string, SharedInformationItemPtr>(key, psharedData));
   }
   SharedInformationItemPtr getSharedData(std::string key) {
      std::map<const std::string, SharedInformationItemPtr>::iterator it = sharedInformation.find(key);
      if (it == sharedInformation.end())
         return SharedInformationItemPtr();
      else
         return it->second;
   }
   DeviceDataBase() {
//      print("DeviceDataBase::DeviceDataBase()\n");
   };
   ~DeviceDataBase();
};

#endif /* DEVICEDATA_HPP_ */
