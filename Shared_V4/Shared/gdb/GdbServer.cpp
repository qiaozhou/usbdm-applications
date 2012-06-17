/*
 * main.cpp
 *
 *  Created on: 06/03/2011
 *      Author: podonoghue
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <unistd.h>
#include <tr1/memory>
#include <string.h>
#include "Common.h"
#include "Log.h"

#include "GdbSerialConnection.h"

#include "USBDM_API.h"
#include "Names.h"
#include "DeviceData.h"
#include "GdbHandler.h"
#include "ProgressTimer.h"

#include "GdbInput.h"
#include "GdbOutput.h"

#include "USBDM_ARM_API.h"

#if TARGET==CFV1
#define TARGET_TYPE T_CFV1
#elif TARGET==CFVx
#define TARGET_TYPE T_CFVx
#elif TARGET==ARM
#define TARGET_TYPE T_ARM_JTAG
#else
#error "Unhandled case"
#endif

#if (TARGET==CFV1) || (TARGET==CFVx)
#define targetConnect() USBDM_Connect()
#elif TARGET==ARM
#define targetConnect() ARM_Connect()
#else
#error "Unhandled case"
#endif

static DeviceData deviceData;
static ProgressTimer *progressTimer;

USBDM_ErrorCode usbdmInit(TargetType_t targetType = T_CFV1) {
   unsigned int deviceCount;
   unsigned int deviceNum;
   USBDM_ErrorCode rc = USBDM_Init();
   if (rc != BDM_RC_OK) {
      return rc;
   }
   rc = USBDM_FindDevices(&deviceCount);
   print( "usbdmInit(): Usb initialised, found %d device(s)\n", deviceCount);
   print("After USBDM_FindDevices, Time = %f\n", progressTimer->elapsedTime());
   if (rc != BDM_RC_OK) {
      return rc;
   }
   deviceNum  = 0;
   rc = USBDM_Open(deviceNum);
   if (rc != BDM_RC_OK) {
      print( "usbdmInit(): Failed to open %s, device #%d\n", getTargetTypeName(targetType), deviceNum);
      return rc;
   }
   print( "usbdmInit(): Opened %s, device #%d\n", getTargetTypeName(targetType), deviceNum);
   print("After USBDM_Open, Time = %f\n", progressTimer->elapsedTime());
   // Set up sensible default since we can't change this (at the moment)
   USBDM_ExtendedOptions_t bdmOptions = {sizeof(USBDM_ExtendedOptions_t), TARGET_TYPE};
   USBDM_GetDefaultExtendedOptions(&bdmOptions);
   bdmOptions.targetVdd                  = BDM_TARGET_VDD_3V3; // BDM_TARGET_VDD_NONE;
   bdmOptions.autoReconnect              = AUTOCONNECT_ALWAYS; // Aggressively auto-connect
   bdmOptions.guessSpeed                 = FALSE;
   bdmOptions.cycleVddOnConnect          = FALSE;
   bdmOptions.cycleVddOnReset            = FALSE;
   bdmOptions.leaveTargetPowered         = FALSE;
   bdmOptions.bdmClockSource             = CS_DEFAULT;
   bdmOptions.useResetSignal             = FALSE;
   bdmOptions.usePSTSignals              = FALSE;
   bdmOptions.interfaceFrequency         = 1000; // 1MHz
   bdmOptions.powerOnRecoveryInterval    = 100;
   bdmOptions.resetDuration              = 100;
   bdmOptions.resetReleaseInterval       = 100;
   bdmOptions.resetRecoveryInterval      = 100;
   rc = USBDM_SetExtendedOptions(&bdmOptions);
   if (rc != BDM_RC_OK) {
      print( "usbdmInit(): USBDM_SetExtendedOptions() failed\n");
      return rc;
   }
   print("After USBDM_SetExtendedOptions, Time = %f\n", progressTimer->elapsedTime());
   rc = USBDM_SetTargetType(targetType);
   if (rc != BDM_RC_OK) {
      print( "usbdmInit(): USBDM_SetTargetType() failed\n");
      return rc;
   }
   print("After USBDM_SetTargetType, Time = %f\n", progressTimer->elapsedTime());
#if TARGET ==ARM
   if (targetType == ARM) {
      rc = ARM_Initialise();
      if (rc != BDM_RC_OK) {
         print( "ARM_Initialise(): USBDM_SetTargetType() failed\n");
         return rc;
      }
   }
#endif
   USBDM_TargetReset((TargetMode_t)(RESET_DEFAULT|RESET_SPECIAL));
   print("After USBDM_TargetReset, Time = %f\n", progressTimer->elapsedTime());
   if (targetConnect() != BDM_RC_OK) {
      print( "usbdmInit(): Connecting failed - retry\n");
      USBDM_TargetReset((TargetMode_t)(RESET_DEFAULT|RESET_SPECIAL));
      rc = targetConnect();
      if (rc != BDM_RC_OK) {
         print( "targetConnect(): USBDM_SetTargetType() failed\n");
         return rc;
      }
   }
   print("After targetConnect, Time = %f\n", progressTimer->elapsedTime());
   return BDM_RC_OK;
}

/* closes currently open device */
int usbdmClose(void) {
   print( "usbdmClose(): Closing the device\n");
   USBDM_Close();
   USBDM_Exit();
   return 0;
}

