/******************************************************************************
 *
 *             Copyright 2009 - 2009, Hisilicon Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 *
 ******************************************************************************
 * File Name     : hi_adp_memmng.c
 * Description   : 内存管理实现文件
 *
 * History       :
 * Version     Date        Author      DefectNum    Modification:
 * 1.1         2009-3-3    q63946      NULL         Created file
 *
 ******************************************************************************/

#include "utils/Logger.h"
#include "linux_list.h"
#include "common/hi_ref_errno.h"
#include "common/hi_ref_mod.h"
//#include "hi_log_cli.h"
//#include "hi_log.h"
//#include "hi_out.h"
#include "common/hi_adp_mutex.h"
#include "common/hi_adp_memmng.h"

#include <stdlib.h>
#include "common/hi_argparser.h"
#ifndef HI_ADVCA_FUNCTION_RELEASE

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

//SVR_MODULE_DECLARE("COMMONMOD")
/*----------------------------------------------*
 * external variables
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants
 *----------------------------------------------*/

/*----------------------------------------------*
 * types
 *----------------------------------------------*/
typedef struct {
    struct list_head    stList;
    HI_VOID             *pvMemAddr;     /*内存首地址*/
    HI_U32              u32MemSize;     /*内存大小*/
    const HI_CHAR       *pszFileName;   /*分配内存空间的文件名*/
    HI_U32              u32Line;        /*分配内存空间的文件中的行号*/
} MEMMNG_ALLOC_INFO;

typedef struct tagMEMMNG_QUEUE_S{
    HI_U32 u32ModId;            /*模块ID*/
    HI_ThreadMutex_S stMutex;   /*每个模块的线程锁*/
    HI_U32 u32MemSize;          /*该模块已分配内存总大小*/
    HI_U32 u32AllocNum;         /*该模块分配内存个数*/
    HI_U32 u32AllocTimes;         /*该模块分配内存个数*/
    struct list_head stListHead;  /*该模块内存信息链表*/
} MEMMNG_QUEUE_S;
/*----------------------------------------------*
 * error code
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables
 *----------------------------------------------*/
static HI_BOOL s_bMemMng_init = HI_FALSE;

static MEMMNG_QUEUE_S s_astMEMMNG_MEMINFO[HI_MOD_MAX_NUM];

/*----------------------------------------------*
 * routine prototypes
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines
 *----------------------------------------------*/
#if HI_OS_TYPE == HI_OS_LINUX
static HI_S32 MEMMNG_InitStruct()
{
    if (HI_TRUE == s_bMemMng_init)
    {
        return HI_SUCCESS;
    }

    s_bMemMng_init = HI_TRUE;

    HI_U32 i = 0;

    for (i = 0; i < HI_MOD_MAX_NUM; i++)
    {
        s_astMEMMNG_MEMINFO[i].u32ModId = HI_MOD_INVALID_ID;
        HI_MutexInit(&s_astMEMMNG_MEMINFO[i].stMutex, NULL);
        HI_MutexLock(&s_astMEMMNG_MEMINFO[i].stMutex);
        s_astMEMMNG_MEMINFO[i].u32MemSize    = 0;
        s_astMEMMNG_MEMINFO[i].u32AllocNum   = 0;
        s_astMEMMNG_MEMINFO[i].u32AllocTimes = 0;
        s_astMEMMNG_MEMINFO[i].stListHead.prev = &s_astMEMMNG_MEMINFO[i].stListHead;
        s_astMEMMNG_MEMINFO[i].stListHead.next = &s_astMEMMNG_MEMINFO[i].stListHead;
        HI_MutexUnLock(&s_astMEMMNG_MEMINFO[i].stMutex);
    }

    return HI_SUCCESS;
}

static HI_U32 MEMMNG_GetActualAllocSize(HI_U32 u32AllocSize)
{
    HI_U32 u32AtActualAllocSize = (u32AllocSize+4);
    if((u32AllocSize+4) <= 16)
    {
        return  16;
    }

    u32AtActualAllocSize =u32AtActualAllocSize+8-u32AtActualAllocSize%8;
    return u32AtActualAllocSize;
}

