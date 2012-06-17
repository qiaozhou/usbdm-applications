/*! \file
   \brief Compatibility wrapper for TBDML.dll library

   \verbatim
   TBDML interface DLL
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
+==============================================================================
|  7 Aug 2009 | Created                                                   - pgo
+==============================================================================
\endverbatim
*/
#include <math.h>
#include "wx/wx.h"
#include "wx/numdlg.h"
#include "../Log.h"
#include "../Version.h"
#include "../Common.h"
#include "../USBDM_API.h"
#include "../gui/USBDM_AUX.h"
#include "TBDML.h"
#include "../gui/hcs12UnsecureDialogue.h"

//bool usbdm_gdi_dll_open(void);
//bool usbdm_gdi_dll_close(void);

static bool askUnsecure = TRUE;

//! Get version of the DLL in BCD format
//!
//! @return 8-bit version number (2 BCD digits, major-minor)
//!
TBDML_API unsigned int  _tbdml_dll_version(void){
	return USBDM_DLLVersion();
}

//! Initialises USB interface
//!
//! This must be done before any device may be opened.
//!
//! @return Number of BDM devices found - Always 1!
//!
TBDML_API unsigned char _tbdml_init(void) {
static int firstCall = TRUE;

   print("_tbdml_init(), #%s\n", firstCall?"First call":"Later call");
//   usbdm_gdi_dll_open();

   // May be called multiple times - only init on the first call.
   if (firstCall)
      USBDM_Init();

   firstCall = FALSE;

   // This is a dummy return
   // Device count etc is done later
   return 1;
}

//! Opens a device
//!
//! @param device_no Number (0..N) of device to open.
//! A device must be open before any communication with the device can take place.
//! @note The range of device numbers must be obtained from init() before calling
//! this function.
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_open(unsigned char device_no) {
   print("_tbdml_open() - dummy\n");

   // Enable prompting to unsecure device
   askUnsecure = TRUE;

   // Opening of devices is deferred until the target is set.
   // The user can then choose the BDM etc
   return BDM_RC_OK;
}

//! Closes currently open device
//!
TBDML_API void _tbdml_close(void) {

   USBDM_Close();
//   usbdm_gdi_dll_close();
}

//! Get software version and type of hardware
//!
//! @return 16-bit version number (2 bytes)   \n
//!  - LSB = USBDM firmware version           \n
//!  - MSB = Hardware version
//!
USBDM_API unsigned int  _tbdml_get_version(void) {
USBDM_Version_t USBDM_Version;

   print("_tbdml_get_version()\n");
   (void)USBDM_GetVersion(&USBDM_Version);
   return (USBDM_Version.bdmHardwareVersion<<8) |
           USBDM_Version.bdmSoftwareVersion;
}

//! Sets target MCU type
//!
//! @param target_type type of target
//!
//! @return 0 => Success,\n !=0 => Fail
//!
//! @note A dialog box will open to allow the user to configure the BDM
//!    The user may be prompted to supply power to target
//!
TBDML_API unsigned char _tbdml_set_target_type(TargetType_t targetType) {
   print("_tbdml_set_target_type()\n");
   return USBDM_OpenTargetWithConfig(targetType);
}

//! Get status of the last command
//!
//! @return 0 => Success,\n 1 => Failure
//!
TBDML_API unsigned char _tbdml_get_last_sts(void) {

   return USBDM_GetCommandStatus();
}

//! Fills user supplied structure with state of BDM communication channel
//!
//! @param bdm_status Pointer to structure to receive status
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_bdm_sts(bdm_status_t *bdmStatus) {
USBDMStatus_t USBDMStatus;
USBDM_ErrorCode rc = BDM_RC_OK;

   print("_tbdml_bdm_sts()\n");

   rc = USBDM_TargetConnectWithRetry(&USBDMStatus);

   if (rc == BDM_RC_OK) {
      bdmStatus->ackn_state       = USBDMStatus.ackn_state;
      bdmStatus->connection_state = USBDMStatus.connection_state;
      bdmStatus->reset_state      = USBDMStatus.reset_recent;
   }
   else {
      bdmStatus->ackn_state       = WAIT;
      bdmStatus->connection_state = SPEED_NO_INFO;
      bdmStatus->reset_state      = NO_RESET_ACTIVITY;
   }
   return rc;
}

//======================================================================
//======================================================================
//======================================================================

//! Connects to Target.
//!
//! This will cause the BDM module to attempt to connect to the Target.
//! In most cases the BDM module will automatically determine the connection
//! speed and successfully connect.  If unsuccessful, it may be necessary
//! to manually set the speed using set_speed().
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_target_sync(void) {

   print("_tbdml_target_sync()\n");
   return USBDM_TargetConnectWithRetry();
}

//! Sets the communication speed.
//!
//! @param crystal_frequency crystal frequency in MHz.
//! This is used to determine the BDM communication speed (assumed
//! to be half the crystal frequency given).
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_set_speed(float crystal_frequency) {
int speed;

   print("_tbdml_set_target_type()");

   speed = floor((1000/2)*crystal_frequency); // Convert to BDM comm freq in kHz
   return USBDM_SetSpeed(speed);              // Set BDM coms speed in kHz
}

