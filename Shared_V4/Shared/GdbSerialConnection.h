/*
 * GdbSerialConnection.h
 *
 *  Created on: 15/06/2011
 *      Author: podonoghue
 */

#ifndef GDBSERIALCONNECTION_H_
#define GDBSERIALCONNECTION_H_

#include <pthread.h>

class GdbPipeConnection {
public:
   static const int MAX_MESSAGE = 1000;
   class GdbPacket {
   public:
      int  size;
      char buffer[MAX_MESSAGE];
      int  checkSum;
      int  sequence;
      bool isBreak() const { return this == &breakToken; }
   };
   enum CommunicationState {ok=0, noData=EOF-1, endOfFile=EOF};

private:
   enum Statetype {hunt, data, escape, checksum1, checksum2, dead};

   static const GdbPacket breakToken;
   char              buff1[MAX_MESSAGE];
   bool              pipeOpen;
   FILE *            pipeIn;
   FILE *            pipeOut;
   bool              abort;
   pthread_t         tid;
   bool              breakFlag;
   Statetype         state;
   GdbPacket         packet;
   const GdbPacket * currentPacket;
   int               gdbChecksum;
   unsigned          gdbCharCount;
   char              gdbBuffer[1000];
   char *            gdbPtr;
   static FILE *     errorLog;
   int               sequenceNum;
   const char *      errorMessage;

private:
   pthread_mutex_t mutex;
   pthread_cond_t  dataNotFull_cv;
   pthread_cond_t  dataNotEmpty_cv;
   bool            full;
   bool            empty;
   bool            isEndOfFile;
   int             buffer;

private:
   bool createThread(void);
   int getchar(void);

private:
   static void *threadFunc(void *arg);

   GdbPipeConnection(FILE *in, FILE *out);
   void txGdbPkt(void);

public:
   ~GdbPipeConnection();
   static GdbPipeConnection *createHandler(FILE *in_, FILE *out_);
   bool isOpen();
   void endLoop(void);
   CommunicationState getGdbChar(void);
   CommunicationState getGdbPacket(const GdbPacket *&pkt);

   // Add values to gdbBuffer
   void putGdbPreamble(char marker='$');
   void putGdbChar(char ch);
   void putGdbHex(unsigned count, unsigned char *buff);
   void putGdbString(const char *s, int size=-1);
   int  putGdbPrintf(const char *format, ...);
   void putGdbChecksum(void);
   void putGdbPostscript(void);

   // Send data to GDB on background thread
   void flushGdbBuffer(void);
   void sendGdbBuffer(void);
   void sendGdbString(const char *buffer, int size=-1);
   void sendGdbNotification(const char *buffer, int size=-1);
   void setErrorMessage(const char *msg) {
      this->errorMessage = msg;
   }
   void sendErrorMessage(void);
};

#endif /* GDBSERIALCONNECTION_H_ */
