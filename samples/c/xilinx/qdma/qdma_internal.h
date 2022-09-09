/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef QDMA_REG_H__
#define QDMA_REG_H__

#include "wdc_defs.h"
#include "wdc_lib.h"
#include "qdma_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/

#if defined(LINUX)
    typedef unsigned char UINT8;
    typedef unsigned short UINT16;
#endif

#ifdef CHAR_BIT
#undef CHAR_BIT
#endif
#define CHAR_BIT 8

#ifdef BIT
#undef BIT
#endif
#define BIT(n)                  (1u << (n))

#ifdef BITS_PER_BYTE
#undef BITS_PER_BYTE
#endif
#define BITS_PER_BYTE           CHAR_BIT

#ifdef BITS_PER_LONG
#undef BITS_PER_LONG
#endif
#define BITS_PER_LONG           (sizeof(UINT32) * BITS_PER_BYTE)

#ifdef BITS_PER_LONG_LONG
#undef BITS_PER_LONG_LONG
#endif
#define BITS_PER_LONG_LONG      (sizeof(UINT64) * BITS_PER_BYTE)

#ifdef GENMASK
#undef GENMASK
#endif
#define GENMASK(h, l) \
    ((0xFFFFFFFF << (l)) & (0xFFFFFFFF >> (BITS_PER_LONG - 1 - (h))))

#ifdef GENMASK_ULL
#undef GENMASK_ULL
#endif
#define GENMASK_ULL(h, l) \
    ((0xFFFFFFFFFFFFFFFF << (l)) & \
            (0xFFFFFFFFFFFFFFFF >> (BITS_PER_LONG_LONG - 1 - (h))))


/*
 * Returns the number of trailing 0s in x, starting addressTanslation LSB.
 * Same as gcc __builtin_ffsll function
*/

#ifdef GCC_COMPILER
static uint8_t getTrailingZeros(UINT64 x)
{
    return (__builtin_ffsll(x) - 1);
}
#else
static unsigned char getTrailingZeros(UINT64 x)
{
    unsigned char n = 1;

    if ((x & 0x0000FFFF) == 0) {
        n = n + 16;
        x = x >> 16;
    }
    if ((x & 0x000000FF) == 0) {
        n = n + 8;
        x = x >> 8;
    }
    if ((x & 0x0000000F) == 0) {
        n = n + 4;
        x = x >> 4;
    }
    if ((x & 0x00000003) == 0) {
        n = n + 2;
        x = x >> 2;
    }

    return n - (x & 1);
}
#endif

#define FIELD_SHIFT(mask)       getTrailingZeros(mask)
#define FIELD_SET(mask, val)    ((val << FIELD_SHIFT(mask)) & mask)
#define FIELD_GET(mask, reg)    ((reg & mask) >> FIELD_SHIFT(mask))

/* polling a register */
#define QDMA_REG_POLL_DFLT_INTERVAL_US  100     /* 100us per poll */
#define QDMA_REG_POLL_DFLT_TIMEOUT_US   (500*1000)  /* 500ms */

/*
 * Q Context programming (indirect)
 */

typedef enum {
    QDMA_CTX_CMD_CLR,
    QDMA_CTX_CMD_WR,
    QDMA_CTX_CMD_RD,
    QDMA_CTX_CMD_INV
} IND_CTX_CMD_OP;

typedef enum {
    QDMA_CTX_SEL_SW_C2H,
    QDMA_CTX_SEL_SW_H2C,
    QDMA_CTX_SEL_HW_C2H,
    QDMA_CTX_SEL_HW_H2C,
    QDMA_CTX_SEL_CR_C2H,
    QDMA_CTX_SEL_CR_H2C,
    QDMA_CTX_SEL_CMPT,
    QDMA_CTX_SEL_PFTCH,
    QDMA_CTX_SEL_INT_COAL,
    QDMA_CTX_SEL_PASID_RAM_LOW,
    QDMA_CTX_SEL_PASID_RAM_HIGH,
    QDMA_CTX_SEL_TIMER,
    QDMA_CTX_SEL_FMAP,
} IND_CTX_CMD_SEL;

#define QDMA_REG_IND_CTX_REG_COUNT                         8
#define QDMA_REG_IND_CTX_WCNT_1                            1
#define QDMA_REG_IND_CTX_WCNT_2                            2
#define QDMA_REG_IND_CTX_WCNT_3                            3
#define QDMA_REG_IND_CTX_WCNT_4                            4
#define QDMA_REG_IND_CTX_WCNT_5                            5
#define QDMA_REG_IND_CTX_WCNT_6                            6
#define QDMA_REG_IND_CTX_WCNT_7                            7
#define QDMA_REG_IND_CTX_WCNT_8                            8

/* ------------------------- QDMA_TRQ_SEL_IND (0x00800) ----------------*/
#define QDMA_OFFSET_IND_CTX_DATA                           0x804
#define QDMA_OFFSET_IND_CTX_MASK                           0x824
#define QDMA_OFFSET_IND_CTX_CMD                            0x844
#define     QDMA_IND_CTX_CMD_BUSY_MASK                     0x1


/* ------------------------ indirect register context fields -----------*/

#define QDMA_INDRIECT_PROG_OP_TIMEOUT_US                    (500*1000)
#define QDMA_IND_CTX_DATA_NUM_REGS                         8

/** QDMA_IND_REG_SEL_FMAP */
#define QDMA_FMAP_CTX_W1_QID_MAX_MASK                      GENMASK(11, 0)
#define QDMA_FMAP_CTX_W0_QID_MASK                          GENMASK(10, 0)

/** QDMA_IND_REG_SEL_SW_C2H */
/** QDMA_IND_REG_SEL_SW_H2C */
#define QDMA_SW_CTX_W4_INTR_AGGR_MASK                      BIT11
#define QDMA_SW_CTX_W4_VEC_MASK                            GENMASK(10, 0)
#define QDMA_SW_CTX_W3_DSC_H_MASK                          GENMASK(31, 0)
#define QDMA_SW_CTX_W2_DSC_L_MASK                          GENMASK(31, 0)
#define QDMA_SW_CTX_W1_IS_MM_MASK                          BIT31
#define QDMA_SW_CTX_W1_MRKR_DIS_MASK                       BIT30
#define QDMA_SW_CTX_W1_IRQ_REQ_MASK                        BIT29
#define QDMA_SW_CTX_W1_ERR_WB_SENT_MASK                    BIT28
#define QDMA_SW_CTX_W1_ERR_MASK                            GENMASK(27, 26)
#define QDMA_SW_CTX_W1_IRQ_NO_LAST_MASK                    BIT25
#define QDMA_SW_CTX_W1_PORT_ID_MASK                        GENMASK(24, 22)
#define QDMA_SW_CTX_W1_IRQ_EN_MASK                         BIT21
#define QDMA_SW_CTX_W1_WBK_EN_MASK                         BIT20
#define QDMA_SW_CTX_W1_MM_CHN_MASK                         BIT19
#define QDMA_SW_CTX_W1_BYP_MASK                            BIT18
#define QDMA_SW_CTX_W1_DSC_SZ_MASK                         GENMASK(17, 16)
#define QDMA_SW_CTX_W1_RNG_SZ_MASK                         GENMASK(15, 12)
#define QDMA_SW_CTX_W1_FETCH_MAX_MASK                      GENMASK(7, 5)
#define QDMA_SW_CTX_W1_AT_MASK                             BIT4
#define QDMA_SW_CTX_W1_WB_INT_EN_MASK                      BIT3
#define QDMA_SW_CTX_W1_WBI_CHK_MASK                        BIT2
#define QDMA_SW_CTX_W1_FCRD_EN_MASK                        BIT1
#define QDMA_SW_CTX_W1_QEN_MASK                            BIT0
#define QDMA_SW_CTX_W0_FUNC_ID_MASK                        GENMASK(24, 17)
#define QDMA_SW_CTX_W0_IRQ_ARM_MASK                        BIT16
#define QDMA_SW_CTX_W0_PIDX                                GENMASK(15, 0)

