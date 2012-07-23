/*! \file
   \brief Simple S-Record Loading

   SimpleSRecords.cpp

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
|  1 Jun 12 | 1.0.0 Created                                               - pgo
+==============================================================================
\endverbatim
*/
#include <ctype.h>
#include "Log.h"
#include "SimpleSRecords.h"

/*! Convert a single HEX character ('0'-'9', 'a'-'f' or 'A'-'F') into a number
 *
 * @param ptr  -  Ptr to the ptr to the character to convert. *ptr is advanced
 *
 * @return - a value in the range 0 to 15
 */
static unsigned int hex1ToDecimal(const char **ptr) {
   uint8_t data = *(*ptr)++;
   if ((data >= '0') && (data <= '9'))
      return data - '0';
   if ((data >= 'a') && (data <= 'f'))
      return data - 'a' + 10;
   if ((data >= 'A') && (data <= 'F'))
      return data - 'A' + 10;
   return 0;
}

/*! Convert two HEX characters ('0'-'9', 'a'-'f' or 'A'-'F') into a number
 *
 * @param ptr  -  Ptr to the ptr to the character to convert. *ptr is advanced
 *
 * @return - a value in the range 0 to 255
 */
static unsigned int hex2ToDecimal(const char **ptr) {
   int data  =             hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   return data;
}

/*! Convert four HEX characters ('0'-'9', 'a'-'f' or 'A'-'F') into a number
 *
 * @param ptr  -  Ptr to the ptr to the character to convert. *ptr is advanced
 *
 * @return - a value in the range 0 to 65535
 */
static uint16_t hex4ToDecimal(const char **ptr) {
   int data  =             hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   return data;
}

/*! Convert six HEX characters ('0'-'9', 'a'-'f' or 'A'-'F') into a number
 *
 * @param ptr  -  Ptr to the ptr to the character to convert. *ptr is advanced
 *
 * @return - a value in the range 0 to 0xFFFFFFU
 */
static uint32_t hex6ToDecimal(const char **ptr) {
   int data  =             hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   return data;
}

/*! Convert six HEX characters ('0'-'9', 'a'-'f' or 'A'-'F') into a number
 *
 * @param ptr  -  Ptr to the ptr to the character to convert. *ptr is advanced
 *
 * @return - a value in the range 0 to 0xFFFFFFU
 */
static uint32_t hex8ToDecimal(const char **ptr) {
   int data  =             hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   data      = data * 16 + hex1ToDecimal(ptr);
   return data;
}

