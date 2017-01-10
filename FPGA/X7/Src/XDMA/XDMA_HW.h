/* $Id: */
/*******************************************************************************
** © Copyright 2012 Xilinx, Inc. All rights reserved.
** This file contains confidential and proprietary information of Xilinx, Inc. and 
** is protected under U.S. and international copyright and other intellectual property laws.
*******************************************************************************
**   ____  ____ 
**  /   /\/   / 
** /___/  \  /   Vendor: Xilinx 
** \   \   \/    
**  \   \        
**  /   /          
** /___/   /\     
** \   \  /  \
**  \___\/\___\ 
** 
*******************************************************************************
**
**  Disclaimer: 
**
**    This disclaimer is not a license and does not grant any rights to the materials 
**    distributed herewith. Except as otherwise provided in a valid license issued to you 
**    by Xilinx, and to the maximum extent permitted by applicable law: 
**    (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
**    AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
**    INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
**    FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
**    or tort, including negligence, or under any other theory of liability) for any loss or damage 
**    of any kind or nature related to, arising under or in connection with these materials, 
**    including for any direct, or any indirect, special, incidental, or consequential loss 
**    or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
**    as a result of any action brought by a third party) even if such damage or loss was 
**    reasonably foreseeable or Xilinx had been advised of the possibility of the same.
**
**  Critical Applications:
**
**    Xilinx products are not designed or intended to be fail-safe, or for use in any application 
**    requiring fail-safe performance, such as life-support or safety devices or systems, 
**    Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
**    or any other applications that could lead to death, personal injury, or severe property or 
**    environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
**    the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
**    to applicable laws and regulations governing limitations on product liability.
**
**  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.
**
*******************************************************************************/
/*++

Module Name:

    XDMA_HW.h

Abstract:

    This file defines all the Xilinx Registers.

Environment:

    Kernel mode
 
    NOTE: These definitions are for memory-mapped register access only.
--*/

#ifndef __XDMA_HW_H_
#define __XDMA_HW_H_

#pragma pack(push,1)

/**************************** Type Definitions *******************************/

// Hardware required fixed parameters, i.e. do not modify unless hardware changes.

// Number of DMA channels supported by Xilinx NWL DMA Back End
#define MAX_NUMBER_DMA_ENGINES              64
// BAR Number that the DMA Engine Registers are located
#define DMA_ENGINE_BAR                      0

// Maximum number of BARs 
#define MAX_BARS                6       

#define	BASIC_TRD_DEVICE_ID				0x7042
#define	CONNECTIVITY_TRD_DEVICE_ID		0x7082
#define	VC709_TRD_DEVICE_ID				0x7083


#define DMA_ENGINE_REGISTER_BLOCK_SIZE  0x100   /**< Separation between engine regs */
#define DMA_COMMON_REGISTER_BLOCK_SIZE  0x4000  /**< Size of the Common Register space */
#define	USER_SPACE_RESERVED_SIZE		0x2000

#define DESCRIPTOR_RESERVED_SIZE		64		/**< Alignment requirement for DMA Descriptors */

// DMA Descriptor specific defines
#define DMA_DESC_BYTE_COUNT_MASK        0x000FFFFF
#define DMA_DESC_COMPLETE               1 << 24
#define DMA_DESC_IRQ_COMPLETE           1 << 24
#define DMA_DESC_SHORT                  1 << 25
#define DMA_DESC_IRQ_ERROR              1 << 25
#define DMA_DESC_C2S_USER_STATUS_LO     1 << 26
#define DMA_DESC_C2S_USER_STATUS_HI     1 << 27
#define DMA_DESC_ERROR                  1 << 28
#define DMA_DESC_EOP                    1 << 30
#define DMA_DESC_SOP                    1 << 31

#pragma warning(disable:4214)  // Bit fields other than int warning

/** @name System to Card (S2C) DMA Descriptor Structure
  */
