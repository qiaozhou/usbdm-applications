/*!
 * \brief Header file for CmdTable.c
*/
#ifndef NAMES_H_
#define NAMES_H_

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
char const *getARMRegName( unsigned int regAddr );
char const *getDSCRegName( unsigned int regNum);
char const *getHCS12RegName( unsigned int regAddr );
char const *getHCS08RegName( unsigned int regAddr );
char const *getRS08RegName( unsigned int regAddr );
char const *getRegName( unsigned int targetType, unsigned int regAddr );
char const *getSWDDebugRegName( unsigned int regAddr );
char const *getARMControlRegName( unsigned int regAddr );

char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS);
char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getRS08_BDCSCR_Name( unsigned int BDCSCR);
char const *getCFV1_XCSR_Name( unsigned int XCSR);
char const *getCFVx_CSR_Name( unsigned int CSR);
char const *getStatusRegName(unsigned int targetType, unsigned int value);

char const *getBDMStatusName(USBDMStatus_t *USBDMStatus);

char const *getTargetModeName(TargetMode_t type);

char const *getControlLevelName(InterfaceLevelMasks_t level);

char const *getClockSelectName(ClkSwValues_t level);
char const *getVoltageStatusName(TargetVddState_t level);
char const *getVoltageSelectName(TargetVddSelect_t level);
char const *getVppSelectName(TargetVppSelect_t level);

const char *getExitAction(int action);

const char *getDHCSRName(uint32_t dhcsrValue);
const char *getDEMCRName(uint32_t dhcsrValue);
// Freescale MDM-AP
const char *getMDM_APStatusName(uint32_t mdmApValue);
const char *getMDM_APControlName(uint32_t mdmApValue);

const char *getSRSLName(uint32_t srslValue);
const char *getSRSHName(uint32_t srshValue);
const char *getDpRegName(int reg);
char const *ARM_GetRegisterName( unsigned int regAddr );
#endif // NAMES_H_
