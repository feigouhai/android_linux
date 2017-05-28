/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_proc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/01/18
  Description   :
  History       :
  1.Date        : 2008/01/18
    Author      : qushen
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "scp_common.h"
#include "common/hi_adp_thread.h"

#include "msg_socket.h"
#include "msg_com.h"
#include "msg_buffer.h"
#include "msg_connq.h"
#include "msg_syncq.h"
#include "msg_priorq.h"
#include "msg_taskmng.h"
#include "msg_submng.h"
#include "msg_rspmng.h"
#include "msg_log.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

//#define __TEST_PROC__
//#define __PROC_SHOW_STATE__

/*消息服务进程内部使用的消息退出类型*/
#define MSG_TYPE_MSGPROC_EXIT  0xFFFFFFFE
#define MSG_TYPE_MSGPROC_STATS 0xFFFFFFFD

static HI_BOOL  s_bStopMsgProc = HI_FALSE;
static MSG_QUEUE_HANDLE s_msgSvrQueueHandle = MSG_QUEUE_INVALID_HANDLE;

MSG_QUEUE_HANDLE MSG_GetSvrHandle(void)
{
    return s_msgSvrQueueHandle;
}

static void dispatch_SP(VTOP_MSG_S_InnerBlock *pMsg, VTOP_TASK_ID *taskList)
{
    int      ret, i;
    VTOP_MSG_TYPE   msgType;
    size_t     taskNum = 0;
    static VTOP_MSG_TASKNAME taskName;
    MSG_QUEUE_HANDLE handle;

    msgType = pMsg->msg.msgType;
    ret = SUB_TAB_Sub2Task(msgType, taskList, &taskNum);
    if ( ret != 0 )
    {
        MSG_ERROR(LOG_PROC, 0, ret, "SUB_TAB_Sub2Task err");
        return;
    }

    for ( i = 0 ; i < taskNum ; i++ )
    {
        pMsg->msg.rcvTaskId = taskList[i];
        handle = TASK_TAB_GetHandle(pMsg->msg.rcvTaskId);
        if( handle == MSG_QUEUE_INVALID_HANDLE)
        {
            MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
            continue;
        }

        ret = MSG_QueuePut(handle, pMsg);
        if ( ret != 0 )
        {
            TASK_TAB_Id2Name(pMsg->msg.rcvTaskId, taskName);
            MSG_ERROR(LOG_PROC, 0, ret, "Sub-Publish to Task \"%s\" err", taskName);
        }
    }
}

/*****************************************************************************
 Prototype       : MSG_PROC_RcvAndDispatch
 Description     : 接收从消息任务发来的消息
 Input           : arg  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/2/3
    Author       : qushen
    Modification : Created function

*****************************************************************************/
void* MSG_PROC_RcvAndDispatch(void *arg)
{
    int ret;
    VTOP_MSG_S_InnerBlock *pInnerMsg = NULL;
    VTOP_TASK_ID *taskList;
    MSG_SHM_S * pMsgShm = NULL;

    pMsgShm = SHM_GetAddr();
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsgShm, NULL, 0);

    taskList = malloc(pMsgShm->maxTaskNum * sizeof(VTOP_TASK_ID));
    if ( taskList == NULL )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_NOMEM, "no mem!");
        return NULL;
    }
    while(!s_bStopMsgProc)

    {
        ret = MSG_QueueGet(s_msgSvrQueueHandle, &pInnerMsg);
        if ( ret != 0 )
        {
            usleep(50*1000);
            continue;
        }
        ASSERT(LOG_PROC, 0,
            VTOP_MSG_isASyncMsg(pInnerMsg->msg.ulFlag) && VTOP_MSG_isSPMsg(pInnerMsg->msg.ulFlag) );

        dispatch_SP(pInnerMsg, taskList);

        MSG_QueueFree(s_msgSvrQueueHandle, pInnerMsg);
    }

    MSG_INFO(LOG_PROC, 0, "MSGProc: quit RECEIVE & DISPATCH LOOP!!");
    free(taskList);

    return NULL;
}

