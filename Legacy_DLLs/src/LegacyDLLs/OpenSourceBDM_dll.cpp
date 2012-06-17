/*! \file
   \brief Compatibility wrapper for opensourcebdm.dll library

   \verbatim
   Opensource interface DLL
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
| 24 Feb 2011 | Moved MCF51AC Hack to this DLL from BDM Firmware          - pgo
|  7 Aug 2009 | Created                                                   - pgo
+==============================================================================
\endverbatim
*/
#include <math.h>
#include "wx/wx.h"
#include "../Log.h"
#include "../Version.h"
#include "../Common.h"
#include "../USBDM_API.h"
#include "../gui/USBDM_AUX.h"
#include "OpensourceBDM.h"
#include "../TargetDefines.h"

//==========================================================================================
// Define the following to enable use of USBDM with MC51AC256 Colfire CPU
// Not extensively tested - may affect other coldfire chips adversely
//
#define MC51AC_HACK (1)

//bool usbdm_gdi_dll_open(void);
//bool usbdm_gdi_dll_close(void);

static bool askUnsecure = TRUE;
static TargetType_t targetType = T_OFF;

//! Get version of the DLL in BCD format
//!
//! @return 8-bit version number (2 BCD digits, major-minor)
//!
OSBDM_API unsigned int  _opensourcebdm_dll_version(void){
	return USBDM_DLLVersion();
}

