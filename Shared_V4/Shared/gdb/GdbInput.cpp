/*! \file
    \brief Handles GDB input on a separate thread

    GdbInput.cpp

    \verbatim
    USBDM
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
    \endverbatim

    \verbatim
   Change History
   -==================================================================================
   | 23 Apr 2012 | Created                                                       - pgo
   +==================================================================================
   \endverbatim
*/

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#include "GdbInput.h"
#include "Log.h"

GdbInput::GdbInput(FILE *in) :
   state(hunt),
   full(false),
   empty(true),
   isEndOfFile(false),
   buffer(NULL)
{
   int inHandle = dup(fileno(in));
   pipeIn= fdopen(inHandle, "rb");

#ifdef _WIN32
   setmode( _fileno( pipeIn ), _O_BINARY );
   setvbuf ( pipeIn,  NULL, _IONBF , 0 );
#else
   setvbuf ( pipeIn,  NULL, _IONBF , 0 );
#endif
   pthread_mutex_init(&mutex, NULL);
   pthread_cond_init(&dataNotFull_cv,  NULL);
   pthread_cond_init(&dataNotEmpty_cv,  NULL);
}

//! Create background thread to monitor input pipe
//!
//! @return true  - Success \n
//!         false - Failed to create thread
//!
bool GdbInput::createThread(void) {
   pthread_t tid;
   int rc = pthread_create(&tid, NULL, threadFunc, (void*)this);
   if (rc) {
      print("AsyncInput::createThread() - ERROR return code from pthread_create() is %d\n", rc);
      return false;
   }
   return true;
}

//! GDB Packet that indicate a break has been received.
//!
const GdbPacket GdbPacket::breakToken  = {
      sizeof("break"),
      "break"
};

//!
//! Convert a HEX char to binary
//!
//! @param ch - char to convert ('0'-'9', 'a'-'f' or 'A' to 'F')
//!
//! @return converted value - no error checks!
//!
static int hex(char ch) {
   if (('0' <= ch) && (ch <= '9'))
      return ch - '0';
   if (('a' <= ch) && (ch <= 'f'))
      return ch - 'a' + 10;
   if (('A' <= ch) && (ch <= 'F'))
      return ch - 'A' + 10;
   return 0;
}

