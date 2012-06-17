/*!
 * \brief Header file for CmdTable.c
*/
#ifndef _NAMES_H_
#define _NAMES_H_

#include "USBDM_API.h"
#include "USBDM_AUX.h"
#ifndef LOG
// Dummy the routines if logging is not required
static inline char const *getCommandName(unsigned char command)        { return ""; } //!< Dummy
static inline char const *getDebugCommandName(unsigned char cmd)       { return ""; } //!< Dummy

static inline char const *getConnectionRetryName(RetryMode mode)       { return ""; } //!< Dummy
static inline char const *getAutoConnectName(AutoConnect_t mode)       { return ""; } //!< Dummy

static inline char const *getDFUCommandName(unsigned char command)     { return ""; } //!< Dummy

static inline char const *getRS08RegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getHCS08RegName( unsigned int regAddr )      { return ""; } //!< Dummy
static inline char const *getHCS12RegName( unsigned int regAddr )      { return ""; } //!< Dummy
static inline char const *getCFV1RegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getCFVxRegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getARMRegName( unsigned int regAddr )   { return ""; } //!< Dummy
static inline char const *getRegName( unsigned int targetType, unsigned int regAddr ) { return ""; } //!< Dummy
static inline char const *getHCS12DebugRegName( unsigned int regAddr ) { return ""; } //!< Dummy
static inline char const *getCFV1DebugRegName( unsigned int regAddr )  { return ""; } //!< Dummy
static inline char const *getCFVxDebugRegName( unsigned int regAddr )  { return ""; } //!< Dummy
static inline char const *getCFV1ControlRegName( unsigned int regAddr ){ return ""; } //!< Dummy
static inline char const *getCFVxControlRegName( unsigned int regAddr ){ return ""; } //!< Dummy

static inline char const *getRS08_BDCSCR_Name( unsigned int BDCSCR)    { return ""; } //!< Dummy
static inline char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR)   { return ""; } //!< Dummy
static inline char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS)   { return ""; } //!< Dummy
static inline char const *getCFV1_XCSR_Name( unsigned int XCSR)        { return ""; } //!< Dummy
static inline char const *getCFVx_CSR_Name( unsigned int CSR)          { return ""; } //!< Dummy
static inline char const *getStatusRegName(unsigned int targetType, unsigned int value) { return ""; } //!< Dummy

static inline char const *getCapabilityName(unsigned int vector)       { return ""; } //!< Dummy

static inline char const *getTargetModeName(TargetMode_t type)         { return ""; } //!< Dummy

static inline char const *getControlLevelName(InterfaceLevelMasks_t level) { return ""; } //!< Dummy

static inline char const *getClockSelectName(ClkSwValues_t level)       { return ""; }
static inline char const *getVoltageSelectName(TargetVddSelect_t level) { return ""; }
static inline char const *getVppSelectName(TargetVppSelect_t level)     { return ""; }
static inline char const *getPinLevelName(PinLevelMasks_t level)        { return ""; }
static inline char const *getExitAction(int action)                     { return ""; }
static inline void printBdmOptions(const USBDM_ExtendedOptions_t *options) { }
static inline char const *getMemSpaceName(MemorySpace_t memSpace)       { return ""; }

#else
char const *getCommandName(unsigned char command);
char const *getDebugCommandName(unsigned char cmd);

char const *getAutoConnectName(AutoConnect_t mode);

char const *getConnectionRetryName(RetryMode mode);

char const *getDFUCommandName(unsigned char command);

char const *getRS08RegName( unsigned int regAddr );
char const *getHCS08RegName( unsigned int regAddr );
char const *getHCS12RegName( unsigned int regAddr );
char const *getCFV1RegName( unsigned int regAddr );
char const *getCFVxRegName( unsigned int regAddr );
char const *getARMRegName( unsigned int regAddr );
char const *getRegName( unsigned int targetType, unsigned int regAddr );
char const *getHCS12DebugRegName( unsigned int regAddr );
char const *getCFV1DebugRegName( unsigned int regAddr );
char const *getCFVxDebugRegName( unsigned int regAddr );
char const *getCFV1ControlRegName( unsigned int regAddr );
char const *getCFVxControlRegName( unsigned int regAddr );

char const *getRS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS);
char const *getCFV1_XCSR_Name( unsigned int XCSR);
char const *getCFVx_CSR_Name( unsigned int CSR);
char const *getStatusRegName(unsigned int targetType, unsigned int value);

char const *getCapabilityName(unsigned int cmd);

char const *getTargetModeName(TargetMode_t type);
char const *getClockSelectName(ClkSwValues_t level);
char const *getVoltageStatusName(TargetVddState_t level);
char const *getVoltageSelectName(TargetVddSelect_t level);
char const *getVppSelectName(TargetVppSelect_t level);
char const *getStatusRegName(unsigned int targetType, unsigned int value);

char const *getControlLevelName(InterfaceLevelMasks_t level);
char const *getPinLevelName(PinLevelMasks_t level);
void printBdmOptions(const USBDM_ExtendedOptions_t *options);
char const *getMDM_APControlName(uint32_t mdmApValue);

char const *getExitAction(int action);

const char *getMemSpaceName(MemorySpace_t memSpace);
#endif // LOG

char const *getHardwareDescription(unsigned int hardwareVersion);
char const *getBriefHardwareDescription(unsigned int hardwareVersion);
char const *getTargetTypeName( unsigned int type );
char const *getErrorName(unsigned int error);
char const *getICPErrorName(unsigned char error);
char const *getConnectionStateName(SpeedMode_t level);
char const *getBDMStatusName(USBDMStatus_t *USBDMStatus);

#endif // _NAMES_H_
