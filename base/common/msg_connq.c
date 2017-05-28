/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_connq.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/24
  Description   :
  History       :
  1.Date        : 2008/01/24
    Author      : qushen
    Modification: Created file

******************************************************************************/

#include "scp_common.h"
#include "msg_buffer.h"
#include "msg_connq.h"
#include "msg_com.h"
#include "msg_taskmng.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#define __TEST_CONNQ__

#if 0

typedef struct hiCONNQ_TASK_BLOCK_S
{
    VTOP_TASK_ID    taskId;
    int   fd;
    MSG_SND_BUF_S   sndBuf;
} CONNQ_TASK_BLOCK_S;

#define CONNQ_TASK_BLKSIZE  sizeof(CONNQ_TASK_BLOCK_S)

static MSG_QUEUE_S *s_pConnQTask = NULL;
static MB_HANDLE    s_connTaskBufHandle = NULL;

int CONNQ_Task_Init(size_t conn_num)
{
    int ret = -1;
    VTOP_MSG_S_Buffer buffer;

    if ( s_pConnQTask != NULL )
    {
        return 0;
    }

    memset(&buffer, 0, sizeof(VTOP_MSG_S_Buffer));
    buffer.name = "CONNQ-Task";
    buffer.blockSize[0] = CONNQ_TASK_BLKSIZE;
    buffer.blockNum[0]  = conn_num;
    ret = MB_Init(&s_connTaskBufHandle, &buffer);
    ASSERT_SUCCESS(LOG_PROC, 0, ret);

    ret = MQ_Init(&s_pConnQTask);
    if ( ret != 0 )
    {
        MSG_ERROR(LOG_PROC, 0, ret, "init MQ err!");
        MB_Deinit(s_connTaskBufHandle);
        s_connTaskBufHandle = NULL;
        return ret;
    }
    return 0;
}

void CONNQ_Task_Deinit(void)
{
    if ( s_pConnQTask == NULL )
    {
        return;
    }
    MQ_Deinit(s_pConnQTask);
    s_pConnQTask = NULL;
    MB_Deinit(s_connTaskBufHandle);
    s_connTaskBufHandle = NULL;
}

/*pConn仅接受的输入是MB_Alloc申请的消息内存*/
int CONNQ_Task_Add(int fd, VTOP_TASK_ID taskId)
{
    QNODE_S *pNode;
    CONNQ_TASK_BLOCK_S *pBlk = NULL;

    ASSERT(LOG_MODULE, 0, fd > 0);
    ASSERT(LOG_MODULE, 0, taskId != VTOP_INVALID_TASK_ID);

    pBlk = (CONNQ_TASK_BLOCK_S *)MB_Alloc(s_connTaskBufHandle, CONNQ_TASK_BLKSIZE);
    ASSERT_NOT_EQUAL(LOG_PROC, 0, pBlk, NULL, VTOP_MSG_ERR_NOMEM);

    pNode = MB_GetQNode((unsigned char *)pBlk);
    pNode->priv_data = (void*)pBlk;
    memset(pBlk, 0, CONNQ_TASK_BLKSIZE);
    pBlk->fd = fd;
    pBlk->taskId = taskId;

    MQ_Put(s_pConnQTask, pNode);

    return 0;
}

int CONNQ_Task_Del(int fd)
{
    struct list_head *pos;
    QNODE_S * pNode;
    CONNQ_TASK_BLOCK_S *pBlk = NULL;

    ASSERT_LARGE(LOG_MODULE, 0, fd, 0, VTOP_MSG_ERR_INVALIDPARA);

    MQ_LOCK(s_pConnQTask);

    list_for_each(pos, &s_pConnQTask->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        pBlk = pNode->priv_data;
        if ( pBlk->fd == fd )
        {
#ifdef __TEST_CONNQ_Task__
            MSG_DEBUG(LOG_MODULE, 0, "prepare for del async fd: %d", fd);
#endif
            list_del(pos);
            MQ_UNLOCK(s_pConnQTask);
            MB_Free(s_pConnQTask, pBlk);
            return 0;
        }
    }

    MQ_UNLOCK(s_pConnQTask);
    return -1;
}

int CONNQ_Task_Send(const VTOP_MSG_S_InnerBlock *pMsg)
{
    int ret;
    struct list_head *pos;
    QNODE_S * pNode;
    CONNQ_TASK_BLOCK_S *pBlk = NULL;
    //static unsigned char buf[MSG_MAX_LEN];

    MQ_LOCK(s_pConnQTask);
    list_for_each(pos, &s_pConnQTask->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        pBlk = pNode->priv_data;
        if ( pBlk->taskId == pMsg->msg.rcvTaskId )
        {
            ret = MSG_Send2TaskRcv(pBlk->fd, &pBlk->sndBuf, pMsg);
            MQ_UNLOCK(s_pConnQTask);
            return ret;
        }
    }
    MQ_UNLOCK(s_pConnQTask);
    return VTOP_MSG_ERRID_TASKNOTEXIST;
}

void CONNQ_Task_DelAll(void)
{
    QNODE_S * pNode;
    QNODE_S * n;

    MQ_LOCK(s_pConnQTask);

    list_for_each_entry_safe(pNode, n, &s_pConnQTask->list, list)
    {
        list_del(&pNode->list);
        MB_Free(s_connTaskBufHandle, pNode->priv_data);
    }
    MQ_UNLOCK(s_pConnQTask);
}

void CONNQ_Task_ShowState(void)
{
    MB_ShowState(s_connTaskBufHandle);
}

#if 0
void CONNQ_Task_List(void)
{
    VTOP_MSG_S_Conn *pConn;
    QNODE_S * pos;
    QNODE_S * n;

    printf("\n$$$$$$$$$$$$ List ConnQ:\n");
    list_for_each_entry_safe(pos, n, &s_pConnQTask->list, list)
    {
        pConn = pos->priv_data;

        printf("fd=%#x, type=%#x", pConn->connFd, pConn->connType);
        if(pConn->connType == VTOP_MSG_FLG_DSC_RCV)
        {
            printf(", taskId=%#x\n", pConn->connInfo.taskId);
        }
        else if(pConn->connType == VTOP_MSG_FLG_DSC_SND)
        {
            printf(", msgID=%#x\n", pConn->connInfo.msgId);
        }
        else if(pConn->connType == VTOP_MSG_FLG_DSC_TRC)
        {
            printf(", traceId=%#x\n", pConn->connInfo.traceId);
        }
    }
    printf("$$$$$$$$$$$$ ConnQ over\n\n");
}
#endif

void CONNQ_Task_SendAll(const VTOP_MSG_S_InnerBlock *pMsg)
{
    int ret;
    struct list_head *pos;
    QNODE_S * pNode;
    CONNQ_TASK_BLOCK_S *pBlk = NULL;
    static VTOP_MSG_TASKNAME taskName;

    MQ_LOCK(s_pConnQTask);
    list_for_each(pos, &s_pConnQTask->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        pBlk = pNode->priv_data;
        if ( pBlk->taskId != VTOP_INVALID_TASK_ID )
        {
            ret = MSG_Send2TaskRcv(pBlk->fd, &pBlk->sndBuf, pMsg);
            if ( ret != 0 )
            {
                TASK_TAB_Id2Name(pBlk->taskId, taskName);
                MSG_ERROR(LOG_MODULE, 0, ret, "Broadcast to Task \"%s\" err", taskName);
            }
            MQ_UNLOCK(s_pConnQTask);
            return;
        }
    }
    MQ_UNLOCK(s_pConnQTask);
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