HI_VOID *HI_MemMng_MallocD(const HI_U32 u32ModId,const HI_U32 u32Size,
                           const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
    MEMMNG_ALLOC_INFO* pstMemInfo;
    HI_U32 u32TempModId = u32ModId;

    /*参数检查*/
    if((u32ModId >= HI_MOD_MAX_NUM) || (u32Size == HI_NULL))
    {
        if(pszFileName != HI_NULL_PTR)
        {
            HI_LOGE("HI_MemMng_MallocD error:HI_EPAERM,<%s>(%u)\n",
                        pszFileName,u32LineNo);
        }
        else
        {
            HI_LOGE("HI_MemMng_MallocD error:HI_EPAERM\n");
        }
        return HI_NULL_PTR;
    }

    if (!strncmp(HI_MOD_GetNameById(u32ModId), "", 1 /* Compare length with empty string */))
    {
        HI_LOGD("HI_MemMng_MallocD error: mod id not register\n");
        u32TempModId = HI_MOD_ID_NULL;
    }

    pstMemInfo = (MEMMNG_ALLOC_INFO*)malloc(sizeof(MEMMNG_ALLOC_INFO));
    if(pstMemInfo == HI_NULL_PTR)
    {
        HI_LOGE("malloc error:HI_AV_ENORES\n");
        return HI_NULL_PTR;
    }

    pstMemInfo->pvMemAddr = malloc(u32Size);
    if(pstMemInfo->pvMemAddr == HI_NULL_PTR)
    {
        HI_LOGE("malloc error:HI_AV_ENORES, size = %d\n", u32Size);
        free(pstMemInfo);
        return HI_NULL_PTR;
    }

    pstMemInfo->u32MemSize = MEMMNG_GetActualAllocSize(u32Size);
    pstMemInfo->pszFileName = pszFileName;
    pstMemInfo->u32Line = u32LineNo;

    (HI_VOID)MEMMNG_InitStruct();

    /*记录内存分配信息到全局变量*/
    HI_MutexLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    list_add(&pstMemInfo->stList, &s_astMEMMNG_MEMINFO[u32TempModId].stListHead);

    s_astMEMMNG_MEMINFO[u32TempModId].u32MemSize += pstMemInfo->u32MemSize;
    s_astMEMMNG_MEMINFO[u32TempModId].u32AllocNum++;
    s_astMEMMNG_MEMINFO[u32TempModId].u32AllocTimes++;
    s_astMEMMNG_MEMINFO[u32TempModId].u32ModId = u32TempModId;

    HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    return (pstMemInfo->pvMemAddr);
}

