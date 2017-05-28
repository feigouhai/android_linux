/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : msg_task.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/1/24
  Last Modified :
  Description   : ��Ϣ����API�ӿ�ʵ��
  Function List :
              VTOP_MSG_Register
  History       :
  1.Date        : 2008/1/24
    Author      : z42136
    Modification: Created file

******************************************************************************/
#include <unistd.h>
#include "common/hi_adp_mutex.h"

#include "common/msg_interface.h"
#include "msg_misc.h"

#include "msg_socket.h"

#include "msg_taskmng.h"
#include "msg_submng.h"

#include "msg_com.h"
#include "msg_log.h"
#include "msg_task.h"
#include "msg_queue.h"

#include "linux_list.h"

#define MSG_FLAG2MODE(flag, mode)           \
do{                                         \
    if ( VTOP_MSG_isP2PMsg(flag) )          \
    {                                       \
        mode = VTOP_MSG_STAT_MODE_P2P;      \
    }                                       \
    else if ( VTOP_MSG_isSPMsg(flag) )      \
    {                                       \
        mode = VTOP_MSG_STAT_MODE_SP;       \
    }                                       \
    else if ( VTOP_MSG_isGRPMsg(flag) )     \
    {                                       \
        mode = VTOP_MSG_STAT_MODE_GROUP;    \
    }                                       \
    else if ( VTOP_MSG_isBRDCSTMsg(flag) )  \
    {                                       \
        mode = VTOP_MSG_STAT_MODE_BROADCAST;\
    }                                       \
}while(0)

extern VTOP_MSG_TYPE TASK_TAB_GetMerge(VTOP_TASK_ID taskId);
extern HI_S32 MSG_QueueGetMerge( MSG_QUEUE_HANDLE handle, VTOP_MSG_TYPE msgtype, VTOP_MSG_S_InnerBlock **ppMsg );
extern int TASK_TAB_SetMerge(VTOP_TASK_ID taskId, VTOP_MSG_TYPE msgtype);

MSGTASK_DATA * g_TaskData = NULL;
int addTaskData(MSGTASK_DATA * pTaskData)
{
    MSGTASK_DATA * pNew = NULL;
    MSGTASK_DATA * pos = NULL;
    MSGTASK_DATA * n;

    if(g_TaskData == NULL)
    {
        // ��һ���ڵ�Ϊͷ��㣬���е�������Ч��ֻʹ��list
        g_TaskData = (MSGTASK_DATA *)malloc(sizeof(MSGTASK_DATA));
        ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, g_TaskData, NULL, VTOP_MSG_ERR_NOMEM);
        INIT_LIST_HEAD(&g_TaskData->list);
    }

    /* task�����Ƿ��Ѿ����� */
    list_for_each_entry_safe(pos, n, &g_TaskData->list, list)
    {
        if(pos->taskId == pTaskData->taskId)
        {
        MSG_INFO(LOG_MODULE, 0, "addTaskData: data already exist!");
        return VTOP_MSG_ERR_GENERAL;
        }
    }

    /* �������� */
    pNew = (MSGTASK_DATA *)malloc(sizeof(MSGTASK_DATA));
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pNew, NULL, VTOP_MSG_ERR_NOMEM);

    memcpy(pNew, pTaskData, sizeof(MSGTASK_DATA));
    INIT_LIST_HEAD(&pNew->list);

    list_add_tail(&pNew->list, &g_TaskData->list);

    return 0;
}

MSGTASK_DATA * getTaskData(VTOP_TASK_ID     taskId)
{
    MSGTASK_DATA * pos = NULL;
    MSGTASK_DATA * n;

    if(g_TaskData == NULL)
    {
        return NULL;
    }

    list_for_each_entry_safe(pos, n, &g_TaskData->list, list)
    {
        if(pos->taskId == taskId)
        {
        return pos;
        }
    }

    MSG_INFO(LOG_MODULE, 0, "delTaskData: task data not exist!");
    return NULL;
}

