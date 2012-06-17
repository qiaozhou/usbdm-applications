//============================================================================
// Name        : JS16_Bootloader.cpp
// Author      : Peter O'Donoghue
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"
#include "ICP.h"
#include "Names.h"
#include "FlashImage.h"
#include "JS16_Bootloader.h"

ICP_ErrorType programBlock(FlashImage *flashImageDescription, uint32_t size, uint32_t startBlock) {
   uint8_t buffer[256];
   memset(buffer, 0xFF, sizeof(buffer));
   ICP_ErrorType rc = ICP_RC_OK;
   while(size>0) {
      uint32_t blockSize = size;
      if (blockSize > sizeof(buffer)) {
         blockSize = sizeof(buffer);
      }
      fprintf(stderr, "Programming block [0x%04X...0x%04X]\n", startBlock, startBlock+blockSize-1);
      // Copy data to buffer
      for (uint32_t index=0; index<blockSize; index++) {
         buffer[index] = flashImageDescription->getValue(startBlock+index);
      }
      // Program buffer
      rc = ICP_Program(startBlock, blockSize, buffer);
      if (rc != ICP_RC_OK) {
         return rc;
      }
      startBlock += blockSize;
      size       -= blockSize;
   }
   return rc;
}

//#define HEX_FILE "Empty.s19"
#define HEX_FILE "USBDM_JS16CWJ_V4.sx"

ICP_ErrorType loadFile(FlashImage *flashImageDescription) {
   FlashImage::Enumerator *enumerator = flashImageDescription->getEnumerator();
   ICP_ErrorType progRc = ICP_RC_OK;
   while (enumerator->isValid()) {
      // Start address of block to program to flash
      uint32_t startBlock = enumerator->getAddress();

      // Find end of block to program
      enumerator->lastValid();
      uint32_t blockSize = enumerator->getAddress() - startBlock + 1;

      //print("Block size = %4.4X (%d)\n", blockSize, blockSize);
      if (blockSize>0) {
         // Program block [startBlock..endBlock]
         progRc = programBlock(flashImageDescription, blockSize, startBlock);
         if (progRc != ICP_RC_OK) {
            print("loadFile() - programming failed, Reason= %s\n", ICP_GetErrorName(progRc));
            break;
         }
      }
      // Move to start of next occupied range
      enumerator->nextValid();
   }
   delete enumerator;
   enumerator = NULL;
   return progRc;
}

ICP_ErrorType ProgramFlash(const char *hexFileName) {
   ICP_ErrorType rc;

   print("ProgramFlash() - Loading file \'%s\'\n", hexFileName);
   FlashImage flashImageDescription;
   FlashImage::ErrorCode Flashrc = flashImageDescription.loadS1S9File(hexFileName, true);
   if (Flashrc != FlashImage::SFILE_RC_OK) {
      print("main() - Failed to load file, Reason: %s\n", FlashImage::getErrorMessage(Flashrc));
      return ICP_RC_FILE_NOT_FOUND;
   }
   print("Total Bytes = %d\n", flashImageDescription.getByteCount());

   do {
      print("ProgramFlash() - Initialising\n");
      rc = ICP_Init();
      if (rc != ICP_RC_OK) {
         continue;
      }
      print("ProgramFlash() - Locating devices\n");
      unsigned devCount;
      rc = ICP_FindDevices(&devCount);
      if (rc != ICP_RC_OK) {
         continue;
      }
      print("ProgramFlash() - Found %d devices\n", devCount);
      print("ProgramFlash() - Opening device\n");
      rc = ICP_Open(0);
      if (rc != ICP_RC_OK) {
         continue;
      }
      print("ProgramFlash() - Erasing device\n");
      rc = ICP_MassErase();
      if (rc != ICP_RC_OK) {
         continue;
      }
      print("ProgramFlash() - Programming device\n");
      rc = loadFile(&flashImageDescription);
      if (rc != ICP_RC_OK) {
         continue;
      }
   } while (false);
   print("ProgramFlash() - Closing device\n");
   ICP_Close();
   ICP_Exit();
   return rc;
}
