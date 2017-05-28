/***********************************************************************************
*              Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_threadmng.c
* Description: wait thread finish model .
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-26   q63946  NULL         Create this file.
***********************************************************************************/
//#include "hi_head.h"
//#include "hi_ref_stb_comm.h"
#include "hi_type.h"
#include "utils/Logger.h"
#include "common/hi_adp_threadmng.h"
#include "common/hi_adp_thread.h"
#include "common/hi_adp_mutex.h"
#include "common/hi_ref_errno.h"
#include "linux_list.h"
#include "common/hi_argparser.h"

#if HI_OS_TYPE == HI_OS_WIN32
#include <process.h>
#include <windows.h>
#define sleep Sleep

#elif HI_OS_TYPE == HI_OS_LINUX
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#ifndef HI_ADVCA_FUNCTION_RELEASE

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/* Modified for advanced security, define status string length for strncpy */
#define MAX_STATUS_STR_LEN (8)
//SVR_MODULE_DECLARE("COMMONMOD")
typedef struct hiPthreadMNG_Thread_S
{
    struct list_head    struList;       /*链表*/
    HI_Pthread_T        u32Thread;      /*线程ID*/
    HI_CHAR*            pszThreadInfo;  /*线程描述*/
    HI_CHAR*            pszCrtFile;     /*创建线程的文件名*/
    HI_S32              s32CrtLine;     /*创建线程的文件名所在行*/
    HI_ThreadFun        pfunThread;     /*线程处理接口*/
    HI_VOID*            pvArg;          /*线程处理接口参数*/
    HI_PID_T            s32PID;         /*线程进程ID*/
    HI_PID_T            s32PPID;        /*线程父进程ID*/
    HI_Pthread_T        u32PthreadID;   /*pthread_self获取的线程ID*/
    HI_BOOL             bPTHMNGCreate;  /*是否是HI_PthreadMNG_Create创建的线程*/
    HI_S32              s32Status;      /*线程运行状态:小于0,
                                               HI_PTHREAD_STATUS_INIT,未运行
                                               HI_PTHREAD_STATUS_START,运行中
                                               HI_PTHREAD_STATUS_STOP,停止*/
    HI_PTR_FN_THREAD_ENDProc pfunEndProc;   /*线程结束处理*/
    HI_VOID             *pvEndProcParam;    /*线程结束处理参数*/
} PthreadMNG_Thread_S;

//PthreadMNG_Thread_S *g_pstruPthreadWait;

/*运行中的线程队列*/
static struct list_head s_stTrdMngPthreadMng = LIST_HEAD_INIT(s_stTrdMngPthreadMng);

/*已经退出的线程队列*/
static struct list_head s_stTrdMngPthreadMng_Exit = LIST_HEAD_INIT(s_stTrdMngPthreadMng_Exit);

//pthread_mutex_t s_stTrdMngMutex = PTHREAD_MUTEX_INITIALIZER;
static HI_ThreadMutex_S s_stTrdMngMutex;//q63946 add for PC
static volatile HI_S32 s_s32TrdMngMutexFlag = 0;
static volatile HI_S32 s_s32TrdMngPTStatus = HI_TRUE;
static HI_BOOL s_bTrdMngAudoWait = HI_FALSE;
static HI_Pthread_T s_hdlTrdMngWait = 0;    /*自动回收线程资源的任务句柄*/

#define HI_INIT_MUTEX() \
    do \
    { \
        if(s_s32TrdMngMutexFlag == 0) \
        { \
            HI_MutexInit(&s_stTrdMngMutex,0); \
            s_s32TrdMngMutexFlag =1; \
        } \
    }while(0)

static HI_VOID* PthreadMng_PthreadProc(HI_VOID* pvArg);
static HI_VOID* pthreadMng_WaitProc(HI_VOID* pvArg);
static HI_S32 PthreadMNG_Wait();

