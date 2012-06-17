/*
 * SmartPtrs.h
 *
 *  Created on: 10/04/2011
 *      Author: podonoghue
 */

#ifndef SMARTPTRS_H_
#define SMARTPTRS_H_

template <class T>
class SmartPtr {
public:
   explicit SmartPtr(T* pointee) : pointee_(pointee);
   SmartPtr& operator=(const SmartPtr& other);
   ~SmartPtr();
   T& operator*() const
   {

      return *pointee_;
   }
   T* operator->() const
   {

      return pointee_;
   }
private:
   T* pointee_;

};
#endif /* SMARTPTRS_H_ */