int delTaskData(VTOP_TASK_ID taskId)
{
    HI_BOOL bExist = HI_FALSE;
    MSGTASK_DATA * pos = NULL;
    MSGTASK_DATA * n;

    if(g_TaskData == NULL)
    {
        MSG_INFO(LOG_MODULE, 0, "delTaskData: task data not exist!");
        return VTOP_MSG_ERR_GENERAL;
    }

    list_for_each_entry_safe(pos, n, &g_TaskData->list, list)
    {
        if(pos->taskId == taskId)
        {
        list_del(&pos->list);
        free(pos);

        bExist = HI_TRUE;
        break;
        }
    }

    if(list_empty(&g_TaskData->list))
    {
        free(g_TaskData);
        g_TaskData = NULL;
    }

    if(bExist == HI_FALSE)
    {
        MSG_INFO(LOG_MODULE, 0, "delTaskData: task data not exist!");
        return VTOP_MSG_ERR_GENERAL;
    }
    else
    {
        return 0;
    }
}

char* VTOP_MSG_GetVersion(void)
{
    char *pVersion =  VTOP_MSG_VERSION;
    return pVersion;
}

char* VTOP_MSG_GetProcVersion(void)
{
    MSG_SHM_S *pMsgShm;
    pMsgShm = SHM_GetAddr();
    if ( NULL == pMsgShm)
    {
        MSG_ERROR(LOG_PROC, 0, VTOP_MSG_ERR_NOSHMMEM, "get shm err!");
        return NULL;
    }
    return pMsgShm->version;
}

HI_BOOL VTOP_MSG_IsVersionMatch(void)
{
    char * vtop_msg_version = VTOP_MSG_VERSION;
    char * vtop_msg_procversion = VTOP_MSG_GetProcVersion();
    if(NULL == vtop_msg_procversion)
        return HI_FALSE;
    return !strncmp(vtop_msg_version, vtop_msg_procversion, strlen(vtop_msg_version)+1);
}

/* VTOP_MSG_Registerʱ���pTaskNameΪ�գ�����ϵͳ����һ�����֣�
   ��������ȡϵͳ��������֡�
*/
int  VTOP_MSG_Register(
                const VTOP_MSG_TASKNAME taskName,
                PFUNC_VTOP_MSG_Handler pMsgHandler,
                PFUNC_VTOP_MSG_Log  pTaskLog,
                VTOP_TASK_ID * pTaskId)
{
    return VTOP_MSG_RegisterX(taskName, pMsgHandler, pTaskLog, pTaskId, TASK_OWNER_TASK);
}

int  VTOP_MSG_RegisterX(
        const VTOP_MSG_TASKNAME taskName,
        PFUNC_VTOP_MSG_Handler pMsgHandler,
        PFUNC_VTOP_MSG_Log  pTaskLog,
        VTOP_TASK_ID * pTaskId,
        TASK_OWNER_E owner)
{
    int ret;

    MSGTASK_DATA msgTaskData;
    VTOP_TASK_ID taskId;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, taskName, NULL, VTOP_MSG_ERR_NULLPTR);
    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pTaskId, NULL, VTOP_MSG_ERR_NULLPTR);

    if(strlen(taskName) >= VTOP_MSG_MAX_TASK_NAME_LEN)
    {
        MSG_INFO(LOG_MODULE, 0, "taskName length should be less than %d!", VTOP_MSG_MAX_TASK_NAME_LEN);
        return VTOP_MSG_ERR_INVALIDPARA;
    }

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    if ( !VTOP_MSG_IsVersionMatch() )
    {
        MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_VERSION,
            "Msg version match error, proc ver:%s, task ver:%s",
            VTOP_MSG_GetProcVersion(), VTOP_MSG_GetVersion());
    }

    if(TASK_TAB_IsTaskExist(taskName) == HI_TRUE)
    {
        return VTOP_MSG_ERR_TASKEXIST;
    }

    taskId = TASK_TAB_Add2(taskName, owner);
    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_TASKFULL);

    msgTaskData.taskId = taskId;
    msgTaskData.pMsgHandler = pMsgHandler;
    msgTaskData.pTaskLog = pTaskLog;
    msgTaskData.Sndfd = -1;
    msgTaskData.Rcvfd = -1;

