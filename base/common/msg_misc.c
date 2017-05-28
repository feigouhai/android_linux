/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_misc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/30
  Last Modified :
  Description   : 消息内部模块其他公共函数实现
  Function List :
  History       :
  1.Date        : 2008/1/30
    Author      : z42136
    Modification: Created file

******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <stdio.h>

#include "hi_type.h"

#include "utils/Logger.h"
#include "common/msg_interface.h"
#include "msg_misc.h"
#include "msg_log.h"

#include "msg_taskmng.h"
#include "msg_submng.h"

//SVR_MODULE_DECLARE("COMMONMOD")

static MSG_SHM_S * g_MsgShmAddr = NULL;

HI_BOOL SHM_Exist(void)
{
    return (g_MsgShmAddr != NULL)? HI_TRUE:HI_FALSE;
}

int SHM_Init(VTOP_MSG_S_Config * pMsgConfig)
{
    unsigned int  shm_len;
    MSG_SHM_S shm;
    unsigned int tt_size, st_size;
    MSG_SHM_S  *pShmHead;

    ASSERT(LOG_MODULE, 0, pMsgConfig!=NULL);

    /*填充msgconfig数据结构*/
    shm.maxTaskNum  = GET_VALUE(pMsgConfig->ulMaxTask, VTOP_MSG_DEFAULT_MAX_TASK);
    shm.maxSubNum   = GET_VALUE(pMsgConfig->ulMaxSub, VTOP_MSG_DEFAULT_MAX_SUB);
    /*注意: maxTaskNumInSub, maxTaskNumInGroup, maxFilterNumInTrc大小默认不应超过
    maxTaskNum, 否则在dispatch申请的同一块内存可能产生越界*/
    shm.maxTaskNumInSub = shm.maxTaskNum;
    // TODO: trace表格是否个数太大，有待优化

    /*计算分配共享内存的大小以及任务表、订阅表和组播表等位置*/
    tt_size = TASK_TAB_CalcSize(shm.maxTaskNum);
    st_size = SUB_TAB_CalcSize(shm.maxSubNum, shm.maxTaskNumInSub);
    shm_len = sizeof(MSG_SHM_S) + tt_size + st_size;

    shm.taskTabPos = sizeof(MSG_SHM_S);
    shm.subTabPos = shm.taskTabPos + tt_size;

    shm.counter_taskId = 0;
    shm.counter_msgId  = 0;

    strncpy(shm.version, VTOP_MSG_VERSION, VTOP_MSG_MAX_VERSION_SIZE);
    shm.version[VTOP_MSG_MAX_VERSION_SIZE-1]='\0';
#if 0
    shmid = shmget(VTOP_MSG_PROC_KEY, shm_len, IPC_CREAT | IPC_EXCL);
    ASSERT_NOTSMALL_PERROR(LOG_MODULE, 0, shmid, 0, VTOP_MSG_ERR_SHMATTCH, "shmget ");

    pShmHead = (MSG_SHM_S*)shmat(shmid, NULL, 0);
    ASSERT_NOT_EQUAL_PERROR(LOG_MODULE, 0, pShmHead, (MSG_SHM_S  *)(-1), VTOP_MSG_ERR_SHMATTCH, "shmat ");
#else
    pShmHead = (MSG_SHM_S*)malloc(shm_len);
    ASSERT_NOT_EQUAL_PERROR(LOG_MODULE, 0, pShmHead, (MSG_SHM_S  *)(-1), VTOP_MSG_ERR_SHMATTCH, "shmat ");
#endif
    // 清空共享内存区
    if(NULL == pShmHead)
    {
        HI_LOGD("malloc failure! pShmHead==NULL");
        return 0;
    }
    memset(pShmHead, 0, shm_len);
    memcpy(pShmHead, &shm, sizeof(shm));

    //shmdt(pShmHead);
    g_MsgShmAddr = pShmHead;

    return 0;
}

int SHM_DeInit(void)
{

#if 0
    shmid=shmget(VTOP_MSG_PROC_KEY, 0, 0);
    ASSERT_NOTSMALL_PERROR(LOG_MODULE, 0, shmid, 0, VTOP_MSG_ERR_SHMATTCH, "shmget ");
#endif
    if(g_MsgShmAddr != NULL)
    {
        //ret = shmdt((void *)g_MsgShmAddr);
        free(g_MsgShmAddr);
        g_MsgShmAddr = NULL;
    }
#if 0
    ret = shmctl(shmid, IPC_RMID, 0);
    ASSERT_EQUAL_PERROR(LOG_MODULE, 0, ret, 0, VTOP_MSG_ERR_SHMATTCH, "shmctl(IPC_RMID) ");
#endif
    return 0;
}

MSG_SHM_S * SHM_GetAddr(void)
{
    return g_MsgShmAddr;
}

int TASK_TAB_GetMaxTaskNum(unsigned int * pMaxTask)
{
    MSG_SHM_S * pMsgShm = NULL;

    pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsgShm, NULL, VTOP_MSG_ERR_SHMATTCH);

    *pMaxTask = pMsgShm->maxTaskNum;
    return HI_SUCCESS;
}

int SUB_TAB_GetMaxSubNum(unsigned int * pMaxSub, unsigned int * pMaxTaskInSub)
{
    MSG_SHM_S * pMsgShm = NULL;

    pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsgShm, NULL, VTOP_MSG_ERR_SHMATTCH);

    *pMaxSub = pMsgShm->maxSubNum;
    *pMaxTaskInSub = pMsgShm->maxTaskNumInSub;

    return HI_SUCCESS;
}

void VTOP_MSG_Show(VTOP_MSG_S_Block * pMsg, const char * file, const char * func, int line)
{
    HI_LOGD("======%s:%d(\"%s\"): msg info======\n", file, line, func);
    VTOP_MSG_ShowTitle();
    VTOP_MSG_Info((VTOP_MSG_S_InnerBlock*)pMsg);
}

void VTOP_MSG_ShowTitle(void)
{
    HI_LOGD("sndTask     rcvTask     msgType     msgID       MsgLen      Prio        Flag\n");
}

void VTOP_MSG_Info(VTOP_MSG_S_InnerBlock * pMsg)
{
    HI_LOGD("%08x    %08x    %08x    %08x    %08x    %08x    %08x\n",
        pMsg->msg.sndTaskId, pMsg->msg.rcvTaskId, pMsg->msg.msgType, pMsg->msg.msgID,
        pMsg->msg.ulMsgLen,  pMsg->msg.ulPrio, pMsg->msg.ulFlag );
}

int getMsgID(VTOP_MSG_ID * pMsgID)
{
    MSG_SHM_S * pMsgShm = NULL;

    pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsgShm, NULL, VTOP_MSG_ERR_SHMATTCH);

    //这样会不会有风险，如果两个任务同时发消息，msgid可能一样
    *pMsgID = pMsgShm->counter_msgId++;

    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
