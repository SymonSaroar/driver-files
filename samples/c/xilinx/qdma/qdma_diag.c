/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

/****************************************************************************
*  File: qdma_diag.c
*
*  Sample user-mode diagnostics application for accessing Xilinx PCI Express
*  cards with QDMA support, using the WinDriver WDC API.
*
*  Note: This code sample is provided AS-IS and as a guiding sample only.
*****************************************************************************/

#include "wdc_diag_lib.h"
#include "pci_menus_common.h"
#include "qdma_lib.h"
#include "status_strings.h"

/*************************************************************
  General definitions
 *************************************************************/
 /* Error messages display */
int QDMA_printf(char *fmt, ...)
#if defined(LINUX)
__attribute__((format(printf, 1, 2)))
#endif
;

/* TODO: Change the device ID value to match your specific device. */
#define QDMA_DEFAULT_VENDOR_ID 0x10EE
const DWORD QDMA_DefaultDeviceIds[QDMA_NUM_PF_FUNC] = {
    0x9038, 0x9138, 0x9238, 0x9338 };

#define QDMA_ERR QDMA_printf
#define MAX_DMA_REQUESTS 256
#define DEFAULT_H2C_RING_SZ_INDEX 15
#define DEFAULT_C2H_RING_SZ_INDEX 15

static const char *cDmaQueueStates[] = { "available", "programmed", "started" };
static const char *cQueueOperation[] = { "add", "remove", "start", "stop" };

/*************************************************************
  Global variables
 *************************************************************/
static DMA_REQUEST_CONTEXT *gDmaRequests[MAX_DMA_REQUESTS];
static DWORD gDmaRequestsIndex = 0;

/* --------------------------------------------------
    QDMA configuration registers information
   -------------------------------------------------- */
   /* Configuration registers information array */
static const WDC_REG gQDMA_CfgRegs[] = {
    { WDC_AD_CFG_SPACE, PCI_VID, WDC_SIZE_16, WDC_READ_WRITE, "VID",
        "Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_DID, WDC_SIZE_16, WDC_READ_WRITE, "DID",
        "Device ID" },
    { WDC_AD_CFG_SPACE, PCI_CR, WDC_SIZE_16, WDC_READ_WRITE, "CMD",
        "Command" },
    { WDC_AD_CFG_SPACE, PCI_SR, WDC_SIZE_16, WDC_READ_WRITE, "STS", "Status" },
    { WDC_AD_CFG_SPACE, PCI_REV, WDC_SIZE_32, WDC_READ_WRITE, "RID_CLCD",
        "Revision ID & Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCSC, WDC_SIZE_8, WDC_READ_WRITE, "SCC",
        "Sub Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CCBC, WDC_SIZE_8, WDC_READ_WRITE, "BCC",
        "Base Class Code" },
    { WDC_AD_CFG_SPACE, PCI_CLSR, WDC_SIZE_8, WDC_READ_WRITE, "CALN",
        "Cache Line Size" },
    { WDC_AD_CFG_SPACE, PCI_LTR, WDC_SIZE_8, WDC_READ_WRITE, "LAT",
        "Latency Timer" },
    { WDC_AD_CFG_SPACE, PCI_HDR, WDC_SIZE_8, WDC_READ_WRITE, "HDR",
        "Header Type" },
    { WDC_AD_CFG_SPACE, PCI_BISTR, WDC_SIZE_8, WDC_READ_WRITE, "BIST",
        "Built-in Self Test" },
    { WDC_AD_CFG_SPACE, PCI_BAR0, WDC_SIZE_32, WDC_READ_WRITE, "BADDR0",
        "Base Address 0" },
    { WDC_AD_CFG_SPACE, PCI_BAR1, WDC_SIZE_32, WDC_READ_WRITE, "BADDR1",
        "Base Address 1" },
    { WDC_AD_CFG_SPACE, PCI_BAR2, WDC_SIZE_32, WDC_READ_WRITE, "BADDR2",
        "Base Address 2" },
    { WDC_AD_CFG_SPACE, PCI_BAR3, WDC_SIZE_32, WDC_READ_WRITE, "BADDR3",
        "Base Address 3" },
    { WDC_AD_CFG_SPACE, PCI_BAR4, WDC_SIZE_32, WDC_READ_WRITE, "BADDR4",
        "Base Address 4" },
    { WDC_AD_CFG_SPACE, PCI_BAR5, WDC_SIZE_32, WDC_READ_WRITE, "BADDR5",
        "Base Address 5" },
    { WDC_AD_CFG_SPACE, PCI_CIS, WDC_SIZE_32, WDC_READ_WRITE, "CIS",
        "CardBus CIS Pointer" },
    { WDC_AD_CFG_SPACE, PCI_SVID, WDC_SIZE_16, WDC_READ_WRITE, "SVID",
        "Sub-system Vendor ID" },
    { WDC_AD_CFG_SPACE, PCI_SDID, WDC_SIZE_16, WDC_READ_WRITE, "SDID",
        "Sub-system Device ID" },
    { WDC_AD_CFG_SPACE, PCI_EROM, WDC_SIZE_32, WDC_READ_WRITE, "EROM",
        "Expansion ROM Base Address" },
    { WDC_AD_CFG_SPACE, PCI_CAP, WDC_SIZE_8, WDC_READ_WRITE, "NEW_CAP",
        "New Capabilities Pointer" },
    { WDC_AD_CFG_SPACE, PCI_ILR, WDC_SIZE_32, WDC_READ_WRITE, "INTLN",
        "Interrupt Line" },
    { WDC_AD_CFG_SPACE, PCI_IPR, WDC_SIZE_32, WDC_READ_WRITE, "INTPIN",
        "Interrupt Pin" },
    { WDC_AD_CFG_SPACE, PCI_MGR, WDC_SIZE_32, WDC_READ_WRITE, "MINGNT",
        "Minimum Required Burst Period" },
    { WDC_AD_CFG_SPACE, PCI_MLR, WDC_SIZE_32, WDC_READ_WRITE, "MAXLAT",
        "Maximum Latency" },
};

#define QDMA_CFG_REGS_NUM (sizeof(gQDMA_CfgRegs)/sizeof(WDC_REG))

/* -----------------------------------------------
    QDMA config block registers information
   ----------------------------------------------- */
   /* Config block registers information array.
    * Note: The address space will be set after opening the device */
