/*
 * GdbInput.h
 *
 *  Created on: 22/04/2012
 *      Author: podonoghue
 */

#ifndef GDBINPUT_H_
#define GDBINPUT_H_

#include <pthread.h>
#include <stdio.h>

class GdbPacket {
public:
   static const GdbPacket breakToken;
   static const int MAX_MESSAGE=1000;
   int  size;
   char buffer[MAX_MESSAGE];
   int  checkSum;
   int  sequence;
   bool isBreak() const { return this == &breakToken; }
};

class GdbInput {
private:
   enum Statetype {hunt, data, escape, checksum1, checksum2};
   Statetype         state;
   bool              full;
   bool              empty;
   bool              isEndOfFile;
   const GdbPacket  *buffer;
   FILE             *pipeIn;
   pthread_mutex_t   mutex;
   pthread_cond_t    dataNotFull_cv;
   pthread_cond_t    dataNotEmpty_cv;

public:
   GdbInput(FILE *in);
   bool createThread(void);
   const GdbPacket *getGdbPacket(void);
   bool isEOF(void) {return isEndOfFile;}
   void flush(void);

private:
   static void *threadFunc(void *arg);
   const GdbPacket *receiveGdbPacket(void);
};

#endif /* GDBINPUT_H_ */
