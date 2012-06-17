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
   +====================================================================
   |   1 Aug 2009 | Modified extensively for new format USB comms
   |   1 Aug 2009 | Fixed input range validation in bdm_usb_open()
   |   1 Jan 2009 | Extensive changes to fold TBDML & OSBDM together
   +====================================================================
    \endverbatim
*/
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include "libusbV0.h"
#else
#error This include file is for windows only!
#endif
#include "Log.h"
#include "hwdesc.h"
#include "Common.h"
#include "USBDM_API.h"
#include "USBDM_API_Private.h"
#include "low_level_usb.h"
#include "Names.h"
//#include "ICP.h"

#ifndef LIBUSB_SUCCESS
#define LIBUSB_SUCCESS (0)
#endif

#ifdef LOG
// Uncomment for copious log of low-level USB transactions
//#define LOG_LOW_LEVEL
#endif

#define EP_CONTROL_OUT (USB_ENDPOINT_OUT|0) //!< EP # for Control Out endpoint
#define EP_CONTROL_IN  (USB_ENDPOINT_IN |0) //!< EP # for Control In endpoint

#define EP_OUT (USB_ENDPOINT_OUT|1) //!< EP # for Out endpoint
#define EP_IN  (USB_ENDPOINT_IN |2) //!< EP # for In endpoint

// Maximum number of BDMs recognized
#define MAX_BDM_DEVICES (10)

static unsigned int timeoutValue = 1000; // us

// Count of devices found
static unsigned deviceCount = 0;

// Pointers to all BDM devices found. Terminated by NULL pointer entry
static struct usb_device *bdmDevices[MAX_BDM_DEVICES+1] = {NULL};

// Handle of opened device
static usb_dev_handle    *usbDeviceHandle      = NULL;

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
   struct timespec sleepStruct = { 0, 1000*milliSeconds };
   nanosleep(&sleepStruct, NULL);
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
   struct usb_version *version;

   print("===========================================\n");
   print("bdm_usb_init()\n");

   // Not initialised
   initialised = FALSE;

   // Clear array of devices found so far
   for (int i=0; i<=MAX_BDM_DEVICES; i++)
      bdmDevices[i]=NULL;  // Clear the list of devices

   deviceCount = 0;

   // Initialise LIBUSB
   usb_init();

   version = usb_get_version();
   if (version == NULL) {
      print("bdm_usb_init() - usb_get_version() failed! \n");
      return BDM_RC_USB_ERROR;
   }

   print( "USBLIB DLL version: %i.%i.%i.%i\n",
          version->dll.major,  version->dll.minor,
          version->dll.micro,  version->dll.nano);

   if (version->driver.major == -1) {
      // driver not running!
      print("bdm_usb_init() - LibUSB Driver not running\n");
      return BDM_RC_USB_ERROR;
   }

   print("USBLIB Driver version: %i.%i.%i.%i\n",version->driver.major,  version->driver.minor,
                                                version->driver.micro,  version->driver.nano);
   print("===========================================\n");

   initialised = TRUE;
   return BDM_RC_OK;
}

