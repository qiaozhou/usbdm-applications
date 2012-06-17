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

   set ::BDMGPR                     0xFF08
   set ::BDMGPR_BGAE                0x80

   set ::HCS12_FSEC_SEC_MASK        0x03   ;# Security bits
   set ::HCS12_FSEC_SEC_UNSEC       0x02   ;# Security bits for unsecured device
   set ::HCS12_FSEC_SEC_KEYEN       0x80   ;# Backdoor Key enable
   set ::HCS12_FSEC_UNSEC_VALUE     0xFFFE ;# Value to use when unsecuring (0xFF:NVSEC value)
                                    
   set ::HCS12_FCLKDIV              0x0100
   set ::HCS12_FSEC                 0x0101
   set ::HCS12_FCCOBIX              0x0102
   set ::HCS12_FCNFG                0x0104
   set ::HCS12_FERCNFG              0x0105
   set ::HCS12_FSTAT                0x0106
   set ::HCS12_FERSTAT              0x0107
   set ::HCS12_FPROT                0x0108
   set ::HCS12_DFPROT               0x0109
   set ::HCS12_FCCOBHI              0x010A
   set ::HCS12_FCCOBLO              0x010B
   set ::HCS12_FECCRHI              0x010E
   set ::HCS12_FECCRLO              0x010F
   set ::HCS12_FOPT                 0x0110
                                    
   set ::HCS12_NVSEC                0x7FFF08  ;# actually SEC as word aligned
                                    
   set ::HCS12_FSTAT_CCIF           0x80
   set ::HCS12_FSTAT_ACCERR         0x20
   set ::HCS12_FSTAT_FPVIOL         0x10
   set ::HCS12_FSTAT_MGBUSY         0x08
   set ::HCS12_FSTAT_MGSTAT1        0x02
   set ::HCS12_FSTAT_MGSTAT0        0x01
   set ::HCS12_FSTAT_CLEAR          [expr ($::HCS12_FSTAT_FPVIOL|$::HCS12_FSTAT_ACCERR)]

   set ::HCS12_FCMD_ERASE_VERIFY    0x01
   set ::HCS12_FCMD_PROG_P_FLASH    0x06
   set ::HCS12_FCMD_ERASE_ALL_BLKS  0x08
   set ::HCS12_FCMD_ERASE_P_SECTOR  0x0A
   set ::HCS12_FCMD_UNSECURE        0x0B
   
   set ::HCS12_BDMSTS               0xFF01
   set ::HCS12_BDMSTS_ENBDM         0x80
   set ::HCS12_BDMSTS_BDMACT        0x40
   set ::HCS12_BDMSTS_CLKSW         0x04
   set ::HCS12_BDMSTS_UNSEC         0x02
                                   
   set ::HCS12_PRDIV8               0x40
                                   
   set ::FLASH_ADDRESSES         "" ;# List of addresses within each unique flash region (incl. eeprom)
}

;######################################################################################
;#
;# @param flashAddresses - list of flash array addresses
;#
proc initTarget { flashAddresses } {
   ;# puts "initTarget {}"
   
   set ::FLASH_ADDRESSES  $flashAddresses 
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc initFlash { busFrequency } {
   ;#  puts "initFlash {}"
   
   set cfmclkd [calculateFlashDivider $busFrequency]
   ;# Set up Flash
   wb $::HCS12_FCLKDIV $cfmclkd ;# Flash divider
   wb $::HCS12_FPROT  0xFF      ;# unprotect P-Flash
   wb $::HCS12_DFPROT 0xFF      ;# unprotect D-Flash
}

;######################################################################################
;#
;#  busFrequency - Target bus busFrequency in kHz
;#
proc calculateFlashDivider { busFrequency } {
;# NOTES:
;#   According to data sheets the Flash uses the BUS clock for timing
;#
   ;# puts "calculateFlashDivider {}"
   if { [expr $busFrequency < 1000] } {
      error "Clock too low for flash programming"
   }
   set cfmclkd  [expr round(($busFrequency-1101.0)/1000)]
   set flashClk [expr round($busFrequency/($cfmclkd+1))]
   ;# puts "cfmclkd = $cfmclkd, flashClk = $flashClk"
   
   return $cfmclkd
}

;######################################################################################
;#
;#  cmd     - command to execute
;#  address - flash address to use
;#  value   - data value to use
;#
proc executeFlashCommand { cmd {address "none"} {data0 "none"} {data1 "none"} {data2 "none"} {data3 "none"} } {
   ;# puts "executeFlashCommand {}"
   
   wb $::HCS12_FSTAT   $::HCS12_FSTAT_CLEAR           ;# clear any error flags
   wb $::HCS12_FCCOBIX   0                            ;# index = 0
   wb $::HCS12_FCCOBHI $cmd                           ;# load program command
   if {$address != "none"} {
      wb $::HCS12_FCCOBLO [expr ($address>>16)&0xFF]  ;# load GPAGE
      wb $::HCS12_FCCOBIX    1                        ;# index = 1
      ww $::HCS12_FCCOBHI [expr $address&0xFFFF]      ;# load addr
      if {$data0 != "none"} { 
         wb $::HCS12_FCCOBIX    2                     ;# index = 2
         ww $::HCS12_FCCOBHI [expr $data0]            ;# load data
         wb $::HCS12_FCCOBIX    3                     ;# index = 3
         ww $::HCS12_FCCOBHI [expr $data1]            ;# load data
         wb $::HCS12_FCCOBIX    4                     ;# index = 4
         ww $::HCS12_FCCOBHI [expr $data2]            ;# load data
         wb $::HCS12_FCCOBIX    5                     ;# index = 5
         ww $::HCS12_FCCOBHI [expr $data3]            ;# load data
      }
   }
   wb $::HCS12_FSTAT $::HCS12_FSTAT_CCIF  ;# Clear CCIF to execute the command 

   ;# Wait for command completion
   set flashBusy 1
   set retry 0
   while { $flashBusy } {
      after 20
      set status [rb $::HCS12_FSTAT]
      set flashBusy  [expr ($status & $::HCS12_FSTAT_CCIF) == 0x00]
      set flashError [expr ($status & ($::HCS12_FSTAT_FPVIOL|$::HCS12_FSTAT_ACCERR))]
      if [expr $flashError != 0] {
         break;
      }
      if [expr $retry == 20] {
         break;
      }
      incr retry
   }
   if [ expr ($flashError || ($retry>=20)) ] {
      ;# puts [ format "Flash command error HCS12_FSTAT=0x%02X, retry=%d" $status $retry ]
      error "Flash command failed"
   }
}

;######################################################################################
;#  Target is erased & unsecured
;#
proc massEraseTarget { } {

   ;# puts "massEraseTarget{}"

   ;# Mass erase flash
   initFlash [expr [speed]/1000]  ;# Flash speed calculated from BDM connection speed
;#   executeFlashCommand $::HCS12_FCMD_UNSECURE
   executeFlashCommand $::HCS12_FCMD_ERASE_ALL_BLKS
   
   ;# Device is now blank & temporarily unsecured
   rb $::HCS12_NVSEC
   
   ;# Confirm unsecured
   ;# puts "Checking if target is unsecured"
   set securityValue [rb $::HCS12_FSEC]
   if [ expr ( $securityValue & $::HCS12_FSEC_SEC_MASK ) != $::HCS12_FSEC_SEC_UNSEC ] {
      ;# puts "Target failed to unsecure"
      error "Target failed to unsecure"
      return
   }
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