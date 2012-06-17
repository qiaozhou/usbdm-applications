/*! \file
   \brief Clock Trimming Code for RS08/HCS08/CFV1

   ClockTrimming.h

   \verbatim
   Copyright (C) 2008  Peter O'Donoghue

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
\endverbatim

\verbatim
 Change History
+==============================================================================
| Revision History
+==============================================================================
+-----------+------------------------------------------------------------------
| 13 Jun 12 | 4.9.5 Created                                               - pgo
+==============================================================================
\endverbatim
*/
#include <math.h>
#include "Common.h"
#include "ProgressTimer.h"
#include "FlashProgramming.h"

//! Configures the ICSCG target clock
//!
//! @param busFrequency    - Resulting BDM frequency after clock adjustment
//! @param clockParameters - Describes clock settings to use
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes that connection with the target has been established so
//!       reports any errors as PROGRAMMING_RC_ERROR_FAILED_CLOCK indicating
//!       a problem programming the target clock.
//!
USBDM_ErrorCode FlashProgrammer::configureICS_Clock(unsigned long         *busFrequency,
                                                    ICS_ClockParameters_t *clockParameters){

   const uint32_t ICSC1   = parameters.getClockAddress();
   const uint32_t ICSC2   = parameters.getClockAddress()+1;
   const uint32_t ICSTRIM = parameters.getClockAddress()+2;
   const uint32_t ICSSC   = parameters.getClockAddress()+3;

   unsigned long bdmFrequency;

   flashReady = FALSE; // Not configured for Flash access

   // ToDo - Review order of writes & need for re-connect()
   if (USBDM_WriteMemory(1,1,ICSTRIM,&clockParameters->icsTrim) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible bus speed change
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,ICSC1,&clockParameters->icsC1) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,ICSC2,&clockParameters->icsC2) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible bus speed change - no delay
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,ICSSC,&clockParameters->icsSC) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   milliSleep(100);
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible FLL change
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_GetSpeed(&bdmFrequency) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   bdmFrequency *= 1000; // Convert to Hz
   *busFrequency = bdmFrequency*parameters.getBDMtoBUSFactor();
   print("configureICS_Clock(): BDM Speed = %ld kHz, Bus Speed = %ld kHz\n",
         bdmFrequency/1000, *busFrequency/1000);
   return PROGRAMMING_RC_OK;
}

//! Configures the ICGCG target clock
//!
//! @param busFrequency    - Resulting BDM frequency after clock adjustment
//! @param clockParameters - Describes clock settings to use
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes that connection with the target has been established so
//!       reports any errors as PROGRAMMING_RC_ERROR_FAILED_CLOCK indicating
//!       a problem programming the target clock.
//!
USBDM_ErrorCode FlashProgrammer::configureICG_Clock(unsigned long         *busFrequency,
                                                    ICG_ClockParameters_t *clockParameters){

   const uint32_t ICGC1    = parameters.getClockAddress();
   const uint32_t ICGC2    = parameters.getClockAddress()+1;
// const uint32_t ICGS1    = parameters.getClockAddress()+2;
// const uint32_t ICGS2    = parameters.getClockAddress()+3;
// const uint32_t ICGFLTU  = parameters.getClockAddress()+4;
// const uint32_t ICGFLTL  = parameters.getClockAddress()+5;
   const uint32_t ICGTRIM  = parameters.getClockAddress()+6;

   unsigned long bdmFrequency;

//   print("ICG Clock: Ad=0x%4.4X, C1=0x%2.2X, C2=0x%2.2X\n",
//           parameters.getClockAddress(),
//           clockParameters->icgC1,
//           clockParameters->icgC2
//           );
   flashReady = FALSE; // Not configured for Flash access

   // ToDo - Review order of writes & need for re-connect()
   if (USBDM_WriteMemory(1,1,ICGTRIM,&clockParameters->icgTrim) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible bus speed change
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,ICGC1,&clockParameters->icgC1) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,ICGC2,&clockParameters->icgC2) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   milliSleep(100);
   if (USBDM_Connect() != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_GetSpeed(&bdmFrequency) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   bdmFrequency *= 1000; // Convert to Hz
   *busFrequency = bdmFrequency*parameters.getBDMtoBUSFactor();
   print("configureICG_Clock(): BDM Speed = %ld kHz, Bus Speed = %ld kHz\n",
         bdmFrequency/1000, *busFrequency/1000);
   return PROGRAMMING_RC_OK;
}

