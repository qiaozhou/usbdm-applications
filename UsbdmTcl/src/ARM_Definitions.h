/*
 * ARM_Definitions.h
 *
 *  Created on: 06/04/2011
 *      Author: podonoghue
 */

#ifndef ARM_DEFINITIONS_H_
#define ARM_DEFINITIONS_H_

#define ARM_PAGE_SIZE (1<<10)

#define ARM_Cortex_M3_IDCODE (0x3BA00477)
#define ARM_Cortex_M4_IDCODE (0x4BA00477)

// ARM JTAG Commands
#define ARM_JTAG_MASTER_IR_LENGTH   (4)     // IR length for commands below

#define JTAG_IDCODE_LENGTH          (32)
#define JTAG_IDCODE_COMMAND         (0x00)  // Device ID Code Register (IDCODE) reg
#define JTAG_EZPORT_IDCODE_LENGTH   (32)
#define JTAG_EZPORT_IDCODE_COMMAND  (0x01)  // EZPORT reg
#define JTAG_DP_ABORT_SEL_LENGTH    (35)
#define JTAG_DP_ABORT_SEL_COMMAND   (0x08)  // JTAG-DP Abort Register (ABORT)
#define JTAG_DP_DPACC_SEL_LENGTH    (35)
#define JTAG_DP_DPACC_SEL_COMMAND   (0x0A)  // JTAG-DP DP Access Register (DPACC)
#define JTAG_DP_APACC_SEL_LENGTH    (35)
#define JTAG_DP_APACC_SEL_COMMAND   (0x0B)  // JTAG-DP AP Access Register (APACC)
#define JTAG_ARM_IDCODE_LENGTH      (32)
#define JTAG_ARM_IDCODE_COMMAND     (0x0E)  // ARM Device ID Code Register (IDCODE)
#define JTAG_BYPASS_LENGTH          (1)
#define JTAG_BYPASS_COMMAND         (~0x00) // BYPASS reg

// Responses from DP/AP access
#define ACK_OK_FAULT       (0x02) //!< Access completed (either OK or FAULT)
#define ACK_WAIT           (0x01) //!< Access incomplete - try again

// Mask for DPACC
// Note: As used in access i.e. A[3:2] as x[1:0]
#define DP_CTRL_STAT_REG   (0x2) //!< R/W access DP STATUS/CONTROL registers
#define DP_SELECT_REG      (0x4) //!< R/W access AP SELECT register (controls AP access address)
#define DP_RDBUFF_REG      (0x6) //!< RAX/WI access to RDBUFF register

#define DP_WRITE           (0x0)
#define DP_READ            (0x1)

#define DP_AP_DP_MASK      (0x8) //!< Mask used to select b/w DP & AP registers
#define DP_DP_SELECT       (0x0)           //!< Select DP registers
#define DP_AP_SELECT       (DP_AP_DP_MASK) //!< Select AP registers

// DP CTRL/STAT register masks
#define CSYSPWRUPACK       (1<<31)
#define CSYSPWRUPREQ       (1<<30)
#define CDBGPWRUPACK       (1<<29)
#define CDBGPWRUPREQ       (1<<28)
#define CDBGSTACK          (1<<27)
#define CDBGSTREQ          (1<<26)
#define TRNCNT_OFF         (12)
#define TRNCNT(N)          (((N)&0x7FF)<<TRNCNT_OFF)
#define MASKLANE_OFF       (8)
#define MASKLANE           (0xF<<MASKLANE_OFF)
#define STICKYERR          (1<<5)
#define STICKYCMP          (1<<4)
#define TRNMODE_OFF        (2)
#define TRNMODE_NORMAL     (0<<TRNMODE_OFF)
#define TRNMODE_VERIFY     (1<<TRNMODE_OFF)
#define TRNMODE_COMPARE    (2<<TRNMODE_OFF)
#define STICKYORUN         (1<<1)
#define ORUNDETECT         (1<<0)