//! Initialises USB interface
//!
//! This must be done before any device may be opened.
//!
//! @return Number of BDM devices found - Always 1!
//!
OSBDM_API unsigned char _opensourcebdm_init(void) {
static int firstCall = TRUE;

   print("_opensourcebdm_init(), #%s\n", firstCall?"First call":"Later call");
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
OSBDM_API unsigned char _opensourcebdm_open(unsigned char device_no) {
   print("_opensourcebdm_open() - dummy\n");

   // Enable prompting to unsecure device
   askUnsecure = TRUE;

   // Opening of devices is deferred until the target is set.
   // The user can then choose the BDM etc
   return BDM_RC_OK;
}

//! Closes currently open device
//!
OSBDM_API void _opensourcebdm_close(void) {

   USBDM_Close();
//   usbdm_gdi_dll_close();
}

//! Get software version and type of hardware
//!
//! @return 16-bit version number (2 bytes)   \n
//!  - LSB = USBDM firmware version           \n
//!  - MSB = Hardware version
//!
OSBDM_API unsigned int  _opensourcebdm_get_version(void) {
USBDM_Version_t USBDM_Version;

   print("_opensourcebdm_get_version()\n");
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
OSBDM_API unsigned char _opensourcebdm_set_target_type(TargetType_t _targetType) {

   if (_targetType == T_RS08) {
      wxMessageBox(_("RS08 devices are no longer supported by USBDM in this version of Codewarrior\n"
                     "Please update to Codewarrior V10 (Eclipse based)\n"),
                   _("Target not supported"),
                   wxICON_ERROR|wxOK|wxSTAY_ON_TOP);
      print("_opensourcebdm_set_target_type(): RS08 target not supported\n");
      return BDM_RC_ILLEGAL_PARAMS;
   }
   targetType = _targetType;
   return USBDM_OpenTargetWithConfig(_targetType);
}

//! Get status of the last command
//!
//! @return 0 => Success,\n 1 => Failure
//!
OSBDM_API unsigned char _opensourcebdm_get_last_sts(void) {

   return USBDM_GetCommandStatus();
}

//! Fills user supplied structure with state of BDM communication channel
//!
//! @param bdm_status Pointer to structure to receive status
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_bdm_sts(bdm_status_t *bdmStatus) {
USBDMStatus_t USBDMStatus;
USBDM_ErrorCode rc;

   print("_opensourcebdm_bdm_sts()\n");

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
OSBDM_API unsigned char _opensourcebdm_target_sync(void) {

   print("_opensourcebdm_target_sync()\n");
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
OSBDM_API unsigned char _opensourcebdm_set_speed(float crystal_frequency) {
int speed;

   print("_opensourcebdm_set_speed()");

   speed = floor((1000/2)*crystal_frequency); // Convert to BDM comm freq in kHz
   return USBDM_SetSpeed(speed);              // Set BDM coms speed in kHz
}

//! Determine crystal frequency of the target
//!
//! @return Crystal Frequency of target in MHz
//!
OSBDM_API float _opensourcebdm_get_speed(void) {
unsigned long speed;

   USBDM_GetSpeed(&speed); // BDM comms speed in kHz
   return 2*speed/1000.0;  // Convert to crystal freq in MHz
}

//! (HCS12, HCS08 & RS08) Read target BDM status register (BDCSC, BDMSTS or XCSR as appropriate)
//!
//! @return 8-bit value
//!
//!
OSBDM_API unsigned char _opensourcebdm_read_status(void){
unsigned long BDMStatus = 0;

   USBDM_ReadStatusReg(&BDMStatus);
   return (U8)BDMStatus;
}

//! (HCS12, HCS08 & RS08) Write to BDM Control Register (BDCSC, BDMSTS or XCSR as appropriate)
//!
//! @param value 8-bit value
//!
OSBDM_API void _opensourcebdm_write_control(unsigned char value){

   USBDM_WriteControlReg(value);
}

//! Resets the target to normal or special mode
//!
//! @param target_mode see \ref target_mode_e
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_target_reset(TargetMode_t targetMode) {

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
OSBDM_API unsigned char _opensourcebdm_target_step(void) {

   print("opensourcebdm_target_step()\n");

   return USBDM_TargetStep();
}

//! Starts target execution from current PC address
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_target_go(void) {

   return USBDM_TargetGo();
}

//! Brings the target into active background mode.  The target will be halted.
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_target_halt(void) {

   return USBDM_TargetHalt();
}

//! (HCS08 & RS08) Reads contents of core registers
//!
//! @param  registers   pointer to structure to receive the register values
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_read_regs(registers_t *registers) {
   U8 rc;
   unsigned long value;

   rc = USBDM_ReadReg(HCS08_RegPC, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hcs08.pc  = (U16)value;
   rc = USBDM_ReadReg(HCS08_RegSP, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hcs08.sp  = (U16)value;
   rc = USBDM_ReadReg(HCS08_RegHX, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hcs08.hx  = (U16)value;
   rc = USBDM_ReadReg(HCS08_RegA, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hcs08.a   = (U8)value;
   rc = USBDM_ReadReg(HCS08_RegCCR, &value);
   if (rc != BDM_RC_OK)
      return rc;
   registers->hcs08.ccr = (U8)value;

   return BDM_RC_OK;
}

//! (HCS12, HCS08 & RS08) Writes PC (HCS12, HC08), CCR+PC (RS08)
//!
//! @param value 16-bit value
//!
OSBDM_API void _opensourcebdm_write_reg_pc(unsigned int value) {

   USBDM_WriteReg(HCS08_RegPC, value);
}

//! (HCS12, HCS08 & RS08) Writes SP (HCS12, HCS08), Shadow PC (RS08)
//!
//! @param value 16-bit value
//!
OSBDM_API void _opensourcebdm_write_reg_sp(unsigned int value) {

   USBDM_WriteReg(HCS08_RegSP, value);
}

//! (HCS12 & HCS08) Writes IX (HCS12), H:X (HCS08)
//!
//! @param value 16-bit value
//!
OSBDM_API void _opensourcebdm_write_reg_x(unsigned int value) {

   USBDM_WriteReg(HCS08_RegHX, value);
}

//! (HCS08 & RS08) Writes  A (HCS08, RS08)
//!
//! @param value 16-bit / 8-bit value
//!
OSBDM_API void _opensourcebdm_write_reg_d(unsigned int value) {

   USBDM_WriteReg(HCS08_RegA, value);
}

//! (HCS12 & HCS08) Writes CCR
//!
//! @param value 8-bit value
//!
OSBDM_API void _opensourcebdm_write_reg_ccr(unsigned int value) {

   USBDM_WriteReg(HCS08_RegCCR, value);
}

//! (HCS08 & RS08) Write to Breakpoint Register
//!
//! @param value 16-bit value
//!
OSBDM_API void _opensourcebdm_write_bkpt(unsigned int value){

   USBDM_WriteDReg(0, value);
}

//! (HCS08 & RS08) Read from Breakpoint Register
//!
//! @return 16-bit value
//!
OSBDM_API unsigned int _opensourcebdm_read_bkpt(void){
unsigned long value;

   USBDM_ReadDReg(0, &value);
   return value;
}

//! (HCS12, HC08 & RS08) Reads one byte from memory
//!
//! @param address 16-bit memory address
//!
//! @return 8-bit value
//!
OSBDM_API unsigned char _opensourcebdm_read_byte(unsigned int address) {
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

OSBDM_API void _opensourcebdm_write_byte( unsigned int  address,
                                          unsigned char data) {
   USBDM_WriteMemory( 1, 1, address, &data);
}

//! (HCS12, HC08 & RS08) Reads block from memory
//!
//! @param addr    16-bit memory address
//! @param count   8-bit byte count
//! @param data    Pointer to buffer to contain data
//!
OSBDM_API void _opensourcebdm_read_block( unsigned int  addr,
                                          unsigned int  count,
                                          unsigned char *data) {

   USBDM_ReadMemory( 1, count, addr, data);
}

//! (HCS12, HC08 & RS08) Writes a block of data to memory
//!
//! For HCS12 & HC08 this writes to RAM.
//! For RS08 this may write to RAM or Flash if preceded by _opensourcebdm_mem_dlstart().
//!
//! @param addr    16-bit memory address
//! @param count   8-bit byte count
//! @param data    Pointer to buffer containing data
//!
OSBDM_API void _opensourcebdm_write_block( unsigned int addr,
                                           unsigned int count,
                                           unsigned char *data) {
   USBDM_WriteMemory( 1, count, addr, data);
}


//===========================================================
//  RS08 Flash memory handling
//

//! (RS08) Sets target derivative type
//!
//! @param derivative type of RFS08 derivative, see \ref rs08_derivative_type_e
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API unsigned char _opensourcebdm_set_derivative_type(unsigned char derivative) {

   // Not supported
   return 0;
}

//! (RS08) Prepare for RS08 Flash programming (Vpp on & Mass erase Flash)
//!
//! _opensourcebdm_write_block() and _opensourcebdm_write_byte() may now be
//! be used to program blocks of data or individual bytes to the RS08 Flash.
//!
OSBDM_API void _opensourcebdm_mem_dlstart(void){

   // Not supported
   return;
}

//! (RS08) Check if ready for next block to program to Flash
//!
//! \warning Needs review
//!
OSBDM_API unsigned char _opensourcebdm_target_readyfor_datald(void){

   // Not supported
   return 0;
}

//! (RS08) Tidy up after Flash programming \n
//! Restore/program clock trim value, Vpp off
//!
OSBDM_API void _opensourcebdm_mem_dlend(void){

   // Not supported
   return;
}

//=========================================================
//=========================================================
//=========================================================
// CFV1
//
//! (CFv1) Write XCSR.msb
//!
//! @param value 8-bit number
//!
OSBDM_API void  _opensourcebdm_write_xcsr_byte(unsigned char value) {

   USBDM_WriteControlReg(value);
}

//! (CFv1) Read XCSR.msb
//!
//! @return 8-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_xcsr_byte(void) {
unsigned long BDMStatus;

   USBDM_ReadStatusReg(&BDMStatus);
   return (U8) BDMStatus;
}

//! (CFv1) Write CSR2.msb
//!
//! @param value 8-bit number
//!
OSBDM_API void  _opensourcebdm_write_csr2_byte(unsigned char value) {

   USBDM_WriteDReg(CFV1_DRegCSR2byte,value);
   return;
}

//! (CFv1) Read CSR2.msb
//!
//! @return 8-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_csr2_byte(void) {
unsigned long regValue;

   USBDM_ReadDReg(CFV1_DRegCSR2byte, &regValue);
   return regValue;
}

//! (CFv1) Write CSR3.msb
//!
//! @param value 8-bit number
//!
OSBDM_API void  _opensourcebdm_write_csr3_byte(unsigned char value) {

   USBDM_WriteDReg(CFV1_DRegCSR3byte,value);
   return;
}

//! (CFv1) Read CSR3.msb
//!
//! @return value 8-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_csr3_byte(void) {
unsigned long regValue;

   USBDM_ReadDReg(CFV1_DRegCSR3byte, &regValue);
   return regValue;
}

//! (CFv1) Write Core registers
//!
//! @param regNo Register #
//! @param value 32-bit value
//!
OSBDM_API void _opensourcebdm_write_reg(unsigned char regNo, unsigned int value) {

   if (regNo>15) {// debugger tries to write to illegal registers !
      print("_opensourcebdm_write_reg(%d,%X) - illegal register\r\n", regNo, value);
      return;
   }
   USBDM_WriteReg(regNo, value);
}

//! (CFv1) Read Core registers
//!
//! @param regNo  Register #
//!
//! @return 32-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_reg(unsigned char regNo) {
unsigned long value;

   USBDM_ReadReg(regNo, &value);
   return value;
}

//! (CFv1) Write Control register
//!
//! @param regNo Register #
//! @param value 32-bit value
//!
OSBDM_API void _opensourcebdm_write_creg(unsigned char regNo, unsigned int value) {

   if (regNo>15) {// debugger tries to write to illegal registers !
      print("_opensourcebdm_write_creg(%d,%X) - illegal register\r\n", regNo, value);
      return;
   }
   USBDM_WriteCReg(regNo, value);
}

//! (CFv1) Read Control register
//!
//! @param regNo  Register #
//!
//! @return 32-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_creg(unsigned char regNo) {
unsigned long value;

   USBDM_ReadCReg(regNo, &value);
   return value;
}

//! (CFv1) Write Debug register
//!
//! @param regNo Register #
//! @param value 32-bit value
//!
OSBDM_API void _opensourcebdm_write_dreg(unsigned char regNo, unsigned int value) {

#ifdef MC51AC_HACK
   if ((targetType == T_CFV1) && (regNo == CFV1_DRegCSR)) {
      // MCF51AC Hack
      print("_opensourcebdm_write_dreg() - hacking CSR value\n", value);
      value |= CFV1_CSR_VBD;
   }
#endif

   if (regNo>31) {// debugger tries to write to illegal registers !
      print("_opensourcebdm_write_dreg(%d,%X) - illegal register\r\n", regNo, value);
      return;
   }
   USBDM_WriteDReg(regNo, value);
}


//! (CFv1) Read Debug register
//!
//! @param regNo  Register #
//!
//! @return 32-bit number
//!
OSBDM_API unsigned int  _opensourcebdm_read_dreg(unsigned char regNo) {
unsigned long value;

   USBDM_ReadDReg(regNo, &value);
   return value;
}

//!  (CFv1) Memory write
//!
//! @param addr         24-bit address
//! @param count        8-bit block size (in bytes)
//! @param elementSize  Memory transfer size (1,2 or 4)
//! @param data         Pointer to buffer containing data
//!
//! @return TRUE => Success,\n FALSE => Fail
//!
OSBDM_API BOOL  _opensourcebdm_write_mem( unsigned int address,
                                          unsigned int count,
                                          unsigned int elementSize,
                                          unsigned char *data) {

   return (USBDM_WriteMemory( elementSize, count, address, data) == BDM_RC_OK);
}

//!  (CFv1) Memory read
//!
//! @param address      24-bit address
//! @param count        Block size (in bytes)
//! @param elementSize  Memory transfer size (1,2 or 4)
//! @param data         Pointer to buffer for data
//!
//! @return 0 => Success,\n !=0 => Fail
//!
OSBDM_API BOOL _opensourcebdm_read_mem( unsigned int  address,
                                        unsigned int  count,
                                        unsigned int  elementSize,
                                        unsigned char *data) {

   return (USBDM_ReadMemory( elementSize, count, address, data) == BDM_RC_OK);
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
   openLogFile("OpenSourceBDM.log");
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