static const WDC_REG gQDMA_ConfigRegs[] = {
    { (DWORD)-1, 0x00,  WDC_SIZE_32, WDC_READ, "CFG_BLOCK_ID", ""},
    { (DWORD)-1, 0x04,  WDC_SIZE_32, WDC_READ, "CFG_BUSDEV", ""},
    { (DWORD)-1, 0x08,  WDC_SIZE_32, WDC_READ, "CFG_PCIE_MAX_PL_SZ", ""},
    { (DWORD)-1, 0x0C,  WDC_SIZE_32, WDC_READ, "CFG_PCIE_MAX_RDRQ_SZ", ""},
    { (DWORD)-1, 0x10,  WDC_SIZE_32, WDC_READ, "CFG_SYS_ID", ""},
    { (DWORD)-1, 0x14,  WDC_SIZE_32, WDC_READ, "CFG_MSI_EN", ""},
    { (DWORD)-1, 0x18,  WDC_SIZE_32, WDC_READ, "CFG_PCIE_DATA_WIDTH", ""},
    { (DWORD)-1, 0x1C,  WDC_SIZE_32, WDC_READ_WRITE, "CFG_PCIE_CTRL", ""},
    { (DWORD)-1, 0x40,  WDC_SIZE_32, WDC_READ, "CFG_AXI_USR_MAX_PL_SZ", ""},
    { (DWORD)-1, 0x44,  WDC_SIZE_32, WDC_READ, "CFG_AXI_USR_MAX_RDRQ_SZ", ""},
    { (DWORD)-1, 0x60,  WDC_SIZE_32, WDC_READ, "CFG_WR_FLUSH_TIMEOUT", ""},

    { (DWORD)-1, 0x100, WDC_SIZE_32, WDC_READ, "GLBL2_ID", ""},
    { (DWORD)-1, 0x104, WDC_SIZE_32, WDC_READ, "GLBL2_PF_BL_INT", ""},
    { (DWORD)-1, 0x108, WDC_SIZE_32, WDC_READ, "GLBL2_PF_VF_BL_INT", ""},
    { (DWORD)-1, 0x10C, WDC_SIZE_32, WDC_READ, "GLBL2_PF_BL_EXT", ""},
    { (DWORD)-1, 0x110, WDC_SIZE_32, WDC_READ, "GLBL2_PF_VF_BL_EXT", ""},
    { (DWORD)-1, 0x114, WDC_SIZE_32, WDC_READ, "GLBL2_CHNL_INST", ""},
    { (DWORD)-1, 0x118, WDC_SIZE_32, WDC_READ, "GLBL2_CHNL_QDMA", ""},
    { (DWORD)-1, 0x11C, WDC_SIZE_32, WDC_READ, "GLBL2_CHNL_STRM", ""},
    { (DWORD)-1, 0x120, WDC_SIZE_32, WDC_READ, "GLBL2_QDMA_CAP", ""},
    { (DWORD)-1, 0x128, WDC_SIZE_32, WDC_READ, "GLBL2_PASID_CAP", ""},
    { (DWORD)-1, 0x12C, WDC_SIZE_32, WDC_READ, "GLBL2_FUNC_RET", ""},
    { (DWORD)-1, 0x130, WDC_SIZE_32, WDC_READ, "GLBL2_SYS_ID", ""},
    { (DWORD)-1, 0x134, WDC_SIZE_32, WDC_READ, "GLBL2_MISC_CAP", ""},
    { (DWORD)-1, 0x1B8, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_PCIE_RQ_0", ""},
    { (DWORD)-1, 0x1BC, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_PCIE_RQ_1", ""},
    { (DWORD)-1, 0x1C0, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_AXIMM_WR_0", ""},
    { (DWORD)-1, 0x1C4, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_AXIMM_WR_1", ""},
    { (DWORD)-1, 0x1C8, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_AXIMM_RD_0", ""},
    { (DWORD)-1, 0x1CC, WDC_SIZE_32, WDC_READ, "GLBL2_DBG_AXIMM_RD_1", ""},

    { (DWORD)-1, 0x204, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_0", ""},
    { (DWORD)-1, 0x208, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_1", ""},
    { (DWORD)-1, 0x20C, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_2", ""},
    { (DWORD)-1, 0x210, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_3", ""},
    { (DWORD)-1, 0x214, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_4", ""},
    { (DWORD)-1, 0x218, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_5", ""},
    { (DWORD)-1, 0x21C, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_6", ""},
    { (DWORD)-1, 0x220, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_7", ""},
    { (DWORD)-1, 0x224, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_8", ""},
    { (DWORD)-1, 0x228, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_9", ""},
    { (DWORD)-1, 0x22C, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_10", ""},
    { (DWORD)-1, 0x230, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_11", ""},
    { (DWORD)-1, 0x234, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_12", ""},
    { (DWORD)-1, 0x238, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_13", ""},
    { (DWORD)-1, 0x23C, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_14", ""},
    { (DWORD)-1, 0x240, WDC_SIZE_32, WDC_READ, "GLBL_RNGSZ_15", ""},

    { (DWORD)-1, 0x244, WDC_SIZE_32, WDC_READ_WRITE, "GLBL_SCRATCH", ""},
    { (DWORD)-1, 0x248, WDC_SIZE_32, WDC_READ, "GLBL_ERR_STAT", ""},
    { (DWORD)-1, 0x24C, WDC_SIZE_32, WDC_READ, "GLBL_ERR_MASK", ""},
    { (DWORD)-1, 0x250, WDC_SIZE_32, WDC_READ, "GLBL_DSC_CFG", ""},
    { (DWORD)-1, 0x254, WDC_SIZE_32, WDC_READ, "GLBL_DSC_ERR_STS", ""},
    { (DWORD)-1, 0x258, WDC_SIZE_32, WDC_READ, "GLBL_DSC_ERR_MSK", ""},
    { (DWORD)-1, 0x25C, WDC_SIZE_32, WDC_READ, "GLBL_DSC_ERR_LOG_0", ""},
    { (DWORD)-1, 0x260, WDC_SIZE_32, WDC_READ, "GLBL_DSC_ERR_LOG_1", ""},
    { (DWORD)-1, 0x264, WDC_SIZE_32, WDC_READ, "GLBL_TRQ_ERR_STS", ""},
    { (DWORD)-1, 0x268, WDC_SIZE_32, WDC_READ, "GLBL_TRQ_ERR_MSK", ""},
    { (DWORD)-1, 0x26C, WDC_SIZE_32, WDC_READ, "GLBL_TRQ_ERR_LOG", ""},
    { (DWORD)-1, 0x270, WDC_SIZE_32, WDC_READ, "GLBL_DSC_DBG_DAT_0", ""},
    { (DWORD)-1, 0x274, WDC_SIZE_32, WDC_READ, "GLBL_DSC_DBG_DAT_1", ""},
    { (DWORD)-1, 0x278, WDC_SIZE_32, WDC_READ, "GLBL_DSC_DBG_CTL", ""},

    { (DWORD)-1, 0x400, WDC_SIZE_32, WDC_READ, "TRQ_SEL_FMAP_0", ""},
    { (DWORD)-1, 0x404, WDC_SIZE_32, WDC_READ, "TRQ_SEL_FMAP_1", ""},
    { (DWORD)-1, 0x408, WDC_SIZE_32, WDC_READ, "TRQ_SEL_FMAP_2", ""},
    { (DWORD)-1, 0x40C, WDC_SIZE_32, WDC_READ, "TRQ_SEL_FMAP_3", ""},

    { (DWORD)-1, 0x804, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_0", ""},
    { (DWORD)-1, 0x808, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_1", ""},
    { (DWORD)-1, 0x80C, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_2", ""},
    { (DWORD)-1, 0x810, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_3", ""},
    { (DWORD)-1, 0x814, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_4", ""},
    { (DWORD)-1, 0x818, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_5", ""},
    { (DWORD)-1, 0x81C, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_6", ""},
    { (DWORD)-1, 0x820, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_DATA_7", ""},

    { (DWORD)-1, 0x824, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_0", ""},
    { (DWORD)-1, 0x828, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_1", ""},
    { (DWORD)-1, 0x82c, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_2", ""},
    { (DWORD)-1, 0x830, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_3", ""},
    { (DWORD)-1, 0x834, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_4", ""},
    { (DWORD)-1, 0x838, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_5", ""},
    { (DWORD)-1, 0x83C, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_6", ""},
    { (DWORD)-1, 0x840, WDC_SIZE_32, WDC_READ_WRITE, "IND_CTXT_MASK_7", ""},

    { (DWORD)-1, 0x844, WDC_SIZE_32, WDC_READ, "IND_CTXT_CMD", ""},

    { (DWORD)-1, 0xA00, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_0", ""},
    { (DWORD)-1, 0xA04, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_1", ""},
    { (DWORD)-1, 0xA08, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_2", ""},
    { (DWORD)-1, 0xA0C, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_3", ""},
    { (DWORD)-1, 0xA10, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_4", ""},
    { (DWORD)-1, 0xA14, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_5", ""},
    { (DWORD)-1, 0xA18, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_6", ""},
    { (DWORD)-1, 0xA1C, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_7", ""},
    { (DWORD)-1, 0xA20, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_8", ""},
    { (DWORD)-1, 0xA24, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_9", ""},
    { (DWORD)-1, 0xA28, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_10", ""},
    { (DWORD)-1, 0xA2C, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_11", ""},
    { (DWORD)-1, 0xA30, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_12", ""},
    { (DWORD)-1, 0xA34, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_13", ""},
    { (DWORD)-1, 0xA38, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_14", ""},
    { (DWORD)-1, 0xA3C, WDC_SIZE_32, WDC_READ, "C2H_TIMER_CNT_15", ""},

    { (DWORD)-1, 0xA40, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_0", ""},
    { (DWORD)-1, 0xA44, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_1", ""},
    { (DWORD)-1, 0xA48, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_2", ""},
    { (DWORD)-1, 0xA4C, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_3", ""},
    { (DWORD)-1, 0xA50, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_4", ""},
    { (DWORD)-1, 0xA54, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_5", ""},
    { (DWORD)-1, 0xA58, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_6", ""},
    { (DWORD)-1, 0xA5C, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_7", ""},
    { (DWORD)-1, 0xA60, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_8", ""},
    { (DWORD)-1, 0xA64, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_9", ""},
    { (DWORD)-1, 0xA68, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_10", ""},
    { (DWORD)-1, 0xA6C, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_11", ""},
    { (DWORD)-1, 0xA70, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_12", ""},
    { (DWORD)-1, 0xA74, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_13", ""},
    { (DWORD)-1, 0xA78, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_14", ""},
    { (DWORD)-1, 0xA7C, WDC_SIZE_32, WDC_READ, "C2H_CNT_THRESH_15", ""},

    { (DWORD)-1, 0xA80, WDC_SIZE_32, WDC_READ, "C2H_QID2VEC_MAP_QID", ""},
    { (DWORD)-1, 0xA84, WDC_SIZE_32, WDC_READ, "C2H_QID2VEC_MAP", ""},
    { (DWORD)-1, 0xA88, WDC_SIZE_32, WDC_READ, "C2H_STAT_S_AXIS_C2H_ACCEPTED",
        ""},
    { (DWORD)-1, 0xA8C, WDC_SIZE_32, WDC_READ, "C2H_STAT_S_AXIS_WRB_ACCEPTED",
        ""},
    { (DWORD)-1, 0xA90, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_RSP_PKT_ACCEPTED",
        ""},
    { (DWORD)-1, 0xA94, WDC_SIZE_32, WDC_READ, "C2H_STAT_AXIS_PKG_CMP", ""},
    { (DWORD)-1, 0xA98, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_RSP_ACCEPTED",
        ""},
    { (DWORD)-1, 0xA9C, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_RSP_CMP", ""},
    { (DWORD)-1, 0xAA0, WDC_SIZE_32, WDC_READ, "C2H_STAT_WRQ_OUT", ""},
    { (DWORD)-1, 0xAA4, WDC_SIZE_32, WDC_READ, "C2H_STAT_WPL_REN_ACCEPTED", ""},
    { (DWORD)-1, 0xAA8, WDC_SIZE_32, WDC_READ, "C2H_STAT_TOTAL_WRQ_LEN", ""},
    { (DWORD)-1, 0xAAC, WDC_SIZE_32, WDC_READ, "C2H_STAT_TOTAL_WPL_LEN", ""},

    { (DWORD)-1, 0xAB0, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_0", ""},
    { (DWORD)-1, 0xAB4, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_1", ""},
    { (DWORD)-1, 0xAB8, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_2", ""},
    { (DWORD)-1, 0xABC, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_3", ""},
    { (DWORD)-1, 0xAC0, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_4", ""},
    { (DWORD)-1, 0xAC4, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_5", ""},
    { (DWORD)-1, 0xAC8, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_6", ""},
    { (DWORD)-1, 0xACC, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_7", ""},
    { (DWORD)-1, 0xAD0, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_8", ""},
    { (DWORD)-1, 0xAD4, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_9", ""},
    { (DWORD)-1, 0xAD8, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_10", ""},
    { (DWORD)-1, 0xADC, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_11", ""},
    { (DWORD)-1, 0xAE0, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_12", ""},
    { (DWORD)-1, 0xAE4, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_13", ""},
    { (DWORD)-1, 0xAE8, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_14", ""},
    { (DWORD)-1, 0xAEC, WDC_SIZE_32, WDC_READ, "C2H_BUF_SZ_15", ""},

    { (DWORD)-1, 0xAF0, WDC_SIZE_32, WDC_READ, "C2H_ERR_STAT", ""},
    { (DWORD)-1, 0xAF4, WDC_SIZE_32, WDC_READ, "C2H_ERR_MASK", ""},
    { (DWORD)-1, 0xAF8, WDC_SIZE_32, WDC_READ, "C2H_FATAL_ERR_STAT", ""},
    { (DWORD)-1, 0xAFC, WDC_SIZE_32, WDC_READ, "C2H_FATAL_ERR_MASK", ""},
    { (DWORD)-1, 0xB00, WDC_SIZE_32, WDC_READ, "C2H_FATAL_ERR_ENABLE", ""},
    { (DWORD)-1, 0xB04, WDC_SIZE_32, WDC_READ, "C2H_ERR_INT", ""},
    { (DWORD)-1, 0xB08, WDC_SIZE_32, WDC_READ, "C2H_PFCH_CFG", ""},
    { (DWORD)-1, 0xB0C, WDC_SIZE_32, WDC_READ, "C2H_INT_TIMER_TICK", ""},
    { (DWORD)-1, 0xB10, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_RSP_DROP_ACCEPTED",
        ""},
    { (DWORD)-1, 0xB14, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_RSP_ERR_ACCEPTED",
        ""},
    { (DWORD)-1, 0xB18, WDC_SIZE_32, WDC_READ, "C2H_STAT_DESC_REQ", ""},
    { (DWORD)-1, 0xB1C, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_0", ""},
    { (DWORD)-1, 0xB20, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_1", ""},
    { (DWORD)-1, 0xB24, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_2", ""},
    { (DWORD)-1, 0xB28, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_3", ""},
    { (DWORD)-1, 0xB2C, WDC_SIZE_32, WDC_READ, "C2H_INTR_MSIX", ""},
    { (DWORD)-1, 0xB30, WDC_SIZE_32, WDC_READ, "C2H_FIRST_ERR_QID", ""},
    { (DWORD)-1, 0xB34, WDC_SIZE_32, WDC_READ, "STAT_NUM_WRB_IN", ""},
    { (DWORD)-1, 0xB38, WDC_SIZE_32, WDC_READ, "STAT_NUM_WRB_OUT", ""},
    { (DWORD)-1, 0xB3C, WDC_SIZE_32, WDC_READ, "STAT_NUM_WRB_DRP", ""},
    { (DWORD)-1, 0xB40, WDC_SIZE_32, WDC_READ, "STAT_NUM_STAT_DESC_OUT", ""},
    { (DWORD)-1, 0xB44, WDC_SIZE_32, WDC_READ, "STAT_NUM_DSC_CRDT_SENT", ""},
    { (DWORD)-1, 0xB48, WDC_SIZE_32, WDC_READ, "STAT_NUM_FCH_DSC_RCVD", ""},
    { (DWORD)-1, 0xB4C, WDC_SIZE_32, WDC_READ, "STAT_NUM_BYP_DSC_RCVD", ""},
    { (DWORD)-1, 0xB50, WDC_SIZE_32, WDC_READ, "C2H_WRB_COAL_CFG", ""},
    { (DWORD)-1, 0xB54, WDC_SIZE_32, WDC_READ, "C2H_INTR_H2C_REQ", ""},
    { (DWORD)-1, 0xB58, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_MM_REQ", ""},
    { (DWORD)-1, 0xB5C, WDC_SIZE_32, WDC_READ, "C2H_INTR_ERR_INT_REQ", ""},
    { (DWORD)-1, 0xB60, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_ST_REQ", ""},
    { (DWORD)-1, 0xB64, WDC_SIZE_32, WDC_READ, "C2H_INTR_H2C_ERR_MM_MSIX_ACK",
        ""},
    { (DWORD)-1, 0xB68, WDC_SIZE_32, WDC_READ, "C2H_INTR_H2C_ERR_MM_MSIX_FAIL",
        ""},
    { (DWORD)-1, 0xB6C, WDC_SIZE_32, WDC_READ, "C2H_INTR_H2C_ERR_MM_NO_MSIX",
        ""},
    { (DWORD)-1, 0xB70, WDC_SIZE_32, WDC_READ, "C2H_INTR_H2C_ERR_MM_CTXT_INVAL",
        ""},
    { (DWORD)-1, 0xB74, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_ST_MSIX_ACK", ""},
    { (DWORD)-1, 0xB78, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_ST_MSIX_FAIL", ""},
    { (DWORD)-1, 0xB7C, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_ST_NO_MSIX", ""},
    { (DWORD)-1, 0xB80, WDC_SIZE_32, WDC_READ, "C2H_INTR_C2H_ST_CTXT_INVAL", ""},
    { (DWORD)-1, 0xB84, WDC_SIZE_32, WDC_READ, "C2H_STAT_WR_CMP", ""},
    { (DWORD)-1, 0xB88, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_4", ""},
    { (DWORD)-1, 0xB8C, WDC_SIZE_32, WDC_READ, "C2H_STAT_DEBUG_DMA_ENG_5", ""},
    { (DWORD)-1, 0xB90, WDC_SIZE_32, WDC_READ, "C2H_DBG_PFCH_QID", ""},
    { (DWORD)-1, 0xB94, WDC_SIZE_32, WDC_READ, "C2H_DBG_PFCH", ""},
    { (DWORD)-1, 0xB98, WDC_SIZE_32, WDC_READ, "C2H_INT_DEBUG", ""},
    { (DWORD)-1, 0xB9C, WDC_SIZE_32, WDC_READ, "C2H_STAT_IMM_ACCEPTED", ""},
    { (DWORD)-1, 0xBA0, WDC_SIZE_32, WDC_READ, "C2H_STAT_MARKER_ACCEPTED", ""},
    { (DWORD)-1, 0xBA4, WDC_SIZE_32, WDC_READ, "C2H_STAT_DISABLE_CMP_ACCEPTED",
        ""},
    { (DWORD)-1, 0xBA8, WDC_SIZE_32, WDC_READ, "C2H_C2H_PAYLOAD_FIFO_CRDT_CNT",
        ""},

    { (DWORD)-1, 0xE00, WDC_SIZE_32, WDC_READ, "H2C_ERR_STAT", ""},
    { (DWORD)-1, 0xE04, WDC_SIZE_32, WDC_READ, "H2C_ERR_MASK", ""},
    { (DWORD)-1, 0xE08, WDC_SIZE_32, WDC_READ, "H2C_ERR_FIRST_QID", ""},
    { (DWORD)-1, 0xE0C, WDC_SIZE_32, WDC_READ, "H2C_DBG_REG_0", ""},
    { (DWORD)-1, 0xE10, WDC_SIZE_32, WDC_READ, "H2C_DBG_REG_1", ""},
    { (DWORD)-1, 0xE14, WDC_SIZE_32, WDC_READ, "H2C_DBG_REG_2", ""},
    { (DWORD)-1, 0xE18, WDC_SIZE_32, WDC_READ, "H2C_DBG_REG_3", ""},
    { (DWORD)-1, 0xE1C, WDC_SIZE_32, WDC_READ, "H2C_DBG_REG_4", ""},

    { (DWORD)-1, 0x1004, WDC_SIZE_32, WDC_READ, "C2H_MM0_CONTROL", ""},
    { (DWORD)-1, 0x1040, WDC_SIZE_32, WDC_READ, "C2H_MM0_STATUS", ""},
    { (DWORD)-1, 0x1048, WDC_SIZE_32, WDC_READ, "C2H_MM0_CMPL_DSC_CNT", ""},
    { (DWORD)-1, 0x1054, WDC_SIZE_32, WDC_READ, "C2H_MM0_ERR_CODE_EN_MASK", ""},
    { (DWORD)-1, 0x1058, WDC_SIZE_32, WDC_READ, "C2H_MM0_ERR_CODE", ""},
    { (DWORD)-1, 0x105C, WDC_SIZE_32, WDC_READ, "C2H_MM0_ERR_INFO", ""},
    { (DWORD)-1, 0x10C0, WDC_SIZE_32, WDC_READ, "C2H_MM0_PERF_MON_CTRL", ""},
    { (DWORD)-1, 0x10C4, WDC_SIZE_32, WDC_READ, "C2H_MM0_PERF_MON_CY_CNT_0",
        ""},
    { (DWORD)-1, 0x10C8, WDC_SIZE_32, WDC_READ, "C2H_MM0_PERF_MON_CY_CNT_1",
        ""},
    { (DWORD)-1, 0x10CC, WDC_SIZE_32, WDC_READ, "C2H_MM0_PERF_MON_DATA_CNT_0",
        ""},
    { (DWORD)-1, 0x10D0, WDC_SIZE_32, WDC_READ, "C2H_MM0_PERF_MON_DATA_CNT_1",
        ""},
    { (DWORD)-1, 0x10E8, WDC_SIZE_32, WDC_READ, "C2H_MM_DBG_INFO_0", ""},

    { (DWORD)-1, 0x1204, WDC_SIZE_32, WDC_READ, "H2C_MM0_CONTROL", ""},
    { (DWORD)-1, 0x1240, WDC_SIZE_32, WDC_READ, "H2C_MM0_STATUS", ""},
    { (DWORD)-1, 0x1248, WDC_SIZE_32, WDC_READ, "H2C_MM0_CMPL_DSC_CNT", ""},
    { (DWORD)-1, 0x1254, WDC_SIZE_32, WDC_READ, "H2C_MM0_ERR_CODE_EN_MASK", ""},
    { (DWORD)-1, 0x1258, WDC_SIZE_32, WDC_READ, "H2C_MM0_ERR_CODE", ""},
    { (DWORD)-1, 0x125C, WDC_SIZE_32, WDC_READ, "H2C_MM0_ERR_INFO", ""},
    { (DWORD)-1, 0x12C0, WDC_SIZE_32, WDC_READ, "H2C_MM0_PERF_MON_CTRL", ""},
    { (DWORD)-1, 0x12C4, WDC_SIZE_32, WDC_READ, "H2C_MM0_PERF_MON_CY_CNT_0",
        ""},
    { (DWORD)-1, 0x12C8, WDC_SIZE_32, WDC_READ, "H2C_MM0_PERF_MON_CY_CNT_1",
        ""},
    { (DWORD)-1, 0x12CC, WDC_SIZE_32, WDC_READ, "H2C_MM0_PERF_MON_DATA_CNT_0",
        ""},
    { (DWORD)-1, 0x12D0, WDC_SIZE_32, WDC_READ, "H2C_MM0_PERF_MON_DATA_CNT_1",
        ""},
    { (DWORD)-1, 0x12E8, WDC_SIZE_32, WDC_READ, "H2C_MM0_DBG_INFO", "" }
};
#define QDMA_CONFIG_REGS_NUM (sizeof(gQDMA_ConfigRegs)/sizeof(WDC_REG))

static WDC_REG gQDMA_ConfigRegsArr[QDMA_NUM_PF_FUNC][QDMA_CONFIG_REGS_NUM] =
{ 0 };

typedef struct {
    WDC_DEVICE_HANDLE *hDevs; /* Device handles array */
    WDC_DEVICE_HANDLE *pCurrentPfDev; /* current active handle */
    WDC_REG *pCurrentConfigRegsArr; /* current config regs array */
    DWORD dwCurrentPfIdx;
} MENU_CTX_QDMA;


/*************************************************************
  Static functions prototypes
 *************************************************************/
 /* -----------------------------------------------
     Main diagnostics menu
    ----------------------------------------------- */
static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *hDevs);

/* -----------------------------------------------
    Devices find, open and close
   ----------------------------------------------- */
static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx);

static void MenuChangePfInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx);

static void MenuDeviceCloseInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx);

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/* -----------------------------------------------
    Read/write the config block registers
   ----------------------------------------------- */
static void MenuRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx);

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */
static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);
static void DmaFreeRequestsBuffer();

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev);

/*************************************************************
  Functions implementation
 *************************************************************/
static DWORD QDMA_Init(WDC_DEVICE_HANDLE *hDevs)
{
    /* Initialize the QDMA library */
    DWORD dwStatus, i, j, dwConfigBarNum, pfIdx;

    dwStatus = QDMA_LibInit(NULL);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        QDMA_ERR("qdma_diag: Failed to initialize the QDMA library: %s",
            QDMA_GetLastErr());
        return dwStatus;
    }

    /* Find and open a QDMA device (by default ID) */
    for (pfIdx = 0, i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        hDevs[pfIdx] = QDMA_DeviceOpen(QDMA_DEFAULT_VENDOR_ID,
            QDMA_DefaultDeviceIds[i]);
        if (hDevs[pfIdx])
        {
            memcpy(&gQDMA_ConfigRegsArr[pfIdx], &gQDMA_ConfigRegs,
                sizeof(gQDMA_ConfigRegs));

            /* Get the configuration BAR number */
            dwConfigBarNum = QDMA_ConfigBarNumGet(hDevs[i]);
            for (j = 0; j < QDMA_CONFIG_REGS_NUM; j++)
            {
                gQDMA_ConfigRegsArr[pfIdx][j].dwAddrSpace = dwConfigBarNum;
            }
            pfIdx++;
        }
    }

    return WD_STATUS_SUCCESS;
}

int main(void)
{
    static WDC_DEVICE_HANDLE hDevs[QDMA_NUM_PF_FUNC] = { 0 };
    DIAG_MENU_OPTION *pMenuRoot;

    printf("\n");
    printf("QDMA diagnostic utility.\n");
    printf("Application accesses hardware using " WD_PROD_NAME ".\n");

    QDMA_Init(hDevs);

    pMenuRoot = MenuMainInit(hDevs);

    return DIAG_MenuRun(pMenuRoot);
}

/* -----------------------------------------------
    Main diagnostics menu
   ----------------------------------------------- */

static BOOL MenuIsCurrentPfIsNull(DIAG_MENU_OPTION *pMenu)
{
    return *(((MENU_CTX_QDMA *)pMenu->pCbCtx)->pCurrentPfDev) == NULL;
}

/* Print all open devices */
DWORD PrintOpenDevices(PVOID pCbCtx)
{
    DWORD i;
    MENU_CTX_QDMA *qdmaCtx = (MENU_CTX_QDMA *)pCbCtx;

    printf("Open devices:\n");

    for (i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        if (qdmaCtx->hDevs[i])
        {
            printf("%d. Vendor id [0x%x], Device id [0x%x] %s\n", i,
                QDMA_GetDeviceId(qdmaCtx->hDevs[i]),
                QDMA_GetVendorId(qdmaCtx->hDevs[i]),
                (i == qdmaCtx->dwCurrentPfIdx) ? "- Active device" : "");
        }
    }
    printf("\n");
    printf("currently active device index: %d\n\n", qdmaCtx->dwCurrentPfIdx);

    return WD_STATUS_SUCCESS;
}

/* Get physical function index */
static DWORD MenuDmaGetPfIdx(PVOID pCbCtx)
{
    MENU_CTX_QDMA *pQdmaCtx = (MENU_CTX_QDMA *)pCbCtx;
    DWORD i, dwAvailablePfs = 0, dwLastAvailablePfIdx = 0;

    printf("\nAvailable physical functions:\n");
    for (i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        if (pQdmaCtx->hDevs[i])
        {
            printf("%d. Vendor id [0x%x], "
                "Device id [0x%x]\n", i, QDMA_GetDeviceId(pQdmaCtx->hDevs[i]),
                QDMA_GetVendorId(pQdmaCtx->hDevs[i]));
            dwLastAvailablePfIdx = i;
            dwAvailablePfs++;
        }
    }
    if (dwAvailablePfs == 0)
    {
        printf("There is no open QDMA device\n");
        return WD_DEVICE_NOT_FOUND;
    }
    if (dwAvailablePfs == 1)
    {
        pQdmaCtx->dwCurrentPfIdx = dwLastAvailablePfIdx;
    }
    else if (dwAvailablePfs > 1)
    {
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&(pQdmaCtx->dwCurrentPfIdx),
            "\nEnter physical function index", FALSE, 0, QDMA_NUM_PF_FUNC))
        {
            return WD_INVALID_PARAMETER;
        }
    }
    if (!pQdmaCtx->hDevs[pQdmaCtx->dwCurrentPfIdx])
    {
        printf("Invalid physical function index\n");
        return WD_DEVICE_NOT_FOUND;
    }

    printf("Physical function %d selected\n", pQdmaCtx->dwCurrentPfIdx);

    /* Update references in menu ctx */
    *(pQdmaCtx->pCurrentPfDev) = pQdmaCtx->hDevs[pQdmaCtx->dwCurrentPfIdx];
    pQdmaCtx->pCurrentConfigRegsArr = gQDMA_ConfigRegsArr
        [pQdmaCtx->dwCurrentPfIdx];

    return WD_STATUS_SUCCESS;
}

static DWORD DevicesClose(PVOID pCbCtx)
{
    DWORD i;
    MENU_CTX_QDMA *pQdmaCtx = (MENU_CTX_QDMA *)pCbCtx;

    for (i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        if (pQdmaCtx->hDevs[i] && !QDMA_DeviceClose(pQdmaCtx->hDevs[i]))
            QDMA_ERR("qdma_diag: Failed to close QDMA device #%d: %s",
                i, QDMA_GetLastErr());
    }

    pQdmaCtx->hDevs[pQdmaCtx->dwCurrentPfIdx] = NULL;

    return WD_STATUS_SUCCESS;
}

static DWORD MenuMainExitCb(PVOID pCbCtx)
{
    DWORD dwStatus;
    WDC_DEVICE_HANDLE hDevs = (WDC_DEVICE_HANDLE *)pCbCtx;

    /* Perform necessary cleanup before exiting the program: */
    /* Close the device handle */
    DevicesClose(hDevs);

    DmaFreeRequestsBuffer();

    /* Uninitialize libraries */
    dwStatus = QDMA_LibUninit();
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        QDMA_ERR("qdma_diag: Failed to uninitialize the QDMA library: %s",
            QDMA_GetLastErr());
    }

    return dwStatus;

}

static DIAG_MENU_OPTION *MenuMainInit(WDC_DEVICE_HANDLE *hDevs)
{
    static MENU_CTX_QDMA qdmaCtx = { 0 };
    static DIAG_MENU_OPTION menuRoot = { 0 };

    qdmaCtx.hDevs = hDevs;
    qdmaCtx.pCurrentPfDev = hDevs;
    qdmaCtx.pCurrentConfigRegsArr = gQDMA_ConfigRegsArr[0];

    strcpy(menuRoot.cTitleName, "QDMA main menu");
    menuRoot.cbEntry = PrintOpenDevices;
    menuRoot.cbExit = MenuMainExitCb;
    menuRoot.pCbCtx = &qdmaCtx;

    MenuCommonScanBusInit(&menuRoot);
    MenuDeviceOpenInit(&menuRoot, &qdmaCtx);
    MenuChangePfInit(&menuRoot, &qdmaCtx);
    MenuReadWriteAddrInit(&menuRoot, qdmaCtx.pCurrentPfDev);
    MenuCfgInit(&menuRoot, qdmaCtx.pCurrentPfDev);
    MenuRwRegsInit(&menuRoot, &qdmaCtx);
    MenuDmaInit(&menuRoot, qdmaCtx.pCurrentPfDev);
    MenuEventsInit(&menuRoot, qdmaCtx.pCurrentPfDev);
    MenuDeviceCloseInit(&menuRoot, &qdmaCtx);

    return &menuRoot;
}
/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */

/* Find and open multiple physical functions */
static DWORD DeviceFindAndOpenMultiplePf(PVOID pCbCtx)
{
    MENU_CTX_QDMA *pQdmaCtx = (MENU_CTX_QDMA *)pCbCtx;
    DWORD dwEmptyId = ULONG_MAX;
    DWORD i;

    for (i = 0; i < QDMA_NUM_PF_FUNC; i++)
    {
        if (!pQdmaCtx->hDevs[i])
        {
            dwEmptyId = i;
            break;
        }
    }
    if (dwEmptyId == ULONG_MAX)
        printf("No more than %d devices can be opened", QDMA_NUM_PF_FUNC);
    else
        pQdmaCtx->hDevs[pQdmaCtx->dwCurrentPfIdx] = QDMA_DeviceOpen(0, 0);

    /* Update references in menu ctx */
    *(pQdmaCtx->pCurrentPfDev) = pQdmaCtx->hDevs[pQdmaCtx->dwCurrentPfIdx];
    pQdmaCtx->pCurrentConfigRegsArr =
        gQDMA_ConfigRegsArr[pQdmaCtx->dwCurrentPfIdx];

    return WD_STATUS_SUCCESS;
}

static void MenuDeviceOpenInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx)
{
    static DIAG_MENU_OPTION deviceOpenMenu = { 0 };

    strcpy(deviceOpenMenu.cOptionName, "Find and open a QDMA device");
    deviceOpenMenu.cbEntry = DeviceFindAndOpenMultiplePf;

    DIAG_MenuSetCtxAndParentForMenus(&deviceOpenMenu, 1, pQdmaCtx,
        pParentMenu);
}

static void MenuChangePfInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx)
{
    static DIAG_MENU_OPTION changePfMenu = { 0 };

    strcpy(changePfMenu.cOptionName, "Change physical function");
    changePfMenu.cbEntry = MenuDmaGetPfIdx;

    DIAG_MenuSetCtxAndParentForMenus(&changePfMenu, 1, pQdmaCtx,
        pParentMenu);
}

static void MenuDeviceCloseInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx)
{
    static DIAG_MENU_OPTION deviceCloseMenu = { 0 };

    strcpy(deviceCloseMenu.cOptionName, "Close device");
    deviceCloseMenu.cbEntry = DevicesClose;
    deviceCloseMenu.cbIsHidden = MenuIsCurrentPfIsNull;

    DIAG_MenuSetCtxAndParentForMenus(&deviceCloseMenu, 1, pQdmaCtx,
        pParentMenu);
}

/* -----------------------------------------------
    Read/write memory and I/O addresses
   ----------------------------------------------- */
static void MenuReadWriteAddrInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_READ_WRITE_ADDR rwAddrMenusCtx;

    rwAddrMenusCtx.phDev = phDev;
    rwAddrMenusCtx.fBlock = FALSE;
    rwAddrMenusCtx.mode = WDC_MODE_32;
    rwAddrMenusCtx.dwAddrSpace = ACTIVE_ADDR_SPACE_NEEDS_INIT;

    MenuCommonRwAddrInit(pParentMenu, &rwAddrMenusCtx);
}


/* -----------------------------------------------
    Read/write the configuration space
   ----------------------------------------------- */
static void MenuCfgInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static MENU_CTX_CFG cfgCtx;

    BZERO(cfgCtx);
    cfgCtx.phDev = phDev;
    cfgCtx.pCfgRegs = gQDMA_CfgRegs;
    cfgCtx.dwCfgRegsNum = QDMA_CFG_REGS_NUM;

    MenuCommonCfgInit(pParentMenu, &cfgCtx);
}

/* -----------------------------------------------
    Read/write the run-time registers
   ----------------------------------------------- */
static void MenuRwRegsInit(DIAG_MENU_OPTION *pParentMenu,
    MENU_CTX_QDMA *pQdmaCtx)
{
    static MENU_CTX_RW_REGS regsMenusCtx;

    BZERO(regsMenusCtx);

    regsMenusCtx.phDev = pQdmaCtx->pCurrentPfDev;
    regsMenusCtx.pRegsArr = pQdmaCtx->pCurrentConfigRegsArr;
    regsMenusCtx.dwRegsNum = QDMA_CONFIG_REGS_NUM;
    regsMenusCtx.fIsConfig = TRUE;
    strcpy(regsMenusCtx.sModuleName, "QDMA");

    MenuCommonRwRegsInit(pParentMenu, &regsMenusCtx);
}

/* -----------------------------------------------
    Direct Memory Access (DMA)
   ---------------------------------------------- */
static BOOL MenuDmaTransferGetInput(BOOL fToDevice, UINT32 *pu32Pattern,
    DWORD *pdwNumPackets, UINT64 *pu64FPGAOffset, BOOL *pfBlocking)
{
    DWORD option;

    if (fToDevice)
    {
        /* Get DMA buffer pattern for host to device transfer */
        if (DIAG_INPUT_SUCCESS != DIAG_InputUINT32(pu32Pattern,
            "\nEnter DMA data pattern as 32 bit packet", TRUE, 0, 0))
        {
            return FALSE;
        }
    }

    /* Get data pattern */
    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwNumPackets,
        "\nEnter number of packets to transfer (32 bit packets)", FALSE, 0, 0))
    {
        return FALSE;
    }

    if (*pdwNumPackets == 0)
    {
        QDMA_ERR("Illegal number of packets\n");
        return FALSE;
    }

    if (DIAG_INPUT_SUCCESS != DIAG_InputUINT64(pu64FPGAOffset,
        "\nEnter FPGA offset for transfer", TRUE, 0, 0))
    {
        return FALSE;
    }

    printf("\nSelect blocking method:");
    printf("\n---------------------\n");
    printf("1. Non-blocking\n");
    printf("2. Blocking\n");
    printf("%d. Cancel\n", DIAG_EXIT_MENU);
    if ((DIAG_INPUT_SUCCESS != DIAG_GetMenuOption(&option, 2)) ||
        (DIAG_EXIT_MENU == option))
    {
        return FALSE;
    }
    *pfBlocking = (2 == option) ? TRUE : FALSE;

    printf("\n");

    return TRUE;
}

enum {
    MENU_DMA_ADD_QUEUE = 1,
    MENU_DMA_REMOVE_QUEUE,
    MENU_DMA_START_QUEUE,
    MENU_DMA_STOP_QUEUE,
    MENU_DMA_READ,
    MENU_DMA_WRITE,
    MENU_DMA_STATUS_QUEUE,
    MENU_REQUESTS,
    MENU_DMA_EXIT = DIAG_EXIT_MENU,
};

static DWORD GetQueuesState(WDC_DEVICE_HANDLE hDev, DWORD dwCurrentQueueId,
    QUEUE_STATE *currentState, QUEUE_STATE *nextState)
{
    DWORD dwStatus;

    *currentState = QUEUE_STATE_QUEUE_MAX;
    *nextState = QUEUE_STATE_QUEUE_MAX;

    dwStatus = QDMA_GetQueueState(hDev, dwCurrentQueueId, currentState);
    if (WD_STATUS_SUCCESS != dwStatus)
        goto Error;

    if (dwCurrentQueueId + 1 < QDMA_MAX_QUEUES_PER_PF)
    {
        dwStatus = QDMA_GetQueueState(hDev, dwCurrentQueueId + 1, nextState);
        if (WD_STATUS_SUCCESS != dwStatus)
            goto Error;
    }

Error:
    return dwStatus;
}

static DWORD PrintQueuesState(WDC_DEVICE_HANDLE hDev, BOOL fFilterIsNeeded,
    QUEUE_STATE filterState, DWORD *pdwFilteredQueuesCounter,
    DWORD *pdwLastFilteredQueueId)
{
    BOOL fIsInRange = FALSE;
    DWORD dwStatus, dwQueueId, dwQueueIdFrom = 0, dwQueueIdTo = 0;
    QUEUE_STATE currentState = QUEUE_STATE_QUEUE_MAX;
    QUEUE_STATE nextState = QUEUE_STATE_QUEUE_MAX;

    if (pdwFilteredQueuesCounter)
        *pdwFilteredQueuesCounter = 0;

    for (dwQueueId = 0; dwQueueId < QDMA_MAX_QUEUES_PER_PF;)
    {
        dwQueueIdFrom = dwQueueId;

        do
        {
            dwStatus = GetQueuesState(hDev, dwQueueId, &currentState,
                &nextState);
            if (WD_STATUS_SUCCESS != dwStatus)
            {
                QDMA_ERR("QDMA_GetQueueState: Failed to get queue state: %s",
                    QDMA_GetLastErr());
                goto Exit;
            }
            if (currentState == nextState)
                fIsInRange = TRUE;

            dwQueueId++;
        } while (currentState == nextState &&
            (!fFilterIsNeeded || nextState == filterState));

        dwQueueIdTo = dwQueueId - 1;

        if (!fFilterIsNeeded || currentState == filterState)
        {
            if (fIsInRange)
            {
                printf("Queues %d-%d are %s\n", dwQueueIdFrom, dwQueueIdTo,
                    cDmaQueueStates[currentState]);
            }
            else
            {
                printf("Queue %d is %s\n", (dwQueueId - 1),
                    cDmaQueueStates[currentState]);
            }

            if (pdwFilteredQueuesCounter)
                *pdwFilteredQueuesCounter += (dwQueueIdTo - dwQueueIdFrom) + 1;
            if (pdwLastFilteredQueueId)
                *pdwLastFilteredQueueId = dwQueueIdTo;
        }

        fIsInRange = FALSE;
    }

    printf("\n");

Exit:
    return dwStatus;
}

static BOOL MenuDmaGetQueueId(WDC_DEVICE_HANDLE hDev, DWORD *pdwQueueId,
    QUEUE_STATE requiredState)
{
    QUEUE_STATE currentState = QUEUE_STATE_QUEUE_MAX;
    DWORD dwStatus, dwCounter, lastCompatibleQueueId;

    printf("%s queues:\n", cDmaQueueStates[requiredState]);

    dwStatus = PrintQueuesState(hDev, TRUE, requiredState, &dwCounter,
        &lastCompatibleQueueId);
    if (dwStatus != WD_STATUS_SUCCESS)
        return FALSE;

    if (dwCounter == 0)
    {
        printf("None of the queues are in the desired state\n\n");
        return FALSE;
    }
    else if (dwCounter == 1)
    {
        *pdwQueueId = lastCompatibleQueueId;
    }
    else
    {
        if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(pdwQueueId,
            "\nEnter queue ID relative to the physical device ID provided",
            FALSE, 0, QDMA_MAX_QUEUES_PER_PF))
        {
            return FALSE;
        }

        if (QDMA_GetQueueState(hDev, *pdwQueueId, &currentState)
            != WD_STATUS_SUCCESS)
        {
            return FALSE;
        }
        if (requiredState != currentState)
        {
            printf("The queue state doesn't support this type of operation"
                "\n\n");
            return FALSE;
        }
    }

    printf("Queue %d index selected\n\n", *pdwQueueId);

    return TRUE;
}

/* DMA requests management menu options */
enum {
    MENU_DMA_SHOW_REQUESTS_LIST = 1,
    MENU_DMA_SHOW_BUFFER_CONTENT,
    MENU_DMA_DELETE_REQUEST,
};

static void setNextAvailableDmaRequestIdx()
{
    DWORD i;

    for (i = 0; i < MAX_DMA_REQUESTS; i++)
    {
        if (gDmaRequests[i] == NULL)
        {
            gDmaRequestsIndex = i;
            break;
        }
    }
    if (i == MAX_DMA_REQUESTS)
        gDmaRequestsIndex = MAX_DMA_REQUESTS;
}

static DWORD showBufferContent(PVOID pCbCtx)
{
    DWORD dwDmaRequestIdx;
    DMA_REQUEST_CONTEXT *dmaRequestContext;

    UNUSED_VAR(pCbCtx);

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwDmaRequestIdx, "\nEnter DMA "
        "request index that you want to display its buffer ", FALSE, 0,
        MAX_DMA_REQUESTS))
    {
        return WD_INVALID_PARAMETER;
    }

    dmaRequestContext = gDmaRequests[dwDmaRequestIdx];

    if (dmaRequestContext &&
        dmaRequestContext->status == DMA_REQUEST_STATUS_FINISHED &&
        dmaRequestContext->pBuf)
    {
        DIAG_PrintHexBuffer(dmaRequestContext->pBuf,
            dmaRequestContext->dwBytes, TRUE);
    }

    return WD_STATUS_SUCCESS;
}

static DWORD deleteRequest(PVOID pCbCtx)
{
    DWORD dwDmaRequestId;
    DMA_REQUEST_CONTEXT *dmaRequestContext;

    UNUSED_VAR(pCbCtx);

    if (DIAG_INPUT_SUCCESS != DIAG_InputDWORD(&dwDmaRequestId,
        "\nEnter DMA request index to delete", FALSE, 1, MAX_DMA_REQUESTS + 1))
    {
        return WD_INVALID_PARAMETER;
    }
    dwDmaRequestId--;

    dmaRequestContext = gDmaRequests[dwDmaRequestId];

    if (dmaRequestContext)
    {
        if (dmaRequestContext->status == DMA_REQUEST_STATUS_FINISHED ||
            dmaRequestContext->status == DMA_REQUEST_STATUS_ERROR)
        {
            free(dmaRequestContext->pBuf);
            free(dmaRequestContext);
            gDmaRequests[dwDmaRequestId] = NULL;
            setNextAvailableDmaRequestIdx();
        }
    }
    else
    {
        printf("Invalid request index\n");
    }

    return WD_STATUS_SUCCESS;
}

static void DmaFreeRequestsBuffer()
{
    DWORD i;

    for (i = 0; i < MAX_DMA_REQUESTS; i++)
    {
        if (gDmaRequests[i])
        {
            if (gDmaRequests[i]->pBuf)
            {
                free(gDmaRequests[i]->pBuf);
                gDmaRequests[i]->pBuf = NULL;
            }

            free(gDmaRequests[i]);
        }
    }
}

static void printSingleRequest(DMA_REQUEST_CONTEXT *dmaRequestContext,
    DWORD dwIndex)
{
    const char *cDmaRequestsStatus[] = { "Unitialized", "Started", "Finished",
        "Error" };
    double time_elapsed;

    if (dmaRequestContext)
    {
        time_elapsed = time_diff(&dmaRequestContext->timeEnd,
            &dmaRequestContext->timeStart);
        printf("Request id %d\n\tStatus: %s\n\tDirection: %s\n\tSize: 0x%x\n",
            dwIndex + 1, cDmaRequestsStatus[dmaRequestContext->status],
            dmaRequestContext->fIsH2C ? "H2C" : "C2H",
            dmaRequestContext->dwBytes);

        if (dmaRequestContext->status == DMA_REQUEST_STATUS_FINISHED)
        {
            printf("\tTransferred %u bytes, elapsed time %f[ms], "
                "rate %f[MB/sec]\n\n", dmaRequestContext->dwBytes, time_elapsed,
                ((dmaRequestContext->dwBytes / (time_elapsed + 1)) * 1000) /
                (UINT64)(1024 * 1024));
        }
    }
}

static DWORD PrintAllRequests(PVOID pCbCtx)
{
    DWORD i;

    UNUSED_VAR(pCbCtx);

    printf("Dma requests:\n\n");

    for (i = 0; i < MAX_DMA_REQUESTS; i++)
    {
        printSingleRequest(gDmaRequests[i], i);
    }
    printf("\n");

    return WD_STATUS_SUCCESS;
}

static DWORD QueueOperation(WDC_DEVICE_HANDLE hDev, QUEUE_OPERATION operation)
{
    DWORD dwStatus, dwQueueId = 0;
    QUEUE_CONFIG conf = { 0 }; // used for add queue only
    QUEUE_STATE state = QDMA_GetRequiredStateByOperation(operation);

    if (!MenuDmaGetQueueId(hDev, &dwQueueId, state))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    switch (operation)
    {
    case QUEUE_OPERATION_ADD:
        conf.h2cRingSizeIdx = DEFAULT_H2C_RING_SZ_INDEX;
        conf.c2hRingSizeIdx = DEFAULT_C2H_RING_SZ_INDEX;
        dwStatus = QDMA_AddQueue(hDev, dwQueueId,
            &conf);
        break;
    case QUEUE_OPERATION_REMOVE:
        dwStatus = QDMA_RemoveQueue(hDev, dwQueueId);
        break;
    case QUEUE_OPERATION_START:
        dwStatus = QDMA_StartQueue(hDev, dwQueueId);
        break;
    case QUEUE_OPERATION_STOP:
        dwStatus = QDMA_StopQueue(hDev, dwQueueId);
        break;
    default:
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    if (dwStatus == WD_STATUS_SUCCESS)
    {
        printf("Queue %sed successfully\n", cQueueOperation[operation]);
    }
    else
    {
        printf("Failed to %s queue. Last Error:\n%s",
            cQueueOperation[operation], QDMA_GetLastErr());
    }

Exit:
    return dwStatus;
}

/* Read/write DMA request from QDMA handle */
DWORD ReadWrite(WDC_DEVICE_HANDLE hDev, BOOL fToDevice)
{
    DWORD dwStatus, dwNumPackets;
    DWORD dwQueueId = 0, i;
    UINT32 u32Pattern;
    UINT64 u64FPGAOffset;
    UINT32 *pu32Buf = NULL;
    DMA_REQUEST_CONTEXT *dmaRequestContext;
    BOOL fBlocking;
    QUEUE_STATE state = QUEUE_STATE_QUEUE_STARTED;

    if (gDmaRequestsIndex >= MAX_DMA_REQUESTS)
    {
        QDMA_ERR("DMA requests array is full\n");
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (!MenuDmaGetQueueId(hDev, &dwQueueId, state) ||
        !MenuDmaTransferGetInput(fToDevice, &u32Pattern, &dwNumPackets,
            &u64FPGAOffset, &fBlocking))
    {
        dwStatus = WD_INVALID_PARAMETER;
        goto Exit;
    }

    gDmaRequests[gDmaRequestsIndex] =
        (DMA_REQUEST_CONTEXT *)calloc(1, sizeof(DMA_REQUEST_CONTEXT));
    if (!gDmaRequests[gDmaRequestsIndex])
    {
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    dmaRequestContext = gDmaRequests[gDmaRequestsIndex];
    dmaRequestContext->hDev = hDev;
    dmaRequestContext->dwQueueId = dwQueueId;
    dmaRequestContext->fIsH2C = fToDevice;
    dmaRequestContext->dwBytes = dwNumPackets * sizeof(UINT32);
    dmaRequestContext->pBuf = calloc(dmaRequestContext->dwBytes, sizeof(BYTE));
    if (!dmaRequestContext->pBuf)
    {
        QDMA_ERR("Failed allocating user memory for SG. size [%d]\n",
            dmaRequestContext->dwBytes);
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (fToDevice)
    {
        pu32Buf = (UINT32 *)dmaRequestContext->pBuf;
        for (i = 0; i < dmaRequestContext->dwBytes / sizeof(UINT32); i++)
            pu32Buf[i] = u32Pattern;
    }

    dwStatus = QDMA_IoMmDma(dmaRequestContext);
    if (dwStatus == WD_STATUS_SUCCESS)
    {
        printf("%s request id [%d] enqueued successfully\n", fToDevice ?
            "Write" : "Read", gDmaRequestsIndex + 1);
    }
    else
    {
        printf("%s request id [%d] failed. Last Error:\n%s", fToDevice ?
            "Write" : "Read", gDmaRequestsIndex + 1, QDMA_GetLastErr());
    }


    if (dwStatus == WD_STATUS_SUCCESS && fBlocking)
    {
        /* Polling until DMA is finished or an error occurred */
        while (gDmaRequests[gDmaRequestsIndex]->status ==
            DMA_REQUEST_STATUS_STARTED);

        printSingleRequest(gDmaRequests[gDmaRequestsIndex], gDmaRequestsIndex);
    }

    setNextAvailableDmaRequestIdx();

Exit:
    return dwStatus;
}

static DWORD MenuDmaAddQueueOptionCb(PVOID pCbCtx)
{
    return QueueOperation(*((WDC_DEVICE_HANDLE *)pCbCtx), QUEUE_OPERATION_ADD);
}

static DWORD MenuDmaRemoveQueueOptionCb(PVOID pCbCtx)
{
    return QueueOperation(*((WDC_DEVICE_HANDLE *)pCbCtx),
        QUEUE_OPERATION_REMOVE);
}

static DWORD MenuDmaStartQueueOptionCb(PVOID pCbCtx)
{
    return QueueOperation(*((WDC_DEVICE_HANDLE *)pCbCtx),
        QUEUE_OPERATION_START);
}

static DWORD MenuDmaStopQueueOptionCb(PVOID pCbCtx)
{
    return QueueOperation(*((WDC_DEVICE_HANDLE *)pCbCtx),
        QUEUE_OPERATION_STOP);
}

static DWORD MenuDmaReadRequestOptionCb(PVOID pCbCtx)
{
    return ReadWrite(*((WDC_DEVICE_HANDLE *)pCbCtx),
        FALSE);
}

static DWORD MenuDmaWriteRequestOptionCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);

    return ReadWrite(*((WDC_DEVICE_HANDLE *)pCbCtx),
        TRUE);
}

static DWORD MenuDmaGetQueueStatusOptionCb(PVOID pCbCtx)
{
    UNUSED_VAR(pCbCtx);

    return PrintQueuesState(*((WDC_DEVICE_HANDLE *)pCbCtx), FALSE,
        QUEUE_STATE_QUEUE_AVAILABLE, NULL, NULL);
}

static void MenuDmaRequestsSubMenuInit(DIAG_MENU_OPTION *pParentMenu)
{
    static DIAG_MENU_OPTION showRequestListMenu = { 0 };
    static DIAG_MENU_OPTION showBufferContentMenu = { 0 };
    static DIAG_MENU_OPTION DeleteRequestMenu = { 0 };
    static DIAG_MENU_OPTION options[3] = { 0 };

    strcpy(showRequestListMenu.cOptionName, "Show requests list");
    showRequestListMenu.cbEntry = PrintAllRequests;

    strcpy(showBufferContentMenu.cOptionName, "Show buffer content");
    showBufferContentMenu.cbEntry = showBufferContent;

    strcpy(DeleteRequestMenu.cOptionName, "Delete request from list");
    DeleteRequestMenu.cbEntry = deleteRequest;

    options[0] = showRequestListMenu;
    options[1] = showBufferContentMenu;
    options[2] = DeleteRequestMenu;

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        NULL, pParentMenu);
}

static void MenuDmaOptionsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION addMmQueueMenu = { 0 };
    static DIAG_MENU_OPTION removeMmQueueMenu = { 0 };
    static DIAG_MENU_OPTION startQueueMenu = { 0 };
    static DIAG_MENU_OPTION stopQueueMenu = { 0 };
    static DIAG_MENU_OPTION newReadRequestMenu = { 0 };
    static DIAG_MENU_OPTION newWriteRequestMenu = { 0 };
    static DIAG_MENU_OPTION getQueueStatusMenu = { 0 };
    static DIAG_MENU_OPTION options[7] = { 0 };

    strcpy(addMmQueueMenu.cOptionName, "Add MM queue");
    addMmQueueMenu.cbEntry = MenuDmaAddQueueOptionCb;

    strcpy(removeMmQueueMenu.cOptionName, "Remove MM queue");
    removeMmQueueMenu.cbEntry = MenuDmaRemoveQueueOptionCb;

    strcpy(startQueueMenu.cOptionName, "Start queue");
    startQueueMenu.cbEntry = MenuDmaStartQueueOptionCb;

    strcpy(stopQueueMenu.cOptionName, "Stop queue");
    stopQueueMenu.cbEntry = MenuDmaStopQueueOptionCb;

    strcpy(newReadRequestMenu.cOptionName, "New read request");
    newReadRequestMenu.cbEntry = MenuDmaReadRequestOptionCb;

    strcpy(newWriteRequestMenu.cOptionName, "New write request");
    newWriteRequestMenu.cbEntry = MenuDmaWriteRequestOptionCb;

    strcpy(getQueueStatusMenu.cOptionName, "Get queue status");
    getQueueStatusMenu.cbEntry = MenuDmaGetQueueStatusOptionCb;

    options[0] = addMmQueueMenu;
    options[1] = removeMmQueueMenu;
    options[2] = startQueueMenu;
    options[3] = stopQueueMenu;
    options[4] = newReadRequestMenu;
    options[5] = newWriteRequestMenu;
    options[6] = getQueueStatusMenu;

    /* Init requets sub menu tree */
    static DIAG_MENU_OPTION dmaRequestsManagement = { 0 };
    strcpy(dmaRequestsManagement.cOptionName, "Requests");
    strcpy(dmaRequestsManagement.cTitleName, "QDMA DMA requests management "
        "menu");
    MenuDmaRequestsSubMenuInit(&dmaRequestsManagement);

    DIAG_MenuSetCtxAndParentForMenus(options, OPTIONS_SIZE(options),
        phDev, pParentMenu);
    DIAG_MenuSetCtxAndParentForMenus(&dmaRequestsManagement, 1, NULL,
        pParentMenu);
}

static void MenuDmaInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    static DIAG_MENU_OPTION dmaMenuOption = { 0 };

    strcpy(dmaMenuOption.cOptionName, "Direct Memory Access (DMA) transaction");
    strcpy(dmaMenuOption.cTitleName, "Direct Memory Access (DMA)");
    dmaMenuOption.cbIsHidden = MenuCommonIsDeviceNull;

    MenuDmaOptionsInit(&dmaMenuOption, phDev);

    DIAG_MenuSetCtxAndParentForMenus(&dmaMenuOption, 1,
        phDev, pParentMenu);
}

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */

/* Diagnostics plug-and-play and power management events handler routine */
static void DiagEventHandler(WDC_DEVICE_HANDLE hDev, DWORD dwAction)
{
    /* TODO: You can modify this function in order to implement your own
             diagnostics events handler routine. */

    printf("\nReceived event notification (device handle 0x%p): ", hDev);
    switch (dwAction)
    {
    case WD_INSERT:
        printf("WD_INSERT\n");
        break;
    case WD_REMOVE:
        printf("WD_REMOVE\n");
        break;
    case WD_POWER_CHANGED_D0:
        printf("WD_POWER_CHANGED_D0\n");
        break;
    case WD_POWER_CHANGED_D1:
        printf("WD_POWER_CHANGED_D1\n");
        break;
    case WD_POWER_CHANGED_D2:
        printf("WD_POWER_CHANGED_D2\n");
        break;
    case WD_POWER_CHANGED_D3:
        printf("WD_POWER_CHANGED_D3\n");
        break;
    case WD_POWER_SYSTEM_WORKING:
        printf("WD_POWER_SYSTEM_WORKING\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING1:
        printf("WD_POWER_SYSTEM_SLEEPING1\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING2:
        printf("WD_POWER_SYSTEM_SLEEPING2\n");
        break;
    case WD_POWER_SYSTEM_SLEEPING3:
        printf("WD_POWER_SYSTEM_SLEEPING3\n");
        break;
    case WD_POWER_SYSTEM_HIBERNATE:
        printf("WD_POWER_SYSTEM_HIBERNATE\n");
        break;
    case WD_POWER_SYSTEM_SHUTDOWN:
        printf("WD_POWER_SYSTEM_SHUTDOWN\n");
        break;
    default:
        printf("0x%x\n", dwAction);
        break;
    }
}

static DWORD MenuEventsRegisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = QDMA_EventRegister(*(pEventsMenusCtx->phDev),
        (QDMA_EVENT_HANDLER)pEventsMenusCtx->DiagEventHandler);

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events registered\n");
        pEventsMenusCtx->fRegistered = TRUE;
    }
    else
    {
        QDMA_ERR("Failed to register events. Last error [%s]\n",
            QDMA_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsUnregisterOptionCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    DWORD dwStatus = QDMA_EventUnregister(*(pEventsMenusCtx->phDev));

    if (WD_STATUS_SUCCESS == dwStatus)
    {
        printf("Events unregistered\n");
        pEventsMenusCtx->fRegistered = FALSE;
    }
    else
    {
        QDMA_ERR("Failed to unregister events. Last error [%s]\n",
            QDMA_GetLastErr());
    }

    return dwStatus;
}

static DWORD MenuEventsCb(PVOID pCbCtx)
{
    MENU_CTX_EVENTS *pEventsMenusCtx = (MENU_CTX_EVENTS *)pCbCtx;
    pEventsMenusCtx->fRegistered = QDMA_EventIsRegistered(
        *pEventsMenusCtx->phDev);

#ifdef WIN32
    if (!pEventsMenusCtx->fRegistered)
    {
        printf("NOTICE: An INF must be installed for your device in order to \n"
            "        call your user-mode event handler.\n"
            "        You can generate an INF file using the DriverWizard.");
    }
#endif

    return WD_STATUS_SUCCESS;
}

static void MenuEventsInit(DIAG_MENU_OPTION *pParentMenu,
    WDC_DEVICE_HANDLE *phDev)
{
    MENU_EVENTS_CALLBACKS eventsMenuCbs = { 0 };
    static MENU_CTX_EVENTS eventsMenusCtx = { 0 };

    eventsMenuCbs.eventsMenuEntryCb = MenuEventsCb;
    eventsMenuCbs.eventsEnableCb = MenuEventsRegisterOptionCb;
    eventsMenuCbs.eventsDisableCb = MenuEventsUnregisterOptionCb;

    eventsMenusCtx.phDev = phDev;
    eventsMenusCtx.DiagEventHandler = (DIAG_EVENT_HANDLER)DiagEventHandler;

    MenuCommonEventsInit(pParentMenu, &eventsMenusCtx, &eventsMenuCbs);
}

int QDMA_printf(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    return 0;
}



