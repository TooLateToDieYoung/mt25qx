# Example: MT25QL512ABB

```c
#include <stdio.h>
#include <stddef.h>
#include "mt25qx.h"

#define MT25QL512ABB_ADDR_HEAD 0x00000000
#define MT25QL512ABB_ADDR_TAIL 0x04000000

extern mt25qxRet_e mt25qxLowLayerCfgCmd(const mt25qxCfgCmd_s *const cpcsCfgCmd);
extern mt25qxRet_e mt25qxLowLayerRxData(unsigned char *const cpnDataBuf, const size_t czDataLen);
extern mt25qxRet_e mt25qxLowLayerTxData(const unsigned char *const cpcnDataBuf, const size_t czDataLen);
extern mt25qxRet_e mt25qxLowLayerSleepMs(unsigned int nMs);

static const unsigned char cnBinaryArray[] = { /* ... */ };

int main(void)
{
    mt25qx_s * psExtQspiFlash = NULL;
    unsigned char nCnt = 0;
    unsigned int nAddr = 0;
    unsigned int nOffset = 0;
    const size_t cnBinaryArraySize = sizeof(cnBinaryArray);

    if ( MT25QL512ABB_ADDR_TAIL < cnBinaryArraySize )
    {
        return -1;
    }

    psExtQspiFlash = mt25qxMake(
        MSMQuadSpi, 
        mt25qxLowLayerCfgCmd, 
        mt25qxLowLayerRxData, 
        mt25qxLowLayerTxData, 
        mt25qxLowLayerSleepMs
    );

    if ( NULL == psExtQspiFlash )
    {
        return -2;
    }

    for ( nAddr = MT25QL512ABB_ADDR_HEAD; cnBinaryArraySize > nAddr; nAddr += ( __EBI_MT25Qx_PAGE_SIZE * 16 ) )
    {
        if (
            MROkay != mt25qxTxPureCfgCmd(psExtQspiFlash, MPCCCWriteEnable) ||
            MROkay != mt25qxErase(psExtQspiFlash, nAddr, MES4KB) || // MES4KB has 16 pages
            MRIdle != mt25qxWaitIdle(psExtQspiFlash, 400)
        ) {
            printf("> Erase Error at 0x%08X\r\n", nAddr);
            goto __exit;
        }

        for ( nCnt = 0; 16 > nCnt; ++nCnt )
        {
            nOffset = nAddr + ( __EBI_MT25Qx_PAGE_SIZE * nCnt );
            if (
                MROkay != mt25qxTxPureCfgCmd(psExtQspiFlash, MPCCCWriteEnable) ||
                MROkay != mt25qxPageProgram(psExtQspiFlash, nOffset, &cnBinaryArray[nOffset], __EBI_MT25Qx_PAGE_SIZE) ||
                MRIdle != mt25qxWaitIdle(psExtQspiFlash, 10)
            ) {
                printf("> Page Program Error at 0x%08X\r\n", nOffset);
                goto __exit;
            }
        }
    }

__exit:
    mt25qxFree(psExtQspiFlash);

    return 0;
}
```
