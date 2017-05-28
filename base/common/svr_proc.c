#include <stdarg.h>
#include "common/svr_proc.h"
#include "hi_common.h"

HI_S32 SVR_MODULE_Register(HI_U32 u32ModuleID, const HI_CHAR * pszModuleName)
{
    return HI_MODULE_Register(u32ModuleID, pszModuleName);
}

HI_S32 SVR_MODULE_RegisterByName(const HI_CHAR * pszModuleName, HI_U32* pu32ModuleID)
{
    return HI_MODULE_RegisterByName(pszModuleName, pu32ModuleID);
}

HI_S32 SVR_MODULE_UnRegister(HI_U32 u32ModuleID)
{
    return HI_MODULE_UnRegister(u32ModuleID);
}

HI_S32 SVR_PROC_AddDir(const HI_CHAR *pszName)
{
    return HI_PROC_AddDir(pszName);
}

HI_S32 SVR_PROC_RemoveDir(const HI_CHAR *pszName)
{
    return HI_PROC_RemoveDir(pszName);
}

HI_S32 SVR_PROC_AddEntry(HI_U32 u32ModuleID, const SVR_PROCENTRY_S* pstEntry)
{
    return HI_PROC_AddEntry(u32ModuleID, (HI_PROC_ENTRY_S*)pstEntry);
}

HI_S32 SVR_PROC_RemoveEntry(HI_U32 u32ModuleID, const SVR_PROCENTRY_S* pstEntry)
{
    return HI_PROC_RemoveEntry(u32ModuleID, (HI_PROC_ENTRY_S*)pstEntry);
}

HI_S32 SVR_PROC_Printf(SVR_PROCSHOW_BUFFER_S *pstBuf, const HI_CHAR *pFmt, ...)
{
    HI_U32 u32Len = 0;
    va_list args = {0};

    if ((HI_NULL == pstBuf) || (HI_NULL == pstBuf->pu8Buf) || (HI_NULL == pFmt))
    {
        return HI_FAILURE;
    }

    /* log buffer overflow */
    if (pstBuf->u32Offset >= pstBuf->u32Size)
    {
        return HI_FAILURE;
    }

    va_start(args, pFmt);
    u32Len = (HI_U32)HI_OSAL_Vsnprintf((HI_CHAR*)pstBuf->pu8Buf + pstBuf->u32Offset,
                            pstBuf->u32Size - pstBuf->u32Offset, pFmt, args);
    va_end(args);

    pstBuf->u32Offset += u32Len;
    return HI_SUCCESS;
}