/** QDMA_IND_REG_SEL_PFTCH */
#define QDMA_PFTCH_CTX_SW_CRDT_GET_H_MASK                  GENMASK(15, 3)
#define QDMA_PFTCH_CTX_SW_CRDT_GET_L_MASK                  GENMASK(2, 0)

#define QDMA_PFTCH_CTX_W1_VALID_MASK                       BIT13
#define QDMA_PFTCH_CTX_W1_SW_CRDT_H_MASK                   GENMASK(12, 0)
#define QDMA_PFTCH_CTX_W0_SW_CRDT_L_MASK                   GENMASK(31, 29)
#define QDMA_PFTCH_CTX_W0_Q_IN_PREFETCH_MASK               BIT28
#define QDMA_PFTCH_CTX_W0_PREFETCH_EN_MASK                 BIT27
#define QDMA_PFTCH_CTX_W0_ERR_MASK                         BIT26
#define QDMA_PFTCH_CTX_W0_PORT_ID_MASK                     GENMASK(7, 5)
#define QDMA_PFTCH_CTX_W0_BUF_SIZE_IDX_MASK                GENMASK(4, 1)
#define QDMA_PFTCH_CTX_W0_BYPASS_MASK                      BIT0

/** QDMA_IND_REG_SEL_CMPT */
#define QDMA_COMPL_CTX_BADDR_GET_H_MASK                    GENMASK_ULL(63, 38)
#define QDMA_COMPL_CTX_BADDR_GET_L_MASK                    GENMASK_ULL(37, 12)
#define QDMA_COMPL_CTX_PIDX_GET_H_MASK                     GENMASK(15, 4)
#define QDMA_COMPL_CTX_PIDX_GET_L_MASK                     GENMASK(3, 0)

#define QDMA_COMPL_CTX_W4_INTR_AGGR_MASK                   BIT15
#define QDMA_COMPL_CTX_W4_INTR_VEC_MASK                    GENMASK(14, 4)
#define QDMA_COMPL_CTX_W4_AT_MASK                          BIT3
#define QDMA_COMPL_CTX_W4_OVF_CHK_DIS_MASK                 BIT2
#define QDMA_COMPL_CTX_W4_FULL_UPDT_MASK                   BIT1
#define QDMA_COMPL_CTX_W4_TMR_RUN_MASK                     BIT0
#define QDMA_COMPL_CTX_W3_USR_TRG_PND_MASK                 BIT31
#define QDMA_COMPL_CTX_W3_ERR_MASK                         GENMASK(30, 29)
#define QDMA_COMPL_CTX_W3_VALID_MASK                       BIT28
#define QDMA_COMPL_CTX_W3_CIDX_MASK                        GENMASK(27, 12)
#define QDMA_COMPL_CTX_W3_PIDX_H_MASK                      GENMASK(11, 0)
#define QDMA_COMPL_CTX_W2_PIDX_L_MASK                      GENMASK(31, 28)
#define QDMA_COMPL_CTX_W2_DESC_SIZE_MASK                   GENMASK(27, 26)
#define QDMA_COMPL_CTX_W2_BADDR_64_H_MASK                  GENMASK(25, 0)
#define QDMA_COMPL_CTX_W1_BADDR_64_L_MASK                  GENMASK(31, 6)
#define QDMA_COMPL_CTX_W0_RING_SZ_MASK                     GENMASK(31, 28)
#define QDMA_COMPL_CTX_W0_COLOR_MASK                       BIT27
#define QDMA_COMPL_CTX_W0_INT_ST_MASK                      GENMASK(26, 25)
#define QDMA_COMPL_CTX_W0_TIMER_IDX_MASK                   GENMASK(24, 21)
#define QDMA_COMPL_CTX_W0_COUNTER_IDX_MASK                 GENMASK(20, 17)
#define QDMA_COMPL_CTX_W0_FNC_ID_MASK                      GENMASK(12, 5)
#define QDMA_COMPL_CTX_W0_TRIG_MODE_MASK                   GENMASK(4, 2)
#define QDMA_COMPL_CTX_W0_EN_INT_MASK                      BIT1
#define QDMA_COMPL_CTX_W0_EN_STAT_DESC_MASK                BIT0

/** QDMA_IND_REG_SEL_HW_C2H */
/** QDMA_IND_REG_SEL_HW_H2C */
#define QDMA_HW_CTX_W1_FETCH_PEND_MASK                     GENMASK(14, 11)
#define QDMA_HW_CTX_W1_EVENT_PEND_MASK                     BIT10
#define QDMA_HW_CTX_W1_IDL_STP_B_MASK                      BIT9
#define QDMA_HW_CTX_W1_DSC_PND_MASK                        BIT8
#define QDMA_HW_CTX_W0_CRD_USE_MASK                        GENMASK(31, 16)
#define QDMA_HW_CTX_W0_CIDX_MASK                           GENMASK(15, 0)

/** QDMA_IND_REG_SEL_CR_C2H */
/** QDMA_IND_REG_SEL_CR_H2C */
#define QDMA_CR_CTX_W0_CREDT_MASK                          GENMASK(15, 0)

/** QDMA_IND_REG_SEL_INTR */
#define QDMA_INTR_CTX_BADDR_GET_H_MASK                     GENMASK_ULL(63, 61)
#define QDMA_INTR_CTX_BADDR_GET_M_MASK                     GENMASK_ULL(60, 29)
#define QDMA_INTR_CTX_BADDR_GET_L_MASK                     GENMASK_ULL(28, 12)

#define QDMA_INTR_CTX_W2_AT_MASK                           BIT18
#define QDMA_INTR_CTX_W2_PIDX_MASK                         GENMASK(17, 6)
#define QDMA_INTR_CTX_W2_PAGE_SIZE_MASK                    GENMASK(5, 3)
#define QDMA_INTR_CTX_W2_BADDR_64_MASK                     GENMASK(2, 0)
#define QDMA_INTR_CTX_W1_BADDR_64_MASK                     GENMASK(31, 0)
#define QDMA_INTR_CTX_W0_BADDR_64_MASK                     GENMASK(31, 15)
#define QDMA_INTR_CTX_W0_COLOR_MASK                        BIT14
#define QDMA_INTR_CTX_W0_INT_ST_MASK                       BIT13
#define QDMA_INTR_CTX_W0_VEC_ID_MASK                       GENMASK(11, 1)
#define QDMA_INTR_CTX_W0_VALID_MASK                        BIT0

/** Constants */
#define QDMA_NUM_RING_SIZES                                 16
#define QDMA_NUM_C2H_TIMERS                                 16
#define QDMA_NUM_C2H_BUFFER_SIZES                           16
#define QDMA_NUM_C2H_COUNTERS                               16
#define QDMA_MM_CONTROL_RUN                                 0x1
#define QDMA_MM_CONTROL_STEP                                0x100
#define QDMA_MAGIC_NUMBER                                   0x1fd3
#define QDMA_PIDX_STEP                                      0x10
#define QDMA_CMPT_CIDX_STEP                                 0x10
#define QDMA_INT_CIDX_STEP                                  0x10

/* ------------------------ QDMA_TRQ_SEL_GLBL (0x00200)-------------------*/
#define QDMA_OFFSET_GLBL_RNG_SZ                             0x204
#define QDMA_OFFSET_GLBL_SCRATCH                            0x244
#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define QDMA_OFFSET_GLBL_DSC_CFG                            0x250
#define QDMA_GLBL_DSC_CFG_WB_ACC_INT_MASK               GENMASK(2, 0)
#define QDMA_GLBL_DSC_CFG_MAX_DSC_FETCH_MASK            GENMASK(5, 3)
#define QDMA_OFFSET_GLBL_DSC_ERR_STS                        0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MSK                        0x258
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG0                       0x25C
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG1                       0x260
#define QDMA_OFFSET_GLBL_TRQ_ERR_STS                        0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MSK                        0x268
#define QDMA_OFFSET_GLBL_TRQ_ERR_LOG                        0x26C
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT0                       0x270
#define QDMA_OFFSET_GLBL_DSC_DBG_DAT1                       0x274
#define QDMA_OFFSET_GLBL_DSC_ERR_LOG2                       0x27C
#define QDMA_OFFSET_GLBL_INTERRUPT_CFG                      0x2C4
#define     QDMA_GLBL_INTR_CFG_EN_LGCY_INTR_MASK            BIT0
#define     QDMA_GLBL_INTR_LGCY_INTR_PEND_MASK              BIT1