HI_VOID *HI_MemMng_ReallocD(const HI_U32 u32ModId,HI_VOID* pSrcMem, const HI_U32 u32Size,
                           const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
    struct list_head* pstPosition = HI_NULL_PTR;
    MEMMNG_ALLOC_INFO* pstMemInfo = HI_NULL_PTR;
    HI_VOID* pNewMemory = HI_NULL_PTR;
    HI_U32 u32TempModId = u32ModId;

    /*参数检查*/
    if((u32ModId >= HI_MOD_MAX_NUM) || (u32Size == HI_NULL))
    {
        if(pszFileName != HI_NULL_PTR)
        {
            HI_LOGE("HI_MemMng_MallocD error:HI_EPAERM,<%s>(%u)\n",
                        pszFileName,u32LineNo);
        }
        else
        {
            HI_LOGE("HI_MemMng_MallocD error:HI_EPAERM\n");
        }
        return HI_NULL_PTR;
    }

    if (!strncmp(HI_MOD_GetNameById(u32ModId), "", 1 /* Compare length with empty string */))
    {
        HI_LOGE("HI_MemMng_ReallocD error: mod id not register\n");
        u32TempModId = HI_MOD_ID_NULL;
    }

    if(HI_NULL_PTR == pSrcMem)
    {
        return HI_MemMng_MallocD(u32TempModId, u32Size, pszFileName ,u32LineNo);
    }

    (HI_VOID)MEMMNG_InitStruct();

    HI_MutexLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    list_for_each(pstPosition, &s_astMEMMNG_MEMINFO[u32TempModId].stListHead)
    {
        pstMemInfo = list_entry(pstPosition, MEMMNG_ALLOC_INFO, stList);
        if(pSrcMem == pstMemInfo->pvMemAddr)
        {
            pNewMemory = realloc(pSrcMem, u32Size);
            if(pNewMemory == HI_NULL_PTR)
            {
                HI_LOGE("realloc error:HI_AV_ENORES\n");
                HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);
                return HI_NULL_PTR;
            }
            pstMemInfo->pvMemAddr = pNewMemory;
            s_astMEMMNG_MEMINFO[u32TempModId].u32MemSize -= pstMemInfo->u32MemSize;
            pstMemInfo->u32MemSize = MEMMNG_GetActualAllocSize(u32Size);
            s_astMEMMNG_MEMINFO[u32TempModId].u32MemSize += pstMemInfo->u32MemSize;
            s_astMEMMNG_MEMINFO[u32TempModId].u32ModId = u32ModId;
            HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);
            return pNewMemory;
        }
    }

    HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    if(pszFileName != HI_NULL_PTR)
    {
        HI_LOGE("HI_MemMng_Free error:HI_FAILURE,<%s>(%d)\n",
                    pszFileName,u32LineNo);
    }
    else
    {
        HI_LOGE("HI_MemMng_Free error:HI_FAILURE\n");
    }
    return HI_NULL_PTR;
}

HI_S32 HI_MemMng_Free(const HI_U32 u32ModId,const HI_VOID* pvMemAddr,
                      const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
    struct list_head* pstPosition = HI_NULL_PTR;
    MEMMNG_ALLOC_INFO* pstMemInfo = HI_NULL_PTR;
    HI_U32 u32TempModId = u32ModId;

    if (HI_FALSE == s_bMemMng_init)
    {
        HI_LOGE("HI_MemMng_Free error:HI_ENOTINIT\n");

        return HI_FAILURE;
    }

    /*参数检查*/
    if(u32ModId >= HI_MOD_MAX_NUM)
    {
        if(pszFileName != HI_NULL_PTR)
        {
            HI_LOGE("HI_MemMng_Free error:HI_EPAERM,<%s>(%d)\n",
                        pszFileName,u32LineNo);
        }
        else
        {
            HI_LOGE("HI_MemMng_Free error:HI_EPAERM\n");
        }
        return HI_EPAERM;
    }

    if (!strncmp(HI_MOD_GetNameById(u32ModId), "", 1 /* Compare length with empty string */))
    {
        HI_LOGE("HI_MemMng_Free error: mod id not register\n");
        u32TempModId = HI_MOD_ID_NULL;
    }

    if(HI_NULL_PTR == pvMemAddr)
    {
        return HI_SUCCESS;
    }

    HI_MutexLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    list_for_each(pstPosition, &s_astMEMMNG_MEMINFO[u32TempModId].stListHead)
    {
        pstMemInfo = list_entry(pstPosition, MEMMNG_ALLOC_INFO, stList);
        if(pvMemAddr == pstMemInfo->pvMemAddr)
        {
            list_del(pstPosition);
            pstPosition = pstPosition->prev;
            s_astMEMMNG_MEMINFO[u32TempModId].u32MemSize -= pstMemInfo->u32MemSize;
            free(pstMemInfo->pvMemAddr);
            pstMemInfo->pvMemAddr = HI_NULL_PTR;
            free(pstMemInfo);
            pstMemInfo = HI_NULL_PTR;
            s_astMEMMNG_MEMINFO[u32TempModId].u32AllocNum--;
            HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);
            return HI_SUCCESS;
        }
    }

    HI_MutexUnLock(&s_astMEMMNG_MEMINFO[u32TempModId].stMutex);

    if(pszFileName != HI_NULL_PTR)
    {
        HI_LOGE("HI_MemMng_Free error:HI_FAILURE,<%s>(%d)\n",
                    pszFileName,u32LineNo);
    }
    else
    {
        HI_LOGE("HI_MemMng_Free error:HI_FAILURE\n");
    }

    return HI_FAILURE;
}