// DP SELECT register masks
#define APSEL_OFFSET       (24)                          // Select AP to access
#define APSEL(N)           (((N)&0xFF)<<APSEL_OFFSET)
#define APSEL_MASK         (0xFF<<APSEL_OFFSET)
#define APBANKSEL_OFFSET   (4)                           // Selects register bank withing AP
#define APBANKSEL(N)       (((N)&0x3)<<APBANKSEL_OFFSET)
#define APBANKSEL_MASK     (0xF<<APBANKSEL_OFFSET)
#define APBANKSEL0         (APBANKSEL(0))
#define APBANKSEL1         (APBANKSEL(1))
#define APBANKSEL2         (APBANKSEL(2))
#define APBANKSEL3         (APBANKSEL(3))

// Access port select - There are two APs defined in the Kinetis chips
#define MDM_AP (1) // Access MDM-AP    - Freescale specific AP
#define AHB_AP (0) // Access AHB-AP    - Memory access (generic MEM-AP)

// The following define addresses in the AP 'address space'
// The address is divided between:
// A[31:24] => DP-AP-SELECT[31:24] (AP Select)
// A[7:4]   => DP-AP-SELECT[7:4]   (Bank select within AP)
// A[3:2]   => APACC[3:2]          (Register select within bank)

// AP#1 - MDM-AP
#define MDM_AP_Status  (0x01000000UL) // MDM-AP Status Register address
#define MDM_AP_Control (0x01000004UL) // MDM-AP Control Register address
#define MDM_AP_Id      (0x010000FCUL) // MDM-AP ID Register address

#define MDM_AP_Flash_Mass_Erase_Ack          (1<<0)
#define MDM_AP_Flash_Ready                   (1<<1)
#define MDM_AP_System_Security               (1<<2)
#define MDM_AP_System_Reset                  (1<<3)
#define MDM_AP_Mass_Erase_Enable             (1<<5)
#define MDM_AP_Backdoor_Access_Enable        (1<<6)
#define MDM_AP_LP_Enable                     (1<<7)
#define MDM_AP_VLP_Enable                    (1<<8)
#define MDM_AP_LLS_Mode_Exit                 (1<<9)
#define MDM_AP_VLLSx_Mode_Exit               (1<<10)
#define MDM_AP_Status_Core_Halted            (1<<16)
#define MDM_AP_Status_Core_SLEEPDEEP         (1<<17)
#define MDM_AP_Status_Core_SLEEPING          (1<<18)

#define MDM_AP_Control_Flash_Mass_Erase      (1<<0)
#define MDM_AP_Control_Debug_Disable         (1<<1)
#define MDM_AP_Control_Debug_Request         (1<<2)
#define MDM_AP_Control_System_Reset_Request  (1<<3)
#define MDM_AP_Control_Core_Hold_Reset       (1<<4)
#define MDM_AP_Control_VLLDBGREQ             (1<<5)
#define MDM_AP_Control_VLLDBGACK             (1<<6)
#define MDM_AP_Control_Status_Ack            (1<<7)

// AP#0 - AHB-AP
// use ARM_CRegisters_t
//#define AHB_AP_CSW     (0x00000000U) // AHB-AP Control/Status Word register
//#define AHB_AP_TAR     (0x00000004U) // AHB-AP Transfer Address register
//#define AHB_AP_DRW     (0x0000000CU) // AHB-AP Data Read/Write register
//
//#define AHB_AP_CFG     (0x000000F4U) // AHB-AP Config register
//#define AHB_AP_Base    (0x000000F8U) // AHB-AP IDebug base address register
//#define AHB_AP_Id      (0x000000FCU) // AHB-AP ID Register

// AHB-AP (MEM-AP) CSW Register masks
#define AHB_AP_CSW_INC_SINGLE    (1<<4)
#define AHB_AP_CSW_INC_PACKED    (2<<4)
#define AHB_AP_CSW_INC_MASK      (3<<4)
#define AHB_AP_CSW_SIZE_BYTE     (0<<0)
#define AHB_AP_CSW_SIZE_HALFWORD (1<<0)
#define AHB_AP_CSW_SIZE_WORD     (2<<0)
#define AHB_AP_CSW_SIZE_MASK     (7<<0)

