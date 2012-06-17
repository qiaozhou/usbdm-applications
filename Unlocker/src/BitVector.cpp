/*! \file
    \brief Basic bitVector implementation

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

#include "Common.h"
#include "BitVector.h"

//const bitVector bitVector::zeroVector(0);
const bitVector bitVector::zeroVector;

const uint8_t bitVector::bitMasks[] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};

//! \brief Create a zero-filled vector
//!
//! @param width => width in bits of required vector
//!
bitVector::bitVector(unsigned int width) {
   if (width>maxVectorLength)
      width = maxVectorLength;
   for (unsigned int index=0; index < sizeof(data)/sizeof(data[0]); index++)
      data[index] = 0;
   this->length = width;
}

//! \brief Create a vector from an array of char
//!
//! @param width    => width in bits of required vector
//! @param initData => array of data to initialise vector,
//!                    dimension >= length/8
//!
bitVector::bitVector(unsigned int width, uint8_t initData[]) {
   unsigned int index;
   if (width>maxVectorLength)
      width = maxVectorLength;
   for (index=0; index <= (width-1)/8; index++)
      data[index] = initData[index];
   for (;index < sizeof(data)/sizeof(data[0]); index++)
      data[index] = 0;
   this->length = width;
}

//! \brief Create a vector with all bits initialised to a value
//!
//! @param width    => width in bits of required vector
//! @param initData => value to initialise vector bits (0 or 1)
//!
bitVector::bitVector(unsigned int width, int initData) {
   unsigned int index;
   uint8_t initValue = (initData?0xFF:0x00);

   if (width>maxVectorLength)
      width = maxVectorLength;

   for (index=0; index < sizeof(data)/sizeof(data[0]); index++)
      data[index] = initValue;
   this->length = width;
}

//! \brief Extracts a field (vector) from a vector
//!
//! @param start   => bit num for start of field
//! @param numBits => length of field
//!
bitVector bitVector::extractField(unsigned int start, unsigned int numBits) {

   if (numBits == 0) {
//      fprintf(stderr, "extractField(): empty field\n");
      return zeroVector;
   }
   if (numBits>maxVectorLength)
      numBits = maxVectorLength;

   // create destination vector to return
   bitVector *dest = new bitVector(numBits);
   unsigned bitOffset = start%8; // offset to shift source bits right
   unsigned int value;
   unsigned int bitCount;
   unsigned int index;

   for (index = 0, bitCount = 0; bitCount < numBits; index++) {
      unsigned int reverseIndex = (this->length-1)/8 -(start/8)-index;
      value = data[reverseIndex];
      if (reverseIndex>0)
         value += 256*data[reverseIndex-1];
      value = value>>bitOffset;
      dest->data[(numBits-1)/8-index] = (unsigned char)value;
      bitCount += 8;
   }
   return *dest;
}

//! \brief Inserts a field (vector) into a vector
//!
//! @param value => bitVector to insert
//! @param start => location to insert
//!
//! @note - bits or overwritten rather than a true insertion
//!
void bitVector::insertField(bitVector &value, unsigned int start) {
#if 0
   unsigned int numBits = value.length;

   if (numBits == 0) {
      fprintf(stderr, "insertField(): empty field\n");
      return;
   }

   // create destination vector to return
   bitVector *dest = new bitVector(numBits);

   unsigned bitOffset = start%8; // offset to shift source bits right
//   unsigned int value;
   unsigned int bitCount;
   unsigned int index;

   for (index = 0, bitCount = 0; bitCount < numBits; index++) {
      unsigned int reverseIndex = (this->length-1)/8 -(start/8)-index;
      value = data[reverseIndex];
      if (reverseIndex>0)
         value += 256*data[reverseIndex-1];
      value = value>>bitOffset;
      dest->data[(numBits-1)/8-index] = (unsigned char)value;
      bitCount += 8;
   }
#endif
}

//! \brief Provides a string representing the contents of the bitVector
//!        as a HEX number.
//!
//! @note - a shared static buffer is used
//!
char *bitVector::toHexString(void) {
   static char s[4+maxVectorLength/4];
   unsigned int index;
   unsigned char mask = bitMasks[length%8];

   sprintf(s, "%2X", data[0]&mask);

   for (index=1; index <= (length-1)/8; index++) {
      sprintf(&s[2*index], "%2.2X", data[index]);
   }
   return s;
}

//! \brief Provides a string representing the contents of the bitVector
//!        as a binary number.
//!
//! @note - a shared static buffer is used
//!
char *bitVector::toBinString(void) {
   static char s[4+maxVectorLength];
   unsigned int index;
   unsigned char mask = 1<<((length-1)%8);

   char *sp = s;

   for (index=0; 8*index+1 <= length; index++) {
      while (mask != 0) {
         *sp++ = (data[index]&mask)?'1':'0';
         mask >>= 1;
      }
      mask = 0x80;
   }
   *sp++='\0';

   return s;
}

//! \brief Allows the bitVector to be indexed as an array of bits
//!
//! @param bitNum => The bit number of the required bit
//!
uint8_t bitVector::operator[](unsigned int bitNum) {
   if (bitNum>=length)
      return 0;
   unsigned char mask = 1<<(bitNum%8);
   unsigned int index = (length-1)/8 - bitNum/8;

   //fprintf(stderr, "m=%x, indx=%d\n", mask, index);

   if (bitNum >= length)
      return 0;

   return (data[index]&mask)?1:0;
}