#define HI_MCR_MEMMNG_MOD_ALL (-1)

#if (UNF_VERSION_CODE >= UNF_VERSION(3, 1))
static HI_S32 MemMng_printMod(HI_PROC_SHOW_BUFFER_S * pstBuf)
{
    HI_U32 i = 0;
    HI_U32 u32TotalUsed = 0;

    (HI_VOID)HI_PROC_Printf(pstBuf,"\r\n=======================Memery info==========================\r\n");

    for(i = 0; i < HI_MOD_MAX_NUM; i++)
    {
        if (HI_MOD_INVALID_ID == s_astMEMMNG_MEMINFO[i].u32ModId)
        {
            continue;
        }

        u32TotalUsed += s_astMEMMNG_MEMINFO[i].u32MemSize;
        (HI_VOID)HI_PROC_Printf(pstBuf,"Mod ID         :%u\r\n"
                                "Mod Name       :%s\r\n"
                                "Use Mem Size   :%u\r\n"
                                "Alloc Mem Num  :%u\r\n"
                                "Alloc Mem Times:%u\r\n",
                                s_astMEMMNG_MEMINFO[i].u32ModId,
                                HI_MOD_GetNameById(i),
                                s_astMEMMNG_MEMINFO[i].u32MemSize,
                                s_astMEMMNG_MEMINFO[i].u32AllocNum,
                                s_astMEMMNG_MEMINFO[i].u32AllocTimes);
        (HI_VOID)HI_PROC_Printf(pstBuf,"===================================================\r\n");
    }
    (HI_VOID)HI_PROC_Printf(pstBuf,"\r\n==================Total used %u bytes=====================\r\n",u32TotalUsed);
    return HI_SUCCESS;
}

static HI_S32 MemMng_printInfo(HI_PROC_SHOW_BUFFER_S * pstBuf,HI_U32 u32ModID,HI_BOOL bZeroOutput)
{
    struct list_head* pstPos;

    if((0 != s_astMEMMNG_MEMINFO[u32ModID].u32MemSize) || (HI_TRUE == bZeroOutput))
    {
        (HI_VOID)HI_PROC_Printf(pstBuf,"\r\n============================================================\r\n");
        (HI_VOID)HI_PROC_Printf(pstBuf,"Mod ID         :%u\r\n"
                                "Mod Name       :%s\r\n"
                                "Use Mem Size   :%u\r\n"
                                "Alloc Mem Num  :%u\r\n"
                                "Alloc Mem Times:%u\r\n",
                                s_astMEMMNG_MEMINFO[u32ModID].u32ModId,
                                HI_MOD_GetNameById(u32ModID),
                                s_astMEMMNG_MEMINFO[u32ModID].u32MemSize,
                                s_astMEMMNG_MEMINFO[u32ModID].u32AllocNum,
                                s_astMEMMNG_MEMINFO[u32ModID].u32AllocTimes);
        (HI_VOID)HI_PROC_Printf(pstBuf,"===================================================\r\n");

        list_for_each(pstPos, &(s_astMEMMNG_MEMINFO[u32ModID].stListHead))
        {
            MEMMNG_ALLOC_INFO* pstMemAlloc;
            pstMemAlloc = list_entry(pstPos, MEMMNG_ALLOC_INFO, stList);
            (HI_VOID)HI_PROC_Printf(pstBuf,"<%s>(%d):%p,%u\r\n",
                                    pstMemAlloc->pszFileName,pstMemAlloc->u32Line,
                                    pstMemAlloc->pvMemAddr,pstMemAlloc->u32MemSize);
            (HI_VOID)HI_PROC_Printf(pstBuf,"===================================================\r\n");
        }

        (HI_VOID)HI_PROC_Printf(pstBuf,"============================================================\r\n");
    }
    return HI_SUCCESS;
}

