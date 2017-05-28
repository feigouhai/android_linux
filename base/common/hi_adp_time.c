/***********************************************************************************
*             Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_time.c
* Description:the interface of time function adpter for all module
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-29   q63946  NULL         Create this file.
***********************************************************************************/

#include "common/hi_adp_time.h"
#include "common/hi_adp_string.h"

#include <stdlib.h>

#if HI_OS_TYPE == HI_OS_WIN32
#include <SYS\TIMEB.H>
#include <windows.h>
#include <winbase.h>
#elif HI_OS_TYPE == HI_OS_LINUX
#include <sys/time.h>
#endif
#ifdef  __cplusplus
#if  __cplusplus
extern "C"{
#endif
#endif
HI_TM_S g_stAdptTm;
/*将time_t timep 转换为tm*/
HI_S32 HI_LocalTime(const HI_TIME_T *ptTime,HI_TM_S *pstDateTime)
{
    time_t tSysTime;
    struct tm *pstSystm;

    if((HI_NULL_PTR == ptTime) || (HI_NULL_PTR == pstDateTime))
    {
        return HI_FAILURE;
    }

    tSysTime = (time_t)*ptTime;

    pstSystm = localtime(&tSysTime);
    if(HI_NULL_PTR == pstSystm)
    {
        return HI_FAILURE;
    }
    HI_ADPT_MCR_SYSTM2ADPTTM(*pstSystm, *pstDateTime);
    return HI_SUCCESS;
}

HI_SIZE_T HI_StrFtime(HI_CHAR*s, HI_SIZE_T max, const HI_CHAR *format,  const HI_TM_S *stime)
{
    struct tm stSystm;
    HI_ADPT_MCR_ADPTTM2SYSTM(*stime, stSystm);
    return strftime(s,max,format,(const struct tm *)&stSystm);
}
HI_CHAR *HI_Ctime(const HI_TIME_T *ptime)
{
    return ctime((time_t *)ptime);
}
/*获取随机数*/
HI_U32 HI_Random(HI_VOID)
{
#if HI_OS_TYPE == HI_OS_LINUX
   return random();
#elif HI_OS_TYPE == HI_OS_WIN32
    static int isSrand = 0;
    if(isSrand == 0)
    {
        srand ((HI_U32) time( HI_NULL ) );
        isSrand = 1;
    }
    return (HI_U32)rand();
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*获取系统时间，一个系统毫秒数,win下为开机毫秒数,linux下为由系统时间计算得出的毫秒数*/
HI_U64 HI_GetSystemTime(HI_VOID)
{
#if HI_OS_TYPE == HI_OS_LINUX
    struct timeval tv;
    HI_U64 time_sec = 0;
    HI_U64 time_usec = 0;
    gettimeofday(&tv,0);
    time_sec = tv.tv_sec;
    time_usec = tv.tv_usec;
   //return (unsigned int)(tv.tv_sec%10000000*1000+tv.tv_usec/1000);
    return (time_sec*1000+time_usec/1000);
#elif HI_OS_TYPE == HI_OS_WIN32
    return GetTickCount();
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*获取系统时间(00:00:00 UTC, January 1, 1970),秒数*/
HI_U32 HI_GetSysTimeSec(HI_VOID)
{
#if HI_OS_TYPE == HI_OS_LINUX
   return time(0);
#elif HI_OS_TYPE == HI_OS_WIN32
   struct _timeb tv;
    _ftime(&tv);
    return (HI_U32)( (HI_U32)tv.time+2208988800);/* 系统是从1970开始,ntp是 1900. 这里的2208988800 就是70年 秒数*/
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*获取当前的系统时间，年，月日时分秒毫秒*/
HI_VOID HI_GetTime(HI_TM_S* curdate)
{
    time_t  timeNow;
    struct tm* pstSystm;
    (HI_VOID)time(&timeNow);
    pstSystm = gmtime(&timeNow);
    if(NULL != pstSystm)
        HI_ADPT_MCR_SYSTM2ADPTTM(*pstSystm, *curdate);
    //(HI_VOID)memcpy(curdate,gmtime(&timeNow),sizeof(HI_TM_S));
}

HI_BOOL HI_Stime(time_t* ptime)
{
#if HI_OS_TYPE == HI_OS_LINUX

#if defined(ANDROID_VERSION)
    return HI_FALSE;
#else
    return stime(ptime)?HI_FALSE:HI_TRUE;
#endif

#elif HI_OS_TYPE == HI_OS_WIN32
    struct tm *pstruTm;
    SYSTEMTIME struSystemTime;

    pstruTm = gmtime(ptime);

    if(HI_NULL_PTR != pstruTm)
    {
        struSystemTime.wYear = pstruTm->tm_year;
        struSystemTime.wMonth = pstruTm->tm_mon;
        struSystemTime.wDayOfWeek = pstruTm->tm_wday;
        struSystemTime.wDay = pstruTm->tm_mday;
        struSystemTime.wHour = pstruTm->tm_hour;
        struSystemTime.wMinute = pstruTm->tm_min;
        struSystemTime.wSecond = pstruTm->tm_sec;
        struSystemTime.wMilliseconds = 0;

        return (HI_FALSE == SetSystemTime(&struSystemTime));
    }
    else
    {
        return HI_FALSE;
    }
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE == HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

HI_S32 HI_GmTime(const HI_TIME_T* ptTime, HI_TM_S* pstAdptTm)
{
    struct tm* pstSysTm;
    time_t tSysTime;

    if((HI_NULL_PTR == ptTime) || (HI_NULL_PTR == pstAdptTm))
    {
        return HI_FAILURE;
    }
    tSysTime = *ptTime;
    pstSysTm = gmtime(&tSysTime);
    if(HI_NULL_PTR == pstSysTm)
    {
        return HI_FAILURE;
    }

    HI_ADPT_MCR_SYSTM2ADPTTM(*pstSysTm, *pstAdptTm);

    return HI_SUCCESS;
}

HI_TIME_T HI_Mktime(HI_TM_S* pstAdptTm)
{
    struct tm stSysTm;
    time_t tTime;
    if(HI_NULL_PTR == pstAdptTm)
    {
        return HI_FAILURE;
    }

    HI_ADPT_MCR_ADPTTM2SYSTM(*pstAdptTm, stSysTm);

    tTime = mktime(&stSysTm);
    if(-1 != tTime)
    {
        HI_ADPT_MCR_SYSTM2ADPTTM(stSysTm, *pstAdptTm);
    }
    return tTime;
}

HI_U32 HI_GetTimeNow(HI_VOID)
{
    struct timespec ts;
    int ret = 0;
    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if(ret < 0)
        return 0;
    else
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif  /* end of __cplusplus */
