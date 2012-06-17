/*
 * FlashEraseMethods.hpp
 *
 *  Created on: 17/06/2009
 *      Author: podonoghue
 */

#ifndef FLASHERASEMETHODS_HPP_
#define FLASHERASEMETHODS_HPP_

//! Information on how to erase a device
//!
class FlashEraseMethod {
public:
   virtual unsigned long
   calculateFlashFrequency(unsigned long clockFreq,
                           uint8_t            cfmclkd
                           ) = 0;
   virtual int findFlashDividerValue(unsigned long clkFreq,
                                    unsigned long fMin,
                                    unsigned long fMax) = 0;

   virtual const char *getDescription(void) = 0;
};

//! Table of methods for different devices.
//!
class FlashEraseMethods {
private:
   static unsigned int numMethods;
   static unsigned int currentMethodIndex;
   static FlashEraseMethod *methods[20];
//   static void loadMethods(void);

public:
   static FlashEraseMethod *getCurrentMethod() { return methods[currentMethodIndex]; };
   static unsigned int getCurrentMethodIndex() { return currentMethodIndex; };
   static unsigned int getNumMethods(void) { return numMethods; }
   static FlashEraseMethod *getMethod(unsigned int index) {
      if (index < numMethods)
         return methods[index];
      else
         return methods[0];
        };
   static void setMethodIndex(unsigned int index) {
      if (index < numMethods)
         currentMethodIndex = index;
      else
         currentMethodIndex = 0;
        };
};

//! Dummy erase methods
//!
class FlashEraseMethod0: public FlashEraseMethod {
public:
   unsigned long
   calculateFlashFrequency(unsigned long clockFreq,
                           uint8_t            cfmclkd
                           ) {
      return 0;
   }
   int findFlashDividerValue(unsigned long clkFreq,
                             unsigned long fMin,
                             unsigned long fMax) {
      return -1;
   }

   const char *getDescription(void) {
      return "0: Not available";
   }

};

//! These methods apply to all the Coldfire device I've checked.
//!
class FlashEraseMethod1: public FlashEraseMethod {
public:
   unsigned long
   calculateFlashFrequency(unsigned long clockFreq,
                           uint8_t            cfmclkd
                           );

   int findFlashDividerValue(unsigned long clkFreq,
                             unsigned long fMin,
                             unsigned long fMax
                             );

   const char *getDescription(void) {
      return "1: f=(PRDIV8?(clockFreq/8):clockFreq)/(DIV+1)";
   }

};

//! These methods apply to all the Coldfire device I've checked.
//!
class FlashEraseMethod2: public FlashEraseMethod1 {
   const char *getDescription(void) {
      return "2: f=(PRDIV8?(clockFreq/8):clockFreq)/(DIV+1)";
   }
};

#endif /* FLASHERASEMETHODS_HPP_ */
