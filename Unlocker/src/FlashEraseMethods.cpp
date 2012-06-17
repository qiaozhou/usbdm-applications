/*! \file
    \brief Erase methods for Coldfire Devices

    \verbatim
    CF_Unlocker
    Copyright (C) 2009  Peter O'Donoghue

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
   |   1 Jun 2009 | Created
   +====================================================================
    \endverbatim
*/
#include <stdio.h>
#include <stdlib.h>

#include "Debug.h"
#include "Common.h"
#include "USBDM_API.h"
#include "BitVector.h"
#include "KnownDevices.h"
#include "JTAG.h"
#include "FlashEraseMethods.h"

//! \brief Calculates Flash clock frequency
//!
//! Method 1
//!
//! This equations applies to all the Coldfire device I've checked.
//!
//! @param clockFreq : Target clock frequency
//! @param cfmclkd   : Clock divider value
//!
//! @return Flash clock frequency
//!
unsigned long
FlashEraseMethod1::calculateFlashFrequency(unsigned long clockFreq,
                                           uint8_t            cfmclkd
                                           ) {
int PRDIV8 = (cfmclkd&(1<<6)); // CLKDIV bit in clock division register
int CLKDIV = (cfmclkd&0x3F);   // Clock division factor

   if ((cfmclkd & 0x80)!= 0) // bit 7 must be zero
      return 0;

   return (PRDIV8?(clockFreq/8):clockFreq)/(CLKDIV+1);
}

//! \brief Searches for the 'best' Flash clock divider register value
//!
//! Best in this case means the value resulting in the clock nearest to the
//! middle of the permitted range.
//!
//! @param clkFreq   : Target clock frequency
//! @param fMin      : Minimum permitted Flash clock frequency
//! @param fMax      : Maximum permitted Flash clock frequency
//!
//! @return  'Best' Clock divider value
//!
int FlashEraseMethod1::findFlashDividerValue(unsigned long clkFreq,
                                             unsigned long fMin,
                                             unsigned long fMax) {
int cfmclkd, returnValue = -1;
unsigned long fclk;
unsigned long error  = 2*(fMax-fMin);
unsigned long target = (fMax+fMin)/2;

//   print("BusFreq=%8ld, Fmin=%8ld, Fmax=%8ld\n", clkFreq, fMin, fMax);

   for (cfmclkd = 0; cfmclkd<256; cfmclkd++) {
      fclk = calculateFlashFrequency(clkFreq, cfmclkd);
      if ((fclk> fMin) && (fclk<=fMax)) { // Possible value?
         // Check if better than last found value
         unsigned long newError = abs(fclk - target);
         if (newError < error) {
            returnValue = cfmclkd;
            error       = newError;
//            print("cfmclkd=0x%2.2x => F=%8ld, error=%ld +\n", returnValue, fclk, newError );
         }
//         else
//            print("cfmclkd=0x%2.2x => F=%8ld, error=%ld -\n", returnValue, fclk, newError );
      }
   }
//   if (returnValue>=0)
//      print("BusFreq=%8ld, Fmin=%8ld, Fmax=%8ld, F=%8ld, cfmclkd=0x%2.2x\n",
//              clkFreq, fMin, fMax, calculateFlashFrequency(clkFreq, returnValue), returnValue );
//   else
//      print("BusFreq=%8ld, Fmin=%8ld, Fmax=%8ld, Failed\n", clkFreq, fMin, fMax );
   return returnValue;
}

FlashEraseMethod *FlashEraseMethods::methods[] = {new FlashEraseMethod0, new FlashEraseMethod1, new FlashEraseMethod2};
unsigned int FlashEraseMethods::numMethods = 3;
unsigned int FlashEraseMethods::currentMethodIndex = 0;

