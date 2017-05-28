#include <time.h>
#include <string.h>
#include <sys/time.h>

#if defined(ANDROID_VERSION)
#include <linux/ioctl.h>
#include <linux/rtc.h>
#include <utils/Atomic.h>
#include <linux/android_alarm.h>

#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <utils/Timers.h>

#include <stdint.h>
#include <sys/types.h>
#endif

#include "common/hi_time.h"
#include "common/hi_ref_errno.h"
#include "Logger.h"

#define TIME_MCR_MJD_1970     (40587)     /*y/m/d=1970/01/01    h/m/s=00/00/00 */

#define TIME_SYSTIME2DATATIME(pSysTm,pDataTime)    \
do  \
{   \
   (pDataTime)->s32Sec   = (pSysTm)->tm_sec        ; \
   (pDataTime)->s32Min   = (pSysTm)->tm_min        ; \
   (pDataTime)->s32Hour  = (pSysTm)->tm_hour       ; \
   (pDataTime)->s32Mday  = (pSysTm)->tm_mday       ; \
   (pDataTime)->s32Mon   = (pSysTm)->tm_mon  + 1   ; \
   (pDataTime)->s32Year  = (pSysTm)->tm_year + 1900; \
   (pDataTime)->s32Wday  = (pSysTm)->tm_wday       ; \
   (pDataTime)->s32Yday  = (pSysTm)->tm_yday + 1   ; \
}   \
while(0)

#define TIME_DATATIME2SYSTIME(pDataTime,pSysTm)    \
do  \
{   \
    (pSysTm)->tm_sec  = (pDataTime)->s32Sec        ; \
    (pSysTm)->tm_min  = (pDataTime)->s32Min        ; \
    (pSysTm)->tm_hour = (pDataTime)->s32Hour       ; \
    (pSysTm)->tm_mday = (pDataTime)->s32Mday       ; \
    (pSysTm)->tm_mon  = (pDataTime)->s32Mon  - 1   ; \
    (pSysTm)->tm_year = (pDataTime)->s32Year - 1900; \
    (pSysTm)->tm_wday = (pDataTime)->s32Wday       ; \
    (pSysTm)->tm_yday = (pDataTime)->s32Yday - 1   ; \
}   \
while(0)

// check param is valid
#define TIME_CHK_PARA(val)        \
do                                  \
{                                   \
    if ((val))                      \
    {                               \
        HI_LOGE("para invalid");    \
        return HI_EINVAL;           \
    };                              \
} while (0)

#define TIME_CHK_RETURN_ERR(val, ret )        \
do                                              \
{                                               \
    if ((val))                                  \
    {                                           \
        HI_LOGE("value invalid");               \
        return ret;                             \
    }                                           \
} while (0)

#define TIME_DOFUNC_RETURN( func )            \
do                                              \
{                                               \
    HI_S32 ret = 0;                             \
    ret = func;                                 \
    if (ret != HI_SUCCESS)                      \
    {                                           \
        HI_LOGE("CALL %s, ret is %d ", # func, ret);  \
        return ret;                             \
    };                                          \
} while (0)

#if 0
#define TIME_DATATIME2SYSTIME(pTime)  do{\
    (pTime)->s32Year -= 1900;   \
    (pTime)->s32Mon -= 1;   \
    (pTime)->s32Yday -= 1;  \
}while(0)

#define TIME_SYSTIME2DATATIME(pTime)  do{\
    (pTime)->s32Year += 1900;   \
    (pTime)->s32Mon += 1;   \
    (pTime)->s32Yday += 1;  \
}while(0)
#endif

typedef struct tagTIME_PRIVATE_DATA_S
{
    HI_S32 s32TimeZone;         /**< Unit is second, Less than 0 possibly. */
    HI_U32 u32DaylightSaving;   /**< Unit is second. */
    HI_S32 s32UserSeconds;      /* The time is set by HI_TIME_SetDateTime, not include the Offset. */
    HI_S32 s32LastSystemSeconds;/* Seconds elapsed from 1970.1.1, 0:0:0 */
    HI_BOOL bModifyTDTStime;       /**< If modify operate system time by TDT. */
    HI_BOOL bModifyTOTStime;       /**< If modify operate system time by TOT. */
} TIME_PRIVATE_DATA_S;