HI_S32 HI_PthreadMNG_Init(HI_BOOL bAutoWait)
{
    HI_S32 s32Rtn;
    /*
    if((s_s32TrdMngPTStatus == HI_TRUE) || (0 == list_empty(&s_stTrdMngPthreadMng))
        || (0 == list_empty(&s_stTrdMngPthreadMng_Exit)))
    {
        return HI_FAILURE;
    }
    */
    s_bTrdMngAudoWait = bAutoWait;
    if(HI_TRUE == bAutoWait)
    {
        s32Rtn = HI_PthreadCreate(&s_hdlTrdMngWait, HI_NULL_PTR, pthreadMng_WaitProc, HI_NULL_PTR);
        if(HI_SUCCESS != s32Rtn)
        {
            HI_LOGE("VTOP_MSG_UnRegister error:%x\n",s32Rtn);
            s_bTrdMngAudoWait = HI_FALSE;
        }
    }

    s_s32TrdMngPTStatus = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Deinit()
{
    s_s32TrdMngPTStatus = HI_FALSE;
    return HI_SUCCESS;
}

HI_S32  HI_PthreadMNG_CreateD(HI_CHAR* pszPthreadInfo,HI_Pthread_T *pu32Threadp,
                             const HI_ThreadAttr_S *pstruAttr,
                             HI_ThreadFun pfunStart_routine, HI_VOID* pvArg,
                             HI_CHAR* pszFile,HI_S32 s32Line)
{
    HI_S32               s32Rtn = 0;
    PthreadMNG_Thread_S* pstruPWTmp = HI_NULL_PTR;
    HI_Pthread_T         u32Threadp = 0;

    if((HI_NULL_PTR == pu32Threadp) || (HI_NULL_PTR == pszPthreadInfo)
       || (HI_NULL_PTR == pfunStart_routine))
    {
        HI_LOGE("parameter error, pu32Threadp = %p,pszPthreadInfo = %p,pfunStart_routine = %p\n",
                pu32Threadp,pszPthreadInfo,pfunStart_routine);
        return HI_EPAERM;
    }

    if(s_s32TrdMngPTStatus == HI_FALSE)
    {
        HI_LOGE("call deinit ,please init\n");
        return HI_FAILURE;
    }

    HI_INIT_MUTEX();//q63946 add for PC
    pstruPWTmp = (PthreadMNG_Thread_S *)malloc(sizeof(PthreadMNG_Thread_S));
    if(HI_NULL_PTR == pstruPWTmp)
    {
        HI_LOGE("malloc error\n");
        return HI_EPAERM;
    }

    memset(pstruPWTmp, 0, sizeof(PthreadMNG_Thread_S));

    pstruPWTmp->bPTHMNGCreate   = HI_TRUE;
    pstruPWTmp->u32Thread       = 0;
    pstruPWTmp->pszThreadInfo   = strdup(pszPthreadInfo);
    pstruPWTmp->pfunThread      = pfunStart_routine;
    pstruPWTmp->pvArg           = pvArg;
    pstruPWTmp->pszCrtFile      = pszFile?pszFile:"";
    pstruPWTmp->s32CrtLine      = s32Line;

    HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_lock(&s_stTrdMngMutex);
    list_add_tail(&(pstruPWTmp->struList), &s_stTrdMngPthreadMng);

    HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_unlock(&s_stTrdMngMutex);

    s32Rtn = HI_PthreadCreate(&u32Threadp, pstruAttr, PthreadMng_PthreadProc,
                              (HI_VOID*)pstruPWTmp);
    if(HI_SUCCESS != s32Rtn)
    {
        HI_LOGE("HI_PthreadCreate Fail,%x\n", s32Rtn);
        HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
        //pthread_mutex_lock(&s_stTrdMngMutex);
        list_del(&(pstruPWTmp->struList));

        HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
        free(pstruPWTmp->pszThreadInfo);
        free(pstruPWTmp);
        return s32Rtn;
    }

    pstruPWTmp->u32Thread = u32Threadp;
    if((pstruPWTmp->s32Status == HI_PTHREAD_STATUS_INIT)
       || (pstruPWTmp->s32Status == HI_PTHREAD_STATUS_START))
    {
    }
    else
    {
        //printf("<%s>(%d)[ INFO ] Thread Status : %x\n",
        //        __FILE__,__LINE__,pstruPWTmp->s32Status);
    }

    *pu32Threadp = u32Threadp;
    //list_add_tail(&(pstruPWTmp->struList), &s_stTrdMngPthreadMng);

    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_RegisterEndProc(HI_Pthread_T u32Thread,
                                    HI_PTR_FN_THREAD_ENDProc pfunEndProc,
                                    HI_VOID* pvParam)
{
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    struct list_head*    pstruPos    = HI_NULL_PTR;
    //HI_Pthread_T    u32Thread   = 0;

    if(HI_NULL_PTR == pfunEndProc)
    {
        HI_LOGE("parameter error, pfunEndProc = %p\n", pfunEndProc);
        return HI_EPAERM;
    }

    HI_INIT_MUTEX();//q63946 add for PC

    HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_lock(&s_stTrdMngMutex);
    list_for_each(pstruPos, &s_stTrdMngPthreadMng)
    {
        pstruPWtmp  = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));
        if(u32Thread == pstruPWtmp->u32Thread)
        {
            pstruPWtmp->pfunEndProc = pfunEndProc;
            pstruPWtmp->pvEndProcParam = pvParam;
            HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
            return HI_SUCCESS;
        }
    }
    HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC

    pfunEndProc(pvParam);

    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Add(HI_Pthread_T u32Thread, HI_CHAR* pszPthreadInfo)
{
    PthreadMNG_Thread_S *pstruPWTmp = HI_NULL_PTR;

    pstruPWTmp = (PthreadMNG_Thread_S *)malloc(sizeof(PthreadMNG_Thread_S));
    if(HI_NULL_PTR == pstruPWTmp)
    {
        return HI_EEMPTY;
    }
    HI_INIT_MUTEX();//q63946 add for PC
    memset(pstruPWTmp, 0, sizeof(PthreadMNG_Thread_S));

    pstruPWTmp->u32Thread = u32Thread;
    pstruPWTmp->pszThreadInfo = (HI_NULL_PTR == pszPthreadInfo)
                                ? pszPthreadInfo : strdup(pszPthreadInfo);
    HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_lock(&s_stTrdMngMutex);
    list_add_tail(&(pstruPWTmp->struList), &s_stTrdMngPthreadMng);
    HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_unlock(&s_stTrdMngMutex);
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Exit()
{
    //return HI_PthreadMNG_Del(pthread_self());
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Del(HI_Pthread_T u32Thread)
{
    struct list_head* pstruPos = HI_NULL_PTR;
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    HI_INIT_MUTEX();//q63946 add for PC
    HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_lock(&s_stTrdMngMutex);
    list_for_each(pstruPos, &s_stTrdMngPthreadMng)
    {
        pstruPWtmp = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));
        if(u32Thread == pstruPWtmp->u32Thread)
        {
            break;
        }
    }

    if(pstruPos == &s_stTrdMngPthreadMng)
    {
        HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
        //pthread_mutex_unlock(&s_stTrdMngMutex);
        return HI_FAILURE;
    }

    if(NULL == pstruPos)
    {
        HI_MutexUnLock(&s_stTrdMngMutex);
        return HI_FAILURE;
    }

    list_del(pstruPos);

    list_add_tail(pstruPos, &s_stTrdMngPthreadMng_Exit);
    HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_unlock(&s_stTrdMngMutex);

    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Join(HI_Pthread_T u32Thread)
{
    struct list_head* pstruPos = HI_NULL_PTR;
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    HI_INIT_MUTEX();//q63946 add for PC
    HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
    //pthread_mutex_lock(&s_stTrdMngMutex);
    list_for_each(pstruPos, &s_stTrdMngPthreadMng)
    {
        pstruPWtmp = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));
        if(u32Thread == pstruPWtmp->u32Thread)
        {
            break;
        }
    }

    if(pstruPos == &s_stTrdMngPthreadMng)
    {
        list_for_each(pstruPos, &s_stTrdMngPthreadMng_Exit)
        {
            pstruPWtmp = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));
            if(u32Thread == pstruPWtmp->u32Thread)
            {
                break;
            }
        }

        if(pstruPos == &s_stTrdMngPthreadMng_Exit)
        {
            HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
            //pthread_mutex_unlock(&s_stTrdMngMutex);
            return HI_FAILURE;
        }
    }

    if(NULL == pstruPos )
    {
        HI_MutexUnLock(&s_stTrdMngMutex);
        return HI_FAILURE;
    }
    list_del(pstruPos);

    //list_add_tail(pstruPos, &s_stTrdMngPthreadMng_Exit);
    HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC

    HI_PthreadJoin(u32Thread, HI_NULL_PTR);
    free(pstruPWtmp->pszThreadInfo);
    free(pstruPWtmp);
    //pthread_mutex_unlock(&s_stTrdMngMutex);

    return HI_SUCCESS;
}