#if 0
    /*Add by qushen, 2008-10-25*/
    ret = MSG_RecvConnect(&msgTaskData.Rcvfd, taskId);
    ASSERT_SUCCESS(LOG_TASK, taskId, ret);
#endif
    ret = addTaskData(&msgTaskData);
    if(ret < 0)
    {
        ret = TASK_TAB_Del(taskId);
        ASSERT_SUCCESS(LOG_TASK, VTOP_INVALID_TASK_ID, ret);

        /*Add by qushen, 2008-10-25*/
        //MSG_RecvDisconnect(msgTaskData.Rcvfd);

        return VTOP_MSG_ERR_GENERAL;
    }

    *pTaskId = taskId;
    return 0;
}

/*  ȥ��ʼ����Ϣģ��  */
int VTOP_MSG_UnRegister(VTOP_TASK_ID taskId)
{
    int ret;
    VTOP_MSG_TYPE * msgTypeList = NULL;
    unsigned int maxSub, maxTaskInSub;
    unsigned int index;

    MSGTASK_DATA *pTaskData = NULL;

    ASSERT_NOT_EQUAL(LOG_TASK, taskId, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    if(TASK_TAB_IsTaskIdExist(taskId) == HI_FALSE)
    {
        return VTOP_MSG_ERR_TASKNOTEXIST;
    }

    pTaskData = getTaskData(taskId);
    ASSERT_NOT_EQUAL(LOG_TASK, taskId, pTaskData, NULL, VTOP_MSG_ERR_DSNOTEXIST);

    if(pTaskData->Rcvfd != -1)
    {
        //MSG_RecvDisconnect(pTaskData->Rcvfd);
        pTaskData->Rcvfd = -1;
    }

    /* ɾ������Ϣ������ʹ�õ����еĶ����� */
    ret = SUB_TAB_GetMaxSubNum(&maxSub, &maxTaskInSub);
    ASSERT_SUCCESS(LOG_TASK, taskId, ret);

    msgTypeList = (VTOP_MSG_TYPE *)malloc(maxSub * sizeof(VTOP_MSG_TYPE));
    ASSERT_NOT_EQUAL(LOG_TASK, taskId, msgTypeList, NULL, VTOP_MSG_ERR_NOMEM);
    memset(msgTypeList,0,maxSub * sizeof(VTOP_MSG_TYPE));

    ret = SUB_TAB_Task2Sub(taskId, msgTypeList, &maxSub);
    //if(ret < 0)
    //{
        //free(msgTypeList);

        //return ret;
    //}

    for ( index = 0 ; index < maxSub; index++ )
    {
        ret = SUB_TAB_Del(msgTypeList[index], taskId);
        if(ret < 0)
        {
        free(msgTypeList);
        return ret;
        }
    }

    free(msgTypeList);

    ret = TASK_TAB_Del(taskId);
    ASSERT_NOTSMALL(LOG_TASK, taskId, ret, 0, VTOP_MSG_ERR_TASKNOTEXIST);

    ret = delTaskData(taskId);
    ASSERT_NOTSMALL(LOG_TASK, taskId, ret, 0, VTOP_MSG_ERR_GENERAL);

    return 0;
}

#if 0
int VTOP_MSG_GetTaskId(const VTOP_MSG_TASKNAME taskName, VTOP_TASK_ID * pTaskId)
{
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskName, NULL, VTOP_MSG_ERR_NULLPTR);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pTaskId, NULL, VTOP_MSG_ERR_NULLPTR);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    *pTaskId = TASK_TAB_Name2Id(taskName);
    return 0;
}
#else
VTOP_TASK_ID VTOP_MSG_GetTaskId(const VTOP_MSG_TASKNAME taskName)
{
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskName, NULL, VTOP_INVALID_TASK_ID);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_INVALID_TASK_ID;
    }

    return TASK_TAB_Name2Id(taskName);
}

#endif
int VTOP_MSG_GetTaskName(VTOP_TASK_ID taskId, VTOP_MSG_TASKNAME taskName)
{
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskName, NULL, VTOP_MSG_ERR_NULLPTR);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    return TASK_TAB_Id2Name(taskId, taskName);
}

