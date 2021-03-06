/*
 * Names.c
 *
 *  Created on: 16/02/2010
 *      Author: podonoghue
 */

/*! \file
    \brief Debugging message file

    This file provides mappings from various code numbers to strings.\n
    It is mostly used for debugging messages.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "USBDM_API.h"
#include "Names.h"
#include "ARM_Definitions.h"
#include "Log.h"
#include "DeviceData.h"

//! Obtain a description of the hardware version
//!
//! @return ptr to static string describing the hardware
//!
const char *getHardwareDescription(unsigned int hardwareVersion) {
   //! BDM hardware descriptions
   static const char *hardwareDescriptions[] = {
  /*  0 */  "Reserved",
  /*  1 */  "USBDM-JB16            - BDM for RS08, HCS08, HCS12, CFV1 (JB16DWE)",
  /*  2 */  "TBDML-JB16            - Minimal BDM for HCS08, HCS12 & CFV1 (JB16)",
  /*  3 */  "TBDML-JB16            - Minimal TBDML (Internal version)",
  /*  4 */  "OSBDM-JB16            - Original OSBDM",
  /*  5 */  "TBDML-WITZ            - Witztronics TBDML/OSBDM",
  /*  6 */  "OSBDM-JB16+           - Extended OSBDM (RS08 support)",
  /*  7 */  "USBDM-JMxx-CLD        - BDM for RS08, HCS08, HCS12, CFV1 (JMxxCLD)",
  /*  8 */  "USBDM-JMxx-CLC        - BDM for RS08, HCS08, HCS12, CFV1 (JMxxCLC)",
  /*  9 */  "USBSPYDER08           - SofTec USBSPYDER08",
  /* 10 */  "USBDM_UF32            - BDM for RS08, HCS08, HCS12, CFV1 (UF32PBE)",
  /* 11 */  "USBDM-CF-JS16         - Minimal BDM for DSC & CFVx (JS16CWJ)",
  /* 12 */  "USBDM-CF-JMxx         - BDM for RS08, HCS08, HCS12, CFV1 & CFVx (JMxxCLD)",
  /* 13 */  "USBDM-JS16            - Minimal BDM for HCS08, HCS12 & CFV1 (JS16CWJ)",
  /* 14 */  "USBDM-AXIOM-M56F8006  - Axiom MC56F8006 Demo board",
  /* 15 */  "Custom                - Reserved for User created custom hardware",
  /* 16 */  "USBDM-CF-SER-JS16     - Minimal BDM for DSC,CFVx,ARM-JTAG (JS16CWJ) with serial",
  /* 17 */  "USBDM-SER-JS16        - Minimal BDM for HCS08, HCS12 & CFV1 (JS16CWJ) with serial",
  /* 18 */  "USBDM-CF-SER-JMxx     - Deluxe BDM for RS08,HCS08,HCS12,DSC,CFVx,ARM-JTAG (JMxxCLD)",
  /* 19 */  "USBDM_TWR_KINETIS     - Tower Kinetis boards",
  /* 20 */  "USBDM_TWR_CFV1        - Tower Coldfire V1 boards",
  /* 21 */  "USBDM_TWR_HCS08       - Tower HCS08 boards",
  /* 22 */  "USBDM_TWR_CFVx        - Tower CFVx boards",
  /* 23 */  "USBDM-SWD-SER-JS16    - Minimal BDM for HCS08,HCS12,CFV1,ARM-SWD (JS16CWJ) with serial",
         };
   const char *hardwareName = "Unknown";
   hardwareVersion &= 0x3F; // mask out BDM processor type
   if (hardwareVersion < sizeof(hardwareDescriptions)/sizeof(hardwareDescriptions[0]))
      hardwareName = hardwareDescriptions[hardwareVersion];
   return hardwareName;
}

//! Obtain a short description of the hardware
//!
//! @return ptr to static string describing the hardware
//!
const char *getBriefHardwareDescription(unsigned int hardwareVersion) {
   static const char *briefHardwareDescriptions[] = {
    /*  0  */  "Reserved",
    /*  1  */  "USBDM - (JB16DWE)",
    /*  2  */  "TBDML - Minimal TBDML",
    /*  3  */  "TBDML - No longer used",
    /*  4  */  "OSBDM - Original OSBDM",
    /*  5  */  "Witztronics TBDML/OSBDM",
    /*  6  */  "OSBDM+ - Extended OSBDM",
    /*  7  */  "USBDM - (JMxxCLD)",
    /*  8  */  "USBDM - (JMxxCLC)",
    /*  9  */  "USBSPYDER08",
    /* 10  */  "USBDM - (UF32PBE)",
    /* 11  */  "USBDM-CF - (JS16CWJ)",
    /* 12  */  "USBDM-CF - (JMxxCLD)",
    /* 13  */  "USBDM - (JS16CWJ)",
    /* 14  */  "Axiom MC56F8006Demo",
    /* 15  */  "Custom",
    /* 16  */  "USBDM-CF - (JS16CWJ-V2)",
    /* 17  */  "USBDM-SER -(JS16CWJ-V2)",
    /* 18  */  "USBDM-CF-SER - (JMxxCLD)",
    /* 19  */  "USBDM_TWR_KINETIS ",
    /* 20  */  "USBDM_TWR_CFV1",
    /* 21  */  "USBDM_TWR_HCS08",
    /* 22  */  "USBDM_TWR_CFVx",
    /* 23  */  "USBDM-SWD-SER -(JS16CWJ-V2)",
         };

   const char *hardwareName = "unknown hardware";
   hardwareVersion &= 0x3F; // mask out BDM processor type
   if (hardwareVersion < sizeof(briefHardwareDescriptions)/sizeof(briefHardwareDescriptions[0]))
      hardwareName = briefHardwareDescriptions[hardwareVersion];
   return hardwareName;
}

//! ICP Error message string from Error #
static const char *const ICPerrorMessages[] = {
    "No Error",                           // 0
    "Illegal parameters",                 // 1
    "Flash Operation Failed",             // 2
    "Verify Failed",                      // 3
};

//! \brief Maps an ICP Error Code # to a string
//!
//! @param error Error number
//!
//! @return pointer to static string describing the error
//!
const char *getICPErrorName(unsigned char error) {
   char const *errorName = NULL;

   if (error < sizeof(ICPerrorMessages)/sizeof(ICPerrorMessages[0]))
      errorName = ICPerrorMessages[error];
   if (errorName == NULL)
      errorName = "UNKNOWN";
   return errorName;
}

