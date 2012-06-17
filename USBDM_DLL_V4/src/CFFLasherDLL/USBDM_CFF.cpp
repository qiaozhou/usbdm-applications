/*! \file
   USBDM_CFF.cpp

   \brief Entry points for USBDM_CFF library

   \verbatim
   USBDM interface DLL
   Copyright (C) 2008  Peter O'Donoghue

   Based on
      Opensource BDM  - interface DLL

   which is based on
      TBDML interface DLL by
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
\endverbatim

\verbatim
 Change History
+===========================================================================================
| 17 Mar 2012 | Implemented some missing functions _ta(),_bkpt(),             - pgo - V4.9.3
+===========================================================================================
\endverbatim
*/

#include "../Common.h"
#include "../Log.h"
#include "../USBDM_API.h"
#include "../USBDM_API_Private.h"
#include "USBDM_CFF.h"
#include "../low_level_usb.h"

static bool initDone = false;

CFF_API int bdm_close_device(void) {
   print("CFF-bdm_close_device()\n");
   USBDM_Close();
   initDone = false;
   return 0;
}

CFF_API int bdm_configure(void) {
   unsigned deviceCount;
   print("CFF-bdm_configure()\n");
   if (USBDM_Init() != BDM_RC_OK)
      return 1;
   if (USBDM_FindDevices(&deviceCount) != BDM_RC_OK)
      return 1;
   if (USBDM_Open(0) != BDM_RC_OK)
      return 1;
   initDone = true;
   return 0;
}

static BDM_Options_t bdmOptions;

CFF_API int bdm_init_device(void) {
   TargetType_t targetType = T_CFVx;

   if (!initDone)
      bdm_configure();

   print("CFF-bdm_init_device()\n");

   // Set up sensible defaults since we can't change this
   bdmOptions.targetVdd          = BDM_TARGET_VDD_3V3;//BDM_TARGET_VDD_NONE;
   bdmOptions.autoReconnect      = TRUE;
   bdmOptions.guessSpeed         = FALSE;
   bdmOptions.cycleVddOnConnect  = FALSE;
   bdmOptions.cycleVddOnReset    = FALSE;
   bdmOptions.leaveTargetPowered = FALSE;
   bdmOptions.useAltBDMClock     = CS_DEFAULT;
   bdmOptions.useResetSignal     = TRUE;
   bdmOptions.usePSTSignals      = FALSE;

   if (USBDM_SetOptions(&bdmOptions) != BDM_RC_OK)
      return 0;

   if (USBDM_SetTargetType(targetType) != BDM_RC_OK)
      return 0;

   // Set conservative speed
   USBDM_SetSpeed(5000);

   if (USBDM_Connect() != BDM_RC_OK)
      return 0;

   return 1;
}
CFF_API int bdm_reset(void) {
   print("CFF-bdm_reset()\n");
   if (USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE)) != BDM_RC_OK)
      return 0;
   if (USBDM_Connect() != BDM_RC_OK)
      return 0;
   return 1;
}
CFF_API int bdm_force_bdm(void) {
   print("CFF-bdm_force_bdm()\n");
   if (USBDM_TargetReset((TargetMode_t)(RESET_SPECIAL|RESET_HARDWARE)) != BDM_RC_OK)
      return 0;
   if (USBDM_TargetHalt() != BDM_RC_OK)
      return 0;
   return 1;
}
CFF_API int bdm_go(void) {
   print("CFF-bdm_go()\n");
   if (USBDM_TargetGo() != BDM_RC_OK)
      return 0;
   return 1;
}
CFF_API int bdm_test_for_halt(void) {
   print("CFF-bdm_test_for_halt()\n");

   USBDMStatus_t USBDMStatus;
   USBDM_GetBDMStatus(&USBDMStatus);

   if (bdmOptions.usePSTSignals) {
      // Check processor state using PST signals
      if (USBDMStatus.halt_state == TARGET_HALTED)
         return 1;
      else
         return 0;
   }
   else {
      // Probe D0 register - if fail assume processor running!
      unsigned long int dummy;
      if (USBDM_ReadReg(CFVx_RegD0, &dummy) == BDM_RC_OK)
         return 1;
      else
         return 0;
   }
}
CFF_API int bdm_write_dreg(int regNo, unsigned long value) {
   print("CFF-bdm_write_dreg(%d, 0x%08X)\n", regNo, value);
   USBDM_WriteReg(CFVx_RegD0+regNo, value);
   return 1;
}
CFF_API int bdm_read_dreg(int regNo) {
   unsigned long temp;

   USBDM_ReadReg(CFVx_RegD0+regNo, &temp);
   print("CFF-bdm_read_dreg(%d) => 0x%08X\n", regNo, temp);
   return temp;
}
CFF_API int bdm_read_areg(int regNo) {
   unsigned long temp;

   print("CFF-bdm_read_areg()\n");
   USBDM_ReadReg(CFVx_RegA0+regNo, &temp);
   return temp;
}
CFF_API int bdm_write_areg(int regNo, unsigned long value) {
   print("CFF-bdm_write_areg(%d, 0x%08X)\n", regNo, value);
   USBDM_WriteReg(CFVx_RegA0+regNo, value);
   return 1;
}
CFF_API int bdm_wcreg(int regNo, unsigned long value) {
   print("CFF-bdm_wcreg(0x%04X, 0x%08X)\n", regNo, value);
   USBDM_WriteCReg(regNo, value);
   return 0;
}
CFF_API int bdm_rcreg(int regNo) {
   unsigned long temp;
   USBDM_ReadCReg(CFVx_RegD0+regNo, &temp);
   print("CFF-bdm_rcreg(%04X) => 0x%08X\n", regNo, temp);
   return temp;
}
CFF_API int bdm_rdmreg(int regNo) {
   unsigned long temp = 0;
   USBDM_ReadDReg(HCS08_DRegBKPT+regNo, &temp);
   print("CFF-bdm_rdmreg(%04X) => 0x%08X - dummy\n", regNo, temp);
   return temp;
}
CFF_API int bdm_wdmreg(int regNo, unsigned long value) {
   print("CFF-bdm_wdmreg(0x04X, 0x%08X)\n", regNo, value);
   USBDM_WriteDReg(regNo, value);
   return 0;
}

