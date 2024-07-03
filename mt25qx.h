#ifndef __EBI_MT25Qx_H
#define __EBI_MT25Qx_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdbool.h>

#define __EBI_MT25Qx_PAGE_SIZE 256U

typedef enum { 
    MROkay, 
    MRFail, 
    MRBusy, 
    MRIdle 
} mt25qxRet_e;

typedef enum { 
    MWA0Wire, 
    MWA1Wire, 
    MWA2Wire, 
    MWA4Wire 
} mt25qxWireAmount_e;

typedef enum { 
    MSMQuadSpi, 
    MSMDualSpi, 
    MSMStandardSpi 
} mt25qxSpiMode_e;

typedef enum { 
    MRStatusReg,
    MRFlagStatusReg,
    MRUnknownReg
} mt25qxReg_e;

typedef enum { 
    MES4KB, 
    MES32KB, 
    MESBulk 
} mt25qxEraseSize_e;

typedef enum {
    MPCCCClearFlagStatusReg = 0x50,
    MPCCCSuspend = 0x75,
    MPCCCResume = 0x7A,
    MPCCCWriteEnable = 0x06,
    MPCCCWriteDisable = 0x04,
    MPCCCUnknownCmd
} mt25qxPureCfgCmdCode_e;

typedef struct mt25qx_s mt25qx_s;

typedef struct {

    struct {
        unsigned char nVal;
        mt25qxWireAmount_e eWireAmount;
    } sCode;

    struct {
        unsigned int nVal;
        mt25qxWireAmount_e eWireAmount;
    } sAddr;

    struct {
        size_t zDataLen;
        mt25qxWireAmount_e eWireAmount;
    } sData;

    bool bIs4BytesAddrMode;
    unsigned char nDummyClkCycles;

} mt25qxCfgCmd_s;

typedef struct {
    unsigned char nManufacturer;
    unsigned char nDevType; // ? 0xBA: 3V, 0xBB: 1.8V
    unsigned char nDevSize; // ? 0x22: 2Gb, 0x21: 1Gb, 0x20: 512Mb, 0x19: 256Mb, 0x18: 128Mb, 0x17: 64Mb
    unsigned char nUnique[17];
} mt25qxId_s;

typedef struct {

    mt25qxReg_e eReg;

    union {
        
        struct {
            unsigned char nStatusRegWritable: 1; // ? 0: writable (default), 1: disabled
            unsigned char nBlockProtectionH: 1; // ? BP[3]
            unsigned char nBlockProtectionDir: 1; // ? 0: top (default), 1: bottom
            unsigned char nBlockProtectionL: 3; // ? BP[2:0]
            unsigned char nWriteEnableLatch: 1; // ? 0: clear (default), 1: set
            unsigned char nWriteInProgress: 1; // ? 0: ready (default), 1: busy => is the inverse of sFlagStatusReg bit 7.
        } sStatusReg;

        struct {
            unsigned char nProgramOrEraseStatus: 1; // ? 0: busy, 1: ready
            unsigned char nEraseSuspend: 1; // ? 0: clear, 1: suspend
            unsigned char nEraseRet: 1; // ? 0: clear, 1: failure or protection error
            unsigned char nProgramRet: 1; // ? 0: clear, 1: failure or protection error
            unsigned char nRes: 1; // ? reserved
            unsigned char nProgramSuspend: 1; // ? 0: clear, 1: suspend
            unsigned char nProtection: 1; // ? 0: clear, 1: failure or protection error => indicates a protection error operation
            unsigned char nAddrMode: 1; // ? 0: 3-byte mode, 1: 4-byte mode
        } sFlagStatusReg;

    } uReg;
    
} mt25qxReg_s;

/**
 * @brief callback function: send configuration commands 
 */
typedef mt25qxRet_e (*mt25qxCfgCmd_f)(const mt25qxCfgCmd_s * const cpcsCfgCmd);

/**
 * @brief callback function: receive N bytes data
 */
typedef mt25qxRet_e (*mt25qxRxData_f)(unsigned char * const cpnDataBuf, const size_t czDataLen);

/**
 * @brief callback function: transmit N bytes data
 */
typedef mt25qxRet_e (*mt25qxTxData_f)(const unsigned char * const cpcnDataBuf, const size_t czDataLen);

/**
 * @brief callback function: sleep for timing
 */
typedef void (*mt25qxSleepMs_f)(unsigned int nMs);

/**
 * @brief make a mt25qx_s instance via dynamic memory
 * @param ceSpiMode set this instance to run in what kind of spi mode
 * @param cfCfgCmd callback function to send configuration commands for this instance
 * @param cfRxData callback function to rx data for this instance
 * @param cfTxData callback function to tx data for this instance
 * @param cfSleep callback function to make thread sleep if need to waiting for this instance
 * @return pointer to this instance
 */
mt25qx_s * 
mt25qxMake(
    const mt25qxSpiMode_e ceSpiMode, 
    const mt25qxCfgCmd_f cfCfgCmd, 
    const mt25qxRxData_f cfRxData, 
    const mt25qxTxData_f cfTxData,
    const mt25qxSleepMs_f cfSleep
);

