/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_com.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/18
  Description   :
  History       :
  1.Date        : 2008/01/18
    Author      : qushen
    Modification: Created file

  2.Date        : 2008/02/04
    Author      : qushen
    Modification: change file name to msg_com.c

******************************************************************************/
#include <sys/time.h>
#include <unistd.h>
#include "scp_common.h"
#include "msg_misc.h"
#include "msg_socket.h"
#include "msg_priorq.h"
#include "msg_taskmng.h"
#include "msg_rspmng.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#define __TEST_COM__

#define MSG_SET_FLAG(pMsg, flag) ((pMsg)->ulFlag) |= (flag)

extern MSG_QUEUE_HANDLE MSG_GetSvrHandle(void);

/*****************************************************************************
 Prototype       : MSG_SendAsyncMsg
 Description     : 发送异步消息
 Input           : asynMsgFlag: P2P, SUB, GROUP
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/2/3
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int MSG_SendAsyncMsg( VTOP_MSG_S_Block *pMsg, unsigned int asynMsgFlag )
{
    int ret;
    VTOP_MSG_S_InnerBlock innerMsg;
    MSG_QUEUE_HANDLE handle;
    VTOP_MSG_TASKNAME taskName;

#if 0
    ASSERT(LOG_MODULE, 0, asynMsgFlag == VTOP_MSG_FLG_P2P ||
                          asynMsgFlag == VTOP_MSG_FLG_SP  ||
                          asynMsgFlag == VTOP_MSG_FLG_GRP ||
                          asynMsgFlag == VTOP_MSG_FLG_BRDCST );
#endif
    ASSERT(LOG_MODULE, 0, asynMsgFlag == VTOP_MSG_FLG_P2P );

    handle = TASK_TAB_GetHandle(pMsg->rcvTaskId);
    if( handle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->sndTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    ret = getMsgID(&pMsg->msgID);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    memcpy(&innerMsg.msg, pMsg, sizeof(VTOP_MSG_S_Block));
    /*0.首先清空消息标志位*/
    innerMsg.msg.ulFlag = 0;
    /*1.设置同步异步标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_ASYNC);
    /*2.设置发送群组标志*/
    MSG_SET_FLAG(&innerMsg.msg, asynMsgFlag);

    /*如果用户配置的异步消息优先级非法，使用默认的优先级*/
    if ( innerMsg.msg.ulPrio > VTOP_MSG_PRIO_HIGH )
    {
        innerMsg.msg.ulPrio = VTOP_MSG_PRIO_DEFAULT;
    }

    ret = MSG_QueuePut(handle, &innerMsg);
    if ( ret !=  HI_SUCCESS )
    {
        TASK_TAB_Id2Name(pMsg->rcvTaskId, taskName);
        MSG_ERROR_RETURN(LOG_MODULE, 0, ret, "Send to task <%s> err!", taskName);
    }

    return ret;
}

/*****************************************************************************
 Prototype       : MSG_SendSyncMsg
 Description     : 发送同步消息
 Input           : None
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/2/3
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int MSG_SendSyncMsg( VTOP_MSG_S_Block *pMsg, VTOP_MSG_S_Block** ppMsgResp, int timeout )
{
    int ret;
    VTOP_MSG_S_InnerBlock innerMsg;
    unsigned char  *pBuf = NULL;
    VTOP_MSG_S_InnerBlock *pMsgResp;
    MSG_QUEUE_HANDLE MsgQueuehandle;
    MSG_RSPMNG_HANDLE RspHandle;
    struct timeval tm;
    HI_S32 startTime, curTime;

    MsgQueuehandle = TASK_TAB_GetHandle(pMsg->rcvTaskId);
    if( MsgQueuehandle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->sndTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    ret = getMsgID(&pMsg->msgID);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    memcpy(&innerMsg.msg, pMsg, sizeof(VTOP_MSG_S_Block));
    /*0.首先清空消息标志位*/
    innerMsg.msg.ulFlag = 0;
    /*1.设置同步异步标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_SYNC);
    /*2.设置发送群组标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_P2P);
    /*3.设置发送描述标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_DSC_SND);

    /*配置同步消息的固定优先级*/
    innerMsg.msg.ulPrio = VTOP_MSG_PRIO_SYN;

    ret = MSG_RSPMNG_New(&RspHandle);
    if ( ret != HI_SUCCESS )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "no synrsp mem!");
    }
    innerMsg.privData = RspHandle;

    ret = MSG_QueuePut(MsgQueuehandle, &innerMsg);
    if ( ret !=  HI_SUCCESS )
    {
        MSG_RSPMNG_Delete(RspHandle);
        VTOP_MSG_TASKNAME taskName;
        TASK_TAB_Id2Name(pMsg->rcvTaskId, taskName);
        MSG_ERROR_RETURN(LOG_MODULE, 0, ret, "Send to task <%s> err!", taskName);
    }

    gettimeofday(&tm, NULL);
    startTime = tm.tv_sec*1000 + tm.tv_usec/1000;
    do
    {
        if(HI_SUCCESS == MSG_RSPMNG_GetData(RspHandle, &pMsgResp))
        {
            pBuf = malloc(VTOP_MSG_MAX_MSG_SIZE + MSG_INNER_HEAD_LEN);
            if ( pBuf == NULL )
            {
                MSG_RSPMNG_Delete(RspHandle);
                MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "no mem!");
            }
            MSG_COPY2BUF(pBuf, pMsgResp);
            MSG_RSPMNG_Delete(RspHandle);

            pMsgResp = (VTOP_MSG_S_InnerBlock*)pBuf;
            pMsgResp->msg.pMsgBody = pBuf + MSG_INNER_HEAD_LEN;
            *ppMsgResp = &pMsgResp->msg;
            return HI_SUCCESS;
        }
        else
        {
            if ( timeout > 0 )
            {
                gettimeofday(&tm, NULL);
                curTime = tm.tv_sec*1000 + tm.tv_usec/1000;
                if ( curTime - startTime < timeout )
                {
                    usleep(20*1000);
                    continue;
                }
                else
                {
                    MSG_RSPMNG_Delete(RspHandle);
                    VTOP_MSG_TASKNAME taskName;
                    TASK_TAB_Id2Name(pMsg->rcvTaskId, taskName);
                    MSG_ERROR(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "Task <%s> timeout!", taskName);
                    return VTOP_MSG_ERR_RESPTIMEOUT;
                }
            }
            else if ( timeout < 0 )
            {
                usleep(20*1000);
                continue;
            }
            else //timeout == 0
            {
                MSG_RSPMNG_Delete(RspHandle);
                VTOP_MSG_TASKNAME taskName;
                TASK_TAB_Id2Name(pMsg->rcvTaskId, taskName);
                MSG_ERROR(LOG_MODULE, 0, VTOP_MSG_ERR_NOMEM, "Task <%s> timeout!", taskName);
                return VTOP_MSG_ERR_RESPTIMEOUT;
            }
        }
    } while(1);

    MSG_RSPMNG_Delete(RspHandle);
    return HI_FAILURE;
}