static HI_S32 MemMng_Queue(HI_PROC_SHOW_BUFFER_S * pstBuf)
{
    HI_U32 u32I;

    for(u32I = 0;u32I < HI_MOD_MAX_NUM; u32I++)
    {
        MemMng_printInfo(pstBuf,u32I,HI_FALSE);
    }

    return HI_SUCCESS;
}

HI_S32 HI_CMD_MemMng_Queue(HI_PROC_SHOW_BUFFER_S * pstBuf, HI_VOID *pPrivData)
{
    MemMng_printMod(pstBuf);
    MemMng_Queue(pstBuf);
    return HI_SUCCESS;
}

#endif
#if 0
static HI_S32 MemMng_printMod(HI_U32 u32Handle)
{
    HI_U32 i = 0;
    HI_U32 u32TotalUsed = 0;

    HI_OUT_Printf(u32Handle,"\r\n============================================================\r\n");

    for(i = 0; i < HI_MOD_MAX_NUM; i++)
    {
        if (HI_MOD_INVALID_ID == s_astMEMMNG_MEMINFO[i].u32ModId)
        {
            continue;
        }

        u32TotalUsed += s_astMEMMNG_MEMINFO[i].u32MemSize;
        HI_OUT_Printf(u32Handle,"Mod ID         :%u\r\n"
                                "Mod Name       :%s\r\n"
                                "Use Mem Size   :%u\r\n"
                                "Alloc Mem Num  :%u\r\n"
                                "Alloc Mem Times:%u\r\n",
                                s_astMEMMNG_MEMINFO[i].u32ModId,
                                HI_MOD_GetNameById(i),
                                s_astMEMMNG_MEMINFO[i].u32MemSize,
                                s_astMEMMNG_MEMINFO[i].u32AllocNum,
                                s_astMEMMNG_MEMINFO[i].u32AllocTimes);
        HI_OUT_Printf(u32Handle,"===================================================\r\n");
    }
    HI_OUT_Printf(u32Handle,"\r\n==================Total used %u bytes=====================\r\n",u32TotalUsed);
    return HI_SUCCESS;
}

static HI_S32 MemMng_TotalUsed(HI_U32 u32Handle)
{
    return HI_FAILURE;
    #if 0
    HI_S32 s32I;

    HI_U32 u32TotalUsed = (HI_U32)HI_DB_MemoryUsed();

    for(s32I = 0;s32I < HI_MOD_MAX_NUM;s32I++)
    {
        u32TotalUsed += s_astMEMMNG_MEMINFO[s32I].u32MemSize;
    }
    HI_OUT_Printf(u32Handle,"\r\n==================Total used %u bytes=====================\r\n",u32TotalUsed);
    #endif
    return HI_SUCCESS;
}

