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

   set ::VBR_REG       0x801;
   set ::RAMBAR_REG    0xC05;
   set ::FLASHBAR_REG  0xC04;
   set ::FLASHBASE     0x00000000;
   set ::RAMBASE       0x20000000;
   set ::IPSBASE       0x40000000;
   set ::RAMBAR_OPTS   0x21;
   set ::FLASHBAR_OPTS 0x61;
      
   set ::CFMMCR       [expr 0x001D0000+$::IPSBASE] ;# 16-bit
   set ::CFMCLKD      [expr 0x001D0002+$::IPSBASE] ;# 8-bit
   set ::CFMSEC       [expr 0x001D0008+$::IPSBASE]
   set ::CFMPROT      [expr 0x001D0010+$::IPSBASE]
   set ::CFMSACC      [expr 0x001D0014+$::IPSBASE]
   set ::CFMDACC      [expr 0x001D0018+$::IPSBASE]
   set ::CFMUSTAT     [expr 0x001D0020+$::IPSBASE] ;# 8-bit
   set ::CFMCMD       [expr 0x001D0024+$::IPSBASE] ;# 8-bit
   set ::CFMCLKSEL    [expr 0x001D004A+$::IPSBASE] ;# 16-bit
   set ::PRDIV8       0x40
   
   set ::PDDPAR       [expr 0x00100074+$::IPSBASE]
   set ::SYNCR        [expr 0x00120000+$::IPSBASE] ;# 16-bit
   set ::CCHR         [expr 0x00120008+$::IPSBASE] ;# 8-bit
}
;######################################################################################
;#
;#
proc initTarget { args } {
   wcreg $::VBR_REG      $::RAMBASE
   wcreg $::RAMBAR_REG   [expr $::RAMBASE+$::RAMBAR_OPTS]
   wcreg $::FLASHBAR_REG [expr $::FLASHBASE+$::FLASHBAR_OPTS]
   
   wb $::PDDPAR    0x0F       ;# Enable PST signals

   return

   ;# The following is not compatible with chips w/o xtals
   wb $::CCHR      0x04       ;# /5
   ww $::SYNCR     0x4103     ;# Set RFD+1 to avoid frequency overshoot
   after 200                  ;# Wait for PLL lock
   ww $::SYNCR     0x4003     ;# Set desired RFD=0 and MFD=4 
   after 100                  ;# Wait for PLL lock
   ww $::SYNCR     0x4007     ;# Switch to using PLL
}

;######################################################################################
;#
;#  frequency - Target bus frequency in kHz
;#
proc initFlash { busFrequency } {
;# Not used
}

;######################################################################################
;#
;#
proc massEraseTarget { } {
;# Not possible unless in JTAG mode
}

;######################################################################################
;#
;#
proc isUnsecure { } {
;# ToDo
}

;######################################################################################
;#
;#
loadSymbols
return

;#]]>