//! \brief Maps a Target type # to a string
//!
//! @param type = Target type #
//!
//! @return pointer to static string describing the target
//!
char const *getTargetTypeName( unsigned int type ) {
   static const char *names[] = {
      "HCS12","HCS08","RS08","CFV1","CFVx","JTAG","EZFlash","MC56F80xx","ARM-JTAG","ARM-SWD"
      };
   const char *typeName = NULL;
   static char unknownBuffer[10];

   if (type < sizeof(names)/sizeof(names[0]))
      typeName = names[type];
   else if (type == T_OFF)
      typeName = "OFF";

   if (typeName == NULL) {
      typeName = unknownBuffer;
      snprintf(unknownBuffer, 8, "%d", type);
   }
   return typeName;
}

char const *getVoltageStatusName(TargetVddState_t level) {
   switch (level) {
      case BDM_TARGET_VDD_NONE : return "Vdd-None";
      case BDM_TARGET_VDD_INT  : return "Vdd-Internal";
      case BDM_TARGET_VDD_EXT  : return "Vdd-External";
      case BDM_TARGET_VDD_ERR  : return "Vdd-Overload";
      default  :                 return "Vdd-??";
   }
}

char const *getConnectionStateName(SpeedMode_t level) {
   switch (level) {
      case SPEED_NO_INFO         : return "No-connection";
      case SPEED_SYNC            : return "Speed-sync";
      case SPEED_GUESSED         : return "Speed-guessed";
      case SPEED_USER_SUPPLIED   : return "Speed-user-supplied";
      default                    : return "Speed-??";
   }
}

//! \brief Maps the BDM status to text
//!
//! @param USBDMStatus => status value to interpret
//!
//! @return pointer to static string buffer describing the value
//!
char const *getBDMStatusName(USBDMStatus_t *USBDMStatus) {
static char buff[150] = "";

      snprintf(buff, sizeof(buff), "%s, %s, %s, %s, %s, %s, %s",
              USBDMStatus->ackn_state?"Ackn":"Wait",
              getConnectionStateName(USBDMStatus->connection_state),
              getVppSelectName(USBDMStatus->flash_state),
              getVoltageStatusName(USBDMStatus->power_state),
              USBDMStatus->reset_state?"RSTO=1":"RSTO=0",
              USBDMStatus->reset_recent?"Reset":"No Reset",
              USBDMStatus->halt_state?"CFVx-halted":"CFVx-running");
      return buff;
}

#ifdef LOG
//! Command String from Command #
static const char *const newCommandTable[]= {
   "CMD_USBDM_GET_COMMAND_RESPONSE"    , // 0
   "CMD_USBDM_SET_TARGET"              , // 1
   "CMD_USBDM_SET_VDD"                 , // 2
   "CMD_USBDM_DEBUG"                   , // 3
   "CMD_USBDM_GET_BDM_STATUS"          , // 4
   "CMD_USBDM_GET_CAPABILITIES"        , // 5
   "CMD_USBDM_SET_OPTIONS"             , // 6
   NULL                                , // 7
   "CMD_USBDM_CONTROL_PINS"            , // 8
   NULL                                , // 9
   NULL                                , // 10
   NULL                                , // 11
   "CMD_USBDM_GET_VER"                 , // 12
   NULL                                , // 13
   "CMD_USBDM_ICP_BOOT"                , // 14

   "CMD_USBDM_CONNECT"                 , // 15
   "CMD_USBDM_SET_SPEED"               , // 16
   "CMD_USBDM_GET_SPEED"               , // 17

   "CMD_USBDM_CONTROL_INTERFACE"       , // 18
   NULL                                , // 19

   "CMD_USBDM_READ_STATUS_REG"         , // 20
   "CMD_USBDM_WRITE_CONROL_REG"        , // 21

   "CMD_USBDM_TARGET_RESET"            , // 22
   "CMD_USBDM_TARGET_STEP"             , // 23
   "CMD_USBDM_TARGET_GO"               , // 24
   "CMD_USBDM_TARGET_HALT"             , // 25

   "CMD_USBDM_WRITE_REG"               , // 26
   "CMD_USBDM_READ_REG"                , // 27

   "CMD_USBDM_WRITE_CREG"              , // 28
   "CMD_USBDM_READ_CREG"               , // 29

   "CMD_USBDM_WRITE_DREG"              , // 30
   "CMD_USBDM_READ_DREG"               , // 31

   "CMD_USBDM_WRITE_MEM"               , // 32
   "CMD_USBDM_READ_MEM"                , // 33

   "CMD_USBDM_TRIM_CLOCK - removed"          , // 34
   "CMD_USBDM_RS08_FLASH_ENABLE - removed"   , // 35
   "CMD_USBDM_RS08_FLASH_STATUS - removed"   , // 36
   "CMD_USBDM_RS08_FLASH_DISABLE - removed"  , // 37

   "CMD_USBDM_JTAG_GOTORESET"          , // 38
   "CMD_USBDM_JTAG_GOTOSHIFT"          , // 39
   "CMD_USBDM_JTAG_WRITE"              , // 40
   "CMD_USBDM_JTAG_READ"               , // 41
   "CMD_USBDM_SET_VPP"                 , // 42,
   "CMD_USBDM_JTAG_READ_WRITE"         , // 43,
   "CMD_USBDM_JTAG_EXECUTE_SEQUENCE"   , // 44,
};

//! \brief Maps a command code to a string
//!
//! @param command Command number
//!
//! @return pointer to static string describing the command
//!
const char *getCommandName(unsigned char command) {
   char const *commandName = NULL;

   command &= ~0x80;

//   if (bdmState.compatibilityMode != C_USBDM_V2) {
//      if (command < sizeof(oldCommandTable)/sizeof(oldCommandTable[0]))
//         commandName = oldCommandTable[command];
//   }
//   else {
      if (command < sizeof(newCommandTable)/sizeof(newCommandTable[0]))
         commandName = newCommandTable[command];
//   }
   if (commandName == NULL)
      commandName = "UNKNOWN";
   return commandName;
}