static HI_BOOL s_bTimeInited = HI_FALSE;
static TIME_PRIVATE_DATA_S s_stTimePrivData = {0};

static void TIME_LogTime(const HI_TIME_INFO_S *pstTimeInfo)
{
    HI_LOGD("%d-%d-%d, %d:%d:%d, wday = %d",
        pstTimeInfo->s32Year, pstTimeInfo->s32Mon,
        pstTimeInfo->s32Mday, pstTimeInfo->s32Hour,
        pstTimeInfo->s32Min, pstTimeInfo->s32Sec,
        pstTimeInfo->s32Wday);
}

#if defined(ANDROID_VERSION)
static int setCurrentTimeMillis(int64_t millis)
{
    struct timeval tv;
    struct timespec ts;
    int fd;
    int res;
    int ret = 0;

    if (millis <= 0 || millis / 1000LL >= INT_MAX) {
        return -1;
    }

    tv.tv_sec = (time_t) (millis / 1000LL);
    tv.tv_usec = (suseconds_t) ((millis % 1000LL) * 1000LL);

    //HI_LOG_DEBUG("Setting time of day to sec=%d\n", (int) tv.tv_sec);

    fd = open("/dev/alarm", O_RDWR);
    if(fd < 0)
    {
        HI_LOGE("Unable to open alarm driver: %s\n", strerror(errno));
        return -1;
    }
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
    res = ioctl(fd, ANDROID_ALARM_SET_RTC, &ts);
    if(res < 0)
    {
        HI_LOGE("Unable to set rtc to %ld: %s\n", tv.tv_sec, strerror(errno));
        ret = -1;
    }
    close(fd);

    return ret;
}
#endif

static HI_S32 TIME_MJDToSeconds(HI_U32 u32Mjd, HI_S32 *ps32Seconds)
{
    HI_S32 s32UtcSecond = 0;

    TIME_CHK_PARA(HI_NULL == ps32Seconds);

    s32UtcSecond = u32Mjd & 0xFFFF;
    TIME_CHK_PARA((TIME_MCR_MJD_1970-70) >= s32UtcSecond);

    s32UtcSecond -= TIME_MCR_MJD_1970;    /* number of days since  1970/01/01 */
    s32UtcSecond *= 24*(60 * 60);   /* number of days (in seconds unit) */

    *ps32Seconds = s32UtcSecond;
    return HI_SUCCESS;
}

static HI_S32 TIME_SecondsToMJD(HI_S32 s32Seconds, HI_U32* pu32Mjd)
{
    HI_S32 s32UtcSecond = s32Seconds;

    TIME_CHK_PARA(HI_NULL == pu32Mjd);

    s32UtcSecond /= (24*(60 * 60));     /* number of days */
    s32UtcSecond += TIME_MCR_MJD_1970;    /* number of days since  1970/01/01 */

    *pu32Mjd = (HI_U32)s32UtcSecond;
    return HI_SUCCESS;
}

static HI_S32 TIME_GetOffset()
{
    HI_S32 s32TimeZone = 0;
    (HI_VOID)HI_TIME_GetTimeZone(&s32TimeZone);
    return s32TimeZone + (HI_S32)s_stTimePrivData.u32DaylightSaving;
}