enum {
   E_OK = 0,
   E_FATAL = 1,
};
#ifndef _WIN32_
#define stricmp(x,y) strcasecmp((x),(y))
#endif

void usage(void) {
   fprintf(stderr,
         "Usage: \n"
         "usbdm-gdbServer args...\n"
         "Args = device  - device to load (use -d to obtain device names)\n"
         "       -noload - Suppress loading of code to flash memory\n"
         "       -d      - list devices in database\n");
}

//
// Device name e.g. MCF51CN128
// -noload - don't load (program) flash
//
USBDM_ErrorCode doArgs(int argc, char **argv) {
   bool noLoad = false;
   bool listDevices = false;
   const char *deviceName = NULL;

   while (argc-- > 1) {
//      fprintf(stderr, "doArgs() - arg = \'%s\'\n", argv[argc]);
      if (stricmp(argv[argc], "-noload")==0) {
         noLoad = true;
      }
      else if (stricmp(argv[argc], "-D")==0) {
         // List targets
         listDevices = true;
      }
      else {
         // Assume device name
         deviceName = argv[argc];
      }
   }
   if ((deviceName == NULL) && !listDevices) {
      fprintf(stderr, "No device specified\n");
      print("doArgs() - No device specified\n");
      return BDM_RC_ILLEGAL_PARAMS;
   }
//   fprintf(stderr, "doArgs() - opening device database\n");
   // Find device details from database
   DeviceDataBase *deviceDatabase = new DeviceDataBase;
   fprintf(stderr, "Loading device database...\n");
   print( "doArgs() - Loading device database\n");
   try {
      deviceDatabase->loadDeviceData();
   } catch (MyException &exception) {
      fprintf(stderr, "Loading device database...Failed\n");
      print("doArgs() - Failed to load device database\n");
      return BDM_RC_DEVICE_DATABASE_ERROR;
   }
   print( "doArgs() - Loaded device database\n");
   if (listDevices) {
      int count = 0;
      std::vector<DeviceData *>::iterator it;
      try {
         for (it = deviceDatabase->begin(); it != deviceDatabase->end(); it++) {
            fprintf(stderr, "%s,\t", (*it)->getTargetName().c_str());
            if (++count == 3) {
               count = 0;
               fprintf(stderr, "\n");
            }
         }
         if (count != 0) {
            fprintf(stderr, "\n");
         }
      } catch (...) {
      }
      delete deviceDatabase;
      return BDM_RC_ERROR_HANDLED;
   }
   else {
      const DeviceData *devicePtr = deviceDatabase->findDeviceFromName(deviceName);
      if (devicePtr == NULL) {
         fprintf(stderr, "Failed to find device '%s' in database\n", deviceName);
         print("doArgs() - Failed to find device '%s' in database\n", deviceName);
         return BDM_RC_UNKNOWN_DEVICE;
      }
//      if (!devicePtr->valid) {
//         fprintf(stderr, "OPPS #1!\n");
//         print("doArgs() - OPPS #1!\n");
//         return E_FATAL;
//      }
      deviceData = *devicePtr;
//      if (!deviceData.valid) {
//         fprintf(stderr, "OPPS #2!\n");
//         print("doArgs() - OPPS #2!\n");
//         return E_FATAL;
//      }
      delete deviceDatabase;
//      if (!deviceData.valid) {
//         fprintf(stderr, "OPPS #3!\n");
//         print("doArgs() - OPPS #3!\n");
//         return E_FATAL;
//      }
   }
   return BDM_RC_OK;
}
#include <unistd.h>  /* sleep(1) */
#include <signal.h>
void ex_program(int sig) {
   (void) signal(SIGINT, ex_program);
}

