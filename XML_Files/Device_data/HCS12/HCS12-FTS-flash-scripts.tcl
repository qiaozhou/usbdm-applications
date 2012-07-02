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

   set ::HCS12_FSEC_SEC_MASK     0x03   ;# Security bits
   set ::HCS12_FSEC_SEC_UNSEC    0x02   ;# Security bits for unsecured device
   set ::HCS12_FSEC_SEC_KEYEN    0x80   ;# Backdoor Key enable
   set ::HCS12_FSEC_SEC_FNORED   0x40   ;# Vector redirection disable
   set ::HCS12_FSEC_UNSEC_VALUE  0xFFFE ;# Value to use when unsecuring (0xFF:NVSEC value)
      
   set ::HCS12_FCLKDIV           0x0100
   set ::HCS12_FSEC              0x0101
   set ::HCS12_FTSTMOD           0x0102
   set ::HCS12_FPROT             0x0104
   set ::HCS12_FSTAT             0x0105
   set ::HCS12_FCMD              0x0106
   set ::HCS12_FADDR             0x0108
   set ::HCS12_FDATA             0x010A

   set ::HCS12_ECLKDIV           0x0110
   set ::HCS12_EPROT             0x0114
   set ::HCS12_ESTAT             0x0115
   set ::HCS12_ECMD              0x0116
   set ::HCS12_EADDR             0x0118
   set ::HCS12_EDATA             0x011A

   set ::HCS12_NVSEC             0xFF0E  ;# actually NVSEC-1 as word aligned

   set ::HCS12_FSTAT_CBEIF       0x80
   set ::HCS12_FSTAT_CCIF        0x40
   set ::HCS12_FSTAT_PVIOL       0x20
   set ::HCS12_FSTAT_ACCERR      0x10
   set ::HCS12_FSTAT_BLANK       0x04
   set ::HCS12_FSTAT_FAIL        0x02  ;# Note: Has to be cleared individually
   set ::HCS12_FSTAT_DONE        0x01
   set ::HCS12_FSTAT_CLEAR       [expr ($::HCS12_FSTAT_PVIOL|$::HCS12_FSTAT_ACCERR)]

   set ::HCS12_FCMD_ERASE_VER    0x05
   set ::HCS12_FCMD_WORD_PROG    0x20
   set ::HCS12_FCMD_SECTOR_ERASE 0x40
   set ::HCS12_FCMD_MASS_ERASE   0x41

   set ::HCS12_FTSTMOD_WRALL     0x10
   
   set ::HCS12_BDMSTS            0xFF01
   set ::HCS12_BDMSTS_ENBDM      0x80
   set ::HCS12_BDMSTS_BDMACT     0x40
   set ::HCS12_BDMSTS_CLKSW      0x04
   set ::HCS12_BDMSTS_UNSEC      0x02

   set ::HCS12_PRDIV8            0x40
   
   set ::INITEE                  0x0012 ;# EEPROM Mapping
   set ::INITEE_EEON             0x01
   set ::INITEE_EE_MASK          0xF8

   set ::FLASH_REGIONS         "" ;# List of addresses within each unique flash region (incl. eeprom)
}

