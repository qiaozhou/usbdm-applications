/*! \file
    \brief Descriptions for known devices

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
#include <string.h>

#include "wx/wx.h"

#include "Debug.h"
#include "Common.h"
#include "ApplicationFiles.h"
#include "Log.h"
#include "KnownDevices.h"

//! Number of known devices
unsigned int KnownDevices::deviceCount;

//! Each known device
DeviceData   KnownDevices::deviceData[MAX_KNOWN_DEVICES];

// Some 'dummy' devices types
//                                                  Idx ID   IRl DRl IRKn UNLK  IDC   Fl  Fmin    Fmax    Name               Description
//! Description of a Non-Freescale device
const DeviceData KnownDevices::nonFreescaleDevice = {0, 0x00, 2, 7,  0,   0x00, 0x00, 0,       0,      0, "Non-Freescale",  "-"};
//! Description of a device without IDCODE
const DeviceData KnownDevices::unknownDevice      = {0, 0x00, 2, 7,  0,   0x00, 0x00, 0,       0,      0, "No JTAG IDCODE", "-" };
//! Dummy description used when no device type is sel
const DeviceData KnownDevices::disabledDevice     = {0, 0x00, 2, 7,  0,   0x00, 0x00, 0,       0,      0, "Disabled",       "-" };
//! Description of a Customisable device (i.e. Freescale - parameters may be cutomised)
const DeviceData KnownDevices::customDevice       = {1, 0x00, 2, 7,  0,   0x0B, 0x00, 1,  150000, 200000, "Custom",         "-" };
//! Description for an unknown Freescale device
const DeviceData KnownDevices::unRecognizedDevice = {1, 0x00, 4, 7,  0,   0x0B, 0x00, 1,  150000, 200000, "Unrecognized",   "Freescale_Unknown"};

//! \brief Loads the known devices list from the config file.
//!
void KnownDevices::loadConfigFile(void) {
FILE *fp;
int  deviceNum = 0;
char lineBuff[200];
int  idcode;
int  irLength;
int  drLength;
int  unlockInstruction;
int  idcodeInstruction;
char description[100];
char name[100];
char type[20];
char *cp;
unsigned long fMin, fMax;
int equation;
int lineNo = 0;


   fp = openApplicationFile("Device_data/JTAG_Devices.cfg", "rt");
   if (fp == NULL) {
      print("Failed to open config file\n");
      return;
   }
   deviceData[0] = disabledDevice;
   deviceData[1] = customDevice;
   deviceNum = 2;

   while (!feof(fp)) {
      fgets(lineBuff, sizeof(lineBuff), fp);
      lineNo++;
//      print("original: %s", lineBuff);
      // Remove comments
      cp = strchr(lineBuff, '#');
      if (cp != NULL) {
         *cp = '\0';
      }
      // Remove eol
      cp = strchr(lineBuff, '\n');
      if (cp != NULL) {
         *cp = '\0';
      }
      // Discard empty lines (white space only)
      cp = lineBuff;
      while (*cp == ' ') {
         cp++;
      }
      if (*cp == '\0') {
         continue;
      }
//      print( "comment stripped: %s\n", lineBuff);
      if (sscanf(lineBuff, "%s %x %d %d %x %x %d %ld %ld %20s %20s",
                 type,
                 &idcode, &irLength, &drLength, &unlockInstruction, &idcodeInstruction,
                 &equation, &fMin, &fMax, name, description) == 11) {
#if TARGET == MC56F80xx
         if (strcmp(type, "DSC") != 0) {
            continue;
         }
#elif TARGET == CFVx
         if (strcmp(type, "CFVx") != 0) {
            continue;
         }
#endif
         deviceData[deviceNum].index             = deviceNum;
         deviceData[deviceNum].idcode            = idcode;
         deviceData[deviceNum].instructionLength = irLength;
         deviceData[deviceNum].dataLength        = drLength;
         deviceData[deviceNum].irLengthKnown     = (irLength>0);
         deviceData[deviceNum].unlockInstruction = unlockInstruction;
         deviceData[deviceNum].idcodeInstruction = idcodeInstruction;
         deviceData[deviceNum].flashEquation     = equation;
         deviceData[deviceNum].fMin              = fMin;
         deviceData[deviceNum].fMax              = fMax;
         deviceData[deviceNum].name              = strdup(name);
         deviceData[deviceNum].description       = strdup(description);
         deviceNum++;
      }
      else
         print( "Illegal line in config file, line #%d \"%s\"\n",
                 lineNo, lineBuff);
      deviceCount = deviceNum;
   }
   fclose(fp);

#ifdef LOG
   for (deviceNum = 0; deviceData[deviceNum].description != NULL; deviceNum++) {
      print("0x%8.8x %d 0x%2.2x %10s %s\n",
              deviceData[deviceNum].idcode,
              deviceData[deviceNum].instructionLength,
              deviceData[deviceNum].unlockInstruction,
              deviceData[deviceNum].name,
              deviceData[deviceNum].description);
   }
#endif
}

//! \brief Finds information about a device from JTAG IDCODE value
//!
//! @param idcode : JTAG IDCODE read from device
//!
const DeviceData *KnownDevices::lookUpDevice(uint32_t idcode) {

unsigned int       jedec          = JEDEC_ID(idcode);
unsigned int       freescalePin   = FREESCALE_PIN(idcode);
unsigned int       partNum        = PART_NUM(idcode);
int                sub;
const DeviceData  *device = NULL;

#ifdef LOG
  print("lookUpDevice: idcode         => 0x%3.3lx\n",  idcode);
  print("lookUpDevice: JEDEC code     => 0x%3.3x%s\n",
           jedec, (jedec==FREESCALE_JEDEC)?"(Freescale)":"");
  print("lookUpDevice: part Number    => 0x%3.3x\n",   partNum);
   if (jedec==FREESCALE_JEDEC)
     print("lookUpDevice: freescalePin   => 0x%3.3x\n",   freescalePin);
#endif

   if (idcode == 0x0) // No IDCODE from device
      device = &unknownDevice;
   else { // Search table for device
      for (sub=0; deviceData[sub].name != NULL; sub++) {
         if (jedec == JEDEC_ID(deviceData[sub].idcode)) { // Correct manufacturer
            if ((jedec == FREESCALE_JEDEC) &&                              // Freescale device?
                (freescalePin == FREESCALE_PIN(deviceData[sub].idcode))) { // Only need to match PIN
               device = &deviceData[sub];
               break;
            }
            if (partNum == PART_NUM(deviceData[sub].idcode)) { // Match entire partNum
               device = &deviceData[sub];
               break;
            }
         }
      }
   }
   if (device == NULL) { // Not found
      if (jedec == FREESCALE_JEDEC) // Unrecognized Freescale device?
         device = &unRecognizedDevice;
      else  // Non-Freescale device
         device = &nonFreescaleDevice;
   }
   return device;
}