//! Debug command string from code
static const char *const debugCommands[] = {
   "ACKN",                        // 0
   "SYNC",                        // 1
   "Test Port",                   // 2
   "USB Disconnect",              // 3
   "Find Stack size",             // 4
   "Vpp Off",                     // 5
   "Vpp On",                      // 6
   "Flash 12V Off",               // 7
   "Flash 12V On",                // 8
   "Vdd Off",                     // 9
   "Vdd 3.3V On",                 // 10
   "Vdd 5V On",                   // 11
   "Cycle Vdd",                   // 12
   "Measure Vdd",                 // 13
   "Measure RS08 Trim - deleted", // 14
   "Test WAITS",                  // 15 //!< - Tests the software counting delays used for BDM communication. (locks up BDM!)
   "Test ALT Speed",              // 16
   "Test BDM Tx Routine",         // 17
   "SWD test",                    // 18
};

//! \brief Maps a Debug Command # to a string
//!
//! @param cmd Debug command number
//!
//! @return pointer to static string describing the command
//!
const char *getDebugCommandName(unsigned char cmd) {
   char const *cmdName = NULL;
   if (cmd < sizeof(debugCommands)/sizeof(debugCommands[0]))
      cmdName = debugCommands[cmd];
   if (cmdName == NULL)
      cmdName = "UNKNOWN";
   return cmdName;
}

char const *getAutoConnectName(AutoConnect_t mode) {
   static char buff[40] = "";
   switch(mode) {
   case AUTOCONNECT_NEVER  : strcpy(buff,"NEVER");      break;
   case AUTOCONNECT_ALWAYS : strcpy(buff,"ALWAYS");      break;
   case AUTOCONNECT_STATUS : strcpy(buff,"STATUS");      break;
   }
   return buff;
}

char const *getConnectionRetryName(RetryMode mode) {
   static char buff[150] = "";

   switch (mode & retryMask) {
   case retryAlways     : strcpy(buff,"ALWAYS");      break;
   case retryNever      : strcpy(buff,"NEVER");       break;
   case retryNotPartial : strcpy(buff,"NOTPARTIAL");  break;
   default              : strcpy(buff,"UNKNOWN!!!");  break;
   }
   if (mode & retryWithInit) {
      strcat(buff,"+INIT");
   }
   if (mode & retryByPower) {
      strcat(buff,"+POWER");
   }
   if (mode & retryByReset) {
      strcat(buff,"+RESET");
   }
   return buff;
}