//
// main
//
int main(int argc, char **argv) {
   (void) signal(SIGINT, SIG_IGN);

   if (progressTimer != NULL) {
      delete progressTimer;
   }
   progressTimer = new ProgressTimer(NULL, 1000);
   progressTimer->restart("Initialising...");

#ifdef LOG
   FILE *errorLog = fopen("c:\\delme.log","wt");
   setLogFileHandle(errorLog);
#if TARGET == ARM
//   ARM_SetLogFile(errorLog);
#endif
#endif
   print("gdbServer(): Starting\n");

   print("Starting, Time = %f\n", progressTimer->elapsedTime());
   USBDM_ErrorCode rc = doArgs(argc, argv);
   if (rc != BDM_RC_OK) {
      usage();
      reportError(rc);
      exit(1);
   }
   print("After doArgs, Time = %f\n", progressTimer->elapsedTime());
   rc = usbdmInit(TARGET_TYPE);
   print("After usbdmInit, Time = %f\n", progressTimer->elapsedTime());
   GdbInput  *gdbInput  = new GdbInput(stdin);
   print("After gdbInput, Time = %f\n", progressTimer->elapsedTime());
   GdbOutput *gdbOutput = new GdbOutput(stdout);
   print("After gdbOutput, Time = %f\n", progressTimer->elapsedTime());

   // Redirect stdout to stderr
   dup2(2,1);
//   int outHandle = dup(fileno(stderr));
//   freopen("delme.txt", "wb", stdout);

   if (!gdbInput->createThread() || (gdbInput == NULL) || (gdbOutput == NULL)) {
      // Quit immediately as we have no way to convey error to gdb
      reportError(BDM_RC_FAIL);
      exit (-1);
   }
   print("After createThread, Time = %f\n", progressTimer->elapsedTime());
   if (rc != BDM_RC_OK) {
      reportError(rc);
      const GdbPacket *pkt;
      print("gdbServer(): usbdmInit() Failed, waiting for GDB to collect error\n");
      fprintf(stderr, "gdbServer(): USBDM Initialisation failed, waiting for GDB to collect error\n");
      usbdmClose();
      for (int i = 0; i<3; i++) {
         if (gdbInput->isEOF()) {
            break;
         }
         pkt = gdbInput->getGdbPacket();
         if (pkt != NULL) {
            gdbOutput->sendAck();
            gdbOutput->setErrorMessage("E.fatal.No connection");
            gdbOutput->sendErrorMessage();
         }
      };
      fclose(stdin);
      return -1;
   }
   // Now do the actual processing of GDB messages
   handleGdb(gdbInput, gdbOutput, deviceData, progressTimer);
   usbdmClose();
   print("gdbServer() - Exiting\n");
   fprintf(stderr, "gdbServer() - Exiting\n");
#ifdef LOG
   fflush(errorLog);
   fclose(errorLog);
#endif
   fflush(stdout);
   fflush(stderr);
   fclose(stdin);
   fclose(stdout);

   return 0;
}
