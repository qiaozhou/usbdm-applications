/*
 * GDI_Aux.hpp
 *
 *  Created on: 25/02/2010
 *      Author: podonoghue
 */

#ifndef GDI_AUX_HPP_
#define GDI_AUX_HPP_
#ifndef LOG
inline static const char *getGDIErrorString( DiReturnT returnCode) { return ""; }
#else
const char *getGDIErrorString( DiReturnT returnCode);
#endif

#endif /* GDI_AUX_HPP_ */
