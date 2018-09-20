#pragma once

// https://github.com/texane/stlink

// st-link vendor cmd's
#define USB_ST_VID				0x0483
#define USB_STLINK_PID			0x3744
#define USB_STLINK_32L_PID		0x3748

// STLINK_DEBUG_RESETSYS, etc:
#define STLINK_OK					0x80
#define STLINK_FALSE				0x81
#define STLINK_CORE_RUNNING			0x80
#define STLINK_CORE_HALTED			0x81
#define STLINK_CORE_STAT_UNKNOWN	  -1

#define STLINK_GET_VERSION			0xF1
#define STLINK_GET_CURRENT_MODE		0xF5

#define STLINK_DEBUG_COMMAND		0xF2
#define STLINK_DFU_COMMAND			0xF3
#define STLINK_DFU_EXIT				0x07
//#define STLINK_DFU_ENTER			0x08 // ?

// STLINK_GET_CURRENT_MODE
#define STLINK_DEV_DFU_MODE			0x00
#define STLINK_DEV_MASS_MODE		0x01
#define STLINK_DEV_DEBUG_MODE		0x02
#define STLINK_DEV_UNKNOWN_MODE		  -1

// jtag mode cmds
#define STLINK_DEBUG_ENTER				0x20
#define STLINK_DEBUG_EXIT				0x21
#define STLINK_DEBUG_READCOREID			0x22
#define STLINK_DEBUG_GETSTATUS			0x01
#define STLINK_DEBUG_FORCEDEBUG			0x02
#define STLINK_DEBUG_RESETSYS			0x03
#define STLINK_DEBUG_READALLREGS		0x04
#define STLINK_DEBUG_READREG			0x05
#define STLINK_DEBUG_WRITEREG			0x06
#define STLINK_DEBUG_READMEM_32BIT		0x07
#define STLINK_DEBUG_WRITEMEM_32BIT		0x08
#define STLINK_DEBUG_RUNCORE			0x09
#define STLINK_DEBUG_STEPCORE			0x0A
#define STLINK_DEBUG_SETFP				0x0B
#define STLINK_DEBUG_WRITEMEM_8BIT		0x0D
#define STLINK_DEBUG_CLEARFP			0x0E
#define STLINK_DEBUG_WRITEDEBUGREG		0x0F
#define STLINK_DEBUG_ENTER_SWD			0xA3
#define STLINK_DEBUG_ENTER_JTAG			0x00

// TODO - possible poor names...
#define STLINK_SWD_ENTER				0x30
#define STLINK_SWD_READCOREID			0x32  // TBD
#define STLINK_JTAG_WRITEDEBUG_32BIT	0x35
#define STLINK_JTAG_READDEBUG_32BIT		0x36
#define STLINK_JTAG_DRIVE_NRST			0x3C

// cortex m3 technical reference manual
#define CM3_REG_CPUID		0xE000ED00
#define CM3_REG_FP_CTRL		0xE0002000
#define CM3_REG_FP_COMP0	0xE0002008

/* cortex core ids */
// TODO clean this up...
#define STM32VL_CORE_ID		0x1BA01477
#define STM32L_CORE_ID		0x2BA01477
#define STM32F4_CORE_ID		0x2BA01477
#define STM32F0_CORE_ID		0x0BB11477
#define CORE_M3_R1			0x1BA00477
#define CORE_M3_R2			0x4BA00477
#define CORE_M4_R0			0x2BA01477

/*
 * Chip IDs are explained in the appropriate programming manual for the
 * DBGMCU_IDCODE register (0xE0042000)
 */
// stm32 chipids, only lower 12 bits..
#define STM32_CHIPID_F1_MEDIUM		0x410
#define STM32_CHIPID_F2				0x411
#define STM32_CHIPID_F1_LOW			0x412
#define STM32_CHIPID_F4				0x413
#define STM32_CHIPID_F1_HIGH		0x414
#define STM32_CHIPID_L1_MEDIUM		0x416
#define STM32_CHIPID_F1_CONN		0x418
#define STM32_CHIPID_F1_VL_MEDIUM	0x420
#define STM32_CHIPID_F1_VL_HIGH		0x428
#define STM32_CHIPID_F1_XL			0x430
#define STM32_CHIPID_F0				0x440

// Constant STM32 memory map figures
#define STM32_FLASH_BASE	0x08000000
#define STM32_SRAM_BASE		0x20000000

/* Cortex™-M3 Technical Reference Manual */
/* Debug Halting Control and Status Register */
//#define DFSR		0xE000ED30
//#define DHCSR		0xE000EDF0
//#define DCRSR		0xE000EDF4
//#define DCRDR		0xE000EDF8
//#define DBGKEY	0xA05F0000
