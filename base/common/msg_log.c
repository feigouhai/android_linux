/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_log.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/28
  Description   :
  History       :
  1.Date        : 2008/01/28
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include "utils/Logger.h"
#include "scp_common.h"
#include "msg_misc.h"
#include "msg_task.h"
//SVR_MODULE_DECLARE("COMMONMOD")

static const char *s_strErrLevel[VTOP_LOG_LEVEL_BUTT] = {"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "CRIT", "ALERT", "FATAL"};

void VTOP_MSG_Log(
                        const char *file,          /* 出错的文件名（短文件名、不含路径）   */
                        const char *function,      /* 出错的函数                           */
                        int line,                /* 出错的位置，即错误处在文件中的行数   */
                        int err,                /* 错误码*/
                        VTOP_LOG_ERRLEVEL_E err_level,    /* 错误的级别*/
                        const char *perr)           /* 错误信息*/
{
    s_strErrLevel[0] = s_strErrLevel[0];
    HI_LOGD("%s:%d(\"%s\"): %s(0x%x, \"%s\")\n",
        file, line, function, s_strErrLevel[err_level], err, perr);
}

typedef struct hiMSG_LOG_S
{
    PFUNC_VTOP_MSG_Log pMsgLogFun;
} MSG_LOG_S;

MSG_LOG_S s_struMsgLog;

/*****************************************************************************
 Prototype       : MSG_Log
 Description     : ...
 Input           : int err
                const char *pLogInfo
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/28
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int MSG_LOG_Init( PFUNC_VTOP_MSG_Log pMsgLogFun )
{
    memset(&s_struMsgLog, 0, sizeof(s_struMsgLog));
    if ( pMsgLogFun )
    {
        s_struMsgLog.pMsgLogFun = pMsgLogFun;
    }
    return 0;
}

void stub_msg_log_deinit(void)
{
    s_struMsgLog.pMsgLogFun = NULL;
}

char * make_vmessage(const char *fmt, va_list ap)
{
    int n, size = 100;
    char *p, *np;

    if ((p = malloc (size)) == NULL)
        return NULL;

    while (1)
    {
        /* Try to print in the allocated space. */
        n = vsnprintf (p, size, fmt, ap);

        /* If that worked, return the string. */
        if (n > -1 && n < size)
        return p;

        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
        size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
        size *= 2;  /* twice the old size */

        if ((np = realloc (p, size)) == NULL)
        {
        free(p);
        return NULL;
        }
        else
        {
        p = np;
        }
    }
}

/*****************************************************************************
 Prototype       : MSG_LOG_OutPut
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/30
    Author       : qushen
    Modification : Created function

*****************************************************************************/
void MSG_LOG_OutPut(
    LOG_OWNER log_owner,
    int log_value,
    const char *file,          /* 出错的文件名（短文件名、不含路径）   */
    const char *function,      /* 出错的函数                           */
    int line,                /* 出错的位置，即错误处在文件中的行数   */
    int err,                /* 错误码                               */
    VTOP_LOG_ERRLEVEL_E err_level,  /* 错误的级别                           */
    const char *perr, ...)
{
    PFUNC_VTOP_MSG_Log pLogFun = NULL;

    char *p;
    va_list ap;

#ifndef __MSG_DEBUG__
    if((log_owner == LOG_MODULE) && (err_level < VTOP_LOG_LEVEL_ERROR))
    {
        return ;
    }
#endif

    if(log_owner >= LOG_BUTT)
    {
        return ;
    }

    if(log_owner == LOG_PROC)
    {
        if( s_struMsgLog.pMsgLogFun )
        {
        pLogFun = s_struMsgLog.pMsgLogFun;
        }
        else
        {
        pLogFun = VTOP_MSG_Log;
        }
    }
    else if(log_owner == LOG_TASK)
    {
        /* task */
        MSGTASK_DATA * pTaskData;
        VTOP_TASK_ID taskId = (VTOP_TASK_ID)(log_value);
        if(taskId == VTOP_INVALID_TASK_ID)
        {
            pLogFun = VTOP_MSG_Log;
        }
        else
        {
            pTaskData = getTaskData((VTOP_TASK_ID)(log_value));
            if(pTaskData == NULL)
            {
                pLogFun = VTOP_MSG_Log;
            }
            else
            {
                pLogFun= pTaskData->pTaskLog;
            }
        }
    }
    else
    {
        /* modules */
        pLogFun = VTOP_MSG_Log;
    }

    va_start(ap, perr);
    p = make_vmessage(perr, ap);
    va_end(ap);

    if(p != NULL)
    {
        if(pLogFun != NULL)
        {
            pLogFun(file, function, line, err, err_level, p);
        }

        free(p);
    }
    else if(pLogFun != NULL)
    {
        pLogFun(file, function, line, err, err_level, "");
    }
}