//! Configures the MCGCG target clock
//!
//! @param busFrequency    - Resulting BDM frequency after clock adjustment
//! @param clockParameters - Describes clock settings to use
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
//! @note Assumes that connection with the target has been established so
//!       reports any errors as PROGRAMMING_RC_ERROR_FAILED_CLOCK indicating
//!       a problem programming the target clock.
//!
USBDM_ErrorCode FlashProgrammer::configureMCG_Clock(unsigned long         *busFrequency,
                                                    MCG_ClockParameters_t *clockParameters){

   const uint32_t MCGC1    = parameters.getClockAddress();
   const uint32_t MCGC2    = parameters.getClockAddress()+1;
   const uint32_t MCGTRIM  = parameters.getClockAddress()+2;
   const uint32_t MCGSC    = parameters.getClockAddress()+3;
   const uint32_t MCGC3    = parameters.getClockAddress()+4;
   const uint32_t MCGT     = parameters.getClockAddress()+5;

   unsigned long bdmFrequency;

//   print("MCG Clock: Ad=0x%4.4X, C1=0x%2.2X, C2=0x%2.2X, C3=0x%2.2X, SC=0x%2.2X, CT/C4=0x%2.2X\n",
//           parameters.getClockAddress(),
//           clockParameters->mcgC1,
//           clockParameters->mcgC2,
//           clockParameters->mcgC3,
//           clockParameters->mcgSC,
//           clockParameters->mcgCT
//           );

   flashReady = FALSE; // Not configured for Flash access

   // ToDo - Review order of writes & need for re-connect()
   if (USBDM_WriteMemory(1,1,MCGTRIM,&clockParameters->mcgTrim) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible bus speed change
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,MCGC1,&clockParameters->mcgC1) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,MCGSC,&clockParameters->mcgSC) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,MCGC2,&clockParameters->mcgC2) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_Connect() != BDM_RC_OK) // re-connect after possible bus speed change
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_WriteMemory(1,1,MCGC3,&clockParameters->mcgC3) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if ((parameters.getClockType() != S08MCGV1) &&
       (USBDM_WriteMemory(1,1,MCGT,&clockParameters->mcgCT) != BDM_RC_OK))
         return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   milliSleep(100);
   if (USBDM_Connect() != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   if (USBDM_GetSpeed(&bdmFrequency) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_FAILED_CLOCK;
   bdmFrequency *= 1000; // Convert to Hz
   *busFrequency = bdmFrequency*parameters.getBDMtoBUSFactor();
   print("configureMCG_Clock(): BDM Speed = %ld kHz, Bus Speed = %ld kHz\n",
         bdmFrequency/1000, *busFrequency/1000);
   return PROGRAMMING_RC_OK;
}

//! Configures the External target clock
//!
//! @param busFrequency    - Resulting BDM frequency after clock adjustment (Hz)
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::configureExternal_Clock(unsigned long  *busFrequency){
   unsigned long bdmFrequency;

   flashReady = FALSE; // Not configured for Flash access

   // Just connect at whatever speed
   if (USBDM_Connect() != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   if (USBDM_GetSpeed(&bdmFrequency) != BDM_RC_OK)
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   bdmFrequency *= 1000; // Convert to Hz
   *busFrequency = bdmFrequency*parameters.getBDMtoBUSFactor();
   print("configureExternal_Clock(): BDM Speed = %ld kHz, Bus Speed = %ld kHz\n",
         bdmFrequency/1000, *busFrequency/1000);
   return PROGRAMMING_RC_OK;

}

//! \brief Configures the target clock appropriately for flash programming
//!        The speed would be the maximum safe value for an untrimmed target
//!
//! @param busFrequency    - Resulting BDM frequency after clock adjustment \n
//!                          For a HCS08/CFV1 with CLKSW=1 this will be the bus frequency
//!
//! @return error code, see \ref USBDM_ErrorCode
//!
USBDM_ErrorCode FlashProgrammer::configureTargetClock(unsigned long  *busFrequency) {

   //! ICSCG parameters for flash programming (Maximum safe speed untrimmed 4/8MHz)
   static const ICS_ClockParameters_t ICS_FlashSpeedParameters = {
         // bdm clock = reference clock * 512/1024
         /* .icsC1   = */ 0x04, // IREFS
         /* .icsC2   = */ 0x40, // BDIV=/2
         /* .icsTrim = */ 0x80, // TRIM=nominal
         /* .icsSC   = */ 0x00, // DRS=0,DMX32=0,FTRIM=0
   };
   //! ICSCG parameters for flash programming (Maximum safe speed untrimmed 4/8MHz)
   static const ICS_ClockParameters_t ICSV4_FlashSpeedParameters = {
         // bdm clock = reference clock * 512/1024
         /* .icsC1   = */ 0x04, // IREFS
         /* .icsC2   = */ 0x20, // BDIV=/2
         /* .icsTrim = */ 0x80, // TRIM=nominal
         /* .icsSC   = */ 0x00, // DRS=0,DMX32=0,FTRIM=0
   };
   //! ICGCG parameters for flash programming
   static const ICG_ClockParameters_t ICG_FlashSpeedParameters = {
         // bdm clock = 64*MFDt*reference clock/(7*RDFt) = 64*14*refClk/(7*2*2) = 32*refClk
         /* .icgC1     = */ 0x48, // CLKS=01, RANGE=1
         /* .icgC2     = */ 0x51, // MFD=5, RFD=1
         /* .icgFilter = */ 0x00, // Not used
         /* .icgTrim   = */ 0x80, // TRIM=128
   };
   //! ICGCG parameters for flash programming 4/8 MHz
   static const MCG_ClockParameters_t MCG_FlashSpeedParameters = {
         // bdm clock = reference clock * 1024/2
         /* .mcgC1   = */ 0x04, // IREFS
         /* .mcgC2   = */ 0x40, // BDIV=/2
         /* .mcgC3   = */ 0x01, // VDIV=x4 (not used)
         /* .mcgTrim = */ 0x80, // TRIM=nominal
         /* .mcgSC   = */ 0x00, // FTRIM=0
         /* .mcgCT   = */ 0x00, // DMX32=0, DRS=0
   };

   ICS_ClockParameters_t   ICS_SpeedParameters   = ICS_FlashSpeedParameters;
   ICS_ClockParameters_t   ICSV4_SpeedParameters = ICSV4_FlashSpeedParameters;
   ICG_ClockParameters_t   ICG_SpeedParameters   = ICG_FlashSpeedParameters;
   MCG_ClockParameters_t   MCG_SpeedParameters   = MCG_FlashSpeedParameters;

//   print("Configuring Target clock\n");

   switch (parameters.getClockType()) {
      case CLKEXT:
      case CLKINVALID:
         return configureExternal_Clock(busFrequency);
      case S08ICGV1:
      case S08ICGV2:
      case S08ICGV3:
      case S08ICGV4:
         // Program clock for approx. 8 MHz
         return configureICG_Clock(busFrequency, &ICG_SpeedParameters);
      case S08ICSV1:
      case S08ICSV2:
      case S08ICSV2x512:
      case S08ICSV3:
      case RS08ICSV1:
      case RS08ICSOSCV1:
         // Program clock for approx. 4/8 MHz
         return configureICS_Clock(busFrequency, &ICS_SpeedParameters);
      case S08ICSV4:
         // Program clock for approx. 4/8 MHz
         return configureICS_Clock(busFrequency, &ICSV4_SpeedParameters);
      case S08MCGV1:
      case S08MCGV3:
         // Program clock for approx. 4/8 MHz
         return configureMCG_Clock(busFrequency, &MCG_SpeedParameters);
         break;
      case S08MCGV2:
         // Program clock for approx. 8 MHz
         MCG_SpeedParameters.mcgCT = 0x01; // DRS = 1
         return configureMCG_Clock(busFrequency, &MCG_SpeedParameters);
         break;
   }
   return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
}

//!  Determines the trim value for the target internal clock.
//!  The target clock is left trimmed for a bus freq. of targetBusFrequency.
//!
//!     Target clock has been suitably configured.
//!
//!  @param      trimAddress           Address of trim register.
//!  @param      targetBusFrequency    Target Bus Frequency to trim to.
//!  @param      returnTrimValue       Resulting trim value (9-bit number)
//!  @param      measuredBusFrequency  Resulting Bus Frequency
//!  @param      do9BitTrim            True to do 9-bit trim (rather than 8-bit)
//!
//!  @return
//!   == \ref PROGRAMMING_RC_OK  => Success \n
//!   != \ref PROGRAMMING_RC_OK  => Various errors
//!
USBDM_ErrorCode FlashProgrammer::trimTargetClock(uint32_t       trimAddress,
                                                 unsigned long  targetBusFrequency,
                                                 uint16_t      *returnTrimValue,
                                                 unsigned long *measuredBusFrequency,
                                                 int            do9BitTrim){
   uint8_t          mask;
   uint8_t          trimMSB, trimLSB, trimCheck;
   int              trimValue;
   int              maxRange;
   int              minRange;
   unsigned         long bdmSpeed;
   int              index;
   USBDM_ErrorCode  rc = PROGRAMMING_RC_OK;

   static const int maxTrim        = 505;   // Maximum acceptable trim value
   static const int minTrim        =   5;   // Minimum acceptable trim value
   static const int SearchOffset   =   8;   // Linear sweep range is +/- this value
   static const unsigned char zero =   0;
   const unsigned long targetBDMFrequency = targetBusFrequency/parameters.getBDMtoBUSFactor();
   int numAverage;     // Number of times to repeat measurements

   double sumX          = 0.0;
   double sumY          = 0.0;
   double sumXX         = 0.0;
   double sumYY         = 0.0;
   double sumXY         = 0.0;
   double num           = 0.0;
   double alpha, beta;
   double trimValueF;

// print("trimTargetClock(targetBusFrequency=%ld, targetBDMFrequency=%ld)\n",
//         targetBusFrequency, targetBDMFrequency);

   flashReady = FALSE; // Not configured for Flash access

   // Set safe defaults
   *returnTrimValue      = 256;
   *measuredBusFrequency = 10000;
   trimMSB               = 0;

   // Set LSB trim value = 0
   if (USBDM_WriteMemory(1,1,trimAddress+1, &zero) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Initial binary search (MSB only)
   for (mask = 0x80; mask > 0x0; mask>>=1) {
      trimMSB |= mask;
      // Set trim value (MSB only)
      if (USBDM_WriteMemory(1, 1, trimAddress, &trimMSB) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      // Check target speed
      if (USBDM_Connect() != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_CONNECT;
      }
      if (USBDM_GetSpeed(&bdmSpeed) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      bdmSpeed *= 1000; // convert to Hz

//    print("trimTargetClock() Binary search: trimMSB=0x%2.2x (%d), bdmSpeed=%ld%c\n",
//            trimMSB, trimMSB, bdmSpeed, (bdmSpeed<targetBDMFrequency)?'-':'+');

      // Adjust trim value
      if (bdmSpeed<targetBDMFrequency) {
         trimMSB &= ~mask; // too slow
      }
   }

   // Binary search value is middle of range to sweep

   trimValue = trimMSB<<1;  // Convert to 9-bit value

   // Linear sweep +/-SEARCH_OFFSET, starting at higher freq (smaller Trim)
   // Range is constrained to [minTrim..maxTrim]

   maxRange = trimValue + SearchOffset;
   if (maxRange > maxTrim) {
      maxRange = maxTrim;
   }
   minRange = trimValue - SearchOffset;
   if (minRange < minTrim) {
      minRange = minTrim;
   }
//   print("trimTargetClock(): Linear sweep, f=%6ld    \n"
//                   "trimTargetClock():    Trim       frequency \n"
//                   "========================================== \n",
//                   targetBDMFrequency/1000);

   if (do9BitTrim) {
      numAverage = 2;
   }
   else {
      numAverage = 4;
   }
   for(trimValue=maxRange; trimValue>=minRange; trimValue--) {
      trimLSB = trimValue&0x01;
      trimMSB = (uint8_t)(trimValue>>1);
      if (do9BitTrim) {
         // Write trim LSB
         if (USBDM_WriteMemory(1, 1, trimAddress+1, &trimLSB) != BDM_RC_OK)
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
         if (USBDM_ReadMemory(1, 1, trimAddress+1,   &trimCheck) != BDM_RC_OK)
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
         if ((trimCheck&0x01) != trimLSB)
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      else if (trimValue&0x01) {
         // skip odd trim values if 8-bit trim
         continue;
      }
      // Write trim MSB
      if (USBDM_WriteMemory(1, 1, trimAddress,   &trimMSB) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      if (USBDM_ReadMemory(1, 1, trimAddress,   &trimCheck) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      if (trimCheck != trimMSB) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      //milliSleep(100);
      // Measure sync multiple times
      for(index=numAverage; index>0; index--) {
         // Check target speed
         if (USBDM_Connect() != BDM_RC_OK)
            return PROGRAMMING_RC_ERROR_BDM_CONNECT;
         if (USBDM_GetSpeedHz(&bdmSpeed) != BDM_RC_OK)
            return PROGRAMMING_RC_ERROR_BDM_CONNECT;
         sumX  += trimValue;
         sumY  += bdmSpeed;
         sumXX += trimValue*trimValue;
         sumYY += bdmSpeed*bdmSpeed;
         sumXY += bdmSpeed*trimValue;
         num   += 1.0;
//         print("trimTargetClock(): %6d    %10ld %10ld\n", trimValue, bdmSpeed, targetBDMFrequency-bdmSpeed);
      }
   }
   for(trimValue=minRange; trimValue<=maxRange; trimValue++) {
      trimLSB = trimValue&0x01;
      trimMSB = (uint8_t)(trimValue>>1);
      if (do9BitTrim) {
         // Write trim LSB
         if (USBDM_WriteMemory(1, 1, trimAddress+1, &trimLSB) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
         }
         if (USBDM_ReadMemory(1, 1, trimAddress+1,   &trimCheck) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
         }
         if ((trimCheck&0x01) != trimLSB) {
            return PROGRAMMING_RC_ERROR_BDM_WRITE;
         }
      }
      else if (trimValue&0x01) {
         // skip odd trim values if 8-bit trim
         continue;
      }
      // Write trim MSB
      if (USBDM_WriteMemory(1, 1, trimAddress,   &trimMSB) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      if (USBDM_ReadMemory(1, 1, trimAddress,   &trimCheck) != BDM_RC_OK) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      if (trimCheck != trimMSB) {
         return PROGRAMMING_RC_ERROR_BDM_WRITE;
      }
      //milliSleep(100);
      // Measure sync multiple times
      for(index=numAverage; index>0; index--) {
         // Check target speed
         if (USBDM_Connect() != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_BDM_CONNECT;
         }
         if (USBDM_GetSpeedHz(&bdmSpeed) != BDM_RC_OK) {
            return PROGRAMMING_RC_ERROR_BDM_CONNECT;
         }
         sumX  += trimValue;
         sumY  += bdmSpeed;
         sumXX += trimValue*trimValue;
         sumYY += bdmSpeed*bdmSpeed;
         sumXY += bdmSpeed*trimValue;
         num   += 1.0;
//         print("trimTargetClock(): %6d    %10ld %10ld\n", trimValue, bdmSpeed, targetBDMFrequency-bdmSpeed);
      }
   }

//   print("N=%f, sumX=%f, sumXX=%f, sumY=%f, sumYY=%f, sumXY=%f\n",
//                    num, sumX, sumXX, sumY, sumYY, sumXY);

   // Calculate linear regression co-efficients
   beta  = (num*sumXY-sumX*sumY)/(num*sumXX-sumX*sumX);
   alpha = (sumY-beta*sumX)/num;

   // Estimate required trim value
   trimValueF = ((targetBDMFrequency-alpha)/beta);

   if ((trimValueF <= 5.0) || (trimValue >= 505.0)) { // resulted in extreme value
      trimValueF = 256.0;                             // replace with 'Safe' trim value
      rc = PROGRAMMING_RC_ERROR_TRIM;
   }

   trimValue = (int)round(trimValueF);
   trimMSB   = trimValue>>1;
   trimLSB   = trimValue&0x01;

//   print("alpha= %f, beta= %f, trimF= %f, trimMSB= %d (0x%2.2X), trimLSB= %d\n",
//           alpha, beta, trimValueF, trimMSB, trimMSB, trimLSB);

//   print("trimTargetClock() Result: trim=0x%3.3x (%d), measured bdmSpeed=%ld\n",
//           savedTrimValue, savedTrimValue, bestFrequency);

   *returnTrimValue  = trimValue;

   // Set trim value (LSB first)
   if ((do9BitTrim && (USBDM_WriteMemory(1, 1, trimAddress+1, &trimLSB) != BDM_RC_OK)) ||
       (USBDM_WriteMemory(1, 1, trimAddress,   &trimMSB) != BDM_RC_OK)) {
      return PROGRAMMING_RC_ERROR_BDM_WRITE;
   }
   // Check connection at that speed
   if (USBDM_Connect() != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }
   if (USBDM_GetSpeedHz(&bdmSpeed) != BDM_RC_OK) {
      return PROGRAMMING_RC_ERROR_BDM_CONNECT;
   }
   *measuredBusFrequency = bdmSpeed*parameters.getBDMtoBUSFactor();

   return rc;
}

USBDM_ErrorCode FlashProgrammer::trimICS_Clock(ICS_ClockParameters_t *clockParameters) {

   static const ICS_ClockParameters_t ICS_LowSpeedParameters = {
         // bus clock = reference clock * (512,1024,512)/8/2
         //           = (32,64,32)*refClk
         //           ~ (1,2,1) MHz
         /* .icsC1   = */ 0x04, // IREFS
         /* .icsC2   = */ 0xC0, // BDIV=/8
         /* .icsTrim = */ 0x80, // TRIM=default
         /* .icsSC   = */ 0x00, // DRS=0,DMX32=0,FTRIM=0
   };
   static const ICS_ClockParameters_t ICSV4_LowSpeedParameters = {
         // bus clock = reference clock * (512,1024,512)/8/2
         //           = (32,64,32)*refClk
         //           ~ (1,2,1) MHz
         /* .icsC1   = */ 0x04, // IREFS
         /* .icsC2   = */ 0x60, // BDIV=/8
         /* .icsTrim = */ 0x80, // TRIM=default
         /* .icsSC   = */ 0x00, // LOLIE=0, CME=0, SCFTRIM=0
   };
   uint32_t           ICSTRIM             = parameters.getClockAddress()+2;
   unsigned int  clockMultiplier;
   unsigned long targetBusFrequency;
   unsigned long originalBusFrequency;
   unsigned long measuredBusFrequency;
   uint16_t           trimValue;
   USBDM_ErrorCode rc;

   print("trimICS_Clock()\n");
   switch (parameters.getClockType()) {
      case S08ICSV1:
      case S08ICSV3:
      case RS08ICSV1:
      case RS08ICSOSCV1:
      case S08ICSV2x512:
         clockMultiplier = 512;
         *clockParameters = ICS_LowSpeedParameters;
         break;
      case S08ICSV2:
          clockMultiplier = 1024;
          *clockParameters = ICS_LowSpeedParameters;
          break;
      case S08ICSV4:
         clockMultiplier = 1024;
         *clockParameters = ICSV4_LowSpeedParameters;
         break;
      default:
         return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   }

   targetBusFrequency = parameters.getClockTrimFreq()*(clockMultiplier/8/2);

   print("trimICSClock(): TrimF = %.2f kHz, clock Multiplier=%d => Trim bus Freq=%.2f kHz\n",
         parameters.getClockTrimFreq()/1000.0, clockMultiplier/8/2, targetBusFrequency/1000.0);

   // Program clock for a low speed to improve accuracy of trimming
   rc = configureICS_Clock(&originalBusFrequency, clockParameters);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   print("trimICSClock(Clock Freq = %4.2f kHz, Bus Freq=%4.2f kHz)\n",
         parameters.getClockTrimFreq()/1000.0, targetBusFrequency/1000.0);

   rc = FlashProgrammer::trimTargetClock(ICSTRIM, targetBusFrequency, &trimValue, &measuredBusFrequency, TRUE);

   print("trimICSClock() rc=%d, Desired Freq = %.2f kHz,"
         " Meas. Freq=%.2f kHz, Trim=0x%2.2X/%1.1X\n",
         rc,
         parameters.getClockTrimFreq()/1000.0,
         (measuredBusFrequency/(clockMultiplier/16))/1000.0,
         trimValue>>1, trimValue&0x01);

   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Update trim value
   clockParameters->icsTrim  = trimValue>>1;
   clockParameters->icsSC   |= trimValue&0x01;

   return rc;
}

USBDM_ErrorCode FlashProgrammer::trimMCG_Clock(MCG_ClockParameters_t *clockParameters) {

   static const MCG_ClockParameters_t MCG_LowSpeedParameters = {
         // bus clock = reference clock * (512,1024)/8/2
         //           = (32,64)*refClk (~32.5kHz)
         //           ~ (1,2) MHz
         /* .mcgC1   = */ 0x04, // IREFS
         /* .mcgC2   = */ 0xC0, // BDIV=/8
         /* .mcgC3   = */ 0x01, // VDIV=x4 (not used)
         /* .mcgTrim = */ 0x80, // TRIM=default
         /* .mcgSC   = */ 0x00, // FTRIM=0
         /* .mcgCT   = */ 0x00, // DMX32=0, DRS=0
   };

   uint32_t MCGTRIM                = parameters.getClockAddress()+2;
   unsigned int  clockMultiplier;
   unsigned long targetBusFrequency;
   unsigned long originalBusFrequency;
   unsigned long measuredBusFrequency;
   uint16_t trimValue;
   USBDM_ErrorCode rc;

   switch (parameters.getClockType()) {
      case S08MCGV2:
      case S08MCGV3:
         clockMultiplier = 512;
         break;
      case S08MCGV1:
         clockMultiplier = 1024;
         break;
      default:
         return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
   }

   targetBusFrequency = parameters.getClockTrimFreq()*(clockMultiplier/8/2);

   *clockParameters = MCG_LowSpeedParameters;

   print("trimMCGClock(): TrimF = %.2f kHz, clock Multiplier=%d => Trim bus Freq=%.2f kHz\n",
         parameters.getClockTrimFreq()/1000.0, (1024/8/2), targetBusFrequency/1000.0);

   // Program clock for a low speed to improve accuracy of trimming
   rc = configureMCG_Clock(&originalBusFrequency, clockParameters);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
//   print("trimMGCClock(Clock Freq = %.2f kHz, bus Freq = %.2f kHz)\n",
//           parameters.getClockTrimFreq()/1000.0, bdmFrequency/1000.0);

   rc = FlashProgrammer::trimTargetClock(MCGTRIM, targetBusFrequency, &trimValue, &measuredBusFrequency, TRUE);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   print("trimMCGClock() Desired Freq = %.2f kHz, "
         "Meas. Freq=%.2f kHz, Trim=0x%2.2X/%1.1X\n",
         parameters.getClockTrimFreq()/1000.0,
         (measuredBusFrequency/(clockMultiplier/16))/1000.0,
         trimValue>>1, trimValue&0x01);

   // Update trim value
   clockParameters->mcgTrim  = trimValue>>1;
   clockParameters->mcgSC   |= trimValue&0x01;

   return rc;
}

USBDM_ErrorCode FlashProgrammer::trimICG_Clock(ICG_ClockParameters_t *clockParameters) {

   static const ICG_ClockParameters_t ICG_LowSpeedParameters = {
         // bus clock = 64*MFDt*reference clock/(7*RDFt)
         //           = 64*14*refClk/(7*16*2)
         //           = 4*refClk  ~ 1MHz
         /* .icgC1     = */ 0x48, // CLKS=01, RANGE=1
         /* .icgC2     = */ 0x54, // MFD=4, RFD=9
         /* .icgFilter = */ 0x00, // Not used
         /* .icgTrim   = */ 0x80, // TRIM=128
   };
   uint32_t ICGTRIM                 = parameters.getClockAddress()+6;
   unsigned long targetBusFrequency = parameters.getClockTrimFreq()*4;
   unsigned long originalBusFrequency;
   unsigned long measuredBusFrequency;
   uint16_t trimValue;
   USBDM_ErrorCode rc;

   *clockParameters = ICG_LowSpeedParameters;

   // Program clock for a low speed to improve accuracy of trimming
   rc = configureICG_Clock(&originalBusFrequency, clockParameters);
   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   print("trimIGCClock(): TrimF = %.2f kHz, clock Multiplier=%d => Trim bus Freq=%.2f kHz\n",
         parameters.getClockTrimFreq()/1000.0, 4, targetBusFrequency/1000.0);

   rc = FlashProgrammer::trimTargetClock(ICGTRIM, targetBusFrequency, &trimValue, &measuredBusFrequency, FALSE);

   print("trimICGClock() Desired Freq = %.1f kHz, Meas. Freq=%.1f kHz, Trim=%2.2X\n",
         parameters.getClockTrimFreq()/1000.0,
         (measuredBusFrequency/4)/1000.0,
         trimValue>>1);

   if (rc != PROGRAMMING_RC_OK) {
      return rc;
   }
   // Update trim value (discard LSB)
   clockParameters->icgTrim = trimValue>>1;

   return rc;
}

//! Determines trim values for the target clock. \n
//! The values determined are written to the Flash image for later programming.
//!
//! @param flashImage - Flash image to be updated.
//!
//! @return error code see \ref USBDM_ErrorCode
//!
//! @note If the trim frequency indicated in parameters is zero then no trimming is done.
//!       This is not an error.
//!
USBDM_ErrorCode FlashProgrammer::setFlashTrimValues(FlashImage *flashImage) {

   ClockParameters clockTrimParameters;
   USBDM_ErrorCode rc;
   uint16_t  ftrimMergeValue;

   // Assume no trim or failure
   parameters.setClockTrimValue(0);

   // No trimming required
   if (parameters.getClockTrimFreq() == 0)
      return PROGRAMMING_RC_OK;

   // print("setFlashTrimValues() - trimming\n");
   progressTimer->restart("Calculating Clock Trim");

   switch (parameters.getClockType()) {
      case CLKEXT:
      case CLKINVALID:
         return PROGRAMMING_RC_OK;
      case S08ICGV1:
      case S08ICGV2:
      case S08ICGV3:
      case S08ICGV4:
         // 8-bit value (LSB ignored)
         rc = trimICG_Clock(&clockTrimParameters.icg);
         if (rc != PROGRAMMING_RC_OK)
            return rc;
         parameters.setClockTrimValue(clockTrimParameters.icg.icgTrim<<1);
         flashImage->setValue(parameters.getClockTrimNVAddress(),
                                         clockTrimParameters.icg.icgTrim);
         return PROGRAMMING_RC_OK;
      case S08ICSV1:
      case S08ICSV2:
      case S08ICSV3:
      case S08ICSV4:
      case RS08ICSV1:
      case RS08ICSOSCV1:
      case S08ICSV2x512:
         // 9-bit value
         rc = trimICS_Clock(&clockTrimParameters.ics);
         if (rc != PROGRAMMING_RC_OK) {
            return rc;
         }
         parameters.setClockTrimValue((clockTrimParameters.ics.icsTrim<<1)|
                                       (clockTrimParameters.ics.icsSC&0x01));
         flashImage->setValue(parameters.getClockTrimNVAddress()+1,
                                         clockTrimParameters.ics.icsTrim);
         // The FTRIM bit may be combined with other bits such as DMX32 or DRS
         ftrimMergeValue = flashImage->getValue(parameters.getClockTrimNVAddress());
         if (((ftrimMergeValue&0xFF) == 0xFF)||(ftrimMergeValue >= 0xFF00U)) {
            // Image location is unused or contains the unprogrammed FLASH value
            // So no value to merge trim with - clear all other bits
            ftrimMergeValue = 0x00;
         }
         print("FlashProgrammer::setFlashTrimValues(): merging FTRIM with 0x%2.2X\n", ftrimMergeValue);
         flashImage->setValue(parameters.getClockTrimNVAddress(),
                                        (ftrimMergeValue&~0x01)|(clockTrimParameters.ics.icsSC&0x01));
         return PROGRAMMING_RC_OK;
      case S08MCGV1:
      case S08MCGV2:
      case S08MCGV3:
         // 9-bit value
         rc = trimMCG_Clock(&clockTrimParameters.mcg);
         if (rc != PROGRAMMING_RC_OK) {
            return rc;
         }
         parameters.setClockTrimValue((clockTrimParameters.mcg.mcgTrim<<1)|
                                      (clockTrimParameters.mcg.mcgSC&0x01));
         flashImage->setValue(parameters.getClockTrimNVAddress()+1,
                                         clockTrimParameters.mcg.mcgTrim);
         flashImage->setValue(parameters.getClockTrimNVAddress(),
                                         clockTrimParameters.mcg.mcgSC&0x01);
         return PROGRAMMING_RC_OK;
   }
   return PROGRAMMING_RC_ERROR_ILLEGAL_PARAMS;
}

//=======================================================================
//! Updates the memory image from the target flash Clock trim location(s)
//!
//! @param flashImage   = Flash image
//!
USBDM_ErrorCode FlashProgrammer::dummyTrimLocations(FlashImage *flashImage) {

unsigned size  = 0;
uint32_t start = 0;

   // Not using trim -> do nothing
   if ((parameters.getClockTrimNVAddress() == 0) ||
       (parameters.getClockTrimFreq() == 0)) {
      print("FlashProgrammer::dummyTrimLocations() - Not using trim, no adjustment to image\n");
      return PROGRAMMING_RC_OK;
   }
   switch (parameters.getClockType()) {
      case S08ICGV1:
      case S08ICGV2:
      case S08ICGV3:
      case S08ICGV4:
         start = parameters.getClockTrimNVAddress();
         size  = 2;
         break;
      case S08ICSV1:
      case S08ICSV2:
      case S08ICSV2x512:
      case S08ICSV3:
      case S08ICSV4:
      case S08MCGV1:
      case S08MCGV2:
      case S08MCGV3:
      case RS08ICSOSCV1:
      case RS08ICSV1:
         start = parameters.getClockTrimNVAddress();
         size  = 2;
         break;
      case CLKEXT:
      default:
         break;
   }
   if (size == 0) {
      return PROGRAMMING_RC_OK;
   }
   // Read existing trim information from target
   uint8_t data[10];
   USBDM_ErrorCode rc = USBDM_ReadMemory(1,size,start,data);
   if (rc != BDM_RC_OK) {
      print("FlashProgrammer::dummyTrimLocations() - Trim location read failed\n");
      return PROGRAMMING_RC_ERROR_BDM_READ;
   }
   print("FlashProgrammer::dummyTrimLocations() - Modifying image[0x%06X..0x%06X]\n",
         start, start+size-1);
   // Update image
   for(uint32_t index=0; index < size; index++ ) {
      flashImage->setValue(start+index, data[index]);
   }
   return PROGRAMMING_RC_OK;
}
