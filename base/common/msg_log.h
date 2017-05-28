/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_log.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/28
  Description   : msg_log.c header file
  History       :
  1.Date        : 2008/01/28
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_LOG_H__
#define __MSG_LOG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <string.h>
#include <errno.h>

#include "scp_common.h"
#include "common/msg_interface.h"

#define DEBUG_POS     SVR_LOG_DEBUG("=======>%s:%d(\"%s\")\n", THIS_FILE, __LINE__, __FUNCTION__)
#define DEBUG_POS_DELAY(x)     \
{   \
    SVR_LOG_DEBUG("=======>%s:%d(\"%s\")\n", THIS_FILE, __LINE__, __FUNCTION__);  \
    sleep(x);  \
}

typedef enum
{
    LOG_PROC,
    LOG_TASK,
    LOG_MODULE,

    LOG_BUTT
}LOG_OWNER;

int MSG_LOG_Init( PFUNC_VTOP_MSG_Log pMsgLogFun );

void MSG_LOG_OutPut(
    LOG_OWNER log_owner,
    int log_value,
    const char *file,          /* 出错的文件名（短文件名、不含路径）   */
    const char *function,      /* 出错的函数                           */
    int line,                /* 出错的位置，即错误处在文件中的行数   */
    int err,                 /* 错误码                               */
    VTOP_LOG_ERRLEVEL_E err_level,  /* 错误的级别                           */
    const char *perr, ...);

/* ========================== 打印 =============================== */
// 调试时所需的输出信息

#ifdef __MSG_DEBUG__
#define MSG_DEBUG(log_owner, log_value, logmsg...)        \
    MSG_LOG_OutPut(log_owner, log_value, THIS_FILE, __FUNCTION__, __LINE__, 0, VTOP_LOG_LEVEL_DEBUG, logmsg)
#else
#define MSG_DEBUG(log_owner, log_value, logmsg...)
#endif

// 程序运行时的状态信息等
#define MSG_INFO(log_owner, log_value, logmsg...)         \
    MSG_LOG_OutPut(log_owner, log_value, THIS_FILE, __FUNCTION__, __LINE__, 0, VTOP_LOG_LEVEL_INFO, logmsg)

// 出错返回时的错误信息
#define MSG_ERROR(log_owner, log_value, err, logmsg...)   \
    MSG_LOG_OutPut(log_owner, log_value, THIS_FILE, __FUNCTION__, __LINE__, err, VTOP_LOG_LEVEL_ERROR, logmsg)

#define MSG_ERROR_RETURN(log_owner, log_value, err, logmsg...) \
    do{ MSG_ERROR(log_owner, log_value, err, logmsg);  return err;  }while(0)

// 出错应用程序退出时的错误信息
#define MSG_FATAL(log_owner, log_value, err, logmsg...)   \
    do {                            \
            MSG_LOG_OutPut(log_owner, log_value, THIS_FILE, __FUNCTION__, __LINE__, err, VTOP_LOG_LEVEL_FATAL, logmsg);   \
            exit(1);           \
    }while(0)

/* ========================== 断言 =============================== */
#define RET_VOID

// 绝对断言: 仅在DEBUG版本起作用，它用于检查"不应该"发生的情况
#define ASSERT(log_owner, log_value, exp)                                         \
    do{                                                     \
        if (!(exp)) {                                       \
            MSG_FATAL(log_owner, log_value, 0, "ASSERT{" #exp "}");       \
        }                                                   \
    }while(0)

/*
** 以下其他的断言，只是用来检查程序运行时是否出错了
** 包含出错时的尽量多的现场信息，便于错误定位
*/
#define ASSERT_SUCCESS(log_owner, log_value, ret)                   \
    do{                                                     \
        if (ret < 0) {                                      \
            MSG_ERROR(log_owner, log_value, ret, "ASSERT{"#ret"=%#x}", ret);     \
            return ret;                                     \
        }                                                   \
    }while(0)

#define ASSERT_SUCCESS_RETURN(log_owner, log_value, ret, err)                   \
    do{                                                     \
        if (ret < 0) {                                      \
            MSG_ERROR(log_owner, log_value, ret, "ASSERT{"#ret"=%#x}", ret);     \
            return err;                                     \
        }                                                   \
    }while(0)

#if 0
#define ASSERT_RETURN(log_owner, log_value, exp, ret)               \
    do{                                                     \
        if (!(exp)) {                                       \
            MSG_ERROR(log_owner, log_value, ret, "\nASSERT{"#exp"}");     \
            return ret;                                     \
        }                                                   \
    }while(0)
#endif

#define ASSERT_GOTO(log_owner, log_value, ret, where)               \
    do{                                                     \
        if (ret < 0) {                                      \
            MSG_ERROR(log_owner, log_value, ret, "ASSERT{"#ret"=%#x}", ret); \
            goto where;                                     \
        }                                                   \
    }while(0)

#define ASSERT_EQUAL(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) != (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOT_EQUAL(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) == (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_LARGE(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) <= (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_SMALL(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) >= (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOTLARGE(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) > (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOTSMALL(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) < (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_EQUAL_NOTRETURN(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) != (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
        }                                                   \
    }while(0)

#define ASSERT_NOT_EQUAL_NOTRETURN(log_owner, log_value, actual, expected, err)   \
    do{                                                     \
        if ((actual) == (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{"#actual"=%#x, "#expected"=%#x}", actual, expected);     \
        }                                                   \
    }while(0)

#define ASSERT_LARGE_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual)<=(expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOTLARGE_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual)>(expected)) {                         \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_SMALL_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual)>=(expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOTSMALL_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual)<(expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_EQUAL_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual) != (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_NOT_EQUAL_PERROR(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual) == (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
            return err;                                     \
        }                                                   \
    }while(0)

#define ASSERT_EQUAL_PERROR_NOTRETURN(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual) != (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
        }                                                   \
    }while(0)

#define ASSERT_NOT_EQUAL_PERROR_NOTRETURN(log_owner, log_value, actual, expected, err, errstr)   \
    do{                                                     \
        if ((actual) == (expected)) {                       \
            MSG_ERROR(log_owner, log_value, err, "ASSERT{%s:%s("#actual"=%#x, "#expected"=%#x)}", errstr, strerror(errno), actual, expected);     \
        }                                                   \
    }while(0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_LOG_H__ */
