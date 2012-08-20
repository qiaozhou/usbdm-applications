/*! \file
    \brief Low-level USB interface to BDM module .

    \verbatim
    USBDM - USB communication DLL
    Copyright (C) 2008  Peter O'Donoghue

    Based on:
    Turbo BDM Light - USB communication
    Copyright (C) 2005  Daniel Malik

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
   +=========================================================================
   |   6 May 2012 | Added BDM_RC_DEVICE_OPEN_FAILED error messages
   |  31 Mar 2011 | Added command toggle
   |  21 Dec 2010 | Fixed 1-off validation of device number in bdm_usb_open()
   |   8 Nov 2010 | Changed USB retry strategy to use BDM_RC_BUSY
   |  20 Oct 2010 | Added libusb_clear_halt() calls to reset data toggle
   |   1 Aug 2009 | Modified extensively for new format USB comms
   |   1 Aug 2009 | Fixed input range validation in bdm_usb_open()
   |   1 Jan 2009 | Extensive changes to fold TBDML & OSBDM together
   +==========================================================================
    \endverbatim
*/

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include "libusb.h"
#else
#include <libusb-1.0/libusb.h>
#include <time.h>
#include <errno.h>
#endif
#include "Log.h"
#include "hwdesc.h"
#include "Common.h"
#include "USBDM_API.h"
#include "USBDM_API_Private.h"
#include "low_level_usb.h"
#include "Names.h"

#ifndef LIBUSB_SUCCESS
#define LIBUSB_SUCCESS (0)
#endif

#ifdef __unix__
const char *libusb_strerror(int errNo) {
      switch (errNo) {

      /** Success (no error) */
      case LIBUSB_SUCCESS:
         return "LIBUSB_SUCCESS";
      /** Input/output error */
      case LIBUSB_ERROR_IO:
         return "LIBUSB_ERROR_IO";
      /** Invalid parameter */
      case LIBUSB_ERROR_INVALID_PARAM:
         return "LIBUSB_ERROR_INVALID_PARAM";
      /** Access denied (insufficient permissions) */
      case LIBUSB_ERROR_ACCESS:
         return "LIBUSB_ERROR_ACCESS";
      /** No such device (it may have been disconnected) */
      case LIBUSB_ERROR_NO_DEVICE:
         return "LIBUSB_ERROR_NO_DEVICE";
      /** Entity not found */
      case LIBUSB_ERROR_NOT_FOUND:
         return "LIBUSB_ERROR_NOT_FOUND";
      /** Resource busy */
      case LIBUSB_ERROR_BUSY:
         return "LIBUSB_ERROR_BUSY";
      /** Operation timed out */
      case LIBUSB_ERROR_TIMEOUT:
         return "LIBUSB_ERROR_TIMEOUT";
      /** Overflow */
      case LIBUSB_ERROR_OVERFLOW:
         return "LIBUSB_ERROR_OVERFLOW";
      /** Pipe error */
      case LIBUSB_ERROR_PIPE:
         return "LIBUSB_ERROR_PIPE";
      /** System call interrupted (perhaps due to signal) */
      case LIBUSB_ERROR_INTERRUPTED:
         return "LIBUSB_ERROR_INTERRUPTED";
      /** Insufficient memory */
      case LIBUSB_ERROR_NO_MEM:
         return "LIBUSB_ERROR_NO_MEM";
      /** Operation not supported or unimplemented on this platform */
      case LIBUSB_ERROR_NOT_SUPPORTED:
         return "LIBUSB_ERROR_NOT_SUPPORTED";
      /** Other error */
      case LIBUSB_ERROR_OTHER:
         return "LIBUSB_ERROR_OTHER";
      default:
         return "LIBUSB_ERROR_????";
      }
}
#endif

#ifdef LOG
// Uncomment for copious log of low-level USB transactions
//#define LOG_LOW_LEVEL
#endif

#define EP_CONTROL_OUT (LIBUSB_ENDPOINT_OUT|0) //!< EP # for Control Out endpoint
#define EP_CONTROL_IN  (LIBUSB_ENDPOINT_IN |0) //!< EP # for Control In endpoint

#define EP_OUT (LIBUSB_ENDPOINT_OUT|1) //!< EP # for Out endpoint
#define EP_IN  (LIBUSB_ENDPOINT_IN |2) //!< EP # for In endpoint

// Maximum number of BDMs recognized
#define MAX_BDM_DEVICES (10)

static unsigned int timeoutValue = 1000; // us

// Count of devices found
static unsigned deviceCount = 0;

// Pointers to all BDM devices found. Terminated by NULL pointer entry
static struct libusb_device *bdmDevices[MAX_BDM_DEVICES+1] = {NULL};

// Handle of opened device
static libusb_device_handle *usbDeviceHandle = NULL;

// Indicates LIBUSB has been initialised
static bool initialised = FALSE;

//**********************************************************
//!
//! Sleep for given number of milliseconds (or longer!)
//!
//! @param milliSeconds - number of milliseconds to sleep
//!
void milliSleep(int milliSeconds) {
#ifdef __unix__
   int rc;
   struct timespec sleepStruct = { 0, 1000000L*milliSeconds };
   print("milliSleep(%ld ns)\n", sleepStruct.tv_nsec);
   do {
      rc = nanosleep(&sleepStruct, &sleepStruct);
      if ((rc < 0) && (errno == EINTR))
         print("milliSleep() - Sleep interrupted\n");
      else if (sleepStruct.tv_sec > 0)
         print("milliSleep() - time not completed!!\n");
   } while ((rc < 0) && (errno == EINTR));
#else
   Sleep(milliSeconds);
#endif
}

