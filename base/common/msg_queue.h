/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_queue.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/26
  Description   : msg_queue.c header file
  History       :
  1.Date        : 2008/01/26
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __MSG_QUEUE_H__
#define __MSG_QUEUE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include "linux_list.h"
#include "common/hi_adp_mutex.h"
#include "msg_misc.h"

typedef HI_VOID* MSG_QUEUE_HANDLE;

#define MSG_QUEUE_INVALID_HANDLE ((MSG_QUEUE_HANDLE)(NULL))

#if 0
typedef struct hiQNODE_S
{
    void *priv_data;
    struct list_head list;
} QNODE_S;

typedef struct hiMSG_QUEUE_S
{
    int num;    /* number of item in msg queue*/
    //int key;
    //char name[32];
    struct list_head list;
    HI_ThreadMutex_S lock;
    //void (*freePrivData)(void *p);
} MSG_QUEUE_S, *MSG_QUEUE_S_PTR;
#endif

#define MQ_LOCK(pMQ)    HI_MutexLock(&pMQ->lock)
#define MQ_UNLOCK(pMQ)  HI_MutexUnLock(&pMQ->lock)

#if 0
void  MQ_Deinit(MSG_QUEUE_S *pMQ);
void  MQ_Get( MSG_QUEUE_S *pMQ, QNODE_S **ppNode );
int MQ_Init(MSG_QUEUE_S **ppMQ);
int MQ_IsEmpty( MSG_QUEUE_S *pMQ );
void  MQ_Put( MSG_QUEUE_S *pMQ, QNODE_S *pNode );
int MQ_GetNum( MSG_QUEUE_S *pMQ );
#endif

HI_VOID MSG_QueueDeinit( MSG_QUEUE_HANDLE handle );
HI_VOID MSG_QueueFree( MSG_QUEUE_HANDLE handle, const VTOP_MSG_S_InnerBlock *pMsg );
HI_S32 MSG_QueueGet( MSG_QUEUE_HANDLE handle, VTOP_MSG_S_InnerBlock **ppMsg );
HI_S32 MSG_QueueInit( HI_CHAR *name, HI_U32 bufsize, MSG_QUEUE_HANDLE *handle );
HI_S32 MSG_QueuePut( MSG_QUEUE_HANDLE handle, const VTOP_MSG_S_InnerBlock *pMsg );
HI_VOID MSG_QueueRemove( MSG_QUEUE_HANDLE handle );

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MSG_QUEUE_H__ */