static HI_S32 PthreadMNG_Wait()
{
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    struct list_head*    pstruPos    = HI_NULL_PTR;
    HI_Pthread_T    u32Thread   = 0;

    HI_INIT_MUTEX();//q63946 add for PC
    while((s_s32TrdMngPTStatus == HI_TRUE) || (0 == list_empty(&s_stTrdMngPthreadMng))
        || (0 == list_empty(&s_stTrdMngPthreadMng_Exit)))
    {
        HI_MutexLock(&s_stTrdMngMutex);//q63946 add for PC
        //pthread_mutex_lock(&s_stTrdMngMutex);
        list_for_each(pstruPos, &s_stTrdMngPthreadMng_Exit)
        {
            pstruPWtmp  = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));
            u32Thread   = pstruPWtmp->u32Thread;
            HI_PthreadJoin(u32Thread, HI_NULL_PTR);

            //HI_PthreadMNG_Del(u32Thread);
            list_del(pstruPos);

            if(HI_NULL_PTR != pstruPWtmp->pfunEndProc)
            {
                pstruPWtmp->pfunEndProc(pstruPWtmp->pvEndProcParam);
            }

            free(pstruPWtmp->pszThreadInfo);
            free(pstruPWtmp);
            //PthreadMNG_Del_Node(u32Thread);
            pstruPos = &s_stTrdMngPthreadMng_Exit;
        }
        HI_MutexUnLock(&s_stTrdMngMutex);//q63946 add for PC
        //pthread_mutex_unlock(&s_stTrdMngMutex);
        sleep(1);
    }

    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Wait()
{
    HI_S32 s32Rtn;

    if(HI_TRUE == s_bTrdMngAudoWait)
    {
        return HI_SUCCESS;
    }

    s32Rtn = PthreadMNG_Wait();
    if(HI_SUCCESS != s32Rtn)
    {
        HI_LOGE("PthreadMNG_Wait error:%x\n",s32Rtn);
    }

    return s32Rtn;
}