/* �����û�����ȡ��Ϣ�ӿ�  2009-4-10, qushen*/
int VTOP_MSG_Recv(VTOP_TASK_ID taskId, VTOP_MSG_S_Block **ppMsg, int timeout)
{
    int ret;
    VTOP_MSG_S_InnerBlock *pInnerMsg = NULL;
    MSG_QUEUE_HANDLE handle;
    VTOP_MSG_TYPE msgtype;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    /*Add by qushen, 2008-10-25*/
    //ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pTaskData->Rcvfd, -1, VTOP_MSG_ERR_INVALIDPARA);

    handle = TASK_TAB_GetHandle(taskId);
    if( handle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, taskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    msgtype = TASK_TAB_GetMerge(taskId);
    if ( msgtype != VTOP_INVALID_MSG_TYPE )
    {
        ret = MSG_QueueGetMerge(handle, msgtype, &pInnerMsg);
    }
    else
    {
        ret = MSG_QueueGet(handle, &pInnerMsg);
    }
    if ( ret != HI_SUCCESS )
    {
        //printf("No data: taskId=%d, handle=%d\n", taskId, handle);
        return VTOP_MSG_ERR_MSGRCV;
    }

    *ppMsg = &(pInnerMsg->msg);
    return HI_SUCCESS;

}

/* �����������Ϣ�ͷŸ���Ϣ�ӿڣ�2009-4-10, qushen */
int VTOP_MSG_RecvFree(VTOP_MSG_S_Block *pMsg)
{
    VTOP_MSG_S_InnerBlock *pInnerMsg = (VTOP_MSG_S_InnerBlock *)pMsg;
    MSG_QUEUE_HANDLE handle;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pMsg, NULL, VTOP_MSG_ERR_INVALIDPARA);
    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pMsg->rcvTaskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    handle = TASK_TAB_GetHandle(pMsg->rcvTaskId);
    if( handle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->rcvTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    MSG_QueueFree(handle, pInnerMsg);

    return HI_SUCCESS;
}

int VTOP_MSG_GetAndDispatch(
        VTOP_TASK_ID taskId,
        int timeout)
{
    int ret;
    VTOP_MSG_S_InnerBlock *pInnerMsg = NULL;
    VTOP_MSG_TYPE msgtype;
    //VTOP_MSG_S_InnerBlock    *pMsg = NULL;
    MSG_QUEUE_HANDLE handle;

    MSGTASK_DATA * pTaskData;
    //VTOP_MSG_STAT_MODE  mode = VTOP_MSG_STAT_MODE_BUTT;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

#if 1
    pTaskData = getTaskData(taskId);
    if(pTaskData == NULL)
    {
        return VTOP_MSG_ERR_TASKNOTEXIST;
    }
#endif

    /*Add by qushen, 2008-10-25*/
    //ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pTaskData->Rcvfd, -1, VTOP_MSG_ERR_INVALIDPARA);

    handle = TASK_TAB_GetHandle(taskId);
    if( handle == MSG_QUEUE_INVALID_HANDLE)
    {
        MSG_ERROR_RETURN(LOG_TASK, taskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    msgtype = TASK_TAB_GetMerge(taskId);
    if ( msgtype != VTOP_INVALID_MSG_TYPE )
    {
        ret = MSG_QueueGetMerge(handle, msgtype, &pInnerMsg);
    }
    else
    {
        ret = MSG_QueueGet(handle, &pInnerMsg);
    }
    if ( ret != HI_SUCCESS )
    {
        //MSG_CONFIG_USLEEP((timeout+10)*1000);
        //printf("No data: taskId=%d, handle=%d\n", taskId, handle);
        //usleep(10000);
        return VTOP_MSG_ERR_TIMEOUT;
    }

    //MSG_FLAG2MODE(pInnerMsg->msg.ulFlag, mode);
    //ASSERT(LOG_TASK, taskId, mode != VTOP_MSG_STAT_MODE_BUTT);
    //TASK_TAB_StatItem(pInnerMsg->msg.rcvTaskId, mode, VTOP_MSG_STAT_SR_RCV);

#if 1
    //To do: ��ȡע�����Ϣ������
    if(pTaskData->pMsgHandler != NULL)
    {
        pTaskData->pMsgHandler(&pInnerMsg->msg);
    }
#endif

    //printf("########### taskId=%d, handle=%d\n", taskId, handle);

    MSG_QueueFree(handle, pInnerMsg);

    return HI_SUCCESS;
}

#define CHECK_MSG_BODY(pMsg, cur_task, size)    \
do{   \
    if((pMsg->pMsgBody == NULL) && (pMsg->ulMsgLen != 0))  \
    {   \
        MSG_ERROR_RETURN(LOG_TASK, cur_task, VTOP_MSG_ERR_INVALIDPARA, "pMsgBody can't be NULL when ulMsgLen=%d!", pMsg->ulMsgLen); \
    }   \
    if((pMsg->pMsgBody != NULL) && (pMsg->ulMsgLen == 0))  \
    {   \
        MSG_ERROR_RETURN(LOG_TASK, cur_task, VTOP_MSG_ERR_INVALIDPARA, "ulMsgLen can't be zero when pMsgBody isn't NULL!"); \
    }   \
    if ( pMsg->ulMsgLen > (size) ) \
    {   \
        MSG_ERROR_RETURN(LOG_TASK, cur_task, VTOP_MSG_ERR_MSGSIZE, "ulMsgLen is %u, more than %u!", pMsg->ulMsgLen, size); \
    }   \
}while(0)

/*---------------------- ��Ϣģʽ1: P2P: Point to Point ----------------------------*/
/* ͬ����Ϣ���� */
int VTOP_MSG_SynSend(
        VTOP_MSG_S_Block *pMsg,
        VTOP_MSG_S_Block** ppMsgResp,
        int timeout)
{
    int ret;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pMsg, NULL, VTOP_MSG_ERR_NULLPTR);
    ASSERT_NOT_EQUAL(LOG_TASK, pMsg->sndTaskId, ppMsgResp, NULL, VTOP_MSG_ERR_NULLPTR);

    CHECK_MSG_BODY(pMsg, pMsg->sndTaskId, VTOP_MSG_MAX_MSG_SIZE);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    if(!TASK_TAB_IsTaskIdExist(pMsg->rcvTaskId))
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->sndTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    ret = MSG_SendSyncMsg(pMsg, ppMsgResp, timeout);

    return ret;
}

/*
** �ýӿ���ͬ����Ϣ��Ӧ��ӿ�
** ͬ����Ϣ���������յ���Ϣ��������Ӧ����֮��Ӧ�õ��ñ��ӿ������߷���һ����Ӧ��Ϣ
** pMsg->rcvTaskId: VTOP_MSG_SendResp�ĵ�����
** msgId: pMsgResp��Ϣ�Ľ�����
** ͬ����Ϣ����msgid��������sndtaskid����Ч�ԣ�����������Ϣ���ղ���Ӧ��
*/
int VTOP_MSG_SendResp(
                VTOP_MSG_S_Block* pMsg,
                VTOP_MSG_S_Block *pMsgResp )
{
    MSGTASK_DATA * pTaskData;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pMsg, NULL, VTOP_MSG_ERR_NULLPTR);
    ASSERT_NOT_EQUAL(LOG_TASK, pMsg->rcvTaskId, pMsgResp, NULL, VTOP_MSG_ERR_NULLPTR);

    if ( !VTOP_MSG_isSyncMsg(pMsg->ulFlag) )
    {
        return VTOP_MSG_ERR_INVALIDPARA;
    }

    CHECK_MSG_BODY(pMsgResp, pMsg->rcvTaskId, VTOP_MSG_MAX_MSG_SIZE);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    pTaskData = getTaskData(pMsg->rcvTaskId);
    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pTaskData, NULL, VTOP_MSG_ERR_TASKNOTEXIST);

    return MSG_SendSyncRsp(pTaskData->Rcvfd, pMsg, pMsgResp);
}

