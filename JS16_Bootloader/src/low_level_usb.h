/*! \file
    \brief Low-level USB interface to BDM module .

*/
#include "ICP.h"

ICP_ErrorType bdm_usb_init(void);
ICP_ErrorType bdm_usb_exit(void);
ICP_ErrorType bdm_usb_findDevices(unsigned *numDevices);
ICP_ErrorType bdm_usb_getStringDescriptor(int index, char *deviceDescription, unsigned maxLength);
ICP_ErrorType bdm_usb_releaseDevices(void);
ICP_ErrorType bdm_usb_getDeviceCount(unsigned int *deviceCount);
ICP_ErrorType bdm_usb_open(unsigned int device_no);
ICP_ErrorType bdm_usb_close(void);
ICP_ErrorType bdm_usb_send_ep0(const unsigned char * data);
ICP_ErrorType bdm_usb_recv_ep0(unsigned char *data, unsigned *actualRxSize);
ICP_ErrorType bdm_usb_raw_send_ep0(unsigned int  request,
                                     unsigned int  wValue,
                                     unsigned int  wIndex,
                                     unsigned int  size,
                                     const unsigned char *data);
ICP_ErrorType bdm_usb_raw_recv_ep0(unsigned int  request,
                                     unsigned int  wValue,
                                     unsigned int  wIndex,
                                     unsigned int  size,
                                     unsigned char *data);

//**********************************************************
//!
//! Sleep for given number of milliseconds (or longer!)
//!
//! @param milliSeconds - number of milliseconds to sleep
//!
void milliSleep(int milliSeconds);