static HI_VOID* PthreadMng_PthreadProc(HI_VOID* pvArg)
{
    HI_S32          s32Rtn = 0;
    PthreadMNG_Thread_S* pstruPWTmp = HI_NULL_PTR;

    pstruPWTmp = (PthreadMNG_Thread_S*)pvArg;

    pstruPWTmp->s32Status = HI_PTHREAD_STATUS_START;

#if HI_OS_TYPE == HI_OS_WIN32
    pstruPWTmp->s32PID = _getpid();
    pstruPWTmp->s32PPID = 0;
    pstruPWTmp->u32PthreadID = 0;
#elif HI_OS_TYPE == HI_OS_LINUX
    pstruPWTmp->s32PID = getpid();
    pstruPWTmp->s32PPID = getppid();
    pstruPWTmp->u32PthreadID = pthread_self();
#endif
    s32Rtn = (HI_S32)pstruPWTmp->pfunThread(pstruPWTmp->pvArg);

    pstruPWTmp->s32Status = HI_PTHREAD_STATUS_STOP;
    HI_PthreadMNG_Del(pstruPWTmp->u32Thread);

    return (HI_VOID*)s32Rtn;
}

static HI_VOID* pthreadMng_WaitProc(HI_VOID* pvArg)
{
    HI_S32 s32Rtn;
    s32Rtn = PthreadMNG_Wait();
    if(HI_SUCCESS != s32Rtn)
    {
        HI_LOGE("PthreadMNG_Wait error:%x\n",s32Rtn);
    }

    return (HI_VOID*)s32Rtn;
}

#if (UNF_VERSION_CODE >= UNF_VERSION(3, 1))
HI_S32 HI_CMD_ThreadList(HI_PROC_SHOW_BUFFER_S * pstBuf, HI_VOID *pPrivData)
{
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    struct list_head*    pstruPos    = HI_NULL_PTR;

    list_for_each(pstruPos, &s_stTrdMngPthreadMng)
    {
        HI_CHAR aszStatus[10]={'\0'};

        pstruPWtmp  = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));

        if(pstruPWtmp->s32Status == HI_PTHREAD_STATUS_INIT)
        {
            strncpy(aszStatus, "INIT", MAX_STATUS_STR_LEN);
        }
        if(pstruPWtmp->s32Status == HI_PTHREAD_STATUS_START)
        {
            strncpy(aszStatus, "START", MAX_STATUS_STR_LEN);
        }
        else
        {
            strncpy(aszStatus, "STOP", MAX_STATUS_STR_LEN);
        }

        (HI_VOID)HI_PROC_Printf(pstBuf, "\r\n===============================\r\n"
                                    "Thread Info             : %s\r\n"
                                    "Thread CreateID         : %u\r\n"
                                    "Thread GetID            : %u\r\n"
                                    "Thread PID              : %d\r\n"
                                    "Thread FatherPID        : %d\r\n"
                                    "Use HI_PthreadMNG_Create: %s\r\n"
                                    "Thread Status           : %s\r\n"
                                    "Thread Create in File   : %s\r\n"
                                    "Thread Create at Line   : %d\r\n",
                                    pstruPWtmp->pszThreadInfo,
                                    pstruPWtmp->u32Thread,
                                    pstruPWtmp->u32PthreadID,
                                    pstruPWtmp->s32PID,
                                    pstruPWtmp->s32PPID,
                                    pstruPWtmp->bPTHMNGCreate ? "Yes" : "NO",
                                    aszStatus,
                                    pstruPWtmp->pszCrtFile,
                                    pstruPWtmp->s32CrtLine);
    }

    return HI_SUCCESS;
}