/* ------------------------- QDMA_TRQ_SEL_C2H (0x00A00) ------------------*/
#define QDMA_OFFSET_C2H_TIMER_CNT                           0xA00
#define QDMA_OFFSET_C2H_CNT_TH                              0xA40
#define QDMA_OFFSET_C2H_QID2VEC_MAP_QID                     0xA80
#define QDMA_OFFSET_C2H_QID2VEC_MAP                         0xA84
#define QDMA_OFFSET_C2H_STAT_S_AXIS_C2H_ACCEPTED            0xA88
#define QDMA_OFFSET_C2H_STAT_S_AXIS_CMPT_ACCEPTED           0xA8C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_PKT_ACCEPTED          0xA90
#define QDMA_OFFSET_C2H_STAT_AXIS_PKG_CMP                   0xA94
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ACCEPTED              0xA98
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_CMP                   0xA9C
#define QDMA_OFFSET_C2H_STAT_WRQ_OUT                        0xAA0
#define QDMA_OFFSET_C2H_STAT_WPL_REN_ACCEPTED               0xAA4
#define QDMA_OFFSET_C2H_STAT_TOTAL_WRQ_LEN                  0xAA8
#define QDMA_OFFSET_C2H_STAT_TOTAL_WPL_LEN                  0xAAC
#define QDMA_OFFSET_C2H_BUF_SZ                              0xAB0
#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define QDMA_OFFSET_C2H_FATAL_ERR_ENABLE                    0xB00
#define QDMA_OFFSET_C2H_ERR_INT                             0xB04
#define QDMA_OFFSET_C2H_PREFETCH_CFG                        0xB08
#define     QDMA_C2H_EVT_QCNT_TH_MASK                       GENMASK(31, 25)
#define     QDMA_C2H_PFCH_QCNT_MASK                         GENMASK(24, 18)
#define     QDMA_C2H_NUM_PFCH_MASK                          GENMASK(17, 9)
#define     QDMA_C2H_PFCH_FL_TH_MASK                        GENMASK(8, 0)
#define QDMA_OFFSET_C2H_INT_TIMER_TICK                      0xB0C
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_DROP_ACCEPTED         0xB10
#define QDMA_OFFSET_C2H_STAT_DESC_RSP_ERR_ACCEPTED          0xB14
#define QDMA_OFFSET_C2H_STAT_DESC_REQ                       0xB18
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_0                0xB1C
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_1                0xB20
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_2                0xB24
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_3                0xB28
#define QDMA_OFFSET_C2H_DBG_PFCH_ERR_CTX                   0xB2C
#define QDMA_OFFSET_C2H_FIRST_ERR_QID                       0xB30
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_IN                    0xB34
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_OUT                   0xB38
#define QDMA_OFFSET_C2H_STAT_NUM_CMPT_DRP                   0xB3C
#define QDMA_OFFSET_C2H_STAT_NUM_STAT_DESC_OUT              0xB40
#define QDMA_OFFSET_C2H_STAT_NUM_DSC_CRDT_SENT              0xB44
#define QDMA_OFFSET_C2H_STAT_NUM_FCH_DSC_RCVD               0xB48
#define QDMA_OFFSET_C2H_STAT_NUM_BYP_DSC_RCVD               0xB4C
#define QDMA_OFFSET_C2H_WRB_COAL_CFG                        0xB50
#define     QDMA_C2H_MAX_BUF_SZ_MASK                        GENMASK(31, 26)
#define     QDMA_C2H_TICK_VAL_MASK                          GENMASK(25, 14)
#define     QDMA_C2H_TICK_CNT_MASK                          GENMASK(13, 2)
#define     QDMA_C2H_SET_GLB_FLUSH_MASK                     BIT1
#define     QDMA_C2H_DONE_GLB_FLUSH_MASK                    BIT0
#define QDMA_OFFSET_C2H_INTR_H2C_REQ                        0xB54
#define QDMA_OFFSET_C2H_INTR_C2H_MM_REQ                     0xB58
#define QDMA_OFFSET_C2H_INTR_ERR_INT_REQ                    0xB5C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_REQ                     0xB60
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_ACK        0xB64
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_FAIL       0xB68
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_MSIX_NO_MSIX    0xB6C
#define QDMA_OFFSET_C2H_INTR_H2C_ERR_C2H_MM_CTX_INVAL      0xB70
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_ACK                0xB74
#define QDMA_OFFSET_C2H_INTR_C2H_ST_MSIX_FAIL               0xB78
#define QDMA_OFFSET_C2H_INTR_C2H_ST_NO_MSIX                 0xB7C
#define QDMA_OFFSET_C2H_INTR_C2H_ST_CTX_INVAL              0xB80
#define QDMA_OFFSET_C2H_STAT_WR_CMP                         0xB84
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_4                0xB88
#define QDMA_OFFSET_C2H_STAT_DEBUG_DMA_ENG_5                0xB8C
#define QDMA_OFFSET_C2H_DBG_PFCH_QID                        0xB90
#define QDMA_OFFSET_C2H_DBG_PFCH                            0xB94
#define QDMA_OFFSET_C2H_INT_DEBUG                           0xB98
#define QDMA_OFFSET_C2H_STAT_IMM_ACCEPTED                   0xB9C
#define QDMA_OFFSET_C2H_STAT_MARKER_ACCEPTED                0xBA0
#define QDMA_OFFSET_C2H_STAT_DISABLE_CMP_ACCEPTED           0xBA4
#define QDMA_OFFSET_C2H_PAYLOAD_FIFO_CRDT_CNT               0xBA8
#define QDMA_OFFSET_C2H_PREFETCH_CACHE_DEPTH                0xBE0
#define QDMA_OFFSET_C2H_CMPT_COAL_BUF_DEPTH                 0xBE4

/* ------------------------- QDMA_TRQ_SEL_H2C (0x00E00) ------------------*/
#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define QDMA_OFFSET_H2C_FIRST_ERR_QID                       0xE08
#define QDMA_OFFSET_H2C_DBG_REG0                            0xE0C
#define QDMA_OFFSET_H2C_DBG_REG1                            0xE10
#define QDMA_OFFSET_H2C_DBG_REG2                            0xE14
#define QDMA_OFFSET_H2C_DBG_REG3                            0xE18
#define QDMA_OFFSET_H2C_DBG_REG4                            0xE1C
#define QDMA_OFFSET_H2C_FATAL_ERR_EN                        0xE20
#define QDMA_OFFSET_H2C_REQ_THROT                           0xE24
#define     QDMA_H2C_REQ_THROT_EN_REQ_MASK                  BIT31
#define     QDMA_H2C_REQ_THRESH_MASK                        GENMASK(25, 17)
#define     QDMA_H2C_REQ_THROT_EN_DATA_MASK                 BIT16
#define     QDMA_H2C_DATA_THRESH_MASK                       GENMASK(15, 0)


/* ------------------------- QDMA_TRQ_SEL_H2C_MM (0x1200) ----------------*/
#define QDMA_OFFSET_H2C_MM_CONTROL                          0x1204
#define QDMA_OFFSET_H2C_MM_CONTROL_W1S                      0x1208
#define QDMA_OFFSET_H2C_MM_CONTROL_W1C                      0x120C
#define QDMA_OFFSET_H2C_MM_STATUS                           0x1240
#define QDMA_OFFSET_H2C_MM_STATUS_RC                        0x1244
#define QDMA_OFFSET_H2C_MM_COMPLETED_DESC_COUNT             0x1248
#define QDMA_OFFSET_H2C_MM_ERR_CODE_EN_MASK                 0x1254
#define QDMA_OFFSET_H2C_MM_ERR_CODE                         0x1258
#define QDMA_OFFSET_H2C_MM_ERR_INFO                         0x125C
#define QDMA_OFFSET_H2C_MM_PERF_MON_CONTROL                 0x12C0
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_0           0x12C4
#define QDMA_OFFSET_H2C_MM_PERF_MON_CYCLE_COUNT_1           0x12C8
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_0            0x12CC
#define QDMA_OFFSET_H2C_MM_PERF_MON_DATA_COUNT_1            0x12D0
#define QDMA_OFFSET_H2C_MM_DEBUG                            0x12E8