//**********************************************************
//!
//! Initialisation of low-level USB interface
//!
//!  @return\n
//!       BDM_RC_OK        - success \n
//!       BDM_RC_USB_ERROR - various errors
//!
USBDM_ErrorCode bdm_usb_init( void ) {
//   print("bdm_usb_init()\n");

   // Not initialised
   initialised = FALSE;

   // Clear array of devices found so far
   for (int i=0; i<=MAX_BDM_DEVICES; i++) {
      bdmDevices[i]=NULL;  // Clear the list of devices
   }
   deviceCount = 0;

   // Initialise LIBUSB
   if (libusb_init(NULL) != LIBUSB_SUCCESS) {
      print("libusb_init() Failed\n");
      return BDM_RC_USB_ERROR;
   }
   initialised = TRUE;
   return BDM_RC_OK;
}

USBDM_ErrorCode bdm_usb_exit( void ) {
//   print("bdm_usb_exit()\n");

   if (initialised) {
      bdm_usb_close();     // Close any open devices
      print("bdm_usb_exit() - libusb_exit() called\n");
      libusb_exit(NULL);
   }
   initialised = false;
   return BDM_RC_OK;
}

//**********************************************************
//!
//! Release all devices referenced by bdm_usb_findDevices
//!
//!  @return\n
//!       BDM_RC_OK        - success \n
//!
USBDM_ErrorCode bdm_usb_releaseDevices(void) {

//   print("bdm_usb_releaseDevices() - \n");

   // Unreference all devices
   for(unsigned index=0; index<deviceCount; index++) {
      print("bdm_usb_releaseDevices() - unreferencing #%d\n", index);
      if (bdmDevices[index] != NULL)
         libusb_unref_device(bdmDevices[index]);
      bdmDevices[index] = NULL;
   }
   deviceCount = 0;
   return BDM_RC_OK;
}

//**********************************************************
//!
//! Find all USBDM devices attached to the computer
//!
//!  @param deviceCount Number of devices found.  This is set
//!                     to zero on any error.
//!
//!  @return\n
//!       BDM_RC_OK        - success - found at least 1 device \n
//!       BDM_RC_USB_ERROR - no device found or various errors
//!
USBDM_ErrorCode bdm_usb_findDevices(unsigned *devCount) {

//   print("bdm_usb_find_devices()\n");

   *devCount = 0; // Assume failure

   USBDM_ErrorCode rc = BDM_RC_OK;

   // Release any currently referenced devices
   bdm_usb_releaseDevices();

   if (!initialised) {
      print("bdm_usb_find_devices() - Not initialised! \n");
      rc = bdm_usb_init(); // try again
      if (rc != BDM_RC_OK) {
         return rc;
      }
   }
   // discover all USB devices
   libusb_device **list;

   ssize_t cnt = libusb_get_device_list(NULL, &list);
   if (cnt < 0) {
      print("bdm_usb_find_devices() - libusb_get_device_list() failed! \n");
      return BDM_RC_USB_ERROR;
   }

   // Copy the ones we are interested in to our own list
   deviceCount = 0;
   for (int deviceIndex=0; deviceIndex<cnt; deviceIndex++) {
      // Check each device and copy any USBDMs to local list
//      print("bdm_usb_find_devices() ==> checking device #%d\n", deviceIndex);
      libusb_device *currentDevice = list[deviceIndex];
      libusb_device_descriptor deviceDescriptor;
      int rc = libusb_get_device_descriptor(currentDevice, &deviceDescriptor);
      if (rc != LIBUSB_SUCCESS) {
         continue; // Skip device
      }
//      print("bdm_usb_find_devices() ==> Checking device VID=%4.4X, PID=%4.4X\n",
//            deviceDescriptor.idVendor, deviceDescriptor.idProduct);
      if (((deviceDescriptor.idVendor==USBDM_VID)&&(deviceDescriptor.idProduct==USBDM_PID)) ||
          ((deviceDescriptor.idVendor==TBLCF_VID)&&(deviceDescriptor.idProduct==TBLCF_PID)) ||
          ((deviceDescriptor.idVendor==OSBDM_VID)&&(deviceDescriptor.idProduct==OSBDM_PID)) ||
          ((deviceDescriptor.idVendor==TBDML_VID)&&(deviceDescriptor.idProduct==TBDML_PID))) {
         // Found a device
//         print("bdm_usb_find_devices() ==> found USBDM device #%d\n", deviceIndex);
         bdmDevices[deviceCount++] = currentDevice; // Record found device
         libusb_ref_device(currentDevice);          // Reference so we don't lose it
         bdmDevices[deviceCount]=NULL;              // Terminate the list again
         if (deviceCount>MAX_BDM_DEVICES) {
            break;
         }
      }
   }
   // Free the original list (devices referenced above are still referenced)
   libusb_free_device_list(list, true);

   *devCount = deviceCount;

//   print("bdm_usb_find_devices() ==> %d\n", deviceCount);

   if(deviceCount>0) {
      return BDM_RC_OK;
   }
   else {
      return BDM_RC_NO_USBDM_DEVICE;
   }
}

