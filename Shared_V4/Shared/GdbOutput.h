/*
 * GdbOutput.h
 *
 *  Created on: 23/04/2012
 *      Author: PODonoghue
 */

#ifndef GDBOUTPUT_H_
#define GDBOUTPUT_H_

#include <stdio.h>

class GdbOutput {
private:
   FILE             *pipeOut;
   const char       *errorMessage;
   int               gdbChecksum;
   unsigned          gdbCharCount;
   char             *gdbPtr;
   char              gdbBuffer[1000];

public:
   GdbOutput(FILE *out);
   // Add values to gdbBuffer
   void putGdbPreamble(char marker='$');
   void putGdbChar(char ch);
   void putGdbHex(unsigned count, unsigned char *buff);
   void putGdbString(const char *s, int size=-1);
   int  putGdbPrintf(const char *format, ...);
   void putGdbChecksum(void);
   void putGdbPostscript(void);

   // Send data to GDB
   void flushGdbBuffer(void);
   void sendGdbBuffer(void);
   void sendGdbString(const char *buffer, int size=-1);
   void sendGdbNotification(const char *buffer, int size=-1);
   void setErrorMessage(const char *msg) {
      this->errorMessage = msg;
   }
   void sendErrorMessage(void);
   void sendAck(char ackValue='+');
private:
   void txGdbPkt(void);
};

#endif /* GDBOUTPUT_H_ */