/* ------------------------- QDMA_TRQ_SEL_C2H_MM (0x1000) ----------------*/
#define QDMA_OFFSET_C2H_MM_CONTROL                          0x1004
#define QDMA_OFFSET_C2H_MM_CONTROL_W1S                      0x1008
#define QDMA_OFFSET_C2H_MM_CONTROL_W1C                      0x100C
#define QDMA_OFFSET_C2H_MM_STATUS                           0x1040
#define QDMA_OFFSET_C2H_MM_STATUS_RC                        0x1044
#define QDMA_OFFSET_C2H_MM_COMPLETED_DESC_COUNT             0x1048
#define QDMA_OFFSET_C2H_MM_ERR_CODE_EN_MASK                 0x1054
#define QDMA_OFFSET_C2H_MM_ERR_CODE                         0x1058
#define QDMA_OFFSET_C2H_MM_ERR_INFO                         0x105C
#define QDMA_OFFSET_C2H_MM_PERF_MON_CONTROL                 0x10C0
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_0           0x10C4
#define QDMA_OFFSET_C2H_MM_PERF_MON_CYCLE_COUNT_1           0x10C8
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_0            0x10CC
#define QDMA_OFFSET_C2H_MM_PERF_MON_DATA_COUNT_1            0x10D0
#define QDMA_OFFSET_C2H_MM_DEBUG                            0x10E8

/* ------------------------- QDMA_TRQ_SEL_GLBL1 (0x0) -----------------*/
#define QDMA_OFFSET_CONFIG_BLOCK_ID                         0x0
#define     QDMA_CONFIG_BLOCK_ID_MASK                       GENMASK(31, 16)


/* ------------------------- QDMA_TRQ_SEL_GLBL2 (0x00100) ----------------*/
#define QDMA_OFFSET_GLBL2_ID                                0x100
#define QDMA_OFFSET_GLBL2_PF_BARLITE_INT                    0x104
#define     QDMA_GLBL2_PF3_BAR_MAP_MASK                     GENMASK(23, 18)
#define     QDMA_GLBL2_PF2_BAR_MAP_MASK                     GENMASK(17, 12)
#define     QDMA_GLBL2_PF1_BAR_MAP_MASK                     GENMASK(11, 6)
#define     QDMA_GLBL2_PF0_BAR_MAP_MASK                     GENMASK(5, 0)
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_INT                 0x108
#define QDMA_OFFSET_GLBL2_PF_BARLITE_EXT                    0x10C
#define QDMA_OFFSET_GLBL2_PF_VF_BARLITE_EXT                 0x110
#define QDMA_OFFSET_GLBL2_CHANNEL_INST                      0x114
#define QDMA_OFFSET_GLBL2_CHANNEL_MDMA                      0x118
#define     QDMA_GLBL2_ST_C2H_MASK                          BIT16
#define     QDMA_GLBL2_ST_H2C_MASK                          BIT17
#define     QDMA_GLBL2_MM_C2H_MASK                          BIT8
#define     QDMA_GLBL2_MM_H2C_MASK                          BIT0
#define QDMA_OFFSET_GLBL2_CHANNEL_STRM                      0x11C
#define QDMA_OFFSET_GLBL2_CHANNEL_QDMA_CAP                  0x120
#define     QDMA_GLBL2_MULTQ_MAX_MASK                       GENMASK(11, 0)
#define QDMA_OFFSET_GLBL2_CHANNEL_PASID_CAP                 0x128
#define QDMA_OFFSET_GLBL2_CHANNEL_FUNC_RET                  0x12C
#define QDMA_OFFSET_GLBL2_SYSTEM_ID                         0x130
#define QDMA_OFFSET_GLBL2_MISC_CAP                          0x134
#define     QDMA_GLBL2_EVEREST_IP_MASK                      BIT28
#define     QDMA_GLBL2_VIVADO_RELEASE_MASK                  GENMASK(27, 24)
#define     QDMA_GLBL2_RTL_VERSION_MASK                     GENMASK(23, 16)
#define     QDMA_GLBL2_MM_CMPT_EN_MASK                      BIT2
#define     QDMA_GLBL2_FLR_PRESENT_MASK                     BIT1
#define     QDMA_GLBL2_MAILBOX_EN_MASK                      BIT0
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ0                      0x1B8
#define QDMA_OFFSET_GLBL2_DBG_PCIE_RQ1                      0x1BC
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR0                     0x1C0
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_WR1                     0x1C4
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD0                     0x1C8
#define QDMA_OFFSET_GLBL2_DBG_AXIMM_RD1                     0x1CC

/* used for VF bars identification */
#define QDMA_OFFSET_VF_USER_BAR_ID                          0x1018
#define QDMA_OFFSET_VF_CONFIG_BAR_ID                        0x1014

/* FLR programming */
#define QDMA_OFFSET_VF_REG_FLR_STATUS                       0x1100
#define QDMA_OFFSET_PF_REG_FLR_STATUS                       0x2500
#define     QDMA_FLR_STATUS_MASK                            0x1

/* VF qdma u32Version */
#define QDMA_OFFSET_VF_VERSION                              0x1014
#define     QDMA_GLBL2_VF_EVEREST_IP_MASK                   BIT12
#define     QDMA_GLBL2_VF_VIVADO_RELEASE_MASK               GENMASK(11, 8)
#define     QDMA_GLBL2_VF_RTL_VERSION_MASK                  GENMASK(7, 0)


/* ------------------------- QDMA_TRQ_SEL_QUEUE_PF (0x18000) ----------------*/

#define QDMA_OFFSET_DMAP_SEL_INT_CIDX                       0x18000
#define QDMA_OFFSET_DMAP_SEL_H2C_DSC_PIDX                   0x18004
#define QDMA_OFFSET_DMAP_SEL_C2H_DSC_PIDX                   0x18008
#define QDMA_OFFSET_DMAP_SEL_CMPT_CIDX                      0x1800C

#define QDMA_OFFSET_VF_DMAP_SEL_INT_CIDX                    0x3000
#define QDMA_OFFSET_VF_DMAP_SEL_H2C_DSC_PIDX                0x3004
#define QDMA_OFFSET_VF_DMAP_SEL_C2H_DSC_PIDX                0x3008
#define QDMA_OFFSET_VF_DMAP_SEL_CMPT_CIDX                   0x300C

#define     QDMA_DMA_SEL_INT_SW_CIDX_MASK                   GENMASK(15, 0)
#define     QDMA_DMA_SEL_INT_RING_IDX_MASK                  GENMASK(23, 16)
#define     QDMA_DMA_SEL_DESC_PIDX_MASK                     GENMASK(15, 0)
#define     QDMA_DMA_SEL_IRQ_EN_MASK                        BIT16
#define     QDMA_DMAP_SEL_CMPT_IRQ_EN_MASK                  BIT28
#define     QDMA_DMAP_SEL_CMPT_STS_DESC_EN_MASK             BIT27
#define     QDMA_DMAP_SEL_CMPT_TRG_MODE_MASK                GENMASK(26, 24)
#define     QDMA_DMAP_SEL_CMPT_TMR_CNT_MASK                 GENMASK(23, 20)
#define     QDMA_DMAP_SEL_CMPT_CNT_THRESH_MASK              GENMASK(19, 16)
#define     QDMA_DMAP_SEL_CMPT_WRB_CIDX_MASK                GENMASK(15, 0)

/* ------------------------- Hardware Errors ------------------------------ */
#define TOTAL_LEAF_ERROR_AGGREGATORS                        7

#define QDMA_OFFSET_GLBL_ERR_INT                            0xB04
#define     QDMA_GLBL_ERR_FUNC_MASK                         GENMASK(7, 0)
#define     QDMA_GLBL_ERR_VEC_MASK                          GENMASK(22, 12)
#define     QDMA_GLBL_ERR_ARM_MASK                          BIT24

