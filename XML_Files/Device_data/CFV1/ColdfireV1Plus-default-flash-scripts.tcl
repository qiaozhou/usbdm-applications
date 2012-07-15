;#<![CDATA[
;#
;######################################################################################
;#  This file defines the following flash functions
;#  
;#  massEraseTarget - Entirely erases the target.
;#                    The target should be left in an unsecured state.
;#
;#  isUnsecure - indicates if the target is secured in some fashion (read/write protected)
;#               Returns TCL_OK if NOT secured
;#
;#  initFlash - initialises the target for flash programing (if needed)
;#
;#  initTarget - initialises the target for general use
;#
;#  In addition the script may do any once-only initialisation such as setting global symbols
;#  when initially loaded into the TCL environment.
;#

;######################################################################################
;#
;#
proc loadSymbols {} {
   # BigEndian format for writing numbers to memory
   setbytesex bigEndian

   set ::CFV1_DRegCSR   0 ;# - CSR
   set ::CFV1_DRegXCSR  1 ;# - XCSR
   set ::CFV1_DRegCSR2  2 ;# - CSR2
   set ::CFV1_DRegCSR3  3 ;# - CSR3
      
   set ::CFV1_ByteRegs       0x1000 ;# Special access to msb
   set ::CFV1_DRegXCSRbyte   [expr $::CFV1_ByteRegs+$::CFV1_DRegXCSR] ;# - XCSR.msb
   set ::CFV1_DRegCSR2byte   [expr $::CFV1_ByteRegs+$::CFV1_DRegCSR2] ;# - CSR2.msb
   set ::CFV1_DRegCSR3byte   [expr $::CFV1_ByteRegs+$::CFV1_DRegCSR3] ;# - CSR3.msb

   set ::PRDIV8                 0x40

   set ::CFV1_XCSR_HALT         0x80
   set ::CFV1_XCSR_CSTAT_MASS   0x00
   set ::CFV1_XCSR_CLKSW        0x04
   set ::CFV1_XCSR_SEC          0x02  ;# read
   set ::CFV1_XCSR_ERASE        0x02  ;# write
   set ::CFV1_XCSR_ENBDM        0x01
   
   set ::CFV1_DBGCR_MASS_ERASE  0x01
   set ::CFV1_DBGCR_LL_STATUS   0x02

   set ::CFV1_DBGSR_ERASE       0x01
   set ::CFV1_DBGSR_FSACC       0x02
   set ::CFV1_DBGSR_ME_EN       0x04
   set ::CFV1_DBGSR_BACK_EN     0x08
   set ::CFV1_DBGSR_VLP_ST      0x10
   set ::CFV1_DBGSR_LLS_EX      0x20
   set ::CFV1_DBGSR_VLLS_EX     0x40
   
   set ::CFV1_CSR3_UI           0x80
   
   set ::SIM_COPC               0xFFFF80CA
   set ::SIM_COPC_VALUE         0x00
   
}

;######################################################################################
;#
;#
proc initTarget { arg } {
   ;# Disable COP
   wb $::SIM_COPC $::SIM_COPC_VALUE
}

;######################################################################################
;#
;#  frequency - Target bus frequency in kHz
;#
proc initFlash { busFrequency } {
;# Not used
}

;######################################################################################
;# Write value to DBGCR register
;#
proc writeDBGCR { value } {
   wdreg $::CFV1_DRegCSR3byte [expr 0x70|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x60|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x50|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x40|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x30|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x20|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr 0x10|($value & 0x0F) ]
   set value [expr $value >> 4]
   wdreg $::CFV1_DRegCSR3byte [expr $::CFV1_CSR3_UI|($value & 0x0F) ]
}

proc readDBGCR { } {
   set status 0
   set status [expr $status | [ rdreg $::CFV1_DRegCSR3byte ]]
   set status [expr $status | [ rdreg $::CFV1_DRegCSR3byte ] << 8]
   set status [expr $status | [ rdreg $::CFV1_DRegCSR3byte ] << 16]
   set status [expr $status | [ rdreg $::CFV1_DRegCSR3byte ] << 24]
   return $status
}

;######################################################################################
;#
;#
proc massEraseTarget { } {

   ;# reset target to be sure
   reset s s
   connect
 
   ;# Write mass erase bit to DBGCR reg
   writeDBGCR $::CFV1_DBGCR_MASS_ERASE

   ;# Wait for command complete
   set flashBusy 1
   set retry 0
   while { $flashBusy } {
      after 50
      set status [readDBGCR]
      set flashBusy [expr !($status & $::CFV1_DBGSR_ERASE)]
      if [expr $retry == 10] {
         break;
      }
      incr retry
   }
   if [ expr ( $status & $::CFV1_DBGSR_ERASE) == 0 ] {
      error "Flash mass erase failed"
   }
}

;######################################################################################
;#
;#
proc isUnsecure { } {
   ;# Check if not read protected   
   set status [ rdreg $::CFV1_DRegXCSRbyte ]
   if [ expr ( $status & ($::CFV1_XCSR_SEC|$::CFV1_XCSR_ENBDM) ) != $::CFV1_XCSR_ENBDM ] {
      error "Target is secured"
   }
}

;######################################################################################
;#
;#
loadSymbols
return

;#]]>