#endif
#if 0
HI_S32 HI_PthreadMNG_PrintInfo(HI_U32 u32OutHandle, HI_PID_T u32ThreadPID)
{
    PthreadMNG_Thread_S* pstruPWtmp  = HI_NULL_PTR;
    struct list_head*    pstruPos    = HI_NULL_PTR;

    list_for_each(pstruPos, &s_stTrdMngPthreadMng)
    {
        HI_CHAR aszStatus[10]={'\0'};

        pstruPWtmp  = (PthreadMNG_Thread_S*)(list_entry(pstruPos, PthreadMNG_Thread_S, struList));

        if(pstruPWtmp->s32Status == HI_PTHREAD_STATUS_INIT)
        {
            strncpy(aszStatus, "INIT", MAX_STATUS_STR_LEN);
        }
        if(pstruPWtmp->s32Status == HI_PTHREAD_STATUS_START)
        {
            strncpy(aszStatus, "START", MAX_STATUS_STR_LEN);
        }
        else
        {
            strncpy(aszStatus, "STOP", MAX_STATUS_STR_LEN);
        }

        if(u32ThreadPID != HI_PTHREAD_INVALID_THREADID)
        {
            if(u32ThreadPID == pstruPWtmp->s32PID)
            {
                HI_OUT_Printf(u32OutHandle, "\r\n===============================\r\n"
                                            "Thread Info             : %s\r\n"
                                            "Thread CreateID         : %u\r\n"
                                            "Thread GetID            : %u\r\n"
                                            "Thread PID              : %d\r\n"
                                            "Thread FatherPID        : %d\r\n"
                                            "Use HI_PthreadMNG_Create: %s\r\n"
                                            "Thread Status           : %s\r\n"
                                            "Thread Create in File   : %s\r\n"
                                            "Thread Create at Line   : %d\r\n",
                                            pstruPWtmp->pszThreadInfo,
                                            pstruPWtmp->u32Thread,
                                            pstruPWtmp->u32PthreadID,
                                            pstruPWtmp->s32PID,
                                            pstruPWtmp->s32PPID,
                                            pstruPWtmp->bPTHMNGCreate ? "Yes" : "NO",
                                            aszStatus,
                                            pstruPWtmp->pszCrtFile,
                                            pstruPWtmp->s32CrtLine);
                break;
            }
        }
        else
        {
            HI_OUT_Printf(u32OutHandle, "\r\n===============================\r\n"
                                        "Thread Info             : %s\r\n"
                                        "Thread CreateID         : %u\r\n"
                                        "Thread GetID            : %u\r\n"
                                        "Thread PID              : %d\r\n"
                                        "Thread FatherPID        : %d\r\n"
                                        "Use HI_PthreadMNG_Create: %s\r\n"
                                        "Thread Status           : %s\r\n"
                                        "Thread Create in File   : %s\r\n"
                                        "Thread Create at Line   : %d\r\n",
                                        pstruPWtmp->pszThreadInfo,
                                        pstruPWtmp->u32Thread,
                                        pstruPWtmp->u32PthreadID,
                                        pstruPWtmp->s32PID,
                                        pstruPWtmp->s32PPID,
                                        pstruPWtmp->bPTHMNGCreate ? "Yes" : "NO",
                                        aszStatus,
                                        pstruPWtmp->pszCrtFile,
                                        pstruPWtmp->s32CrtLine);
        }

    }

    return HI_SUCCESS;
}

