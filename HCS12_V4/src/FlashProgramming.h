#ifndef _FLASHPROGRAMMING_H_
#define _FLASHPROGRAMMING_H_

#include "Common.h"
#include "DeviceData.h"
#include "FlashImage.h"
#include "USBDM_API.h"
#include "usbdmTcl.h"

class ProgressTimer;

#pragma pack(1)
//! Header at the start of flash programming code (describes flash code)
struct LargeTargetImageHeader {
   uint16_t         loadAddress;       // Address where to load this image
   uint16_t         entry;             // Ptr to entry routine (for currently loaded routine)
   uint16_t         capabilities;      // Capabilities of routine
   uint16_t         copctlAddress;     // Address of COPCTL register
   uint32_t         calibFactor;       // Calibration factor for speed determination
   uint16_t         flashData;         // Ptr to information about operation
};

//! Header at the start of timing data (controls program action & holds result)
struct LargeTargetTimingDataHeader {
   uint16_t         flags;             // Controls actions of routine
   uint16_t         errorCode;         // Error code from action
   uint16_t         controller;        // Ptr to flash controller (unused)
   uint32_t         timingCount;       // Timing count
};

//! Header at the start of flash programming buffer (controls program action)
struct LargeTargetFlashDataHeader {
   uint16_t         flags;             // Controls actions of routine
   uint16_t         errorCode;         // Error code from action
   uint16_t         controller;        // Ptr to flash controller
   uint16_t         frequency;         // Target frequency (kHz)
   uint16_t         sectorSize;        // Size of Flash memory sectors (smallest erasable block)
   uint32_t         address;           // Memory address being accessed (reserved/page/address)
   uint16_t         dataSize;          // Size of memory range being accessed
   uint16_t         dataAddress;       // Address of RAM data buffer
};
//! Holds program execution result
struct ResultStruct {
   uint16_t          flags;                  // Incomplete actions
   uint16_t          errorCode;              // Error code from operation
   uint8_t           reserved[6];
};
#pragma pack()

//! Describes the flash programming code (created from loaded flash routines)
struct TargetProgramInfo {
   uint32_t         entry;                   //!< Address of entry routine (for currently loaded routine)
   uint32_t         headerAddress;           //!< Address where to load data image (including header)
   uint32_t         dataOffset;              //!< Offset to data buffer within image
   uint32_t         maxDataSize;             //!< Maximum data buffer size
   uint32_t         capabilities;            // Capabilities of routine
   uint16_t         calibFrequency;          // Frequency (kHz) used for calibFactor
   uint32_t         calibFactor;             // Calibration factor for speed determination
   bool             smallProgram;            // Indicates small version of flash code being used
   bool             usePagedAddresses;       // Set up paged addressing information
   uint32_t         programOperation;        // either DO_PROGRAM_RANGE or DO_BLANK_CHECK_RANGE|DO_PROGRAM_RANGE|DO_VERIFY_RANGE
};
enum FlashOperation {OpNone, OpSelectiveErase, OpBlockErase, OpBlankCheck, OpProgram, OpVerify, OpWriteRam, OpPartitionFlexNVM, OpTiming};

//! Information for the flash operation to be done
struct FlashOperationInfo {
   uint8_t          operation;               // Controls actions of routine
   uint32_t         controller;              // Address of flash controller
   uint32_t         pageAddress;             // Address of PPAGE or EPAGE register
   uint32_t         flashAddress;            // Memory flashAddress being programmed/erased (may include pageNo etc)
   uint32_t         dataSize;                // Size of data to program/verify
   uint16_t         sectorSize;              // Sector size
   uint32_t         targetBusFrequency;      // Measured target bus frequency (kHz)
   uint32_t         alignment;               // Flash programming alignment (1,2,4,8,16,32 bytes)
   uint16_t         flexNVMPartition;        // Value for partitioning FlexNVM
};
class FlashProgrammer {

private:
   // Error codes return from the flash driver
   enum FlashDriverError_t {
        FLASH_ERR_OK                = (0),
        FLASH_ERR_LOCKED            = (1),  // Flash is still locked
        FLASH_ERR_ILLEGAL_PARAMS    = (2),  // Parameters illegal
        FLASH_ERR_PROG_FAILED       = (3),  // STM - Programming operation failed - general
        FLASH_ERR_PROG_WPROT        = (4),  // STM - Programming operation failed - write protected
        FLASH_ERR_VERIFY_FAILED     = (5),  // Verify failed
        FLASH_ERR_ERASE_FAILED      = (6),  // Erase or Blank Check failed
        FLASH_ERR_TRAP              = (7),  // Program trapped (illegal instruction/location etc.)
        FLASH_ERR_PROG_ACCERR       = (8),  // Kinetis/CFVx - Programming operation failed - ACCERR
        FLASH_ERR_PROG_FPVIOL       = (9),  // Kinetis/CFVx - Programming operation failed - FPVIOL
        FLASH_ERR_PROG_MGSTAT0      = (10), // Kinetis - Programming operation failed - MGSTAT0
        FLASH_ERR_CLKDIV            = (11), // CFVx - Clock divider not set
        FLASH_ERR_ILLEGAL_SECURITY  = (12), // Kinetis - Illegal value for security location
        FLASH_ERR_UNKNOWN           = (13), // Unspecified error
        FLASH_ERR_TIMEOUT           = (14), // Timeout waiting for completion
   };

   enum AddressModifiers {
      ADDRESS_LINEAR = 1UL<<31,
      ADDRESS_EEPROM = 1UL<<30,
   };

   typedef USBDM_ErrorCode (*CallBackT)(USBDM_ErrorCode status, int percent, const char *message);