/*
** ͬ����Ϣ���������յ���Ӧ��Ϣ֮��Ӧ�õ��ñ��ӿ��ͷŻ�Ӧ��Ϣ������
** pMsgResp->rcvTaskId: VTOP_MSG_SynRespFree�ĵ�����
** pMsgResp->sndTaskId: pMsgResp��Ϣ�ķ�����(�Ѿ�����)
*/
int VTOP_MSG_SynRespFree(VTOP_MSG_S_Block * pMsgResp)
{
    unsigned char *pBuf;

    if ( pMsgResp == NULL )
    {
        MSG_ERROR_RETURN(LOG_TASK, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_NULLPTR, "null pointer!\n");
    }

    pBuf = pMsgResp->pMsgBody - MSG_INNER_HEAD_LEN;
    free(pBuf);
    return 0;
}

int VTOP_MSG_SetMerge(VTOP_TASK_ID taskId, VTOP_MSG_TYPE msgtype)
{
    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }
    return TASK_TAB_SetMerge(taskId, msgtype);
}

/* �첽��Ϣ���� */
int VTOP_MSG_AsynSend(VTOP_MSG_S_Block *pMsg)
{
    int ret;

    ASSERT_NOT_EQUAL(LOG_TASK, VTOP_INVALID_TASK_ID, pMsg, NULL, VTOP_MSG_ERR_NULLPTR);

    CHECK_MSG_BODY(pMsg, pMsg->sndTaskId, VTOP_MSG_MAX_MSG_SIZE);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    if(!TASK_TAB_IsTaskIdExist(pMsg->rcvTaskId))
    {
        MSG_ERROR_RETURN(LOG_TASK, pMsg->sndTaskId, VTOP_MSG_ERR_TASKNOTEXIST, "receiver task not exist!");
    }

    ret = MSG_SendAsyncMsg(pMsg, VTOP_MSG_FLG_P2P);

    return ret;
}

