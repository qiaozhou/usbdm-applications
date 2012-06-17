/*
 * bitVector.hpp
 *
 *  Created on: 13/06/2009
 *      Author: podonoghue
 */
#ifndef BITVECTOR_HPP_
#define BITVECTOR_HPP_

//=============================================================================
//
//! \brief A basic bit-vector class
//!
class bitVector {
private:
   unsigned int length;                      //!< Length of the vector in bits
   static const bitVector zeroVector;        //!< A zero length vector
   static const uint8_t bitMasks[];               //!< Bits masks for clipping odd bytes
   static const unsigned int maxVectorLength = 1000;  //!< Maximum length of a vector in bits
   unsigned char data[maxVectorLength/8];    //!< Storage for the vector

public:
   bitVector(unsigned int width=0);
   bitVector(unsigned int width, uint8_t initData[]);
   bitVector(unsigned int width, int initValue);
   void insertField(bitVector &value, unsigned int start);
   bitVector extractField(unsigned int start, unsigned int numBits);

   //! Returns the length of the vector in bits
   unsigned int  getLength(void) const {
      return length;
   }

   //! Returns the internal char array used to store the vector
   //!
   //! @note - The vector is left-justified in the array but the bits are
   //!         right-justified in the bytes i.e. all but the first byte will
   //!         be 8-bits.  The 1st byte may have leading zeroes that are not
   //!         part of the vector.
   //!
   unsigned char *getArray(void) {
      return data;
   }

   char *toHexString(void);
   char *toBinString(void);
   uint8_t operator[](unsigned int index);

}; // class bitVector

#endif /* BITVECTOR_HPP_ */
