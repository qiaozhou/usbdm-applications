/*
 * Debug.hpp
 *
 *  Created on: 14/06/2009
 *      Author: podonoghue
 */

#ifndef DEBUG_HPP_
#define DEBUG_HPP_

//=========================================================
// Debugging options
#define DEBUG_DEVICES (1<<0) // Add dummy devices to JTAG chain
#define DEBUG_NOTLIVE (1<<2) // Don't use hardware
#define DEBUG 0 //DEBUG_DEVICES|DEBUG_NOTLIVE

#endif /* DEBUG_HPP_ */