;######################################################################################
;#
;# @param flashRegions - list of flash array addresses
;#
proc initTarget { flashRegions } {
   ;# puts "initTarget {}"
   set ::FLASH_REGIONS  $flashRegions 
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc initFlash { busFrequency } {
   ;#  puts "initFlash {}"
   
   set cfmclkd [calculateFlashDivider $busFrequency]

   ;# Set up Flash divider
   wb $::HCS12_FCLKDIV $cfmclkd ;# Flash divider
   wb $::HCS12_FPROT 0xFF       ;# unprotect Flash

   foreach flashRegion $::FLASH_REGIONS {
      ;# puts "flashRegion = $flashRegion"
      lassign  $flashRegion type address
      ;# puts "type='$type', address='$address'"
      switch $type {
         "MemEEPROM" {
            ;# Set up Eeprom divider
            wb $::HCS12_ECLKDIV $cfmclkd
            wb $::HCS12_EPROT 0xFF
            ;# Re-map EEPROM to base address
            wb $::INITEE [expr $::INITEE_EEON|(($address>>8)&$::INITEE_EE_MASK)]
         }
         default {
         }
      }
   }
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc calculateFlashDivider { busFrequency } {
;# NOTES:
;# According to data sheets the Flash uses the Oscillator clock for timing but 
;# it is also influenced by the bus period.
;#
;# This code assumes that oscillator clock = 2 * bus clock
;#
   ;#  puts "calculateFlashDivider {}"
   set clockFreq [expr 2*$busFrequency]
   
   if { [expr $busFrequency < 1000] } {
      error "Clock too low for flash programming"
   }
   set cfmclkd 0
   if { [expr $clockFreq > 12800] } {
      set cfmclkd [expr $::HCS12_PRDIV8 + round(floor(0.249999+1.25*($busFrequency/1000.0)))]
      set flashClk [expr $clockFreq / (8*(($cfmclkd&0x3F)+1))]
   } else {
      set cfmclkd [expr round(floor(0.99999+($busFrequency/100.0)))+1]
      set flashClk [expr ($clockFreq) / (($cfmclkd&0x3F)+1)]
   }
   ;# puts "cfmclkd = $cfmclkd, flashClk = $flashClk"
   if { [expr ($flashClk<150)] } {
      error "Not possible to find suitable flash clock divider"
   }      
   return $cfmclkd
}

;######################################################################################
;#
;#  cmd     - command to execute
;#  address - flash address to use
;#  value   - data value to use
;#
proc executeFlashCommand { cmd address value } {

   ;#  puts "executeFlashCommand {}"
   
   wb $::HCS12_FSTAT    $::HCS12_FSTAT_CLEAR      ;# Clear PVIOL/ACCERR
   wb $::HCS12_FTSTMOD  0x00                      ;# Clear WRALL?
   wb $::HCS12_FSTAT    $::HCS12_FSTAT_FAIL       ;# Clear FAIL
   if [ expr (($cmd == $::HCS12_FCMD_MASS_ERASE) || ($cmd == $::HCS12_FCMD_ERASE_VER)) ] {
      wb $::HCS12_FTSTMOD $::HCS12_FTSTMOD_WRALL  ;# Apply to all flash blocks
      ww $::HCS12_FADDR    $address               ;# Flash address
      ww $::HCS12_FDATA    $value                 ;# Flash data
   } else {
      ww $address $value                          ;# Write to flash to buffer address and data
   }
   wb $::HCS12_FCMD     $cmd                      ;# Write command
   wb $::HCS12_FSTAT    $::HCS12_FSTAT_CBEIF      ;# Clear CBEIF to execute the command 
   
   ;# Wait for command completion
   set flashBusy 1
   set retry 0
   while { $flashBusy } {
      after 20
      set status [rb $::HCS12_FSTAT]
      set flashBusy  [expr ($status & $::HCS12_FSTAT_CCIF) == 0x00]
      set flashError [expr ($status & ($::HCS12_FSTAT_PVIOL|$::HCS12_FSTAT_ACCERR))]
      if [expr $flashError != 0] {
         break;
      }
      if [expr $retry == 20] {
         break;
      }
      incr retry
   }
   wb $::HCS12_FTSTMOD  0x00                      ;# Clear WRALL
   if [ expr ($flashError || ($retry>=20)) ] {
      ;#  puts [ format "Flash command error HCS12_FSTAT=0x%02X, retry=%d" $status $retry ]
      error "Flash command failed"
   }
}

;######################################################################################
;#
;#  cmd     - command to execute
;#  address - EEPROM address to use
;#  value   - data value to use
;#
proc executeEepromCommand { cmd address value } {

   ;#  puts "executeEepromCommand {}"
   
   wb $::HCS12_ESTAT    $::HCS12_FSTAT_CLEAR  ;# Clear PVIOL/ACCERR
   wb $::HCS12_ESTAT    $::HCS12_FSTAT_FAIL   ;# Clear FAIL
   if [ expr (($cmd == $::HCS12_FCMD_MASS_ERASE) || ($cmd == $::HCS12_FCMD_ERASE_VER)) ] {
      ww $::HCS12_EADDR $address              ;# EEPROM address
      ww $::HCS12_EDATA $value                ;# EEPROM data
   } else {
      ww $address $value                      ;# Write to EEPROM to buffer address and data
   }
   wb $::HCS12_ECMD     $cmd                  ;# Write command
   wb $::HCS12_ESTAT    $::HCS12_FSTAT_CBEIF  ;# Clear CBEIF to execute the command 
   
   ;# Wait for command completion
   set flashBusy 1
   set retry 0
   while { $flashBusy } {
      after 20
      set status [rb $::HCS12_ESTAT]
      set flashBusy  [expr ($status & $::HCS12_FSTAT_CCIF) == 0x00]
      set flashError [expr ($status & ($::HCS12_FSTAT_PVIOL|$::HCS12_FSTAT_ACCERR))]
      if [expr $flashError != 0] {
         break;
      }
      if [expr $retry == 20] {
         break;
      }
      incr retry
   }
   if [ expr ($flashError || ($retry>=20)) ] {
      ;#  puts [ format "EEPROM command error HCS12_ESTAT=0x%02X, retry=%d" $status $retry ]
      error "EEPROM command failed"
   }
}

;######################################################################################
;#  Target is erased & unsecured
;#
proc massEraseTarget { } {

   ;#  puts "massEraseTarget{}"
   
   ;# No initial connect as may fail.  Assumed done by caller.

   initFlash [expr [speed]/1000]  ;# Flash speed calculated from BDM connection speed

   foreach flashRegion $::FLASH_REGIONS {
      ;# puts "flashRegion = $flashRegion"
      lassign  $flashRegion type address
      ;# puts "type='$type', address='$address'"
      switch $type {
         "MemEEPROM" {
             ;# puts "Erasing Eeprom @$address"
             executeEepromCommand $::HCS12_FCMD_MASS_ERASE $address 0xFFFF
         }
         "MemFLASH" {
             ;# puts "Erasing Flash @$address"
             executeFlashCommand  $::HCS12_FCMD_MASS_ERASE $address  0xFFFF
         }
         default {
             ;# Ignore others
         }
       }
   }
   ;# Device is now Blank but may not be unsecured
   ;#  puts "Checking if target is already unsecured"
   set securityValue [rb $::HCS12_FSEC]
   if [ expr ( $securityValue & $::HCS12_FSEC_SEC_MASK ) == $::HCS12_FSEC_SEC_UNSEC ] {
      ;# Target was originally unsecured - just return now
      ;#  puts "Target was originally unsecured"
      return
   }
   ;# Reset & re-connect for changes to be effected.  Device is partially unsecured
   ;# BDMSTS.UNSEC == 1
   reset s h
   connect   ;# shouldn't fail
   
   ;# Program security location to unsecured value
   initTarget $::FLASH_REGIONS
   initFlash [expr [speed]/1000]   ;# Flash speed calculated from BDM connection speed
   executeFlashCommand  $::HCS12_FCMD_WORD_PROG $::HCS12_NVSEC $::HCS12_FSEC_UNSEC_VALUE
   
   ;# Reset to have unsecured (finally) but not blank device
   reset s h
   connect   ;# shouldn't fail

   ;# Confirm unsecured
   ;#  puts "Checking if target is unsecured"
   set securityValue [rb $::HCS12_FSEC]
   if [ expr ( $securityValue & $::HCS12_FSEC_SEC_MASK ) != $::HCS12_FSEC_SEC_UNSEC ] {
      ;#  puts "Target failed to unsecure"
      error "Target failed to unsecure"
      return
   }
   ;# Erase security location so device is unsecured and blank!
   initTarget $::FLASH_REGIONS
   initFlash [expr [speed]/1000]   ;# Flash speed calculated from BDM connection speed
   executeFlashCommand  $::HCS12_FCMD_SECTOR_ERASE $::HCS12_NVSEC 0xFFFF

   ;# Flash is now Blank and unsecured
}

;######################################################################################
;#
proc isUnsecure { } {
   ;#  puts "Checking if unsecured"
   set securityValue [rb $::HCS12_FSEC]

   if [ expr ( $securityValue & $::HCS12_FSEC_SEC_MASK ) != $::HCS12_FSEC_SEC_UNSEC ] {
      error "Target is secured"
   }
}

;######################################################################################
;# Actions on initial load
;#
loadSymbols

;#]]>