USBDM_ErrorCode bdm_usb_exit( void ) {
   print("===========================================\n");
   print("bdm_usb_exit()\n");

  if (initialised) {
      bdm_usb_close();     // Close any open devices
      print("bdm_usb_exit() - libusb_exit() called\n");
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

   print("bdm_usb_releaseDevices() - \n");

   // Unreference all devices
   for(unsigned index=0; index<deviceCount; index++) {
//      print("bdm_usb_releaseDevices() - unreferencing #%d\n", index);
//      if (bdmDevices[index] != NULL)
//         libusb_unref_device(bdmDevices[index]);
      bdmDevices[index] = NULL;
   }
   deviceCount = 0;
   return BDM_RC_OK;
}

//**********************************************************
//!
//! Find all OSBDM/TBDML devices attached to the computer
//!
//!  @param deviceCount Number of devices found.  This is set
//!                     to zero on any error.
//!
//!  @return\n
//!       BDM_RC_OK        - success - found at least 1 device \n
//!       BDM_RC_USB_ERROR - no device found or various errors
//!
USBDM_ErrorCode bdm_usb_findDevices(unsigned *devCount) {
   struct usb_bus     *libusb_bus;
   struct usb_device  *libusb_dev;
   unsigned int i=0;

   print("===========================================\n");
   print("bdm_usb_find_devices()\n");

   *devCount = 0; // Assume failure

//   USBDM_ErrorCode rc = BDM_RC_OK;

   // Release any currently referenced devices
   bdm_usb_releaseDevices();

   if (!initialised) {
      print("bdm_usb_find_devices() - Not initialised! \n");
      bdm_usb_init(); // try again
      }

   if (!initialised)
      return BDM_RC_USB_ERROR;

   // discover all USB devices
   usb_find_busses();      // enumerate all busses
   usb_find_devices();     // enumerate all devices
   i = 0;
   for (libusb_bus = usb_get_busses(); libusb_bus; libusb_bus = libusb_bus->next) {    /* scan through all busses */
      for (libusb_dev = libusb_bus->devices; libusb_dev; libusb_dev = libusb_dev->next) { /* scan through all devices */
         if (
             ((libusb_dev->descriptor.idVendor==USBDM_VID)&&(libusb_dev->descriptor.idProduct==USBDM_PID)) ||
             ((libusb_dev->descriptor.idVendor==TBLCF_VID)&&(libusb_dev->descriptor.idProduct==TBLCF_PID)) ||
             ((libusb_dev->descriptor.idVendor==OSBDM_VID)&&(libusb_dev->descriptor.idProduct==OSBDM_PID)) ||
             ((libusb_dev->descriptor.idVendor==TBDML_VID)&&(libusb_dev->descriptor.idProduct==TBDML_PID))
             ) {
            /* found a device */
            deviceCount++;
            bdmDevices[i++]=libusb_dev;  /* add pointer to the device */
            bdmDevices[i]=NULL;          /* terminate the list again */
            if (i>=(sizeof(bdmDevices)/sizeof(struct usb_device*)))
               return BDM_RC_USB_ERROR;   /* too many devices! */
         }
      }
   }
   *devCount = deviceCount;

   print("===========================================\n");
   print("bdm_usb_find_devices() ==> %d\n", deviceCount);

   if(deviceCount>0)
      return BDM_RC_OK;
   else
      return BDM_RC_NO_USBDM_DEVICE;
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

   print("===========================================\n");
   print("bdm_usb_open( %d )\n", device_no);

   if (!initialised) {
      print("bdm_usb_open() - Not initialised! \n");
      return BDM_RC_USB_ERROR;
   }
   if (device_no > deviceCount) {
      print("bdm_usb_open() - Illegal device #\n");
      return BDM_RC_ILLEGAL_PARAMS;
   }
   if (usbDeviceHandle != NULL) {
      bdm_usb_close();
   }
   usbDeviceHandle  = usb_open(bdmDevices[device_no]);
   if (usbDeviceHandle ==NULL) {
      print("bdm_usb_open() - Failed to open device\n");
      return BDM_RC_USB_ERROR;
      }
   else
      print("bdm_usb_open() - libusb_open() OK\n");

   int rc = usb_set_configuration(usbDeviceHandle, 1);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - Failed set configuration, rc = %d\n", rc);
      usb_close(usbDeviceHandle);
      usbDeviceHandle = NULL;
      return BDM_RC_USB_ERROR;
   }
   else
      print("bdm_usb_open() - usb_set_configuration(..,0) OK\n");
   rc = usb_claim_interface(usbDeviceHandle, 0);
   if (rc != LIBUSB_SUCCESS) {
      print("bdm_usb_open() - Failed to claim interface, rc = %d\n", rc);
      // Release the device
      usb_close(usbDeviceHandle);
      usbDeviceHandle = NULL;
      return BDM_RC_USB_ERROR;
   }
   else
      print("bdm_usb_open() - libusb_claim_interface(..,0) OK\n");


   print("bdm_usb_open() - success\n");
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

   print("===========================================\n");
   print("bdm_usb_close()\n");
   if (usbDeviceHandle != NULL) {
      rc = usb_release_interface(usbDeviceHandle, 0);   // Release the interface
      if (rc != LIBUSB_SUCCESS)
         print("bdm_usb_close() - libusb_release_interface() failed, rc = %d\n", rc);
      else
         print("bdm_usb_close() - libusb_release_interface() OK\n");

      rc = usb_set_configuration(usbDeviceHandle, 0);   // Un-set the configuration
      if (rc != LIBUSB_SUCCESS)
         print("bdm_usb_close() - libusb_set_configuration(0) failed, rc = %d\n", rc);
      else
         print("bdm_usb_close() - libusb_set_configuration(0) OK\n");
      usb_close(usbDeviceHandle );                 // Close the device
      usbDeviceHandle = NULL;
   }

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
   int rc = usb_control_msg(usbDeviceHandle ,
                      USB_ENDPOINT_IN|USB_TYPE_STANDARD, // bmRequestType
                      USB_REQ_GET_DESCRIPTOR,            // bRequest
                      (DT_STRING << 8) + index,          // wValue
                      0,                                 // wIndex
                      (char *)descriptorBuffer,          // data
                      maxLength,                         // size
                      timeoutValue);                          // timeout

   if ((rc < 0) || (descriptorBuffer[1] != DT_STRING)) {
      memset(descriptorBuffer, '\0', maxLength);
      print("bdm_usb_getDeviceDescription() - bdm_usb_getStringDescriptor() failed\n");
      return BDM_RC_USB_ERROR;
   }
   else {
      print("bdm_usb_getStringDescriptor() - read %d bytes\n", rc);
      printDump((unsigned char *)descriptorBuffer, rc);
   }
   // Determine length
   unsigned length = descriptorBuffer[0];
   if (length>maxLength-2U)
      length = maxLength-2U;

   // Make sure UTF-16-LE string is terminated
   descriptorBuffer[length+2] = 0;
   descriptorBuffer[length+3] = 0;

   print("bdm_usb_getDeviceDescription() => OK\n"); // Too hard to print UTF-16-LE!

   return BDM_RC_OK;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//!
//! \brief Sends a message+data to the TBDML/OSBDM device over EP0
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
   rc=usb_control_msg(usbDeviceHandle ,
               USB_TYPE_VENDOR|USB_ENDPOINT_OUT,               // requestType=Vendor
               setupPkt[1],                                    // request
               setupPkt[2]+(setupPkt[3]<<8),                   // value
               setupPkt[4]+(setupPkt[5]<<8),                   // index
               (char *)((size>5)?data+6:setupPkt),             // data bytes
               (size>5)?(size-5):0,                            // size (# of data bytes)
               timeoutValue);                                       // how long to wait for reply
   if (rc < 0) {
      print("bdm_usb_send_ep0() - USB EP0 send: Send failed (USB error = %d)\n", rc);
      return(BDM_RC_USB_ERROR);
   }
   return(BDM_RC_OK);
}

