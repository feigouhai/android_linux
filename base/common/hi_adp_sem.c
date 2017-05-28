/******************************************************************************
 *
 *             Copyright 2009 - 2009, Hisilicon Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 *
 ******************************************************************************
 * File Name     : hi_adpt_sem.c
 * Description   : 信号量实现
 *
 * History       :
 * Version     Date        Author      DefectNum    Modification:
 * 1.1         2009-2-27   q63946      NULL         Created file
 *
 ******************************************************************************/

#include "hi_type.h"
#include "common/hi_adp_sem.h"

#if HI_OS_TYPE == HI_OS_LINUX

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#elif HI_OS_TYPE == HI_OS_WIN32
#include <Windows.h>
#include <Winbase.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

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

/*----------------------------------------------*
 * error code
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables
 *----------------------------------------------*/

/*----------------------------------------------*
 * routine prototypes
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines
 *----------------------------------------------*/

HI_S32 HI_SEM_Init(HI_SEM_T *pSem, HI_S32 pshared, HI_U32 value)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_init((sem_t *)pSem, pshared, value);
#elif HI_OS_TYPE == HI_OS_WIN32
    HANDLE hdlSemtmp;
    HI_S32 a,b;
    hdlSemtmp = (HI_SEM_T *)CreateSemaphore(HI_NULL, value, 100, HI_NULL);
    if (NULL == hdlSemtmp)
    {
        return HI_FAILURE;
    }
    a= sizeof(HI_SEM_T);
    b= sizeof(HANDLE);
    memcpy(pSem,&hdlSemtmp,sizeof(hdlSemtmp));
#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_Destroy(HI_SEM_T *pSem)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_destroy((sem_t *)pSem);
#elif HI_OS_TYPE == HI_OS_WIN32
    CloseHandle(*(HANDLE*)pSem);
#endif
    return HI_SUCCESS;
}

HI_SEM_T * HI_SEM_Open(const HI_CHAR *pszName, HI_S32 s32Oflag,HI_MODE_T tMode,
                   HI_U32 u32Value)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return (HI_SEM_T *)sem_open(pszName, s32Oflag, tMode, u32Value);
#elif HI_OS_TYPE == HI_OS_WIN32

#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_Close(HI_SEM_T *pSem)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_close((sem_t *)pSem);
#elif HI_OS_TYPE == HI_OS_WIN32

#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_Wait(HI_SEM_T *pSem)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_wait((sem_t *)pSem);
#elif HI_OS_TYPE == HI_OS_WIN32
    WaitForSingleObject(*(HANDLE*)pSem, INFINITE);
#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_TryWait(HI_SEM_T *pSem)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_trywait((sem_t *)pSem);
#elif HI_OS_TYPE == HI_OS_WIN32

#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_TimedWait(HI_SEM_T *pSem,const HI_Timespec_S* pstAbs_timeout)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_timedwait((sem_t *)pSem, (const struct timespec*)pstAbs_timeout);
#elif HI_OS_TYPE == HI_OS_WIN32

#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_Post(HI_SEM_T *pSem)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_post((sem_t *)pSem);
#elif HI_OS_TYPE == HI_OS_WIN32
    ReleaseSemaphore(*(HANDLE*)pSem, 1, HI_NULL);
#endif
    return HI_SUCCESS;
}

HI_S32 HI_SEM_Getvalue(HI_SEM_T *pSem,HI_S32* ps32Sval)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sem_getvalue((sem_t *)pSem, ps32Sval);
#elif HI_OS_TYPE == HI_OS_WIN32

#endif
    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