/**
 * @brief free dynamic memory
 * @param cpsThis pointer to this instance
 */
void 
mt25qxFree(
    void * pvThis
);

/**
 * @brief waiting for this idle
 * @param cpsThis pointer to this instance
 * @param nTimeoutMs timeout in milliseconds
 * @return MRIdle, MRBusy, MRFail 
 * @details
 * - this function will let thread sleep, do not use it in interrupt status
 */
mt25qxRet_e
mt25qxWaitIdle(
    mt25qx_s * const cpsThis,
    const unsigned int nTimeoutMs
);

/**
 * @brief getting device ID information
 * @param cpsThis pointer to this instance
 * @param cpsId pointer to store the received device ID information
 * @return MROkay, MRFail
 * @details
 * - if the returned value is not MROkay, the value in cpsId is not available
 */
mt25qxRet_e 
mt25qxGetId(
    mt25qx_s * const cpsThis, 
    mt25qxId_s * const cpsId
);

/**
 * @brief getting the register value depends on the cpsReg
 * @param cpsThis pointer to this instance
 * @param cpsReg pointer to store the received register value
 * @return MROkay, MRFail
 * @details
 * - if the returned value is not MROkay, the value in cpsReg is not available
 */
mt25qxRet_e 
mt25qxGetReg(
    mt25qx_s * const cpsThis, 
    mt25qxReg_s * const cpsReg
);

/**
 * @brief setting the register value depends on the cpsReg
 * @param cpsThis pointer to this instance
 * @param cpcsReg pointer to configuration register value
 * @return MROkay, MRFail
 */
mt25qxRet_e 
mt25qxSetReg(
    mt25qx_s * const cpsThis, 
    const mt25qxReg_s * const cpcsReg
);

/**
 * @brief Standard/Dual/Quad SPI fast read (1-1-1, 1-1-2, 1-1-4 modes)
 * @param cpsThis pointer to this instance
 * @param cnAddr 0x00000000 to end of flash size
 * @param cpnDataBuf store data to be read
 * @param czDataLen would like to receive length
 * @return MROkay, MRFail
 * @warning
 * - czDataLen also needs to consider to ( length of cpnDataBuf ) and ( boundary of this flash )
 */
mt25qxRet_e 
mt25qxFastRead(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    unsigned char * const cpnDataBuf, 
    const size_t czDataLen
);

/**
 * @brief Standard/Dual/Quad SPI page program (1-1-1, 1-1-2, 1-1-4 modes)
 * @param cpsThis pointer to this instance
 * @param cnAddr 0x00000000 + ( N * __EBI_MT25Qx_PAGE_SIZE ) to end of flash size
 * @param cpnDataBuf data to be written
 * @param czDataLen 1 to __EBI_MT25Qx_PAGE_SIZE
 * @return MROkay, MRFail
 * @details 
 * - return MRFail if cnAddr is not end of 0x00
 * - return MROkay if "czDataLen" equals to 0
 * - auto cut off data more than __EBI_MT25Qx_PAGE_SIZE bytes
 * @warning
 * - mt25qxTxPureCfgCmd(cpsThis, MPCCCWriteEnable) is required
 * - needs to be erased if the program location has been written
 */
mt25qxRet_e 
mt25qxPageProgram(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    const unsigned char * const cpcnDataBuf, 
    const size_t czDataLen
);

/**
 * @brief erase operation
 * @param cpsThis pointer to this instance
 * @param cnAddr 0x00000000 to end of flash size
 * @param ceSize 4KB, 32KB, or all = 512Mb = 64MB
 * @return MROkay, MRFail
 * @details
 * - user can call mt25qxGetReg() to check sFlagStatusReg.nEraseRet bit if it returned "MRFail"
 * @warning
 * - mt25qxTxPureCfgCmd(cpsThis, MPCCCWriteEnable) is required
 * - this function will sleep this thread depends on "ceSize"
 *   > 4KB: 50ms
 *   > 32KB: 100ms
 *   > all: 153s
 */
mt25qxRet_e 
mt25qxErase(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    const mt25qxEraseSize_e ceSize
);

/**
 * @brief check if flash is still being page programmed or erased
 * @param cpsThis pointer to this instance
 * @return MRIdle, MRBusy, MRFail 
 * @details
 * - this function does not put the thread to sleep, so it can be used in interrupts
 * @warning
 * - not thread safe
 */
mt25qxRet_e
mt25qxChkBusy(
    mt25qx_s * const cpsThis
);

/**
 * @brief sending pure configuration commands
 * @param cpsThis pointer to this instance
 * @param ceCode operations
 * @return MROkay, MRFail
 * @details
 * - user can check register value to monitor flash state
 */
mt25qxRet_e
mt25qxTxPureCfgCmd(
    mt25qx_s * const cpsThis,
    const mt25qxPureCfgCmdCode_e ceCode
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EBI_MT25Qx_H */
