/***********************************************************************************
*             Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_thread.c
* Description:the interface of thread function adpter for all module
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-10   q63946  NULL         Create this file.
***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "hi_type.h"
#include "common/hi_ref_errno.h"
#include "common/hi_ref_mod.h"
#include "common/hi_adp_mutex.h"
#include "common/hi_adp_thread.h"
#include <stdlib.h>
#include "common/hi_adp_interface.h"

#if HI_OS_TYPE == HI_OS_LINUX
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <pthread.h>
#elif HI_OS_TYPE == HI_OS_WIN32

#include <windows.h>
#include <winbase.h>
#include <process.h>
//HI_S32 errno = 0;
#endif

#ifdef  __cplusplus
#if  __cplusplus
extern "C"{
#endif
#endif

/*返回errno*/
HI_S32 HI_GetLastErr(HI_VOID)
{
    return errno;
}
/*设置errno*/
HI_VOID HI_SetLastErr(HI_S32 newErrNo)
{
    errno = newErrNo;
}

HI_S32 HI_Execv(const HI_CHAR *path, HI_CHAR *const argv[])
{
#if HI_OS_TYPE == HI_OS_LINUX
     return execv(path,argv);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _execv(path,argv);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Execvp(const HI_CHAR *file, HI_CHAR *const argv[])
{
#if HI_OS_TYPE == HI_OS_LINUX
     return execvp(file,argv);
#elif HI_OS_TYPE == HI_OS_WIN32
    return _execvp(file,argv);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Waitpid(HI_PID_T pid,  HI_S32 *status, HI_S32 options)
    {
#if HI_OS_TYPE == HI_OS_LINUX
     return waitpid(pid,status,options);
#elif HI_OS_TYPE == HI_OS_WIN32
    HI_S32 result;
    options = options;
    result=GetExitCodeProcess((HANDLE)pid,status);
    if(0==result)
    {
        return -1;
    }
    return result;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

#if HI_OS_TYPE == HI_OS_WIN32

HI_PID_T HI_CreateProcess(HI_CHAR *lpszImageName, HI_CHAR *lpszCmdLine, HI_U32 fdwCreate,HI_PROCESS_S_INFORMATION * lppiProcInfo)
{
    int result=0;
    result= CreateProcess(  lpszImageName, lpszCmdLine, HI_NULL_PTR, HI_NULL_PTR, HI_NULL_PTR, fdwCreate,
                                       HI_NULL_PTR, HI_NULL_PTR, HI_NULL_PTR,(LPPROCESS_INFORMATION) lppiProcInfo);
    if(result == 0)
    {
        return 0;
    }
    else
    {
        return (HI_PID_T)lppiProcInfo->hProcess;
    }

}
#endif

#ifndef HI_OS_SUPPORT_UCLINUX

HI_PID_T HI_Fork(HI_VOID)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return fork();
#elif HI_OS_TYPE == HI_OS_WIN32
    return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}
#endif

HI_S32 HI_PthreadAttrInit(HI_ThreadAttr_S *attr)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_attr_init((pthread_attr_t *)attr);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadAttrDestroy(HI_ThreadAttr_S *attr)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return pthread_attr_destroy((pthread_attr_t *)attr);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadAttrSetdetachstate(HI_ThreadAttr_S *attr, HI_S32 detachstate)
{
 #if HI_OS_TYPE == HI_OS_LINUX
   return pthread_attr_setdetachstate((pthread_attr_t *)attr,detachstate);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}
/*
int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
*/

HI_S32 HI_PthreadSetCancelState(HI_S32 state, HI_S32 *oldstate)
{
#if HI_OS_TYPE == HI_OS_LINUX

#if defined(ANDROID_VERSION)
    return HI_SUCCESS;
#else
    return pthread_setcancelstate(state,oldstate);
#endif

#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadSetCancelType(HI_S32 type, HI_S32 *oldtype)
{
#if HI_OS_TYPE == HI_OS_LINUX

#if defined(ANDROID_VERSION)
    return HI_SUCCESS;
#else
    return pthread_setcanceltype(type,oldtype);
#endif

#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadDetach (HI_Pthread_T th)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return pthread_detach(th);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_GetPriority(HI_S32 which, HI_S32 who)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return getpriority(which,who);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_SetPriority(HI_S32 which, HI_S32 who, HI_S32 prio)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return setpriority(which,who,prio);
#elif HI_OS_TYPE == HI_OS_WIN32
     return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32  HI_PthreadCreate (HI_Pthread_T *threadp, const HI_ThreadAttr_S *attr,
                         HI_ThreadFun start_routine, HI_VOID *arg)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return pthread_create((pthread_t *)threadp,  (pthread_attr_t *)attr, start_routine,  arg);
#elif HI_OS_TYPE == HI_OS_WIN32
    HANDLE hThread;
    HI_U32 threadid;
    attr=attr;
    hThread = CreateThread( NULL,                                   /* no security attributes */
                            0,                                      /* use default stacksize  */
                            (LPTHREAD_START_ROUTINE)start_routine,  /* thread function */
                            arg,                                    /* argument to thread function */
                            0,                                      /* use default creation flags */
                            &threadid);                             /* returns the thread identifier */
    if(hThread == HI_NULL_PTR)
    {
        return HI_ERR_OSCALL_ERROR;
    }
    *threadp = (HI_Pthread_T)hThread;
    return HI_SUCCESS;
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_Pthread_T  HI_PthreadSelf (HI_VOID)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return pthread_self();
#elif HI_OS_TYPE == HI_OS_WIN32
     return GetCurrentThreadId();
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_VOID HI_PthreadExit ( HI_VOID* retVal)
{
#if HI_OS_TYPE == HI_OS_LINUX
    pthread_exit(retVal);
#elif HI_OS_TYPE == HI_OS_WIN32
    /* *(HI_S32 *)retVal = 0;
       _endthread();*/
      ExitThread(*(HI_U32 *)retVal);
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadCancel(HI_Pthread_T thread)
{
#if HI_OS_TYPE == HI_OS_LINUX

#if defined(ANDROID_VERSION)
    return HI_SUCCESS;
#else
    return pthread_cancel(thread);
#endif

#elif HI_OS_TYPE == HI_OS_WIN32
    HI_S32 result;
    result =  TerminateThread ((HANDLE)thread,0);
    if(result != 0)
    {
        return HI_SUCCESS;
    }
    else
    {
        return HI_ERR_OSCALL_ERROR;

}
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_PthreadJoin (HI_Pthread_T th, HI_VOID **thread_return)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return pthread_join(th,thread_return);
#elif HI_OS_TYPE == HI_OS_WIN32
    thread_return=thread_return;
    return (HI_S32)WaitForSingleObject((void *)th,HI_INFINITE);
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_Kill(HI_PID_T pid, HI_S32 sig)
{
#if HI_OS_TYPE == HI_OS_LINUX
     return kill(pid,sig);

#elif HI_OS_TYPE == HI_OS_WIN32
       if(0 == TerminateProcess( (HANDLE)pid, sig))
       {
           return HI_ERR_OSCALL_ERROR;
       }

    else
       {
           return HI_SUCCESS;
}
#else
#error YOU MUST DEFINE OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*秒级sleep*/
HI_U32 HI_Sleep(HI_U32 seconds)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return sleep(seconds);
#elif HI_OS_TYPE == HI_OS_WIN32
    Sleep(seconds * 1000);
    return (seconds * 1000);
#endif
}

/*毫秒级sleep*/
HI_U32 HI_SleepMs(HI_U32 ms)
{
#if HI_OS_TYPE == HI_OS_LINUX
    return usleep(ms*1000);
#elif HI_OS_TYPE == HI_OS_WIN32
    Sleep(ms);
    return (ms);
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* end of __cplusp*/