#define QDMA_OFFSET_GLBL_ERR_STAT                           0x248
#define QDMA_OFFSET_GLBL_ERR_MASK                           0x24C
#define     QDMA_GLBL_ERR_RAM_SBE_MASK                      BIT0
#define     QDMA_GLBL_ERR_RAM_DBE_MASK                      BIT1
#define     QDMA_GLBL_ERR_DSC_MASK                          BIT2
#define     QDMA_GLBL_ERR_TRQ_MASK                          BIT3
#define     QDMA_GLBL_ERR_ST_C2H_MASK                       BIT8
#define     QDMA_GLBL_ERR_ST_H2C_MASK                       BIT11

#define QDMA_OFFSET_C2H_ERR_STAT                            0xAF0
#define QDMA_OFFSET_C2H_ERR_MASK                            0xAF4
#define     QDMA_C2H_ERR_MTY_MISMATCH_MASK                  BIT0
#define     QDMA_C2H_ERR_LEN_MISMATCH_MASK                  BIT1
#define     QDMA_C2H_ERR_QID_MISMATCH_MASK                  BIT3
#define     QDMA_C2H_ERR_DESC_RSP_ERR_MASK                  BIT4
#define     QDMA_C2H_ERR_ENG_WPL_DATA_PAR_ERR_MASK          BIT6
#define     QDMA_C2H_ERR_MSI_INT_FAIL_MASK                  BIT7
#define     QDMA_C2H_ERR_ERR_DESC_CNT_MASK                  BIT9
#define     QDMA_C2H_ERR_PORTID_CTX_MISMATCH_MASK          BIT10
#define     QDMA_C2H_ERR_PORTID_BYP_IN_MISMATCH_MASK        BIT11
#define     QDMA_C2H_ERR_CMPT_INV_Q_ERR_MASK                BIT12
#define     QDMA_C2H_ERR_CMPT_QFULL_ERR_MASK                BIT13
#define     QDMA_C2H_ERR_CMPT_CIDX_ERR_MASK                 BIT14
#define     QDMA_C2H_ERR_CMPT_PRTY_ERR_MASK                 BIT15
#define     QDMA_C2H_ERR_ALL_MASK                           0xFEDB

#define QDMA_OFFSET_C2H_FATAL_ERR_STAT                      0xAF8
#define QDMA_OFFSET_C2H_FATAL_ERR_MASK                      0xAFC
#define     QDMA_C2H_FATAL_ERR_MTY_MISMATCH_MASK            BIT0
#define     QDMA_C2H_FATAL_ERR_LEN_MISMATCH_MASK            BIT1
#define     QDMA_C2H_FATAL_ERR_QID_MISMATCH_MASK            BIT3
#define     QDMA_C2H_FATAL_ERR_TIMER_FIFO_RAM_RDBE_MASK     BIT4
#define     QDMA_C2H_FATAL_ERR_PFCH_II_RAM_RDBE_MASK        BIT8
#define     QDMA_C2H_FATAL_ERR_CMPT_CTX_RAM_RDBE_MASK      BIT9
#define     QDMA_C2H_FATAL_ERR_PFCH_CTX_RAM_RDBE_MASK      BIT10
#define     QDMA_C2H_FATAL_ERR_DESC_REQ_FIFO_RAM_RDBE_MASK  BIT11
#define     QDMA_C2H_FATAL_ERR_INT_CTX_RAM_RDBE_MASK       BIT12
#define     QDMA_C2H_FATAL_ERR_CMPT_COAL_DATA_RAM_RDBE_MASK BIT14
#define     QDMA_C2H_FATAL_ERR_TUSER_FIFO_RAM_RDBE_MASK     BIT15
#define     QDMA_C2H_FATAL_ERR_QID_FIFO_RAM_RDBE_MASK       BIT16
#define     QDMA_C2H_FATAL_ERR_PAYLOAD_FIFO_RAM_RDBE_MASK   BIT17
#define     QDMA_C2H_FATAL_ERR_WPL_DATA_PAR_MASK            BIT18
#define     QDMA_C2H_FATAL_ERR_ALL_MASK                     0x7DF1B

#define QDMA_OFFSET_H2C_ERR_STAT                            0xE00
#define QDMA_OFFSET_H2C_ERR_MASK                            0xE04
#define     QDMA_H2C_ERR_ZERO_LEN_DESC_MASK                 BIT0
#define     QDMA_H2C_ERR_CSI_MOP_MASK                       BIT1
#define     QDMA_H2C_ERR_NO_DMA_DSC_MASK                    BIT2
#define     QDMA_H2C_ERR_SBE_MASK                           BIT3
#define     QDMA_H2C_ERR_DBE_MASK                           BIT4
#define     QDMA_H2C_ERR_ALL_MASK                           0x1F

#define QDMA_OFFSET_GLBL_DSC_ERR_STAT                       0x254
#define QDMA_OFFSET_GLBL_DSC_ERR_MASK                       0x258
#define     QDMA_GLBL_DSC_ERR_POISON_MASK                   BIT0
#define     QDMA_GLBL_DSC_ERR_UR_CA_MASK                    BIT1
#define     QDMA_GLBL_DSC_ERR_PARAM_MASK                    BIT2
#define     QDMA_GLBL_DSC_ERR_ADDR_MASK                     BIT3
#define     QDMA_GLBL_DSC_ERR_TAG_MASK                      BIT4
#define     QDMA_GLBL_DSC_ERR_FLR_MASK                      BIT5
#define     QDMA_GLBL_DSC_ERR_TIMEOUT_MASK                  BIT9
#define     QDMA_GLBL_DSC_ERR_DAT_POISON_MASK               BIT16
#define     QDMA_GLBL_DSC_ERR_FLR_CANCEL_MASK               BIT19
#define     QDMA_GLBL_DSC_ERR_DMA_MASK                      BIT20
#define     QDMA_GLBL_DSC_ERR_DSC_MASK                      BIT21
#define     QDMA_GLBL_DSC_ERR_RQ_CANCEL_MASK                BIT22
#define     QDMA_GLBL_DSC_ERR_DBE_MASK                      BIT23
#define     QDMA_GLBL_DSC_ERR_SBE_MASK                      BIT24
#define     QDMA_GLBL_DSC_ERR_ALL_MASK                      0x1F9023F

#define QDMA_OFFSET_GLBL_TRQ_ERR_STAT                       0x264
#define QDMA_OFFSET_GLBL_TRQ_ERR_MASK                       0x268
#define     QDMA_GLBL_TRQ_ERR_UNMAPPED_MASK                 BIT0
#define     QDMA_GLBL_TRQ_ERR_QID_RANGE_MASK                BIT1
#define     QDMA_GLBL_TRQ_ERR_VF_ACCESS_MASK                BIT2
#define     QDMA_GLBL_TRQ_ERR_TCP_TIMEOUT_MASK              BIT3
#define     QDMA_GLBL_TRQ_ERR_ALL_MASK                      0xF