/*****************************************************************************
 Prototype       : MSG_SendSyncRsp
 Description     : 发送同步应答消息
 Input           : int fd
 Output          : None
 Return Value    : int
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/2/3
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int MSG_SendSyncRsp( int fd,
                VTOP_MSG_S_Block* pMsg, VTOP_MSG_S_Block *pMsgResp  )
{
    int ret;
    VTOP_MSG_S_InnerBlock innerMsg;

    //ASSERT(LOG_MODULE, 0, fd > 0);

    ret = getMsgID(&pMsgResp->msgID);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    memcpy(&innerMsg.msg, pMsgResp, sizeof(VTOP_MSG_S_Block));
    innerMsg.resp2id = pMsg->msgID;
    /*0.首先清空消息标志位*/
    innerMsg.msg.ulFlag = 0;
    /*1.设置同步异步标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_RESP);
    /*2.设置发送群组标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_P2P);

    /*配置同步应答消息的固定优先级*/
    innerMsg.msg.ulPrio = VTOP_MSG_PRIO_RSP;

    /*配置同步应答消息的固定发送和接收任务*/
    innerMsg.msg.sndTaskId = pMsg->rcvTaskId;
    innerMsg.msg.rcvTaskId = pMsg->sndTaskId;
    innerMsg.msg.msgType = pMsg->msgType;
    /*暂时只能在回调中调用*/
    innerMsg.privData = ((VTOP_MSG_S_InnerBlock*)pMsg)->privData;

    ret = MSG_RSPMNG_PutData(innerMsg.privData, &innerMsg);
    if ( ret != HI_SUCCESS )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_MSGSND, "MSG_RSPMNG_PutData error!");
    }
    return ret;
}

/*****************************************************************************
 Prototype       : MSG_PublishMsg
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2009/3/26
    Author       : qushen
    Modification : Created function

*****************************************************************************/
HI_S32 MSG_PublishMsg( VTOP_MSG_S_Block *pMsg )
{
    int ret;
    VTOP_MSG_S_InnerBlock innerMsg;
    MSG_QUEUE_HANDLE handle;
    VTOP_MSG_TASKNAME taskName;

    handle = MSG_GetSvrHandle();
    if( handle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->sndTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    ret = getMsgID(&pMsg->msgID);
    ASSERT_SUCCESS(LOG_MODULE, 0, ret);

    memcpy(&innerMsg.msg, pMsg, sizeof(VTOP_MSG_S_Block));
    /*0.首先清空消息标志位*/
    innerMsg.msg.ulFlag = 0;
    /*1.设置同步异步标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_ASYNC);
    /*2.设置发送群组标志*/
    MSG_SET_FLAG(&innerMsg.msg, VTOP_MSG_FLG_SP);

    /*如果用户配置的异步消息优先级非法，使用默认的优先级*/
    if ( innerMsg.msg.ulPrio > VTOP_MSG_PRIO_HIGH )
    {
        innerMsg.msg.ulPrio = VTOP_MSG_PRIO_DEFAULT;
    }

    ret = MSG_QueuePut(handle, &innerMsg);
    if ( ret !=  HI_SUCCESS )
    {
        TASK_TAB_Id2Name(pMsg->rcvTaskId, taskName);
        MSG_ERROR_RETURN(LOG_MODULE, 0, ret, "Send to task <%s> err!", taskName);
    }

    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
