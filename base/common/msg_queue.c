/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_queue.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/03/15
  Description   :
  History       :
  1.Date        : 2009/03/15
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include "hi_type.h"
#include "msg_misc.h"
#include "msg_buffer.h"
#include "msg_queue.h"
#include "msg_log.h"

typedef struct hiMSG_QUEUE_S
{
    HI_S32 num;    /* number of item in msg queue*/
    struct list_head list;
    HI_ThreadMutex_S lock;
    MB_HANDLE bufHandle;
} MSG_QUEUE_S;

#define MSG_QUEUE_LOCK()   HI_MutexLock(&pFltrQueue->lock);
#define MSG_QUEUE_UNLOCK() HI_MutexUnLock(&pFltrQueue->lock);

#if 0
#define MSG_MAX_MSG_SIZE    512
#define MSG_MAX_MSG_NUM     64
#endif

/*****************************************************************************
 Prototype       : MSG_QueueInit
 Description     : ...
 Input           : HI_S32 bufsize
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_QueueInit( HI_CHAR *name, HI_U32 bufsize, MSG_QUEUE_HANDLE *handle )
{
    HI_S32 ret = -1;
    VTOP_MSG_S_Buffer buffer;
    //MB_HANDLE    s_connTaskBufHandle = NULL;
    MSG_QUEUE_S *pFltrQueue = NULL;

    if ( bufsize < 1024 )
    {
        return HI_FAILURE;
    }

    pFltrQueue = (MSG_QUEUE_S*)malloc(sizeof(MSG_QUEUE_S));
    if ( pFltrQueue == NULL )
    {
        return -1;
    }

    memset(&buffer, 0, sizeof(VTOP_MSG_S_Buffer));
    buffer.name = name;
    buffer.blockSize[0] = 256;
    buffer.blockNum[0]  = 128;

    buffer.blockSize[1] = 512;
    buffer.blockNum[1]  = 8;

    ret = MB_Init(&pFltrQueue->bufHandle, &buffer);
    if(ret != 0)
    {
        free(pFltrQueue); //Add by z00225813
        return -1;
    }

    INIT_LIST_HEAD(&pFltrQueue->list);
    HI_MutexInit(&pFltrQueue->lock, NULL);
    pFltrQueue->num = 0;

    *handle = pFltrQueue;
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_QueueDeinit
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_VOID
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_VOID MSG_QueueDeinit( MSG_QUEUE_HANDLE handle )
{
    MSG_QUEUE_S *pFltrQueue = handle;
    if ( pFltrQueue == NULL )
    {
        return;
    }

    MSG_QueueRemove(handle);

    MB_Deinit(pFltrQueue->bufHandle);
    HI_MutexDestroy(&pFltrQueue->lock);
    free(pFltrQueue);
}

/*****************************************************************************
 Prototype       : MSG_QueuePut
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_QueuePut( MSG_QUEUE_HANDLE handle, const VTOP_MSG_S_InnerBlock *pMsg )
{
    MSG_QUEUE_S *pFltrQueue = handle;
    HI_U8 *pBuf = NULL;
    QNODE_S * pNode;

    QNODE_S * pNode2;
    QNODE_S * n;
    VTOP_MSG_S_InnerBlock *pMsg2;

    if ( pFltrQueue == NULL || pMsg == NULL )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "invalid param");
    }

    /*3. Check msg len*/
    if ( pMsg->msg.ulMsgLen > VTOP_MSG_MAX_MSG_SIZE )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_MSGSIZE, "priorq invalid msg body len: %d", pMsg->msg.ulMsgLen);
    }
    if ( (pMsg->msg.pMsgBody == NULL && pMsg->msg.ulMsgLen != 0)
      || (pMsg->msg.pMsgBody != NULL && pMsg->msg.ulMsgLen == 0) )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_INVALIDPARA, "priorq invalid msg body len(%d) ptr(%d)",
            pMsg->msg.ulMsgLen, pMsg->msg.pMsgBody);
    }

    /*4. Alloc msg buffer*/
    MSG_QUEUE_LOCK();

    pBuf = (HI_U8*)MB_Alloc(pFltrQueue->bufHandle, pMsg->msg.ulMsgLen + MSG_INNER_HEAD_LEN);
    if ( NULL == pBuf )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_NOMEM, "MSG Buf is full: %u!", MB_GetTotalNum(pFltrQueue->bufHandle));
        VTOP_MSG_ShowTitle();
        VTOP_MSG_Info((VTOP_MSG_S_InnerBlock*)pMsg);

        #if 1
        VTOP_MSG_ShowTitle();
        list_for_each_entry_safe(pNode2, n, &pFltrQueue->list, list)
        {
            pMsg2 = (VTOP_MSG_S_InnerBlock*)(pNode2->priv_data);
            VTOP_MSG_Info(pMsg2);
        }
        #endif

        MSG_QUEUE_UNLOCK();
        return -1;
    }

    /*将长度和数据拷贝到buffer快中 len(4bytes)+data(len bytes)*/
    MSG_COPY2BUF(pBuf, pMsg);

    pNode = MB_GetQNode(pBuf);
    pNode->priv_data = pBuf;
    list_add_tail(&pNode->list, &pFltrQueue->list);
    pFltrQueue->num++;

    MSG_QUEUE_UNLOCK();

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_QueueGet
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_QueueGet( MSG_QUEUE_HANDLE handle, VTOP_MSG_S_InnerBlock **ppMsg )
{
    MSG_QUEUE_S *pFltrQueue = handle;
    QNODE_S * pNode;

    if ( pFltrQueue == NULL || ppMsg == NULL )
    {
        MSG_ERROR_RETURN(LOG_PROC, 0, VTOP_MSG_ERR_INVALIDPARA, "invalid param");
    }

    MSG_QUEUE_LOCK();

    if ( !list_empty(&pFltrQueue->list) )
    {
        pNode = list_entry(pFltrQueue->list.next, QNODE_S, list);
        list_del(&pNode->list);
        pFltrQueue->num--;
        *ppMsg = (VTOP_MSG_S_InnerBlock*)(pNode->priv_data);
        MSG_QUEUE_UNLOCK();
        return HI_SUCCESS;
    }

    MSG_QUEUE_UNLOCK();
    return HI_FAILURE;
}