//**********************************************************
//!
//! Get count of USBDM devices found previously
//!
//!  @param deviceCount Number of devices found.  This is set
//!                     to zero on any error.
//!
//!  @return\n
//!       BDM_RC_OK 
//!
USBDM_ErrorCode bdm_usb_getDeviceCount(unsigned int *pDeviceCount) {

   *pDeviceCount = deviceCount;

   return BDM_RC_OK;
}

//**********************************************************
//!
//! Open connection to device enumerated by bdm_usb_find_devices()
//!
//! @param device_no Device number to open
//!
//! @return \n
//!    == BDM_RC_OK (0)     => Success\n
//!    == BDM_RC_USB_ERROR  => USB failure
//!
USBDM_ErrorCode bdm_usb_open( unsigned int device_no ) {

//   print("bdm_usb_open( %d )\n", device_no);

   if (!initialised) {
      print("bdm_usb_open() - Not initialised! \n");
      return PROGRAMMING_RC_ERROR_INTERNAL_CHECK_FAILED;
   }
   if (device_no >= deviceCount) {
      print("bdm_usb_open() - Illegal device #\n");
      return BDM_RC_ILLEGAL_PARAMS;
   }
   if (usbDeviceHandle != NULL) {
      print("bdm_usb_open() - Closing previous device\n");
      bdm_usb_close();
   }
   int rc = libusb_open(bdmDevices[device_no], &usbDeviceHandle);

   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - libusb_open() failed, rc = %s\n", libusb_strerror((libusb_error)rc));
      usbDeviceHandle = NULL;
      return BDM_RC_DEVICE_OPEN_FAILED;
      }
   int configuration = 0;
   rc = libusb_get_configuration(usbDeviceHandle, &configuration);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - libusb_get_configuration() failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }
   if (configuration != 1) {
      rc = libusb_set_configuration(usbDeviceHandle, 1);
      if (rc != LIBUSB_SUCCESS) {
         print("bdm_usb_open() - libusb_set_configuration(1) failed, rc = %s\n", libusb_strerror((libusb_error)rc));
         // Release the device
         libusb_close(usbDeviceHandle);
         usbDeviceHandle = NULL;
         return BDM_RC_DEVICE_OPEN_FAILED;
      }
   }
   rc = libusb_claim_interface(usbDeviceHandle, 0);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - libusb_claim_interface(0) failed, rc = %s\n", libusb_strerror((libusb_error)rc));
      // Release the device
      libusb_set_configuration(usbDeviceHandle, 0);
      libusb_close(usbDeviceHandle);
      usbDeviceHandle = NULL;
      return BDM_RC_DEVICE_OPEN_FAILED;
   }
   rc = libusb_clear_halt(usbDeviceHandle, 0x01);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - libusb_clear_halt(...,0x01) failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }
   rc = libusb_clear_halt(usbDeviceHandle, 0x82);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - libusb_clear_halt(...,0x82) failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }

//   libusb_set_debug(NULL, 4);

//   int maxTransferSize = libusb_get_max_packet_size(libusb_get_device(usbDeviceHandle), EP_OUT);
//   if (maxTransferSize<0) {
//      print("bdm_usb_open() - libusb_get_max_packet_size(EP_OUT) failed, rc = %d\n", maxTransferSize);
//   }
//   else {
//      print("bdm_usb_open() - libusb_get_max_packet_size(EP_OUT) => %d\n", maxTransferSize);
//   }
//
//   maxTransferSize = libusb_get_max_packet_size(libusb_get_device(usbDeviceHandle), EP_IN);
//   if (maxTransferSize<0) {
//      print("bdm_usb_open() - libusb_get_max_packet_size(EP_IN) failed, rc = %d\n", maxTransferSize);
//   }
//   else {
//      print("bdm_usb_open() - libusb_get_max_packet_size(EP_IN) => %d\n", maxTransferSize);
//   }

   return (BDM_RC_OK);
}

//**********************************************************
//!
//! Closes connection to the currently open device
//!
//! @return BDM_RC_OK => success (ignores errors)
//!
USBDM_ErrorCode bdm_usb_close( void ) {
   int rc;

   //   print("bdm_usb_close()\n");
   if (usbDeviceHandle == NULL) {
      print("bdm_usb_close() - device not open - no action\n");
      return BDM_RC_OK;
   }
   rc = libusb_release_interface(usbDeviceHandle, 0);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_close() - libusb_release_interface() failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }
   int configValue;
   rc = libusb_get_configuration(usbDeviceHandle, &configValue);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_close() - libusb_get_configuration() failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }
   // Unconfigure BDM
   // I know the libusb documentation says to use -1 but this ends up being passed
   // to the USB device WHICH IS NOT A GOOD THING!
#ifdef WIN32
   rc = libusb_set_configuration(usbDeviceHandle, 0);
#else
   rc = libusb_set_configuration(usbDeviceHandle, -1);
