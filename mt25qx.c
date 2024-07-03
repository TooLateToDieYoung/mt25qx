#include "mt25qx.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct mt25qx_s {
    mt25qxSpiMode_e eSpiMode;
    mt25qxCfgCmd_f fCfgCmd; 
    mt25qxRxData_f fRxData; 
    mt25qxTxData_f fTxData;
    mt25qxSleepMs_f fSleep;
    bool bIs4BytesAddrMode;
};

static
mt25qxRet_e
_txPureCfgCmd(
    mt25qx_s * const cpsThis,
    const unsigned char cnCode
) {
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis )
    {
        return MRFail;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sCode.nVal = cnCode;
    sCfgCmd.sAddr.eWireAmount = MWA0Wire;
    sCfgCmd.sAddr.nVal = 0;
    sCfgCmd.sData.eWireAmount = MWA0Wire;
    sCfgCmd.sData.zDataLen = 0;
    sCfgCmd.nDummyClkCycles = 0;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    return cpsThis->fCfgCmd(&sCfgCmd);
}

mt25qx_s * 
mt25qxMake(
    const mt25qxSpiMode_e ceSpiMode, 
    const mt25qxCfgCmd_f cfCfgCmd, 
    const mt25qxRxData_f cfRxData, 
    const mt25qxTxData_f cfTxData,
    const mt25qxSleepMs_f cfSleep
) {
assert( NULL != cfCfgCmd );
assert( NULL != cfRxData );
assert( NULL != cfTxData );
assert( NULL != cfSleep );

    mt25qxId_s sId = {0};
    mt25qxReg_s sReg = {0};
    mt25qx_s * const cpsThis = (mt25qx_s *)calloc(1, sizeof(mt25qx_s));
    if ( NULL == cpsThis )
    {
        goto __error;
    }
    else
    {
        cpsThis->eSpiMode = ceSpiMode;
        cpsThis->fCfgCmd = cfCfgCmd;
        cpsThis->fRxData = cfRxData;
        cpsThis->fTxData = cfTxData;
        cpsThis->fSleep = cfSleep;
    }

    if ( 
        MROkay != _txPureCfgCmd(cpsThis, 0x66) || // ? 0x66: Reset enable
        MROkay != _txPureCfgCmd(cpsThis, 0x99)    // ? 0x99: reset memory
    ) {
        goto __error;
    }

    if ( 
        MRIdle != mt25qxWaitIdle(cpsThis, 10) ||
        MROkay != mt25qxGetId(cpsThis, &sId) ||
        0x20 != sId.nManufacturer
    ) {
        goto __error;
    }

    /* Flash size is larger than 128Mb (0x18): ask to enter 4-byte addr mode */
    cpsThis->bIs4BytesAddrMode = ( 0x18 < sId.nDevSize ) ? ( true ) : ( false ) ;
    if ( true == cpsThis->bIs4BytesAddrMode ) 
    {
        sReg.eReg = MRFlagStatusReg;
        if ( 
            MRIdle != mt25qxWaitIdle(cpsThis, 10) ||
            MROkay != _txPureCfgCmd(cpsThis, 0xB7) || // ? 0xB7: enter 4-byte addr mode cmd
            MRIdle != mt25qxWaitIdle(cpsThis, 10) ||
            MROkay != mt25qxGetReg(cpsThis, &sReg) || // ? check if set up 4-byte addr mode
            1 != sReg.uReg.sFlagStatusReg.nAddrMode
        ) {
            goto __error;
        }
    }

    return cpsThis;

__error:
    mt25qxFree(cpsThis);
    return NULL;
}

void 
mt25qxFree(
    void * pvThis
) {
    free(pvThis);
}

mt25qxRet_e
mt25qxWaitIdle(
    mt25qx_s * const cpsThis,
    const unsigned int nTimeoutMs
) {
    unsigned int nTryTimes = ( 0 == nTimeoutMs ) ? ( 1 ) : ( nTimeoutMs ) ;

    if ( NULL == cpsThis )
    {
        return MRFail;
    }

    do {
        cpsThis->fSleep(1);

        switch ( mt25qxChkBusy(cpsThis) )
        {
        case MRIdle:
            return MRIdle;

        case MRFail:
            return MRFail;
        
        default:
            --nTryTimes;
            break;
        }
    } while ( nTryTimes > 0 );

    return MRBusy;
}