/*****************************************************************************
 Prototype       : VTOP_MSG_Main
 Description     : 启动消息服务进程
 Input           : None
 Output          : None
 Return Value    : HI_S32
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/18
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int VTOP_MSG_Main(VTOP_MSG_S_Config * pMsgConfig, VTOP_MSG_S_ops * pMsg_ops)
{
    int ret;
    HI_Pthread_T thd_dispatch;
#ifdef __PROC_SHOW_STATE__
    HI_Pthread_T thd_debug;
#endif
    VTOP_MSG_S_Config defaultMsgConfig;
    VTOP_MSG_S_ops defaultMsg_ops;

    VTOP_MSG_S_Config * pMsgConfig_used = NULL;
    VTOP_MSG_S_ops * pMsg_ops_used = NULL;
    MSG_SHM_S *pMsgShm = NULL;

    if(SHM_Exist() == HI_TRUE)
    {
        MSG_INFO(LOG_PROC, 0, "Msg proc already exist, please check!");
        return HI_SUCCESS;
    }

    pMsgConfig_used = pMsgConfig;
    if(pMsgConfig_used == NULL)
    {
        memset(&defaultMsgConfig, 0, sizeof(defaultMsgConfig));
        pMsgConfig_used = &defaultMsgConfig;
    }

    pMsg_ops_used = pMsg_ops;
    if(pMsg_ops_used == NULL)
    {
        memset(&defaultMsg_ops, 0, sizeof(defaultMsg_ops));
        pMsg_ops_used = &defaultMsg_ops;
    }

    ret = MSG_LOG_Init(pMsg_ops_used->pMsgLog);
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_LOG);

    /* init public share memory tab */
    ret = SHM_Init(pMsgConfig_used);
    if(ret < 0)
    {
        MSG_INFO(LOG_PROC, 0, "Msg proc already exist, please check!");
        //ret = VTOP_MSG_ERR_PROCEXIST;

        goto deinit_SHM;
    }

    /*init tab*/
    ret = TASK_TAB_Init();
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_TASK_TAB);

    ret = SUB_TAB_Init();
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_SUB_TAB);

    //ret = PRIORQ_Init(pMsgConfig_used->blockNum);
    //ASSERT_GOTO(LOG_PROC, 0, ret, deinit_PRIORQ);

    /*conn-sync缓冲的个数 = 最大消息缓冲个数*/
    //ret = CONNQ_Sync_Init(PRIORQ_GetMbNum());
    //ASSERT_GOTO(LOG_PROC, 0, ret, deinit_CONNQ_Sync);

    ret = MSG_RSPMNG_Init();
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_RSPMNG);

    /*conn-task缓冲的个数 = 最大消息任务数*/
    pMsgShm = SHM_GetAddr();
    if ( pMsgShm == NULL )
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_NOSHMMEM, "get shm err!");
        goto deinit_CONNQ_Sync;
    }
    /*init msg queue and buffer*/
    //ret = CONNQ_Task_Init(pMsgShm->maxTaskNum);
    //ASSERT_GOTO(LOG_PROC, 0, ret, deinit_CONNQ_Task);

    ret = MSG_QueueInit("MSGSVR", 32*1024, &s_msgSvrQueueHandle);
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_CONNQ_Task);

    /*启动消息服务处理任务*/
    s_bStopMsgProc = HI_FALSE;
    ret = HI_PthreadCreate (&thd_dispatch, NULL, MSG_PROC_RcvAndDispatch, NULL);
    ASSERT_GOTO(LOG_PROC, 0, ret, deinit_CONNQ_Task);

    MSG_INFO(LOG_PROC, 0, "MSGProc %s Start success!!", pMsgShm->version);

    /*主消息接收循环*/
    //ret = MSG_PROC_RcvAndDispatch(NULL);
    return HI_SUCCESS;

    //ret = HI_PthreadJoin(thd_dispatch, NULL);

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: begin to quit!!");
    #endif

deinit_CONNQ_Task:
    //CONNQ_Task_Deinit();

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: CONNQ-Task deinit done!!");
    #endif

deinit_RSPMNG:
    MSG_RSPMNG_Deinit();
    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: RSPMNG deinit done!!");
    #endif

deinit_CONNQ_Sync:
    //CONNQ_Sync_Deinit();

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: TRACE_TAB deinit done!!");
    #endif

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: GROUP_TAB deinit done!!");
    #endif

deinit_SUB_TAB:
    SUB_TAB_DeInit();

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: SUB_TAB deinit done!!");
    #endif

deinit_TASK_TAB:
    TASK_TAB_DeInit();

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: TASK_TAB deinit done!!");
    #endif

deinit_SHM:
    SHM_DeInit();

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: SHM deinit done!!");
    #endif

deinit_LOG:
    // nothing to do

    #ifdef __TEST_PROC__
    MSG_INFO(LOG_PROC, 0, "MSGProc: LOG deinit done!!");
    #endif

    MSG_INFO(LOG_PROC, 0, "MSGProc: quit ALL!!");

    return ret;
}

/*****************************************************************************
 Prototype       : VTOP_MSG_Destroy
 Description     : destroy msg proc
 Input           : bExitByForce  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/1/25
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int VTOP_MSG_Destroy(HI_BOOL bExitByForce)
{
#if 0
    int ret;
    VTOP_MSG_S_Block msg;

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    s_bStopMsgProc = HI_TRUE;
    HI_PthreadJoin(, NULL);

    GROUP_TAB_DeInit();
    SUB_TAB_DeInit();
    TASK_TAB_DeInit();

    SHM_DeInit();
#endif

    // TODO : 待实现
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