#endif
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_close() - libusb_set_configuration(0) failed, rc = %s\n", libusb_strerror((libusb_error)rc));
   }
   libusb_close(usbDeviceHandle);
   usbDeviceHandle = NULL;
   return BDM_RC_OK;
}

//**************************************************************************
//!
//! Obtain a string descriptor from currently open BDM
//!
//! @param index              Index of string to obtain
//! @param deviceDescription  Ptr to buffer for descriptor
//! @param maxLength          Size of buffer
//!
//! @return \n
//!    == BDM_RC_OK (0)          => Success\n
//!    == BDM_RC_USB_ERROR       => USB failure
//!
USBDM_ErrorCode bdm_usb_getStringDescriptor(int index, char *descriptorBuffer, unsigned maxLength) {
   const int DT_STRING = 3;

   memset(descriptorBuffer, '\0', maxLength);

   if (usbDeviceHandle == NULL) {
      print("bdm_usb_getStringDescriptor() - Device handle NULL! \n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }
   int rc = libusb_control_transfer(usbDeviceHandle ,
                                    LIBUSB_REQUEST_TYPE_STANDARD|EP_CONTROL_IN, // bmRequestType
                                    LIBUSB_REQUEST_GET_DESCRIPTOR,              // bRequest
                                    (DT_STRING << 8) + index,                   // wValue
                                    0,                                          // wIndex
                                    (unsigned char *)descriptorBuffer,          // data
                                    maxLength,                                  // size
                                    timeoutValue);                              // timeout

   if ((rc < 0) || (descriptorBuffer[1] != DT_STRING)) {
      memset(descriptorBuffer, '\0', maxLength);
      print("bdm_usb_getDeviceDescription() - bdm_usb_getStringDescriptor() failed\n");
      return BDM_RC_USB_ERROR;
   }
//   else {
//      print("bdm_usb_getStringDescriptor() - read %d bytes\n", rc);
//      printDump((unsigned char *)descriptorBuffer, rc);
//   }
   // Determine length
   unsigned length = descriptorBuffer[0];
   if (length>maxLength-2U) {
      length = maxLength-2U;
   }
   // Make sure UTF-16-LE string is terminated
   descriptorBuffer[length+2] = 0;
   descriptorBuffer[length+3] = 0;

   return BDM_RC_OK;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//!
//! \brief Sends a message+data to the USBDM device over EP0
//!
//! No response is expected from device.
//!
//! Since the EP0 transfer is unidirectional in this case, data returned by the
//! device must be read in a separate transfer - this includes any status.
//!
//! @param data \n
//!    data[0]    = N, the number of bytes to send (excluding this byte)\n
//!    data[1]    = Command byte \n
//!    data[2..N] = parameter(s) for command \n
//!
//! @return \n
//!    == BDM_RC_OK (0)     => Success\n
//!    == BDM_RC_USB_ERROR  => USB failure
//!
USBDM_ErrorCode bdm_usb_send_ep0( const unsigned char * data ) {
   unsigned char setupPkt[] = {0,0,0,0,0,0,0};
   unsigned char size       = data[0];   // Size is the first byte
   int rc;
   unsigned index;

   if (usbDeviceHandle == NULL) {
      print("bdm_usb_send_ep0() - Device handle NULL!\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }
#ifdef LOG_LOW_LEVEL
   print("============================\n");
   print("bdm_usb_send_ep0() - USB EP0 send (%s, size=%d):\n", getCommandName(data[1]), size);
   if (data[1] == CMD_USBDM_DEBUG)
      print("bdm_usb_send_ep0() - Debug cmd = %s\n", getDebugCommandName(data[2]));
   printDump(data, size+1);
#endif // LOG_LOW_LEVEL

   // Copy data in case <5 bytes to avoid possible illegal de-referencing
   for(index=0; index<(sizeof(setupPkt)/sizeof(setupPkt[0])); index++) {
      if (index > size)
         break;
      setupPkt[index] = data[index];
   }
   rc=libusb_control_transfer(usbDeviceHandle,
               LIBUSB_REQUEST_TYPE_VENDOR|EP_CONTROL_OUT,      // requestType=Vendor
               setupPkt[1],                                    // request
               setupPkt[2]+(setupPkt[3]<<8),                   // value
               setupPkt[4]+(setupPkt[5]<<8),                   // index
               (unsigned char *)((size>5)?data+6:setupPkt),    // data bytes
               (size>5)?(size-5):0,                            // size (# of data bytes)
               timeoutValue);                                  // how long to wait for reply
   if (rc < 0) {
      print("bdm_usb_send_ep0() - USB EP0 send: Send failed (USB error = %d)\n", rc);
      return(BDM_RC_USB_ERROR);
   }
   return(BDM_RC_OK);
}

//! \brief Sends a message of 5 bytes to the USBDM device over EP0.
//!
//!  An immediate response is expected
//!
//! @param data - Entry \n
//!    data[0]    = N, the number of bytes to receive from the device \n
//!    data[1]    = Command byte \n
//!    data[2..5] = parameter(s) for OSBDM command \n
//! @note data must be an array of at least 5 bytes even if there are no parameters!
//!
//! @param data - Exit \n
//!    data[0]      = cmd response from OSBDM\n
//!    data[1..N-1] = data response from the device (cleared on error)\n
//!
//! @return \n
//!    == BDM_RC_OK (0)     => Success, OK response from device\n
//!    == BDM_RC_USB_ERROR  => USB failure \n
//!    == else              => Error code from Device
//!
USBDM_ErrorCode bdm_usb_recv_ep0(unsigned char *data, unsigned *actualRxSize) {
   unsigned char size = data[0];   // Transfer size is the first byte
   unsigned char cmd  = data[1];   // OSBDM/TBDML Command byte
   int rc;
   int retry = 5;

   *actualRxSize = 0;

   if (usbDeviceHandle == NULL) {
      print("bdm_usb_recv_ep0() - ERROR : Device handle NULL!\n");
      data[0] = BDM_RC_DEVICE_NOT_OPEN;
      return BDM_RC_DEVICE_NOT_OPEN;
   }

#ifdef LOG_LOW_LEVEL
   print("============================\n");
   print("bdm_usb_recv_ep0(%s, size=%d) - \n", getCommandName(cmd), size);
   if (data[1] == CMD_USBDM_DEBUG)
      print("bdm_usb_recv_ep0() - Debug cmd = %s\n", getDebugCommandName(data[2]));
   printDump(data, 6);
#endif // LOG_LOW_LEVEL

   do {
      rc = libusb_control_transfer(usbDeviceHandle,
               LIBUSB_REQUEST_TYPE_VENDOR|EP_CONTROL_IN,      // bmRequestType
               cmd,                                           // bRequest
               data[2]+(data[3]<<8),                          // wValue
               data[4]+(data[5]<<8),                          // wIndex
               (unsigned char*)data,                          // ptr to data buffer (for Rx)
               size,                                          // wLength = size of transfer
               5*timeoutValue                                 // timeout
               );
      if (rc < 0) {
         print("bdm_usb_recv_ep0(size=%d) - Transfer error (USB error = %s) - retry %d \n", size, libusb_strerror((libusb_error)rc), retry);
         milliSleep(100); // So we don't monopolise the USB
      }
   } while ((rc < 0) && (--retry>0));

   if (rc < 0) {
      print("bdm_usb_recv_ep0() - Transfer failed (USB error = %s)\n", libusb_strerror((libusb_error)rc));
      data[0] = BDM_RC_USB_ERROR;
      *actualRxSize = 0;
   }
   else {
      *actualRxSize = (unsigned) rc;
   }
   if ((data[0] != BDM_RC_OK) && (data[0] != cmd)){ // Error at BDM?
      print("bdm_usb_recv_ep0() - Cmd Failed (%s):\n", getErrorName(data[0]));
      printDump(data,*actualRxSize);
      memset(&data[1], 0x00, size-1);
      return (USBDM_ErrorCode)data[0];
   }
#ifdef LOG_LOW_LEVEL
   print("bdm_usb_recv_ep0(size = %d, recvd = %d):\n", size, rc);
   printDump(data,rc);
#endif // LOG_LOW_LEVEL

   return(BDM_RC_OK);
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

/*! \brief Sends a message+data to the Device in ICP mode
 *
 *  No response is expected from device.
 *
 *  Since the EP0 transfer is unidirectional in this case, data returned by the
 *  device must be read in a separate transfer - this includes any status.
 *
 * @param request
 * @param wValue
 * @param wIndex
 * @param size
 * @param data
 *
 * @return
 *   == 0 => USB transfer OK \n
 *   != 0 => USB transfer Failed
 *
*/
USBDM_ErrorCode bdm_usb_raw_send_ep0(unsigned int  request,
                                     unsigned int  wValue,
                                     unsigned int  wIndex,
                                     unsigned int  size,
                                     const unsigned char *data) {
   int rc;
   if (usbDeviceHandle == NULL) {
      print("bdm_usb_raw_send_ep0() - device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }
#ifdef LOG_LOW_LEVEL
   print("============================\n");
   print("bdm_usb_raw_send_ep0(req=%2.2X, val=%2.2X, ind=%d, size=%d)\n",
         request, wValue, wIndex, size);
#endif
   rc = libusb_control_transfer(usbDeviceHandle,
            LIBUSB_REQUEST_TYPE_VENDOR|EP_CONTROL_OUT,      // bmRequestType
            request,                                        // bRequest
            wValue,                                         // value
            wIndex,                                         // index
            (unsigned char*)data,                           // data
            size,                                           // size (# of data bytes)
            timeoutValue);                                  // how long to wait for reply
   if (rc < 0) {
      print("bdm_usb_raw_send_ep0() - Send failed (USB error = %s)\n", libusb_strerror((libusb_error)rc));
      return(BDM_RC_USB_ERROR);
   }
   return(BDM_RC_OK);
}

//! \brief Sends a message+data to the Device in ICP mode
//!
//!  An immediate response is expected from device.
//!
//!  Since the EP0 transfer is unidirectional in this case, data returned by the
//!  device must be read in a separate transfer - this includes any status.
//!
//!  @param request
//!  @param wValue
//!  @param wIndex
//!  @param size
//!  @param data
//!
//!  @return \n
//!     == BDM_RC_OK (0)     => Success, OK response from device\n
//!     == BDM_RC_USB_ERROR  => USB failure \n
//!     == else              => Error code from Device
//!
USBDM_ErrorCode bdm_usb_raw_recv_ep0(unsigned int  request,
                                     unsigned int  wValue,
                                     unsigned int  wIndex,
                                     unsigned int  size,
                                     unsigned char *data) {
   int rc;
   if (usbDeviceHandle == NULL) {
      print("bdm_usb_raw_recv_ep0() - device not open\n");
      data[0] = BDM_RC_DEVICE_NOT_OPEN;
      return BDM_RC_DEVICE_NOT_OPEN;
   }
#ifdef LOG_LOW_LEVEL
   print("============================\n");
   print("bdm_usb_raw_recv_ep0(req=%2.2X, val=%2.2X, ind=%d, size=%d)\n",
         request, wValue, wIndex, size);
#endif // LOG_LOW_LEVEL

   rc = libusb_control_transfer(usbDeviceHandle,
            LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_ENDPOINT_IN,  // bmRequestType
            request,                                        // bRequest
            wValue,                                         // value
            wIndex,                                         // index
            (unsigned char*)data,                           // data
            size,                                           // size (# of data bytes)
            timeoutValue);                                  // how long to wait for reply

   if (rc < 0) {
      print("bdm_usb_raw_recv_ep0() - Transaction failed (USB error = %s)\n", libusb_strerror((libusb_error)rc));
      data[0] = BDM_RC_USB_ERROR;
   }
//#ifdef LOG_LOW_LEVEL
//   else
//      printDump(data, rc);
//#endif
   return (USBDM_ErrorCode)data[0];
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//! \brief Sends a command to the TBDML device over the Out Bulk Endpoint
//!
//! Since the EP transfer is unidirectional, data returned by the
//! device must be read in a separate transfer - this includes any status
//! from extended command execution.
//!
//! @param count = # of bytes to Tx (N)
//!
//! @param data                                 \n
//!    OUT                                      \n
//!    =======================================  \n
//!    data[0]      = reserved                  \n
//!    data[1]      = command byte              \n
//!    data[2..N-1] = parameters                \n
//!                                             \n
//!    IN                                       \n
//!    ======================================== \n
//!    data[0]      = error code (=rc)
//!
//! @return
//!   BDM_RC_OK        => USB transfer OK \n
//!   BDM_RC_USB_ERROR => USB transfer Failed
//!
USBDM_ErrorCode bdm_usb_send_epOut(unsigned int count, const unsigned char *data) {
   int rc;
   int transferCount;

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_send_epOut(): device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }

#ifdef LOG_LOW_LEVEL
   print("============================\n");
   if (data[0] == 0) { // Zero as first byte indicates split USBDM command
      print("bdm_usb_send_epOut() - USB EP0ut send (SPLIT, size=%d):\n", count);
   }
   else {
      print("bdm_usb_send_epOut() - USB EP0ut send (%s, size=%d):\n", getCommandName(data[1]), count);
      if (data[1] == CMD_USBDM_DEBUG)
         print("bdm_usb_send_epOut() - Debug cmd = %s\n", getDebugCommandName(data[2]));
   }
   printDump(data, count);
#endif // LOG_LOW_LEVEL

   rc = libusb_bulk_transfer(usbDeviceHandle,
                       EP_OUT,                  // Endpoint & direction
                       (unsigned char *)data,   // ptr to Tx data
                       count,                   // number of bytes to Tx
                       &transferCount,          // number of bytes actually Tx
                       timeoutValue             // timeout
                       );
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_send_epOut() - Transfer failed (USB error = %s, timeout=%d)\n", libusb_strerror((libusb_error)rc), timeoutValue);
      return BDM_RC_USB_ERROR;
   }
   return BDM_RC_OK;
}

//! Temporary buffer for IN transactions
//!
unsigned char dummyBuffer[512];

//! \brief Receives a response from the USBDM device over the In Bulk Endpoint
//! Responses are retried to allow for target execution
//!
//! @param count = Maximum number of bytes to receive
//!
//! @param data
//!    IN                                       \n
//!    ======================================== \n
//!    data[0]      = error code (= rc)         \n
//!    data[1..N]   = data received             \n
//!
//! @param actualCount = Actual number of bytes received
//!
//! @return                                                       \n
//!    == BDM_RC_OK (0)     => Success, OK response from device   \n
//!    == BDM_RC_USB_ERROR  => USB failure                        \n
//!    == else              => Error code from Device
//!
USBDM_ErrorCode bdm_usb_recv_epIn(unsigned count, unsigned char *data, unsigned *actualCount) {
   int rc;
   int retry = 40; // ~4s of retries
   int transferCount;

   *actualCount = 0; // Assume failure

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_recv_epIn(): device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }

#ifdef LOG_LOW_LEVEL
   print("bdm_usb_recv_epIn(%d, ...)\n", count);
#endif // LOG_LOW_LEVEL

#if OLD
   do {
      rc = libusb_bulk_transfer(usbDeviceHandle,
                          EP_IN,                         // Endpoint & direction
                          (unsigned char *)dummyBuffer,  // ptr to Rx data
                          sizeof(dummyBuffer)-5,         // number of bytes to Rx
                          &transferCount,                // number of bytes actually Rx
                          timeoutValue                   // timeout
                          );
      if (rc != LIBUSB_SUCCESS) {
         print("bdm_usb_recv_epIn(count=%d) - Transfer timeout (USB error = %s, timeout=%d) - retry\n", count, libusb_strerror((libusb_error)rc), timeoutValue);
         milliSleep(100); // So we don't monopolise the USB
      }
   } while ((rc!=LIBUSB_SUCCESS) && (--retry>0));
#else
   do {
      rc = libusb_bulk_transfer(usbDeviceHandle,
                                EP_IN,                         // Endpoint & direction
                                (unsigned char *)dummyBuffer,  // ptr to Rx data
                                sizeof(dummyBuffer)-5,         // number of bytes to Rx
                                &transferCount,                // number of bytes actually Rx
                                timeoutValue                   // timeout
                                );
      if ((rc == LIBUSB_SUCCESS)  && (dummyBuffer[0] == BDM_RC_BUSY)) {
         // The BDM has indicated it's busy for a while - try again in 1/10 second
         print("bdm_usb_recv_epIn(retry=%d) - BDM Busy - retry\n", retry);
         milliSleep(100); // So we don't monopolise the USB
      }
   } while ((rc == LIBUSB_SUCCESS) && (dummyBuffer[0] == BDM_RC_BUSY)  && (--retry>0));
#endif
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_recv_epIn(count=%d) - Transfer failed (USB error = %s)\n", count, libusb_strerror((libusb_error)rc));
      data[0] = BDM_RC_USB_ERROR;
      memset(&data[1], 0x00, count-1);
      return BDM_RC_USB_ERROR;
   }

   memcpy(data, dummyBuffer, transferCount);
   rc = data[0]&0x7F;

   if (rc != BDM_RC_OK) {
      print("bdm_usb_recv_epIn() - Error Return %d (%s):\n", rc, getErrorName(rc));
      print("bdm_usb_recv_epIn(size = %d, recvd = %d):\n", count, transferCount);
      printDump(data, transferCount);
      memset(&data[1], 0x00, count-1);
   }

#ifdef LOG_LOW_LEVEL
   printDump(data, transferCount);
#endif // LOG_LOW_LEVEL

   *actualCount = transferCount;

   return (USBDM_ErrorCode)(rc);
}

//! \brief Executes an USB transaction.
//! This consists of a transmission of a command and reception of the response
//! JB16 Version - see \ref bdm_usb_transaction()
//!
static
USBDM_ErrorCode bdmJB16_usb_transaction( unsigned int   txSize,
                                         unsigned int   rxSize,
                                         unsigned char *data,
                                         unsigned int  *actualRxSize) {
   USBDM_ErrorCode rc;

   if (txSize <= 5) {
      // Transmission fits in SETUP pkt, Use single IN Data transfer to/from EP0
      *data = rxSize;
      rc = bdm_usb_recv_ep0( data, actualRxSize);
   }
   else {
      // Transmission requires separate IN transaction
      *data = txSize;
      rc = bdm_usb_send_ep0( (const unsigned char *)data );
      if (rc == BDM_RC_OK) {
         // Get response
         data[0] = rxSize;
         data[1] = CMD_USBDM_GET_COMMAND_RESPONSE; // dummy command
         rc = bdm_usb_recv_ep0(data, actualRxSize);
      }
   }
   if (rc != BDM_RC_OK) {
      data[0] = rc;
      memset(&data[1], 0x00, rxSize-1);
   }
   return rc;
}

//! \brief Executes an USB transaction.
//! This consists of a transmission of a command and reception of the response
//! JMxx Version - see \ref bdm_usb_transaction()
//!
static
USBDM_ErrorCode bdmJMxx_usb_transaction( unsigned int   txSize,
                                         unsigned int   rxSize,
                                         unsigned char *data,
                                         unsigned int  *actualRxSize) {
   USBDM_ErrorCode rc;
   const unsigned MaxFirstTransaction = 30;
   static bool commandToggle = 0;
   unsigned retry=6;
   static int sequence = 0;
   bool reportFlag = false;
   uint8_t sendBuffer[txSize];

   if (data[1] == CMD_USBDM_GET_CAPABILITIES) {
//      print("bdmJMxx_usb_transaction() Setting toggle=0\n");
      commandToggle = 0;
      sequence = 0;
   }
   sequence++;
   memcpy(sendBuffer, data, txSize);
   do {
      // An initial transaction of up to MaxFirstTransaction bytes is sent
      // This is guaranteed to fit in a single pkt (<= endpoint MAXPACKETSIZE)
      sendBuffer[0] = txSize;
      if (commandToggle) {
         sendBuffer[1] |= 0x80;
      }
      else {
         sendBuffer[1] &= ~0x80;
      }
      rc = bdm_usb_send_epOut(txSize>MaxFirstTransaction?MaxFirstTransaction:txSize,
                              (const unsigned char *)sendBuffer);

      if (rc != BDM_RC_OK) {
         reportFlag = true;
         print("bdmJMxx_usb_transaction() Tx1 failed\n");
         continue;
      }
      // Remainder of data (if any) is sent as 2nd transaction
      // The size of this transaction is know to the receiver
      // Zero is sent as first byte (size) to allow differentiation from 1st transaction
      if ((rc == BDM_RC_OK) && (txSize>MaxFirstTransaction)) {
         print("bdmJMxx_usb_transaction() Splitting command\n");
         uint8_t saveByte = sendBuffer[MaxFirstTransaction-1];
         sendBuffer[MaxFirstTransaction-1] = 0; // Marker indicating later transaction
         rc = bdm_usb_send_epOut(txSize+1-MaxFirstTransaction,
               (const unsigned char *)(sendBuffer+MaxFirstTransaction-1));
         sendBuffer[MaxFirstTransaction-1] = saveByte;
      }
      if (rc != BDM_RC_OK) {
         reportFlag = true;
         print("bdmJMxx_usb_transaction() Tx2 failed\n");
         continue;
      }
      // Get response
      rc = bdm_usb_recv_epIn(rxSize, data, actualRxSize);
      bool receivedCommandToggle = (data[0]&0x80) != 0;

      if ((rc == BDM_RC_USB_ERROR) || (commandToggle != receivedCommandToggle)) {
         // Retry on single USB fail or toggle error
         print("bdmJMxx_usb_transaction() USB or Toggle error, seq = %d, S=%d, R=%d\n", sequence, commandToggle?1:0, receivedCommandToggle?1:0);
         milliSleep(100);
         rc = bdm_usb_recv_epIn(rxSize, data, actualRxSize);
         receivedCommandToggle = (data[0]&0x80) != 0;
         if ((rc == BDM_RC_USB_ERROR) || (commandToggle != receivedCommandToggle)) {
            print("bdmJMxx_usb_transaction() Immediate retry failed, seq = %d, S=%d, R=%d\n", sequence, commandToggle?1:0, receivedCommandToggle?1:0);
         }
         else {
            print("bdmJMxx_usb_transaction() Immediate retry succeeded, seq = %d, S=%d, R=%d\n", sequence, commandToggle?1:0, receivedCommandToggle?1:0);
         }
      }
      // Mask toggle bit out of data
      data[0] &= ~0x80;

      if (rc == BDM_RC_USB_ERROR) {
         // Retry entire command
         reportFlag = true;
         print("bdmJMxx_usb_transaction() USB error, seq = %d, retrying command\n", sequence);
         continue;
      }
      // Don't toggle on busy -
      if (rc == BDM_RC_BUSY) {
         reportFlag = true;
         print("bdmJMxx_usb_transaction() BUSY response, seq = %d\n", sequence);
         continue;
      }
      if (reportFlag) {
         print("bdmJMxx_usb_transaction() Report, seq = %d, S=%d, R=%d\n", sequence, commandToggle?1:0, receivedCommandToggle?1:0);
      }
      commandToggle = !commandToggle;

      if (rc != BDM_RC_OK) {
         data[0] = rc;
         *actualRxSize = 0;
         memset(&data[1], 0x00, rxSize-1);
      }
   } while ((rc == BDM_RC_USB_ERROR) && (retry-->0));

   return rc;
}

//! \brief Executes an USB transaction.
//! This consists of a transmission of a command and reception of the response
//!
//! @param txSize = size of transmitted packet
//!
//! @param rxSize = maximum size of received packet
//!
//! @param data   = IN/OUT buffer for data                           \n
//!    Transmission                                                  \n
//!    ===================================                           \n
//!    data[0]    = reserved for USB layer                           \n
//!    data[1]    = command                                          \n
//!    data[2..N] = data                                             \n
//!                                                                  \n
//!    Reception                                                     \n
//!    ============================================================= \n
//!    data[0]    = response code (error code - see \ref USBDM_ErrorCode) \n
//!    data[1..N] = command response
//!
//! @param timeout      = timeout in ms
//! @param actualRxSize = number of bytes actually received (may be NULL if not required)
//!
//! @return                                                          \n
//!    == BDM_RC_OK (0)     => Success, OK response from device      \n
//!    == BDM_RC_USB_ERROR  => USB failure                           \n
//!    == else              => Error code from BDM
//!
USBDM_ErrorCode bdm_usb_transaction( unsigned int   txSize,
                                     unsigned int   rxSize,
                                     unsigned char *data,
                                     unsigned int   timeout,
                                     unsigned int  *actualRxSize) {
   USBDM_ErrorCode rc;
   unsigned tempRxSize;
   uint8_t command = data[1];

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_transaction(): device not open\n");
	  return BDM_RC_DEVICE_NOT_OPEN;
   }
   timeoutValue = timeout;

   if (bdmState.useOnlyEp0) {
      rc = bdmJB16_usb_transaction( txSize, rxSize, data, &tempRxSize);
   }
   else {
      rc = bdmJMxx_usb_transaction( txSize, rxSize, data, &tempRxSize);
   }
   if (actualRxSize != NULL) {
      // Variable size data expected
      *actualRxSize = tempRxSize;
   }
   else if ((rc == 0) && (tempRxSize != rxSize)) {
      // Expect exact data size
      print("bdm_usb_transaction() - cmd = %s, Expected %d; received %d\n",
            getCommandName(command), rxSize, tempRxSize);
      rc = BDM_RC_UNEXPECTED_RESPONSE;
   }
   if (rc != BDM_RC_OK) {
      print("bdm_usb_transaction() - Failed, cmd = %s, rc = %s\n",
            getCommandName(command), getErrorName(rc));
   }
   return rc;
}