#define QDMA_OFFSET_RAM_SBE_STAT                            0xF4
#define QDMA_OFFSET_RAM_SBE_MASK                            0xF0
#define     QDMA_SBE_ERR_MI_H2C0_DAT_MASK                   BIT0
#define     QDMA_SBE_ERR_MI_C2H0_DAT_MASK                   BIT4
#define     QDMA_SBE_ERR_H2C_RD_BRG_DAT_MASK                BIT9
#define     QDMA_SBE_ERR_H2C_WR_BRG_DAT_MASK                BIT10
#define     QDMA_SBE_ERR_C2H_RD_BRG_DAT_MASK                BIT11
#define     QDMA_SBE_ERR_C2H_WR_BRG_DAT_MASK                BIT12
#define     QDMA_SBE_ERR_FUNC_MAP_MASK                      BIT13
#define     QDMA_SBE_ERR_DSC_HW_CTX_MASK                   BIT14
#define     QDMA_SBE_ERR_DSC_CRD_RCV_MASK                   BIT15
#define     QDMA_SBE_ERR_DSC_SW_CTX_MASK                   BIT16
#define     QDMA_SBE_ERR_DSC_CPLI_MASK                      BIT17
#define     QDMA_SBE_ERR_DSC_CPLD_MASK                      BIT18
#define     QDMA_SBE_ERR_PASID_CTX_RAM_MASK                BIT19
#define     QDMA_SBE_ERR_TIMER_FIFO_RAM_MASK                BIT20
#define     QDMA_SBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT21
#define     QDMA_SBE_ERR_QID_FIFO_RAM_MASK                  BIT22
#define     QDMA_SBE_ERR_TUSER_FIFO_RAM_MASK                BIT23
#define     QDMA_SBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT24
#define     QDMA_SBE_ERR_INT_QID2VEC_RAM_MASK               BIT25
#define     QDMA_SBE_ERR_INT_CTX_RAM_MASK                  BIT26
#define     QDMA_SBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT27
#define     QDMA_SBE_ERR_PFCH_CTX_RAM_MASK                 BIT28
#define     QDMA_SBE_ERR_WRB_CTX_RAM_MASK                  BIT29
#define     QDMA_SBE_ERR_PFCH_LL_RAM_MASK                   BIT30
#define     QDMA_SBE_ERR_H2C_PEND_FIFO_MASK                 BIT31
#define     QDMA_SBE_ERR_ALL_MASK                           0xFFFFFF11

#define QDMA_OFFSET_RAM_DBE_STAT                            0xFC
#define QDMA_OFFSET_RAM_DBE_MASK                            0xF8
#define     QDMA_DBE_ERR_MI_H2C0_DAT_MASK                   BIT0
#define     QDMA_DBE_ERR_MI_C2H0_DAT_MASK                   BIT4
#define     QDMA_DBE_ERR_H2C_RD_BRG_DAT_MASK                BIT9
#define     QDMA_DBE_ERR_H2C_WR_BRG_DAT_MASK                BIT10
#define     QDMA_DBE_ERR_C2H_RD_BRG_DAT_MASK                BIT11
#define     QDMA_DBE_ERR_C2H_WR_BRG_DAT_MASK                BIT12
#define     QDMA_DBE_ERR_FUNC_MAP_MASK                      BIT13
#define     QDMA_DBE_ERR_DSC_HW_CTX_MASK                   BIT14
#define     QDMA_DBE_ERR_DSC_CRD_RCV_MASK                   BIT15
#define     QDMA_DBE_ERR_DSC_SW_CTX_MASK                   BIT16
#define     QDMA_DBE_ERR_DSC_CPLI_MASK                      BIT17
#define     QDMA_DBE_ERR_DSC_CPLD_MASK                      BIT18
#define     QDMA_DBE_ERR_PASID_CTX_RAM_MASK                BIT19
#define     QDMA_DBE_ERR_TIMER_FIFO_RAM_MASK                BIT20
#define     QDMA_DBE_ERR_PAYLOAD_FIFO_RAM_MASK              BIT21
#define     QDMA_DBE_ERR_QID_FIFO_RAM_MASK                  BIT22
#define     QDMA_DBE_ERR_TUSER_FIFO_RAM_MASK                BIT23
#define     QDMA_DBE_ERR_WRB_COAL_DATA_RAM_MASK             BIT24
#define     QDMA_DBE_ERR_INT_QID2VEC_RAM_MASK               BIT25
#define     QDMA_DBE_ERR_INT_CTX_RAM_MASK                  BIT26
#define     QDMA_DBE_ERR_DESC_REQ_FIFO_RAM_MASK             BIT27
#define     QDMA_DBE_ERR_PFCH_CTX_RAM_MASK                 BIT28
#define     QDMA_DBE_ERR_WRB_CTX_RAM_MASK                  BIT29
#define     QDMA_DBE_ERR_PFCH_LL_RAM_MASK                   BIT30
#define     QDMA_DBE_ERR_H2C_PEND_FIFO_MASK                 BIT31
#define     QDMA_DBE_ERR_ALL_MASK                           0xFFFFFF11


/** QDMA Context array size */
#define QDMA_FMAP_NUM_WORDS                    2
#define QDMA_SW_CONTEXT_NUM_WORDS              5
#define QDMA_PREFETCH_CONTEXT_NUM_WORDS        2
#define QDMA_CMPT_CONTEXT_NUM_WORDS            5
#define QDMA_HW_CONTEXT_NUM_WORDS              2
#define QDMA_CR_CONTEXT_NUM_WORDS              1
#define QDMA_IND_INTR_CONTEXT_NUM_WORDS        3

/* CSR Default values */
#define DEFAULT_MAX_DSC_FETCH               6
#define DEFAULT_WRB_INT                     QDMA_WRB_INTERVAL_128
#define DEFAULT_PFCH_STOP_THRESH            256
#define DEFAULT_PFCH_NUM_ENTRIES_PER_Q      8
#define DEFAULT_PFCH_MAX_Q_CNT              16
#define DEFAULT_C2H_INTR_TIMER_TICK         25
#define DEFAULT_CMPT_COAL_TIMER_CNT         5
#define DEFAULT_CMPT_COAL_TIMER_TICK        25
#define DEFAULT_CMPT_COAL_MAX_BUF_SZ        32
#define DEFAULT_H2C_THROT_DATA_THRESH       0x4000
#define DEFAULT_THROT_EN_DATA               1
#define DEFAULT_THROT_EN_REQ                0
#define DEFAULT_H2C_THROT_REQ_THRESH        0x60

typedef struct {
    /** @numPfs - Num of PFs*/
    BYTE numPfs;
    /** @numQueues - Num of Queues */
    WORD numQueues;
    /** @fIsFlrPresent - FLR resent or not? */
    BOOL fIsFlrPresent : 1;
    /** @fIsStreamingEnabled - ST mode supported or not? */
    BOOL fIsStreamingEnabled : 1;
    /** @fIsMmEnabled - MM mode supported or not? */
    BOOL fIsMmEnabled : 1;
    /** @fIsMmCompletionsSupported - MM with Completions supported or not? */
    BOOL fIsMmCompletionsSupported : 1;
    /** @fIsMailboxEnabled - Mailbox supported or not? */
    BOOL fIsMailboxEnabled : 1;
    /** @mmChannelMax - Num of MM channels */
    BYTE mmChannelMax;
} QDMA_DEVICE_ATTRIBUTES;

typedef enum {
    /** @QDMA_WRB_INTERVAL_4 - writeback update interval of 4 */
    QDMA_WRB_INTERVAL_4,
    /** @QDMA_WRB_INTERVAL_8 - writeback update interval of 8 */
    QDMA_WRB_INTERVAL_8,
    /** @QDMA_WRB_INTERVAL_16 - writeback update interval of 16 */
    QDMA_WRB_INTERVAL_16,
    /** @QDMA_WRB_INTERVAL_32 - writeback update interval of 32 */
    QDMA_WRB_INTERVAL_32,
    /** @QDMA_WRB_INTERVAL_64 - writeback update interval of 64 */
    QDMA_WRB_INTERVAL_64,
    /** @QDMA_WRB_INTERVAL_128 - writeback update interval of 128 */
    QDMA_WRB_INTERVAL_128,
    /** @QDMA_WRB_INTERVAL_256 - writeback update interval of 256 */
    QDMA_WRB_INTERVAL_256,
    /** @QDMA_WRB_INTERVAL_512 - writeback update interval of 512 */
    QDMA_WRB_INTERVAL_512,
    /** @QDMA_NUM_WRB_INTERVALS - total number of writeback intervals */
    QDMA_NUM_WRB_INTERVALS
} QDMA_WRB_INTERVAL;

typedef struct {
    /** @qbase - queue base for the function */
    UINT32 qbase : 11;
    /** @ reserved1 - Reserved */
    UINT32 reserved1 : 21;
    /** @qmax - maximum queues in the function */
    UINT32 qmax : 12;
    /** @ reserved1 - Reserved */
    UINT32 reserved2 : 20;
} QDMA_FMAP_CONFIG;


/* ----------- constants ----------- */