/*****************************************************************************
 Prototype       : MSG_QueueGet
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_QueueGetMerge( MSG_QUEUE_HANDLE handle, VTOP_MSG_TYPE msgtype, VTOP_MSG_S_InnerBlock **ppMsg )
{
    MSG_QUEUE_S *pFltrQueue = handle;
    QNODE_S * pNode;
    QNODE_S * n;
    VTOP_MSG_S_InnerBlock *pInnerMsg, *pLastInnerMsg;

    if ( pFltrQueue == NULL )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_INVALIDPARA, "invalid param");
        return HI_FAILURE;
    }

    MSG_QUEUE_LOCK();

    if ( list_empty(&pFltrQueue->list) )
    {
        MSG_QUEUE_UNLOCK();
        return HI_FAILURE;
    }

    /*step1: 先收取第一个消息*/
    pNode = list_entry(pFltrQueue->list.next, QNODE_S, list);
    list_del(&pNode->list);
    pFltrQueue->num--;
    *ppMsg = (VTOP_MSG_S_InnerBlock*)(pNode->priv_data);

    pLastInnerMsg = *ppMsg;
    /*step2: 如果消息队列中存在该类型的消息，从消息队列中直接过滤掉*/
    list_for_each_entry_safe(pNode, n, &pFltrQueue->list, list)
    {
        pInnerMsg = (VTOP_MSG_S_InnerBlock*)(pNode->priv_data);
        if ( msgtype == pInnerMsg->msg.msgType )
        {
            //printf("Merge msgtype = 0x%x, ID = 0x%x, leave num: %d\n", pInnerMsg->msg.msgType, pInnerMsg->msg.msgID, pFltrQueue->num);
            list_del(&pNode->list);
            pFltrQueue->num--;
            /*释放上一次的buffer*/
            MB_Free(pFltrQueue->bufHandle, pLastInnerMsg);
            pLastInnerMsg = pInnerMsg;
        }
    }
    *ppMsg = pLastInnerMsg;

    MSG_QUEUE_UNLOCK();
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : MSG_QueueFree
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_VOID
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_VOID MSG_QueueFree( MSG_QUEUE_HANDLE handle, const VTOP_MSG_S_InnerBlock *pMsg )
{
    MSG_QUEUE_S *pFltrQueue = handle;

    if ( pFltrQueue == NULL || pMsg == NULL )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_INVALIDPARA, "invalid param");
        return;
    }
    // TODO: 这里加锁后导致收发速率不匹配
    MSG_QUEUE_LOCK();

    MB_Free(pFltrQueue->bufHandle, (HI_VOID*)pMsg);

    MSG_QUEUE_UNLOCK();
    return ;
}
/*****************************************************************************
 Prototype       : MSG_QueueRemove
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/2
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_VOID MSG_QueueRemove( MSG_QUEUE_HANDLE handle )
{
    MSG_QUEUE_S *pFltrQueue = handle;
    QNODE_S * pNode;
    QNODE_S * n;

    if ( pFltrQueue == NULL )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_INVALIDPARA, "invalid param");
        return;
    }

    MSG_QUEUE_LOCK();

    list_for_each_entry_safe(pNode, n, &pFltrQueue->list, list)
    {
        list_del(&pNode->list);
        MB_Free(pFltrQueue->bufHandle, pNode->priv_data);
    }

    MSG_QUEUE_UNLOCK();
}
