/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _PLX_REGS_6466_H_
#define _PLX_REGS_6466_H_

/*******************************************************************************
* File - plx_regs_6466.h - PLX6466 configuration space and address spaces
*                          definitions
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*******************************************************************************/

/* -----------------------------------------------
    PLX6466 configuration registers offsets
   ----------------------------------------------- */
enum {
    PLX6466_VID   = 0x00, /* Vendor ID */
    PLX6466_DID   = 0x02, /* Device ID */
    PLX6466_CR    = 0x04, /* Command register */
    PLX6466_PSR   = 0x06, /* Primary Status register */
    PLX6466_REV   = 0x08, /* Revision ID */
    PLX6466_CCR   = 0x09, /* Class code */
    PLX6466_CCSC  = 0x0a, /* Sub class code */
    PLX6466_CCBC  = 0x0b, /* Base class code */
    PLX6466_CLSR  = 0x0c, /* Cache line size */
    PLX6466_PLTR  = 0x0d, /* Primary latency timer */
    PLX6466_HDR   = 0x0e, /* Header type */
    PLX6466_BISTR = 0x0f, /* Built-in self test */
    PLX6466_BAR0  = 0x10, /* Base address register */
    PLX6466_BAR1  = 0x14, /* Base address register */
    PLX6466_PBN   = 0x18, /* Primary bus number */
    PLX6466_SBN   = 0x19, /* Secondary bus number */
    PLX6466_SUBN  = 0x1a, /* Subordinate bus number */
    PLX6466_SLTR  = 0x1b, /* Secondary latency timer */
    PLX6466_IOB   = 0x1c, /* I/O Base */
    PLX6466_IOL   = 0x1d, /* I/O Limit */
    PLX6466_SSR   = 0x1e, /* Secondary Status register */
    PLX6466_MEMB  = 0x20, /* Memory Base */
    PLX6466_MEML  = 0x22, /* Memory Limit */
    PLX6466_PMB   = 0x24, /* Prefetchable Memory Base */
    PLX6466_PML   = 0x26, /* Prefetchable Memory Limit */
    PLX6466_PMBU  = 0x28, /* Prefetchable Memory Base Upper 32 bits */
    PLX6466_PMLU  = 0x2c, /* Prefetchable Memory Limit Upper 32 bits */
    PLX6466_CAP   = 0x34, /* New capability pointer */
    PLX6466_IPR   = 0x3d, /* Interrupt pin */
    PLX6466_BCTL  = 0x3e, /* Bridge Control */
    PLX6466_CCTL  = 0x40, /* Chip Control */
    PLX6466_DCTL  = 0x41, /* Diagnostic Control */
    PLX6466_ACTL  = 0x42, /* Arbiter Control */
    PLX6466_PFTC  = 0x44, /* Primary Flow-Through Control */
    PLX6466_TCTL  = 0x45, /* Timeout Control */
    PLX6466_MOP   = 0x46, /* Miscellanous Options */
    PLX6466_PIPC  = 0x48, /* Primary Initial Prefetch Count*/
    PLX6466_SIPC  = 0x49, /* Secondary Initial Prefetch Count*/
    PLX6466_PINPC = 0x4a, /* Primary Incremental Prefetch Count*/
    PLX6466_SINPC = 0x4b, /* Secondary Incremental Prefetch Count*/
    PLX6466_PMPC  = 0x4c, /* Primary Maximum Prefetch Count*/
    PLX6466_SMPC  = 0x4d, /* Secondary Maximum Prefetch Count*/
    PLX6466_SFTC  = 0x4e, /* Secondary Flow-Through Control */
    PLX6466_BUFC  = 0x4f, /* Buffer Control */
    PLX6466_IAC   = 0x50, /* Internal Arbiter Control */
    PLX6466_TST   = 0x52, /* Test */
    PLX6466_SEEC  = 0x54, /* Serial EEPROM Control */
    PLX6466_SEEA  = 0x55, /* Serial EEPROM Address */
    PLX6466_SEED  = 0x56, /* Serial EEPROM Data */
    PLX6466_TCN   = 0x61, /* Timer Control */
    PLX6466_TCO   = 0x62, /* Timer Counter */
    PLX6466_PSED  = 0x64, /* P_SERR# Event Disable */
    PLX6466_GOD0  = 0x65, /* GPIO[3:0] Output Data */
    PLX6466_GOE0  = 0x66, /* GPIO[3:0] Output Enable */
    PLX6466_GID0  = 0x67, /* GPIO[3:0] Input Data */
    PLX6466_CLKC  = 0x68, /* Cloack Control */
    PLX6466_PSES  = 0x6a, /* P_SERR# Status */
    PLX6466_CLKR  = 0x6b, /* Cloack Run */
    PLX6466_PRMB  = 0x6c, /* Private Memory Base */
    PLX6466_PRML  = 0x6e, /* Private Memory Limit */
    PLX6466_PRMBU = 0x70, /* Private Memory Base Upper 32 bits */
    PLX6466_PRMLU = 0x74, /* Private Memory Limit Upper 32 bits */
    PLX6466_HSRO  = 0x9c, /* Hot Swap Switch and Read-Only Register Control */
    PLX6466_GOD4  = 0x9d, /* GPIO[7:4] Output Data */
    PLX6466_GOE4  = 0x9e, /* GPIO[7:4] Output Enable */
    PLX6466_GID4  = 0x9f, /* GPIO[7:4] Input Data */
    PLX6466_PAS   = 0xa0, /* Power-Up Status */
    PLX6466_GOD8  = 0xa1, /* GPIO[15:14, 12:8] Output Data */
    PLX6466_GOE8  = 0xa2, /* GPIO[15:14, 12:8] Output Enable */
    PLX6466_GID8  = 0xa3, /* GPIO[15:14, 12:8] Input Data */
    PLX6466_ERI   = 0xd3, /* Extended Register Index */
    PLX6466_ERD   = 0xd4, /* Extended Register Data */
    PLX6466_PMCI  = 0xdc, /* Power Management Capability ID (01h) */
    PLX6466_PMNCP = 0xdd, /* Power Management Next Capability Pointer (E4h) */
    PLX6466_PMC   = 0xde, /* Power Management Capabilities */
    PLX6466_PMCS  = 0xe0, /* Power Management Control/Status */
    PLX6466_PBSE  = 0xe2, /* PMCSR Bridge Supports Extension */
    PLX6466_PMD   = 0xe3, /* Power Management Data */
    PLX6466_HSCL  = 0xe4, /* Hot Swap Control (Capability ID) (06h) */
    PLX6466_HSNCP = 0xe5, /* Hot Swap Next Capability Pointer (E8h) */
    PLX6466_HSCS  = 0xe6, /* Hot Swap Control/Status (0h) */
    PLX6466_VCI   = 0xe8, /* VPD Capability ID (03h) */
    PLX6466_VNCP  = 0xe9, /* VPD Capability Pointer (F0h) */
    PLX6466_VPDA  = 0xea, /* VPD AdDress (0h) */
    PLX6466_VPDD  = 0xec /* VPD Data (0h) */
};

/* PLX6466 base address spaces (BARs) */
enum {
    AD_PLX6466_BAR0 = 0,
    AD_PLX6466_BAR1 = 1,
};

#endif
