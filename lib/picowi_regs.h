// CYW43xxx register definitions

// SPI bus config registers 
#define SPI_BUS_CONTROL_REG             ((uint32_t)0x0000)  // SPI_BUS_CONTROL
#define SPI_RESPONSE_DELAY_REG          ((uint32_t)0x0001)  // SPI_RESPONSE_DELAY
#define SPI_STATUS_ENABLE_REG           ((uint32_t)0x0002)  // SPI_STATUS_ENABLE
#define SPI_RESET_BP_REG                ((uint32_t)0x0003)  // SPI_RESET_BP
#define SPI_INTERRUPT_REG               ((uint32_t)0x0004)  // SPI_INTERRUPT_REGISTER
#define SPI_INTERRUPT_ENABLE_REG        ((uint32_t)0x0006)  // SPI_INTERRUPT_ENABLE_REGISTER
#define SPI_STATUS_REG                  ((uint32_t)0x0008)  // SPI_STATUS_REGISTER
#define SPI_READ_TEST_REG               ((uint32_t)0x0014)  // SPI_READ_TEST_REGISTER
#define SPI_RESP_DELAY_F0_REG           ((uint32_t)0x001c)  // SPI_RESP_DELAY_F0
#define SPI_RESP_DELAY_F1_REG           ((uint32_t)0x001d)  // SPI_RESP_DELAY_F1
#define SPI_RESP_DELAY_F2_REG           ((uint32_t)0x001e)  // SPI_RESP_DELAY_F2
#define SPI_RESP_DELAY_F3_REG           ((uint32_t)0x001f)  // SPI_RESP_DELAY_F3

// SDIO bus config registers
#define BUS_CONTROL             0x000   // SPI_BUS_CONTROL
#define BUS_IOEN_REG            0x002   // SDIOD_CCCR_IOEN          I/O enable
#define BUS_IORDY_REG           0x003   // SDIOD_CCCR_IORDY         Ready indication
#define BUS_INTEN_REG           0x004   // SDIOD_CCCR_INTEN
#define BUS_INTPEND_REG         0x005   // SDIOD_CCCR_INTPEND
#define BUS_BI_CTRL_REG         0x007   // SDIOD_CCCR_BICTRL        Bus interface control
#define BUS_SPI_STATUS_REG      0x008   // SPI_STATUS_REGISTER
#define BUS_SPEED_CTRL_REG      0x013   // SDIOD_CCCR_SPEED_CONTROL Bus speed control  
#define BUS_BRCM_CARDCAP_REG    0x0f0   // SDIOD_CCCR_BRCM_CARDCAP
#define BUS_BAK_BLKSIZE_REG     0x110   // SDIOD_CCCR_F1BLKSIZE_0   Backplane blocksize 
#define BUS_RAD_BLKSIZE_REG     0x210   // SDIOD_CCCR_F2BLKSIZE_0   WiFi radio blocksize

// Backplane config registers
#define BAK_WIN_ADDR_REG        0x1000a // SDIO_BACKPLANE_ADDRESS_LOW Window addr 
#define SPI_FRAME_CONTROL       0x1000d
#define BAK_CHIP_CLOCK_CSR_REG  0x1000e // SDIO_CHIP_CLOCK_CSR      Chip clock ctrl 
#define BAK_PULLUP_REG          0x1000f // SDIO_PULL_UP             Pullups
#define BAK_WAKEUP_REG          0x1001e // SDIO_WAKEUP_CTRL
#define BAK_SLEEP_CSR_REG       0x1001f

// Silicon backplane
#define BAK_BASE_ADDR           0x18000000              // CHIPCOMMON_BASE_ADDRESS
#define BAK_GPIOOUT_REG         (BAK_BASE_ADDR + 0x64)
#define BAK_GPIOOUTEN_REG       (BAK_BASE_ADDR + 0x68)
#define BAK_GPIOCTRL_REG        (BAK_BASE_ADDR + 0x68)
                                                        //
#define MAC_BASE_ADDR           (BAK_BASE_ADDR+0x1000)  // DOT11MAC_BASE_ADDRESS
#define MAC_BASE_WRAP           (MAC_BASE_ADDR+0x100000)
#define MAC_IOCTRL_REG          (MAC_BASE_WRAP+0x408)   // +AI_IOCTRL_OFFSET
#define MAC_RESETCTRL_REG       (MAC_BASE_WRAP+0x800)   // +AI_RESETCTRL_OFFSET
#define MAC_RESETSTATUS_REG     (MAC_BASE_WRAP+0x804)   // +AI_RESETSTATUS_OFFSET

#define SB_BASE_ADDR            (BAK_BASE_ADDR+0x2000)  // SDIO_BASE_ADDRESS
#define SB_INT_STATUS_REG       (SB_BASE_ADDR +0x20)    // SDIO_INT_STATUS
#define SB_INT_HOST_MASK_REG    (SB_BASE_ADDR +0x24)    // SDIO_INT_HOST_MASK
#define SB_FUNC_INT_MASK_REG    (SB_BASE_ADDR +0x34)    // SDIO_FUNCTION_INT_MASK
#define SB_TO_SB_MBOX_REG       (SB_BASE_ADDR +0x40)    // SDIO_TO_SB_MAILBOX
#define SB_TO_SB_MBOX_DATA_REG  (SB_BASE_ADDR +0x48)    // SDIO_TO_SB_MAILBOX_DATA
#define SB_TO_HOST_MBOX_DATA_REG (SB_BASE_ADDR+0x4C)    // SDIO_TO_HOST_MAILBOX_DATA

