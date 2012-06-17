/*! \file
    \brief Header file for USBDM_ARM_API.c

    \verbatim
    Copyright (C) 2010  Peter O'Donoghue

    Based on material from OSBDM-JM60 Target Interface Software Package
    Copyright (C) 2009  Freescale

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

    Change History
   +====================================================================
   |    May 2010 | Created
   +====================================================================
    \endverbatim
*/
#ifndef USBDM_ARM_API_H_
#define USBDM_ARM_API_H_

#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

#ifdef _WIN32
   #if defined(DLL)
      //! This definition is used when being exported (creating DLL)
      #define USBDM_ARM_API WINAPI     __declspec(dllexport)
   #elif defined (TESTER) || defined (DOC) || defined (BOOTLOADER)
      //! This empty definition is used for the command-line version that statically links the DLL routines
      #define USBDM_ARM_API WINAPI
   #else
      //! This definition is used when being imported (linking against DLL)
      #define USBDM_ARM_API WINAPI     __declspec(dllimport)
   #endif
#else
   #if defined(DLL)
      //! This definition is used when being exported (creating DLL)
      #define USBDM_ARM_API __attribute__ ((visibility ("default")))
   #else
      //! This definition is used when being imported etc
      #define USBDM_ARM_API
   #endif
#endif

//! Information about the target Debug interface
typedef struct {
   uint32_t dhcsr;         //!< original DHCSR register read from target
   uint32_t mdmApStatus;   //!< MDM AP STATUS
   uint32_t reserved1;     //!< Not used
   uint32_t reserved2;     //!< Not used
} ArmStatus;

//! Initialise ARM interface
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_Initialise(void);


//! Get ARM target status
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_GetStatus(ArmStatus *status);

//! Connect to target
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_Connect(void);

//! Reset Target
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_TargetReset(TargetMode_t targetMode);

//! Start Target execution at current PC
//!
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_TargetGo(void);

//! Execute N instructions from current PC
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_TargetStepN(unsigned numSteps);

//! Execute 1 instruction from current PC
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_TargetStep(void);

//! Halts target execution
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_TargetHalt(void);

//! Read data from target memory
//!
//! @param elementSize = Size of data (1/2/4 bytes)
//! @param byteCount   = Number of bytes to transfer
//! @param address     = Memory address
//! @param data        = Where to place data
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ARM_API
USBDM_ErrorCode  ARM_ReadMemory( unsigned int  elementSize,
                                 unsigned int  byteCount,
                                 unsigned int  address,
                                 unsigned char *data);

//! Write data to target memory
//!
//! @param elementSize = Size of data elements (1/2/4 bytes)
//! @param byteCount   = Number of _bytes_ to transfer
//! @param address     = Memory address
//! @param data        = Ptr to block of data to write
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_ARM_API
USBDM_ErrorCode  ARM_WriteMemory( unsigned int  elementSize,
                                  unsigned int  byteCount,
                                  unsigned int  address,
                                  unsigned const char *data);

//! Read Register
//!
//! @param regNo     - Register number
//! @param regValue  - Value from register
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_ReadRegister(ARM_Registers_t regNo, unsigned long *regValue);

//! Write Register
//!
//! @param regNo     - Register number
//! @param regValue  - Value for register
//!
//! @note Assumes Core TAP is active & in RUN-TEST/IDLE
//! @note Leaves Core TAP in RUN-TEST/IDLE
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_WriteRegister(ARM_Registers_t regNo, unsigned long regValue);

//! Read Control Register (
//!
//! @param regNo     - Register number in AP address space
//!                     e.g. MDM-AP.Status  = 0x0100000
//!                          MDM-AP.Control = 0x0100004
//!                          MDM-AP.Ident   = 0x01000FC
//! @param regValue  - Value from register
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_ReadCReg(unsigned int regNo, unsigned long *regValue);

//! Write Control Register (
//!
//! @param regNo     - Register number in AP address space
//!                     e.g. MDM-AP.Status  = 0x0100000
//!                          MDM-AP.Control = 0x0100004
//!                          MDM-AP.Ident   = 0x01000FC
//! @param regValue  - Value from register
//!
USBDM_ARM_API
USBDM_ErrorCode ARM_WriteCReg(unsigned int regNo, unsigned long regValue);

//! Set logfile for debug code to use
//! @param fp - FILE pointer to use
//!
USBDM_ARM_API
void ARM_SetLogFile(FILE *fp);

#if defined __cplusplus
    }
#endif

#endif /* USBDM_ARM_API_H_ */