/*
thread
    -pid [0~]   ;打印指定线程PID信息，0代表打印所以线程
*/
HI_S32 HI_CMD_ThreadList(HI_U32 u32Handle, HI_S32 argc, const char* argv[])
{
    HI_S32          _help;
    HI_S32          _s32Pid = 0;
    HI_S32          s32Rtn = 0;

     HI_CHAR        *pszPrintSz  = HI_NULL_PTR;

     /*lint -e786 */ /*String concatenation within initializer*/
    ARG_OPT_S opts[] =
    {
        {"?",       ARG_TYPE_NO_PARA|ARG_TYPE_SINGLE|ARG_TYPE_STRING, HI_NULL_PTR, HI_FALSE, {0},
                    ARG_S4"print help msg\r\n", (HI_VOID*)(&_help),4},
        {"pid",  ARG_TYPE_MUST|ARG_TYPE_NO_OPT|ARG_TYPE_INT,"0~", HI_FALSE, {0},
                    ARG_S4 "The pid NO. you want to see, 0 for all\r\n"\
                    ARG_S4 ARG_S4"For example:\r\n"\
                    ARG_S4 ARG_S4"threadlist 0\r\n" , (HI_VOID*)(&_s32Pid),4},
        {"END",     ARG_TYPE_END, NULL, HI_FALSE, {0},
                    ARG_S4"END\n", NULL,0},
    };

     if((argc == 2) && (0 == strncmp(argv[1] ,"?", 1 /* Length of '?' */)))
    {
        HI_ARG_PrintHelp(u32Handle,opts);
        return HI_SUCCESS;
    }

    s32Rtn = HI_ARG_Parser(argc, argv, opts);
    if(s32Rtn != HI_SUCCESS)
    {
        pszPrintSz =    "unrecongnized or incomplete command line.\r\n"
                        "USAGE:\r\n"
                        ARG_S4"thread [0|pid]\r\n";

        HI_OUT_Printf(u32Handle,   "[Error][Data-Length:%d]\r\n%s",strlen(pszPrintSz),pszPrintSz);
        return HI_FAILURE;
    }

    if (HI_SUCCESS == HI_ARG_OptIsSet("?", opts))
    {
        HI_ARG_PrintHelp(u32Handle,opts);
        return HI_SUCCESS;
    }

    if ( HI_PTHREAD_INVALID_THREADID ==  _s32Pid)
    {
        _s32Pid = HI_PTHREAD_INVALID_THREADID;
    }

    s32Rtn = HI_PthreadMNG_PrintInfo(u32Handle,_s32Pid);

    if ( HI_SUCCESS != s32Rtn)
    {
        return HI_FAILURE;
    }
    return HI_SUCCESS;

}
#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#else

HI_S32 HI_PthreadMNG_Init()
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Deinit()
{
    return HI_SUCCESS;
}

HI_S32  HI_PthreadMNG_CreateD(HI_CHAR* pszPthreadInfo,HI_Pthread_T *pu32Threadp,
                             const HI_ThreadAttr_S *pstruAttr,
                             HI_ThreadFun pfunStart_routine, HI_VOID* pvArg,
                             HI_CHAR* pszFile,HI_S32 s32Line)
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_RegisterEndProc(HI_Pthread_T u32Thread,
                                    HI_PTR_FN_THREAD_ENDProc pfunEndProc,
                                    HI_VOID* pvParam)
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Join(HI_Pthread_T u32Thread)
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Add(HI_Pthread_T u32Thread, HI_CHAR* pszPthreadInfo)
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Del(HI_Pthread_T u32Thread)
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Exit()
{
    return HI_SUCCESS;
}

HI_S32 HI_PthreadMNG_Wait()
{
    return HI_SUCCESS;
}

//HI_S32 HI_PthreadMNG_Query(HI_Pthread_T u32Thread);

HI_S32 HI_PthreadMNG_PrintInfo(HI_U32 u32OutHandle, HI_PID_T u32ThreadPID)
{
    return HI_SUCCESS;
}

/*
thread
    -pid [0~]   ;打印指定线程PID信息，0代表打印所以线程
*/
#if (UNF_VERSION_CODE >= UNF_VERSION(3, 1))
HI_S32 HI_CMD_ThreadList(HI_PROC_SHOW_BUFFER_S * pstBuf, HI_VOID *pPrivData)
{
    return HI_SUCCESS;
}

#endif
#if 0
HI_S32 HI_CMD_ThreadList(HI_U32 u32Handle, HI_S32 argc, const char* argv[])
{
    return HI_SUCCESS;
}

#endif

#endif/*HI_ADVCA_FUNCTION_RELEASE*/
