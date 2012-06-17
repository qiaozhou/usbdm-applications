/*
 * ArmDebug.h
 *
 *  Created on: 04/03/2011
 *      Author: PODonoghue
 */

#ifndef ARMDEBUG_H_
#define ARMDEBUG_H_
#ifndef LOG
static inline const char *getMDM_APControlName(uint32_t mdmApValue) { return "";}
static inline const char *getMDM_APStatusName(uint32_t mdmApValue)  { return "";}
static inline const char *ARM_GetMemoryName(uint32_t address)  { return "";}
static inline const char *getDHCSRName(uint32_t dhcsrValue)  { return "";}
static inline const char *getDEMCRName(uint32_t dhcsrValue)  { return "";}
#else
//! Get name of ARM memory location/region
//!
//! @param address address to check
//!
//! @return ptr to static buffer of form <name>:<address>
//!
const char *ARM_GetMemoryName(uint32_t address);

//! Get WAIT/OK_FAULT name
const char *getACKName(int ack);

//! Get DP reg name ("CTRL/STAT","SELECT","RDBUFF")
const char *getDpRegName(int reg);

//! Get DHCSR bit names
const char *getDHCSRName(uint32_t dhcsrValue);

//! Get DHCSR bit names
const char *getDEMCRName(uint32_t dhcsrValue);

//! Get Freescale MDM-AP Status bit names
const char *getMDM_APStatusName(uint32_t mdmApValue);

//! Get Freescale MDM-AP Control bit names
const char *getMDM_APControlName(uint32_t mdmApValue);
#endif

#endif /* ARMDEBUG_H_ */
