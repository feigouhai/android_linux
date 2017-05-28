/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_priorq.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/23
  Description   :
  History       :
  1.Date        : 2008/01/23
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "hi_type.h"
#include "msg_misc.h"
#include "msg_priorq.h"
#include "msg_buffer.h"
#include "msg_log.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#if 0

typedef struct hiPRIORQ_S
{
    MSG_QUEUE_S *pMsgQ[VTOP_MSG_PRIO_MAX];
    int totalNum;
} PRIORQ_S;

static PRIORQ_S     s_priorQ;
static MB_HANDLE    s_priorBufHandle = NULL;

static const size_t default_block_size[VTOP_MSG_BL_BUTT] = {256, 512, 1024, 4096};
static const size_t default_block_num[VTOP_MSG_BL_BUTT]  = {256, 128, 64, 16};

int PRIORQ_Init(VTOP_MSG_BLOCKNUM blockNum)
{
    int ret;
    unsigned int  i;
    VTOP_MSG_S_Buffer buffer;

    if ( s_priorBufHandle != NULL )
    {
        return 0;
    }

    memset(&buffer, 0, sizeof(VTOP_MSG_S_Buffer));
    buffer.name = "PRIOR";
    if ( blockNum[VTOP_MSG_BL_0] == 0 && blockNum[VTOP_MSG_BL_1] == 0
      && blockNum[VTOP_MSG_BL_2] == 0 && blockNum[VTOP_MSG_BL_3] == 0 )
    {
        for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
        {
            buffer.blockNum[i] = default_block_num[i];
        }
    }
    else
    {
        for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
        {
            buffer.blockNum[i] = blockNum[i];
        }
    }
    for ( i = 0 ; i < VTOP_MSG_BL_BUTT ; i++ )
    {
        buffer.blockSize[i] = default_block_size[i] + MSG_INNER_HEAD_LEN;
    }
    ret = MB_Init(&s_priorBufHandle, &buffer);
    ASSERT_SUCCESS(LOG_PROC, 0, ret);

    memset(&s_priorQ, 0, sizeof(PRIORQ_S));
    for ( i = 0 ; i < VTOP_MSG_PRIO_MAX ; i++ )
    {
        ret = MQ_Init(&s_priorQ.pMsgQ[i]);
        if ( s_priorQ.pMsgQ[i] == NULL )
        {
            PRIORQ_Deinit();
            MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "no mem!");
        }
    }
    //MSG_DEBUG(LOG_MODULE, 0, "PRIORQ Init ok!");
    return 0;
}

void PRIORQ_Deinit(void)
{
    unsigned int i;
    if ( s_priorBufHandle == NULL )
    {
        return;
    }

    for ( i = 0 ; i < VTOP_MSG_PRIO_MAX ; i++ )
    {
        if ( s_priorQ.pMsgQ[i] != NULL )
        {
            MQ_Deinit(s_priorQ.pMsgQ[i]);
        }
    }
    memset(&s_priorQ, 0, sizeof(PRIORQ_S));
    MB_Deinit(s_priorBufHandle);
    s_priorBufHandle = NULL;
}

//#define MAX_SENDBUF_NUM 0x1fffffff

/*仅接受的输入是MB_Alloc申请的消息内存*/
int PRIORQ_Put(const VTOP_MSG_S_InnerBlock *pMsg)
{
    MSG_QUEUE_S *pMsgQ;
    QNODE_S * pNode;
    unsigned char *pBuf = NULL;

    /*1. param check*/
    ASSERT(LOG_MODULE, 0, pMsg != NULL);

    /*2. write special msg queue*/
    if ( pMsg->msg.ulFlag & VTOP_MSG_FLG_SYNC )
    {
        ASSERT(LOG_MODULE, 0, pMsg->msg.ulPrio == VTOP_MSG_PRIO_SYN);
    }
    else if ( pMsg->msg.ulFlag & VTOP_MSG_FLG_RESP )
    {
        ASSERT(LOG_MODULE, 0, pMsg->msg.ulPrio == VTOP_MSG_PRIO_RSP);
    }
    else
    {
        ASSERT(LOG_MODULE, 0, pMsg->msg.ulPrio <= VTOP_MSG_PRIO_HIGH);
    }

    /*3. Check msg len*/
    if ( pMsg->msg.ulMsgLen > VTOP_MSG_MAX_MSG_SIZE )
    {
        MSG_ERROR_RETURN(LOG_PROC, 0, VTOP_MSG_ERR_MSGSIZE, "priorq invalid msg body len: %d", pMsg->msg.ulMsgLen);
    }

    /*2st, 消息加入优先级队列*/
    pBuf = (unsigned char*)MB_Alloc(s_priorBufHandle, pMsg->msg.ulMsgLen + MSG_INNER_HEAD_LEN);
    if ( NULL == pBuf )
    {
        MSG_ERROR_RETURN(LOG_PROC, 0, VTOP_MSG_ERR_NOMEM, "MSG Buf is full: %u!", MB_GetTotalNum(s_priorBufHandle));
    }

    MSG_COPY2BUF(pBuf, pMsg);

    /*3. get prior Q related to prior*/
    pMsgQ = s_priorQ.pMsgQ[pMsg->msg.ulPrio];

    pNode = MB_GetQNode(pBuf);
    pNode->priv_data = pBuf;
    MQ_Put(pMsgQ, pNode);
    s_priorQ.totalNum++;

    return 0;
}

int PRIORQ_Get(VTOP_MSG_S_InnerBlock **ppMsg)
{
    int i;
    MSG_QUEUE_S *pMsgQ;
    QNODE_S * pNode;

    for ( i = VTOP_MSG_PRIO_RSP ; i >=0 ; i-- )
    {
        pMsgQ = s_priorQ.pMsgQ[i];
        if ( !MQ_IsEmpty(pMsgQ))
        {
            MQ_Get(pMsgQ, &pNode);
            *ppMsg = (VTOP_MSG_S_InnerBlock*)(pNode->priv_data);
            s_priorQ.totalNum--;
            return 0;
        }
    }
    //MSG_DEBUG(LOG_MODULE, 0, "Q is empty!");
    return -1;
}

void PRIORQ_Free(VTOP_MSG_S_InnerBlock *pMsg)
{
    MB_Free(s_priorBufHandle, pMsg);
}

size_t PRIORQ_GetMbNum(void)
{
    return MB_GetTotalNum(s_priorBufHandle);
}

#ifdef __MSG_DEBUG__
int PRIORQ_GetNum(void)
{
    return s_priorQ.totalNum;
}

static const char *s_strPrior[VTOP_MSG_PRIO_MAX] = {"LOW ", "MID ", "HIGH", "SYNC", "RSP "};

void PRIORQ_ShowState(void)
{
    unsigned int i;

    printf("\n*************   PRIORQ State   *************************\n");
    printf("  index         Qtype         num         IsEmpty\n");
    for ( i = 0 ; i < VTOP_MSG_PRIO_MAX ; i++ )
    {
        printf("    %2u          %s        %4u        %4u\n",
            i, s_strPrior[i], MQ_GetNum(s_priorQ.pMsgQ[i]), MQ_IsEmpty(s_priorQ.pMsgQ[i]));
    }
    printf("********************************************************\n");
    fflush(stdout);

    MB_ShowState(s_priorBufHandle);
}
#else
void PRIORQ_ShowState(void)
{
    return;
}

#endif

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
