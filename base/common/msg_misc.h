/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_misc.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/31
  Last Modified :
  Description   : msg.c header file
  Function List :
  History       :
  1.Date        : 2008/1/31
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifndef __MSG_MISC_H__
#define __MSG_MISC_H__

#include "common/msg_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifdef __MSG_DEBUG__
#define VTOP_MSG_VERSION "V1.1.0.0(D)"
#else
#define VTOP_MSG_VERSION "V1.1.0.0(R)"
#endif

#define VTOP_MSG_MAX_VERSION_SIZE    32

/* 定义本地SOCKET使用的文件名 */
//#define MSG_SOCKET_DOMAIN "/var/.s3Pmsgdomain"
#define MSG_SOCKET_DOMAIN 46326

typedef enum hiTASK_OWNER_E
{
    TASK_OWNER_TASK  = 0,   /*普通消息任务*/
    TASK_OWNER_BUTT
} TASK_OWNER_E;

#if 0
#define VTOP_MSG_PRJ_ID        1

/* 定义消息服务进程名 */
#define VTOP_MSG_NAME_PROC  ".s3Pmsgproc"

/* 定义每个数据表格的信号量所使用的文件名 */
#define VTOP_MSG_NAME_SEM_TAB_TASK  ".s3Pmsgtask"
#define VTOP_MSG_NAME_SEM_TAB_SUB      ".s3Pmsgsub"
#define VTOP_MSG_NAME_SEM_TAB_GROUP ".s3Pmsggroup"
#else
/* 定义消息服务进程名 */
#define VTOP_MSG_PROC_KEY           0xB2790105

/* 定义每个数据表格的信号量所使用的文件名 */
#define VTOP_MSG_SEM_TASK_KEY       0xF1800813
#define VTOP_MSG_SEM_SUB_KEY        0xAA791019
#endif

/* 定义消息系统默认配置值 */
#define VTOP_MSG_DEFAULT_MAX_TASK        32
#define VTOP_MSG_DEFAULT_MAX_SUB        64

#define VTOP_MSG_DEFAULT_MAX_MEM        (256*1024)

/* 0xffff0001 ~ 0xfffffffe reserved for internal */
#define VTOP_MAX_TASK_ID        0xffff0000

/* msg flag: not for user */
#define VTOP_MSG_FLG_DSC_RCV    0x00001000  /* 接收消息头描述 */
#define VTOP_MSG_FLG_DSC_SND    0x00002000  /* 发送消息头描述 */
#define VTOP_MSG_FLG_DSC_RCV_BYE    0x00008000  /* 接收消息头结束描述 */

/* msg flag: not for user */
#define VTOP_MSG_isRCVMsg(flag)     ((flag) & VTOP_MSG_FLG_DSC_RCV)
#define VTOP_MSG_isSNDMsg(flag)     ((flag) & VTOP_MSG_FLG_DSC_SND)

/*check if a exist then use a, or use value b*/
#define GET_VALUE(a, b) ((a) ? (a) : (b))

typedef struct
{
    unsigned int  maxTaskNum;          /* 消息系统最大消息任务数 */

    unsigned int  maxSubNum;          /* 消息系统最大订阅表数 */
    unsigned int  maxTaskNumInSub;      /* 每订阅表所含最大消息任务数 */

    unsigned int  taskTabPos;        /* 消息任务表的相对位置 */
    unsigned int  subTabPos;         /* 订阅表的相对位置*/

    VTOP_TASK_ID counter_taskId;
    VTOP_MSG_ID  counter_msgId;

    char    version[VTOP_MSG_MAX_VERSION_SIZE];
} MSG_SHM_S;

typedef struct
{
    VTOP_MSG_S_Block msg;   // 用户消息。
    VTOP_MSG_ID resp2id;    // 如果msg为回应消息，则resp2id则为原消息id
    unsigned int magic;
    int privData; //同步应答消息句柄
}VTOP_MSG_S_InnerBlock;

#define MSG_INNER_HEAD_LEN sizeof(VTOP_MSG_S_InnerBlock)

#define MSG_MAX_LEN (MSG_INNER_HEAD_LEN + VTOP_MSG_MAX_MSG_SIZE)

typedef struct hiMSG_SND_BUF_S
{
    size_t sntSize;
    size_t msgSize;
    unsigned char  buffer[MSG_MAX_LEN];
} MSG_SND_BUF_S;

/* 放入私有头文件 */

#define MSG_COPY2BUF(pBuf, pMsg)    \
    do{     \
        memcpy(pBuf, pMsg, MSG_INNER_HEAD_LEN);\
        if((pMsg)->msg.ulMsgLen > 0)\
        {       \
            memcpy(pBuf + MSG_INNER_HEAD_LEN, (pMsg)->msg.pMsgBody, (pMsg)->msg.ulMsgLen);\
            ((VTOP_MSG_S_InnerBlock *)pBuf)->msg.pMsgBody = pBuf + MSG_INNER_HEAD_LEN; \
        }   \
    }while(0)

#define VTOP_MSG_MAGIC 0x7fff

HI_BOOL SHM_Exist(void);
int SHM_Init(VTOP_MSG_S_Config * pMsgConfig);
int SHM_DeInit(void);

extern MSG_SHM_S * SHM_GetAddr(void);

extern int TASK_TAB_GetMaxTaskNum(unsigned int * pMaxTask);
extern int SUB_TAB_GetMaxSubNum(unsigned int * pMaxSub, unsigned int * pMaxTaskInSub);

extern int getMsgID(VTOP_MSG_ID * pMsgID);

extern void VTOP_MSG_ShowTitle(void);
extern void VTOP_MSG_Info(VTOP_MSG_S_InnerBlock * pMsg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*  __MSG_MISC_H__  */