static HI_U32 TIME_GetTimeNow(HI_VOID)
{
    struct timespec ts;
    int ret = 0;
    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if(ret < 0)
        return 0;
    else
        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

HI_S32 HI_TIME_Init(const HI_VOID *pVoid)
{
    TIME_CHK_RETURN_ERR(HI_TRUE == s_bTimeInited, HI_SUCCESS);

    memset(&s_stTimePrivData, 0, sizeof(TIME_PRIVATE_DATA_S));
    s_stTimePrivData.bModifyTDTStime = HI_TRUE;
    s_stTimePrivData.bModifyTOTStime = HI_TRUE;

    s_bTimeInited = HI_TRUE;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_DeInit(const HI_VOID *pVoid)
{
    TIME_CHK_RETURN_ERR(HI_FALSE == s_bTimeInited, HI_SUCCESS);

    s_bTimeInited = HI_FALSE;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_GetTimeZone(HI_S32 *ps32TimeZoneSeconds)
{
    TIME_CHK_PARA(HI_NULL == ps32TimeZoneSeconds);

    if (s_stTimePrivData.bModifyTOTStime)
    {
        struct timeval tv;
        struct timezone tz;

        memset(&tv, 0, sizeof(tv));
        memset(&tz, 0, sizeof(tz));
        HI_S32 s32Ret = gettimeofday(&tv, &tz);
        if (s32Ret != 0)
        {
            HI_LOGE("gettimeofday error, s32Ret = %d", s32Ret);
        }

        *ps32TimeZoneSeconds = tz.tz_minuteswest * 60;
    }
    else
    {
        *ps32TimeZoneSeconds = s_stTimePrivData.s32TimeZone;
    }

    return HI_SUCCESS;
}

HI_S32 HI_TIME_SetTimeZone(HI_S32 s32TimeZoneSeconds)
{
    const HI_S32 TIME_ZONE_THRESHOLD = 12;
    HI_S32 s32TimeZoneHours = s32TimeZoneSeconds / (60 * 60);

//    HI_LOG_DEBUG("Set time zone %ds = %dh", s32TimeZoneSeconds, s32TimeZoneHours);

    if (s32TimeZoneHours < -TIME_ZONE_THRESHOLD || TIME_ZONE_THRESHOLD < s32TimeZoneHours)
    {
        HI_LOGE("Time zone out of threshold, %ds = %dh", s32TimeZoneSeconds, s32TimeZoneHours);
        return HI_EINVAL;
    }

    if (s_stTimePrivData.bModifyTOTStime)
    {
        struct timeval tv;
        struct timezone tz;
        HI_S32 s32TimeZoneMinutes = s32TimeZoneSeconds / 60;

        memset(&tv, 0, sizeof(tv));
        memset(&tz, 0, sizeof(tz));
        HI_S32 s32Ret = gettimeofday(&tv, &tz);
        tz.tz_minuteswest = s32TimeZoneMinutes;
        s32Ret |= settimeofday(&tv, &tz);
        if (s32Ret != 0)
        {
            HI_LOGE("Set time zone to system error, s32Ret = %d", s32Ret);
        }
    }
    else
    {
        s_stTimePrivData.s32TimeZone = s32TimeZoneSeconds;
    }

    return HI_SUCCESS;
}

HI_S32 HI_TIME_SetDaylight(HI_BOOL bEnable)
{
    if (bEnable)
    {
        s_stTimePrivData.u32DaylightSaving = 1 * 60 * 60;
    }
    else
    {
        s_stTimePrivData.u32DaylightSaving = 0;
    }

    return HI_SUCCESS;
}

HI_S32 HI_TIME_GetOffset(HI_S32 *ps32OffsetSeconds)
{
    TIME_CHK_PARA(HI_NULL == ps32OffsetSeconds);

    *ps32OffsetSeconds = TIME_GetOffset();

    return HI_SUCCESS;
}

HI_S32 HI_TIME_GetSeconds(HI_S32 *ps32Seconds, HI_BOOL bWithOffset)
{
    HI_S32 s32systemTime = 0;

    TIME_CHK_PARA(HI_NULL == ps32Seconds);

    HI_S32 s32Ret = HI_SUCCESS;
    if (s_stTimePrivData.s32UserSeconds <= 0)
    {
        HI_LOGD("User did not set data time previously.");
        s32Ret = HI_FAILURE;
    }

    /* Calculate seconds of passed */
    if (s_stTimePrivData.bModifyTDTStime)/* avoid get incorrect time when system time be modified by another app */
    {
        time_t systemTime = 0;
        (HI_VOID)time(&systemTime);
        s32systemTime = (HI_S32)systemTime;
    }
    else
    {
        HI_S32 systemTime = (HI_S32)TIME_GetTimeNow()/1000;
        s32systemTime = systemTime;
    }

    HI_S32 s32Difference = s32systemTime - s_stTimePrivData.s32LastSystemSeconds;

    *ps32Seconds = s_stTimePrivData.s32UserSeconds + s32Difference;
    if (bWithOffset)
    {
        *ps32Seconds += TIME_GetOffset();
    }

    return s32Ret;
}

HI_S32 HI_TIME_GetDateTime(HI_TIME_INFO_S *pstTimeInfo, HI_BOOL bWithOffset)
{
    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

    HI_S32 sec = 0;
    HI_S32 s32Ret = HI_TIME_GetSeconds(&sec, bWithOffset);
    s32Ret |= HI_TIME_SecondsToDateTime(sec, pstTimeInfo);

    return s32Ret;
}

HI_S32 HI_TIME_SetDateTime(const HI_TIME_INFO_S *pstTimeInfo, HI_BOOL bWithOffset)
{
    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

//    HI_LOG_DEBUG("Set time to %d-%d-%d, %d:%d:%d, wday = %d", pstTimeInfo->s32Year, pstTimeInfo->s32Mon, pstTimeInfo->s32Mday, pstTimeInfo->s32Hour, pstTimeInfo->s32Min, pstTimeInfo->s32Sec, pstTimeInfo->s32Wday);

    HI_S32 s32Seconds = 0;
    TIME_DOFUNC_RETURN(HI_TIME_DateTimeToSeconds(pstTimeInfo, &s32Seconds));
    if (bWithOffset)
    {
        s32Seconds -= TIME_GetOffset();
    }

    if (s_stTimePrivData.bModifyTDTStime)
    {
    /* Modify operate system time. */
    // HI_LOG_DEBUG("Modify operate system time, seconds = 0x%x", s32Seconds);
#if defined(ANDROID_VERSION)
        HI_S64 s64MillSec = (HI_S64)s32Seconds * 1000;
        (HI_VOID)setCurrentTimeMillis(s64MillSec);
#else
        (HI_VOID)stime((time_t *)&s32Seconds);
#endif
        (HI_VOID)time((time_t *)&s_stTimePrivData.s32LastSystemSeconds);
    }
    else
    {
        s_stTimePrivData.s32LastSystemSeconds = (HI_S32)TIME_GetTimeNow()/1000;
    }

    s_stTimePrivData.s32UserSeconds = s32Seconds;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_SetTimeToSystem(HI_BOOL bEnable)
{
    s_stTimePrivData.bModifyTDTStime = bEnable;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_SetTimeZoneToSystem(HI_BOOL bEnable)
{
    s_stTimePrivData.bModifyTOTStime = bEnable;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_GetMJDUTCTime(HI_TIME_MJDUTC_S *pstMjdUtcTime, HI_BOOL bWithOffset)
{
    TIME_CHK_PARA(HI_NULL == pstMjdUtcTime);

    HI_S32 sec = 0;
    HI_S32 s32Ret = HI_TIME_GetSeconds(&sec, bWithOffset);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    return HI_TIME_SecondsToMJDUTC(sec, pstMjdUtcTime);
}

HI_S32 HI_TIME_CompareDateTime(const HI_TIME_INFO_S *pstTimeInfo1, const HI_TIME_INFO_S *pstTimeInfo2, HI_S32 *ps32Result)
{
    HI_S32 s32Rtn;
    HI_S32 s32Second1 = 0, s32Second2 = 0;

    TIME_CHK_PARA((HI_NULL == pstTimeInfo1) || (HI_NULL == pstTimeInfo2)
        || (HI_NULL == ps32Result));

    s32Rtn = HI_TIME_DateTimeToSeconds(pstTimeInfo1, &s32Second1);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    s32Rtn = HI_TIME_DateTimeToSeconds(pstTimeInfo2, &s32Second2);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    *ps32Result = s32Second1-s32Second2;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_CompareMJDUTCTime(const HI_TIME_MJDUTC_S *pstTimeInfo1, const HI_TIME_MJDUTC_S *pstTimeInfo2, HI_S32 *ps32Result)
{
    HI_S32 s32Rtn;
    HI_S32 s32Second1 = 0, s32Second2 = 0;

    TIME_CHK_PARA((HI_NULL == pstTimeInfo1) || (HI_NULL == pstTimeInfo2)
        || (HI_NULL == ps32Result));

    //HI_LOG_ERROR("pstTimeInfo1, MJD = %d, UTC = %d", pstTimeInfo1->u16MJD, pstTimeInfo1->u32UTC);
    //HI_LOG_ERROR("pstTimeInfo2, MJD = %d, UTC = %d", pstTimeInfo2->u16MJD, pstTimeInfo2->u32UTC);

    s32Rtn = HI_TIME_MJDUTCToSeconds(pstTimeInfo1, &s32Second1);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    s32Rtn = HI_TIME_MJDUTCToSeconds(pstTimeInfo2, &s32Second2);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    *ps32Result = s32Second1-s32Second2;

    return HI_SUCCESS;
}

/**
 * ps32Seconds: second count, from 1970 year.
 */
HI_S32 HI_TIME_DateTimeToSeconds(const HI_TIME_INFO_S *pstTimeInfo, HI_S32* ps32Seconds)
{
    HI_S32 s32Seconds = 0;
    struct tm sysTm;

    TIME_CHK_PARA((HI_NULL == pstTimeInfo) || (HI_NULL == ps32Seconds));

    memset(&sysTm, 0, sizeof(sysTm));
    TIME_DATATIME2SYSTIME(pstTimeInfo, &sysTm);
    s32Seconds = mktime(&sysTm);
    if (s32Seconds == -1)
    {
        HI_LOGD("mktime return -1");
        TIME_LogTime(pstTimeInfo);
        return HI_FAILURE;
    }

#if defined(ANDROID_VERSION)
    struct timeval tv;
    struct timezone tz;

    memset(&tv, 0, sizeof(tv));
    memset(&tz, 0, sizeof(tz));
    gettimeofday(&tv, &tz);
    s32Seconds -= tz.tz_minuteswest * 60;
#else
    tzset();
    HI_LOGD( "daylight = %d\n", daylight );
    HI_LOGD( "timezone = %ld\n", timezone );
    HI_LOGD( "tzname[0] = %s\n", tzname[0] );
    s32Seconds -= timezone;
#endif

    *ps32Seconds = s32Seconds;
    return HI_SUCCESS;
}

/**
 * s32Seconds: second count, from 1970 year.
 */
HI_S32 HI_TIME_SecondsToDateTime(HI_S32 s32Seconds, HI_TIME_INFO_S *pstTimeInfo)
{
    struct tm *pSysTm = HI_NULL;
    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

    pSysTm = gmtime((const time_t *)&s32Seconds);
    if (pSysTm != HI_NULL)
    {
        TIME_SYSTIME2DATATIME(pSysTm, pstTimeInfo);
    }

    return HI_SUCCESS;
}

HI_S32 HI_TIME_MJDToYMD(HI_U32 u32MJD, HI_TIME_INFO_S *pstTimeInfo)
{
    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

    HI_S32 seconds = 0;
    HI_S32 ret;
    HI_TIME_INFO_S stTimeInfo;

    ret = TIME_MJDToSeconds(u32MJD, &seconds);
    TIME_CHK_RETURN_ERR(ret != HI_SUCCESS, ret);

    memset(&stTimeInfo, 0, sizeof(HI_TIME_INFO_S));
    ret = HI_TIME_SecondsToDateTime(seconds, &stTimeInfo);
    TIME_CHK_RETURN_ERR(ret != HI_SUCCESS, ret);

    pstTimeInfo->s32Year = stTimeInfo.s32Year;
    pstTimeInfo->s32Mon = stTimeInfo.s32Mon;
    pstTimeInfo->s32Mday = stTimeInfo.s32Mday;
    pstTimeInfo->s32Wday = stTimeInfo.s32Wday;
    pstTimeInfo->s32Yday = stTimeInfo.s32Yday;
    //pstTimeInfo->s32Isdst = stTimeInfo.s32Isdst;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_BCDUTCToHMS(HI_U32 u32UTC, HI_TIME_INFO_S *pstTimeInfo)
{
    HI_U32 u32H,u32M,u32S;

    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

    u32H = (u32UTC&0xFF0000)>>16;
    u32M = (u32UTC&0x00FF00)>>8;
    u32S = (u32UTC&0x0000FF);

    pstTimeInfo->s32Hour = (HI_S32)HI_TIME_BCD_DEC(u32H);
    pstTimeInfo->s32Min = (HI_S32)HI_TIME_BCD_DEC(u32M);
    pstTimeInfo->s32Sec = (HI_S32)HI_TIME_BCD_DEC(u32S);

    return HI_SUCCESS;
}

HI_S32 HI_TIME_HMSToBCDUTC(const HI_TIME_INFO_S *pstTimeInfo, HI_U32 *pu32UTC)
{
    HI_U32 u32H,u32M,u32S;

    /*0xHHMMSS*/

    TIME_CHK_PARA(HI_NULL == pstTimeInfo);
    TIME_CHK_PARA(HI_NULL == pu32UTC);

    u32H = (HI_U32)pstTimeInfo->s32Hour;
    u32M = (HI_U32)pstTimeInfo->s32Min;
    u32S = (HI_U32)pstTimeInfo->s32Sec;

    u32H = (((u32H%100)/10)<<4)+(u32H%10);
    u32M = (((u32M%100)/10)<<4)+(u32M%10);
    u32S = (((u32S%100)/10)<<4)+(u32S%10);

    u32H = (u32H&0xFF) << 16;
    u32M = (u32M&0xFF) << 8;
    u32S = u32S&0xFF;

    *pu32UTC = u32H|u32M|u32S;

    return HI_SUCCESS;
}

HI_S32 HI_TIME_BCDUTCToSeconds(HI_U32 u32UTC, HI_S32 *ps32Seconds)
{
    HI_U32 u32H,u32M,u32S;
    /*0xHHMMSS*/

    TIME_CHK_PARA(HI_NULL == ps32Seconds);

    u32H = (u32UTC&0xFF0000)>>16;
    u32M = (u32UTC&0x00FF00)>>8;
    u32S = (u32UTC&0x0000FF);

    *ps32Seconds  = (HI_S32)HI_TIME_BCD_DEC(u32H)*60*60;
    *ps32Seconds += (HI_S32)HI_TIME_BCD_DEC(u32M)*60;
    *ps32Seconds += (HI_S32)HI_TIME_BCD_DEC(u32S);

    return HI_SUCCESS;
}

HI_S32 HI_TIME_SecondsToBCDUTC(HI_S32 s32Seconds, HI_U32 *pu32UTC)
{
    HI_U32 u32H,u32M,u32S;
    /*0xHHMMSS*/

    if((HI_NULL == pu32UTC) || (0 > s32Seconds))
    {
        return HI_FAILURE;
    }

    u32H = (HI_U32)(s32Seconds/(60*60))%24;
    u32M = (HI_U32)(s32Seconds/60)%60;
    u32S = (HI_U32)s32Seconds%60;
    u32H = HI_TIME_BCD_ENC(u32H);
    u32M = HI_TIME_BCD_ENC(u32M);
    u32S = HI_TIME_BCD_ENC(u32S);

    u32H = u32H << 16;
    u32M = u32M << 8;

    //HI_LOG_DEBUG("H M S=%#x %#x %#x\n",u32H, u32M, u32S);
    *pu32UTC = u32H | u32M | u32S;
    return HI_SUCCESS;
}

HI_S32 HI_TIME_MJDUTCToSeconds(const HI_TIME_MJDUTC_S *pstMjdUtcTime, HI_S32 *ps32Seconds)
{
    HI_S32 s32Rtn;
    HI_S32 s32MjdSeconds = 0;
    HI_S32 s32UtcSeconds = 0;

    TIME_CHK_PARA((HI_NULL == pstMjdUtcTime) || (HI_NULL == ps32Seconds));

    s32Rtn = TIME_MJDToSeconds(pstMjdUtcTime->u16MJD, &s32MjdSeconds);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    s32Rtn = HI_TIME_BCDUTCToSeconds(pstMjdUtcTime->u32UTC, &s32UtcSeconds);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    *ps32Seconds = s32MjdSeconds + s32UtcSeconds;

    #if 0
    s32MjdSeconds += s32UtcSeconds/(24*60*60);
    s32UtcSeconds %= (24*60*60);
    *ps32Seconds = s32MjdSeconds
                  + ((s32MjdSeconds>=0)?(s32UtcSeconds):((24*60*60) - s32UtcSeconds));
    #endif
    return HI_SUCCESS;
}

HI_S32 HI_TIME_SecondsToMJDUTC(HI_S32 s32Seconds, HI_TIME_MJDUTC_S *pstMjdUtcTime)
{
    HI_S32 s32Rtn;

    TIME_CHK_PARA(HI_NULL == pstMjdUtcTime);

    HI_U32 u32Mjd = 0;
    s32Rtn = TIME_SecondsToMJD(s32Seconds, &u32Mjd);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);
    pstMjdUtcTime->u16MJD = (HI_U16)u32Mjd;

    s32Rtn = HI_TIME_SecondsToBCDUTC(s32Seconds, &pstMjdUtcTime->u32UTC);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    return HI_SUCCESS;
}

HI_S32 HI_TIME_MJDUTCToDateTime(const HI_TIME_MJDUTC_S *pstMjdUtcTime, HI_TIME_INFO_S *pstTimeInfo)
{
    HI_S32 s32Rtn;
    HI_S32 s32Seconds = 0;

    TIME_CHK_PARA((HI_NULL == pstMjdUtcTime) || (HI_NULL == pstTimeInfo));

    s32Rtn = HI_TIME_MJDUTCToSeconds(pstMjdUtcTime, &s32Seconds);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    s32Rtn = HI_TIME_SecondsToDateTime(s32Seconds, pstTimeInfo);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    return HI_SUCCESS;
}

HI_S32 HI_TIME_DateTimeToMJDUTC(const HI_TIME_INFO_S *pstTimeInfo, HI_TIME_MJDUTC_S *pstMjdUtcTime)
{
    HI_S32 s32Rtn;
    HI_S32 s32Seconds = 0;

    TIME_CHK_PARA((HI_NULL == pstTimeInfo) || (HI_NULL == pstMjdUtcTime));

    s32Rtn = HI_TIME_DateTimeToSeconds(pstTimeInfo, &s32Seconds);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    s32Rtn = HI_TIME_SecondsToMJDUTC(s32Seconds, pstMjdUtcTime);
    TIME_CHK_RETURN_ERR(s32Rtn != HI_SUCCESS, s32Rtn);

    return HI_SUCCESS;
}

HI_S32 HI_TIME_AddSeconds(HI_TIME_INFO_S *pstTimeInfo, HI_S32 s32Seconds)
{
    TIME_CHK_PARA(HI_NULL == pstTimeInfo);

    HI_S32 s32SecondTmp = 0;
    TIME_DOFUNC_RETURN(HI_TIME_DateTimeToSeconds(pstTimeInfo, &s32SecondTmp));

    s32SecondTmp += s32Seconds;

    TIME_DOFUNC_RETURN(HI_TIME_SecondsToDateTime(s32SecondTmp, pstTimeInfo));

    return HI_SUCCESS;
}
