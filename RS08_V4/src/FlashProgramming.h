#ifndef _FLASHPROGRAMMING_H_
#define _FLASHPROGRAMMING_H_

#include "Common.h"
#include "DeviceData.h"
#include "FlashImage.h"
#include "USBDM_API.h"
#include "usbdmTcl.h"

class ProgressTimer;

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

   //! Structure for MCGCG parameters
   struct MCG_ClockParameters_t {
      uint8_t mcgC1;
      uint8_t mcgC2;
      uint8_t mcgC3;
      uint8_t mcgTrim;
      uint8_t mcgSC;
      uint8_t mcgCT;
   } ;

   //! Structure for ICGCG parameters
   struct ICG_ClockParameters_t {
      uint8_t  icgC1;        //!< ICSC1 value
      uint8_t  icgC2;        //!< ICSC1 value
      uint16_t icgFilter;    //!< Not used
      uint8_t  icgTrim;      //!< Trim value
   } ;

   //! Structure for ICSCG parameters
   struct ICS_ClockParameters_t {
      uint8_t icsC1;      //!< ICSC1 value
      uint8_t icsC2;      //!< ICSC2 value
      uint8_t icsTrim;    //!< ICSTRM value
      uint8_t icsSC;      //!< ICSSC value (FTRIM)
   } ;

   union ClockParameters {
      ICG_ClockParameters_t icg;
      MCG_ClockParameters_t mcg;
      ICS_ClockParameters_t ics;
   } ;

   typedef USBDM_ErrorCode (*CallBackT)(USBDM_ErrorCode status, int percent, const char *message);

   DeviceData              parameters;       //!< Parameters describing the target device
   Tcl_Interp              *tclInterpreter;  //!< TCL interpreter
   bool                    useTCLScript;
   bool                    flashReady;       //!< Safety check - only TRUE when flash is ready for programming
   bool                    initTargetDone;   //! Indicates initTarget() has been done.
   uint32_t                targetBusFrequency;  //! kHz
   FlashProgramPtr         currentFlashProgram;
   ProgressTimer          *progressTimer;
   bool                    doRamWrites;
   MemoryRegionPtr         flashMemoryRegionPtr;

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
   USBDM_ErrorCode trimICS_Clock(ICS_ClockParameters_t *clockParameters);
   USBDM_ErrorCode trimICG_Clock(ICG_ClockParameters_t *clockParameters);
   USBDM_ErrorCode trimMCG_Clock(MCG_ClockParameters_t *clockParameters);
   USBDM_ErrorCode setFlashTrimValues(FlashImage *flashImageDescription);

   USBDM_ErrorCode configureICS_Clock(unsigned long         *busFrequency,
                                      ICS_ClockParameters_t *clockParameters);
   USBDM_ErrorCode configureICG_Clock(unsigned long         *busFrequency,
                                      ICG_ClockParameters_t *clockParameters);
   USBDM_ErrorCode configureMCG_Clock(unsigned long         *busFrequency,
                                      MCG_ClockParameters_t *clockParameters);
   USBDM_ErrorCode configureTargetClock(unsigned long  *busFrequency);
   USBDM_ErrorCode configureExternal_Clock(unsigned long  *busFrequency);
   USBDM_ErrorCode blankCheckTarget();
   USBDM_ErrorCode blindUnsecure();
   USBDM_ErrorCode doFlashCommand(uint8_t command);
   USBDM_ErrorCode calculateFlashDelay(uint8_t *delayValue);
   USBDM_ErrorCode writeFlashBlock( unsigned int        byteCount,
                                 unsigned int        address,
                                 unsigned const char *data,
                                 uint8_t                  delayValue);
   USBDM_ErrorCode programBlock(FlashImage    *flashImageDescription,
                                unsigned int   blockSize,
                                uint32_t       flashAddress);
   USBDM_ErrorCode doVerify(FlashImage *flashImageDescription);
   USBDM_ErrorCode blankCheckBlock(FlashImage   *flashImageDescription,
                                unsigned int  blockSize,
                                unsigned int  flashAddress);
   void RS08_doFixups(uint8_t buffer[]);
   USBDM_ErrorCode loadAndStartExecutingTargetProgram();
   USBDM_ErrorCode setPPAGE(uint32_t address);
   USBDM_ErrorCode dummyTrimLocations(FlashImage *flashImageDescription);
   USBDM_ErrorCode bulkEraseMemoryRegion(MemoryRegionPtr memoryRegion);

public:
   USBDM_ErrorCode initTCL(void);
   USBDM_ErrorCode releaseTCL(void);
   USBDM_ErrorCode setDeviceData(const DeviceData  &theParameters);
   USBDM_ErrorCode checkTargetUnSecured();
   USBDM_ErrorCode runTCLScript(TclScriptPtr script);
   USBDM_ErrorCode runTCLCommand(const char *command);
   USBDM_ErrorCode massEraseTarget();
   USBDM_ErrorCode programFlash(FlashImage *flashImageDescription, CallBackT errorCallBack=NULL, bool doRamWrites=false);
   USBDM_ErrorCode verifyFlash(FlashImage  *flashImageDescription, CallBackT errorCallBack=NULL);
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
