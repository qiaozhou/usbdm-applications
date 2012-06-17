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

   set ::CFV1_XCSR_HALT         0x80
   set ::CFV1_XCSR_CSTAT_MASS   0x00
   set ::CFV1_XCSR_CLKSW        0x04
   set ::CFV1_XCSR_SEC          0x02  ;# read
   set ::CFV1_XCSR_ERASE        0x02  ;# write
   set ::CFV1_XCSR_ENBDM        0x01

   set ::CFV1_PRDIV8       0x40
}
;######################################################################################
;#
;#
proc initTarget { arg } {
;# Not used
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc initFlash { busFrequency } {
;# Not used
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc calculateFlashDivider { busFrequency } {

   if { [expr $busFrequency < 300] } {
      error "Clock too low for flash programming"
   }
   set cfmclkd 0
   if { [expr $busFrequency > 12800] } {
      set cfmclkd $::CFV1_PRDIV8
      set busFrequency [expr $busFrequency / 8]
   }
   set cfmclkd [expr $cfmclkd | (($busFrequency-1)/200)]
   set flashClk [expr $busFrequency / (($cfmclkd&0x3F)+1)]
;#   puts "cfmclkd = $cfmclkd, flashClk = $flashClk"
   if { [expr ($flashClk<150)||($flashClk>200)] } {
      error "Not possible to find suitable flash clock divider"
   }      
   return $cfmclkd
}

;######################################################################################
;#
;#
proc massEraseTarget { } {

   ;# reset target to be sure
   reset s s
   
   ;# Make sure BDM is using Bus clock
   ;#  wdreg $::CFV1_DRegXCSRbyte [expr $::CFV1_XCSR_CLKSW|$::CFV1_XCSR_ENBDM]
   ;#  Re-connect
   connect
   
   ;# Get target speed from BDM connection speed
   set busSpeedkHz [expr [speed]/1000]
   set cfmclkd [calculateFlashDivider $busSpeedkHz]

   ;# Set Flash clock divider via BDM reg
   wdreg $::CFV1_DRegCSR3byte $cfmclkd

   ;# Mass erase via BDM registers 
   ;#   puts "Mass erasing device"
   ;# (HALT is readonly but this is what the manual says) 
   wdreg $::CFV1_DRegXCSRbyte [expr $::CFV1_XCSR_HALT|$::CFV1_XCSR_CSTAT_MASS|$::CFV1_XCSR_ERASE|$::CFV1_XCSR_ENBDM|$::CFV1_XCSR_CLKSW]

   ;# Wait for command complete
   set flashBusy $::CFV1_XCSR_SEC
   set retry 0
   while { $flashBusy } {
      after 50
      set status [ rdreg $::CFV1_DRegXCSRbyte ]
      set flashBusy [expr $status & $::CFV1_XCSR_SEC]
      if [expr $retry == 10] {
         break;
      }
      incr retry
   }
   if [ expr (($status & $::CFV1_XCSR_SEC ) != 0) ] {
      error "Flash mass erase failed"
   }
   ;# reset so security change has effect
   reset s s
   connect
   }

;######################################################################################
;#
;#
proc isUnsecure { } {
   ;# Check if not read protected   
   set status [ rdreg $::CFV1_DRegXCSRbyte ]
   if [ expr ( $status & $::CFV1_XCSR_SEC ) != 0 ] {
      error "Target is secured"
   }
}

;######################################################################################
;#
;#
loadSymbols
return

;#]]>