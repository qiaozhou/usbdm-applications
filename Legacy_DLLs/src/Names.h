/*!
 * \brief Header file for CmdTable.c
*/
#ifndef _NAMES_H_
#define _NAMES_H_

#include <USBDM_API.h>
//#include <USBDM_AUX.h>

#ifndef LOG
// Dummy the routines if logging is not required
static inline char const *getCommandName(unsigned char command)        { return ""; } //!< Dummy
static inline char const *getDebugCommandName(unsigned char cmd)       { return ""; } //!< Dummy

static inline char const *getCFV1ControlRegName( unsigned int regAddr ){ return ""; } //!< Dummy
static inline char const *getCFVxControlRegName( unsigned int regAddr ){ return ""; } //!< Dummy

static inline char const *getCFV1DebugRegName( unsigned int regAddr )  { return ""; } //!< Dummy
static inline char const *getCFVxDebugRegName( unsigned int regAddr )  { return ""; } //!< Dummy

static inline char const *getCFV1RegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getCFVxRegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getHCS12RegName( unsigned int regAddr )      { return ""; } //!< Dummy
static inline char const *getHCS08RegName( unsigned int regAddr )      { return ""; } //!< Dummy
static inline char const *getRS08RegName( unsigned int regAddr )       { return ""; } //!< Dummy
static inline char const *getRegName( unsigned int targetType, unsigned int regAddr ) { return ""; } //!< Dummy

static inline char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS)   { return ""; } //!< Dummy
static inline char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR)   { return ""; } //!< Dummy
static inline char const *getRS08_BDCSCR_Name( unsigned int BDCSCR)    { return ""; } //!< Dummy
static inline char const *getCFV1_XCSR_Name( unsigned int XCSR)        { return ""; } //!< Dummy
static inline char const *getCFVx_CSR_Name( unsigned int CSR)          { return ""; } //!< Dummy
static inline char const *getStatusRegName(unsigned int targetType, unsigned int value) { return ""; } //!< Dummy

static inline char const *getCapabilityName(unsigned int vector)       { return ""; } //!< Dummy
static inline char const *getDFUCommandName(unsigned char command)     { return ""; } //!< Dummy

static inline char const *getTargetModeName(TargetMode_t type)         { return ""; } //!< Dummy

static inline char const *getControlLevelName(InterfaceLevelMasks_t level) { return ""; } //!< Dummy

static inline char const *getClockSelectName(ClkSwValues_t level) { return ""; }
static inline char const *getVoltageSelectName(TargetVddSelect_t level) { return ""; }
static inline char const *getVppSelectName(TargetVppSelect_t level) { return ""; }

static inline char const *getExitAction(int action) { return ""; }

static inline void printBdmOptions(USBDM_ExtendedOptions_t *options) { };

#endif // LOG

const char *getTargetTypeName( unsigned int type );
const char *getHardwareDescription(unsigned int hardwareVersion);
const char *getBriefHardwareDescription(unsigned int hardwareVersion);

const char *getDFUCommandName(unsigned char command);
const char *getTargetTypeName( unsigned int type );
const char *getCommandName(unsigned char command);
const char *getErrorName(unsigned int error);
const char *getICPErrorName(unsigned char error);
const char *getDebugCommandName(unsigned char cmd);
const char *getCapabilityName(unsigned int cmd);

char const *getCFV1ControlRegName( unsigned int regAddr );
char const *getCFVxControlRegName( unsigned int regAddr );

char const *getHCS12DebugRegName( unsigned int regAddr );
char const *getCFV1DebugRegName( unsigned int regAddr );
char const *getCFVxDebugRegName( unsigned int regAddr );

char const *getCFV1RegName( unsigned int regAddr );
char const *getCFVxRegName( unsigned int regAddr );
char const *getHCS12RegName( unsigned int regAddr );
char const *getHCS08RegName( unsigned int regAddr );
char const *getRS08RegName( unsigned int regAddr );
char const *getRegName( unsigned int targetType, unsigned int regAddr );

char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS);
char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getRS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getCFV1_XCSR_Name( unsigned int XCSR);
char const *getCFVx_CSR_Name( unsigned int CSR);
char const *getStatusRegName(unsigned int targetType, unsigned int value);

char const *getBDMStatusName(USBDMStatus_t *USBDMStatus);

char const *getTargetModeName(TargetMode_t type);

char const *getControlLevelName(InterfaceLevelMasks_t level);
char const *getPinLevelName(PinLevelMasks_t level);

char const *getClockSelectName(ClkSwValues_t level);
char const *getVoltageStatusName(TargetVddState_t level);
char const *getVoltageSelectName(TargetVddSelect_t level);
char const *getVppSelectName(TargetVppSelect_t level);

const char *getExitAction(int action);

char const *getConnectionStateName(SpeedMode_t level);
char const *getVoltageStatusName(TargetVddState_t level);
#if 0
char const *getConnectionRetryName(RetryMode mode);
#endif
char const *getTargetTypeName( unsigned int type );
char const *getBDMStatusName(USBDMStatus_t *USBDMStatus);

void printBdmOptions(USBDM_ExtendedOptions_t *options);
#endif // _NAMES_H_