mt25qxRet_e 
mt25qxGetId(
    mt25qx_s * const cpsThis, 
    mt25qxId_s * const cpsId
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis || NULL == cpsId )
    {
        return MRFail;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sCode.nVal = 0x9E;
    sCfgCmd.sAddr.eWireAmount = MWA0Wire;
    sCfgCmd.sAddr.nVal = 0;
    sCfgCmd.sData.eWireAmount = MWA1Wire;
    sCfgCmd.sData.zDataLen = sizeof(mt25qxId_s);
    sCfgCmd.nDummyClkCycles = 0;

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    eRet = cpsThis->fRxData((unsigned char *)cpsId, sCfgCmd.sData.zDataLen);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    return MROkay;
}

mt25qxRet_e 
mt25qxGetReg(
    mt25qx_s * const cpsThis, 
    mt25qxReg_s * const cpsReg
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis || NULL == cpsReg )
    {
        return MRFail;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.eWireAmount = MWA0Wire;
    sCfgCmd.sAddr.nVal = 0;
    sCfgCmd.sData.eWireAmount = MWA1Wire;
    sCfgCmd.sData.zDataLen = sizeof(cpsReg->uReg);
    sCfgCmd.nDummyClkCycles = 0;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    switch ( cpsReg->eReg )
    {
    case MRStatusReg:
        sCfgCmd.sCode.nVal = 0x05;
        break;
    
    case MRFlagStatusReg:
        sCfgCmd.sCode.nVal = 0x70;
        break;

    default: 
        return MRFail;
    }

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    eRet = cpsThis->fRxData((unsigned char *)&cpsReg->uReg, sCfgCmd.sData.zDataLen);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    return MROkay;
}

mt25qxRet_e 
mt25qxSetReg(
    mt25qx_s * const cpsThis, 
    const mt25qxReg_s * const cpcsReg
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis || NULL == cpcsReg )
    {
        return MRFail;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.eWireAmount = MWA0Wire;
    sCfgCmd.sAddr.nVal = 0;
    sCfgCmd.sData.eWireAmount = MWA1Wire;
    sCfgCmd.sData.zDataLen = sizeof(cpcsReg->uReg);
    sCfgCmd.nDummyClkCycles = 0;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    switch ( cpcsReg->eReg )
    {
    case MRStatusReg:
        sCfgCmd.sCode.nVal = 0x01;
        break;

#if 0 /* Using mt25qxTxPureCfgCmd(cpsThis, MPCCCClearFlagStatusReg) */
    case MRFlagStatusReg:
        sCfgCmd.sCode.nVal = 0xFF;
        break;
#endif

    default: 
        return MRFail;
    }

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    eRet = cpsThis->fTxData((unsigned char *)&cpcsReg->uReg, sCfgCmd.sData.zDataLen);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    return MROkay;
}

mt25qxRet_e 
mt25qxFastRead(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    unsigned char * const cpnDataBuf, 
    const size_t czDataLen
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis || NULL == cpnDataBuf )
    {
        return MRFail;
    }

    if ( 0 != ( cnAddr &  0x000000FF ) )
    {
        return MRFail;
    }

    if ( 0 == czDataLen )
    {
        return MROkay;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.nVal = cnAddr;
    sCfgCmd.sData.zDataLen = czDataLen;
    sCfgCmd.nDummyClkCycles = 8;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    switch ( cpsThis->eSpiMode )
    {
    case MSMQuadSpi:
        sCfgCmd.sCode.nVal = 0x6B;
        sCfgCmd.sData.eWireAmount = MWA4Wire;
        break;

    case MSMDualSpi:
        sCfgCmd.sCode.nVal = 0x3B;
        sCfgCmd.sData.eWireAmount = MWA2Wire;
        break;

    default: 
        sCfgCmd.sCode.nVal = 0x0B;
        sCfgCmd.sData.eWireAmount = MWA1Wire;
        break;
    }

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    eRet = cpsThis->fRxData(cpnDataBuf, czDataLen);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    return MROkay;
}