static HI_S32 MemMng_DbUsed(HI_U32 u32Handle)
{
    return HI_FAILURE;
    #if 0
    HI_S32 s32I;
    HI_U32 u32TotalUsed = (HI_U32)HI_DB_MemoryUsed();
    HI_OUT_Printf(u32Handle,"\r\n==================SQLite used %u bytes=====================\r\n",u32TotalUsed);
    #endif
    return HI_SUCCESS;
}
static HI_S32 MemMng_printInfo(HI_U32 u32Handle,HI_U32 u32ModID,HI_BOOL bZeroOutput)
{
    struct list_head* pstPos;

    if((0 != s_astMEMMNG_MEMINFO[u32ModID].u32MemSize) || (HI_TRUE == bZeroOutput))
    {
        HI_OUT_Printf(u32Handle,"\r\n============================================================\r\n");
        HI_OUT_Printf(u32Handle,"Mod ID         :%u\r\n"
                                "Mod Name       :%s\r\n"
                                "Use Mem Size   :%u\r\n"
                                "Alloc Mem Num  :%u\r\n"
                                "Alloc Mem Times:%u\r\n",
                                s_astMEMMNG_MEMINFO[u32ModID].u32ModId,
                                HI_MOD_GetNameById(u32ModID),
                                s_astMEMMNG_MEMINFO[u32ModID].u32MemSize,
                                s_astMEMMNG_MEMINFO[u32ModID].u32AllocNum,
                                s_astMEMMNG_MEMINFO[u32ModID].u32AllocTimes);
        HI_OUT_Printf(u32Handle,"===================================================\r\n");

        list_for_each(pstPos, &(s_astMEMMNG_MEMINFO[u32ModID].stListHead))
        {
            MEMMNG_ALLOC_INFO* pstMemAlloc;
            pstMemAlloc = list_entry(pstPos, MEMMNG_ALLOC_INFO, stList);
            HI_OUT_Printf(u32Handle,"<%s>(%d):%p,%u\r\n",
                                    pstMemAlloc->pszFileName,pstMemAlloc->u32Line,
                                    pstMemAlloc->pvMemAddr,pstMemAlloc->u32MemSize);
            HI_OUT_Printf(u32Handle,"===================================================\r\n");
        }

        HI_OUT_Printf(u32Handle,"============================================================\r\n");
    }
    return HI_SUCCESS;
}

static HI_S32 MemMng_Queue(HI_U32 u32Handle,HI_U32 u32ModID)
{
    HI_U32 u32I;

    if(HI_MCR_MEMMNG_MOD_ALL != u32ModID)
    {
        if(u32ModID >= HI_MOD_MAX_NUM)
        {
            HI_OUT_Printf(u32Handle,"Mod isn't exist\r\n");
            return HI_EPAERM;
        }

        return MemMng_printInfo(u32Handle,u32ModID,HI_TRUE);
    }

    for(u32I=0;u32I<HI_MOD_MAX_NUM;u32I++)
    {
        MemMng_printInfo(u32Handle,u32I,HI_FALSE);
    }

    return HI_SUCCESS;
}
/*
mem
    -mod [0~56|all] ;打印指定模块分配内存情况
    -modlist        ;打印模块信息列表
*/
HI_S32  HI_CMD_MemMng_Queue(HI_U32 u32Handle,int argc, const char* argv[])
{
    HI_S32 s32Rtn;
    HI_S32  _help;
    HI_CHAR _mod[8];
    HI_CHAR _modid[8];
    /*
    HI_CHAR szErrPrintStr[] = "unrecongnized or incomplete command line.\r\n"
                              "USAGE:\r\n"
                              ARG_S4"snapcfg {-act set|list|snap} [-chn chnID] [-qval qval] [-pre presnap_num]\r\n"
                              "type \"snapcfg -?\" to get help";
    */

    ARG_OPT_S opts[] =
    {
        {"?", ARG_TYPE_NO_PARA|ARG_TYPE_SINGLE, HI_NULL_PTR, HI_FALSE, {0},
              ARG_S4"print help msg\r\n",(HI_VOID*)(&_help),4},

        {"mod", ARG_TYPE_NO_OPT|ARG_TYPE_STRING,
                    HI_NULL_PTR, HI_FALSE, {0},
                    ARG_S4 "ModID,Must option!!!\r\n"
                    ARG_S4 "Range of value: [0~52|all].\r\n", (HI_VOID*)&_mod,4},

        {"modlist", ARG_TYPE_NO_PARA|ARG_TYPE_STRING,
                    HI_NULL_PTR, HI_FALSE, {0},
                    ARG_S4 "list mod ID,Must option!!!\r\n", (HI_VOID*)&_modid,4},

        {"all", ARG_TYPE_NO_PARA|ARG_TYPE_STRING,
                    HI_NULL_PTR, HI_FALSE, {0},
                    ARG_S4 "Total used memory in bytes,Must option!!!\r\n", (HI_VOID*)&_modid,4},
        {"db", ARG_TYPE_NO_PARA|ARG_TYPE_STRING,
                    HI_NULL_PTR, HI_FALSE, {0},
                    ARG_S4 "SQLite used memory in bytes,Must option!!!\r\n", (HI_VOID*)&_modid,4},
        {"END",     ARG_TYPE_END, NULL, HI_FALSE, {0},
                    ARG_S4"END\n", NULL,0},
    };

    /*按opts 规则解析用户命令*/
    s32Rtn = HI_ARG_Parser(argc, argv, opts);
    if (HI_SUCCESS != s32Rtn)
    {
        HI_LOG_ERROR("HI_ARG_Parser error:%d\n",
                    s32Rtn);
        return HI_FAILURE;
    }

    if (HI_SUCCESS == HI_ARG_OptIsSet("?", opts))
    {
        HI_ARG_PrintHelp(u32Handle,opts);
        return HI_SUCCESS;
    }

    if (HI_SUCCESS == HI_ARG_OptIsSet("modlist", opts))
    {
        MemMng_printMod(u32Handle);
        return HI_SUCCESS;
    }
    if (HI_SUCCESS == HI_ARG_OptIsSet("all", opts))
    {
        MemMng_TotalUsed(u32Handle);
        return HI_SUCCESS;
    }
    if (HI_SUCCESS == HI_ARG_OptIsSet("db", opts))
    {
        MemMng_DbUsed(u32Handle);
        return HI_SUCCESS;
    }
    if(HI_SUCCESS != HI_ARG_OptIsSet("mod",opts))
    {
        return MemMng_Queue(u32Handle,HI_MCR_MEMMNG_MOD_ALL);
    }
    else
    {
        return MemMng_Queue(u32Handle,atol(_mod));
    }

    return HI_SUCCESS;
}
#endif