//!   Loads Freescale S-records
//!
//! @param srec         : SRECs to load
//! @param buffer       : Buffer for the image
//! @param bufferSize   : Size of buffer (in memoryElementType)
//! @param loadedSize   : Size of loaded image (in memoryElementType)
//! @param loadAddr     : Memory address for start of buffer
//!
//! @return error code
//!
//! @note Assumes consecutive SRECs
//! @note Modified for weird DSC S-file format (word addresses)
//!
USBDM_ErrorCode loadSRec(const char          *pSrec,
                         memoryElementType    buffer[],
                         unsigned             bufferSize,
                         unsigned            *loadedSize,
                         uint32_t            *loadAddr) {
   memoryElementType *ptr         = buffer;
   unsigned           size        = 0;
   bool               discardSrec = false;
   uint32_t           nextAddr    = 0xFFFFFFFF;
   unsigned           srecSize;

   uint32_t  addr;
   uint8_t   checkSum;

   *loadAddr   = 0xFFFFFFFF;
   *loadedSize = 0x0;

//   print("loadSRec()\n");
//   print("loadSRec() - loading:\n%s\n", pSrec);

   for(;;) {
      // Find first non-blank
      while (isspace(*pSrec)) {
         pSrec++;
      }
      if (*pSrec == '\0') {
         break;
      }
//      print("loadSRec() - Doing record starting '%10s...'\n", pSrec);
      // Check if S-record
      if ((*pSrec != 'S') && (*pSrec != 's')) {
         print("loadSRec() - Illegal SREC - expected 's' or 'S', found '%10s...'\n", pSrec);
         return BDM_RC_ILLEGAL_PARAMS;
      }
      switch (*(pSrec+1)) {
         case '0': // Information header
         case '7': // 32-bit start address
         case '8': // 24-bit start address
         case '9': // 16-bit start address
            // Discard S0, S8 & S9 records
//            print("loadSRec() - Discarding\n");
            while ((*pSrec != '\n') && (*pSrec != '\0')) {
               pSrec++;
            }
            continue;
         case '1':
            // S1 = 16-bit address, data record
            pSrec +=2; // Skip 'S1'
            srecSize = hex2ToDecimal( &pSrec );
            addr = hex4ToDecimal( &pSrec );
            checkSum = (uint8_t)srecSize + (uint8_t)(addr>>8) + (uint8_t)addr;
            srecSize -= 3; // subtract 3 from byte count (size + 2 addr bytes)
//            print("loadSRec() - S1: (%2.2X)%4.4lX:\n",srecSize,addr);
            break;
         case '2':
            // S2 = 24-bit address, data record
            pSrec +=2; // Skip 'S2'
            srecSize = hex2ToDecimal( &pSrec );
            addr = hex6ToDecimal( &pSrec );
            checkSum = (uint8_t)srecSize + (uint8_t)(addr>>16) + (uint8_t)(addr>>8) + (uint8_t)addr;
            srecSize -= 4; // subtract 4 from byte count (size + 3 addr bytes)
//            print("loadSRec() - S2: size=0x%02X, addr=0x%06X\n",srecSize,addr);
            break;
         case '3':
            // S3 32-bit address, data record
            pSrec +=2; // Skip 'S3'
            srecSize = hex2ToDecimal( &pSrec );
            addr = hex8ToDecimal( &pSrec );
            checkSum = (uint8_t)srecSize + (uint8_t)(addr>>24) + (uint8_t)(addr>>16) + (uint8_t)(addr>>8) + (uint8_t)addr;
            srecSize -= 5; // subtract 5 from byte count (size + 4 addr bytes)
//            print("loadSRec() - S3: size=0x%02X, addr=0x%08X, initial chk=0x%02X\n", srecSize, addr, checkSum);
            break;
         default:
            print("loadSRec() - Illegal SREC - unrecognised record type '%2s'\n", pSrec);
            return BDM_RC_ILLEGAL_PARAMS;
      }
#if (TARGET == HC12) || (TARGET == HCS08)
      // Discard reset vector
      discardSrec = (addr == 0xFFFE);
#endif
      if ((*loadAddr == 0xFFFFFFFF) && !discardSrec) {
         print("loadSRec() - Load address = 0x%08X\n", addr);
         *loadAddr = addr;
         nextAddr = addr;
      }
      if ((addr != nextAddr) && !discardSrec) {
         print("loadSRec() - SRECs must be consecutive, next addr=0x%08X,  load addr=0x%08X\n", nextAddr, addr);
         return BDM_RC_ILLEGAL_PARAMS;
      }
      if (sizeof(memoryElementType) == 1) {
//         print("loadSRec() - SRECs byte array\n");
         while (srecSize>0) {
            unsigned data;
            data      = hex2ToDecimal( &pSrec );
            checkSum += (uint8_t)data;
            srecSize--;
            if (!discardSrec) {
               *ptr++ = (memoryElementType)data;
               size++;
               nextAddr++;
            }
            if (size >= bufferSize) {
               print("loadSRec() - SRECs too large\n");
               return BDM_RC_ILLEGAL_PARAMS;
            }
//         print("%02X",data);
         }
      }
      else {
//         print("loadSRec() - SRECs word array\n");
         while (srecSize>0) {
            uint16_t data1, data2;
            data1     = hex2ToDecimal( &pSrec );
            checkSum += (uint8_t)data1;
            data2     = hex2ToDecimal( &pSrec );
            checkSum += (uint8_t)data2;
            srecSize -= 2;
            if (!discardSrec) {
               *ptr++ = (memoryElementType)((data2<<8)+data1); // Assumes little-endian format
               size++;
               nextAddr++;
            }
//          print("%02X",data);
            if (size >= bufferSize) {
               print("loadSRec() - SRECs too large\n");
               return BDM_RC_ILLEGAL_PARAMS;
            }
         }
      }
      // Get checksum from record
      unsigned data = hex2ToDecimal( &pSrec );
      if ((uint8_t)~checkSum != data) {
         print("loadSRec() - SRECs checksum error, expected 0x%02X, found 0x%02X\n", (uint8_t)~checkSum, data);
         return BDM_RC_ILLEGAL_PARAMS;
      }
   }
   print("loadSRec() - Size         = 0x%08X\n", size);
   *loadedSize = size;
   return BDM_RC_OK;
}