#define ARM_BASE_ADDR           (BAK_BASE_ADDR+0x3000)  // WLAN_ARMCM3_BASE_ADDRESS
#define ARM_BASE_WRAP           (ARM_BASE_ADDR+0x100000)
#define ARM_IOCTRL_REG          (ARM_BASE_WRAP+0x408)   // +AI_IOCTRL_OFFSET
#define ARM_RESETCTRL_REG       (ARM_BASE_WRAP+0x800)   // +AI_RESETCTRL_OFFSET
#define ARM_RESETSTATUS_REG     (ARM_BASE_WRAP+0x804)   // +AI_RESETSTATUS_OFFSET

#define SRAM_BASE_ADDR          (BAK_BASE_ADDR+0x4000)  // SOCSRAM_BASE_ADDRESS
#define SRAM_BANKX_IDX_REG      (SRAM_BASE_ADDR+0x10)   // SOCSRAM_BANKX_INDEX
#define SRAM_BANKX_PDA_REG      (SRAM_BASE_ADDR+0x44)   // SOCSRAM_BANKX_PDA
#define SRAM_BASE_WRAP          (SRAM_BASE_ADDR+0x100000)
#define SRAM_IOCTRL_REG         (SRAM_BASE_WRAP+0x408)  // +AI_IOCTRL_OFFSET
#define SRAM_RESETCTRL_REG      (SRAM_BASE_WRAP+0x800)  // +AI_RESETCTRL_OFFSET
#define SRAM_RESETSTATUS_REG    (SRAM_BASE_WRAP+0x804)  // +AI_RESETSTATUS_OFFSET

// Save-restore
#define SR_CONTROL1             (BAK_BASE_ADDR+0x508)   // CHIPCOMMON_SR_CONTROL1

// Backplane window
#define SB_32BIT_WIN    0x8000
#define SB_ADDR_MASK    0x7fff
#define SB_WIN_MASK     (~SB_ADDR_MASK)

// Cores
#define ARM_CORE_ADDR       (ARM_BASE_ADDR | 0x100000)
#define RAM_CORE_ADDR       (SRAM_BASE_ADDR | 0x100000)

#define SDIO_INT_HOST_MASK  (SB_BASE_ADDR + 0x24)

#define SDIO_FUNCTION2_WATERMARK    0x10008

#define AI_IOCTRL_OSET      0x408
#define AI_RESETCTRL_OSET   0x800
#define AI_RESETSTATUS_OSET 0x804

// SPI_INTERRUPT_REG (reg 4) values
#define SPI_INT_DATA_UNAVAILABLE        0x0001 /* Requested data not available; Clear by writing a "1" */
#define SPI_INT_F2_F3_FIFO_RD_UNDERFLOW 0x0002
#define SPI_INT_F2_F3_FIFO_WR_OVERFLOW  0x0004
#define SPI_INT_COMMAND_ERROR           0x0008 /* Cleared by writing 1 */
#define SPI_INT_DATA_ERROR              0x0010 /* Cleared by writing 1 */
#define SPI_INT_F2_PACKET_AVAILABLE     0x0020
#define SPI_INT_F3_PACKET_AVAILABLE     0x0040
#define SPI_INT_F1_OVERFLOW             0x0080 /* Due to last write. Bkplane has pending write requests */
#define SPI_INT_MISC_INTR0              0x0100
#define SPI_INT_MISC_INTR1              0x0200
#define SPI_INT_MISC_INTR2              0x0400
#define SPI_INT_MISC_INTR3              0x0800
#define SPI_INT_MISC_INTR4              0x1000
#define SPI_INT_F1_INTR                 0x2000
#define SPI_INT_F2_INTR                 0x4000
#define SPI_INT_F3_INTR                 0x8000

// SPI_STATUS_REG (reg 8) values
#define SPI_STATUS_DATA_NOT_AVAIL       0x0001
#define SPI_STATUS_UNDERFLOW            0x0002
#define SPI_STATUS_OVERFLOW             0x0004
#define SPI_STATUS_F2_INTR              0x0008
#define SPI_STATUS_F3_INTR              0x0010
#define SPI_STATUS_F2_RX_READY          0x0020
#define SPI_STATUS_F3_RX_READY          0x0040
#define SPI_STATUS_HOST_CMD_DATA_ERR    0x0080
#define SPI_STATUS_PKT_AVAIL            0x0100
#define SPI_STATUS_LEN_SHIFT            9
#define SPI_STATUS_LEN_MASK             0x7ff
#define STATUS_F3_PKT_AVAILABLE         0x00100000
#define STATUS_F3_PKT_LEN_SHIFT         21

// EOF
