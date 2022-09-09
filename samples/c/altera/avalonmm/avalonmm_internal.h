/* Jungo Connectivity Confidential. Copyright (c) 2022 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef AVALON_REG_H__
#define AVALON_REG_H__

#include "wdc_defs.h"
#include "wdc_lib.h"
#include "avalonmm_lib.h"

/*************************************************************
  Internal definitions
 *************************************************************/

#define AVALONMM_MAX_DESCRIPTORS 128
#define AVALONMM_TRANSACTION_MAX_TRANSFER_SIZE \
    (4096 * (AVALONMM_MAX_DESCRIPTORS - 1))

typedef struct {
    /* When set, the Descriptor Controller writes the Done bit for each
     * descriptor in the status table. When not set the Descriptor Controller
     * writes the Done for the final descriptor, as specified by
     * RD_DMA_LAST_PTR. In both cases, the Descriptor Controller sends a MSI to
     * the host after the completion of the last descriptor along with the
     * status update for the last descriptor. */
    UINT32 u32Done : 1;
    UINT32 u32Reserved : 31;
} AVALONMM_DESCRIPTOR_CONTROL_BITS;

typedef union {
    AVALONMM_DESCRIPTOR_CONTROL_BITS bits;
    UINT32 value;
} AVALONMM_DESCRIPTOR_CONTROL;

/* The following struct describes the registers in the internal DMA Descriptor
 * Controller. When the DMA Descriptor Controller is externally instantiated,
 * these registers are accessed through a BAR. The offsets must be added to
 * the base address for the read/write controller. When the Descriptor
 * Controller is internally instantiated these registers are accessed through
 * BAR0.
 * The read controller is at offset 0x0000.
 * The write controller is at offset 0x0100. */
typedef struct {
    UINT32 u32DescriptorTableAddrLow;
    UINT32 u32DescriptorTableAddrHigh;
    UINT32 u32DescriptorFifoAddrLow;
    UINT32 u32DescriptorFifoAddrHigh;
    UINT32 u32DmaLastPtr;
    UINT32 u32TableSize;
    AVALONMM_DESCRIPTOR_CONTROL u32Control;
} AVALONMM_DESCRIPTOR_CONTROLLER_REGISTERS;

typedef struct {
    /* The transfer size in DWORDs (UINT32s). Must be nonzero. The maximum
    * transfer size is (1 MB - 4 bytes). If the specified transfer size is less
    * than the maximum, the transfer size is the actual size entered. */
    UINT32 u32Size : 18;
    /* Specifies the Descriptor ID. Descriptor ID 0 is at the beginning of the
     * table. Descriptor ID is at the end of the table. */
    UINT32 u32Id : 7;
    UINT32 u32Reserved : 7;
} AVALONMM_CONTROL_AND_LENGTH_BITS;

typedef union {
    AVALONMM_CONTROL_AND_LENGTH_BITS bits;
    UINT32 value;
} AVALONMM_CONTROL_AND_LENGTH;

typedef struct {
    UINT32 u32SrcAddrLow;   /* Lower UINT32 of DMA source address */
    UINT32 u32SrcAddrHigh;  /* Upper UINT32 of DMA source address */
    UINT32 u32DestAddrLow;  /* Lower UINT32 of the DMA destination address */
    UINT32 u32DestAddrHigh; /* Upper UINT32 of the DMA destination address */
    AVALONMM_CONTROL_AND_LENGTH controlAndLength; /* Specifies the ID and the
												    * Size */
    UINT32 u32Reserved1;
    UINT32 u32Reserved2;
    UINT32 u32Reserved3;
} AVALONMM_DESCRIPTOR;

typedef struct {
    /* The Descriptor Controller writes a 1 to the done bit of the status
     * DWORD (UINT32) to indicate successful completion. */
    UINT32 u32Done : 1;
    UINT32 u32Reserved : 31;
} AVALONMM_DESCRIPTOR_STATUS_BITS;

typedef union {
    AVALONMM_DESCRIPTOR_STATUS_BITS bits;
    UINT32 value;
} AVALONMM_DESCRIPTOR_STATUS;

/* Each table has 128 consecutive DWORD (AVALONMM_DESCRIPTOR_STATUS) entries
 * that correspond to the 128 descriptors. */
typedef struct AVALONMM_DESCRIPTOR_TABLE {
    /* Status descriptors */
    AVALONMM_DESCRIPTOR_STATUS descriptorsStatus[AVALONMM_MAX_DESCRIPTORS];
    /* Write/Read Descriptors */
    AVALONMM_DESCRIPTOR        descriptors[AVALONMM_MAX_DESCRIPTORS];
} AVALONMM_DESCRIPTOR_TABLE;


#define AVALONMM_RXM_BAR0 AD_PCI_BAR0
#define AVALONMM_RXM_BAR4 AD_PCI_BAR2

/* Intel-Defined VSEC Registers */
#define AVALONMM_VSEC_CAPHDR_REG   0x200 /* Altera-Defined VSEC Capability
                                         * header */
#define AVALONMM_VERNEXTCAP_REG    0x202 /* Altera VSEC Version &
                                           * Next capability offset */
#define AVALONMM_VSECID_REG        0x204 /* VSEC ID Intel-Defined,
                                           * Vendor-Specific Header */
#define AVALONMM_VSECREVLEN_REG    0x206 /* VSEC Revision & Length */
#define AVALONMM_MARKER_REG        0x208 /* Intel Marker */
#define AVALONMM_JTAGDW0_REG       0x20C /* JTAG Silicon ID DW0 JTAG Silicon
                                           * ID */
#define AVALONMM_JTAGDW1_REG       0x210 /* JTAG Silicon ID DW1 JTAG Silicon
                                           * ID */