//! \brief Maps a Coldfire V1 Control register # to a string
//! (As used by WRITE_CREG/READ_CREG)
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the register
//!
char const *getCFV1ControlRegName( unsigned int regAddr ){
   static const char *names[] = {
      "_A7","VBR","CPUCR",NULL,NULL,NULL,NULL,NULL,
      NULL,NULL,NULL,NULL,NULL,NULL,"SR","PC",
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      };
   const char *regName = NULL;

   regAddr &= 0x1F; // CRN field is only 5 bits
   regName = names[regAddr];
   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a Coldfire V2,3,4 Control register # to a string
//! (As used by RCREG/WCREG commands)
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the register
//!
char const *getCFVxControlRegName( unsigned int regAddr ){
static const char *regs[] = {"D0","D1","D2","D3","D4","D5","D6","D7",
                             "A0","A1","A2","A3","A4","A5","A6","USP"};
static const char *regs1[] = {NULL,NULL,"CACR","ASID","ACR0","ACR1","ACR2","ACR3",
                              "MMUBAR"};
static const char *regs2[] = {"SSP","VBR",NULL,NULL,"MACSR","MASK","ACC",NULL,
                               NULL,NULL,NULL,NULL,NULL,NULL,"SR","PC"};
const char *regName = NULL;

   if (regAddr <= 8) // Register Misc (Some targets only)
      regName = regs1[regAddr];
   else if ((regAddr >= 0x080) && (regAddr <= 0x08F)) // Register D0-A7
      regName = regs[regAddr-0x80];
   else if ((regAddr >= 0x180) && (regAddr <= 0x18F)) // Register D0-A7
      regName = regs[regAddr-0x180];
   else if ((regAddr >= 0x800) && (regAddr <= 0x80F)) // Register Misc
      regName = regs2[regAddr-0x800];
   else if (regAddr == 0xC04)
      regName = "FLASHBAR";
   else if (regAddr == 0xC05)
      regName = "RAMBAR";
   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a ARM-SWD Debug register # to a string
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getARMControlRegName( unsigned int regAddr ) {
   //! The regAddr is actually a AP bus address as follows:
   //!    A[31:24]  => DP-AP-SELECT[31:24] (AP # Select) \n
   //!    A[23:8]   => unused (0)
   //!    A[7:4]    => DP-AP-SELECT[7:4]   (Bank select within AP) \n
   //!    A[3:2]    => APACC[3:2]          (Register select within AP bank)
   //!    A[1:0]    => unused (0)
   //!
   switch(regAddr) {
      // AP#0 - Common ARM AHB-AP
      case ARM_CRegAHB_AP_CSW     : return "AHB_AP.CSW";  // AHB-AP Control/Status Word register
      case ARM_CRegAHB_AP_TAR     : return "AHB_AP.TAR";  // AHB-AP Transfer Address register
      case ARM_CRegAHB_AP_DRW     : return "AHB_AP.DRW";  // AHB-AP Data Read/Write register

      case ARM_CRegAHB_AP_CFG     : return "AHB_AP.CFG";  // AHB-AP Config register
      case ARM_CRegAHB_AP_Base    : return "AHB_AP.Base"; // AHB-AP IDebug base address register
      case ARM_CRegAHB_AP_Id      : return "AHB_AP.Id";   // AHB-AP ID Register

      // AP#1 - Kinetis MDM-AP registers
      case ARM_CRegMDM_AP_Status  : return "MDM_AP.Status";  //!< Status register
      case ARM_CRegMDM_AP_Control : return "MDM_AP.Control"; //!< Control register
      case ARM_CRegMDM_AP_Ident   : return "MDM_AP.Ident";   //!< Identifier register (should read 0x001C_0000)
      default: break;
   };
   static char buff[100];
   snprintf(buff, sizeof(buff), "0x%08X (AP#0x%02X:B#0x%1X:R#%d)",
         regAddr, (regAddr>>24)&0xFF, (regAddr>>4)&0x0F, (regAddr>>2)&0x2);
   return buff;
}

//! \brief Maps a Coldfire V1 Debug register # to a string
//! (As used by WRITE_DREG/READ_DREG)
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getCFV1DebugRegName( unsigned int regAddr ){
   static const char *names[] = {
     "CSR","XCSR","CSR2","CSR3",// 00-03
     NULL,"BAAR","AATR","TDR",  // 04-07
     "PBR0","PBMR",NULL,NULL,   // 08-0B
     "ABHR","ABLR","DBR","DBMR",// 0C-0F
     NULL,NULL,NULL,NULL,       // 10-13
     NULL,NULL,NULL,NULL,       // 14-17
     "PBR1",NULL,"PBR2","PBR3", // 18-1B
	 NULL,NULL,NULL,NULL,
                                };
   static const char *names2[] = {
      "CSR.byte","XCSR.byte","CSR2.byte","CSR3.byte",
      };
   const char *regName = NULL;

   if (regAddr > CFV1_ByteRegs) {// Byte access?
      regAddr -= CFV1_ByteRegs;
      if (regAddr < sizeof(names2)/sizeof(names2[0]))
          regName = names2[regAddr];
   }
   else {
      if (regAddr < sizeof(names)/sizeof(names[0]))
          regName = names[regAddr];
   }
   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a Coldfire V2,3,4 Debug register # to a string
//! (As used by WDMREG)/RDMREG))
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getCFVxDebugRegName( unsigned int regAddr ){
   static const char *names[] = {"CSR", NULL, NULL, NULL,   // 00-03
                                 NULL,"BAAR","AATR","TDR",  // 04-07
                                 "PBR0","PBMR",NULL,NULL,   // 08-0B
                                 "ABHR","ABLR","DBR","DBMR",// 0C-0F
                                 NULL,NULL,NULL,NULL,       // 10-13
                                 NULL,NULL,NULL,NULL,       // 14-17
                                 "PBR1",NULL,"PBR2","PBR3", // 18-1B
                                };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a ARM-SWD Debug register # to a string
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getSWDDebugRegName( unsigned int regAddr ) {
   static const char *names[] = {
   "DP_IDCODE",    //!< IDCODE reg - read only
   "DP_STATUS",    //!< STATUS reg - read only
   "DP_RESEND",    //!< RESEND reg - read only
   "DP_RDBUFF",    //!< RDBUFF reg - read only
   "DP_ABORT",     //!< IDCODE reg - write only
   "DP_CONTROL",   //!< IDCODE reg - write only
   "DP_SELECT",    //!< IDCODE reg - write only
   };
   if (regAddr>=(sizeof(names)/sizeof(names[0]))) {
      return "DP_Illegal register";
   }
   else {
      return names[regAddr];
   }
}

//! \brief Maps a Coldfire V2,3,4 Debug register # to a string
//! (As used by WDMREG)/RDMREG))
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getHCS12DebugRegName( unsigned int regAddr ) {
   if (regAddr == 0xFF01)
      return "BDMSTS";
   if (regAddr == 0xFF06)
      return "BDMCCR";
   if (regAddr == 0xFF07)
      return "BDMCCRH";
   if (regAddr == 0xFF08)
      return "BDMGPR";

   return "Unknown";
}

//! \brief Maps a HCS12 register # to a string
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getHCS12RegName( unsigned int regAddr ) {
   static const char *names[] = {
      NULL,NULL,NULL,"PC","D","X","Y","SP"
      };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a HCS08 register # to a string
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getHCS08RegName( unsigned int regAddr ) {
   static const char *names[] = {
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      "A","CCR",NULL,"PC","HX",NULL,NULL,"SP"
      };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a RS08 register # to a string
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getRS08RegName( unsigned int regAddr ) {
   static const char *names[] = {
      NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
      "A",NULL,NULL,"CCR_PC",NULL,NULL,NULL,"SPC"
      };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a Coldfire V1 register # to a string
//! (As used by WRITE_Rn/READ_Rn)
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getCFV1RegName( unsigned int regAddr ){
   static const char *names[] = {
      "D0","D1","D2","D3","D4","D5","D6","D7",
      "A0","A1","A2","A3","A4","A5","A6","USP",
      "PST0","PST1","PST2","PST3","PST4","PST5",
      "PST6","PST7","PST8","PST9","PST10","PST11",
      };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

//! \brief Maps a Coldfire V2,3,4 register # to a string
//! (As used by WAREG/RAREG,WDREG/RDREG)
//!
//! @param regAddr = register address
//!
//! @return pointer to static string describing the command
//!
char const *getCFVxRegName( unsigned int regAddr ){
   static const char *names[] = {
      "D0","D1","D2","D3","D4","D5","D6","D7",
      "A0","A1","A2","A3","A4","A5","A6","USP",
      };
   const char *regName = NULL;

   if (regAddr < sizeof(names)/sizeof(names[0]))
       regName = names[regAddr];

   if (regName == NULL)
      regName = "unknown";

   return regName;
}

char const *getDSCRegName( unsigned int regNum) {
   static const char *const regNames[] =  {
      // Core registers
      "DSC_RegX0",
      "DSC_RegY0",
      "DSC_RegY1",
      "DSC_RegA0",
      "DSC_RegA1",
      "DSC_RegA2",
      "DSC_RegB0",
      "DSC_RegB1",
      "DSC_RegB2",
      "DSC_RegC0",
      "DSC_RegC1",                                  // 10
      "DSC_RegC2",
      "DSC_RegD0",
      "DSC_RegD1",
      "DSC_RegD2",
      "DSC_RegOMR",
      "DSC_RegSR",
      "DSC_RegLA",
      "DSC_RegLA2", /* read only */
      "DSC_RegLC",
      "DSC_RegLC2", /* read only */                 //  20
      "DSC_RegHWS0",
      "DSC_RegHWS1",
      "DSC_RegSP",
      "DSC_RegN3",
      "DSC_RegM01",
      "DSC_RegN",
      "DSC_RegR0",
      "DSC_RegR1",
      "DSC_RegR2",
      "DSC_RegR3",                                  // 30
      "DSC_RegR4",
      "DSC_RegR5",
      "DSC_RegsHM01",
      "DSC_RegsHN",
      "DSC_RegsHR0",
      "DSC_RegsHR1",
      "DSC_RegPC",
      // JTAG registers
      "DSC_RegIDCODE",                              // JTAG Core IDCODE
      // ONCE registers
      "DSC_RegOCR",                                 // OnCE Control register
      "DSC_RegOSCNTR",                              // Once Instruction Step Counter
      "DSC_RegOSR",                                 // OnCE Status register
      "DSC_RegOPDBR",                               // OnCE Program Data Bus Register
      "DSC_RegOBASE",                               // OnCE Peripheral Base Address regitsre
      "DSC_RegOTXRXSR",                             // OnCE Tx & Rx Status & Control register
      "DSC_RegOTX",                                 // Once Transmit register
      "DSC_RegOTX1",                                // Once Transmit register
      "DSC_RegORX",                                 // Once Receive register
      "DSC_RegORX1",                                // Once Receive register
      "DSC_RegOTBCR",                               // OnCE Trace buffer control register
      "DSC_RegOTBPR",                               // OnCE Trace Buffer Pointer register
      "DSC_RegOTB",                                 // Trace Buffer Register Stages
      "DSC_RegOB0CR",                               // Breakpoint Unit 0 Control register
      "DSC_RegOB0AR1",                              // Breakpoint Unit 0 Address register 1
      "DSC_RegOB0AR2",                              // Breakpoint Unit 0 Address register 2
      "DSC_RegOB0MSK",                              // Breakpoint Unit 0 Mask register
      "DSC_RegOB0CNTR",                             // Breakpoint Unit 0 Counter
   } ;
   const char *regName = NULL;

   if (regNum < sizeof(regNames)/sizeof(regNames[0]))
       regName = regNames[regNum];
   if (regName == NULL)
      regName = "unknown";
   return regName;
}

//! \brief Maps a register # to a string
//!
//! @param targetType = target type (T_HC12 etc)
//! @param regNo      = register address
//!
//! @return pointer to static string describing the command
//!
char const *getRegName( unsigned int targetType,
                        unsigned int regNo ){
   switch (targetType) {
      case T_HC12 :
         return getHCS12RegName(regNo);
      case T_HCS08 :
         return getHCS08RegName(regNo);
      case T_RS08 :
         return getRS08RegName(regNo);
         break;
      case T_CFV1 :
         return getCFV1RegName(regNo);
      case T_CFVx :
         return getCFVxRegName(regNo);
      case T_ARM_JTAG :
         return getARMRegName(regNo);
      case T_ARM_SWD :
         return getARMRegName(regNo);
      case T_MC56F80xx:
         return getDSCRegName(regNo);
   };
   return "Invalid target!";
}

//! \brief Maps a CFVx CSR register value to a string
//!
//! @param CSR = CSR register value
//!
//! @return pointer to static string buffer describing the CSR
//!
char const *getCFVx_CSR_Name( unsigned int CSR) {
   static const char *hrlName[] = {
      "A","B","C","D","?","?","?","?",
      "?","B+","?","D+","?","?","?","?"
      };
   static const char *bstatMode[] = {
      "No bkpt enabled",
      "Waiting for level-1 bkpt",
      "Level-1 bkpt triggered",
      "-?-",
      "-?-",
      "Waiting for level-2 bkpt",
      "Level-2 bkpt triggered",
      "-?-", "-?-", "-?-", "-?-",
      "-?-", "-?-", "-?-", "-?-",
      "-?-", "-?-",
      };
   static char buff[100];

   snprintf(buff, sizeof(buff), "%s, %s%s%s%sHRL=%s, %s%s%s%s%s%s%s%s%s",
               bstatMode[(CSR>>28)&0xF],
               (CSR&(1<<27))?"FOF, ":"",
               (CSR&(1<<26))?"TRG, ":"",
               (CSR&(1<<25))?"HALT, ":"",
               (CSR&(1<<24))?"BKPT, ":"",
               hrlName[(CSR>>20)&0xF],
               (CSR&(1<<17))?"PCD, ":"",
               (CSR&(1<<16))?"IPW, ":"",
               (CSR&(1<<15))?"MAP, ":"",
               (CSR&(1<<14))?"TRC, ":"",
               (CSR&(1<<13))?"EMU, ":"",
               (CSR&(1<<10))?"UHE, ":"",
               (CSR&(1<<6))?"NPL, ":"",
               (CSR&(1<<5))?"IPI, ":"",
               (CSR&(1<<4))?"SSM, ":""
               );
   return buff;
}

//! \brief Maps a HCS12 BDMSTS register value to a string
//!
//! @param BDMSTS = BDMSTS register value
//!
//! @return pointer to static string buffer describing the BDMSTS
//!
char const *getHCS12_BDMSTS_Name( unsigned int BDMSTS) {
   static char buff[100];

   snprintf(buff, sizeof(buff), "(0x%2.2X) = %s%s%s%s%s%s%s",
                  BDMSTS,
                 (BDMSTS&(1<<7))?"ENBDM, ":"",
                 (BDMSTS&(1<<6))?"BDACT, ":"",
                 (BDMSTS&(1<<5))?"ENTAG, ":"",
                 (BDMSTS&(1<<4))?"SDV, ":"",
                 (BDMSTS&(1<<3))?"TRACE, ":"",
                 (BDMSTS&(1<<2))?"CLKSW, ":"",
                 (BDMSTS&(1<<1))?"UNSEC, ":"" );
   return buff;
}

//! \brief Maps a HCS08 BDCSCR register value to a string
//!
//! @param BDCSCR = BDCSCR register value
//!
//! @return pointer to static string buffer describing the BDCSCR
//!
char const *getHCS08_BDCSCR_Name( unsigned int BDCSCR) {
   static char buff[100];

   snprintf(buff, sizeof(buff), "(0x%2.2X) = %s%s%s%s%s%s%s%s",
                  BDCSCR,
                 (BDCSCR&(1<<7))?"ENBDM, ":"",
                 (BDCSCR&(1<<6))?"BDACT, ":"",
                 (BDCSCR&(1<<5))?"BKPTEN, ":"",
                 (BDCSCR&(1<<4))?"FTS, ":"",
                 (BDCSCR&(1<<3))?"CLKSW, ":"",
                 (BDCSCR&(1<<2))?"WS, ":"",
                 (BDCSCR&(1<<1))?"WSF, ":"",
                 (BDCSCR&(1<<0))?"DVF, ":"" );
   return buff;
}

//! \brief Maps a RS08 BDCSCR register value to a string
//!
//! @param BDCSCR = BDCSCR register value
//!
//! @return pointer to static string buffer describing the BDCSCR
//!
char const *getRS08_BDCSCR_Name( unsigned int BDCSCR) {
static char buff[100];
   snprintf(buff, sizeof(buff), "(0x%2.2X) = %s%s%s%s%s%s%s",
                  BDCSCR,
                 (BDCSCR&(1<<7))?"ENBDM, ":"",
                 (BDCSCR&(1<<6))?"BDACT, ":"",
                 (BDCSCR&(1<<5))?"BKPTEN, ":"",
                 (BDCSCR&(1<<4))?"FTS, ":"",
                 (BDCSCR&(1<<3))?"CLKSW, ":"",
                 (BDCSCR&(1<<2))?"WS, ":"",
                 (BDCSCR&(1<<1))?"WSF, ":"" );
   return buff;
}

//! \brief Maps a CFV1 XCSR register MSB value to a string
//!
//! @param XCSR = XCSR register value
//!
//! @return pointer to static string buffer describing the XCSR
//!
char const *getCFV1_XCSR_Name( unsigned int XCSR) {
   static char buff[100];
   static const char *mode[] = {"RUN,", "STOP,", "HALT,", "???,"};

   snprintf(buff, sizeof(buff), "(0x%2.2X) = %s%s%s%s%s",
                 XCSR,
                 mode[(XCSR>>6)&0x3],
                 (XCSR&(1<<5))?"OVERRUN, ":(XCSR&(1<<4))?"ILLEGAL, ":(XCSR&(1<<3))?"INVALID, ":"DONE/MASS,",
                 (XCSR&(1<<2))?"CLKSW, ":"",
                 (XCSR&(1<<1))?"SEC/ERASE, ":"",
                 (XCSR&(1<<0))?"ENBDM, ":"" );
   return buff;
}

//! \brief Maps a Status register value to a string
//!
//! @param targetType = Type of target (T_HC12 etc)
//! @param XCSR       = XCSR register value
//!
//! @return pointer to static string buffer describing the XCSR
//!
char const *getStatusRegName(unsigned int targetType, unsigned int value) {

   switch (targetType) {
      case T_HC12:
         return getHCS12_BDMSTS_Name(value);
      case T_HCS08:
         return getHCS08_BDCSCR_Name(value);
      case T_RS08:
         return getRS08_BDCSCR_Name(value);
      case T_CFV1:
         return getCFV1_XCSR_Name(value);
      case T_CFVx:
         return getCFVx_CSR_Name(value);
      default:
         return "Illegal Target!";
   }
}

//! \brief Maps a Capability vector to Text
//!
//! @param capability => capability vector
//!
//! @return pointer to static string buffer describing the XCSR
//!
const char *getCapabilityName(unsigned int capability) {
unsigned index;
static char buff[250] = "";
static const char *capabilityTable[] = {
                                  "HCS12|",
                                  "RS08|",
                                  "VDDCONTROL|",
                                  "VDDSENSE|",

                                  "CFVx|",
                                  "HCS08|",
                                  "CFV1|",
                                  "JTAG|",

                                  "DSC|",
                                  "ARM_JTAG|",
                                  "RST|",
                                  "PST|",
                                  "CDC|",
                                  "ARM_SWD|",
                               };
   sprintf(buff, "(0x%4.4X) ", capability);
   for (index=0;
        index<sizeof(capabilityTable)/sizeof(capabilityTable[0]);
         index++) {
      if ((capability&(1<<index)) != 0) {
         strcat(buff,capabilityTable[index]);
      }
   }

   return buff;
}

char const *getTargetModeName(TargetMode_t type) {
static char buff[100] = "";
static const char *resetMethod[] = {"ALL",
                                    "HARDWARE",
                                    "SOFTWARE",
                                    "POWER",
                                    "Illegal",
                                    "Illegal",
                                    "Illegal",
                                    "DEFAULT",
                                    };
static const char *resetMode[] = {"SPECIAL",
                                  "NORMAL",
                                  "Illegal",
                                  "Illegal",
                                  };
         snprintf(buff, sizeof(buff), "0x%02X(%s, %s)",
                             type,
                             resetMethod[(type&RESET_METHOD_MASK)>>2],
                             resetMode[type&RESET_MODE_MASK]
                             );
         return buff;
}

char const *getPinLevelName(PinLevelMasks_t level) {
static char buff[100];

   buff[0] = '\0';

   if (level == -1) {
      return "Release";
   }
   switch (level & PIN_BKGD) {
      case PIN_BKGD_3STATE : strcat(buff, "PIN_BKGD_3STATE|"); break;
      case PIN_BKGD_HIGH   : strcat(buff, "PIN_BKGD_HIGH|");   break;
      case PIN_BKGD_LOW    : strcat(buff, "PIN_BKGD_LOW|");    break;
   }
   switch (level & PIN_RESET) {
      case PIN_RESET_3STATE : strcat(buff, "PIN_RESET_3STATE|"); break;
      case PIN_RESET_LOW    : strcat(buff, "PIN_RESET_LOW|");    break;
      case PIN_RESET_NC     : break;
      default               : strcat(buff, "PIN_RESET_??|");    break;
   }
   switch (level & PIN_TA) {
      case PIN_TA_3STATE : strcat(buff, "PIN_TA_3STATE|");   break;
      case PIN_TA_LOW    : strcat(buff, "PIN_TA_LOW|");    break;
      case PIN_TA_NC     : break;
      default            : strcat(buff, "PIN_TA_??|");    break;
   }
   switch (level & PIN_TRST) {
      case PIN_TRST_3STATE : strcat(buff, "PIN_TRST_3STATE|"); break;
      case PIN_TRST_LOW    : strcat(buff, "PIN_TRST_LOW|");   break;
      case PIN_TRST_NC     : break;
      default              : strcat(buff, "PIN_TRST_??|");    break;
   }
   switch (level & PIN_BKPT) {
      case PIN_BKPT_3STATE : strcat(buff, "PIN_BKPT_3STATE|"); break;
      case PIN_BKPT_LOW    : strcat(buff, "PIN_BKPT_LOW|");    break;
      case PIN_BKPT_NC     : break;
      default              : strcat(buff, "PIN_BKPT_??|");    break;
   }
   switch (level & PIN_SWD) {
      case PIN_SWD_3STATE : strcat(buff, "PIN_SWD_3STATE|"); break;
      case PIN_SWD_HIGH   : strcat(buff, "PIN_SWD_HIGH|");   break;
      case PIN_SWD_LOW    : strcat(buff, "PIN_SWD_LOW|");    break;
   }
   return buff;
}

char const *getControlLevelName(InterfaceLevelMasks_t level) {
static char buff[100];

   if (level == SI_DISABLE)
      return "SI_DISABLE";

   sprintf(buff, "(0x%4.4X) ", level);

   switch (level & SI_BKGD) {
      case SI_BKGD_3STATE : strcat(buff, "SI_BKGD_3STATE|"); break;
      case SI_BKGD_HIGH   : strcat(buff, "SI_BKGD_HIGH|");   break;
      case SI_BKGD_LOW    : strcat(buff, "SI_BKGD_LOW|");    break;
   }
   switch (level & SI_TRST) {
      case SI_TRST_3STATE : strcat(buff, "SI_TRST_3STATE|"); break;
      case SI_TRST_LOW    : strcat(buff, "SI_TRST_LOW|");    break;
   }
   switch (level & SI_RESET) {
      case SI_RESET_3STATE : strcat(buff, "SI_RESET_3STATE|"); break;
      case SI_RESET_LOW    : strcat(buff, "SI_RESET_LOW|");    break;
   }
   switch (level & SI_TA) {
      case SI_TA_3STATE : strcat(buff, "SI_TA_HIGH|");   break;
      case SI_TA_LOW    : strcat(buff, "SI_TA_LOW|");    break;
   }

   return buff;
}

char const *getVoltageSelectName(TargetVddSelect_t level) {
   switch (level) {
      case BDM_TARGET_VDD_OFF      : return "Vdd-Off";
      case BDM_TARGET_VDD_3V3      : return "Vdd-3V3";
      case BDM_TARGET_VDD_5V       : return "Vdd-5V";
      case BDM_TARGET_VDD_ENABLE   : return "Vdd-Enable";
      case BDM_TARGET_VDD_DISABLE  : return "Vdd-Disable";
      default :                      return "Vdd-??";
   }
}

char const *getVppSelectName(TargetVppSelect_t level) {
   switch (level) {
      case BDM_TARGET_VPP_OFF       : return "Vpp-Off";
      case BDM_TARGET_VPP_STANDBY   : return "Vpp-Standby";
      case BDM_TARGET_VPP_ON        : return "Vpp-On";
      case BDM_TARGET_VPP_ERROR     : return "Vpp-Error";
      default :                       return "Vpp-??";
   }
}

char const *getClockSelectName(ClkSwValues_t level) {
   switch (level) {
      case CS_DEFAULT     : return "Clock-Default";
      case CS_ALT_CLK     : return "Clock-Alt";
      case CS_NORMAL_CLK  : return "Clock-Normal";
      default :         return "Clock-??";
   }
}

//! Map JTAG Exit action codes to strings
//!
const char *getExitAction(int action) {
static char buff[100];
const char *exitAction;
const char *fillMode;

   switch (action&JTAG_EXIT_ACTION_MASK) {
      case JTAG_STAY_SHIFT :
         exitAction =  "JTAG_STAY_SHIFT-DR/IR";
         break;
      case JTAG_EXIT_SHIFT_DR :
         exitAction =  "JTAG_EXIT_SHIFT_DR";
         break;
      case JTAG_EXIT_SHIFT_IR :
         exitAction =  "JTAG_EXIT_SHIFT_IR";
         break;
      case JTAG_EXIT_IDLE :
         exitAction =  "JTAG_EXIT_RUN_TEST/IDLE";
         break;
      default:
         exitAction = "JTAG_EXIT_?";
         break;
   }
   switch (action&JTAG_WRITE_MASK) {
      case JTAG_WRITE_0 :
         fillMode =  "";//"|JTAG_WRITE_0";
         break;
      case JTAG_WRITE_1 :
         fillMode =  "|JTAG_WRITE_1";
         break;
      default:
         fillMode = "|JTAG-WRITE_?";
         break;
   }
   snprintf(buff, sizeof(buff), "%s%s", exitAction, fillMode);
   return buff;
}

//! Print bdm option structure to log file
//!
//! @param options - options to report
//!
void printBdmOptions(const USBDM_ExtendedOptions_t *options) {
   print("========================================\n"
         "autoReconnect         => %s\n"
         "bdmClockSource        => %s\n"
         "cycleVddOnConnect     => %s\n"
         "cycleVddOnReset       => %s\n"
         "guessSpeed            => %s\n"
         "interfaceFrequency    => %d kHz\n"
         "leaveTargetPowered    => %s\n"
         "maskInterrupts        => %s\n"
         "powerOffDuration      => %d ms\n"
         "powerRecoveryInterval => %d ms\n"
         "resetDuration         => %d ms\n"
         "resetReleaseInterval  => %d ms\n"
         "resetRecoveryInterval => %d ms\n"
         "size                  => %d\n"
         "targetType            => %s\n"
         "targetVdd             => %s\n"
         "usePSTSignals         => %s\n"
         "useResetSignal        => %s\n"
         "========================================\n",
         getAutoConnectName(options->autoReconnect),
         getClockSelectName(options->bdmClockSource),
         options->cycleVddOnConnect?"T":"F",
         options->cycleVddOnReset?"T":"F",
         options->guessSpeed?"T":"F",
         options->interfaceFrequency,
         options->leaveTargetPowered?"T":"F",
         options->maskInterrupts?"T":"F",
         options->powerOffDuration,
         options->powerOnRecoveryInterval,
         options->resetDuration,
         options->resetReleaseInterval,
         options->resetRecoveryInterval,
         options->size,
         getTargetTypeName(options->targetType),
         getVoltageSelectName(options->targetVdd),
         options->usePSTSignals?"T":"F",
         options->useResetSignal?"T":"F"
         );
}

//! \brief Maps a ARM register # to a string
//!
//! @param regAddr = register number
//!
//! @return pointer to static string describing the register
//!
char const *getARMRegName( unsigned int regAddr ) {
static const char *regs[] = {"R0","R1","R2","R3","R4","R5","R6","R7",
                             "R8","R9","R10","R11","R12","SP","LR","PC",
                             "PSR","MSP","PSP",
                             "MISC/PRIMASK", "FAULTMASK", "BASEPRI", "CONTROL"};
const char *regName = NULL;

   if (regAddr < sizeof(regs)/sizeof(regs[0]))
      regName = regs[regAddr];
   else if (regAddr==ARM_RegFPSCR) {
      regName = "FPSCR";
   }
   else if ((regAddr>=ARM_RegFPS0) && (regAddr<=ARM_RegFPS0+31)) {
      regName = "FPSn";
   }
   if (regName == NULL) {
      regName = "unknown";
   }
   return regName;
}

const char *getDpRegName(int reg) {
   static const char *names[] = {"Reserved","CTRL/STAT","SELECT","RDBUFF",
                                 "AP_REGx00", "AP_REGx04", "AP_REGx08", "AP_REGx0C", };
   return names[reg&0x03];
}

const char *getDHCSRName(uint32_t dhcsrValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
bitInfo bitNames[] = {
      {"S_RESET|",     DHCSR_S_RESET_ST},
      {"S_RETIRE|",    DHCSR_S_RETIRE_ST},
      {"S_LOCKUP|",    DHCSR_S_LOCKUP},
      {"S_SLEEP|",     DHCSR_S_SLEEP},
      {"S_HALT|",      DHCSR_S_HALT},
      {"S_REGRDY|",    DHCSR_S_REGRDY},
      {"C_SNAPSTALL|", DHCSR_C_SNAPSTALL},
      {"C_MASKINTS|",  DHCSR_C_MASKINTS},
      {"C_STEP|",      DHCSR_C_STEP},
      {"C_HALT|",      DHCSR_C_HALT},
      {"C_DEBUGEN|",   DHCSR_C_DEBUGEN},
      {NULL, 0},
};
bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   while (bitPtr->bitMask != 0) {
      if ((dhcsrValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}

const char *getDEMCRName(uint32_t demcrValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
bitInfo bitNames[] = {
      {"TRCENA|",       DEMCR_TRCENA},
      {"VC_HARDERR|",   DEMCR_VC_HARDERR},
      {"VC_INTERR|",    DEMCR_VC_INTERR},
      {"VC_BUSERR|",    DEMCR_VC_BUSERR},
      {"VC_STATERR|",   DEMCR_VC_STATERR},
      {"VC_CHKERR|",    DEMCR_VC_CHKERR},
      {"VC_NOCPERR|",   DEMCR_VC_NOCPERR},
      {"VC_MMERR|",     DEMCR_VC_MMERR},
      {"VC_CORERESET|", DEMCR_VC_CORERESET},
      {NULL, 0},
};
bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   if ((demcrValue & DEMCR_VC_ALL_ERRORS) == DEMCR_VC_ALL_ERRORS) {
      strcat(buffer, "ALL_ERRORS|");
      demcrValue &= ~DEMCR_VC_ALL_ERRORS;
   }
   while (bitPtr->bitMask != 0) {
      if ((demcrValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}

const char *getMDM_APStatusName(uint32_t mdmApValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
static const bitInfo bitNames[] = {
      {"MASS_ERASE_ACK|",  MDM_AP_Flash_Mass_Erase_Ack},
      {"FLASH_READY|",     MDM_AP_Flash_Ready},
      {"SECURE|",          MDM_AP_System_Security},
      {"RESET|",           MDM_AP_System_Security},
      {"MASS_ERASE_EN|",   MDM_AP_Mass_Erase_Enable},
      {"BACKDOOR_EN|",     MDM_AP_Backdoor_Access_Enable},
      {"LOW_POWER_EN|",    MDM_AP_LP_Enable},
      {"LLS_EXIT|",        MDM_AP_LLS_Mode_Exit},
      {"VLLSx_EXIT|",      MDM_AP_VLLSx_Mode_Exit},
      {"HALT|",            MDM_AP_Status_Core_Halted},
      {"SLEEP_DEEP|",      MDM_AP_Status_Core_SLEEPDEEP},
      {"SLEEPING|",        MDM_AP_Status_Core_SLEEPING},
      {NULL, 0},
};
const bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   while (bitPtr->bitMask != 0) {
      if ((mdmApValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}
const char *getMDM_APControlName(uint32_t mdmApValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
static const bitInfo bitNames[] = {
      {"MASS_ERASE|",    MDM_AP_Control_Flash_Mass_Erase},
      {"DEBUG_DISABLE|", MDM_AP_Control_Debug_Disable},
      {"DEBUG_REQUEST|", MDM_AP_Control_Debug_Request},
      {"RESET_REQUEST|", MDM_AP_Control_System_Reset_Request},
      {"CORE_HOLD|",     MDM_AP_Control_Core_Hold_Reset},
      {"VLLDBGREQ|",     MDM_AP_Control_VLLDBGREQ},
      {"VLLDBGACK|",     MDM_AP_Control_VLLDBGACK},
      {"STATUS_ACK|",    MDM_AP_Control_Status_Ack},
      {NULL, 0},
};
const bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   while (bitPtr->bitMask != 0) {
      if ((mdmApValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}

const char *getSRSHName(uint32_t srshValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
bitInfo bitNames[] = {
      {"SW|",     1<<2},
      {"LOCKUP|", 1<<1},
      {"JTAG|",   1<<0},
      {NULL, 0},
};
bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   while (bitPtr->bitMask != 0) {
      if ((srshValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}

const char *getSRSLName(uint32_t srslValue) {
typedef struct {
   const char *bitName;
   const uint32_t   bitMask;
} bitInfo;
bitInfo bitNames[] = {
      {"POR|",    1<<7},
      {"PIN|",    1<<6},
      {"COP|",    1<<5},
      {"LOC",     1<<2},
      {"LVD|",    1<<1},
      {"WAKEUP|", 1<<0},
      {NULL, 0},
};
bitInfo *bitPtr = bitNames;
static char buffer[200];
   buffer[0] = '\0';
   while (bitPtr->bitMask != 0) {
      if ((srslValue & bitPtr->bitMask) != 0) {
         strcat(buffer, bitPtr->bitName);
      }
      bitPtr++;
   }
   return buffer;
}

const char *getMemSpaceName(MemorySpace_t memSpace) {
   static char buffer[100];
   switch(memSpace) {
     case MS_PWord   :  return "MS_PWord";
     case MS_PLong   :  return "MS_PLong";
     case MS_XByte   :  return "MS_XByte";
     case MS_XWord   :  return "MS_XWord";
     case MS_XLong   :  return "MS_XLong";
     default: break;
   };
   switch(memSpace&MS_SPACE) {
     case MS_None    :  strcpy(buffer,"-|");                 break;
     case MS_Program :  strcpy(buffer,"MS_Program|");      break;
     case MS_Data    :  strcpy(buffer,"MS_Data|");         break;
     case MS_Global  :  strcpy(buffer,"MS_Global|");       break;
     default         :  strcpy(buffer,"MS_UnknownSpace|"); break;
   };
   switch(memSpace&MS_SIZE) {
     case MS_Byte   :  strcat(buffer,"MS_Byte");        break;
     case MS_Word   :  strcat(buffer,"MS_Word");        break;
     case MS_Long   :  strcat(buffer,"MS_Long");        break;
     default        :  strcat(buffer,"-"); break;
   };
   return buffer;
}
#endif // LOG