typedef struct  _S2C_DMA_DESCRIPTOR {
    union {
        unsigned long ULONG_REG;
        struct { /*  Bit  Access */
            unsigned long     BYTE_COUNT:20;    // Bits 0 - 19
            unsigned long       :4;             // Bits 20 - 23
            unsigned long     CMP:1;            // Bit 24
            unsigned long     SHT:1;            // Bit 25
            unsigned long       :2;             // Bits 26, 27
            unsigned long     ERR:1;            // Bit 28
            unsigned long       :3;             // Bits 29 - 31
        } BIT;
    } volatile STATUS_FLAG_BYTE_COUNT;
    union {
        unsigned long long ULONG64;
        struct {
            unsigned long LOW;
            unsigned long HIGH;
        } ULONG_REG;
    } USER_CONTROL;
    unsigned long CARD_ADDRESS;
    union {
        unsigned long ULONG_REG;
        struct { /*  Bit  Access */
            unsigned long     BYTE_COUNT:20;    // Bits 0 -19
            unsigned long       :4;             // Bits 20 - 23
            unsigned long     IRQC:1;           // Bit 24
            unsigned long     IRQER:1;          // Bit 25
            unsigned long       :4;             // Bits 26 - 29
            unsigned long     EOP:1;            // Bit 30
            unsigned long     SOP:1;            // Bit 31
        } BIT;
    } CONTROL_BYTE_COUNT;
    union {
        unsigned long long ULONG64;
        struct {
            unsigned long LOW;
            unsigned long HIGH;
        } ULONG_REG;
    } SYSTEM_ADDRESS_PHYS;
    unsigned long   NEXT_DESC_PHYS_PTR;
} S2C_DMA_DESCRIPTOR, *PS2C_DMA_DESCRIPTOR;

/** @name Card to System (C2S) DMA Descriptor Structure
  */
typedef struct  _C2S_DMA_DESCRIPTOR {
    union {
        unsigned long ULONG_REG;
        struct { /*  Bit  Access */
            unsigned long     BYTE_COUNT:20;    // Bits 0 - 19      
            unsigned long       :4;             // Bits 20 - 23
            unsigned long     CMP:1;            // Bit 24
            unsigned long     SHT:1;            // Bit 25
            unsigned long     L0:1;             // Bit 26
            unsigned long     HI0:1;            // Bit 27
            unsigned long     ERR:1;            // Bit 28
            unsigned long       :1;             // Bit 29
            unsigned long     EOP:1;            // Bit 30
            unsigned long     SOP:1;            // Bit 31
        } BIT;
    } volatile STATUS_FLAG_BYTE_COUNT;
    union {
        unsigned long long ULONG64;
        struct {
            unsigned long LOW;
            unsigned long HIGH;
        } ULONG_REG;
    } volatile USER_STATUS;
    unsigned long CARD_ADDRESS;
    union {
        unsigned long ULONG_REG;
        struct { /*  Bit  Access */
            unsigned long     BYTE_COUNT:20;    // Bits 0 - 19
            unsigned long       :4;             // Bits 20 - 23
            unsigned long     IRQC:1;           // Bit 24
            unsigned long     IRQER:1;          // Bit 25
            unsigned long       :6;             // Bits 26 - 31
        } BIT;
    } CONTROL_BYTE_COUNT;
    union {
        unsigned long long ULONG64;
        struct {
            unsigned long LOW;
            unsigned long HIGH;
        } ULONG_REG;
    } SYSTEM_ADDRESS_PHYS;
    unsigned long   NEXT_DESC_PHYS_PTR;
} C2S_DMA_DESCRIPTOR, *PC2S_DMA_DESCRIPTOR;

#pragma warning(disable:4201)  // nameless struct/union warning

/** @name Unified DMA Descriptor Structure
  */
typedef struct _DMA_DESCRIPTOR
{
    union
    {
        S2C_DMA_DESCRIPTOR      S2C;
        C2S_DMA_DESCRIPTOR      C2S;
    };
    PVOID                       NEXT_DESC_VIRT_PTR;     /* Pointer to the next descriptor */
    PDATA_TRANSFER_PARAMS       XFER_PARAMS;
} DMA_DESCRIPTOR, *PDMA_DESCRIPTOR;

#pragma warning(default:4201)

/** @name Aligned DMA Descriptor Structure
  */
typedef struct _DMA_DESCRIPTOR_ALIGNED
{
	DMA_DESCRIPTOR				DESC;
	UCHAR	reserved[DESCRIPTOR_RESERVED_SIZE - sizeof(DMA_DESCRIPTOR)];
} DMA_DESCRIPTOR_ALIGNED, *PDMA_DESCRIPTOR_ALIGNED;


/** @name Bitmasks of DMA ENGINE REGISTERS, DMA Engine
 *  Control Register. @{
 */