#define AVALONMM_JTAGDW2_REG       0x214 /* JTAG Silicon ID DW2 JTAG Silicon
                                           * ID */
#define AVALONMM_JTAGDW3_REG       0x218 /* JTAG Silicon ID DW3 JTAG Silicon
                                           * ID */
#define AVALONMM_BOARDTYPEID_REG   0x21C /* User Device or Board Type ID */
#define AVALONMM_CVPSTAT_REG       0x21E /* CvP Status */
#define AVALONMM_CVPMODECTRL_REG   0x220 /* CvP Mode Control */
#define AVALONMM_CVPDATA2_REG      0x224 /* CvP Data2 Register */
#define AVALONMM_CVPDATA_REG       0x228 /* CvP Data Register */
#define AVALONMM_CVPPROG_REG       0x22C /* CvP Programming Control
                                           * Register */
#define AVALONMM_UCSTATMASK_REG    0x234 /* Uncorrectable Internal Error
                                           * Status Register */
#define AVALONMM_UCMASK_REG        0x238 /* Uncorrectable Internal Error Mask
                                           * Register */
#define AVALONMM_CSTATMASK_REG     0x23C /* Correctable Internal Error Status
                                           * Register */
#define AVALONMM_CMASK_REG         0x240 /* Correctable Internal Error Mask
                                           * Register */

/* DMA Descriptor Controller Registers */

/* Note: Read and Write are from the perspective of the FPGA.
 * A read is from a PCIe address space to the FPGA Avalon-MM address space.
 * A write is to a PCIe address space from the FPGA Avalon-MM address space. */


/* Specifies the lower 32-bits of the base address of the read status and
 * descriptor table in the Root Complex memory */
#define AVALONMM_RC_READ_STATUS_DESCRIPTOR_LOW     0x0000
/* Specifies the upper 32-bits of the base address of the read status and
 * descriptor table in the Root Complex memory */
#define AVALONMM_RC_READ_STATUS_DESCRIPTOR_HIGH    0x0004
/* Specifies the lower 32 bits of the base address of the read descriptor FIFO
 * in Endpoint memory */
#define AVALONMM_EP_READ_DESCRIPTOR_FIFO_LOW       0x0008
/* Specifies the upper 32 bits of the base address of the read descriptor table
 * in Endpoint Avalon-MM memory */
#define AVALONMM_EP_READ_DESCRIPTOR_FIFO_HIGH      0x000C
/* When read, returns the ID of the last descriptor requested
 * When written, specifies the ID of the last descriptor requested */
#define AVALONMM_RD_DMA_LAST_PTR                   0x0010
/* Specifies the size of the Read descriptor table.
 * Set to (the number of descriptors - 1) */
#define AVALONMM_RD_TABLE_SIZE                     0x0014
/* When the 0th bit is set, the Descriptor Controller writes the Done bit for
 * each descriptor in the status table. When is not set, the Descriptor
 * Controller writes the Done bit for the final descriptor, as specified by
 * RD_DMA_LAST_PTR */
#define AVALONMM_RD_CONTROL                        0x0018

/* Specifies the lower 32-bits of the base address of the write status and
 * descriptor table in the Root Complex memory */
#define AVALONMM_RC_WRITE_STATUS_DESCRIPTOR_LOW    0x0100
/* Specifies the upper 32-bits of the base address of the write status and
 * descriptor table in the Root Complex memory */
#define AVALONMM_RC_WRITE_STATUS_DESCRIPTOR_HIGH   0x00104
/* Specifies the lower 32 bits of the base address of the write descriptor FIFO
 * in Endpoint memory */
#define AVALONMM_EP_WRITE_DESCRIPTOR_FIFO_LOW      0x0108
/* Specifies the upper 32 bits of the base address of the write descriptor table
 * in Endpoint Avalon-MM memory */
#define AVALONMM_EP_WRITE_DESCRIPTOR_FIFO_HIGH     0x010C
/* When read, returns the ID of the last descriptor requested
 * When written, specifies the ID of the last descriptor requested */
#define AVALONMM_WR_DMA_LAST_PTR                   0x0110
/* Specifies the size of the Read descriptor table.
 * Set to (the number of descriptors - 1) */
#define AVALONMM_WR_TABLE_SIZE                     0x0114
/* When the 0th bit is set, the Descriptor Controller writes the Done bit for
 * each descriptor in the status table. When is not set, the Descriptor
 * Controller writes the Done bit for the final descriptor, as specified by
 * RD_DMA_LAST_PTR */
#define AVALONMM_WR_CONTROL                        0x0118

#define AVALONMM_CONTROL_REGISTERS_TO_DEVICE_OFFSET    0x0
#define AVALONMM_CONTROL_REGISTERS_FROM_DEVICE_OFFSET  0x0100

#define AVALONMM_OFFSET_DESCRIPTORS_LOW    0x0
#define AVALONMM_OFFSET_DESCRIPTORS_HIGH   0x4
#define AVALONMM_OFFSET_FIFO_LOW           0x8
#define AVALONMM_OFFSET_FIFO_HIGH          0xC
#define AVALONMM_OFFSET_LAST_PTR           0x10
#define AVALONMM_OFFSET_TABLE_SIZE         0x14
#define AVALONMM_OFFSET_CONTROL            0x18

/* rd_dts_slave */
#define AVALONMM_ENDPOINT_READ_BASE_LOW    0x80000000
#define AVALONMM_ENDPOINT_READ_BASE_HIGH   0x0

/* wr_dts_slave */
#define AVALONMM_ENDPOINT_WRITE_BASE_LOW   0x80002000
#define AVALONMM_ENDPOINT_WRITE_BASE_HIGH  0x0

#endif /* ifndef AVALON_REG_H__ */