// AHB-AP (MEM-AP) CFG Register masks
#define AHB_AP_CFG_BIGENDIAN    (1<<0)

// The following are addresses in the target memory space (Accessed through AHB-AP
#define DHCSR (0xE000EDF0) // RW Debug Halting Control and Status Register
#define DCRSR (0xE000EDF4) // WO Debug Core Register Selector Register
#define DCRDR (0xE000EDF8) // RW Debug Core Register Data Register
#define DEMCR (0xE000EDFC) // RW Debug Exception and Monitor Control Register
#define DFSR  (0xE000ED30) // Debug Fault Status Register
#define AISCR (0xE000ED0C) // Application Interrupt and Reset Control Register

#define MC_SRSH (0x4007E000) // System Reset Status Register High
#define MC_SRSL (0x4007E001) // System Reset Status Register Low

#define DHCSR_DBGKEY       (0xA05F<<16)
#define DHCSR_S_RESET_ST   (1<<25)
#define DHCSR_S_RETIRE_ST  (1<<24)
#define DHCSR_S_LOCKUP     (1<<19)
#define DHCSR_S_SLEEP      (1<<18)
#define DHCSR_S_HALT       (1<<17)
#define DHCSR_S_REGRDY     (1<<16)
#define DHCSR_C_SNAPSTALL  (1<<5)
#define DHCSR_C_MASKINTS   (1<<3)
#define DHCSR_C_STEP       (1<<2)
#define DHCSR_C_HALT       (1<<1)
#define DHCSR_C_DEBUGEN    (1<<0)

#define DCRSR_WRITE    (1<<16)
#define DCRSR_READ     (0<<16)
#define DCRSR_REGMASK  (0x7F)

#define DEMCR_TRCENA        (1<<24)
#define DEMCR_VC_HARDERR    (1<<10)
#define DEMCR_VC_INTERR     (1<<9)
#define DEMCR_VC_BUSERR     (1<<8)
#define DEMCR_VC_STATERR    (1<<7)
#define DEMCR_VC_CHKERR     (1<<6)
#define DEMCR_VC_NOCPERR    (1<<5)
#define DEMCR_VC_MMERR      (1<<4)
#define DEMCR_VC_CORERESET  (1<<0)
#define DEMCR_VC_ALL_ERRORS (DEMCR_VC_HARDERR|DEMCR_VC_INTERR|DEMCR_VC_BUSERR|DEMCR_VC_STATERR|DEMCR_VC_CHKERR|DEMCR_VC_NOCPERR|DEMCR_VC_MMERR)

#define DFSR_EXTERN    (1<<4)
#define DFSR_VCATCH    (1<<3)
#define DFSR_DWTTRAP   (1<<2)
#define DFSR_BKPT      (1<<1)
#define DFSR_HALTED    (1<<0)

#define AISCR_VECTKEY      (0x05FA<<16)   // Key value
#define AISCR_SYSRESETREQ  (1<<2)         // System Reset Request
#define AISCR_VECTRESET    (1<<0)         // Local system reset (only in debug state!)

#define WDOG_STCTRLH (0x40052000)
#define WDOG_STCTRLL (0x40052002)
#define WDOG_TOVALH  (0x40052004)
#define WDOG_TOVALL  (0x40052006)
#define WDOG_WINH    (0x40052008)
#define WDOG_WINL    (0x4005200A)
#define WDOG_REFRESH (0x4005200C)
#define WDOG_UNLOCK  (0x4005200E)
#define WDOG_TMROUTH (0x40052010)
#define WDOG_TMROUTL (0x40052012)
#define WDOG_RSTCNT  (0x40052014)
#define WDOG_PRESC   (0x40052016)

#endif /* ARM_DEFINITIONS_H_ */
