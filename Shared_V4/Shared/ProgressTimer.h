/*
 * ProgressTimer.h
 *
 *  Created on: 19/02/2012
 *      Author: pgo
 */

#ifndef PROGRESSTIMER_H_
#define PROGRESSTIMER_H_

#include <sys/time.h>
#include <time.h>
#include "USBDM_API.h"
#include "FlashProgramming.h"

class ProgressTimer {
public:
   //! @param status  - Status to pass to caller
   //! @param percent - Percentage progress so far
   //! @param message - Message to display
   //!
   //! @return user response to dialogue, BDM_RC_OK to continue
   //!
   typedef USBDM_ErrorCode (*CallBackT)(USBDM_ErrorCode status, int percent, const char *message);

private:
   CallBackT       progressCallBack;
   unsigned        maximumBytes;
   struct timeval  timeStart;
   const char     *message;
   int             lastBytesDone;
   int             bytesDone;

public:
   //! Create timer
   //!
   //! @param progressCallBack - Call back used to post messages
   //! @param maximum          - Maximum cumulative value for progress
   //!
   ProgressTimer(CallBackT progressCallBack, unsigned maximum);

   //! Restart timer from 0
   //!
   //! @param message - Message to post, May be NULL to indicate no change.
   //!
   USBDM_ErrorCode restart(const char *message);

   //! Post progress message
   //!
   //! @param progress - How many bytes of progress since last call
   //! @param message  - Message to post, May be NULL to indicate no change.
   //!
   USBDM_ErrorCode progress(int progress, const char *_message);

   //! @return - elapsed time in seconds since creation or restart()
   //!
   double elapsedTime(void);

   //! @return - maximum value of timer progress
   //!
   void setMaximum(int maximum) {
      maximumBytes = maximum;
   }

   //! @return - maximum value of timer progress
   //!
   unsigned getMaximum(void) {
      return maximumBytes;
   }
};

#endif /* PROGRESSTIMER_H_ */