/* Interrupt activity and acknowledgement bits */
#define DMA_ENG_INTERRUPT_ENABLE            1 << 0      /**< Enable interrupts */
#define DMA_ENG_INTERRUPT_DISABLE           0           /**< Disable interrupts */
#define DMA_ENG_INTERRUPT_ACTIVE            1 << 1      /**< Interrupt active */
#define DMA_ENG_DESCRIPTOR_COMPLETE         1 << 2      /**< Interrupt Descriptor Complete */
#define DMA_ENG_DESCRIPTOR_ALIGNMENT_ERROR  1 << 3      /**< Interupt DMA Descriptor Alignment error */
#define DMA_ENG_DESCRIPTOR_FETCH_ERROR      1 << 4      /**< Interrupt Descriptor Fetch Error */
#define DMA_ENG_SW_ABORT_ERROR              1 << 5      /**< Interrupt Software Abort error */

#define DMA_ENG_DMA_ENABLE                  1 << 8      // Start the DMA Engine
#define DMA_ENG_DMA_RUNNING                 1 << 10     // DMA Engine Running indicator
#define DMA_ENG_DMA_WAITING                 1 << 11     // DMA Engine Waiting
#define DMA_ENG_DMA_RESET_REQUEST           1 << 14     // DMA Engine Reset Request
#define DMA_ENG_DMA_RESET                   1 << 15     // DMA Engine Reset

/** @name DMA Engine control registers (one per DMA Engine) structure
  */
typedef struct _DMA_ENGINE_REGISTERS
{
    union
    {
        unsigned long   ULONG;
        struct
        {
            unsigned long     ENGINE_PRESET:1;          // Bit 0
            unsigned long     CARD_TO_SYSTEM:1;         // Bit 1
            unsigned long     :2;                       // Bit 2, 3
            unsigned long     FIFO_PACKET_ENGINE:1;     // Bit 4
            unsigned long     ADDR_PACKET_ENGINE:1;     // Bit 5
            unsigned long     :2;                       // Bits 6, 7
            unsigned long     ENGINE_NUMBER:8;          // Bit 8 - 15
            unsigned long     CARD_ADDRESS_SIZE:8;      // Bit 16 - 23
            unsigned long     DESC_MAX_BYTE_COUNT:6;    // Bit 24- 29
            unsigned long     SCALING_FACTOR:2;         // Bit 30, 31
        } BIT;
    } CAPABILITIES;
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     INTERRUPT_ENABLE:1;       // Bit 0
            unsigned long     INTERRUPT_ACTIVE:1;       // Bit 1
            unsigned long     DESCRIPTOR_COMPLETE:1;    // Bit 2     
            unsigned long     ALIGNMENT_ERROR:1;        // Bit 3
            unsigned long     FETCH_ERROR:1;            // Bit 4
            unsigned long     SW_ABORT_ERROR:1;         // Bit 5
            unsigned long     :2;                       // Bits 6, 7
            unsigned long     DMA_ENABLE:1;             // Bit 8
            unsigned long     :1;                       // Bit 9
            unsigned long     DMA_RUNNING:1;            // Bit 10
            unsigned long     DMA_WAITING:1;            // Bit 11
            unsigned long     :2;                       // Bits 12, 13
            unsigned long     DMA_RESET_REQUEST:1;      // Bit 14
            unsigned long     DMA_RESET:1;              // Bit 15
            unsigned long     :16;                      // Bit 16 - 31
        } BIT;
    } volatile CONTROL;
    unsigned long   NEXT_DESCRIPTOR_PHYS_PTR;
    unsigned long   SOFTWARE_DESCRIPTOR_PHYS_PTR;
    unsigned long   COMPLETED_DESCRIPTOR_PHYS_PTR;
    volatile unsigned long   ENGINE_ACTIVE_TIME;
	volatile unsigned long	ENGINE_WAIT_TIME;
    volatile unsigned long   COMPLETED_BYTE_COUNT;
} DMA_ENGINE_REGISTERS, *PDMA_ENGINE_REGISTERS;

#define ENGINE_STATS_MASK	0xFFFFFFFC					// Keep all but the first two bits

/** @name Packed DMA Descriptor Structure with reserved space
  */
typedef struct  _PACKED_DMA_ENGINE_REGISTERS
{
    DMA_ENGINE_REGISTERS    DMA_ENGINE_REGS;
    unsigned char   Reserved[DMA_ENGINE_REGISTER_BLOCK_SIZE - (sizeof(DMA_ENGINE_REGISTERS))];
}PACKED_DMA_ENGINE_REGISTERS, *PPACKED_DMA_ENGINE_REGISTERS;

/** @name Bitmasks of DMA COmmon Control and Status Register (offset 0x4000).
 * @{
 */
