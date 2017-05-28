/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_syncq.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/04/02
  Description   :
  History       :
  1.Date        : 2008/04/02
    Author      : qushen
    Modification: Created file

******************************************************************************/

#include "scp_common.h"
#include "msg_buffer.h"
#include "msg_connq.h"
#include "msg_com.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#define __TEST_CONNQ_Sync__

#if 0
typedef struct hiCONNQ_SYNC_BLOCK_S
{
    VTOP_MSG_ID     msgId;
    int   fd;
} CONNQ_SYNC_BLOCK_S;

#define CONNQ_SYNC_BLKSIZE  sizeof(CONNQ_SYNC_BLOCK_S)

static MSG_QUEUE_S *s_pConnQSync = NULL;
static MB_HANDLE    s_connSyncBufHandle = NULL;

int CONNQ_Sync_Init(size_t conn_num)
{
    int ret = -1;
    VTOP_MSG_S_Buffer buffer;

    if ( s_pConnQSync != NULL )
    {
        return 0;
    }

    memset(&buffer, 0, sizeof(VTOP_MSG_S_Buffer));
    buffer.name = "CONNQ-Sync";
    buffer.blockSize[0] = CONNQ_SYNC_BLKSIZE;
    buffer.blockNum[0]  = conn_num;
    ret = MB_Init(&s_connSyncBufHandle, &buffer);
    ASSERT_SUCCESS(LOG_PROC, 0, ret);

    ret = MQ_Init(&s_pConnQSync);
    if ( ret != 0 )
    {
        MSG_ERROR(LOG_PROC, 0, ret, "init MQ err!");
        MB_Deinit(s_connSyncBufHandle);
        s_connSyncBufHandle = NULL;
        return ret;
    }
    return 0;
}

void CONNQ_Sync_Deinit(void)
{
    if ( s_pConnQSync == NULL )
    {
        return;
    }
    MQ_Deinit(s_pConnQSync);
    s_pConnQSync = NULL;
    MB_Deinit(s_connSyncBufHandle);
    s_connSyncBufHandle = NULL;
}

/*pConn仅接受的输入是MB_Alloc申请的消息内存*/
int CONNQ_Sync_Add(int fd, VTOP_MSG_ID msgId)
{
    QNODE_S *pNode;
    CONNQ_SYNC_BLOCK_S *pBlk = NULL;

    ASSERT(LOG_MODULE, 0, fd > 0);

    pBlk = (CONNQ_SYNC_BLOCK_S *)MB_Alloc(s_connSyncBufHandle, CONNQ_SYNC_BLKSIZE);
    ASSERT_NOT_EQUAL(LOG_PROC, 0, pBlk, NULL, VTOP_MSG_ERR_NOMEM);

    pNode = MB_GetQNode((unsigned char *)pBlk);
    pNode->priv_data = (void*)pBlk;
    pBlk->fd = fd;
    pBlk->msgId = msgId;

    MQ_Put(s_pConnQSync, pNode);
    return 0;
}

int CONNQ_Sync_Del(int fd)
{
    struct list_head *pos;
    QNODE_S * pNode;
    CONNQ_SYNC_BLOCK_S *pBlk = NULL;

    ASSERT_LARGE(LOG_MODULE, 0, fd, 0, VTOP_MSG_ERR_INVALIDPARA);

    MQ_LOCK(s_pConnQSync);

    list_for_each(pos, &s_pConnQSync->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        pBlk = pNode->priv_data;
        if ( pBlk->fd == fd )
        {
#ifdef __TEST_CONNQ_Sync__
            MSG_DEBUG(LOG_MODULE, 0, "prepare for del sync fd: %d", fd);
#endif
            list_del(pos);
            MQ_UNLOCK(s_pConnQSync);
            MB_Free(s_pConnQSync, pBlk);
            return 0;
        }
    }

    MQ_UNLOCK(s_pConnQSync);
    return -1;
}

int CONNQ_Sync_MsgId2Fd(VTOP_MSG_ID msgId)
{
    struct list_head *pos;
    QNODE_S * pNode;
    CONNQ_SYNC_BLOCK_S *pBlk = NULL;
    int fd;

    MQ_LOCK(s_pConnQSync);

    list_for_each(pos, &s_pConnQSync->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        pBlk = pNode->priv_data;
        if ( pBlk->msgId == msgId )
        {
#ifdef __TEST_CONNQ_Sync__
            MSG_DEBUG(LOG_MODULE, 0, "find msg id: %u", msgId);
#endif
            fd = pBlk->fd;
            MQ_UNLOCK(s_pConnQSync);
            return fd;
        }
    }

    MQ_UNLOCK(s_pConnQSync);
    return -1;
}

#if 0
void CONNQ_Sync_DelAll(CONNQ_Sync_S *pConnQ)
{
    struct list_head *pos;
    QNODE_S * pNode;
    VTOP_MSG_S_Conn *pConn;

    list_for_each(pos, &s_pConnQSync->list)
    {
        pNode = list_entry(pos, QNODE_S, list);
        if ( pNode == NULL )
        {
            continue;
        }
        list_del(pos);
        free(pNode->priv_data);
        free(pNode);
        //printf("<%d> ", pConn->connFd);
    }
}
#else
void CONNQ_Sync_DelAll(void)
{
    QNODE_S * pNode;
    QNODE_S * n;

    MQ_LOCK(s_pConnQSync);

    list_for_each_entry_safe(pNode, n, &s_pConnQSync->list, list)
    {
        list_del(&pNode->list);
        MB_Free(s_connSyncBufHandle, pNode->priv_data);
    }
    MQ_UNLOCK(s_pConnQSync);
}
#endif

void CONNQ_Sync_ShowState(void)
{
    MB_ShowState(s_connSyncBufHandle);
}

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