/*---------------------- ��Ϣģʽ2: Pub/Sub: Publish/Subscribe ----------------------------*/
// ����ĳ�����͵���Ϣ
int VTOP_MSG_Subscribe(VTOP_TASK_ID taskId, VTOP_MSG_TYPE msgType)
{
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, msgType, VTOP_INVALID_MSG_TYPE, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    return SUB_TAB_Add(msgType, taskId);
}

// ȡ��ĳ�����͵���Ϣ����
int VTOP_MSG_UnSubscribe(VTOP_TASK_ID taskId, VTOP_MSG_TYPE msgType)
{
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, taskId, VTOP_INVALID_TASK_ID, VTOP_MSG_ERR_INVALIDPARA);
    ASSERT_NOT_EQUAL(LOG_MODULE, 0, msgType, VTOP_INVALID_MSG_TYPE, VTOP_MSG_ERR_INVALIDPARA);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    return SUB_TAB_Del(msgType, taskId);
}

// ����ĳ�����͵���Ϣ,��Ϣ������pMsg->ulMsgType�и���
int VTOP_MSG_Publish(VTOP_MSG_S_Block *pMsg)
{
    int ret;
    int index;

    ASSERT_NOT_EQUAL(LOG_MODULE, 0, pMsg, NULL, VTOP_MSG_ERR_NULLPTR);

    if(SHM_Exist() == HI_FALSE)
    {
        return VTOP_MSG_ERR_PROCNOTEXIST;
    }

    if(!SUB_TAB_IsExist(pMsg->msgType, (unsigned int *)&index))
    {
        //MSG_ERROR_RETURN(LOG_MODULE, 0, VTOP_MSG_ERR_SUBNOTEXIST, "noboy sub msg type %d!", pMsg->msgType);
        return VTOP_MSG_ERR_SUBNOTEXIST;
    }

    //ret = MSG_SendAsyncMsg(pMsg, VTOP_MSG_FLG_SP);
    ret = MSG_PublishMsg(pMsg);

    return ret;
}

// ���е�subscriber(������)ͨ�� VTOP_MSG_GetAndDispatch ����publisher��������Ϣ.