CFF_API int bdm_fillb(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_fillb(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_WriteMemory(1, byteCount, address, data);
   return 0;
}
CFF_API int bdm_fillw(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_fillw(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_WriteMemory(2, byteCount, address, data);
   return 0;
}
CFF_API int bdm_filll(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_filll(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_WriteMemory(4, byteCount, address, data);
   return 0;
}

CFF_API int bdm_dumpb(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_dumpb(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_ReadMemory(1, byteCount, address, data);
   return 0;
}
CFF_API int bdm_dumpw(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_dumpw(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_ReadMemory(2, byteCount, address, data);
   return 0;
}
CFF_API int bdm_dumpl(unsigned long address, unsigned char *data, unsigned long byteCount) {
   print("CFF-bdm_dumpl(0x%08X,0x%08X,%p)\n", address, byteCount, data);
   USBDM_ReadMemory(4, byteCount, address, data);
   return 0;
}
CFF_API int bdm_nop(void) {
   print("CFF-bdm_nop() - ignored\n");
   return 0;
}
CFF_API int bdm_ta(void) {
   print("CFF-bdm_ta()\n");
   USBDM_ControlPins(PIN_TA_LOW, 0);
   milliSleep(500 /* ms */);
   USBDM_ControlPins(PIN_RELEASE, 0);
   return 0;
}
CFF_API int bdm_bkpt(void) {
   print("CFF-bdm_bkpt()\n");
   USBDM_ControlPins(PIN_BKPT_LOW, 0);
   milliSleep(500 /* ms */);
   USBDM_ControlPins(PIN_RELEASE, 0);
   return 0;
}
CFF_API int bdm_write_byte(unsigned long address, unsigned long value) {
   print("CFF-bdm_write_byte(0x%08X, 0x%08X)\n", address, value);
   uint8_t data;
   data = value;
   USBDM_WriteMemory(1, 1, address, &data);
   return 0;
}
CFF_API int bdm_write_word(unsigned long address, unsigned long value) {
   print("CFF-bdm_write_word(0x%08X, 0x%08X)\n", address, value);
   uint8_t data[2];
   data[0] = value>>8;
   data[1] = value;
   USBDM_WriteMemory(2, 2, address, data);
   return 0;
}
CFF_API int bdm_write_long(unsigned long address, unsigned long value) {
   print("CFF-bdm_write_long(0x%08X, 0x%08X)\n", address, value);
   uint8_t data[4];
   data[0] = value>>24;
   data[1] = value>>16;
   data[2] = value>>8;
   data[3] = value;
   USBDM_WriteMemory(4, 4, address, data);
   return 0;
}
CFF_API int bdm_read_byte(unsigned long address) {
   uint8_t data;
   USBDM_ReadMemory(1, 1, address, &data);
   print("CFF-bdm_read_byte()\n");
   return data;
}
CFF_API int bdm_read_word(unsigned long address) {
   uint8_t data[2];
   unsigned long value;
   USBDM_ReadMemory(2, 2, address, data);
   value = (data[0]<<8)+data[1];
   print("CFF-bdm_read_word()\n");
   return value;
}
CFF_API int bdm_read_long(unsigned long address) {
   uint8_t data[4];
   unsigned long value;
   USBDM_ReadMemory(4, 4, address, data);
   value = (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
   print("CFF-bdm_read_long(0x%08X) = > 0x%08X\n", address, value);
   return value;
}

//======================================================
CFF_API int bdm_load_bin(void) {
   print("CFF-bdm_load_bin() - not implemented\n");
   return 0;
}
CFF_API int bdm_load_srec(void) {
   print("CFF-bdm_load_srec() - not implemented\n");
   return 0;
}
