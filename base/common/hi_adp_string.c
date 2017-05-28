/***********************************************************************************
*             Copyright 2006 - 2006, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: hi_adpt_string.c
* Description:the interface of string function adpter for all module
*
* History:
* Version   Date         Author     DefectNum    Description
* 1.1       2006-04-10   q63946  NULL         Create this file.
* 1.2       2006-05-15   t41030     NULL         delete(D) some function.
***********************************************************************************/
/*
D HI_StrLen strlen
D HI_StrCpy strcpy
D HI_StrCmp strcmp
D HI_StrNCpy strncpy
D HI_StrNCmp strncmp
D HI_StrStr  strstr
D HI_StrCat  strcat
D HI_Strtol  strtol
D HI_Strtoul strtoul
D HI_StrChr  strchr
D HI_StrrChr strrchr
D HI_CharToLower tolower
D HI_StrToFloat atof
*/
#include "common/hi_adp_string.h"
#include "common/hi_ref_errno.h"
#include "common/hi_adp_interface.h"
#include "common/hi_adp_thread.h"
//#include "hi_adpt_mem.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#if HI_OS_TYPE == HI_OS_LINUX
 #include <ctype.h>

#elif HI_OS_TYPE == HI_OS_WIN32
#include <string.h>
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

HI_CHAR* HI_StrConcat (const HI_CHAR *string1, ...)
{
     HI_U32  l;
     va_list   args;
     HI_CHAR   *s;
     HI_CHAR   *concat;
     HI_CHAR   *ptr;

     l = 1 + strlen(string1);
     HI_VA_START (args, string1);
     s = HI_VA_ARG (args, HI_CHAR*);
     while (0!=strlen(s))
     {
        l += strlen (s);
        s = HI_VA_ARG (args, HI_CHAR*);
     }
     HI_VA_END (args);

     concat = (HI_CHAR*)malloc( l);
     if(HI_NULL_PTR == concat)
        {
            return HI_NULL_PTR;
        }
     ptr = concat;

     ptr = (HI_CHAR *)strncat (ptr, string1, strlen(string1) + 1);
     HI_VA_START (args, string1);
     s = HI_VA_ARG (args, HI_CHAR*);
     while (0!=strlen(s))
       {
         ptr = (HI_CHAR *)strncat (ptr, s, strlen(s) + 1);
         s = HI_VA_ARG (args, HI_CHAR*);
       }
     HI_VA_END (args);

     return concat;
}
/*将字符串转为全大写*/
HI_CHAR* HI_StrToUper(HI_CHAR *szSource)
{
 #if HI_OS_TYPE == HI_OS_LINUX
    HI_CHAR *temp;
    temp = szSource;
    while(*temp != '\0')
    {
        if('a'<= *temp && 'z'>= *temp)
        {
            *temp = (HI_CHAR)(*temp-32);
        }
        temp++;
    }
    return szSource;
#elif HI_OS_TYPE == HI_OS_WIN32
    return _strupr(szSource);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*将字符串转为全小写*/
HI_CHAR* HI_StrToLower(HI_CHAR *szSource)
{
#if HI_OS_TYPE == HI_OS_LINUX
   HI_CHAR *temp;
    temp = szSource;
    while(*temp != '\0')
    {
        if('A'<= *temp && 'Z'>= *temp)
        {
            *temp = (HI_CHAR)(*temp+32);
        }
        temp++;
    }
    return szSource;
#elif HI_OS_TYPE == HI_OS_WIN32
    return _strlwr(szSource);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/*转换nptr为int32 value*/
HI_S32 HI_StrToInt32(const HI_CHAR *nptr, HI_S32 *value)
{
    HI_CHAR *temp;
    *value = strtol(nptr, &temp,  0);

    if(HI_GetLastErr()== ERANGE || nptr == temp)
        {
            HI_SetLastErr(0);
            return HI_FAILURE;
        }
    return (HI_S32)HI_SUCCESS;
}

/*整数转换为字符串,string 空间由外部提供*/
HI_CHAR* HI_IntToStr(HI_S32 value, HI_CHAR* string)
{
    HI_CHAR chTmp[256];
    snprintf(chTmp, 256, "%d", value);
    strncpy(string, chTmp, strlen(chTmp) + 1);
    return string;
}

/*整数转换为字符串,string 空间由外部提供*/
HI_CHAR* HI_Int64ToStr(HI_S64 value, HI_CHAR* string)
{
    HI_CHAR chTmp[256];

#if HI_OS_TYPE == HI_OS_LINUX
    snprintf(chTmp, 256, "%lld", value);
#elif HI_OS_TYPE == HI_OS_WIN32
    snprintf(chTmp, 256, "%I64d", value);
#endif
    strncpy(string, chTmp, strlen(chTmp) + 1);
    return string;
}

/*浮点数转换为字符串*/
HI_CHAR *HI_FloatToStr(HI_FLOAT floatValue, HI_CHAR *strValue)
{
    HI_CHAR chTmp[256];
    snprintf(chTmp, 256, "%f", floatValue);
    strncpy(strValue, chTmp, strlen(chTmp) + 1);
    return strValue;
}

HI_S32 HI_Sprintf(HI_CHAR *str, const HI_CHAR *format, ...)
{
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );
    dwBufLen = vsprintf(str, format, marker);
    va_end(marker);

    return dwBufLen;
}

HI_S32 HI_Snprintf(HI_CHAR *str, HI_SIZE_T size, const HI_CHAR *format, ...)
{
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );

#if HI_OS_TYPE == HI_OS_LINUX
        dwBufLen = vsnprintf(str,size, format, marker);

#elif HI_OS_TYPE == HI_OS_WIN32
        dwBufLen = _vsnprintf(str,size, format, marker);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
    va_end(marker);

    return dwBufLen;
}

HI_S32 HI_VSprintf(HI_CHAR *str, const HI_CHAR *format, HI_VA_LIST argList)
{
    return  vsprintf(str, format, argList);
}

HI_S32 HI_VSnprintf(HI_CHAR *str, HI_SIZE_T size, const HI_CHAR *format, HI_VA_LIST argList)
{
#if HI_OS_TYPE == HI_OS_LINUX
        return vsnprintf(str,size, format, argList);

#elif HI_OS_TYPE == HI_OS_WIN32
        return  _vsnprintf(str,size, format, argList);
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

/* Write formatted output to stdout. */
HI_S32  HI_Printf (const HI_CHAR *format, ...)
{
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );
    dwBufLen = vprintf( format, marker);
    va_end(marker);

    return dwBufLen;
}

/* Read formatted input from stdin.*/
HI_S32 HI_Scanf (const HI_CHAR * format, ...)
{
#if HI_OS_TYPE == HI_OS_LINUX
    HI_S32 dwBufLen;
    va_list marker;

    va_start( marker, format );
    dwBufLen = vscanf( format, marker);
    va_end(marker);
    return dwBufLen;
#elif HI_OS_TYPE == HI_OS_WIN32
    return HI_SUCCESS;
#else
#error YOU MUST DEFINE HI OS TYPE HI_OS_TYPE=HI_OS_WIN32 OR HI_OS_LINUX !
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