   DeviceData              parameters;               //!< Parameters describing the target device
   Tcl_Interp              *tclInterpreter;          //!< TCL interpreter
   bool                    useTCLScript;
   bool                    flashReady;               //!< Safety check - only TRUE when flash is ready for programming
   bool                    initTargetDone;           //!< Indicates initTarget() has been done.
   TargetProgramInfo       targetProgramInfo;        //!< Describes loaded flash code
   FlashOperationInfo      flashOperationInfo;       //!< Describes flash operation

   FlashProgramPtr         currentFlashProgram;
   FlashOperation          currentFlashOperation;
   uint32_t                currentFlashAlignment;
   ProgressTimer          *progressTimer;
   bool                    doRamWrites;

   USBDM_ErrorCode initialiseTargetFlash();
   USBDM_ErrorCode initialiseTarget();
   USBDM_ErrorCode setFlashSecurity(FlashImage      &flashImage,
                                    MemoryRegionPtr flashRegion);
   USBDM_ErrorCode setFlashSecurity(FlashImage  &flashImage);
   USBDM_ErrorCode trimTargetClock(uint32_t       trimAddress,
                                   unsigned long  targetBusFrequency,
                                   uint16_t      *returnTrimValue,
                                   unsigned long *measuredBusFrequency,
                                   int            do9BitTrim);
   USBDM_ErrorCode configureExternal_Clock(unsigned long  *busFrequency);
   USBDM_ErrorCode eraseFlash(void);
   USBDM_ErrorCode convertTargetErrorCode(FlashDriverError_t rc);
   USBDM_ErrorCode initSmallTargetBuffer(memoryElementType *buffer);
   USBDM_ErrorCode initLargeTargetBuffer(memoryElementType *buffer);
   USBDM_ErrorCode executeTargetProgram(memoryElementType *buffer=0, uint32_t size=0);
   USBDM_ErrorCode checkUnsupportedTarget();
   USBDM_ErrorCode determineTargetSpeed(void);
   USBDM_ErrorCode doFlashBlock(FlashImage    *flashImage,
                                unsigned int   blockSize,
                                uint32_t      &flashAddress,
                                FlashOperation flashOperation);
   USBDM_ErrorCode selectiveEraseFlashSecurity(void);
   USBDM_ErrorCode doTargetVerify(FlashImage *flashImage);
   USBDM_ErrorCode doReadbackVerify(FlashImage *flashImage);
   USBDM_ErrorCode applyFlashOperation(FlashImage *flashImage, FlashOperation flashOperation);
   USBDM_ErrorCode doVerify(FlashImage *flashImage);
   USBDM_ErrorCode doSelectiveErase(FlashImage  *flashImage);
   USBDM_ErrorCode doProgram(FlashImage  *flashImage);
   USBDM_ErrorCode doBlankCheck(FlashImage *flashImage);
   USBDM_ErrorCode doWriteRam(FlashImage *flashImage);
   USBDM_ErrorCode loadTargetProgram(FlashOperation flashOperation);
   USBDM_ErrorCode loadTargetProgram(FlashProgramPtr flashProgram, FlashOperation flashOperation);
   USBDM_ErrorCode loadSmallTargetProgram(memoryElementType *buffer,
                                          uint32_t           loadAddress,
                                          uint32_t           size,
                                          FlashProgramPtr    flashProgram,
                                          FlashOperation     flashOperation);
   USBDM_ErrorCode loadLargeTargetProgram(memoryElementType *buffer,
                                          uint32_t           loadAddress,
                                          uint32_t           size,
                                          FlashProgramPtr    flashProgram,
                                          FlashOperation     flashOperation);
   USBDM_ErrorCode probeMemory(MemorySpace_t memorySpace, uint32_t address);
   USBDM_ErrorCode dummyTrimLocations(FlashImage *flashImage);
   USBDM_ErrorCode getPageAddress(MemoryRegionPtr memoryRegionPtr, uint32_t address, uint8_t *pageNo);
   USBDM_ErrorCode setPageRegisters(uint32_t physicalAddress);
   USBDM_ErrorCode partitionFlexNVM(void);

public:
   USBDM_ErrorCode initTCL(void);
   USBDM_ErrorCode releaseTCL(void);
   USBDM_ErrorCode setDeviceData(const DeviceData  &theParameters);
   USBDM_ErrorCode checkTargetUnSecured();
   USBDM_ErrorCode runTCLScript(TclScriptPtr script);
   USBDM_ErrorCode runTCLCommand(const char *command);
   USBDM_ErrorCode massEraseTarget();
   USBDM_ErrorCode getTargetBusSpeed(unsigned long *busFrequency);
   USBDM_ErrorCode programFlash(FlashImage *flashImage, CallBackT errorCallBack=NULL, bool doRamWrites=false);
   USBDM_ErrorCode verifyFlash(FlashImage  *flashImage, CallBackT errorCallBack=NULL);
   USBDM_ErrorCode readTargetChipId(uint32_t *targetSDID, bool doInit=false);
   USBDM_ErrorCode confirmSDID(void);
   
   USBDM_ErrorCode getCalculatedTrimValue(uint16_t &value) {
      value = parameters.getClockTrimValue();
      return PROGRAMMING_RC_OK;
   }
   USBDM_ErrorCode resetAndConnectTarget(void);
   FlashProgrammer() :
      tclInterpreter(NULL),
      useTCLScript(true),
      flashReady(false),
      initTargetDone(false),
      currentFlashOperation(OpNone),
      currentFlashAlignment(0),
      progressTimer(NULL),
      doRamWrites(false) {
//      print("FlashProgrammer()\n");
   }
   ~FlashProgrammer();
};
#endif // _FLASHPROGRAMMING_H_