#define COMMON_GLOBAL_INTERRUPT_ENABLE  0x00000001  /**< Global Interrupt Enable */
#define COMMON_GLOBAL_INTERRUPT_DISABLE 0x00000000  /**< Global Interrupt Disable */
#define COMMON_INTERRUPT_ACTIVE         0x00000002  /**< Global Interrupt Active */
#define COMMON_INTERRUPT_PENDING        0x00000004  /**< Global Interrupt Pending */
#define COMMON_INTERRUPT_MODE           0x00000008  /**< Global Interrupt Mode, MSI or Legacy */
#define COMMON_USER_INTERRUPT_ENABLE    0x00000010  /**< User Interrupt Enable */
#define COMMON_USER_INTERRUPT_ACTIVE    0x00000020  /**< User Interrupt Active */
#define COMMON_S2C_INTERRUPT_MASK       0x00FF0000  /**< S2C DMA Engines Interupt Statuses */
#define COMMON_C2S_INTERRUPT_MASK       0xFF000000  /**< C2S DMA Engines Interupt Statuses */

#pragma warning(disable:4201)  // nameless struct/union warning

/** @name DMA Common Control Registers structure
  */
typedef struct _DMA_COMMON_CONTROL_REGISTERS
{
    union
    {
        unsigned long   ULONG;
        struct 
        {
            unsigned long     GLOBAL_INTERRUPT_ENABLE:1; // Bit 0       
            unsigned long     INTERRUPT_ACTIVE:1;       // Bit 1
            unsigned long     INTERRUPT_PENDING:1;      // Bit 2
            unsigned long     INTERRUPT_MODE:1;         // Bit 3
            unsigned long     USER_INTERRUPT_ENABLE:1;  // Bit 4      
            unsigned long     USER_INTERRUPT_ACTIVE:1;  // Bit 5      
            unsigned long     :10;                      // Bit 6 - 15
            unsigned long     S2C_INTERRUPT_STATUS:8;   // Bits 16 - 23    
            unsigned long     C2S_INTERRUPT_STATUS:8;   // Bits 24 - 31    
        } BIT;
    };
} volatile DMA_COMMON_CONTROL_REGISTERS, *PDMA_COMMON_CONTROL_REGISTERS;

#pragma warning(default:4201)
#pragma warning(default:4214)

/** @name Packed DMA Common Control Structure with reserved
 *        space.
  */
typedef struct _PACKED_DMA_COMMON_CONTROL_REGISTERS
{
    DMA_COMMON_CONTROL_REGISTERS    COMMON_CONTROL;
    unsigned char   Reserved[DMA_COMMON_REGISTER_BLOCK_SIZE - (sizeof(DMA_COMMON_CONTROL_REGISTERS))];
}PACKED_DMA_COMMON_CONTROL_REGISTERS, *PPACKED_DMA_COMMON_CONTROL_REGISTERS;



/** @name DMA Register memory map (usually located in BAR 0).
  */
typedef struct _DMA_REGISTER_MAP
{
    PACKED_DMA_ENGINE_REGISTERS DMA_ENGINE[MAX_NUMBER_DMA_ENGINES];     // Offset 0 in BAR 0
    PACKED_DMA_COMMON_CONTROL_REGISTERS     COMMON_CONTROL;             // Offset 0x4000
	USER_SPACE_MAP							USER_SPACE;					// Offset 0x8000
//	unsigned char USER_SPACE[USER_SPACE_RESERVED_SIZE];					// Offset 0x8000 - 0x8FFF
//	PACKED_DESIGN_AND_STATUS_REGISTERS      DESIGN_REGISTERS;           // Offset 0x9000
//	PACKED_APPLICATION_REGISTERS			APP0_REGISTERS;				// Offset 0x9100
//	PACKED_APPLICATION_REGISTERS			APP1_REGISTERS;				// Offset 0x9200
//	MEMORY_CONTROL_REGISTERS				MEM_CTRL_CHNL_0;			// Offset 0x9300
//	MEMORY_CONTROL_REGISTERS				MEM_CTRL_CHNL_1;			// Offset 0x9310
//	MEMORY_CONTROL_REGISTERS				MEM_CTRL_CHNL_2;			// Offset 0x9320
//	MEMORY_CONTROL_REGISTERS				MEM_CTRL_CHNL_3;			// Offset 0x9330
}DMA_REGISTER_MAP, *PDMA_REGISTER_MAP;



#pragma pack(pop)

#endif  // __XDMA_HW_H_

