/*! \file
    \brief Header file for USBDM_API.c

*/
#ifndef _USBDM_API_H_
#define _USBDM_API_H_

#ifndef WINAPI
#define WINAPI __attribute__((__stdcall__)) //!< -
#endif

#if defined __cplusplus
    extern "C" {
#endif

#ifdef WIN32
   #if defined(DLL)
      //! This definition is used when USBDM_API.h is being exported (creating DLL)
      #define USBDM_API WINAPI     __declspec(dllexport)
      #define OSBDM_API extern "C" __declspec(dllexport)
   #elif defined (TESTER) || defined (DOC) || defined (BOOTLOADER)
      //! This empty definition is used for the command-line version that statically links the DLL routines
   #define USBDM_API WINAPI
   #define OSBDM_API extern "C"
   #else
       //! This definition is used when USBDM_API.h is being imported (linking against DLL)
      #define USBDM_API WINAPI     __declspec(dllimport)
      #define OSBDM_API extern "C" __declspec(dllimport)
   #endif
#endif

#ifdef __unix__
   #if defined(DLL)
      //! This definition is used when USBDM_API.h is being exported (creating DLL)
      #define USBDM_API __attribute__ ((visibility ("default")))
      #define OSBDM_API extern "C" __attribute__ ((visibility ("default")))
   #else
       //! This definition is used when USBDM_API.h is being imported etc
      #define USBDM_API
      #define OSBDM_API extern "C"
   #endif
#endif

// ToDo - Fix this!
#include "Common.h"

////! Struct to contain the Flash programming parameters for each
////! RS08 Variant
////!
//typedef struct {
//   U8    directRamStart;      //!< Direct RAM start address
//   U8    directRamEnd;        //!< Direct RAM end address
//   U16   SDIDAddress;         //!< System Device Identification Reg address
//   U16   SDIDValue;           //!< System Device Identification value
//   U16   FLCRAddress;         //!< Flash Control Register address
//   U16   SOPTAddress;         //!< System Option Register address
//   U8    programLength;       //!< Length of Flash programming code
//   U8    program[48];         //!< Flash programming code
//} RS08_FlashInformationT;

//! Error codes returned from BDM routines and BDM commands.
//!
typedef enum  {
 BDM_RC_OK                      = 0,     //!< - No error
 BDM_RC_ILLEGAL_PARAMS          = 1,     //!< - Illegal parameters to command
 BDM_RC_FAIL                    = 2,     //!< - General Fail
 BDM_RC_BUSY                    = 3,     //!< - Busy with last command - try again - don't change
 BDM_RC_ILLEGAL_COMMAND         = 4,     //!< - Illegal (unknown) command (may be in wrong target mode)
 BDM_RC_NO_CONNECTION           = 5,     //!< - No connection to target
 BDM_RC_OVERRUN                 = 6,     //!< - New command before previous command completed
 BDM_RC_CF_ILLEGAL_COMMAND      = 7,     //!< - Coldfire BDM interface did not recognize the command
 //
 BDM_RC_UNKNOWN_TARGET          = 15,    //!< - Target not supported
 BDM_RC_NO_TX_ROUTINE           = 16,    //!< - No Tx routine available at measured BDM communication speed
 BDM_RC_NO_RX_ROUTINE           = 17,    //!< - No Rx routine available at measured BDM communication speed
 BDM_RC_BDM_EN_FAILED           = 18,    //!< - Failed to enable BDM mode in target
 BDM_RC_RESET_TIMEOUT_FALL      = 19,    //!< - RESET signal failed to fall
 BDM_RC_BKGD_TIMEOUT            = 20,    //!< - BKGD signal failed to rise/fall
 BDM_RC_SYNC_TIMEOUT            = 21,    //!< - No response to SYNC sequence
 BDM_RC_UNKNOWN_SPEED           = 22,    //!< - Communication speed is not known or cannot be determined
 BDM_RC_WRONG_PROGRAMMING_MODE  = 23,    //!< - Attempted Flash programming when in wrong mode (e.g. Vpp off)
 BDM_RC_FLASH_PROGRAMING_BUSY   = 24,    //!< - Busy with last Flash programming command
 BDM_RC_VDD_NOT_REMOVED         = 25,    //!< - Target Vdd failed to fall
 BDM_RC_VDD_NOT_PRESENT         = 26,    //!< - Target Vdd not present/failed to rise
 BDM_RC_VDD_WRONG_MODE          = 27,    //!< - Attempt to cycle target Vdd when not controlled by BDM interface
 BDM_RC_CF_BUS_ERROR            = 28,    //!< - Illegal bus cycle on target (Coldfire)
 BDM_RC_USB_ERROR               = 29,    //!< - Indicates USB transfer failed (returned by driver not BDM)
 BDM_RC_ACK_TIMEOUT             = 30,    //!< - Indicates an expected ACK was missing
 BDM_RC_FAILED_TRIM             = 31,    //!< - Trimming of target clock failed (out of clock range?).
 BDM_RC_FEATURE_NOT_SUPPORTED   = 32,    //!< - Feature not supported by this version of hardware/firmware
 BDM_RC_RESET_TIMEOUT_RISE      = 33,    //!< - RESET signal failed to rise

 // The following are used by additional USBDM code
 BDM_RC_WRONG_BDM_REVISION      = 34,    //!< - BDM Hardware is incompatible with driver/program
 BDM_RC_WRONG_DLL_REVISION      = 35,    //!< - Program is incompatible with DLL
 BDM_RC_NO_USBDM_DEVICE         = 36,    //!< - No usbdm device was located

 BDM_RC_JTAG_UNMATCHED_REPEAT   = 37,    //!< - Unmatched REPEAT-END_REPEAT
 BDM_RC_JTAG_UNMATCHED_RETURN   = 38,    //!< - Unmatched CALL-RETURN
 BDM_RC_JTAG_UNMATCHED_IF       = 39,    //!< - Unmatched IF-END_IF
 BDM_RC_JTAG_STACK_ERROR        = 40,    //!< - Underflow in call/return sequence, unmatched REPEAT etc.
 BDM_RC_JTAG_ILLEGAL_SEQUENCE   = 41,    //!< - Illegal JTAG sequence
 BDM_RC_TARGET_BUSY             = 42,    //!< - Target is busy (executing?)
 BDM_RC_JTAG_TOO_LARGE          = 43,    //!< - Subroutine is too large to cache
 BDM_RC_DEVICE_NOT_OPEN         = 44,    //!< - USBDM Device has not been opened
 BDM_RC_UNKNOWN_DEVICE          = 45,    //!< - Device is not in database
 BDM_RC_DEVICE_DATABASE_ERROR   = 46,    //!< - Device database not found or failed to open/parse
} USBDM_ErrorCode;

//! Capabilities of the hardware
//!
typedef enum  {
   BDM_CAP_NONE         = (0),
   BDM_CAP_ALL          = (0xFFFF),
   BDM_CAP_HCS12        = (1<<0),   //!< - Supports HCS12
   BDM_CAP_RS08         = (1<<1),   //!< - 12 V Flash programming supply available (RS08 support)
   BDM_CAP_VDDCONTROL   = (1<<2),   //!< - Control over target Vdd
   BDM_CAP_VDDSENSE     = (1<<3),   //!< - Sensing of target Vdd
   BDM_CAP_CFVx         = (1<<4),   //!< - Support for CFV 1,2 & 3
   BDM_CAP_HCS08        = (1<<5),   //!< - Supports HCS08 targets - inverted when queried
   BDM_CAP_CFV1         = (1<<6),   //!< - Supports CFV1 targets  - inverted when queried
   BDM_CAP_JTAG         = (1<<7),   //!< - Supports JTAG targets
   BDM_CAP_DSC          = (1<<8),   //!< - Supports DSC targets
   BDM_CAP_RST          = (1<<10),  //!< - Control & sensing of RESET
   BDM_CAP_PST          = (1<<11),  //!< - Supports PST signal sensing
   BDM_CAP_CDC          = (1<<12),  //!< - Supports CDC Serial over USB interface
} HardwareCapabilities_t;

//==========================================================================================
// Targets and visible capabilities supported - related to above but not exactly!
// e.g. CAP_HCS12 => CAP_BDM+CAP_RST_IO
//      CAP_RS08  => CAP_BDM+CAP_FLASH(+CAP_RST_IO)
//      CAP_HCS08 => CAP_BDM(+CAP_RST_IO)
//      CAP_CFVx  => CAP_JTAG_HW+CAP_CFVx_HW+CAP_RST_IO
//      CAP_DSC   => CAP_JTAG_HW+CAP_RST_IO + s/w routines
//      CAP_JTAG  => CAP_JTAG_HW+CAP_RST_IO
//      CAP_RST   => CAP_RST_IO
// TARGET_CAPABILITY
//
#define CAP_HCS12       (1<<0)      // Supports HCS12 targets
#define CAP_RS08        (1<<1)      // Supports RS08 targets
#define CAP_VDDCONTROL  (1<<2)      // Control over target Vdd
#define CAP_VDDSENSE    (1<<3)      // Sensing of target Vdd
#define CAP_CFVx        (1<<4)		// Supports CFVx
#define CAP_HCS08       (1<<5)		// Supports HCS08 targets - inverted when queried
#define CAP_CFV1        (1<<6)		// Supports CFV1 targets  - inverted when queried
#define CAP_JTAG        (1<<7)		// Supports JTAG targets
#define CAP_DSC         (1<<8)		// Supports DSC targets
#define CAP_RST         (1<<10)     // Control & sensing of RESET
#define CAP_PST         (1<<11)     // Supports PST signal sensing
#define CAP_CDC         (1<<12)     // Supports CDC Serial over USB interface
//===================================================================================
//!  Target microcontroller types
//!
typedef enum {
   T_HC12      = 0,     //!< - HC12 or HCS12 target
   T_HCS08     = 1,     //!< - HCS08 target
   T_RS08      = 2,     //!< - RS08 target
   T_CFV1      = 3,     //!< - Coldfire Version 1 target
   T_CFVx      = 4,     //!< - Coldfire Version 2,3,4 target
   T_JTAG      = 5,     //!< - JTAG target - TAP is set to \b RUN-TEST/IDLE
   T_EZFLASH   = 6,     //!< - EzPort Flash interface (SPI?)
   T_MC56F80xx = 7,     //!< - JTAG target with MC56F80xx optimised subroutines
   T_OFF       = 0xFF,  //!< - Turn off interface (no target)
} TargetType_t;

//!  Target RS08 microcontroller derivatives
//!
typedef enum {
   KA1 = 0, //!< - RS08KA1
   KA2 = 1, //!< - RS08KA2
   LA8 = 2, //!< - RS08LA8
   KA4 = 4, //!< - RS08KA4
   KA8 = 5, //!< - RS08KA8
   LE4 = 6, //!< - RS08LE4
} DerivativeType_t;

//! Target supports ACKN or uses fixed delay {WAIT} instead
//!
typedef enum {
   WAIT  = 0,   //!< - Use WAIT (delay) instead
   ACKN  = 1,   //!< - Target supports ACKN feature and it is enabled
} AcknMode_t;

//! Target speed selection
//!
typedef enum {
   SPEED_NO_INFO        = 0,   //!< - Not connected
   SPEED_SYNC           = 1,   //!< - Speed determined by SYNC
   SPEED_GUESSED        = 2,   //!< - Speed determined by trial & error
   SPEED_USER_SUPPLIED  = 3    //!< - User has specified the speed to use
} SpeedMode_t;

//! Target RSTO state
//!
typedef enum {
   RSTO_ACTIVE=0,     //!< - RSTO* is currently active [low]
   RSTO_INACTIVE=1    //!< - RSTO* is currently inactive [high]
} ResetState_t;

//! Target reset status values
//!
typedef enum {
   NO_RESET_ACTIVITY    = 0,   //!< - No reset activity since last polled
   RESET_INACTIVE       = NO_RESET_ACTIVITY,
   RESET_DETECTED       = 1    //!< - Reset since last polled
} ResetMode_t;

//! Target Halt state
//!
typedef enum {
   TARGET_RUNNING    = 0,   //!< - CFVx target running (ALLPST == 0)
   TARGET_HALTED     = 1    //!< - CFVx target halted (ALLPST == 1)
} TargetRunState_t;

//! Target Voltage supply state
//!
typedef enum  {
   BDM_TARGET_VDD_NONE      = 0,   //!< - Target Vdd not detected
   BDM_TARGET_VDD_EXT       = 1,   //!< - Target Vdd external
   BDM_TARGET_VDD_INT       = 2,   //!< - Target Vdd internal
   BDM_TARGET_VDD_ERR       = 3,   //!< - Target Vdd error
} TargetVddState_t;

//====================================================================================

//! Internal Target Voltage supply selection
//!
typedef enum  {
   BDM_TARGET_VDD_OFF       = 0,   //!< - Target Vdd Off
   BDM_TARGET_VDD_3V3       = 1,   //!< - Target Vdd internal 3.3V
   BDM_TARGET_VDD_5V        = 2,   //!< - Target Vdd internal 5.0V
} TargetVddSelect_t;

//! Internal Programming Voltage supply selection
//!
typedef enum  {
   BDM_TARGET_VPP_OFF       = 0,   //!< - Target Vpp Off
   BDM_TARGET_VPP_STANDBY   = 1,   //!< - Target Vpp Standby (Inverter on, Vpp off)
   BDM_TARGET_VPP_ON        = 2,   //!< - Target Vpp On
   BDM_TARGET_VPP_ERROR     = 3,   //!< - Target Vpp ??
} TargetVppSelect_t;

//! Target BDM Clock selection
//!
typedef enum {
   CS_DEFAULT           = 0xFF,  //!< - Use default clock selection (don't modify target's reset default)
   CS_ALT_CLK           =  0,    //!< - Force ALT clock (CLKSW = 0)
   CS_NORMAL_CLK        =  1,    //!< - Force Normal clock (CLKSW = 1)
} ClkSwValues_t;

//!  Reset mode as used by CMD_USBDM_TARGET_RESET
//!
typedef enum { /* type of reset action required */
   RESET_MODE_MASK   = (3<<0), //!< Mask for reset mode (SPECIAL/NORMAL)
   RESET_SPECIAL     = (0<<0), //!< - Special mode [BDM active, Target halted]
   RESET_NORMAL      = (1<<0), //!< - Normal mode [usual reset, Target executes]

   RESET_TYPE_MASK   = (3<<2), //!< Mask for reset type (Hardware/Software/Power)
   RESET_ALL         = (0<<2), //!< Use all reset stategies as appropriate
   RESET_HARDWARE    = (1<<2), //!< Use hardware RESET pin reset
   RESET_SOFTWARE    = (2<<2), //!< Use software (BDM commands) reset
   RESET_POWER       = (3<<2), //!< Cycle power

   // Legacy methods
//   SPECIAL_MODE = RESET_SPECIAL|RESET_ALL,  //!< - Special mode [BDM active, Target halted]
//   NORMAL_MODE  = RESET_NORMAL|RESET_ALL,   //!< - Normal mode [usual reset, Target executes]

} TargetMode_t;

#ifdef USBDM_API

//=======================================================================
//
// regNo Parameter values for USBDM_ReadReg()
//
//=======================================================================


//! regNo Parameter for USBDM_ReadReg{} with HCS12 target
//!
//! @note CCR is accessed through USBDM_ReadDReg{}
typedef enum {
   HCS12_RegPC = 3, //!< - PC reg
   HCS12_RegD  = 4, //!< - D reg
   HCS12_RegX  = 5, //!< - X reg
   HCS12_RegY  = 6, //!< - Y reg
   HCS12_RegSP = 7, //!< - SP reg
   HCS12_RegCCR   = 0x80,  //!< - CCR reg - redirected to USBDM_ReadDReg()
} HCS12_Registers_t;

//! regNo Parameter for USBDM_ReadReg{} with HCS08 target
//!
typedef enum {
   HCS08_RegPC  = 0xB, //!< - PC  reg
   HCS08_RegSP  = 0xF, //!< - SP  reg
   HCS08_RegHX  = 0xC, //!< - HX  reg
   HCS08_RegA   = 8,   //!< - A   reg
   HCS08_RegCCR = 9,   //!< - CCR reg
} HCS08_Registers_t;

//! regNo Parameter for USBDM_ReadReg{} with RS08 target
//!
typedef enum {
   RS08_RegCCR_PC  = 0xB, //!< - Combined CCR/PC register
   RS08_RegSPC     = 0xF, //!< - Shadow PC
   RS08_RegA       = 8,   //!< - A reg
} RS08_Registers_t;

//! regNo Parameter for USBDM_ReadReg{} with CFV1 target
//!
typedef enum {
   CFV1_RegD0     = 0, //!< - D0
   CFV1_RegD1     = 1, //!< - D1
   CFV1_RegD2     = 2, //!< - D2
   CFV1_RegD3     = 3, //!< - D3
   CFV1_RegD4     = 4, //!< - D4
   CFV1_RegD5     = 5, //!< - D5
   CFV1_RegD6     = 6, //!< - D6
   CFV1_RegD7     = 7, //!< - D7
   CFV1_RegA0     = 8, //!< - A0
   CFV1_RegA1     = 9, //!< - A1
   CFV1_RegA2     = 10, //!< - A2
   CFV1_RegA3     = 11, //!< - A3
   CFV1_RegA4     = 12, //!< - A4
   CFV1_RegA5     = 13, //!< - A5
   CFV1_RegA6     = 14, //!< - A6
   CFV1_RegA7     = 15, //!< - A7
   CFV1_PSTBASE   = 16, //!< - Start of PST registers, access as CFV1_PSTBASE+n
} CFV1_Registers_t;

//! regNo Parameter for USBDM_ReadReg{} with CFVx target
//!
typedef enum {
   CFVx_RegD0  = 0, //!< - D0
   CFVx_RegD1  = 1, //!< - D1
   CFVx_RegD2  = 2, //!< - D2
   CFVx_RegD3  = 3, //!< - D3
   CFVx_RegD4  = 4, //!< - D4
   CFVx_RegD5  = 5, //!< - D5
   CFVx_RegD6  = 6, //!< - D6
   CFVx_RegD7  = 7, //!< - D7
   CFVx_RegA0  = 8, //!< - A0
   CFVx_RegA1  = 9, //!< - A1
   CFVx_RegA2  = 10, //!< - A2
   CFVx_RegA3  = 11, //!< - A3
   CFVx_RegA4  = 12, //!< - A4
   CFVx_RegA5  = 13, //!< - A5
   CFVx_RegA6  = 14, //!< - A6
   CFVx_RegA7  = 15, //!< - A7
} CFVx_Registers_t;

//=======================================================================
//
// regNo Parameter values for USBDM_ReadCReg()
//
//=======================================================================


//! regNo Parameter for USBDM_ReadCReg{} with CFV1 target
//!
typedef enum {
   CFV1_CRegOTHER_A7  = 0,  //!< - Other A7 (not active in target)
   CFV1_CRegVBR       = 1,  //!< - Vector Base register
   CFV1_CRegCPUCR     = 2,  //!< - CPUCR
   CFV1_CRegSR        = 14, //!< - Status register
   CFV1_CRegPC        = 15, //!< - Program Counter
} CFV1_CRegisters_t;

//! regNo Parameter for USBDM_ReadCReg{} with CFVx target
//!
typedef enum {
   CFVx_CRegD0        = 0x80, //!< - D0-D7
   CFVx_CRegD1,
   CFVx_CRegD2,
   CFVx_CRegD3,
   CFVx_CRegD4,
   CFVx_CRegD5,
   CFVx_CRegD6,
   CFVx_CRegD7,
   CFVx_CRegA0,               //!< - A0-A7
   CFVx_CRegA1,
   CFVx_CRegA2,
   CFVx_CRegA3,
   CFVx_CRegA4,
   CFVx_CRegA5,
   CFVx_CRegA6,
   CFVx_CRegUSER_SP,
   CFVx_CRegOTHER_SP  = 0x800, //!< - Other A7 (not active in target)
   CFVx_CRegVBR       = 0x801, //!< - Vector Base register
   CFVx_CRegSR        = 0x80E, //!< - Status Register
   CFVx_CRegPC        = 0x80F, //!< - Program Counter
   CFV1_CRegFLASHBAR  = 0xC04, //!< - Dlash Base register
   CFV1_CRegRAMBAR    = 0xC05, //!< - RAM Base register
   // May be others
} CFVx_CRegisters_t;

//=======================================================================
//
// regNo Parameter values for USBDM_ReadDReg()
//
//=======================================================================


//! regNo Parameter for USBDM_ReadDReg{} with HCS12 target [BD Space]
//!
//! @note: There may be other registers
//!
typedef enum {
   // 8-bit accesses using READ_BD_BYTE
   HCS12_DRegBDMSTS = 0xFF01, //!< - BDMSTS (debug status/control) register
   HCS12_DRegCCR    = 0xFF06, //!< - Saved Target CCR
   HCS12_DRegBDMINR = 0xFF07, //!< - BDM Internal Register Position Register
   // Others may be device dependent
} HCS12_DRegisters_t;

//! regNo Parameter for USBDM_ReadDReg{} with HCS08 target [BKPT reg]
//!
typedef enum {
   HCS08_DRegBKPT = 0x0, //!< - Breakpoint register
} HCS08_DRegisters_t;

//! regNo Parameter for \ref USBDM_ReadDReg() with RS08 target (BKPT)
//!
typedef enum {
   RS08_DRegBKPT = 0x0, //!< - Breakpoint register
} RS08_DRegisters_t;

//! regNo Parameter for USBDM_ReadDReg{} with CFV1 target
//!
//! @note: There may be other registers
typedef enum {
   CFV1_DRegCSR    = 0, //!< - CSR
   CFV1_DRegXCSR   = 1, //!< - XCSR
   CFV1_DRegCSR2   = 2, //!< - CSR2
   CFV1_DRegCSR3   = 3, //!< - CSR3
   // There are others but I'm lazy!

   CFV1_ByteRegs       = 0x1000, // Special access to msb
   CFV1_DRegXCSRbyte   = CFV1_ByteRegs+CFV1_DRegXCSR, //!< - XCSR.msb
   CFV1_DRegCSR2byte   = CFV1_ByteRegs+CFV1_DRegCSR2, //!< - CSR2.msb
   CFV1_DRegCSR3byte   = CFV1_ByteRegs+CFV1_DRegCSR3, //!< - CSR3.msb
} CFV1_DRegisters_t;

//! regNo Parameter for USBDM_ReadDReg{} with CFV1 target
//!
typedef enum {
   CFVx_DRegCSR    = 0, //!< - CSR reg
} CFVx_DRegisters_t;

//=======================================================================
//
//=======================================================================


//! State of BDM Communication
//!
typedef struct {
   TargetType_t         target_type;       //!< - Type of target (HCS12, HCS08 etc)
   AcknMode_t           ackn_state;        //!< - Supports ACKN ?
   SpeedMode_t          connection_state;  //!< - Connection status & speed determination method
   ResetState_t         reset_state;       //!< - Current target RST0 state
   ResetMode_t          reset_recent;      //!< - Target reset recently?
   TargetRunState_t     halt_state;        //!< - CFVx halted (from ALLPST)?
   TargetVddState_t     power_state;       //!< - Target has power?
   TargetVppSelect_t    flash_state;       //!< - State of Target Vpp
} USBDMStatus_t;

//=======================================================================
//
//  JTAG Interface
//
//=======================================================================

//! Options used with JTAG commands
//!
typedef enum {
   JTAG_STAY_SHIFT    = 0,     //!< - Remain in SHIFT-DR or SHIFT-IR
   JTAG_EXIT_IDLE     = 1,     //!< - Exit SHIFT-XX to RUN-TEST/IDLE
   JTAG_EXIT_SHIFT_DR = 2,     //!< - Exit SHIFT-XX & enter SHIFT-DR w/o crossing RUN-TEST/IDLE
   JTAG_EXIT_SHIFT_IR = 3,     //!< - Exit SHIFT-XX & enter SHIFT-IR w/o crossing RUN-TEST/IDLE
   JTAG_EXIT_ACTION_MASK = 0x3,

   JTAG_WRITE_0       = 0x00,  //!< - Write 0's when reading - combined with above
   JTAG_WRITE_1       = 0x80,  //!< - Write 1's when reading - combined with above
   JTAG_WRITE_MASK    = 0x80,

   JTAG_SHIFT_DR      = 0,     //!< - Enter SHIFT-DR (from TEST-LOGIC-RESET or RUN-TEST/IDLE)
   JTAG_SHIFT_IR      = 1,     //!< - Enter SHIFT-IR (from TEST-LOGIC-RESET or RUN-TEST/IDLE)

   // Deprecated
//   STAY_SHIFT    = JTAG_STAY_SHIFT,
//   EXIT_IDLE     = JTAG_EXIT_IDLE,
//   EXIT_SHIFT_DR = JTAG_EXIT_SHIFT_DR,
//   EXIT_SHIFT_IR = JTAG_EXIT_SHIFT_IR,
//   WRITE_0       = JTAG_WRITE_0,
//   WRITE_1       = JTAG_WRITE_1,
//   SHIFT_DR      = JTAG_SHIFT_DR,
//   SHIFT_IR      = JTAG_SHIFT_IR,

} JTAG_ExitActions_t;

//=======================================================================
//
//  ICP Mode
//
//=======================================================================

//! Error codes returned by JMxx BDM when in ICP mode
//!
typedef enum {
   ICP_RC_OK          = 0,    //!< - No error
   ICP_RC_ILLEGAL     = 1,    //!< - Illegal command or parameters
   ICP_RC_FLASH_ERR   = 2,    //!< - Flash failed to program etc
   ICP_RC_VERIFY_ERR  = 3,    //!< - Verify failed
} ICP_ErrorCode_t;


//=======================================================================
//
//=======================================================================

//! Control signal masks for CMD_USBDM_CONTROL_INTERFACE
typedef enum {
   SI_BKGD           = (3<<0), //!< - Mask for BKGD values (SI_BKGD_LOW, SI_BKGD_HIGH & SI_BKGD_3STATE)
   SI_BKGD_LOW       = (0<<0), //!<    - Set BKGD low
   SI_BKGD_HIGH      = (1<<0), //!<    - Set BKGD high
   SI_BKGD_3STATE    = (2<<0), //!<    - Set BKGD 3-state

   SI_RESET          = (3<<2), //!< - Mask for RESET values (SI_RESET_LOW & SI_RESET_3STATE)
   SI_RESET_LOW      = (0<<2), //!<    - Set Reset low
   SI_RESET_3STATE   = (2<<2), //!<    - Set Reset 3-state

   SI_TA             = (3<<4), //!< - Mask for TA signal (not implemented)
   SI_TA_LOW         = (0<<4), //!<    - Set TA low
   SI_TA_3STATE      = (2<<4), //!<    - Set TA 3-state

   SI_TRST           = (3<<6), //!< - Mask for TRST signal (not implemented)
   SI_TRST_LOW       = (0<<6), //!<    - Set TRST low
   SI_TRST_3STATE    = (2<<6), //!<    - Set TRST 3-state

   SI_DISABLE        = -1,     //!< - Release control (or use CMD_SET_TARGET)
} InterfaceLevelMasks_t ;

//! BDM interface options
//!
typedef struct {
   // Options passed to the BDM
   int  targetVdd;                //!< - Target Vdd (off, 3.3V or 5V)
   int  cycleVddOnReset;          //!< - Cycle target Power  when resetting
   int  cycleVddOnConnect;        //!< - Cycle target Power if connection problems)
   int  leaveTargetPowered;       //!< - Leave target power on exit
   int  autoReconnect;            //!< - Automatically re-connect to target (for speed change)
   int  guessSpeed;               //!< - Guess speed for target w/o ACKN
   int  useAltBDMClock;           //!< - Use alternative BDM clock source in target
   int  useResetSignal;           //!< - Whether to use RESET signal on BDM interface
   int  maskInterrupts;           //!< - Whether to mask interrupts when  stepping
   int  reserved1[2];             //!< - Reserved

   // Options used internally by DLL
   int  manuallyCycleVdd;         //!< - Prompt user to manually cycle Vdd on connection problems
   int  derivative_type;          //!< - RS08 Derivative
   int  targetClockFreq;          //!< - CFVx - Target clock frequency. \n
                                  //!< - Other targets automatically determine clock frequency.
   int  miscOptions;              //!< - Various misc options
   int  usePSTSignals;            //!< - CFVx, PST Signal monitors
   int  reserved2[2];             //!< - Reserved
} BDM_Options_t;

//! Structure to hold version information for BDM
//! @note bdmHardwareVersion should always equal icpHardwareVersion
typedef struct {
   unsigned char bdmSoftwareVersion; //!< Version of USBDM Firmware
   unsigned char bdmHardwareVersion; //!< Version of USBDM Hardware
   unsigned char icpSoftwareVersion; //!< Version of ICP bootloader Firmware
   unsigned char icpHardwareVersion; //!< Version of Hardware (reported by ICP code)
} USBDM_Version_t;

//! Structure describing characteristics of currently open BDM
typedef struct {
   unsigned                size;                 //!< Size of this structure
   int                     BDMsoftwareVersion;   //!< BDM Firmware version
   int                     BDMhardwareVersion;   //!< Hardware version reported by BDM firmware
   int                     ICPsoftwareVersion;   //!< ICP Firmware version
   int                     ICPhardwareVersion;   //!< Hardware version reported by ICP firmware
   HardwareCapabilities_t  capabilities;         //!< BDM Capabilities
   unsigned                commandBufferSize;    //!< Size of BDM Communication buffer
   unsigned                jtagBufferSize;       //!< Size of JTAG buffer (if supported)
} USBDM_bdmInformation_t;

// The following functions are available when in BDM mode
//====================================================================
//
//! Initialises USB interface
//!
//! This must be done before any other operations.
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => USB Error - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_Init(void);
//! Clean up
//!
//! This must be called after all USBDM operations are finished
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => USB Error - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_Exit(void);
//! Get version of the DLL in BCD format
//!
//! @return 8-bit version number (2 BCD digits, major-minor)
//!
USBDM_API
unsigned int USBDM_DLLVersion(void);
//! Get version string for DLL
//!
//! @return ptr to static ASCII string
//!
USBDM_API 
const char *USBDM_DLLVersionString(void);
//! Gets string describing a USBDM error code
//!
//! @param errorCode - error code returned from USBDM API routine.
//!
//! @return - ptr to static string describing the error.
//!
USBDM_API 
const char      *USBDM_GetErrorString(USBDM_ErrorCode errorCode);

//=============================================================================
//=============================================================================
//=============================================================================
// USBDM Device handling

//! Find USBDM Devices \n
//! This function creates an internal list of USBDM devices.  The list is held until
//! USBDM_ReleaseDevices() is called. \n
//! USBDM_FindDevices() must be done before any device may be opened.
//!
//! @param deviceCount Number of BDM devices found
//!
//! @return \n
//!     BDM_RC_OK                => OK \n
//!     BDM_RC_NO_USBDM_DEVICE   => No devices found.
//!     other                    => other Error - see \ref USBDM_ErrorCode
//!
//! @note deviceCount == 0 on any error so may be used w/o checking rc
//! @note The device list is held until USBDM_ReleaseDevices() is called
//!
USBDM_API 
USBDM_ErrorCode  USBDM_FindDevices(unsigned int *deviceCount);
//! Release USBDM Device list
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => USB Error - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_ReleaseDevices(void);
//! Obtain serial number of the currently opened BDM
//!
//! @param deviceSerialNumber  Updated to point to UTF-16LE serial number
//!
//! @return \n
//!    == BDM_RC_OK (0)     => Success\n
//!    == BDM_RC_USB_ERROR  => USB failure
//!
//! @note deviceDescription points at a statically allocated buffer - do not free
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetBDMSerialNumber(const char **deviceSerialNumber);
//! Obtain description of the currently opened BDM
//!
//! @param deviceDescription  Updated to point to UTF-16LE device description
//!
//! @return \n
//!    == BDM_RC_OK (0)     => Success\n
//!    == BDM_RC_USB_ERROR  => USB failure
//!
//! @note deviceDescription points at a statically allocated buffer - do not free
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetBDMDescription(const char **deviceDescription);
//! Opens a device
//!
//! @param deviceNo Number (0..N) of device to open.
//! A device must be open before any communication with the device can take place.
//!
//! @note The range of device numbers must be obtained from USBDM_FindDevices() before
//! calling this function.
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_Open(unsigned char deviceNo);
//! Closes currently open device
//!
//! @return \n
//!     BDM_RC_OK => OK
//!
USBDM_API 
USBDM_ErrorCode  USBDM_Close(void);
//! Gets BDM software version and type of hardware
//!
//! @param version Version numbers (4 bytes)
//!  - USBDM software version              \n
//!  - USBDM hardware type                 \n
//!  - ICP software version                \n
//!  - ICP hardware type                   \n
//!  USBDM & ICP hardware versions should always agree
//!
//! @return \n
//!     BDM_RC_OK   => OK \n
//!     other       => Error code - see \ref USBDM_ErrorCode
//!
//! @note - This command is directed to EP0 and is also allowed in ICP mode
//!
//! @deprecated: use USBDM_GetBdmInformation()
//!
USBDM_API 
USBDM_ErrorCode USBDM_GetVersion(USBDM_Version_t *version);
//! \brief Obtains the Capability vector from the BDM interface
//!
//! @param capabilities : ptr to where to return capability vector see \ref HardwareCapabilities_t
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
//! @deprecated: use USBDM_GetBdmInformation()
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetCapabilities(HardwareCapabilities_t *capabilities);
//! \brief Obtains information about the currently open BDM interface
//!
//! @param info ptr to structure to contain the information
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
//! @note The size element of info should be initialised before call.
//! @note Uses cached information from when the BDM was opened.
//!
USBDM_API 
USBDM_ErrorCode USBDM_GetBdmInformation(USBDM_bdmInformation_t *info);
//! Set BDM interface options
//!
//! @param bdmOptions : Options to pass to BDM interface
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_SetOptions(BDM_Options_t *bdmOptions);
//! Sets Target programming voltage
//!
//! @param targetVpp => control value for Vpp
//!
//! @note Before enabling target Vpp it is necessary to do the following: \n
//! - Target device must be set to T_RS08 \n
//! - Target Vdd must be present (internally or externally) \n
//! - The Target Vpp must be first set to BDM_TARGET_VPP_STANDBY then BDM_TARGET_VPP_ON \n
//! The above is checked by the BDM firmware
//!
//! @return \n
//!     BDM_RC_OK     => OK \n
//!     else          => Various errors
//!
USBDM_API 
USBDM_ErrorCode  USBDM_SetTargetVpp(TargetVppSelect_t targetVpp);
//! Directly manipulate interface levels
//!
//! @param control       => mask indicating interface levels see \ref InterfaceLevelMasks_t
//! @param duration_10us => time (in 10us ticks) to assert level
//!                         (0 = indefinite)
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
//! @note - Only partially implemented in BDM firmware
//!
USBDM_API 
USBDM_ErrorCode USBDM_ControlInterface(unsigned char duration_10us, unsigned int  control);
//=============================================================================
//=============================================================================
//=============================================================================
// Target handling

//! Sets target MCU type
//!
//! @param target_type type of target
//!
//! @return 0 => Success,\n !=0 => Fail
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_SetTargetType(TargetType_t target_type);
//! Execute debug command (various, see DebugSubCommands)
//!
//! @param usb_data - Command for BDM
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_Debug(unsigned char *usb_data);
//! Get status of the last command
//!
//! @return \n
//!     BDM_RC_OK          => OK \n
//!     BDM_RC_USB_ERROR   => USB failure \n
//!     other              => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API
USBDM_ErrorCode USBDM_GetCommandStatus(void);
//! Fills user supplied structure with state of BDM communication channel
//!
//! @param USBDMStatus Pointer to structure to receive status, see \ref USBDMStatus_t
//!
//! @return \n
//!     BDM_RC_OK => OK \n
//!     other     => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetBDMStatus(USBDMStatus_t *USBDMStatus);
//! Connects to Target.
//!
//! This will cause the BDM module to attempt to connect to the Target.
//! In most cases the BDM module will automatically determine the connection
//! speed and successfully connect.  If unsuccessful, it may be necessary
//! to manually set the speed using set_speed().
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_Connect(void);
//! Sets the BDM communication speed.
//!
//! @param frequency => BDM Communication speed in kHz \n
//! - T_CFVx, T_JTAG, T_MC56F80xx : JTAG clock frequency \n
//! - RS08, HCS08, HCS12, CFV1    : BDM interface frequency \n
//!    Usually equal to the CPU Bus frequency. \n
//!    May be unrelated to bus speed if alternative BDM clock is in use. \n
//!    A value of zero causes a connect sequence
//!    instead i.e. automatically try to determine connection speed. \n
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_SetSpeed( unsigned long frequency);
//! Get the BDM communication speed in kHz
//!
//! @param frequency => BDM Communication speed in kHz
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetSpeed(unsigned long *frequency /* kHz */);
//! Get the BDM communication speed in Hz
//!
//! @param frequency => BDM Communication speed in Hz
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_GetSpeedHz(unsigned long *frequency /* Hz */);
//! Reads Target Status register byte
//!
//! @param BDMStatusReg => status register value read. \n
//! The register read depends on target:
//!  - HCS12 = BDMSTS, BDM Status register \n
//!  - HCS08 = BDCSCR, BDM Status & Control register \n
//!  - RS08  = BDCSCR, BDM Status & Control register \n
//!  - CFV1  = XCSR[31..24], Extended Configuration/Status Register \n
//!  - CFVx  = CSR, Configuration/Status Register (CSR)
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
//! @note \n
//!    The BDM may resynchronize with the target before doing this read.
//!
USBDM_API 
USBDM_ErrorCode  USBDM_ReadStatusReg(unsigned long *BDMStatusReg);
//! Write Target Control Register byte
//!
//! @param value => value to write
//! The register written depends on target:
//!  - HCS12 = BDMSTS, BDM Status register \n
//!  - HCS08 = BDCSCR, BDM Status & Control register \n
//!  - RS08  = BDCSCR, BDM Status & Control register \n
//!  - CFV1  = XCSR[31..24], Extended Configuration/Status Register \n
//!  - CFVx  not supported (Access CSR through writeControlReg())
//!
//! @return \n
//!     BDM_RC_OK  => OK \n
//!     other      => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_WriteControlReg(unsigned int value);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// Target run control

//! Resets the target to normal or special mode
//!
//! @param target_mode see
//!
//! @return \n
//!     BDM_RC_OK   => OK \n
//!     other       => Error code - see \ref USBDM_ErrorCode
//!
//! @note V4.4 onwards - Communication with the target is lost.  It is necessary to use
//! USBDM_Connect() to re-connect.  This is no longer done automatically
//! as it may interfere with security checking if attempted too soon after reset.
//!
USBDM_API 
USBDM_ErrorCode  USBDM_TargetReset(TargetMode_t target_mode);
//! Steps over a single target instruction
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_TargetStep(void);
//! Starts target execution from current PC address
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_TargetGo(void);
//! Brings the target into active background mode.  The target will be halted.
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_TargetHalt(void);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// Target register Read/Write

//! Write Target Core register
//!
//! @param regNo    Register #
//!  - HCS12 = D,X,Y,SP,PC see \ref HCS12_Registers_t          \n
//!  - HCS08 = A,HX,SP,CCR,PC see \ref HCS08_Registers_t       \n
//!  - RS08  = CCR_PC,SPC,A see \ref RS08_Registers_t          \n
//!  - CFV1  = Core register space D0-D7, A0-A7 see \ref CFV1_Registers_t  \n
//!  - CFVx  = Core register space D0-D7, A0-A7 see \ref CFVx_Registers_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_WriteReg(unsigned int regNo, unsigned long regValue);
//! Read Target Core register
//!
//! @param regNo    Register #
//!  - HCS12 = D,X,Y,SP,PC see \ref HCS12_Registers_t          \n
//!  - HCS08 = A,HX,SP,CCR,PC see \ref HCS08_Registers_t       \n
//!  - RS08  = CCR_PC,SPC,A see \ref RS08_Registers_t          \n
//!  - CFV1  = Core register space D0-D7, A0-A7, PSTBASE+n, see \ref CFV1_Registers_t  \n
//!  - CFVx  = Core register space D0-D7, A0-A7 see \ref CFVx_Registers_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
//! @note HCS12_RegCCR is accessed through \ref USBDM_ReadDReg()
//!
USBDM_API 
USBDM_ErrorCode USBDM_ReadReg(unsigned int regNo, unsigned long *regValue);
//! Write Target Control register
//!
//! @param regNo    Register #
//!    - HCS12 = not used \n
//!    - HCS08 = not used \n
//!    - RS08  = not used \n
//!    - CFV1  = Control register space, see \ref CFV1_CRegisters_t\n
//!    - CFVx  = Control register space, see \ref CFVx_CRegisters_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_WriteCReg(unsigned int regNo, unsigned long regValue);
//! Read Target Control register
//!
//! @param regNo    Register #
//!    - HCS12 = not used \n
//!    - HCS08 = not used \n
//!    - RS08  = not used \n
//!    - CFV1  = Control register space, see \ref CFV1_CRegisters_t\n
//!    - CFVx  = Control register space, see \ref CFVx_CRegisters_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API
USBDM_ErrorCode USBDM_ReadCReg(unsigned int regNo, unsigned long *regValue);
//! Write Target Debug register
//!
//! @param regNo    Register #
//!   - HCS12 = BD memory space, see \ref HCS12_DRegisters_t   \n
//!   - HCS08 = BKPT register, see \ref HCS08_DRegisters_t   \n
//!   - RS08  = BKPT register, see \ref RS08_DRegisters_t   \n
//!   - CFV1  = Debug register space, see \ref CFV1_DRegisters_t\n
//!   - CFVx  = Debug register space, see \ref CFVx_DRegisters_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_WriteDReg(unsigned int regNo, unsigned long regValue);
//! Read Target Debug register
//!
//! @param regNo    Register #
//!   - HCS12 = BD memory space, see \ref HCS12_DRegisters_t   \n
//!   - HCS08 = BKPT register, see \ref HCS08_DRegisters_t   \n
//!   - RS08  = BKPT register, see \ref RS08_DRegisters_t   \n
//!   - CFV1  = Debug register space, see \ref CFV1_DRegisters_t\n
//!   - CFVx  = Debug register space, see \ref CFVx_DRegisters_t
//! @param regValue 8/16/32-bit value
//!
//! @return error code \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode USBDM_ReadDReg(unsigned int regNo, unsigned long *regValue);
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
USBDM_API 
USBDM_ErrorCode  USBDM_WriteMemory( unsigned int  elementSize,
                                              unsigned int  byteCount,
                                              unsigned int  address,
                                              unsigned const char *data);
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
USBDM_API 
USBDM_ErrorCode  USBDM_ReadMemory( unsigned int  elementSize,
                                             unsigned int  byteCount,
                                             unsigned int  address,
                                             unsigned char *data);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// JTAG Entry points
//

//! JTAG - Takes the TAP to \b TEST-LOGIC-RESET state
//! TMS=11111, TDI=00000 or similar
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_Reset(void);

//! JTAG - move the TAP to \b SHIFT-DR or \b SHIFT-IR state
//! SHIFT_DR => TMS=100,  TDI=0 (Actually TMS=0000_0100)
//! SHIFT_IR => TMS=1100, TDI=0 (Actually TMS=0000_1100)
//!
//! @param mode Action, ref \ref JTAG_ExitActions_t \n
//!      - SHIFT_DR - Enter SHIFT-DR \n
//!      - SHIFT_IR - Enter SHIFT-IR \n
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
//! @note - Requires the tap to be initially in \b TEST-LOGIC-RESET or \b RUN-TEST/IDLE
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_SelectShift(unsigned char mode);

//! JTAG - write data to JTAG shift register
//! STAY_SHIFT    => TMS=Nx0,       TDI=NxData
//! EXIT_SHIFT_DR => TMS=Nx0,11100  TDI=NxData,00000
//! EXIT_SHIFT_IR => TMS=Nx0,111100 TDI=NxData,000000
//! EXIT_IDLE     => TMS=Nx0,10     TDI=NxData,00
//!
//!  @param exit action after shift, see \ref JTAG_ExitActions_t \n
//!      - JTAG_STAY_SHIFT    - Remain in SHIFT-DR or SHIFT-IR \n
//!      - JTAG_EXIT_IDLE     - Exit SHIFT-XX to RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_DR - Exit SHIFT-XX & enter SHIFT-DR w/o crossing RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_IR - Exit SHIFT-XX & enter SHIFT-IR w/o crossing RUN-TEST/IDLE
//!
//!  @param bitCount 8-bit count of \b bits to shift in. [>0]\n
//!                  It is not possible to do 0 bits.
//!  @param buffer Pointer to data buffer.  The data is shifted in LSB (last byte) first,
//!                unused bits (if any) are in the MSB (first) byte;
//!  @note Requires the tap to be in \b SHIFT-DR or \b SHIFT-IR state.
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_Write(unsigned char       bitCount,
                                            unsigned char       exit,
                                            const unsigned char *buffer);
//! JTAG - read data from JTAG shift register
//! SHIFT_DR => TMS=Nx0, TDI=0, TDO=NxData (captured)
//!
//!  @param exit action after shift, see \ref JTAG_ExitActions_t \n
//!      - JTAG_STAY_SHIFT    - Remain in SHIFT-DR or SHIFT-IR \n
//!      - JTAG_EXIT_IDLE     - Exit SHIFT-XX to RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_DR - Exit SHIFT-XX & enter SHIFT-DR w/o crossing RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_IR - Exit SHIFT-XX & enter SHIFT-IR w/o crossing RUN-TEST/IDLE
//!      - +JTAG_WRITE_0      - Write 0's when reading - combined with one of above
//!      - +JTAG_WRITE_1      - Write 1's when reading - combined with one of above
//!
//!  @param bitCount 8-bit count of \b bits to shift in. [>0]\n
//!                  It is not possible to do 0 bits.
//!  @param buffer Pointer to buffer for data read out of the device (first bit in LSB of the last byte in the buffer)
//!  @note Requires the tap to be in \b SHIFT-DR or \b SHIFT-IR state.
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_Read( unsigned char bitCount,
                                            unsigned char exit,
                                            unsigned char *buffer);

//! JTAG - read data from JTAG shift register
//! SHIFT_DR => TMS=Nx0, TDI=0, TDO=NxData (captured)
//!
//!  @param exit action after shift, see \ref JTAG_ExitActions_t \n
//!      - JTAG_STAY_SHIFT    - Remain in SHIFT-DR or SHIFT-IR \n
//!      - JTAG_EXIT_IDLE     - Exit SHIFT-XX to RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_DR - Exit SHIFT-XX & enter SHIFT-DR w/o crossing RUN-TEST/IDLE
//!      - JTAG_EXIT_SHIFT_IR - Exit SHIFT-XX & enter SHIFT-IR w/o crossing RUN-TEST/IDLE
//!      - +JTAG_WRITE_0      - Write 0's when reading - combined with one of above
//!      - +JTAG_WRITE_1      - Write 1's when reading - combined with one of above
//!
//!  @param bitCount 8-bit count of \b bits to shift in. [>0]\n
//!                  It is not possible to do 0 bits.
//!  @param inBuffer  Pointer to buffer for data written to the device (first bit in LSB of the last byte in the buffer)
//!  @param outBuffer Pointer to buffer for data read out of the device (first bit in LSB of the last byte in the buffer)
//!  @note Requires the tap to be in \b SHIFT-DR or \b SHIFT-IR state.
//!  @note inbuffer and outbuffer may be the same location
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_ReadWrite( unsigned char bitCount,
                                                 unsigned char exit,
                                                 const unsigned char *outBuffer,
                                                 unsigned char *inBuffer);

//! Execute JTAG Sequence
//!  @param length   - JTAG sequence length.
//!  @param sequence - Pointer to sequence.
//!  @param inLength - Expected length of input data
//!  @param inBuffer - Returned values
//!
//! @return \n
//!     BDM_RC_OK    => OK \n
//!     other        => Error code - see \ref USBDM_ErrorCode
//!
USBDM_API 
USBDM_ErrorCode  USBDM_JTAG_ExecuteSequence(unsigned char       length,
                                                      const unsigned char *sequence,
                                                      unsigned char       inLength,
                                                      unsigned char       *inBuffer);

//=====================================================================================================
// ICP functions

//! Set BDM for ICP mode & immediately reboots - used in BDM mode only
//!
//! The BDM resets in ICP mode after this command
//! @note Communication is lost.
//!
USBDM_API 
void USBDM_RebootToICP(void);

//
// The following functions are available in ICP mode only
//

//! ICP mode - erase BDM Flash memory
//!
//! @param addr    32-bit memory address
//! @param count   number of bytes to erase
//!
//! @return
//!      - == ICP_RC_OK => success \n
//!      - != ICP_RC_OK => fail, see \ref ICP_ErrorCode_t
//!
//! @note Flash memory alignment requirements should be taken into account.  The
//! range erased should be a multiple of the Flash erase block size.
//!
USBDM_API 
ICP_ErrorCode_t  USBDM_ICP_Erase( unsigned int addr,
                                            unsigned int count);
//! ICP mode - program BDM Flash memory
//!
//! @param addr    32-bit memory address
//! @param count   number of bytes to program
//! @param data    Pointer to buffer containing data
//!
//! @return
//!      - == ICP_RC_OK => success \n
//!      - != ICP_RC_OK => fail, see \ref ICP_ErrorCode_t
//!
USBDM_API 
ICP_ErrorCode_t  USBDM_ICP_Program( unsigned int  addr,
                                              unsigned int  count,
                                              unsigned char *data);
//! ICP mode - verify BDM Flash memory
//!
//! @param addr    32-bit memory address
//! @param count   number of bytes to verify
//! @param data    Pointer to buffer containing data
//!
//! @return
//!      - == ICP_RC_OK => success \n
//!      - != ICP_RC_OK => fail, see \ref ICP_ErrorCode_t
//!
USBDM_API 
ICP_ErrorCode_t  USBDM_ICP_Verify( unsigned int  addr,
                                             unsigned int  count,
                                             unsigned char *data);
//! ICP mode - reboot
//!
//! The BDM does a normal reset
//!
//! Used in ICP mode to reset to normal (BDM) mode.
//! @note Communication is lost.  \n
//!       The BDM will do the usual flash validity checks before entering BDM mode so if flash is not correctly 
//!        programmed the BDM may reman in ICP mode.  
//!
USBDM_API 
void USBDM_ICP_Reboot( void );
#endif // USBDM_API

#ifdef COMMANDLINE
void dll_initialize(HINSTANCE _hDLLInst);
void dll_uninitialize(void);
#endif // COMMANDLINE

#if defined __cplusplus
    }
#endif

#endif //_USBDM_API_H_