//! \brief Sends a message of 5 bytes to the TBDML/OSBDM device over EP0.
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
      print("============================\n");
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
      rc=usb_control_msg(usbDeviceHandle ,
               USB_TYPE_VENDOR|USB_ENDPOINT_IN,               // bmRequestType
               cmd,                                           // bRequest
               data[2]+(data[3]<<8),                          // wValue
               data[4]+(data[5]<<8),                          // wIndex
               (char*)data,                                   // ptr to data buffer (for Rx)
               size,                                          // wLength = size of transfer
               5*timeoutValue                                 // timeout
               );
      if (rc < 0) {
         print("bdm_usb_recv_ep0(size=%d) - Transfer error (USB error = %d) - retry %d \n", size, rc, retry);
         milliSleep(100); // So we don't monopolise the USB
      }
   } while ((rc < 0) && (--retry>0));

   if (rc < 0) {
      print("bdm_usb_recv_ep0() - Transfer failed (USB error = %d)\n", rc);
      data[0] = BDM_RC_USB_ERROR;
      *actualRxSize = 0;
   }
   else
      *actualRxSize = (unsigned) rc;
   if ((data[0] != BDM_RC_OK) && (data[0] != cmd)){ // Error at BDM?
      print("bdm_usb_recv_ep0() - Cmd Failed (%s):\n", getErrorName(data[0]));
      printDump(data,*actualRxSize);
      memset(&data[1], 0x00, size-1);
      return (USBDM_ErrorCode)data[0];
   }