#elif HI_OS_TYPE == HI_OS_WIN32
/****************************************************************************
 * Function     : HI_MemMng_MallocD
 * Description  : 申请内存
 * Input        : const HI_U32 u32ModId,模块ID
 *                const HI_U32 u32Size,待申请内存的大小
 *                const HI_CHAR *pszFileName,调用该接口的文件名，可以使用"__FILE__"
 *                const HI_U32 u32LineNo,调用该接口的文件所在行，可以使用"__LINGE__"
 * Output       : None
 * Return       : HI_VOID *,申请到的内存首地址，空为未申请到
 * Data Accessed: None
 * Data Updated : None
 * Others       : None
 *****************************************************************************/
HI_VOID *HI_MemMng_MallocD(const HI_U32 u32ModId,const HI_U32 u32Size,
                           const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
        return malloc(u32Size);
}

HI_VOID *HI_MemMng_ReallocD(const HI_U32 u32ModId,HI_VOID* pSrcMem, const HI_U32 u32Size,
                           const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
        return realloc(u32Size);
}

/****************************************************************************
 * Function     : HI_MemMng_Free
 * Description  : 释放内存
 * Input        : const HI_U32 u32ModId,模块ID
 *                const HI_VOID* pvMemAddr,内存首地址
 *                const HI_CHAR *pszFileName,调用该接口的文件名，可以使用"__FILE__"
 *                const HI_U32 u32LineNo,调用该接口的文件所在行，可以使用"__LINGE__"
 * Output       : None
 * Return       : HI_S32
 * Data Accessed: None
 * Data Updated : None
 * Others       : None
 *****************************************************************************/
HI_S32 HI_MemMng_Free(const HI_U32 u32ModId,const HI_VOID* pvMemAddr,
                      const HI_CHAR *pszFileName, const HI_U32 u32LineNo)
{
    return free(pvMemAddr);
}
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#else

#if (UNF_VERSION_CODE >= UNF_VERSION(3, 1))
HI_S32 HI_CMD_MemMng_Queue(HI_PROC_SHOW_BUFFER_S * pstBuf, HI_VOID *pPrivData)
{
    return HI_SUCCESS;
}
#else
HI_S32  HI_CMD_MemMng_Queue(HI_U32 u32Handle,int argc, const char* argv[])
{
    return HI_SUCCESS;
}

#endif

#endif /*HI_ADVCA_FUNCTION_RELEASE*/