/* register base addresses */
#define QDMA_TRQ_SEL_GLBL_BASE1 0x00000000
#define QDMA_TRQ_SEL_ATTRIB_BASE 0x00000100
#define QDMA_TRQ_SEL_GLBL_BASE 0x00000200
#define QDMA_TRQ_SEL_FMAP_BASE 0x00000400
#define QDMA_TRQ_SEL_IND_BASE 0x00000800
#define QDMA_TRQ_SEL_C2H_BASE 0x00000A00
#define QDMA_TRQ_SEL_IRQ_BASE 0x00000C00
#define QDMA_TRQ_SEL_H2C_BASE 0x00000E00
#define QDMA_TRQ_SEL_C2H_MM0_BASE 0x00001000
#define QDMA_TRQ_SEL_C2H_MM1_BASE 0x00001100
#define QDMA_TRQ_SEL_H2C_MM0_BASE 0x00001200
#define QDMA_TRQ_SEL_H2C_MM1_BASE 0x00001300

#define QDMA_TRQ_MSIX_PBA 0x00014000
#define QDMA_TRQ_MSIX_TABLE 0x00010000
#define QDMA_TRQ_EXT_BASE 0x00002400
#define QDMA_TRQ_SEL_QUEUE_PF_BASE 0x00018000

/* Register locations */
#define QDMA_GLBL2_PF_BARLITE_EXT 0x0000010C
#define QDMA_GLBL2_CHANNEL_FUNC_RET 0x0000012C

/* masks */
#define MASK_QDMA_ID 0xFFF00000ULL
#define VAL_QDMA_ID 0x1FD00000ULL
#define BAR_LITE_MASK 0x3F

#define PF_BAR_MAP_SIZE 6

/* ---------- struct definitions ---------- */

#define QDMA_GLBL_NUM_RESOURCES 16

typedef struct {
    UINT32 u64Reserved1;
    UINT32 pu32RingSize[QDMA_GLBL_NUM_RESOURCES]; /* bits [15:0] */
    UINT32 u32Scratch;
    UINT32 u32GlobalErrorStatus;
    UINT32 u32GlobalErrorMask;

    /* writeback accumulation 2^(val+1); max=256; disable via queue */
    UINT32 u32WritebackAccumulation;
    UINT32 u32DescErrorStatus;
    UINT32 u32DescErrorMask;
    UINT32 u32DescErrorLog0;
    UINT32 u32DescErrorLog1;
    UINT32 u32TrqErrorStatus;   /* Transmit request error status */
    UINT32 u32TrqErrorMask;     /* Transmit request error mask */
    UINT32 u32TrqErrorLog;      /* Transmit request error log */
    UINT32 u32DescDbgData0;
    UINT32 u32DescDbgData1;
} QDMA_GLOBAL_REG;

typedef struct {
    UINT32 writebackCidx : 16;
    UINT32 countIdx : 4;
    UINT32 timerId : 4;
    UINT32 triggerMode : 3;
    UINT32 statusDescEnable : 1;
    UINT32 interruptEnable : 1;
    UINT32 reserved_0 : 3;
} CSR_WB_BITS;

union csr_wb_reg {
    UINT32 u32;
    CSR_WB_BITS bits;
};

typedef struct {
    UINT32 pidx : 16;
    UINT32 fIrqEnabled : 1;
    UINT32 reserved1 : 15;
} CSR_DESC_PIDX_BITS;

typedef union {
    UINT32 u32;
    CSR_DESC_PIDX_BITS bits;
} CSR_DESC_PIDX;

typedef enum {
    bytes_8 = 0,
    bytes_16 = 1,
    bytes_32 = 2,
    unknown = 3,
} DESC_SIZE_T;

/* ---------- descriptor structures ---------- */

typedef struct {
    UINT64 addr;

    UINT64 length : 28;
    UINT64 valid : 1;
    UINT64 isStartOfPacket : 1;
    UINT64 isEndOfPacket : 1;
    UINT64 reserved_0 : 33;

    UINT64 u64DestinationAddress;

    UINT64 u64Reserved1;
} MM_DESCRIPTOR;

/* ---------- writeback structures ---------- */
typedef struct {
    UINT64 reserved_0 : 1;
    UINT64 color : 1;
    UINT64 descError : 1;
    UINT64 u64Reserved1 : 1;
    UINT64 length : 16;
    UINT64 userDefined0 : 44;
} C2H_WB_HEADER_8B;

typedef struct {
    UINT64 reserved_0 : 1;
    UINT64 color : 1;
    UINT64 descError : 1;
    UINT64 u64Reserved1 : 1;
    UINT64 length : 16;
    UINT64 userDefined0 : 44;
    UINT64 userDefined1;
} C2H_WB_HEADER_16B;

typedef struct {
    UINT64 reserved_0 : 1;
    UINT64 color : 1;
    UINT64 descError : 1;
    UINT64 reserved_1 : 1;
    UINT64 length : 16;
    UINT64 userDefined0 : 44;
    UINT64 userDefined1;
    UINT64 userDefined2[2];
} C2H_WB_HEADER_32B;


typedef struct {
    UINT16 u16Pidx;
    UINT16 u16Cidx;
    UINT32 color : 1;
    UINT32 irqState : 2;
    UINT32 reserved : 29;
} C2H_WB_STATUS;

typedef struct {
    DWORD dwPfNumber;
    DWORD dwBdf; /* Board definition files */
    BOOL fIsMasterPf;
    QDMA_DEVICE_ATTRIBUTES deviceInfo;
} QDMA_DEVICE_CONFIGURATION;

typedef enum {
    POLLING_MODE, /* Polling mode */
    INTERRUPT_MODE, /* */
    INTERRUPT_COALESCE_MODE
} QUEUE_MODE;

typedef enum {
    QUEUE_TYPE_MEMORY_MAPPED,
    QUEUE_TYPE_STREAMING,
    QUEUE_TYPE_NONE
} QUEUE_TYPE;

typedef struct {
    DWORD dwSize;     /* including status write-back */
    DWORD dwCapacity; /* NOT including status write-back */
    WD_DMA *pDma;
    volatile struct _WB_STATUS_BASE *writebackStatus;
    volatile UINT32 u32SwIdx;   /* usage: e.g. [1] u16Pidx for MM C2H,
                                   MM H2C, ST H2C. [2] u16Cidx for ST WB */
    volatile UINT32 u32HwIdx;   /* usage: e.g. [1] u16Cidx for MM C2H, MM H2C,
                                   ST H2C. [2] u16Pidx for ST WB */
    volatile UINT32 u32Credits;
    struct {
        DWORD dwTotalAcceptedDescriptors;
        DWORD dwTotalProcessedDescriptors;
        DWORD dwTotalDropeedDescriptors;
    } stats;
} RING_BUFFER;

typedef struct {
    DWORD dwNumDescriptors;
    void *pCtx;
} REQ_CTX;

typedef struct {
    REQ_CTX *requests;
    DWORD dwCapacity;
    volatile UINT16 u16Pidx; /* producer index */
    volatile UINT16 u16Cidx; /* consumer index */
} DMA_REQUEST_TRACKER;

typedef struct {
    BOOL fIrqEnabled;
    UINT32 u32VectorId;
} LIBQDMA_QUEUE_CONFIG;

typedef struct queue {
    QUEUE_CONFIG userConfig;
    LIBQDMA_QUEUE_CONFIG libConfig;
    RING_BUFFER descriptorsRing;
    DMA_REQUEST_TRACKER requestTracker;
    HANDLE hMutex;
    BOOL fIsH2C;
} QUEUE;

typedef struct _QUEUE_PAIR {
    struct _QUEUE_PAIR *next;
    WDC_DEVICE_HANDLE hDev;
    struct _QDMA_THREAD *thread;
    QUEUE_TYPE type;
    QUEUE_STATE state;
    DWORD dwId;         /* queue index - relative to this PF */
    DWORD dwIdAbsolute; /* queue index - absolute across all PF */
    QUEUE h2cQueue;
    QUEUE c2hQueue;
} QUEUE_PAIR;

typedef struct _QDMA_THREAD {
    BOOL fTerminate;
    DWORD dwIndex;
    HANDLE hThread;
    HANDLE hOsEvent;
    DWORD dwWeight;
    HANDLE hMutex;
    QUEUE_PAIR *queueListHead;
} QDMA_THREAD;

