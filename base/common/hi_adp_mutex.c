/***********************************************************************************
*             Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_mutex.c
* Description:the interface of mutex function adpter for all module
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-27   q63946     NULL         Create this file.
***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "common/hi_adp_mutex.h"

#if HI_OS_TYPE == HI_OS_LINUX
#include <pthread.h>

#elif HI_OS_TYPE == HI_OS_WIN32
#include <windows.h>

#endif

#ifdef  __cplusplus
#if  __cplusplus
extern "C"{
#endif
#endif

/***************************************************************************
***********                            MUTEX »¥³â                                       **********
****************************************************************************/
HI_S32 HI_MutexRecursiveInit(HI_ThreadMutex_S *mutex, HI_ThreadAttr_S *mutex_attr)
{
#if HI_OS_TYPE == HI_OS_LINUX
        pthread_mutexattr_init((pthread_mutexattr_t *)mutex_attr);
#ifdef __USE_UNIX98
        pthread_mutexattr_settype((pthread_mutexattr_t *)mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
        return pthread_mutex_init((pthread_mutex_t *)mutex,(pthread_mutexattr_t *)mutex_attr);
#elif HI_OS_TYPE == HI_OS_WIN32
        return HI_FAILURE;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*pthread_mutex_init, for windows, the args attr is nouse*/
HI_S32  HI_MutexInit (HI_ThreadMutex_S *mutex,const HI_ThreadAttr_S *mutex_attr)
{
    const HI_ThreadAttr_S *temp = mutex_attr;
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_mutex_init((pthread_mutex_t *)mutex,(pthread_mutexattr_t *)temp);
#elif HI_OS_TYPE == HI_OS_WIN32
    temp=temp;
    InitializeCriticalSection((LPCRITICAL_SECTION)mutex);
    return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*pthread_mutex_destroy, for windeos, mutex is handle*/
HI_S32 HI_MutexDestroy (HI_ThreadMutex_S *mutex)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_mutex_destroy((pthread_mutex_t *)mutex);
#elif HI_OS_TYPE == HI_OS_WIN32
     DeleteCriticalSection((LPCRITICAL_SECTION)mutex);
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/* Try to lock MUTEX.
   Only for linux
*/
HI_S32 HI_MutexTryLock (const HI_ThreadMutex_S *mutex)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_mutex_trylock((pthread_mutex_t *)mutex);
#elif HI_OS_TYPE == HI_OS_WIN32
       return  HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/* Wait until lock for MUTEX becomes available and lock it.  */
HI_S32 HI_MutexLock (HI_ThreadMutex_S *mutex)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_mutex_lock((pthread_mutex_t *)mutex);
#elif HI_OS_TYPE == HI_OS_WIN32
    EnterCriticalSection((LPCRITICAL_SECTION)mutex);
    return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/* Unlock MUTEX.  */
HI_S32 HI_MutexUnLock (HI_ThreadMutex_S *mutex)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_mutex_unlock((pthread_mutex_t *)mutex);
#elif HI_OS_TYPE == HI_OS_WIN32
     LeaveCriticalSection((LPCRITICAL_SECTION)mutex);
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* end of __cplusplus */