mt25qxRet_e 
mt25qxPageProgram(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    const unsigned char * const cpcnDataBuf, 
    const size_t czDataLen
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};

    if ( NULL == cpsThis || NULL == cpcnDataBuf )
    {
        return MRFail;
    }

    if ( 0 == czDataLen )
    {
        return MROkay;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.nVal = cnAddr;
    sCfgCmd.sData.zDataLen = ( czDataLen > 256 ) ? ( 256 ) : ( czDataLen );
    sCfgCmd.nDummyClkCycles = 0;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    switch ( cpsThis->eSpiMode )
    {
    case MSMQuadSpi:
        sCfgCmd.sCode.nVal = 0x32;
        sCfgCmd.sData.eWireAmount = MWA4Wire;

        break;

    case MSMDualSpi:
        sCfgCmd.sCode.nVal = 0xA2;
        sCfgCmd.sData.eWireAmount = MWA2Wire;
        break;

    default: 
        sCfgCmd.sCode.nVal = 0x02;
        sCfgCmd.sData.eWireAmount = MWA1Wire;
        break;
    }

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    eRet = cpsThis->fTxData(cpcnDataBuf, czDataLen);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    // ! needs to sleep for 1ms after page programming (to solve the timing problem)
    cpsThis->fSleep(1); 
    return MROkay;
}

mt25qxRet_e 
mt25qxErase(
    mt25qx_s * const cpsThis,
    const unsigned int cnAddr,
    const mt25qxEraseSize_e ceSize
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxCfgCmd_s sCfgCmd = {0};
    size_t zTypDelayMs = 0;

    if ( NULL == cpsThis )
    {
        return MRFail;
    }

    sCfgCmd.sCode.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.eWireAmount = MWA1Wire;
    sCfgCmd.sAddr.nVal = cnAddr;
    sCfgCmd.sData.eWireAmount = MWA0Wire;
    sCfgCmd.sData.zDataLen = 0;
    sCfgCmd.nDummyClkCycles = 0;
    sCfgCmd.bIs4BytesAddrMode = cpsThis->bIs4BytesAddrMode;

    switch ( ceSize )
    {
    case MES4KB:
        sCfgCmd.sCode.nVal = 0x20;
        zTypDelayMs = 50;
        break;

    case MES32KB:
        sCfgCmd.sCode.nVal = 0x52;
        zTypDelayMs = 100;
        break;

    default: /* MESBulk */
        sCfgCmd.sCode.nVal = 0x60;
        sCfgCmd.sAddr.eWireAmount = MWA0Wire;
        sCfgCmd.sAddr.nVal = 0;
        zTypDelayMs = 153000;
        break;
    }

    eRet = cpsThis->fCfgCmd(&sCfgCmd);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    cpsThis->fSleep(zTypDelayMs);
    return MROkay;
}

mt25qxRet_e
mt25qxChkBusy(
    mt25qx_s * const cpsThis
) {
    mt25qxRet_e eRet = MROkay;
    mt25qxReg_s sReg = {0};

    if ( NULL == cpsThis )
    {
        return MRFail;
    }

    sReg.eReg = MRStatusReg;
    eRet = mt25qxGetReg(cpsThis, &sReg);
    if ( MROkay != eRet )
    {
        return eRet;
    }

    return ( 1 == sReg.uReg.sStatusReg.nWriteInProgress ) ? ( MRBusy ) : ( MRIdle ) ;
}

mt25qxRet_e
mt25qxTxPureCfgCmd(
    mt25qx_s * const cpsThis,
    const mt25qxPureCfgCmdCode_e ceCode
) {
    if ( NULL == cpsThis )
    {
        return MRFail;
    }

    switch ( ceCode )
    {
    case MPCCCClearFlagStatusReg:
    case MPCCCSuspend:
    case MPCCCResume:
    case MPCCCWriteEnable:
    case MPCCCWriteDisable:
        break;

    default:
        return MRFail;
    }

    return _txPureCfgCmd(cpsThis, ceCode);
}