//! Determine crystal frequency of the target
//!
//! @return Crystal Frequency of target in MHz
//!
TBDML_API float _tbdml_get_speed(void) {
unsigned long speed;

   USBDM_GetSpeed(&speed); // BDM comms speed in kHz
   return 2*speed/1000.0;  // Convert to crystal freq in MHz
}

//! Resets the target to normal or special mode
//!
//! @param target_mode see \ref target_mode_e
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_target_reset(TargetMode_t targetMode) {

   USBDM_ErrorCode rc;

   rc = USBDM_TargetReset(targetMode);
   // Automatically connect after a reset
   if (rc == BDM_RC_OK)
      rc = USBDM_Connect();
   return rc;
}

//! Steps over a single target instruction
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_target_step(void) {

   print("tbdml_target_step()\n");

   return USBDM_TargetStep();
}

//! Starts target execution from current PC address
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_target_go(void) {

   return USBDM_TargetGo();
}

//! Brings the target into active background mode.  The target will be halted.
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_target_halt(void) {

   return USBDM_TargetHalt();
}

//! (HCS12) Reads contents of core registers
//!
//! @param  registers   pointer to structure to receive the register values
//!
//! @return 0 => Success,\n !=0 => Fail
//!
TBDML_API unsigned char _tbdml_read_regs(registers_t *registers) {
   U8 rc;
   unsigned long value;

   rc = USBDM_ReadReg(HCS12_RegPC, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hc12.pc  = (U16)value;
   rc = USBDM_ReadReg(HCS12_RegSP, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hc12.sp  = (U16)value;
   rc = USBDM_ReadReg(HCS12_RegX, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hc12.ix  = (U16)value;
   rc = USBDM_ReadReg(HCS12_RegY, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hc12.iy  = (U16)value;
   rc = USBDM_ReadReg(HCS12_RegD, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hc12.d   = (U16)value;
   rc = USBDM_ReadDReg(0xFF06,&value);
   if (rc != BDM_RC_OK)
      return rc;
	registers->hc12.ccr = (U8)value;

   return BDM_RC_OK;
}

//! (HCS12, HCS08 & RS08) Writes PC (HCS12, HC08), CCR+PC (RS08)
//!
//! @param value 16-bit value
//!
TBDML_API void _tbdml_write_reg_pc(unsigned int value) {

   USBDM_WriteReg(HCS12_RegPC, value);
}

//! (HCS12, HCS08 & RS08) Writes SP (HCS12, HCS08), Shadow PC (RS08)
//!
//! @param value 16-bit value
//!
TBDML_API void _tbdml_write_reg_sp(unsigned int value) {

   USBDM_WriteReg(HCS12_RegSP, value);
}

//! (HCS12) Writes IY
//!
//! @param value 16-bit value
//!
TBDML_API void _tbdml_write_reg_y(unsigned int value) {

   USBDM_WriteReg(HCS12_RegY, value);
}

//! (HCS12 & HCS08) Writes IX (HCS12), H:X (HCS08)
//!
//! @param value 16-bit value
//!
TBDML_API void _tbdml_write_reg_x(unsigned int value) {

   USBDM_WriteReg(HCS12_RegX, value);
}

//! (HCS12, HCS08 & RS08) Writes D=B:A (HCS12) or A (HCS08, RS08)
//!
//! @param value 16-bit / 8-bit value
//!
TBDML_API void _tbdml_write_reg_d(unsigned int value) {

   USBDM_WriteReg(HCS12_RegD, value);
}

//! (HCS12 & HCS08) Writes CCR
//!
//! @param value 8-bit value
//!
TBDML_API void _tbdml_write_reg_ccr(unsigned int value) {

   USBDM_WriteDReg(0xFF06,value);
}
//! (HCS12, HC08 & RS08) Reads one byte from memory
//!
//! @param address 16-bit memory address
//!
//! @return 8-bit value
//!
TBDML_API unsigned char _tbdml_read_byte(unsigned int address) {
U8 value;

   USBDM_ReadMemory( 1, 1, address, &value);
   return value;
}

//! (HCS12, HC08 & RS08) Writes one byte to memory
//!
//! For HCS12 & HC08 this writes to RAM.
//! For RS08 this may write to RAM or Flash if preceded by a call to mem_dlstart().
//!
//! @param address 16-bit memory address
//! @param data    8-bit data value
//!

TBDML_API void _tbdml_write_byte( unsigned int  address,
                                   unsigned char data) {
   USBDM_WriteMemory( 1, 1, address, &data);
}

//! (HCS12, HC08 & RS08) Reads block from memory
//!
//! @param addr    16-bit memory address
//! @param count   8-bit byte count
//! @param data    Pointer to buffer to contain data
//!
TBDML_API void _tbdml_read_block( unsigned int addr,
                                  unsigned int count,
                                  unsigned char *data) {


   USBDM_ReadMemory( 1, count, addr, data);
}

//! (HCS12, HC08 & RS08) Writes a block of data to memory
//!
//! For HCS12 & HC08 this writes to RAM.
//!
//! @param addr    16-bit memory address
//! @param count   8-bit byte count
//! @param data    Pointer to buffer containing data
//!
TBDML_API void _tbdml_write_block( unsigned int addr,
                                   unsigned int count,
                                   unsigned char *data) {
   USBDM_WriteMemory( 1, count, addr, data);
}

//! (HCS12) Reads one word from memory (address must be aligned)
//!
//! @param address 16-bit memory address
//!
//! @return 16-bit data value
//!
TBDML_API unsigned int _tbdml_read_word(unsigned int address) {
U8 buffer[2];

   USBDM_ReadMemory( 2, 2, address, buffer);
   return (buffer[0]<<8)+buffer[1];
}

//! (HCS12) Writes one word to memory (address must be aligned)
//!
//! @param address 16-bit memory address
//! @param data 16-bit data value
//!
TBDML_API void _tbdml_write_word(unsigned int address, unsigned int data) {
U8 buffer[2];

   buffer[0] = data>>8;
   buffer[1] = data;
   USBDM_WriteMemory( 2, 2, address, buffer);
}

#define HC12_BDMSTS     0xFF01   //!< Address of HC12 BDM Status register

//! (HCS12) Reads one byte from the BDM address space
//!
//! @param address 16-bit memory address
//! @return 8-bit data value
//!
//! @note Access to BDMSTS register address is mapped to USBDM_ReadStatusReg()
//!
TBDML_API unsigned char _tbdml_read_bd(unsigned int address) {
U8 rc;
unsigned long value;

   if (address == HC12_BDMSTS)
      rc = USBDM_ReadStatusReg(&value);
   else
      rc = USBDM_ReadDReg(address, &value);

   print("_tbdml_read_bd(%4.4X) => %2.2X\n", address, value);

   if (rc != BDM_RC_OK)
      return 0;

	return (U8)value;
}

//! (HCS12) Writes one byte to the BDM address space
//!
//! @param address 16-bit memory address
//! @param data    8-bit data value
//!
//! @return 0 => Success,\n !=0 => Fail
//!
//! @note Access to Control register addres is mapped to USBDM_WriteControlReg()
//!
TBDML_API unsigned char _tbdml_write_bd(unsigned int address, unsigned char data) {

   if (address == HC12_BDMSTS)
      return USBDM_WriteControlReg(data);
   else
      return USBDM_WriteDReg(address, data);
}

//===================================================================

class minimalApp : public  wxApp {
   DECLARE_CLASS( minimalApp )

public:
   minimalApp() :
      wxApp() {
      fprintf(stderr, "minimalApp::minimalApp()\n");
   }

   ~minimalApp(){
      fprintf(stderr, "minimalApp::~minimalApp()\n");
   }

   bool OnInit() {
      fprintf(stderr, "minimalApp::OnInit()\n");
      return true;
   }

   int OnExit() {
      fprintf(stderr, "minimalApp::OnExit()\n");
      return 0;
   }
};

IMPLEMENT_APP_NO_MAIN( minimalApp )
IMPLEMENT_CLASS( minimalApp, wxApp )

static bool wxInitializationDone = false;

bool usbdm_gdi_dll_open(void) {
   int argc = 1;
   char  arg0[] = "usbdm";
   char *argv[]={arg0, NULL};

   if (wxTheApp == NULL) {
      wxInitializationDone = wxEntryStart(argc, argv);
      print("usbdm_gdi_dll_open() - wxTheApp = %p\n", wxTheApp);
      print("usbdm_gdi_dll_open() - AppName = %s\n", (const char *)wxTheApp->GetAppName().ToAscii());
   }
   openLogFile("TBDML.log");
   if (wxInitializationDone)
      print("usbdm_gdi_dll_open() - wxEntryStart() successful\n");
   else {
      print("usbdm_gdi_dll_open() - wxEntryStart() failed\n");
      return false;
   }
   return true;
}

bool usbdm_gdi_dll_close(void) {
   print("usbdm_gdi_dll_close()\n");
#ifdef WIN32
   if (wxInitializationDone) {
      print("usbdm_gdi_dll_close() - Doing wxEntryCleanup()\n");
      wxEntryCleanup();
      print("usbdm_gdi_dll_close() - Done wxEntryCleanup()\n");
   }
#endif
   closeLogFile();
   return true;
}

extern "C" WINAPI __declspec(dllexport)
BOOL DllMain(HINSTANCE  hDLLInst,
             DWORD      fdwReason,
             LPVOID     lpvReserved) {

   switch (fdwReason) {
      case DLL_PROCESS_ATTACH:
         usbdm_gdi_dll_open();
//         fprintf(stderr, "=================================\n"
//                         "DLL_PROCESS_ATTACH\n");
         break;
      case DLL_PROCESS_DETACH:
//         fprintf(stderr, "DLL_PROCESS_DETACH\n"
//                         "=================================\n");
         usbdm_gdi_dll_close();
         break;
      case DLL_THREAD_ATTACH:
         //dll_initialize(_hDLLInst);
         break;
      case DLL_THREAD_DETACH:
         //dll_uninitialize();
         break;
   }
   return TRUE;
}