typedef union qdma_ind_ctxt_cmd {
    UINT32 word;
    struct {
        UINT32 busy : 1;
        UINT32 sel : 4;
        UINT32 op : 2;
        UINT32 queueId : 11;
        UINT32 reserved1 : 14;
    } bits;
} QDMA_IND_CTXT_CMD;

typedef struct _WB_STATUS_BASE {
    UINT16 u16Pidx; /* producer index */
    UINT16 u16Cidx; /* consumer index */
} WB_STATUS_BASE;

typedef struct {
    UINT32 pu32IndirectRegistersCtx[QDMA_IND_CTX_DATA_NUM_REGS];
    UINT32 pu32IndirectRegistersMask[QDMA_IND_CTX_DATA_NUM_REGS];
    union qdma_ind_ctxt_cmd cmd;
} QDMA_INDIRECT_CTX_REGS;

typedef struct {
    DWORD dwActiveThreads;
    HANDLE hMutex;
    QDMA_THREAD threads[MAX_PROCESSORS];
} THREAD_MANAGER;

typedef struct {
    /** @cidx - consumer index */
    UINT32 cidx : 16;
    /** @creditsConsumed - credits consumed */
    UINT32 creditsConsumed : 16;
    /** @reserved1 - Reserved */
    UINT32 reserved1 : 8;
    /** @descriptorsPending - descriptors pending */
    UINT32 descriptorsPending : 1;
    /** @invalidAndNoDescPending -Queue invalid and no descriptors pending */
    UINT32 invalidAndNoDescPending : 1;
    /** @eventPending - Event pending */
    UINT32 eventPending : 1;
    /** @descriptorFetchPending -Descriptor fetch pending */
    UINT32 descriptorFetchPending : 4;
    /** @reserved2 - reserved */
    UINT32 reserved2 : 1;
} QDMA_DESCQ_HW_CTX;


/**
 * struct qdma_descq_sw_ctxt - descq SW context config data structure
 */
typedef struct {
    /** @pidx - initial producer index */
    UINT32 pidx : 16;
    /** @interruptArm - Interrupt Arm */
    UINT32 interruptArm : 1;
    /** @functionId - Function ID */
    UINT32 functionId : 8;
    /** @reserved1 - Reserved */
    UINT32 reserved1 : 7;
    /** @qEnabled - Indicates that the queue is enabled */
    UINT32 qEnabled : 1;
    /** @enableFetchCredit -Enable fetch credit */
    UINT32 enableFetchCredit : 1;
    /** @wbInterruptAfterPendingCheck -Writeback/Interrupt after pending check */
    UINT32 wbInterruptAfterPendingCheck : 1;
    /** @wbInterruptInterval -Write back/Interrupt interval */
    UINT32 wbInterruptInterval : 1;
    /** @addressTanslation - Address tanslation */
    UINT32 addressTanslation : 1;
    /** @fetchMax - Maximum number of descriptor fetches outstanding */
    UINT32 fetchMax : 3;
    /** @reserved2 - Reserved */
    UINT32 reserved2 : 4;
    /** @descriptorRingSizeIdx - Descriptor ring size index */
    UINT32 descriptorRingSizeIdx : 4;
    /** @descriptorFetchSize -Descriptor fetch size */
    UINT32 descriptorFetchSize : 2;
    /** @bypassEnabled - bypassEnabled enable */
    UINT32 bypassEnabled : 1;
    /** @mmChannel - MM channel */
    UINT32 mmChannel : 1;
    /** @wbEnabled -Writeback enable */
    UINT32 wbEnabled : 1;
    /** @fIrqEnabled -Interrupt enable */
    UINT32 fIrqEnabled : 1;
    /** @portId -Port_id */
    UINT32 portId : 3;
    /** @irqNoLast - No interrupt was sent */
    UINT32 irqNoLast : 1;
    /** @error - Error status */
    UINT32 error : 2;
    /** @errorWbSent -writeback/interrupt was sent for an error */
    UINT32 errorWbSent : 1;
    /** @irqReq - Interrupt due to error waiting to be sent */
    UINT32 irqReq : 1;
    /** @markerDisabled - Marker disable */
    UINT32 markerDisabled : 1;
    /** @isMm - MM mode */
    UINT32 isMm : 1;
    /** @ringBaseAddress - ring base address */
    UINT64 ringBaseAddress;
    /** @vector - vector number */
    UINT32 vector : 11;
    /** @interruptAggregationEnabled - interrupt aggregation enable */
    UINT32 interruptAggregationEnabled : 1;
} QDMA_DESCQ_SW_CTX;

typedef struct {
    /** @enableStatusDesc - Enable Completion Status writes */
    UINT32 enableStatusDesc : 1;
    /** @enableInterrupts - Enable Completion interrupts */
    UINT32 enableInterrupts : 1;
    /** @triggerMode - Interrupt and Completion Status Write Trigger Mode */
    UINT32 triggerMode : 3;
    /** @functionId - Function ID */
    UINT32 functionId : 8;
    /** @reserved1 - Reserved */
    UINT32 reserved1 : 4;
    /** @counterId - Index to counter register */
    UINT32 counterId : 4;
    /** @timerId - Index to timer register */
    UINT32 timerId : 4;
    /** @interruptState - Interrupt State */
    UINT32 interruptState : 2;
    /** @color - initial color bit to be used on Completion */
    UINT32 color : 1;
    /** @ringszId - Completion ring size index to ring size registers */
    UINT32 ringszId : 4;
    /** @reserved2 - Reserved */
    UINT32 reserved2 : 6;
    /** @baseAddress - completion ring base address */
    UINT64 baseAddress;
    /** @descriptorFetchSize  -descriptor size */
    UINT32 descriptorFetchSize : 2;
    /** @pidx_l - producer index low */
    UINT32 pidx : 16;
    /** @cidx - consumer index */
    UINT32 cidx : 16;
    /** @valid  - context valid */
    UINT32 valid : 1;
    /** @error - error status */
    UINT32 error : 2;
    /**
     * @userTrigPending - user logic initiated interrupt is
     * pending to be generate
     */
    UINT32 userTrigPending : 1;
    /** @timerRunning - timer is running on this queue */
    UINT32 timerRunning : 1;
    /** @fullUpdate - Full update */
    UINT32 fullUpdate : 1;
    /** @overflowCheckDisabled - Completion Ring Overflow Check Disable */
    UINT32 overflowCheckDisabled : 1;
    /** @addressTanslation -Address Translation */
    UINT32 addressTanslation : 1;
    /** @vector - Interrupt Vector */
    UINT32 vector : 11;
    /** @interruptAggregration -Interrupt Aggregration */
    UINT32 interruptAggregration : 1;
    /** @reserved3 - Reserved */
    UINT32 reserved3 : 17;
} QDMA_DESCQ_COMPLETION_CTX;

/* PCI device information struct */
typedef struct {
    WDC_DEVICE_HANDLE hDev; /* Device handle */
    QDMA_EVENT_HANDLER funcDiagEventHandler; /* Event handler routine */
    DWORD dwFunctionId; /* Function id */
    DWORD dwConfigBarNum; /* Config bar bar number */
    DWORD dwUserBarNum; /* User bar bar number */
    HANDLE hRegisterAccessMutex; /* Register access mutex handle */
    QDMA_DEVICE_CONFIGURATION deviceConf; /* Device configuration structure */
    QUEUE_MODE mode; /* Queue mode */
    HANDLE hQueueAccessMutex; /* Queue access mutex handle */
    QUEUE_PAIR queuePairs[QDMA_MAX_QUEUES_PER_PF]; /* Queue pairs array */
    THREAD_MANAGER threadManager; /* Thread manager structure */
} QDMA_DEV_CTX, *PQDMA_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */

typedef enum ClearType {
    CLEAR_TYPE_HW,
    CLEAR_TYPE_SW,
    CLEAR_TYPE_CREDIT,
    CLEAR_TYPE_MAX,
} CLEAR_TYPE;

void ErrLog(const CHAR *sFormat, ...);
void TraceLog(const CHAR *sFormat, ...);

#endif /* ifndef QDMA_REG_H__ */