//! Receive packet from debugger (blocking but runs on background thread)
//!
//! @return - !=NULL => pkt ready
//!           ==NULL => pipe closed etc
//!
const GdbPacket *GdbInput::receiveGdbPacket(void) {
   static GdbPacket packet1;
   static GdbPacket packet2;
   static GdbPacket *packet = &packet1;
   static unsigned char  checksum    = 0;
   static unsigned char  xmitcsum    = 0;
   static unsigned int   sequenceNum = 0;

//   print("GdbPipeConnection::getGdbPacket()\n");
   if (isEndOfFile) {
	   return NULL;
   }
   while(1) {
      int ch = fgetc(pipeIn);
      if (ch == EOF) {
         int rc = ferror(pipeIn);
         if (rc != 0) {
//            print("GdbPipeConnection::getGdbPacket(): ferror() => %d\n", rc);
//            perror("PipeIn error:");
         }
//         print("GdbPipeConnection::getGdbPacket() - EOF\n");
         isEndOfFile = true;
         return NULL;
      }
      switch(state) {
      case hunt: // Looking for start of pkt (or Break)
         //         print("GdbPipeConnection::busyLoop(): h:%c\n", ch);
         if (ch == '$') { // Start token
            state        = data;
            checksum     = 0;
            packet->size  = 0;
         }
         else if (ch == 0x03) { // Break request
            state         = hunt;
//            print("GdbPipeConnection::busyLoop(): BREAK\n");
            return &GdbPacket::breakToken;
         }
         break;
      case data: // Data bytes within pkt
         //         print("GdbPipeConnection::busyLoop(): d:%c\n", ch);
         if (ch == '}') { // Escape token
            state     = escape;
            checksum  = checksum + ch;
         }
         else if (ch == '$') { // Unexpected start token
            state        = data;
            checksum         = 0;
            packet->size      = 0;
         }
         else if (ch == '#') { // End of data token
            state    = checksum1;
         }
         else { // Regular data
            if (packet->size < GdbPacket::MAX_MESSAGE) {
               packet->buffer[packet->size++] = ch;
            }
            checksum  = checksum + ch;
         }
         break;
      case escape: // Escaped byte within data
         //         print("GdbPipeConnection::busyLoop(): e:%c\n", ch);
         state    = data;
         checksum = checksum + ch;
         ch       = ch ^ 0x20;
         if (packet->size < GdbPacket::MAX_MESSAGE) {
            packet->buffer[packet->size++] = ch;
         }
         break;
      case checksum1: // 1st Checksum byte
         //         print("GdbPipeConnection::busyLoop(): c1:%c\n", ch);
         state    = checksum2;
         xmitcsum = hex(ch) << 4;
         break;
      case checksum2: // 2nd Checksum byte
         xmitcsum += hex(ch);
         //         print("GdbPipeConnection::busyLoop(): c2:%c\n", ch);
         if (checksum != xmitcsum) {
            // Invalid pkt - discard and start again
//            print("\nBad checksum: my checksum = %2.2X, ", checksum);
//            print("sent checksum = %2.2X\n", xmitcsum);
//            print(" -- Bad buffer: \"%s\"\n", packet->buffer);
//            fputc('-', pipeOut); // failed checksum
//            fflush(pipeOut);
            state = hunt;
         }
         else {
            // Valid pkt
            packet->checkSum = checksum;
            packet->buffer[packet->size] = '\0';
            state = hunt;
            packet->sequence = ++sequenceNum;
//            print("#%40s<-:%d:%03d$%*.*s#%2.2X\n", "", packet->sequence, packet->size, packet->size, packet->size, packet->buffer, packet->checkSum );
            // Pkt ready
//            print("<=:%03d:\'%*s\'\n", packet->size, packet->size, packet->buffer);
            GdbPacket *pkt = packet;
            if (packet == &packet1) {
               packet = &packet2;
            }
            else {
               packet = &packet1;
            }
            return pkt;
         }
         break;
      }
   }
   return NULL;
}

//! Background thread to monitor input pipe
//!
//! @param arg - this pointer
//!
void *GdbInput::threadFunc(void *arg) {
   GdbInput *me = (GdbInput *)arg;
   do {
      // Receive a pkt
      const GdbPacket *pkt = me->receiveGdbPacket();
      // Wait until last pkt processed
      pthread_mutex_lock(&me->mutex);
      while (me->full) {
         pthread_cond_wait(&me->dataNotFull_cv, &me->mutex);
      }
      me->buffer = pkt;
      me->full   = true;
      me->empty  = false;
      pthread_mutex_unlock(&me->mutex);
      pthread_cond_signal(&me->dataNotEmpty_cv);
   } while(true);
   return NULL;
}

const GdbPacket *GdbInput::getGdbPacket(void) {
   timespec timeToWait = {0, 0}; // Absolute time - no waiting
   if (isEndOfFile) {
      return NULL;
   }
   pthread_mutex_lock(&mutex);
   while (empty) {
      int rc = pthread_cond_timedwait(&dataNotEmpty_cv, &mutex, &timeToWait);
      if (rc != 0) {
         pthread_mutex_unlock(&mutex);
         return NULL;
      }
      // Expected - re-check empty
      continue;
   }
   const GdbPacket *pkt = buffer;
   empty = true;
   full  = false;
   pthread_mutex_unlock(&mutex);
   pthread_cond_signal(&dataNotFull_cv);
   return pkt;
}

void GdbInput::flush(void) {
   print("GdbInput::flush()\n");
   fflush(pipeIn);
}
