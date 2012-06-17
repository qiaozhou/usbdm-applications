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

   set ::RS08_SOPT              0x0019
   set ::RS08_SOPT_COPE         0x80
   set ::RS08_SOPT_STOPE        0x20
   set ::RS08_SOPT_BKGDPE       0x02
   set ::RS08_SOPT_RSTPE        0x01
   set ::RS08_SOPT_INIT         [expr ($::RS08_SOPT_STOPE|$::RS08_SOPT_BKGDPE)]
   
   set ::RS08_FOPT              0x023C
   set ::RS08_FLCR              0x023D
   
   set ::RS08_FLASHLOC          0x3800

   set ::RS08_FOPT_SECD_MASK    0x01   ;# Security bits
   set ::RS08_FOPT_SECD_UNSEC   0x01   ;# Security bits for unsecured device
   
   set ::RS08_FLCR_HVEN         0x08
   set ::RS08_FLCR_MASS         0x04
   set ::RS08_FLCR_PGM          0x01
   set ::RS08_FLCR_MASS_HVEN    [expr ($::RS08_FLCR_MASS|$::RS08_FLCR_HVEN)]
   
}
;######################################################################################
;#
;#
proc initTarget { sopt fopt flcr } {
   set ::RS08_SOPT  $sopt
   set ::RS08_FOPT  $fopt
   set ::RS08_FLCR  $flcr

   wb $::RS08_SOPT $::RS08_SOPT_INIT ;# Disable COP
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc initFlash { busFrequency } {
;# Not used
}

;######################################################################################
;#  Target is erased and unsecured
;#
proc massEraseTarget { } {

   ;# Mass erase flash
   ;# puts "Mass erasing device"
   
   settargetvpp standby
   set status [ catch {
      settargetvpp on
      wb $::RS08_FLCR     $::RS08_FLCR_MASS
      wb $::RS08_FLASHLOC 0x00
      wb $::RS08_FLCR     $::RS08_FLCR_MASS_HVEN
      after 510                                    ;# > 500 ms
      wb $::RS08_FLCR     $::RS08_FLCR_HVEN
      wb $::RS08_FLCR     0x00
   } ]
   settargetvpp off
   if [ expr ($status) ] {
      error "Target mass erase failed"
   }
   ;# Reset to take effect
   reset s s
   connect
   wb $::RS08_SOPT $::RS08_SOPT_INIT ;# Disable COP
   
   ;# Should be unsecure
   isUnsecure
}

;######################################################################################
;#
;#
proc isUnsecure { } {
   ;# puts "Checking if unsecured"
   set securityValue [rb $::RS08_FOPT]

   if [ expr ( $securityValue & $::RS08_FOPT_SECD_MASK ) != $::RS08_FOPT_SECD_UNSEC ] {
      error "Target is secured"
   }
}

;######################################################################################
;#
;#
loadSymbols
return

;#]]>