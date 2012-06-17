#ifndef _FLASHPROGRAMMING_H_
#define _FLASHPROGRAMMING_H_

#include "Common.h"
#include "DeviceData.h"
#include "FlashImage.h"
#include "USBDM_API.h"
#include "usbdmTcl.h"

class ProgressTimer;

#pragma pack(1)

// Describes a block to be programmed & result
struct FlashData_t {
   uint16_t         flags;             // Controls actions of routine
   uint16_t         errorCode;         // Error code from action
   uint32_t         controller;        // Ptr to flash controller
   uint16_t         frequency;         // Target frequency (kHz)
   uint16_t         sectorSize;        // Size of Flash memory sectors (smallest erasable block)
   uint32_t         address;           // Memory address being accessed (reserved/page/address)
   uint16_t         size;              // Size of memory range being accessed
   uint16_t         pad;
   uint32_t         data;              // Ptr to data to program
} ;

// Timing information
struct TimingData_t {
   uint16_t         flags;             // Controls actions of routine
   uint16_t         errorCode;         // Error code from action
   uint32_t         controller;        // Ptr to flash controller (unused)
   uint32_t         timingCount;       // Timing count
};

//! Describe the flash programming code
struct FlashProgramHeader_t {
   uint32_t         loadAddress;       // Address where to load this image
   uint32_t         entry;             // Ptr to entry routine
   uint16_t         capabilities;      // Capabilities of routine
   uint16_t         calibFrequency;    // Frequency (kHz) used for calibFactor
   uint32_t         calibFactor;       // Calibration factor for speed determination
   uint32_t         flashData;         // Ptr to information about operation
} ;
#pragma pack()

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

   typedef USBDM_ErrorCode (*CallBackT)(USBDM_ErrorCode status, int percent, const char *message);

   DeviceData              parameters;       //!< Parameters describing the target device
   Tcl_Interp              *tclInterpreter;  //!< TCL interpreter
   bool                    useTCLScript;
   bool                    flashReady;       //!< Safety check - only TRUE when flash is ready for programming
   bool                    initTargetDone;   //! Indicates initTarget() has been done.
   FlashProgramHeader_t    flashProgramHeader;
   FlashData_t             flashData;
   uint32_t                targetBusFrequency;  //! kHz
   FlashProgramPtr         currentFlashProgram;
   ProgressTimer          *progressTimer;
   bool                    doRamWrites;

   USBDM_ErrorCode initialiseTargetFlash();
   USBDM_ErrorCode initialiseTarget();
   USBDM_ErrorCode setFlashSecurity(FlashImage      &flashImageDescription,
                                 MemoryRegionPtr flashRegion);
   USBDM_ErrorCode setFlashSecurity(FlashImage  &flashImageDescription);
   USBDM_ErrorCode trimTargetClock(uint32_t           trimAddress,
                                   unsigned long targetBusFrequency,
                                   uint16_t           *returnTrimValue,
                                   unsigned long *measuredBusFrequency,
                                   int           do9BitTrim);
   USBDM_ErrorCode configureExternal_Clock(unsigned long  *busFrequency);
   USBDM_ErrorCode eraseFlash(void);
   USBDM_ErrorCode convertTargetErrorCode(FlashDriverError_t rc);
   USBDM_ErrorCode executeTargetProgram(FlashData_t *flashProgramData, uint32_t size);
   USBDM_ErrorCode determineTargetSpeed(void);
   USBDM_ErrorCode doFlashBlock(FlashImage    *flashImageDescription,
                                unsigned int   blockSize,
                                uint32_t      &flashAddress,
                                uint32_t       flashoperation);
   USBDM_ErrorCode applyFlashOperation(FlashImage *flashImageDescription, uint32_t operation);
   USBDM_ErrorCode doVerify(FlashImage *flashImageDescription);
   USBDM_ErrorCode doSelectiveErase(FlashImage  *flashImageDescription);
   USBDM_ErrorCode doProgram(FlashImage  *flashImageDescription);
   USBDM_ErrorCode doBlankCheck(FlashImage *flashImageDescription);
   USBDM_ErrorCode doWriteRam(FlashImage *flashImage);
   USBDM_ErrorCode loadTargetProgram();
   USBDM_ErrorCode loadTargetProgram(FlashProgramPtr flashProgram);
   USBDM_ErrorCode probeMemory(MemorySpace_t memorySpace, uint32_t address);

public:
   USBDM_ErrorCode initTCL(void);
   USBDM_ErrorCode releaseTCL(void);
   USBDM_ErrorCode setDeviceData(const DeviceData  &theParameters);
   USBDM_ErrorCode checkTargetUnSecured();
   USBDM_ErrorCode runTCLScript(TclScriptPtr script);
   USBDM_ErrorCode runTCLCommand(const char *command);
   USBDM_ErrorCode massEraseTarget();
   USBDM_ErrorCode getTargetBusSpeed(unsigned long *busFrequency);
   USBDM_ErrorCode programFlash(FlashImage *flashImageDescription, CallBackT errorCallBack=NULL, bool doRamWrites=false);
   USBDM_ErrorCode verifyFlash(FlashImage  *flashImageDescription, CallBackT errorCallBack=NULL, bool doRamWrites=false);
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
      targetBusFrequency(0),
      progressTimer(NULL),
      doRamWrites(false) {
//      print("FlashProgrammer()\n");
   }
   ~FlashProgrammer();
};
#endif // _FLASHPROGRAMMING_H_