#ifdef LOG_LOW_LEVEL
   print("bdm_usb_recv_ep0(size = %d, recvd = %d):\n", size, rxSize);
   printDump(data,rxSize);
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
      print("bdm_usb_raw_send_ep0() - USB EP0 raw send: device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }
   print("============================\n");
   print("bdm_usb_raw_send_ep0(req=%2.2X, val=%2.2X, ind=%d, size=%d)\n",
         request, wValue, wIndex, size);
   rc=usb_control_msg(usbDeviceHandle ,
            USB_TYPE_VENDOR|USB_ENDPOINT_OUT,               // bmRequestType
            request,                                        // bRequest
            wValue,                                         // value
            wIndex,                                         // index
            (char*)data,                                    // data
            size,                                           // size (# of data bytes)
            timeoutValue);                                       // how long to wait for reply
   if (rc < 0) {
      print("bdm_usb_raw_send_ep0() - Send failed (USB error = %d)\n", rc);
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
   print("============================\n");
   if (usbDeviceHandle == NULL) {
      print("bdm_usb_raw_recv_ep0() - device not open\n");
      data[0] = BDM_RC_DEVICE_NOT_OPEN;
      return BDM_RC_DEVICE_NOT_OPEN;
   }
#ifdef LOG_LOW_LEVEL
   print("bdm_usb_raw_recv_ep0(req=%2.2X, val=%2.2X, ind=%d, size=%d)\n",
         request, wValue, wIndex, size);
#endif // LOG_LOW_LEVEL

   rc=usb_control_msg(usbDeviceHandle ,
            USB_TYPE_VENDOR|USB_ENDPOINT_IN,                // bmRequestType
            request,                                        // bRequest
            wValue,                                         // value
            wIndex,                                         // index
            (char*)data,                                    // data
            size,                                           // size (# of data bytes)
            timeoutValue);                                       // how long to wait for reply

   if (rc < 0) {
      print("bdm_usb_raw_recv_ep0() - Transaction failed (USB error = %d)\n", rc);
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

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_send_epOut(): device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }

#ifdef LOG_LOW_LEVEL
   print("============================\n");
   print("bdm_usb_send_epOut() - USB EP0ut send (%s, size=%d):\n", getCommandName(data[1]), count);
   if (data[1] == CMD_USBDM_DEBUG)
      print("bdm_usb_send_epOut() - Debug cmd = %s\n", getDebugCommandName(data[2]));
   printDump(data, count);
#endif // LOG_LOW_LEVEL

   rc = usb_bulk_write(usbDeviceHandle ,
                       EP_OUT,                  // Endpoint & direction
                       (char *)data,             // ptr to Tx data
                       count,                   // number of bytes to Tx
                       timeoutValue                  // timeout
                       );
   if (rc<0) {
      print("bdm_usb_send_epOut() - Transfer failed (USB error = %d)\n", rc);
      return BDM_RC_USB_ERROR;
   }
   return BDM_RC_OK;
}

//! Temporary buffer for IN transactions
//!
unsigned char dummyBuffer[200];

//! \brief Receives a response from the TBDML device over the In Bulk Endpoint
//! Responses are retried to allow for target execution
//!
//! @param count = Maximum number of bytes to receive
//!
//! @param data
//!    IN                                       \n
//!    ======================================== \n
//!    data[0]      = error code (=rc)          \n
//!    data[1..N]   = data received             \n
//!
//! @return                                                       \n
//!    == BDM_RC_OK (0)     => Success, OK response from device   \n
//!    == BDM_RC_USB_ERROR  => USB failure                        \n
//!    == else              => Error code from Device
//!
USBDM_ErrorCode bdm_usb_recv_epIn(unsigned count, unsigned char *data, unsigned *actualCount) {
   int retry = 5;
   int transferCount;

   *actualCount = 0; // Assume failure

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_recv_epIn(): device not open\n");
      return BDM_RC_DEVICE_NOT_OPEN;
   }

#ifdef LOG_LOW_LEVEL
   print("bdm_usb_recv_epIn(%d, ...)\n", count);
#endif // LOG_LOW_LEVEL

   do {
      transferCount = usb_bulk_read(usbDeviceHandle ,
                          EP_IN,                         // Endpoint & direction
                          (char *)dummyBuffer,           // ptr to Rx data
                          sizeof(dummyBuffer)-5,         // number of bytes to Tx
                          timeoutValue                        // timeout
                          );
      if (transferCount<0) {
         print("bdm_usb_recv_epIn(count=%d) - Transfer timeout (USB error = %d) - retry\n", count, transferCount);
         Sleep(100); // So we don't monopolise the USB
      }
   } while ((transferCount<0) && (--retry>0));

   if (transferCount<0) {
      print("bdm_usb_recv_epIn(count=%d) - Transfer failed (USB error = %d)\n", count, transferCount);
      data[0] = BDM_RC_USB_ERROR;
      memset(&data[1], 0x00, count-1);
      return BDM_RC_USB_ERROR;
   }

   if ((unsigned)transferCount != count) {
      print("bdm_usb_recv_epIn() - Expected %d; received %d\n", count, transferCount);
   }
   memcpy(data, dummyBuffer, transferCount);

   if (data[0] != BDM_RC_OK) { // Error?
      print("bdm_usb_recv_epIn() - Error Return (%s):\n", getErrorName(data[0]));
      print("bdm_usb_recv_epIn(size = %d, recvd = %d):\n", count, transferCount);
      printDump(data, transferCount);
      memset(&data[1], 0x00, count-1);
   }

#ifdef LOG_LOW_LEVEL
   printDump(data, transferCount);
#endif // LOG_LOW_LEVEL

   *actualCount = transferCount;

   return (USBDM_ErrorCode)data[0];
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

   // An initial transaction of up to MaxFirstTransaction bytes is sent
   // This is guaranteed to fit in a single pkt (<= endpoint MAXPACKETSIZE)
   data[0] = txSize;
   rc = bdm_usb_send_epOut(txSize>MaxFirstTransaction?MaxFirstTransaction:txSize,
                           (const unsigned char *)data);

   // Remainder of data (if any) is sent as 2nd transaction
   // The size of this transaction is know to the receiver
   if ((rc == BDM_RC_OK) && (txSize>MaxFirstTransaction))
      rc = bdm_usb_send_epOut(txSize-MaxFirstTransaction,
            (const unsigned char *)data+MaxFirstTransaction);

   if (rc == BDM_RC_OK) {
      // Get response
      rc = bdm_usb_recv_epIn(rxSize, data, actualRxSize);
   }
   else {
      data[0] = rc;
      *actualRxSize = 0;
      memset(&data[1], 0x00, rxSize-1);
   }
   return rc;
}

//! \brief Executes an USB transaction.
//! This consists of a transmission of a command and reception of the response
//!
//! @param txSize = size of transmitted packet
//! @param rxSize = maximum size of received packet
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

   if (usbDeviceHandle==NULL) {
      print("bdm_usb_transaction(): device not open\n");
	  return BDM_RC_DEVICE_NOT_OPEN;
   }

   timeoutValue = timeout;

   if (bdmState.useOnlyEp0)
      rc = bdmJB16_usb_transaction( txSize, rxSize, data, &tempRxSize);
   else
      rc = bdmJMxx_usb_transaction( txSize, rxSize, data, &tempRxSize);

   if (actualRxSize != NULL) {
      // Variable size data expected
      *actualRxSize = tempRxSize;
   }
   else if ((rc == 0) && (tempRxSize != rxSize)) {
      // Expect exact data size
      print("bdm_usb_transaction() - Expected %d; received %d\n", rxSize, tempRxSize);
   }
   return rc;
}
